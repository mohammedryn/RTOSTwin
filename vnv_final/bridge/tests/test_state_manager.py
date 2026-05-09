import os
import struct
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))

from decoder import DecodedPacket
from state_manager import StateManager


def test_state_manager_ignores_delta_until_keyframe() -> None:
    manager = StateManager()

    delta_packet = DecodedPacket(
        packet_type=0x01,
        sequence_num=0x1235,
        timestamp_ticks=0x01020368,
        payload=bytes.fromhex("f5 20 1e 00 00 f7 0a"),
    )

    state = manager.update(delta_packet)

    assert manager.has_baseline is False
    assert state.packet_count == 1
    assert state.heap_free_bytes == 0
    assert state.tasks == {}


def test_state_manager_applies_keyframe_then_delta(keyframe_payload_bytes: bytes) -> None:
    manager = StateManager()

    keyframe_packet = DecodedPacket(
        packet_type=0x02,
        sequence_num=0x1234,
        timestamp_ticks=0x01020304,
        payload=keyframe_payload_bytes,
    )

    keyframe_state = manager.update(keyframe_packet)

    assert manager.has_baseline is True
    assert keyframe_state.timestamp_ticks == 0x01020304
    assert keyframe_state.heap_free_bytes == 8_000
    assert keyframe_state.heap_min_ever_bytes == 7_680
    assert keyframe_state.cpu_utilization_pct == 7
    assert keyframe_state.tasks[0].name == "IDLE"
    assert keyframe_state.tasks[0].state == 1
    assert keyframe_state.tasks[0].runtime_ticks == 100

    delta_payload = (
        bytes([0xF5]) + struct.pack("<I", 7_712)
        + bytes([0xF7, 10])
        + bytes([0x01, 2])
        + bytes([0x04]) + struct.pack("<I", 150)
    )
    delta_packet = DecodedPacket(
        packet_type=0x01,
        sequence_num=0x1235,
        timestamp_ticks=0x01020368,
        payload=delta_payload,
    )

    delta_state = manager.update(delta_packet)

    assert delta_state.timestamp_ticks == 0x01020368
    assert delta_state.heap_free_bytes == 7_712
    assert delta_state.cpu_utilization_pct == 10
    assert delta_state.tasks[0].state == 2
    assert delta_state.tasks[0].runtime_ticks == 150
    assert delta_state.packet_count == 2
