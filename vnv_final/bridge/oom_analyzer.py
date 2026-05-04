"""
oom_analyzer.py
---------------
Detects memory leaks and projects time-to-crash.

Implements TECH_SPEC §4.2 — two detection algorithms:
  1. Linear Regression  — catches steady monotonic leaks.
  2. Rolling Minimum    — catches sawtooth/bursty leaks (slope ≈ 0).

NOTE: This file is owned by RYN. VNV has fixed only the Python syntax
error (C-style doc block) and aligned the public API with TECH_SPEC §4.2
so main.py can import it. Core algorithm is unchanged.
"""

from collections import deque
from typing import List, Tuple

import numpy as np
from scipy.stats import linregress


class OOMAnalyzer:
    """
    Analyzes heap trends to predict Out-Of-Memory (OOM) events.

    Uses two detection strategies (TECH_SPEC §5.2):
      1. Linear Regression — catches steady monotonic leaks.
      2. Rolling Minimum   — catches sawtooth/bursty leaks.

    Args:
        window_size:            Number of samples in the sliding window
                                (default 600 = 60 seconds at 10 Hz).
        min_r_squared:          Minimum R² for regression to confirm a leak
                                (default 0.7).
        rolling_min_threshold:  Fraction of total heap that must drop before
                                the rolling-min alert fires (default 0.10 = 10%).
        total_heap_bytes:       Total heap size in bytes (default 128 KB).
    """

    def __init__(
        self,
        window_size: int = 600,
        min_r_squared: float = 0.7,
        rolling_min_threshold: float = 0.10,
        total_heap_bytes: int = 131_072,
        # Legacy alias kept for backward compat
        r_squared_threshold: float = None,
    ) -> None:
        self.window_size = window_size
        self.min_r_squared = r_squared_threshold if r_squared_threshold is not None else min_r_squared
        self.rolling_min_threshold = rolling_min_threshold
        self.total_heap_bytes = total_heap_bytes

        # Stores (timestamp_s, heap_free_bytes) tuples
        self._history: deque = deque(maxlen=window_size)

        # Cached regression results (updated on get_projection_seconds())
        self._slope: float = 0.0
        self._r_squared: float = 0.0

    # ------------------------------------------------------------------
    # Public API (TECH_SPEC §4.2)
    # ------------------------------------------------------------------

    def add_sample(self, timestamp_s: float, heap_free_bytes: int) -> None:
        """
        Add a new heap measurement to the sliding window.

        Args:
            timestamp_s:     Wall-clock time of measurement (time.monotonic()).
            heap_free_bytes: Free heap bytes from xPortGetFreeHeapSize().
        """
        self._history.append((timestamp_s, heap_free_bytes))

    def get_projection_seconds(self) -> float:
        """
        Predict seconds remaining before heap hits zero.

        Returns:
            -1.0  if heap is stable or there is insufficient data.
            -2.0  if only the rolling-minimum detector fired (no regression).
            >0.0  projected seconds to OOM (regression detector fired).
        """
        if len(self._history) < 30:
            return -1.0

        data = np.array(self._history)
        x: np.ndarray = data[:, 0]
        y: np.ndarray = data[:, 1]
        x_norm = x - x[0]   # Normalise to avoid large-number errors

        # --- Detector 1: Linear Regression ---
        slope, _intercept, r_value, _p, _stderr = linregress(x_norm, y)
        self._slope = float(slope)
        self._r_squared = float(r_value ** 2)

        if slope < 0 and self._r_squared >= self.min_r_squared:
            current_heap = float(y[-1])
            return current_heap / abs(slope)

        # --- Detector 2: Rolling Minimum ---
        window_min_start = float(np.min(y[:len(y) // 2]))  # First-half minimum
        window_min_now   = float(np.min(y))
        drop_fraction = (window_min_start - window_min_now) / self.total_heap_bytes

        if drop_fraction > self.rolling_min_threshold:
            return -2.0   # Rolling-min alert: leak detected, no slope estimate

        return -1.0   # Stable

    # ------------------------------------------------------------------
    # Properties (TECH_SPEC §4.2)
    # ------------------------------------------------------------------

    @property
    def regression_slope_bytes_per_second(self) -> float:
        """Bytes-per-second slope from the last regression run."""
        return self._slope

    @property
    def r_squared(self) -> float:
        """R² goodness-of-fit from the last regression run."""
        return self._r_squared

    @property
    def rolling_minimum_bytes(self) -> int:
        """Current rolling minimum heap value across the window."""
        if not self._history:
            return 0
        return int(min(v for _, v in self._history))
