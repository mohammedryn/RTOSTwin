/**
 * @file state_manager.py
 * @brief Reconstructs full system state from delta-encoded packets.
 */

from typing import Dict, Optional
from dataclasses import dataclass
from .decoder import DecodedPacket, TaskSnapshot

@dataclass
class DeviceState:
    """The 'Current Truth' for a specific hardware device."""
    timestamp_ticks: int = 0
    heap_free_bytes: int = 0
    heap_min_ever_bytes: int = 0
    cpu_utilization_pct: int = 0
    tasks: Dict[int, TaskSnapshot] = None

    def __post_init__(self):
        if self.tasks is None:
            self.tasks = {}

class StateManager:
    """
    Handles the logic of applying deltas to a base keyframe.
    """
    def __init__(self):
        self._current_state = DeviceState()

    def update(self, packet: DecodedPacket) -> DeviceState:
        """
        Updates the internal state with a new packet.
        If it's a keyframe, the state is reset. If delta, only 
        changed fields are updated.
        """
        self._current_state.timestamp_ticks = packet.timestamp_ticks

        # Logic for Task 14: Applying deltas
        # (Note: For simplicity in the initial bridge, we assume the 
        # decoder handles the field extraction. The StateManager 
        # ensures persistence across packets.)
        
        if packet.packet_type == 0x02:  # Keyframe
            self._current_state.heap_free_bytes = packet.heap_free_bytes
            self._current_state.heap_min_ever_bytes = packet.heap_min_ever_bytes
            self._current_state.cpu_utilization_pct = packet.cpu_utilization_pct
            self._current_state.tasks = {i: t for i, t in enumerate(packet.tasks)}
        else:
            # Apply individual memory deltas if present
            if hasattr(packet, 'heap_free_bytes') and packet.heap_free_bytes is not None:
                self._current_state.heap_free_bytes = packet.heap_free_bytes
            
            if hasattr(packet, 'cpu_utilization_pct') and packet.cpu_utilization_pct is not None:
                self._current_state.cpu_utilization_pct = packet.cpu_utilization_pct

            # Apply task deltas
            for idx, task_delta in packet.task_deltas.items():
                if idx in self._current_state.tasks:
                    # Update existing task fields
                    current_task = self._current_state.tasks[idx]
                    if task_delta.state is not None: current_task.state = task_delta.state
                    if task_delta.priority is not None: current_task.priority = task_delta.priority
                    if task_delta.stack_hwm_words is not None: 
                        current_task.stack_hwm_words = task_delta.stack_hwm_words

        return self._current_state

    @property
    def current_state(self) -> DeviceState:
        return self._current_state
