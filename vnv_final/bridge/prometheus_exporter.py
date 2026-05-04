"""
prometheus_exporter.py
-----------------------
Serves an HTTP endpoint at /metrics for Prometheus to scrape.

All 8 required RTOS metrics from TECH_SPEC.md §4.3 are implemented.
The HTTP server runs in a background thread (started by prometheus_client
automatically) so it does not block the main async loop.

Metric names use underscores (Prometheus convention).
The matching OTel names (dots) live in otlp_exporter.py.

Ownership: VNV
"""

import logging
from typing import Dict

from prometheus_client import Gauge, start_http_server

from state_manager import DeviceState

logger = logging.getLogger(__name__)

# ---------------------------------------------------------------------------
# Metric definitions — exactly 8 metrics per TECH_SPEC §4.3
# ---------------------------------------------------------------------------

# 1. Task state — 1 if task is in this state, 0 otherwise
#    Labels: device_id, task_name, state
RTOS_TASK_STATE = Gauge(
    "rtos_task_state",
    "1 if the task is currently in the labelled state, 0 otherwise",
    ["device_id", "task_name", "state"],
)

# 2. Stack watermark — bytes remaining above the stack pointer
#    Labels: device_id, task_name
RTOS_TASK_STACK_WATERMARK = Gauge(
    "rtos_task_stack_watermark_bytes",
    "Stack high-watermark: bytes remaining before stack overflow",
    ["device_id", "task_name"],
)

# 3. Per-task CPU fraction (0.0 – 1.0)
#    Labels: device_id, task_name
RTOS_TASK_CPU_RATIO = Gauge(
    "rtos_task_cpu_ratio",
    "Per-task CPU utilization as a fraction 0.0-1.0",
    ["device_id", "task_name"],
)

# 4. Current free heap bytes
#    Labels: device_id
RTOS_HEAP_FREE = Gauge(
    "rtos_heap_free_bytes",
    "Current free heap in bytes (xPortGetFreeHeapSize)",
    ["device_id"],
)

# 5. Historical minimum free heap bytes (never goes up)
#    Labels: device_id
RTOS_HEAP_MIN_EVER = Gauge(
    "rtos_heap_min_ever_bytes",
    "Lowest free heap ever observed (xPortGetMinimumEverFreeHeapSize)",
    ["device_id"],
)

# 6. OOM projection in seconds (-1.0 = stable)
#    Labels: device_id
RTOS_HEAP_OOM_PROJECTION = Gauge(
    "rtos_heap_oom_projection_seconds",
    "Projected seconds until heap OOM. -1 means no leak detected.",
    ["device_id"],
)

# 7. Total CPU utilization fraction (0.0 – 1.0)
#    Labels: device_id
RTOS_CPU_UTILIZATION = Gauge(
    "rtos_cpu_utilization_ratio",
    "Total system CPU utilization as a fraction 0.0-1.0",
    ["device_id"],
)

# 8. Telemetry packet loss ratio
#    Labels: device_id
RTOS_PACKET_LOSS = Gauge(
    "rtos_telemetry_packet_loss_ratio",
    "Fraction of telemetry packets dropped (sequence gap detection)",
    ["device_id"],
)

# Human-readable state names — maps eTaskState integer to a label string
_STATE_NAMES: Dict[int, str] = {
    0: "running",
    1: "ready",
    2: "blocked",
    3: "suspended",
    4: "deleted",
}


# ---------------------------------------------------------------------------
# Public API
# ---------------------------------------------------------------------------

class PrometheusExporter:
    """
    Wraps the prometheus_client HTTP server and metric update logic.

    Usage:
        exporter = PrometheusExporter(port=8000)
        exporter.start()                        # Once at startup
        exporter.update_metrics("COM3", state)  # Every decoded packet
    """

    def __init__(self, port: int = 8000) -> None:
        self._port = port
        self._started = False

    def start(self) -> None:
        """
        Start the HTTP server in a background thread.
        Prometheus will scrape http://localhost:<port>/metrics.
        Safe to call only once.
        """
        if not self._started:
            start_http_server(self._port, addr='0.0.0.0')
            self._started = True
            logger.info("[prometheus] Metrics server started on port %d", self._port)

    def update_metrics(
        self,
        device_id: str,
        state: DeviceState,
        oom_projection_seconds: float = -1.0,
    ) -> None:
        logger.warning("[prometheus] Updating metrics for %s (heap_free=%d)", 
                     device_id, state.heap_free_bytes)
        """
        Push the latest DeviceState into all Prometheus Gauges.

        Args:
            device_id:              Unique device identifier (label value).
            state:                  Current DeviceState from StateManager.
            oom_projection_seconds: OOM time projection from OOMAnalyzer
                                    (-1.0 = stable, >0 = seconds to OOM).
        """
        # --- system-level metrics ---
        RTOS_HEAP_FREE.labels(device_id=device_id).set(state.heap_free_bytes)
        RTOS_HEAP_MIN_EVER.labels(device_id=device_id).set(state.heap_min_ever_bytes)
        RTOS_HEAP_OOM_PROJECTION.labels(device_id=device_id).set(oom_projection_seconds)
        RTOS_CPU_UTILIZATION.labels(device_id=device_id).set(
            state.cpu_utilization_pct / 100.0
        )

        # --- packet loss ratio ---
        total = state.packet_count
        loss_ratio = (state.drop_count / total) if total > 0 else 0.0
        RTOS_PACKET_LOSS.labels(device_id=device_id).set(loss_ratio)

        # --- per-task metrics ---
        total_runtime = sum(t.runtime_ticks for t in state.tasks.values()) or 1

        for task in state.tasks.values():
            task_name = task.name if task.name else f"task_{id(task)}"

            # Task state: emit 1 for the current state, 0 for all others
            for state_id, state_label in _STATE_NAMES.items():
                value = 1 if task.state == state_id else 0
                RTOS_TASK_STATE.labels(
                    device_id=device_id,
                    task_name=task_name,
                    state=state_label,
                ).set(value)

            # Stack watermark: convert words → bytes (ARM = 4 bytes/word)
            stack_bytes = task.stack_hwm_words * 4
            RTOS_TASK_STACK_WATERMARK.labels(
                device_id=device_id, task_name=task_name
            ).set(stack_bytes)

            # Per-task CPU ratio: runtime fraction vs total runtime
            cpu_ratio = task.runtime_ticks / total_runtime
            RTOS_TASK_CPU_RATIO.labels(
                device_id=device_id, task_name=task_name
            ).set(cpu_ratio)
