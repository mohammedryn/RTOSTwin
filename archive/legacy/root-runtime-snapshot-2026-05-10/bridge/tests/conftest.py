import sys
import os

# Allow bridge/tests/ to import from bridge/ (sibling directory)
sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))

import pytest

"""
conftest.py
-----------
This file contains shared 'fixtures' (test data) for our bridge tests.
Instead of copy-pasting raw bytes into every test, we define them here.
"""

@pytest.fixture
def valid_packet_bytes() -> bytes:
    """
    A manually constructed known-good RTOSTwin packet.
    Payload: b'HELLO'
    Type: 0x01 (Delta)
    Seq: 1
    Time: 100
    """
    # Header: Sync0(AA), Sync1(55), Ver(01), Type(01), Seq(01 00), Time(64 00 00 00), Len(05 00)
    # Payload: 48 45 4c 4c 4f (HELLO)
    # CRC: (Calculated from Ver through Payload)
    
    header_ver_through_len = bytes([
        0x01,                   # Version
        0x01,                   # Type (Delta)
        0x01, 0x00,             # Sequence (1)
        0x64, 0x00, 0x00, 0x00, # Timestamp (100)
        0x05, 0x00              # Length (5)
    ])
    payload = b"HELLO"
    
    # We need the real CRC to make this packet valid
    # In a real test, we calculate this or hardcode a known good value
    from decoder import crc16_ccitt
    crc_val = crc16_ccitt(header_ver_through_len + payload)
    crc_bytes = bytes([crc_val & 0xFF, (crc_val >> 8) & 0xFF])
    
    sync_bytes = bytes([0xAA, 0x55])
    
    return sync_bytes + header_ver_through_len + payload + crc_bytes
