"""
test_e2e.py
-----------
End-to-end integration test for the RTOSTwin bridge.

Runs entirely in one Python process — no OS pipes, no PowerShell quirks.
Generates 30 packets using mock_device logic, feeds them through the full
bridge pipeline, and prints the decoded state to the console.

Run from the project root:
    python bridge/test_e2e.py

Expected output: 30 packets decoded, heap shrinking 10 bytes/packet (leak mode).
"""

import sys
import os
import time

# Add bridge directory to path
sys.path.insert(0, os.path.join(os.path.dirname(__file__)))

from decoder import PacketDecoder
from state_manager import StateManager
from oom_analyzer import OOMAnalyzer

# Re-use mock_device's packet generation logic
import struct

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
    timestamp = seq * 100   # Simulated 100ms per tick
    task_count = 4
    header = struct.pack("<HIB", seq, timestamp, task_count)

    tasks = b""
    for name, state, prio, stack, runtime in [
        ("SensorTask", 2, 3, 512, 10000 + seq * 10),
        ("CommsTask",  2, 2, 384, 8000  + seq * 8),
        ("ProcTask",   1, 2, 256, 15000 + seq * 15),
        ("IDLE",       0, 0, 128, 960000 + seq * 960),
    ]:
        name_b = name.encode()[:TASK_NAME_LEN].ljust(TASK_NAME_LEN, b"\x00")
        tasks += name_b + struct.pack("<BBHi", state, prio, stack, runtime)

    # Pad to MAX_TASKS
    tasks += b"\x00" * ((MAX_TASKS - task_count) * 24)

    heap_min = heap_free  # simplified: min = current for test
    mem = struct.pack("<IIB", heap_free, heap_min, 20)  # 20% CPU

    payload = header + tasks + mem

    frame_header = struct.pack("<BBHiH", VERSION, TYPE_KEYFRAME, seq, timestamp, len(payload))
    crc_val = crc16_ccitt(frame_header + payload)
    return bytes([SYNC_0, SYNC_1]) + frame_header + payload + struct.pack("<H", crc_val)


def main() -> None:
    print("=" * 60)
    print("  RTOSTwin Bridge — End-to-End Integration Test")
    print("=" * 60)

    decoder = PacketDecoder()
    manager = StateManager()
    oom     = OOMAnalyzer(
        window_size=600,
        min_r_squared=0.7,
        total_heap_bytes=131_072,
    )

    heap_free = 131_072  # Start full
    t0 = time.monotonic()

    for seq in range(30):
        # Simulate 10 Hz: leak 10 bytes per packet
        heap_free -= 10

        raw_packet = make_packet(seq, heap_free)
        packets = decoder.feed_bytes(raw_packet)

        for pkt in packets:
            state = manager.update(pkt)
            now = time.monotonic()
            oom.add_sample(timestamp_s=now - t0, heap_free_bytes=state.heap_free_bytes)
            oom_s = oom.get_projection_seconds()

            print(f"\n--- Packet #{seq:03d} ---")
            print(f"  heap_free : {state.heap_free_bytes:,} bytes")
            print(f"  heap_min  : {state.heap_min_ever_bytes:,} bytes")
            print(f"  cpu       : {state.cpu_utilization_pct}%")
            print(f"  tasks     : {len(state.tasks)}")
            for i, task in state.tasks.items():
                print(f"    [{i}] {task.name:<14} state={task.state} prio={task.priority} stack={task.stack_hwm_words}w runtime={task.runtime_ticks}")
            if oom_s > 0:
                print(f"  ⚠️  OOM DETECTED — {oom_s:.1f}s until crash")
            elif oom_s == -2.0:
                print(f"  ⚠️  OOM DETECTED (rolling min alert)")
            else:
                print(f"  oom       : stable (need more data)")

    print("\n" + "=" * 60)
    print(f"  Done. Decoded {decoder.drop_count} drops, {decoder.sequence_gap_count} gaps.")
    print(f"  Final heap: {heap_free:,} bytes")
    print("=" * 60)


if __name__ == "__main__":
    main()
