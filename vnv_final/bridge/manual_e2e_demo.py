"""
manual_e2e_demo.py
------------------
Manual end-to-end bridge demo for RTOSTwin.

Runs the decoder, state manager, and OOM analyzer in one Python process,
feeding them packets generated with the same framing logic as mock_device.py.
This is a demonstration script, not part of the automated pytest suite.

Run from the vnv_final project root:
    python bridge/manual_e2e_demo.py
"""

import os
import struct
import sys
import time

sys.path.insert(0, os.path.join(os.path.dirname(__file__)))

from decoder import PacketDecoder
from oom_analyzer import OOMAnalyzer
from state_manager import StateManager

SYNC_0 = 0xAA
SYNC_1 = 0x55
VERSION = 0x01
TYPE_KEYFRAME = 0x02
MAX_TASKS = 16
TASK_NAME_LEN = 16


def crc16_ccitt(data: bytes) -> int:
    crc = 0xFFFF
    for byte in data:
        crc ^= (byte << 8)
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021) if (crc & 0x8000) else (crc << 1)
            crc &= 0xFFFF
    return crc


def make_packet(seq: int, heap_free: int) -> bytes:
    timestamp = seq * 100
    task_count = 4
    header = struct.pack("<HIB", seq, timestamp, task_count)

    tasks = b""
    for name, state, prio, stack, runtime in [
        ("SensorTask", 2, 3, 512, 10000 + seq * 10),
        ("CommsTask", 2, 2, 384, 8000 + seq * 8),
        ("ProcTask", 1, 2, 256, 15000 + seq * 15),
        ("IDLE", 0, 0, 128, 960000 + seq * 960),
    ]:
        name_bytes = name.encode()[:TASK_NAME_LEN].ljust(TASK_NAME_LEN, b"\x00")
        tasks += name_bytes + struct.pack("<BBHi", state, prio, stack, runtime)

    tasks += b"\x00" * ((MAX_TASKS - task_count) * 24)

    heap_min = heap_free
    memory = struct.pack("<IIB", heap_free, heap_min, 20)
    payload = header + tasks + memory

    frame_header = struct.pack("<BBHIH", VERSION, TYPE_KEYFRAME, seq, timestamp, len(payload))
    crc_value = crc16_ccitt(frame_header + payload)
    return bytes([SYNC_0, SYNC_1]) + frame_header + payload + struct.pack("<H", crc_value)


def main() -> None:
    print("=" * 60)
    print("  RTOSTwin Bridge Manual End-to-End Demo")
    print("=" * 60)

    decoder = PacketDecoder()
    manager = StateManager()
    oom = OOMAnalyzer(
        window_size=600,
        min_r_squared=0.7,
        total_heap_bytes=131_072,
    )

    heap_free = 131_072
    start_time = time.monotonic()

    for seq in range(30):
        heap_free -= 10
        raw_packet = make_packet(seq, heap_free)
        packets = decoder.feed_bytes(raw_packet)

        for packet in packets:
            state = manager.update(packet)
            now = time.monotonic()
            oom.add_sample(timestamp_s=now - start_time, heap_free_bytes=state.heap_free_bytes)
            oom_seconds = oom.get_projection_seconds()

            print(f"\n--- Packet #{seq:03d} ---")
            print(f"  heap_free : {state.heap_free_bytes:,} bytes")
            print(f"  heap_min  : {state.heap_min_ever_bytes:,} bytes")
            print(f"  cpu       : {state.cpu_utilization_pct}%")
            print(f"  tasks     : {len(state.tasks)}")
            for index, task in state.tasks.items():
                print(
                    f"    [{index}] {task.name:<14} state={task.state} "
                    f"prio={task.priority} stack={task.stack_hwm_words}w "
                    f"runtime={task.runtime_ticks}"
                )
            if oom_seconds > 0:
                print(f"  OOM DETECTED: {oom_seconds:.1f}s until crash")
            elif oom_seconds == -2.0:
                print("  OOM DETECTED: rolling minimum alert")
            else:
                print("  oom       : stable (need more data)")

    print("\n" + "=" * 60)
    print(
        f"  Done. Decoded {decoder.drop_count} drops, "
        f"{decoder.sequence_gap_count} gaps."
    )
    print(f"  Final heap: {heap_free:,} bytes")
    print("=" * 60)


if __name__ == "__main__":
    main()
