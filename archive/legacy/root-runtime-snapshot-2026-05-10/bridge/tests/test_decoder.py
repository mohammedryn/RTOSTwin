try:
    import pytest
except ImportError:
    pytest = None # type: ignore
import sys
import os

# Add the parent directory (bridge) to the path so we can import 'decoder'
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))

from decoder import PacketDecoder, crc16_ccitt, DecodedPacket

"""
test_decoder.py
---------------
Unit tests for the PacketDecoder class.
Follows Task 6 requirements from TASK_QUEUE.md.
"""

def test_python_crc_matches_standard_vector():
    """Requirement 1: Verify CRC test vector matches C output (0x29B1)."""
    assert crc16_ccitt(b"123456789") == 0x29B1

def test_decoder_happy_path(valid_packet_bytes):
    """Requirement 2: Decode a manually constructed known-good packet."""
    decoder = PacketDecoder()
    
    # We feed the whole block of bytes at once
    packets = decoder.feed_bytes(valid_packet_bytes)
    
    # Check that we found exactly 1 packet
    assert len(packets) == 1
    
    pkt = packets[0]
    assert isinstance(pkt, DecodedPacket)
    assert pkt.payload == b"HELLO"
    assert pkt.sequence_num == 1
    assert pkt.timestamp_ms == 100
    assert decoder.drop_count == 0

def test_decoder_incremental_bytes(valid_packet_bytes):
    """Test feeding bytes one-by-one (State Machine check)."""
    decoder = PacketDecoder()
    
    # Feed every byte individually except the last one
    for i in range(len(valid_packet_bytes) - 1):
        result = decoder.feed_byte(valid_packet_bytes[i])
        assert result is None # Packet should not be finished yet
    
    # Feed the very last byte
    final_result = decoder.feed_byte(valid_packet_bytes[-1])
    
    assert final_result is not None
    assert final_result.payload == b"HELLO"

def test_decoder_crc_failure(valid_packet_bytes):
    """Requirement 3: CRC fails -> packet discarded + drop_count incremented."""
    decoder = PacketDecoder()
    
    # Corrupt one byte of the payload (the 'H' in HELLO)
    corrupted_data = bytearray(valid_packet_bytes)
    corrupted_data[12] = 0xFF 
    
    packets = decoder.feed_bytes(bytes(corrupted_data))
    
    assert len(packets) == 0
    assert decoder.drop_count == 1

def test_decoder_sequence_gap(valid_packet_bytes):
    """Requirement 4: Sequence gap detected."""
    decoder = PacketDecoder()
    
    # 1. Feed packet with Seq 1
    decoder.feed_bytes(valid_packet_bytes)
    assert decoder.sequence_gap_count == 0
    
    # 2. Feed packet with Seq 3 (missing Seq 2)
    p3_data = bytearray(valid_packet_bytes)
    p3_data[4] = 0x03 # Change Seq LOW byte to 3
    
    # We must recalculate CRC for this fake packet to be valid
    header_part = p3_data[2:12]
    payload_part = p3_data[12:-2]
    new_crc = crc16_ccitt(header_part + payload_part)
    p3_data[-2] = new_crc & 0xFF
    p3_data[-1] = (new_crc >> 8) & 0xFF
    
    decoder.feed_bytes(bytes(p3_data))
    
    assert decoder.sequence_gap_count == 1

def test_decoder_resync_after_garbage(valid_packet_bytes):
    """Requirement 5: Garbage bytes before valid packet -> decoder re-syncs."""
    decoder = PacketDecoder()
    
    garbage = b"\x12\x34\xAA\xAA\x56" # Random noise including a partial sync
    full_stream = garbage + valid_packet_bytes
    
    packets = decoder.feed_bytes(full_stream)
    
    assert len(packets) == 1
    assert packets[0].payload == b"HELLO"

if __name__ == "__main__":
    # If pytest is missing, we can run this file directly
    print("--- Running Python Decoder Tests (Manual) ---")
    try:
        test_python_crc_matches_standard_vector()
        print("1. CRC Standard Vector:      PASSED ✅")
        
        from conftest import valid_packet_bytes
        bytes_data = valid_packet_bytes()
        
        test_decoder_happy_path(bytes_data)
        print("2. Happy Path (HELLO):       PASSED ✅")
        
        test_decoder_incremental_bytes(bytes_data)
        print("3. Incremental Bytes:        PASSED ✅")
        
        test_decoder_crc_failure(bytes_data)
        print("4. CRC Failure Detection:    PASSED ✅")
        
        test_decoder_sequence_gap(bytes_data)
        print("5. Sequence Gap Detection:   PASSED ✅")
        
        test_decoder_resync_after_garbage(bytes_data)
        print("6. Garbage Resync:           PASSED ✅")
        
        print("\nALL TASKS IN TASK 6 PASSED! 🚀")
    except Exception as e:
        print(f"\nTEST FAILED ❌: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)
