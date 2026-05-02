/**
 * @file oom_analyzer.py
 * @brief Detects memory leaks and projects time-to-crash.
 */

import time
from collections import deque
import numpy as np
from scipy.stats import linregress

class OOMAnalyzer:
    """
    Analyzes heap trends to predict Out-Of-Memory (OOM) events.
    Uses two strategies:
    1. Linear Regression (for steady leaks)
    2. Rolling Minimum (for sawtooth leaks)
    """
    def __init__(self, window_size: int = 600, r_squared_threshold: float = 0.7):
        self.window_size = window_size
        self.r_squared_threshold = r_squared_threshold
        # Stores (timestamp, heap_bytes)
        self._history = deque(maxlen=window_size)

    def add_sample(self, heap_bytes: int):
        """Adds a new measurement to the sliding window."""
        self._history.append((time.time(), heap_bytes))

    def get_projection_seconds(self) -> float:
        """
        Predicts how many seconds remain before heap hits zero.
        Returns:
            -1.0 if heap is stable or increasing.
            Value > 0 if a leak is detected with high confidence.
        """
        if len(self._history) < 30:
            return -1.0  # Not enough data for reliable regression

        # Convert to numpy arrays for math
        data = np.array(self._history)
        x = data[:, 0]  # Timestamps
        y = data[:, 1]  # Heap values

        # Normalize X to prevent large number math errors
        x_norm = x - x[0]

        # Perform Linear Regression
        slope, intercept, r_value, p_value, std_err = linregress(x_norm, y)
        r_squared = r_value ** 2

        # Criteria for a "Confident Leak":
        # 1. Slope must be negative (leaking)
        # 2. R-squared must be high (line is straight, not just noise)
        if slope < 0 and r_squared >= self.r_squared_threshold:
            current_heap = y[-1]
            # Time = Distance / Speed
            time_to_zero = current_heap / abs(slope)
            return float(time_to_zero)

        return -1.0

    @property
    def confidence(self) -> float:
        """Returns the R-squared value of the current trend."""
        if len(self._history) < 2: return 0.0
        data = np.array(self._history)
        _, _, r_value, _, _ = linregress(data[:,0], data[:,1])
        return r_value ** 2
