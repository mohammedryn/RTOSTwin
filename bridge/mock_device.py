"""
mock_device.py
--------------
Simulates an RTOSTwin MCU agent for hardware-free testing of the bridge.

What it does:
1. Creates a virtual TCP server (or just prints to stdout to be piped).
2. Generates dummy FreeRTOS task data and memory snapshots.
3. Encodes them into the binary wire format (CRC, framing).
4. Includes a simulated heap leak to trigger the OOM analyzer.

Usage:
    python bridge/mock_device.py | python bridge/main.py --serial /dev/stdin
    (Or more simply, the script can just send to a local socket)
"""

import time
import struct
import random

# Mirror of wire_format.h constants
SYNC0 = 0xAA
SYNC1 = 0x55
VERSION = 0x01
TYPE_KEYFRAME = 0x02

def crc16_ccitt(data: bytes) -> int:
    crc = 0xFFFF
    for byte in data:
        crc ^= (byte << 8)
        for _ in range(8):
            if crc & 0x8000:
                crc = (crc << 1) ^ 0x1021
            else:
                crc = (crc << 1)
            crc &= 0xFFFF
    return crc

def generate_keyframe(seq: int, heap: int) -> bytes:
    # Full snapshot payload (simplified but follows structure)
    # Header: seq(2), ts(4), task_count(1)
    timestamp = int(time.time() * 1000) & 0xFFFFFFFF
    header = struct.pack("<HI B", seq, timestamp, 2)
    
    # Task 1: "GUI"
    task1 = b"GUI".ljust(16, b'\x00') + struct.pack("<BBH I", 0, 10, 512, 5000)
    # Task 2: "IDLE"
    task2 = b"IDLE".ljust(16, b'\x00') + struct.pack("<BBH I", 1, 0, 128, 95000)
    # Padding for remaining tasks (14 * 24 bytes)
    padding = b"\x00" * (14 * 24)
    
    # Memory: free(4), min(4), cpu(1)
    mem = struct.pack("<IIB", heap, 65536, 5)
    
    payload = header + task1 + task2 + padding + mem
    
    # Frame it
    ver_type_seq_ts_len = struct.pack("<BBHIH", VERSION, TYPE_KEYFRAME, seq, timestamp, len(payload))
    crc_val = crc16_ccitt(ver_type_seq_ts_len + payload)
    
    return bytes([SYNC0, SYNC1]) + ver_type_seq_ts_len + payload + struct.pack("<H", crc_val)

def main():
    # Only use stderr for logging to keep stdout clean for binary data
    import sys
    print("Starting RTOSTwin Mock Device...", file=sys.stderr)
    print("Streaming binary telemetry to stdout. Pipe this to bridge/main.py", file=sys.stderr)
    
    seq = 0
    current_heap = 120000
    
    try:
        while True:
            # Generate a keyframe for simplicity (mocking deltas is complex for a script)
            packet = generate_keyframe(seq, current_heap)
            sys.stdout.buffer.write(packet)
            sys.stdout.buffer.flush()
            
            # Simulate a slow leak
            current_heap = max(current_heap - 50, 1000)
            
            seq = (seq + 1) & 0xFFFF
            time.sleep(0.1) # 10 Hz like the real MCU
    except KeyboardInterrupt:
        pass

if __name__ == "__main__":
    main()
