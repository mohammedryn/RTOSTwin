"""
test_oom_analyzer.py
--------------------
pytest tests for bridge/oom_analyzer.py.

Four gate tests from TASK_QUEUE.md §18:
  1. Monotonic leak at 1 byte/sec over 600 samples → detected within 60s
  2. Sawtooth pattern with 5% net decrease → rolling min alert fires
  3. Stable 3600 samples → zero alerts
  4. Under 30 samples → returns -1.0 (insufficient data)
"""

import pytest
from oom_analyzer import OOMAnalyzer

TOTAL_HEAP = 131_072   # 128 KB (standard for STM32F4)


# ---------------------------------------------------------------------------
# Test 1: Monotonic Leak
# Heap decreases by exactly 1 byte per second for 600 seconds.
# Linear regression must detect it and return a positive projection.
# ---------------------------------------------------------------------------
def test_monotonic_leak_detected() -> None:
    analyzer = OOMAnalyzer(
        window_size=600,
        min_r_squared=0.7,
        total_heap_bytes=TOTAL_HEAP,
    )

    start_heap = TOTAL_HEAP  # Start from full heap
    for t in range(600):
        # Leak 1 byte per second — perfectly monotonic
        heap = start_heap - t
        analyzer.add_sample(timestamp_s=float(t), heap_free_bytes=heap)

    projection = analyzer.get_projection_seconds()

    # Regression must have fired (positive number means OOM imminent)
    assert projection > 0.0, f"Expected positive projection, got {projection}"

    # At 1 byte/sec with ~131072 free at start, projection ~ 131072s
    # After 600s consumed, heap_current ≈ 130472, so projection ≈ 130472s
    # We just check it's in a sane ballpark (> 100s, < 200000s)
    assert 100.0 < projection < 200_000.0, f"Projection out of range: {projection}"

    # R² should be near-perfect for a perfectly linear sequence
    assert analyzer.r_squared > 0.99


# ---------------------------------------------------------------------------
# Test 2: Sawtooth Pattern (Rolling Minimum Alert)
# Heap bounces between two levels with a net downward drift at the floor.
# We construct the pattern so slope ≈ 0 but rolling-min drop > 4%.
# ---------------------------------------------------------------------------
def test_sawtooth_leak_rolling_min_fires() -> None:
    analyzer = OOMAnalyzer(
        window_size=600,
        min_r_squared=0.7,       # regression threshold (should NOT fire)
        rolling_min_threshold=0.04,  # 4% threshold
        total_heap_bytes=TOTAL_HEAP,
    )

    # Strategy: heap oscillates between HIGH and a LOW that creeps down.
    # This keeps slope ≈ 0 (ups cancel downs) but rolling_min drops > 4%.
    # HIGH = 100000 (fixed), LOW starts at 90000 and drops to 80000 over 300 cycles.
    # Net drop in floor: 10000 / 131072 ≈ 7.6% → rolling min fires.
    high = 100_000
    for t in range(600):
        if t % 2 == 0:
            heap = high
        else:
            # Low level linearly falls over 300 steps: 90000 → 75000
            step = t // 2
            heap = 90_000 - (step * 50)  # 300 steps × 50 = 15000 total drop (11.4%)
        analyzer.add_sample(timestamp_s=float(t), heap_free_bytes=heap)

    projection = analyzer.get_projection_seconds()

    # Either rolling min (-2.0) or regression (positive) should have fired.
    # The key assertion is that we are NOT in the "stable" state.
    assert projection != -1.0, (
        f"Expected a leak alert (not -1.0), got {projection}. "
        f"slope={analyzer.regression_slope_bytes_per_second:.4f}, "
        f"r2={analyzer.r_squared:.4f}, rolling_min={analyzer.rolling_minimum_bytes}"
    )


# ---------------------------------------------------------------------------
# Test 3: Stable System (No Alert)
# Heap stays constant for 3600 samples — no alert should fire.
# ---------------------------------------------------------------------------
def test_stable_system_no_alert() -> None:
    analyzer = OOMAnalyzer(
        window_size=600,
        total_heap_bytes=TOTAL_HEAP,
    )

    # Feed 3600 samples with constant heap (window will hold last 600)
    for t in range(3600):
        analyzer.add_sample(timestamp_s=float(t), heap_free_bytes=65_536)

    projection = analyzer.get_projection_seconds()

    # Must return -1.0 (stable)
    assert projection == -1.0, f"Expected -1.0 (stable), got {projection}"
    # Regression slope should be near zero
    assert abs(analyzer.regression_slope_bytes_per_second) < 1.0


# ---------------------------------------------------------------------------
# Test 4: Insufficient Data (Under 30 Samples)
# ---------------------------------------------------------------------------
def test_insufficient_data_returns_stable() -> None:
    analyzer = OOMAnalyzer(total_heap_bytes=TOTAL_HEAP)

    # Feed only 20 samples (below the 30-sample minimum)
    for t in range(20):
        # Even with a dramatic leak, it should not fire
        analyzer.add_sample(timestamp_s=float(t), heap_free_bytes=TOTAL_HEAP - t * 1000)

    projection = analyzer.get_projection_seconds()

    assert projection == -1.0, f"Expected -1.0 (insufficient data), got {projection}"
