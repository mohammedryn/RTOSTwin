"""
decoder.py
-----------
Packet decoding and validation for the RTOSTwin Python Bridge.

This module is responsible for reading a raw byte stream, synchronizing
with the packet frame boundaries, and yielding validated payload data.
"""

from typing import Optional, List
from dataclasses import dataclass
import enum
import struct

# Protocol Constants (Mapped from wire_format.h)
SYNC_0 = 0xAA
SYNC_1 = 0x55
PROTOCOL_VERSION = 0x01
CRC_POLY = 0x1021
CRC_INIT = 0xFFFF

# Header is 12 bytes (Sync0, Sync1, Ver, Type, Seq(2), Time(4), Len(2))
# But state machine handles Sync bytes separately. 
# Internal header (after sync) is 10 bytes.
HEADER_INFO_SIZE = 10 
CRC_SIZE = 2

def crc16_ccitt(data: bytes, initial: int = CRC_INIT) -> int:
    """
    Calculate the CRC-16-CCITT checksum for a byte array.
    """
    crc = initial
    for byte in data:
        crc ^= byte << 8
        for _ in range(8):
            if crc & 0x8000:
                crc = ((crc << 1) ^ CRC_POLY)
            else:
                crc = (crc << 1)
    return crc & 0xFFFF


@dataclass
class TaskSnapshot:
    """Python equivalent of 'task_snapshot_t' from the C Agent."""
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
    timestamp_ms: int
    payload: bytes


class DecoderState(enum.Enum):
    """Phases of the packet reception state machine."""
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
        
        self._expected_payload_len = 0
        self._pkt_type = 0
        self._seq_num = 0
        self._timestamp = 0
        
        self._drop_count = 0
        self._sequence_gap_count = 0
        self._last_seq = -1

    @property
    def drop_count(self) -> int: return self._drop_count

    @property
    def sequence_gap_count(self) -> int: return self._sequence_gap_count

    def feed_byte(self, byte: int) -> Optional[DecodedPacket]:
        """Processes a single byte and returns a packet if complete."""
        
        if self._state == DecoderState.WAIT_SYNC_0:
            if byte == SYNC_0:
                self._state = DecoderState.WAIT_SYNC_1
            return None

        if self._state == DecoderState.WAIT_SYNC_1:
            if byte == SYNC_1:
                self._state = DecoderState.READ_HEADER
                self._buffer = bytearray() # Clear buffer for header contents
            elif byte == SYNC_0:
                # Stay in WAIT_SYNC_1! This handles 0xAA 0xAA 0x55 case.
                self._state = DecoderState.WAIT_SYNC_1 
            else:
                self._state = DecoderState.WAIT_SYNC_0
            return None

        if self._state == DecoderState.READ_HEADER:
            self._buffer.append(byte)
            if len(self._buffer) == HEADER_INFO_SIZE:
                # Map bytes to variables (Rule: Little Endian)
                # 'B' = 1 byte, 'H' = 2 bytes (unsigned), 'I' = 4 bytes (unsigned)
                # '<' = Little Endian
                try:
                    # FIX: Changed last 'I' (4 bytes) to 'H' (2 bytes)
                    # This matches the 10-byte HEADER_INFO_SIZE
                    ver, self._pkt_type, self._seq_num, self._timestamp, self._expected_payload_len = \
                        struct.unpack("<BBHIH", self._buffer)
                    
                    if ver != PROTOCOL_VERSION:
                        self._reset_machine()
                        return None
                        
                    if self._expected_payload_len == 0:
                        self._state = DecoderState.READ_CRC
                    else:
                        self._state = DecoderState.READ_PAYLOAD
                    self._payload_buffer = bytearray()
                except Exception:
                    self._reset_machine()
            return None

        if self._state == DecoderState.READ_PAYLOAD:
            self._payload_buffer.append(byte)
            if len(self._payload_buffer) == self._expected_payload_len:
                self._state = DecoderState.READ_CRC
                self._buffer_for_crc = bytearray() # Re-clear for CRC bytes
            return None

        if self._state == DecoderState.READ_CRC:
            self._buffer_for_crc.append(byte)
            if len(self._buffer_for_crc) == CRC_SIZE:
                # 1. Validate CRC
                received_crc = struct.unpack("<H", self._buffer_for_crc)[0]
                
                # CRC covers VERSION (1st byte of _buffer) through end of PAYLOAD
                data_to_verify = self._buffer + self._payload_buffer
                calculated_crc = crc16_ccitt(data_to_verify)
                
                if received_crc != calculated_crc:
                    self._drop_count += 1
                    self._reset_machine()
                    return None
                
                # 2. Check Sequence Gap
                if self._last_seq != -1:
                    expected_next = (self._last_seq + 1) & 0xFFFF
                    if self._seq_num != expected_next:
                        self._sequence_gap_count += 1
                self._last_seq = self._seq_num

                # 3. Build Result
                packet = DecodedPacket(
                    packet_type=self._pkt_type,
                    sequence_num=self._seq_num,
                    timestamp_ms=self._timestamp,
                    payload=bytes(self._payload_buffer)
                )
                self._reset_machine()
                return packet
            return None

        return None

    def feed_bytes(self, data: bytes) -> List[DecodedPacket]:
        """Batch process a list of bytes."""
        packets = []
        for b in data:
            p = self.feed_byte(b)
            if p:
                packets.append(p)
        return packets

        

    def _reset_machine(self) -> None:
        """Go back to start and clear temporary buffers."""
        self._state = DecoderState.WAIT_SYNC_0
        self._buffer = bytearray()
        self._payload_buffer = bytearray()
