"""
mock_device.py
--------------
Simulates an RTOSTwin MCU agent for hardware-free testing of the bridge.
"""

import argparse
import struct
import sys
import time
from typing import List, Tuple

SYNC_0 = 0xAA
SYNC_1 = 0x55
VERSION = 0x01
TYPE_KEYFRAME = 0x02
MAX_TASKS = 16
TASK_NAME_LEN = 16


def crc16_ccitt(data: bytes) -> int:
    crc = 0xFFFF
    for byte in data:
        crc ^= byte << 8
        for _ in range(8):
            if crc & 0x8000:
                crc = (crc << 1) ^ 0x1021
            else:
                crc = crc << 1
            crc &= 0xFFFF
    return crc


def _encode_task(
    name: str,
    state: int,
    priority: int,
    stack_hwm_words: int,
    runtime_ticks: int,
) -> bytes:
    name_bytes = bytearray(TASK_NAME_LEN)
    encoded_name = name.encode("ascii")[: TASK_NAME_LEN - 1]
    name_bytes[: len(encoded_name)] = encoded_name
    return bytes(name_bytes) + struct.pack("<BBHI", state, priority, stack_hwm_words, runtime_ticks)


def _encode_memory(heap_free: int, heap_min_ever: int, cpu_pct: int) -> bytes:
    return struct.pack("<IIB", heap_free, heap_min_ever, cpu_pct)


def _build_keyframe_payload(
    seq: int,
    tasks: List[Tuple[str, int, int, int, int]],
    heap_free: int,
    heap_min: int,
    cpu_pct: int,
) -> bytes:
    task_count = min(len(tasks), MAX_TASKS)
    timestamp_ticks = int(time.monotonic() * 1000) & 0xFFFF_FFFF
    header = struct.pack("<HIB", seq, timestamp_ticks, task_count)
    task_bytes = b"".join(_encode_task(*task) for task in tasks[:task_count])
    mem_bytes = _encode_memory(heap_free, heap_min, cpu_pct)
    return header + task_bytes + mem_bytes


def _frame_packet(payload: bytes, packet_type: int, seq: int, timestamp_ticks: int) -> bytes:
    header = struct.pack("<BBHIH", VERSION, packet_type, seq, timestamp_ticks, len(payload))
    crc_value = crc16_ccitt(header + payload)
    return bytes([SYNC_0, SYNC_1]) + header + payload + struct.pack("<H", crc_value)


def _tasks_normal() -> List[Tuple[str, int, int, int, int]]:
    return [
        ("SensorTask", 2, 3, 512, 10_000),
        ("CommsTask", 2, 2, 384, 8_000),
        ("ProcTask", 1, 2, 256, 15_000),
        ("IDLE", 0, 0, 128, 960_000),
    ]


def _tasks_saturated() -> List[Tuple[str, int, int, int, int]]:
    return [
        ("SensorTask", 0, 5, 64, 200_000),
        ("CommsTask", 1, 4, 80, 180_000),
        ("ProcTask", 1, 3, 96, 150_000),
        ("IDLE", 1, 0, 128, 10_000),
    ]


def main() -> None:
    parser = argparse.ArgumentParser(
        description="RTOSTwin mock MCU - streams binary telemetry to stdout."
    )
    parser.add_argument(
        "--mode",
        choices=["normal", "leak", "saturated"],
        default="normal",
        help=(
            "normal     : stable 4-task system, ~20%% CPU, no heap drift.\n"
            "leak       : same as normal but heap drops 10 bytes/sec -> OOM.\n"
            "saturated  : all tasks running, 95%% CPU, low stack margins.\n"
        ),
    )
    args = parser.parse_args()
    mode = args.mode

    print(f"[mock_device] mode={mode}  - streaming to stdout at 10 Hz", file=sys.stderr)
    print("[mock_device] Pipe this to: python bridge/main.py --port stdin", file=sys.stderr)

    seq = 0
    heap_free = 131_072
    heap_min_ever = heap_free

    try:
        while True:
            tasks = _tasks_normal() if mode in ("normal", "leak") else _tasks_saturated()

            if mode == "leak":
                heap_free = max(heap_free - 10, 0)
                heap_min_ever = min(heap_min_ever, heap_free)

            cpu_pct = 95 if mode == "saturated" else 20
            timestamp_ticks = int(time.monotonic() * 1000) & 0xFFFF_FFFF
            payload = _build_keyframe_payload(seq, tasks, heap_free, heap_min_ever, cpu_pct)
            packet = _frame_packet(payload, TYPE_KEYFRAME, seq, timestamp_ticks)

            sys.stdout.buffer.write(packet)
            sys.stdout.buffer.flush()

            seq = (seq + 1) & 0xFFFF
            time.sleep(0.1)
    except KeyboardInterrupt:
        print("\n[mock_device] Stopped.", file=sys.stderr)


if __name__ == "__main__":
    main()
