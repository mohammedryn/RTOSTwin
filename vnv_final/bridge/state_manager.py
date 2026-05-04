"""
state_manager.py
----------------
Reconstructs the full per-device system state from decoded packets.

The mock_device and the real MCU send keyframes (full snapshot) and
delta packets (only changed fields). The StateManager applies these
on top of a stored baseline so the rest of the bridge always sees a
complete, up-to-date DeviceState — never raw bytes.

Ownership: VNV (ARCHITECTURE.md — Component 2, Python Bridge side)
Dependency: consumes DecodedPacket from decoder.py (RYN's module).
"""

import struct
import time
from dataclasses import dataclass, field
from typing import Dict, List, Optional

from decoder import DecodedPacket, TaskSnapshot

# ---------------------------------------------------------------------------
# Wire format constants — must match wire_format.h
# ---------------------------------------------------------------------------
WF_TYPE_DELTA    = 0x01
WF_TYPE_KEYFRAME = 0x02

# Delta field IDs (TECH_SPEC §2.4)
FIELD_TASK_STATE  = 0x01
FIELD_TASK_PRIO   = 0x02
FIELD_TASK_STACK  = 0x03
FIELD_TASK_RUNTIME= 0x04
FIELD_HEAP_FREE   = 0x05
FIELD_HEAP_MIN    = 0x06
FIELD_CPU_UTIL    = 0x07

MAX_TASKS      = 16
TASK_NAME_LEN  = 16


# ---------------------------------------------------------------------------
# DeviceState — the "current truth" for one connected MCU
# ---------------------------------------------------------------------------
@dataclass
class DeviceState:
    """
    Complete reconstructed state for a single physical device.

    This is what the Prometheus and OTLP exporters read.
    Every field mirrors the equivalent field in full_snapshot_t (C agent).

    Attributes:
        timestamp_ticks:    Last RTOS tick count received.
        heap_free_bytes:    Current free heap in bytes.
        heap_min_ever_bytes: Historical minimum free heap (never goes up).
        cpu_utilization_pct: Whole-number CPU utilization 0-100.
        tasks:              Dict of task_index → TaskSnapshot.
        packet_count:       Total packets processed for this device.
        drop_count:         Sequence gaps detected (potential packet loss).
        last_seen_s:        Unix timestamp of last received packet.
    """
    timestamp_ticks:     int             = 0
    heap_free_bytes:     int             = 0
    heap_min_ever_bytes: int             = 0
    cpu_utilization_pct: int             = 0
    tasks:               Dict[int, TaskSnapshot] = field(default_factory=dict)
    packet_count:        int             = 0
    drop_count:          int             = 0
    last_seen_s:         float           = 0.0


# ---------------------------------------------------------------------------
# Keyframe payload parser
# ---------------------------------------------------------------------------
def _parse_keyframe_payload(payload: bytes) -> Optional[DeviceState]:
    """
    Deserialise a raw keyframe payload into a DeviceState.

    Keyframe layout (mirrors full_snapshot_t serialisation in mock_device.py):
      seq_num      (2 bytes LE)
      timestamp    (4 bytes LE)
      task_count   (1 byte)
      tasks        (task_count × 24 bytes, then zero-padded to MAX_TASKS × 24)
      memory       (9 bytes: heap_free(4) + heap_min(4) + cpu(1))

    Args:
        payload: Raw bytes from DecodedPacket.payload.

    Returns:
        Populated DeviceState, or None if payload is too short.
    """
    # Minimum size: 7-byte header + 0 tasks + 9-byte memory = 16 bytes
    header_size  = 7                          # seq(2) + ts(4) + count(1)
    task_size    = 24                         # per-task record
    memory_size  = 9
    total_tasks_bytes = MAX_TASKS * task_size
    min_payload  = header_size + total_tasks_bytes + memory_size

    if len(payload) < min_payload:
        return None

    # --- header ---
    seq_num, timestamp, task_count = struct.unpack_from("<HIB", payload, 0)
    offset = header_size

    # --- tasks ---
    tasks: Dict[int, TaskSnapshot] = {}
    actual_count = min(task_count, MAX_TASKS)
    for i in range(actual_count):
        name_bytes = payload[offset: offset + TASK_NAME_LEN]
        name = name_bytes.rstrip(b"\x00").decode("ascii", errors="replace")
        state, priority, stack_hwm_words, runtime_ticks = struct.unpack_from(
            "<BBHi", payload, offset + TASK_NAME_LEN
        )
        tasks[i] = TaskSnapshot(
            name=name,
            state=state,
            priority=priority,
            stack_hwm_words=stack_hwm_words,
            runtime_ticks=runtime_ticks,
        )
        offset += task_size

    # Skip padding tasks (already advanced for actual tasks, skip the rest)
    offset = header_size + total_tasks_bytes

    # --- memory ---
    heap_free, heap_min, cpu_pct = struct.unpack_from("<IIB", payload, offset)

    state_obj = DeviceState(
        timestamp_ticks=timestamp,
        heap_free_bytes=heap_free,
        heap_min_ever_bytes=heap_min,
        cpu_utilization_pct=cpu_pct,
        tasks=tasks,
    )
    return state_obj


# ---------------------------------------------------------------------------
# Delta payload parser
# ---------------------------------------------------------------------------
def _apply_delta_payload(payload: bytes, state: DeviceState) -> None:
    """
    Apply a delta-encoded payload onto an existing DeviceState in-place.

    Delta layout (TECH_SPEC §2.4):
      Each changed field is encoded as:
        TAG byte  (1 byte): upper nibble = task_index (or 0xF for system fields)
                            lower nibble = field_id
        Value     (N bytes): size depends on field_id

    Args:
        payload: Raw bytes from a TYPE_DELTA DecodedPacket.
        state:   DeviceState to update in-place.
    """
    i = 0
    while i < len(payload):
        tag = payload[i]
        i += 1

        task_idx = (tag >> 4) & 0x0F
        field_id =  tag       & 0x0F

        # System-level field (task_idx = 0xF)
        if task_idx == 0xF:
            if field_id == FIELD_HEAP_FREE and i + 4 <= len(payload):
                state.heap_free_bytes = struct.unpack_from("<I", payload, i)[0]
                i += 4
            elif field_id == FIELD_HEAP_MIN and i + 4 <= len(payload):
                state.heap_min_ever_bytes = struct.unpack_from("<I", payload, i)[0]
                i += 4
            elif field_id == FIELD_CPU_UTIL and i + 1 <= len(payload):
                state.cpu_utilization_pct = payload[i]
                i += 1
            else:
                break  # Unknown field — stop to avoid parsing garbage

        # Per-task field
        else:
            if task_idx not in state.tasks:
                # Task appeared mid-stream (no keyframe yet) — skip safely
                # Field widths: state=1, prio=1, stack=2, runtime=4
                widths = {FIELD_TASK_STATE: 1, FIELD_TASK_PRIO: 1,
                          FIELD_TASK_STACK: 2, FIELD_TASK_RUNTIME: 4}
                i += widths.get(field_id, 0)
                continue

            task = state.tasks[task_idx]
            if field_id == FIELD_TASK_STATE and i + 1 <= len(payload):
                task.state = payload[i]; i += 1
            elif field_id == FIELD_TASK_PRIO and i + 1 <= len(payload):
                task.priority = payload[i]; i += 1
            elif field_id == FIELD_TASK_STACK and i + 2 <= len(payload):
                task.stack_hwm_words = struct.unpack_from("<H", payload, i)[0]
                i += 2
            elif field_id == FIELD_TASK_RUNTIME and i + 4 <= len(payload):
                task.runtime_ticks = struct.unpack_from("<i", payload, i)[0]
                i += 4
            else:
                break  # Unknown field — stop


# ---------------------------------------------------------------------------
# StateManager — one instance per physical device
# ---------------------------------------------------------------------------
class StateManager:
    """
    Maintains and updates the complete state for one physical device.

    Usage:
        manager = StateManager()
        state = manager.update(decoded_packet)   # Call on every packet
        print(state.heap_free_bytes)

    The manager handles both keyframe (full reset) and delta (partial
    update) packets correctly. It does NOT parse raw bytes — it consumes
    DecodedPacket objects produced by decoder.py (RYN's module).
    """

    def __init__(self) -> None:
        self._state: DeviceState = DeviceState()
        self._has_keyframe: bool = False       # True once first keyframe seen

    def update(self, packet: DecodedPacket) -> DeviceState:
        """
        Apply a decoded packet to the internal state and return it.

        Args:
            packet: A DecodedPacket produced by PacketDecoder.feed_byte(s).

        Returns:
            The updated DeviceState (the same object, mutated in-place).
        """
        self._state.packet_count += 1
        self._state.last_seen_s  = time.monotonic()

        if packet.packet_type == WF_TYPE_KEYFRAME:
            parsed = _parse_keyframe_payload(packet.payload)
            if parsed is not None:
                # Preserve counters — reset everything else
                packet_count = self._state.packet_count
                drop_count   = self._state.drop_count
                self._state  = parsed
                self._state.packet_count = packet_count
                self._state.drop_count   = drop_count
                self._state.last_seen_s  = time.monotonic()
                self._has_keyframe = True

        elif packet.packet_type == WF_TYPE_DELTA:
            if self._has_keyframe:
                # Only apply deltas once we have a baseline keyframe
                _apply_delta_payload(packet.payload, self._state)
                self._state.timestamp_ticks = packet.timestamp_ms
            # If no keyframe yet, silently discard delta (can't apply without baseline)

        return self._state

    def record_drop(self) -> None:
        """Increment the drop counter (called by main loop on seq gap)."""
        self._state.drop_count += 1

    @property
    def current_state(self) -> DeviceState:
        """Read-only access to the current device state."""
        return self._state

    @property
    def has_baseline(self) -> bool:
        """True once at least one keyframe has been received."""
        return self._has_keyframe
