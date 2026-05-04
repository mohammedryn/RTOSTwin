"""
main.py
-------
Entry point for the RTOSTwin Python Bridge.

Wires together the decoder, state manager, Prometheus exporter, OTLP
exporter, and OOM analyzer into a single read loop. Reads from a serial
port (real MCU) or stdin (piped from mock_device.py for testing).

Usage — with real hardware:
    python bridge/main.py --port COM3

Usage — with mock device (no hardware):
    python bridge/mock_device.py --mode leak | python bridge/main.py --port stdin

Usage — full stack (Docker):
    docker-compose up        # Starts Prometheus + Grafana
    python bridge/main.py    # Bridge feeds Prometheus

Ownership: VNV
"""

import argparse
import logging
import sys
import time
from typing import Optional

import serial  # pip install pyserial

from config import (
    DEFAULT_BAUD_RATE,
    DEFAULT_SERIAL_PORT,
    OOM_MIN_R_SQUARED,
    OOM_ROLLING_MIN_THRESHOLD,
    OOM_WINDOW_SIZE,
    PROMETHEUS_PORT,
    TOTAL_HEAP_BYTES,
)
from decoder import PacketDecoder
from device_registry import DeviceRegistry
from oom_analyzer import OOMAnalyzer
from prometheus_exporter import PrometheusExporter
from otlp_exporter import OTLPExporter

# ---------------------------------------------------------------------------
# Logging setup
# ---------------------------------------------------------------------------
logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(name)s: %(message)s",
    datefmt="%H:%M:%S",
)
logger = logging.getLogger("bridge.main")


# ---------------------------------------------------------------------------
# Transport helpers
# ---------------------------------------------------------------------------

def _open_transport(port: str, baud: int):
    """
    Open the byte source. Returns a file-like object with a .read(n) method.

    'stdin' is a special keyword that reads from standard input — used when
    piping mock_device.py output directly into this bridge for testing.

    Args:
        port: Serial port name (e.g. 'COM3', '/dev/ttyUSB0') or 'stdin'.
        baud: Baud rate (ignored for stdin).

    Returns:
        A serial.Serial object or sys.stdin.buffer.
    """
    if port.lower() == "stdin":
        logger.info("[main] Reading from stdin (mock device mode)")
        return sys.stdin.buffer

    try:
        ser = serial.Serial(port, baud, timeout=0.5)
        logger.info("[main] Opened serial port %s @ %d baud", port, baud)
        return ser
    except serial.SerialException as exc:
        logger.error("[main] Cannot open port %s: %s", port, exc)
        sys.exit(1)


# ---------------------------------------------------------------------------
# Main bridge loop
# ---------------------------------------------------------------------------

def run_bridge(port: str, baud: int, device_id: Optional[str] = None) -> None:
    """
    Main loop: read bytes → decode → update state → push metrics.

    Args:
        port:      Serial port name or 'stdin'.
        baud:      Serial baud rate.
        device_id: Override the device identifier. Defaults to the port name.
    """
    dev_id = device_id or port

    # ---- initialise components ----
    transport   = _open_transport(port, baud)
    decoder     = PacketDecoder()
    registry    = DeviceRegistry()
    prom        = PrometheusExporter(port=PROMETHEUS_PORT)
    otlp        = OTLPExporter()
    oom         = OOMAnalyzer(
                    window_size=OOM_WINDOW_SIZE,
                    min_r_squared=OOM_MIN_R_SQUARED,
                    rolling_min_threshold=OOM_ROLLING_MIN_THRESHOLD,
                    total_heap_bytes=TOTAL_HEAP_BYTES,
                  )

    # ---- start Prometheus HTTP server ----
    prom.start()
    logger.info("[main] Prometheus metrics at http://localhost:%d/metrics", PROMETHEUS_PORT)
    logger.info("[main] Bridge running. Press Ctrl-C to stop.")

    # ---- packet counters for logging ----
    total_packets   = 0
    last_log_time   = time.monotonic()
    LOG_INTERVAL_S  = 10.0

    try:
        while True:
            # 1. Read a chunk of raw bytes from the transport
            raw = transport.read(256)
            if not raw:
                time.sleep(0.01)  # Avoid busy-loop when no data arrives
                continue

            # 2. Feed bytes into the state machine decoder
            packets = decoder.feed_bytes(raw)

            for packet in packets:
                total_packets += 1

                # 3. Update the device state (keyframe or delta)
                manager = registry.get_or_create(dev_id)

                # Record sequence gaps as drops
                if decoder.sequence_gap_count > manager.current_state.drop_count:
                    manager.record_drop()

                state = manager.update(packet)

                # 4. Feed heap sample into OOM analyzer
                now_s = time.monotonic()
                oom.add_sample(timestamp_s=now_s, heap_free_bytes=state.heap_free_bytes)
                oom_seconds = oom.get_projection_seconds()

                if oom_seconds > 0:
                    logger.warning(
                        "[oom] LEAK DETECTED on %s — OOM in %.1f seconds",
                        dev_id, oom_seconds,
                    )

                # 5. Push to Prometheus (pull endpoint — always fresh)
                prom.update_metrics(dev_id, state, oom_seconds)

                # 6. Update OTLP cache (SDK exports on its own timer)
                # otlp.export_metrics(dev_id, state, oom_seconds)

            # 7. Periodic status log
            now = time.monotonic()
            if now - last_log_time >= LOG_INTERVAL_S:
                logger.info(
                    "[main] %d packets received | drops=%d | seq_gaps=%d",
                    total_packets,
                    decoder.drop_count,
                    decoder.sequence_gap_count,
                )
                last_log_time = now

    except KeyboardInterrupt:
        logger.info("[main] Stopped by user.")
    except Exception as exc:
        logger.exception("[main] Unhandled error: %s", exc)
        raise


# ---------------------------------------------------------------------------
# CLI entry point
# ---------------------------------------------------------------------------

def main() -> None:
    parser = argparse.ArgumentParser(
        description="RTOSTwin Python Bridge — decodes RTOS telemetry and feeds Prometheus + OTLP"
    )
    parser.add_argument(
        "--port",
        default=DEFAULT_SERIAL_PORT,
        help='Serial port (e.g. COM3 or /dev/ttyUSB0) or "stdin" for mock device',
    )
    parser.add_argument(
        "--baud",
        type=int,
        default=DEFAULT_BAUD_RATE,
        help="Baud rate (default 115200)",
    )
    parser.add_argument(
        "--device-id",
        default=None,
        help="Override device identifier (default: port name)",
    )
    parser.add_argument(
        "--log-level",
        default="INFO",
        choices=["DEBUG", "INFO", "WARNING", "ERROR"],
        help="Logging verbosity",
    )

    args = parser.parse_args()
    logging.getLogger().setLevel(args.log_level)

    run_bridge(port=args.port, baud=args.baud, device_id=args.device_id)


if __name__ == "__main__":
    main()
