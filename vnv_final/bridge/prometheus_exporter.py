"""
prometheus_exporter.py
-----------------------
Serves an HTTP endpoint at /metrics for Prometheus to scrape.

All 8 required RTOS metrics from TECH_SPEC.md §4.3 are implemented.
The HTTP server runs in a background thread (started by prometheus_client
automatically) so it does not block the bridge loop.
"""

import logging
from typing import Dict

from prometheus_client import CollectorRegistry, Gauge, generate_latest, start_http_server

from state_manager import DeviceState

logger = logging.getLogger(__name__)

_STATE_NAMES: Dict[int, str] = {
    0: "running",
    1: "ready",
    2: "blocked",
    3: "suspended",
    4: "deleted",
}


class PrometheusExporter:
    """Wrap the Prometheus registry, gauges, and optional HTTP server."""

    def __init__(self, port: int = 8000, registry: CollectorRegistry | None = None) -> None:
        self._port = port
        self._started = False
        self._registry = registry or CollectorRegistry()

        self._task_state = Gauge(
            "rtos_task_state",
            "1 if the task is currently in the labelled state, 0 otherwise",
            ["device_id", "task_name", "state"],
            registry=self._registry,
        )
        self._task_stack_watermark = Gauge(
            "rtos_task_stack_watermark_bytes",
            "Stack high-watermark: bytes remaining before stack overflow",
            ["device_id", "task_name"],
            registry=self._registry,
        )
        self._task_cpu_ratio = Gauge(
            "rtos_task_cpu_ratio",
            "Per-task CPU utilization as a fraction 0.0-1.0",
            ["device_id", "task_name"],
            registry=self._registry,
        )
        self._heap_free = Gauge(
            "rtos_heap_free_bytes",
            "Current free heap in bytes (xPortGetFreeHeapSize)",
            ["device_id"],
            registry=self._registry,
        )
        self._heap_min_ever = Gauge(
            "rtos_heap_min_ever_bytes",
            "Lowest free heap ever observed (xPortGetMinimumEverFreeHeapSize)",
            ["device_id"],
            registry=self._registry,
        )
        self._heap_oom_projection = Gauge(
            "rtos_heap_oom_projection_seconds",
            "Projected seconds until heap OOM. -1 means no leak detected.",
            ["device_id"],
            registry=self._registry,
        )
        self._cpu_utilization = Gauge(
            "rtos_cpu_utilization_ratio",
            "Total system CPU utilization as a fraction 0.0-1.0",
            ["device_id"],
            registry=self._registry,
        )
        self._packet_loss = Gauge(
            "rtos_telemetry_packet_loss_ratio",
            "Fraction of telemetry packets dropped (sequence gap detection)",
            ["device_id"],
            registry=self._registry,
        )

    def start(self) -> None:
        """Start the HTTP server once."""
        if not self._started:
            start_http_server(self._port, addr="0.0.0.0", registry=self._registry)
            self._started = True
            logger.info("[prometheus] Metrics server started on port %d", self._port)

    def render_metrics(self) -> bytes:
        """Render the current metrics snapshot in Prometheus exposition format."""
        return generate_latest(self._registry)

    def update_metrics(
        self,
        device_id: str,
        state: DeviceState,
        oom_projection_seconds: float = -1.0,
    ) -> None:
        """Push the latest DeviceState into all Prometheus gauges."""
        self._heap_free.labels(device_id=device_id).set(state.heap_free_bytes)
        self._heap_min_ever.labels(device_id=device_id).set(state.heap_min_ever_bytes)
        self._heap_oom_projection.labels(device_id=device_id).set(oom_projection_seconds)
        self._cpu_utilization.labels(device_id=device_id).set(
            state.cpu_utilization_pct / 100.0
        )

        total = state.packet_count
        loss_ratio = (state.drop_count / total) if total > 0 else 0.0
        self._packet_loss.labels(device_id=device_id).set(loss_ratio)

        total_runtime = sum(task.runtime_ticks for task in state.tasks.values()) or 1

        for task in state.tasks.values():
            task_name = task.name if task.name else "unknown"

            for state_id, state_label in _STATE_NAMES.items():
                self._task_state.labels(
                    device_id=device_id,
                    task_name=task_name,
                    state=state_label,
                ).set(1 if task.state == state_id else 0)

            self._task_stack_watermark.labels(
                device_id=device_id,
                task_name=task_name,
            ).set(task.stack_hwm_words * 4)

            self._task_cpu_ratio.labels(
                device_id=device_id,
                task_name=task_name,
            ).set(task.runtime_ticks / total_runtime)
