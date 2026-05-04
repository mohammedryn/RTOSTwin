"""
mock_device.py
--------------
Simulates an RTOSTwin MCU agent for hardware-free testing of the bridge.

What it does:
  1. Generates dummy FreeRTOS task data and memory snapshots.
  2. Encodes them into the binary wire format (CRC, framing).
  3. Outputs the binary packet stream to stdout at 10 Hz.

Modes (select with --mode):
  normal     Steady-state system: 4 tasks, stable heap, ~20% CPU.
  leak       Same as normal but heap shrinks by 10 bytes/sec (OOM trigger).
  saturated  All tasks running, 95% CPU, minimal stack remaining.

Usage:
  python bridge/mock_device.py --mode normal    | python bridge/main.py --port stdin
  python bridge/mock_device.py --mode leak      | python bridge/main.py --port stdin
  python bridge/mock_device.py --mode saturated | python bridge/main.py --port stdin

Note:
  All logging goes to stderr so stdout stays clean for binary piping.
"""

import argparse
import struct
import sys
import time
from typing import List, Tuple

# ---------------------------------------------------------------------------
# Wire format constants — mirror of agent/core/wire_format.h
# ---------------------------------------------------------------------------
SYNC_0          = 0xAA
SYNC_1          = 0x55
VERSION         = 0x01
TYPE_DELTA      = 0x01
TYPE_KEYFRAME   = 0x02
HEADER_SIZE     = 12        # Bytes before payload
CRC_SIZE        = 2         # Bytes after payload
MAX_TASKS       = 16
TASK_NAME_LEN   = 16        # Fixed-width name field in the snapshot struct

# ---------------------------------------------------------------------------
# CRC-16-CCITT — must produce identical output to the C implementation.
# Test vector: crc16_ccitt(b"123456789") == 0x29B1
# ---------------------------------------------------------------------------
def crc16_ccitt(data: bytes) -> int:
    """Calculate CRC-16-CCITT (poly=0x1021, init=0xFFFF, no reflection)."""
    crc = 0xFFFF
    for byte in data:
        crc ^= (byte << 8)
        for _ in range(8):
            if crc & 0x8000:
                crc = ((crc << 1) ^ 0x1021)
            else:
                crc = (crc << 1)
            crc &= 0xFFFF
    return crc


# ---------------------------------------------------------------------------
# Task definition helpers
# ---------------------------------------------------------------------------
def _encode_task(name: str, state: int, priority: int,
                 stack_hwm_words: int, runtime_ticks: int) -> bytes:
    """
    Encode a single task into its 24-byte binary representation.

    Layout (matches task_snapshot_t in snapshot.h):
      name[16]          — null-padded ASCII string
      state (1 byte)    — eTaskState: 0=Running 1=Ready 2=Blocked 3=Suspended
      priority (1 byte) — uxCurrentPriority
      stack_hwm (2 B)   — uxTaskGetStackHighWaterMark() result (words)
      runtime (4 bytes) — ulRunTimeCounter ticks
    Total: 16 + 1 + 1 + 2 + 4 = 24 bytes
    """
    name_bytes = name.encode("ascii")[:TASK_NAME_LEN].ljust(TASK_NAME_LEN, b"\x00")
    return name_bytes + struct.pack("<BBHi", state, priority, stack_hwm_words, runtime_ticks)


def _encode_memory(heap_free: int, heap_min_ever: int, cpu_pct: int) -> bytes:
    """
    Encode the memory/CPU snapshot into its 9-byte binary representation.

    Layout (matches memory_snapshot_t in snapshot.h):
      heap_free_bytes   (4 bytes) — xPortGetFreeHeapSize()
      heap_min_ever     (4 bytes) — xPortGetMinimumEverFreeHeapSize()
      cpu_utilization   (1 byte)  — 0-100 percent
    Total: 4 + 4 + 1 = 9 bytes
    """
    return struct.pack("<IIB", heap_free, heap_min_ever, cpu_pct)


# ---------------------------------------------------------------------------
# Full snapshot payload builder (keyframe — full_snapshot_t serialised)
# ---------------------------------------------------------------------------
def _build_keyframe_payload(seq: int, tasks: List[Tuple], heap_free: int,
                             heap_min: int, cpu_pct: int) -> bytes:
    """
    Serialise a full_snapshot_t into bytes.

    Header:
      sequence_num   (2 bytes)
      timestamp_tick (4 bytes)  — simulated tick count
      task_count     (1 byte)
    Then: task_count × 24-byte task records
    Then: remaining (MAX_TASKS - task_count) × 24-byte zero padding
    Then: 9-byte memory block
    """
    timestamp = int(time.monotonic() * 1000) & 0xFFFF_FFFF
    task_count = len(tasks)

    header = struct.pack("<HIB", seq, timestamp, task_count)

    task_bytes = b""
    for t in tasks:
        task_bytes += _encode_task(*t)

    # Pad up to MAX_TASKS tasks so the struct size is always the same
    padding_tasks = MAX_TASKS - task_count
    task_bytes += b"\x00" * (padding_tasks * 24)

    mem_bytes = _encode_memory(heap_free, heap_min, cpu_pct)

    return header + task_bytes + mem_bytes


# ---------------------------------------------------------------------------
# Packet framer (mirrors framer.c frame_packet())
# ---------------------------------------------------------------------------
def _frame_packet(payload: bytes, packet_type: int, seq: int,
                  timestamp_ticks: int) -> bytes:
    """
    Wrap a payload in the full RTOSTwin wire-format packet.

    Layout:
      SYNC_0 (1)  SYNC_1 (1)  VERSION (1)  TYPE (1)
      SEQ_NUM (2 LE)  TIMESTAMP (4 LE)  LENGTH (2 LE)
      PAYLOAD (N)
      CRC_16 (2 LE) — over VERSION..PAYLOAD
    """
    payload_len = len(payload)

    header = struct.pack("<BBHiH",
                         VERSION,
                         packet_type,
                         seq,
                         timestamp_ticks,
                         payload_len)

    crc_val  = crc16_ccitt(header + payload)
    crc_bytes = struct.pack("<H", crc_val)

    return bytes([SYNC_0, SYNC_1]) + header + payload + crc_bytes


# ---------------------------------------------------------------------------
# Mode: normal — stable 4-task system, ~20% CPU, steady heap
# ---------------------------------------------------------------------------
def _tasks_normal() -> List[Tuple]:
    return [
        # (name,      state, priority, stack_hwm_words, runtime_ticks)
        ("SensorTask",  2,   3,  512,   10_000),   # Blocked (waiting for sensor)
        ("CommsTask",   2,   2,  384,    8_000),   # Blocked (waiting for UART)
        ("ProcTask",    1,   2,  256,   15_000),   # Ready
        ("IDLE",        0,   0,  128,  960_000),   # Running
    ]


# ---------------------------------------------------------------------------
# Mode: saturated — all tasks running, high CPU, low stack margins
# ---------------------------------------------------------------------------
def _tasks_saturated() -> List[Tuple]:
    return [
        ("SensorTask",  0,  5,   64,  200_000),   # Running (high priority)
        ("CommsTask",   1,  4,   80,  180_000),   # Ready
        ("ProcTask",    1,  3,   96,  150_000),   # Ready
        ("IDLE",        1,  0,  128,   10_000),   # Ready (barely runs)
    ]


# ---------------------------------------------------------------------------
# Main entry point
# ---------------------------------------------------------------------------
def main() -> None:
    parser = argparse.ArgumentParser(
        description="RTOSTwin mock MCU — streams binary telemetry to stdout."
    )
    parser.add_argument(
        "--mode",
        choices=["normal", "leak", "saturated"],
        default="normal",
        help=(
            "normal     : stable 4-task system, ~20%% CPU, no heap drift.\n"
            "leak       : same as normal but heap drops 10 bytes/sec → OOM.\n"
            "saturated  : all tasks running, 95%% CPU, low stack margins.\n"
        ),
    )
    args = parser.parse_args()
    mode = args.mode

    print(f"[mock_device] mode={mode}  — streaming to stdout at 10 Hz",
          file=sys.stderr)
    print("[mock_device] Pipe this to: python bridge/main.py --port stdin",
          file=sys.stderr)

    seq           = 0
    total_heap    = 131_072          # 128 KB — matches OOM analyzer default
    heap_free     = total_heap       # Start with a full heap
    heap_min_ever = total_heap
    runtime_base  = 0

    try:
        while True:
            # ---- choose task list for this mode ----
            if mode in ("normal", "leak"):
                tasks = _tasks_normal()
            else:  # saturated
                tasks = _tasks_saturated()

            # ---- apply mode-specific heap drift ----
            if mode == "leak":
                heap_free     = max(heap_free - 10, 0)   # 10 bytes/sec leak
                heap_min_ever = min(heap_min_ever, heap_free)

            # ---- CPU percentage ----
            cpu_pct = 95 if mode == "saturated" else 20

            # ---- build and frame packet ----
            # Every WF_KEYFRAME_INTERVAL (50) packets we send a keyframe;
            # for simplicity the mock always sends keyframes (safe for bridge).
            packet_type = TYPE_KEYFRAME

            timestamp_ticks = int(time.monotonic() * 1000) & 0xFFFF_FFFF

            payload = _build_keyframe_payload(
                seq=seq,
                tasks=tasks,
                heap_free=heap_free,
                heap_min=heap_min_ever,
                cpu_pct=cpu_pct,
            )

            packet = _frame_packet(payload, packet_type, seq, timestamp_ticks)

            # ---- write binary to stdout ----
            sys.stdout.buffer.write(packet)
            sys.stdout.buffer.flush()

            # ---- advance counters ----
            seq           = (seq + 1) & 0xFFFF   # Wrap at 65535
            runtime_base += 100

            time.sleep(0.1)  # 10 Hz

    except KeyboardInterrupt:
        print("\n[mock_device] Stopped.", file=sys.stderr)


if __name__ == "__main__":
    main()
