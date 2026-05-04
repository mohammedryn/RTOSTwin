"""
device_registry.py
------------------
Manages all physical devices connected to the bridge simultaneously.

Each device is identified by a unique string (e.g. serial port name
"COM3", or a WiFi IP "192.168.1.42"). Metrics from different devices
are kept in completely separate StateManager instances so they never
cross-contaminate each other in Prometheus or Grafana.

Ownership: VNV (ARCHITECTURE.md — Component 2)
"""

import logging
import time
from typing import Dict, List, Optional

from state_manager import StateManager, DeviceState
from config import DEVICE_OFFLINE_TIMEOUT_S

logger = logging.getLogger(__name__)


class DeviceRegistry:
    """
    Central store for all active device StateManagers.

    Usage:
        registry = DeviceRegistry()
        manager  = registry.get_or_create("COM3")
        state    = manager.update(packet)

    Attributes:
        _devices: Maps device_id string → its StateManager.
    """

    def __init__(self) -> None:
        self._devices: Dict[str, StateManager] = {}

    # ------------------------------------------------------------------
    # Public API
    # ------------------------------------------------------------------

    def get_or_create(self, device_id: str) -> StateManager:
        """
        Return the StateManager for a device, creating one if it is new.

        Args:
            device_id: Unique string identifier for the device
                       (e.g. serial port name or IP address).

        Returns:
            The StateManager responsible for that device.
        """
        if device_id not in self._devices:
            logger.info("[registry] New device registered: %s", device_id)
            self._devices[device_id] = StateManager()
        return self._devices[device_id]

    def remove(self, device_id: str) -> None:
        """
        Remove a device from the registry (e.g. port disconnected).

        Args:
            device_id: The device to remove.
        """
        if device_id in self._devices:
            del self._devices[device_id]
            logger.info("[registry] Device removed: %s", device_id)

    def list_devices(self) -> List[str]:
        """Return a list of all currently tracked device IDs."""
        return list(self._devices.keys())

    def get_state(self, device_id: str) -> Optional[DeviceState]:
        """
        Get the current DeviceState for a device without creating one.

        Args:
            device_id: Device to query.

        Returns:
            DeviceState if the device exists, None otherwise.
        """
        manager = self._devices.get(device_id)
        return manager.current_state if manager is not None else None

    def all_states(self) -> Dict[str, DeviceState]:
        """
        Return a snapshot dict of all device_id → DeviceState pairs.

        Used by the Prometheus exporter to update all metrics in one pass.

        Returns:
            Dict mapping every known device_id to its current DeviceState.
        """
        return {
            dev_id: manager.current_state
            for dev_id, manager in self._devices.items()
        }

    def purge_offline_devices(self) -> List[str]:
        """
        Remove devices that have not sent a packet in DEVICE_OFFLINE_TIMEOUT_S.

        Returns:
            List of device_ids that were removed.
        """
        now = time.monotonic()
        offline = [
            dev_id
            for dev_id, manager in self._devices.items()
            if (now - manager.current_state.last_seen_s) > DEVICE_OFFLINE_TIMEOUT_S
        ]
        for dev_id in offline:
            logger.warning("[registry] Device timed out (offline): %s", dev_id)
            self.remove(dev_id)
        return offline
