import os
import sys
import struct

import pytest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))

from decoder import crc16_ccitt


@pytest.fixture
def valid_packet_bytes() -> bytes:
    """Root frozen golden delta vector."""
    return bytes.fromhex(
        "aa 55 01 01 35 12 68 03 02 01 07 00 f5 20 1e 00 00 f7 0a 36 2d"
    )


@pytest.fixture
def corrupted_packet_bytes() -> bytes:
    """Root frozen corrupted delta vector."""
    return bytes.fromhex(
        "aa 55 01 01 35 12 68 03 02 01 07 00 f5 20 1e 00 00 f7 0a 36 d2"
    )


@pytest.fixture
def keyframe_payload_bytes() -> bytes:
    task_name = b"IDLE".ljust(16, b"\x00")
    return (
        struct.pack("<HIB", 0x1234, 0x01020304, 1)
        + task_name
        + struct.pack("<BBHI", 1, 0, 32, 100)
        + struct.pack("<IIB", 8_000, 7_680, 7)
    )


@pytest.fixture
def valid_keyframe_packet_bytes(keyframe_payload_bytes: bytes) -> bytes:
    header = struct.pack("<BBHIH", 0x01, 0x02, 0x1234, 0x01020304, len(keyframe_payload_bytes))
    crc = crc16_ccitt(header + keyframe_payload_bytes)
    return b"\xAA\x55" + header + keyframe_payload_bytes + struct.pack("<H", crc)
