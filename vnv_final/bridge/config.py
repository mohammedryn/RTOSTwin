"""
config.py
---------
Global configuration and defaults for the RTOSTwin Bridge.

All values can be overridden via CLI arguments (see main.py) or
environment variables where noted. This file is the single source
of truth for every tunable parameter in the bridge.
"""

import os

# ---------------------------------------------------------------------------
# Serial / Transport Settings
# ---------------------------------------------------------------------------
DEFAULT_SERIAL_PORT: str = "COM3"          # Override with --port
DEFAULT_BAUD_RATE:   int = 115200          # Must match agent transport_init()

# ---------------------------------------------------------------------------
# Prometheus Exporter Settings
# ---------------------------------------------------------------------------
PROMETHEUS_PORT: int = 8000                # http://localhost:8000/metrics

# ---------------------------------------------------------------------------
# OTLP Exporter Settings
# ---------------------------------------------------------------------------
# Set OTEL_EXPORTER_OTLP_ENDPOINT env variable to point at Grafana Cloud,
# a self-hosted OTel Collector, or any OTLP-compatible backend.
OTLP_ENDPOINT: str = os.getenv(
    "OTEL_EXPORTER_OTLP_ENDPOINT",
    "http://localhost:4318/v1/metrics"
)
OTLP_EXPORT_INTERVAL_MS: int = 30_000     # Push every 30 seconds
OTLP_ENABLED: bool = os.getenv("RTOSTWIN_ENABLE_OTLP", "0") == "1"

# ---------------------------------------------------------------------------
# OOM Analyzer Settings
# ---------------------------------------------------------------------------
OOM_WINDOW_SIZE:          int   = 600      # Samples in sliding window (60s @ 10 Hz)
OOM_MIN_R_SQUARED:        float = 0.7     # Minimum R² to confirm linear leak
OOM_ROLLING_MIN_THRESHOLD: float = 0.10  # 10% heap drop triggers rolling-min alert
TOTAL_HEAP_BYTES:         int   = 131_072  # 128 KB — adjust for your MCU

# ---------------------------------------------------------------------------
# Protocol Settings (must mirror wire_format.h)
# ---------------------------------------------------------------------------
PROTOCOL_VERSION:    int = 1
KEYFRAME_INTERVAL:   int = 50             # Every 50 packets = forced keyframe

# ---------------------------------------------------------------------------
# Device Timeout
# ---------------------------------------------------------------------------
DEVICE_OFFLINE_TIMEOUT_S: int = 30        # Seconds before "device offline" alert
