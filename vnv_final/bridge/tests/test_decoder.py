import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))

from decoder import DecodedPacket, PacketDecoder, crc16_ccitt


def test_python_crc_matches_standard_vector():
    assert crc16_ccitt(b"123456789") == 0x29B1


def test_decoder_happy_path(valid_packet_bytes):
    decoder = PacketDecoder()
    packets = decoder.feed_bytes(valid_packet_bytes)

    assert len(packets) == 1
    packet = packets[0]
    assert isinstance(packet, DecodedPacket)
    assert packet.packet_type == 0x01
    assert packet.sequence_num == 0x1235
    assert packet.timestamp_ticks == 0x01020368
    assert packet.payload == bytes.fromhex("f5 20 1e 00 00 f7 0a")
    assert decoder.drop_count == 0


def test_decoder_incremental_bytes(valid_packet_bytes):
    decoder = PacketDecoder()

    for index in range(len(valid_packet_bytes) - 1):
        assert decoder.feed_byte(valid_packet_bytes[index]) is None

    final_result = decoder.feed_byte(valid_packet_bytes[-1])
    assert final_result is not None
    assert final_result.sequence_num == 0x1235


def test_decoder_crc_failure(corrupted_packet_bytes):
    decoder = PacketDecoder()

    packets = decoder.feed_bytes(corrupted_packet_bytes)

    assert len(packets) == 0
    assert decoder.drop_count == 1


def test_decoder_sequence_gap(valid_packet_bytes):
    decoder = PacketDecoder()

    decoder.feed_bytes(valid_packet_bytes)
    assert decoder.sequence_gap_count == 0

    gap_data = bytearray(valid_packet_bytes)
    gap_data[4] = 0x37
    gap_data[5] = 0x12
    new_crc = crc16_ccitt(gap_data[2:-2])
    gap_data[-2] = new_crc & 0xFF
    gap_data[-1] = (new_crc >> 8) & 0xFF

    decoder.feed_bytes(bytes(gap_data))
    assert decoder.sequence_gap_count == 1


def test_decoder_resync_after_garbage(valid_packet_bytes):
    decoder = PacketDecoder()

    garbage = b"\x12\x34\xAA\xAA\x56"
    packets = decoder.feed_bytes(garbage + valid_packet_bytes)

    assert len(packets) == 1
    assert packets[0].sequence_num == 0x1235
