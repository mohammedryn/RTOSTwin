"""
state_manager.py
----------------
Reconstructs the full per-device system state from decoded packets.
"""

from dataclasses import dataclass, field
import struct
import time
from typing import Dict, Optional

from decoder import DecodedPacket, TaskSnapshot

WF_TYPE_DELTA = 0x01
WF_TYPE_KEYFRAME = 0x02

FIELD_TASK_STATE = 0x01
FIELD_TASK_PRIO = 0x02
FIELD_TASK_STACK = 0x03
FIELD_TASK_RUNTIME = 0x04
FIELD_HEAP_FREE = 0x05
FIELD_HEAP_MIN = 0x06
FIELD_CPU_UTIL = 0x07

MAX_TASKS = 16
TASK_NAME_LEN = 16


@dataclass
class DeviceState:
    timestamp_ticks: int = 0
    heap_free_bytes: int = 0
    heap_min_ever_bytes: int = 0
    cpu_utilization_pct: int = 0
    tasks: Dict[int, TaskSnapshot] = field(default_factory=dict)
    packet_count: int = 0
    drop_count: int = 0
    last_seen_s: float = 0.0


def _parse_keyframe_payload(payload: bytes) -> Optional[DeviceState]:
    header_size = 7
    task_size = 24
    memory_size = 9

    if len(payload) < (header_size + memory_size):
        return None

    sequence_num, timestamp_ticks, task_count = struct.unpack_from("<HIB", payload, 0)
    del sequence_num

    if task_count > MAX_TASKS:
        return None

    expected_len = header_size + (task_count * task_size) + memory_size
    if len(payload) != expected_len:
        return None

    offset = header_size
    tasks: Dict[int, TaskSnapshot] = {}

    for index in range(task_count):
        name_bytes = payload[offset : offset + TASK_NAME_LEN]
        name = name_bytes.rstrip(b"\x00").decode("ascii", errors="replace")
        state, priority, stack_hwm_words, runtime_ticks = struct.unpack_from(
            "<BBHI", payload, offset + TASK_NAME_LEN
        )
        tasks[index] = TaskSnapshot(
            name=name,
            state=state,
            priority=priority,
            stack_hwm_words=stack_hwm_words,
            runtime_ticks=runtime_ticks,
        )
        offset += task_size

    heap_free_bytes, heap_min_ever_bytes, cpu_utilization_pct = struct.unpack_from(
        "<IIB", payload, offset
    )

    return DeviceState(
        timestamp_ticks=timestamp_ticks,
        heap_free_bytes=heap_free_bytes,
        heap_min_ever_bytes=heap_min_ever_bytes,
        cpu_utilization_pct=cpu_utilization_pct,
        tasks=tasks,
    )


def _apply_delta_payload(payload: bytes, state: DeviceState) -> None:
    index = 0
    while index < len(payload):
        tag = payload[index]
        index += 1

        task_idx = (tag >> 4) & 0x0F
        field_id = tag & 0x0F

        if task_idx == 0x0F:
            if field_id == FIELD_HEAP_FREE and index + 4 <= len(payload):
                state.heap_free_bytes = struct.unpack_from("<I", payload, index)[0]
                index += 4
            elif field_id == FIELD_HEAP_MIN and index + 4 <= len(payload):
                state.heap_min_ever_bytes = struct.unpack_from("<I", payload, index)[0]
                index += 4
            elif field_id == FIELD_CPU_UTIL and index + 1 <= len(payload):
                state.cpu_utilization_pct = payload[index]
                index += 1
            else:
                break
        else:
            if task_idx not in state.tasks:
                widths = {
                    FIELD_TASK_STATE: 1,
                    FIELD_TASK_PRIO: 1,
                    FIELD_TASK_STACK: 2,
                    FIELD_TASK_RUNTIME: 4,
                }
                index += widths.get(field_id, 0)
                continue

            task = state.tasks[task_idx]
            if field_id == FIELD_TASK_STATE and index + 1 <= len(payload):
                task.state = payload[index]
                index += 1
            elif field_id == FIELD_TASK_PRIO and index + 1 <= len(payload):
                task.priority = payload[index]
                index += 1
            elif field_id == FIELD_TASK_STACK and index + 2 <= len(payload):
                task.stack_hwm_words = struct.unpack_from("<H", payload, index)[0]
                index += 2
            elif field_id == FIELD_TASK_RUNTIME and index + 4 <= len(payload):
                task.runtime_ticks = struct.unpack_from("<I", payload, index)[0]
                index += 4
            else:
                break


class StateManager:
    def __init__(self) -> None:
        self._state: DeviceState = DeviceState()
        self._has_keyframe = False

    def update(self, packet: DecodedPacket) -> DeviceState:
        self._state.packet_count += 1
        self._state.last_seen_s = time.monotonic()

        if packet.packet_type == WF_TYPE_KEYFRAME:
            parsed = _parse_keyframe_payload(packet.payload)
            if parsed is not None:
                packet_count = self._state.packet_count
                drop_count = self._state.drop_count
                self._state = parsed
                self._state.packet_count = packet_count
                self._state.drop_count = drop_count
                self._state.last_seen_s = time.monotonic()
                self._has_keyframe = True
        elif packet.packet_type == WF_TYPE_DELTA and self._has_keyframe:
            _apply_delta_payload(packet.payload, self._state)
            self._state.timestamp_ticks = packet.timestamp_ticks

        return self._state

    def record_drop(self) -> None:
        self._state.drop_count += 1

    @property
    def current_state(self) -> DeviceState:
        return self._state

    @property
    def has_baseline(self) -> bool:
        return self._has_keyframe
