import os
import sys

# Add the parent directory (bridge) to the path so we can import 'decoder'
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))

from decoder import DecodedPacket, PacketDecoder, crc16_ccitt

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

    packets = decoder.feed_bytes(valid_packet_bytes)

    assert len(packets) == 1

    pkt = packets[0]
    assert isinstance(pkt, DecodedPacket)
    assert pkt.payload == b"HELLO"
    assert pkt.sequence_num == 1
    assert pkt.timestamp_ms == 100
    assert decoder.drop_count == 0


def test_decoder_incremental_bytes(valid_packet_bytes):
    """Test feeding bytes one-by-one (state machine check)."""
    decoder = PacketDecoder()

    for index in range(len(valid_packet_bytes) - 1):
        result = decoder.feed_byte(valid_packet_bytes[index])
        assert result is None

    final_result = decoder.feed_byte(valid_packet_bytes[-1])

    assert final_result is not None
    assert final_result.payload == b"HELLO"


def test_decoder_crc_failure(valid_packet_bytes):
    """Requirement 3: CRC fails and the packet is discarded."""
    decoder = PacketDecoder()

    corrupted_data = bytearray(valid_packet_bytes)
    corrupted_data[12] = 0xFF

    packets = decoder.feed_bytes(bytes(corrupted_data))

    assert len(packets) == 0
    assert decoder.drop_count == 1


def test_decoder_sequence_gap(valid_packet_bytes):
    """Requirement 4: Sequence gap detected."""
    decoder = PacketDecoder()

    decoder.feed_bytes(valid_packet_bytes)
    assert decoder.sequence_gap_count == 0

    p3_data = bytearray(valid_packet_bytes)
    p3_data[4] = 0x03

    header_part = p3_data[2:12]
    payload_part = p3_data[12:-2]
    new_crc = crc16_ccitt(header_part + payload_part)
    p3_data[-2] = new_crc & 0xFF
    p3_data[-1] = (new_crc >> 8) & 0xFF

    decoder.feed_bytes(bytes(p3_data))

    assert decoder.sequence_gap_count == 1


def test_decoder_resync_after_garbage(valid_packet_bytes):
    """Requirement 5: Garbage bytes before valid packet are skipped."""
    decoder = PacketDecoder()

    garbage = b"\x12\x34\xAA\xAA\x56"
    full_stream = garbage + valid_packet_bytes

    packets = decoder.feed_bytes(full_stream)

    assert len(packets) == 1
    assert packets[0].payload == b"HELLO"
