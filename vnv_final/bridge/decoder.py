"""
decoder.py
-----------
Packet decoding and validation for the RTOSTwin Python bridge.
"""

from dataclasses import dataclass
import enum
import struct
from typing import List, Optional

SYNC_0 = 0xAA
SYNC_1 = 0x55
PROTOCOL_VERSION = 0x01
CRC_POLY = 0x1021
CRC_INIT = 0xFFFF
HEADER_INFO_SIZE = 10
CRC_SIZE = 2


def crc16_ccitt(data: bytes, initial: int = CRC_INIT) -> int:
    """Calculate the CRC-16-CCITT checksum for a byte array."""
    crc = initial
    for byte in data:
        crc ^= byte << 8
        for _ in range(8):
            if crc & 0x8000:
                crc = (crc << 1) ^ CRC_POLY
            else:
                crc = crc << 1
            crc &= 0xFFFF
    return crc


@dataclass
class TaskSnapshot:
    """Python equivalent of task_snapshot_t from the C agent."""

    name: str = ""
    state: int = 0
    priority: int = 0
    stack_hwm_words: int = 0
    runtime_ticks: int = 0


@dataclass
class DecodedPacket:
    """Validated packet from the MCU."""

    packet_type: int
    sequence_num: int
    timestamp_ticks: int
    payload: bytes


class DecoderState(enum.Enum):
    WAIT_SYNC_0 = 1
    WAIT_SYNC_1 = 2
    READ_HEADER = 3
    READ_PAYLOAD = 4
    READ_CRC = 5


class PacketDecoder:
    """State machine for processing RTOSTwin serial packets."""

    def __init__(self):
        self._state = DecoderState.WAIT_SYNC_0
        self._buffer = bytearray()
        self._payload_buffer = bytearray()
        self._buffer_for_crc = bytearray()

        self._expected_payload_len = 0
        self._pkt_type = 0
        self._seq_num = 0
        self._timestamp_ticks = 0

        self._drop_count = 0
        self._sequence_gap_count = 0
        self._last_seq = -1

    @property
    def drop_count(self) -> int:
        return self._drop_count

    @property
    def sequence_gap_count(self) -> int:
        return self._sequence_gap_count

    def feed_byte(self, byte: int) -> Optional[DecodedPacket]:
        """Process a single byte and return a packet if one completes."""
        if self._state == DecoderState.WAIT_SYNC_0:
            if byte == SYNC_0:
                self._state = DecoderState.WAIT_SYNC_1
            return None

        if self._state == DecoderState.WAIT_SYNC_1:
            if byte == SYNC_1:
                self._state = DecoderState.READ_HEADER
                self._buffer = bytearray()
            elif byte == SYNC_0:
                self._state = DecoderState.WAIT_SYNC_1
            else:
                self._state = DecoderState.WAIT_SYNC_0
            return None

        if self._state == DecoderState.READ_HEADER:
            self._buffer.append(byte)
            if len(self._buffer) == HEADER_INFO_SIZE:
                try:
                    version, self._pkt_type, self._seq_num, self._timestamp_ticks, self._expected_payload_len = struct.unpack(
                        "<BBHIH", self._buffer
                    )
                except struct.error:
                    self._reset_machine()
                    return None

                if version != PROTOCOL_VERSION:
                    self._reset_machine()
                    return None

                self._payload_buffer = bytearray()
                if self._expected_payload_len == 0:
                    self._state = DecoderState.READ_CRC
                    self._buffer_for_crc = bytearray()
                else:
                    self._state = DecoderState.READ_PAYLOAD
            return None

        if self._state == DecoderState.READ_PAYLOAD:
            self._payload_buffer.append(byte)
            if len(self._payload_buffer) == self._expected_payload_len:
                self._state = DecoderState.READ_CRC
                self._buffer_for_crc = bytearray()
            return None

        if self._state == DecoderState.READ_CRC:
            self._buffer_for_crc.append(byte)
            if len(self._buffer_for_crc) == CRC_SIZE:
                received_crc = struct.unpack("<H", self._buffer_for_crc)[0]
                calculated_crc = crc16_ccitt(self._buffer + self._payload_buffer)

                if received_crc != calculated_crc:
                    self._drop_count += 1
                    self._reset_machine()
                    return None

                if self._last_seq != -1:
                    expected_next = (self._last_seq + 1) & 0xFFFF
                    if self._seq_num != expected_next:
                        self._sequence_gap_count += 1
                self._last_seq = self._seq_num

                packet = DecodedPacket(
                    packet_type=self._pkt_type,
                    sequence_num=self._seq_num,
                    timestamp_ticks=self._timestamp_ticks,
                    payload=bytes(self._payload_buffer),
                )
                self._reset_machine()
                return packet
            return None

        return None

    def feed_bytes(self, data: bytes) -> List[DecodedPacket]:
        """Batch process a byte string."""
        packets: List[DecodedPacket] = []
        for byte in data:
            packet = self.feed_byte(byte)
            if packet is not None:
                packets.append(packet)
        return packets

    def _reset_machine(self) -> None:
        self._state = DecoderState.WAIT_SYNC_0
        self._buffer = bytearray()
        self._payload_buffer = bytearray()
        self._buffer_for_crc = bytearray()
