/**
 * @file device_registry.py
 * @brief Manages multiple hardware devices connected to the bridge.
 */

from typing import Dict
from .state_manager import StateManager

class DeviceRegistry:
    """
    Acts as a container for all active devices. 
    Ensures that metrics from 'Board A' never get mixed up 
    with metrics from 'Board B'.
    """
    def __init__(self):
        self._devices: Dict[str, StateManager] = {}

    def get_manager(self, device_id: str) -> StateManager:
        """
        Retrieves the StateManager for a specific device.
        If the device is new, a new manager is created.
        """
        if device_id not in self._devices:
            print(f"[REGISTRY] Registered new device: {device_id}")
            self._devices[device_id] = StateManager()
        return self._devices[device_id]

    def remove_device(self, device_id: str):
        if device_id in self._devices:
            del self._devices[device_id]
            print(f"[REGISTRY] Unregistered device: {device_id}")

    def list_devices(self) -> list:
        return list(self._devices.keys())
