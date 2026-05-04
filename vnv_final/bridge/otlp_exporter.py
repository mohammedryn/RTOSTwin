"""
otlp_exporter.py
-----------------
Pushes RTOS telemetry metrics to any OpenTelemetry-compatible backend
via OTLP/HTTP (e.g. Grafana Cloud, self-hosted OTel Collector, Datadog).

Metric names use the OTel dot-separated convention (TECH_SPEC §4.4).
The matching Prometheus names (underscores) live in prometheus_exporter.py.

OTLP endpoint is read from the OTEL_EXPORTER_OTLP_ENDPOINT environment
variable, defaulting to http://localhost:4318/v1/metrics.

Ownership: VNV
"""

import logging
import os
from typing import Dict, Optional

from opentelemetry import metrics
from opentelemetry.sdk.metrics import MeterProvider
from opentelemetry.sdk.metrics.export import PeriodicExportingMetricReader
from opentelemetry.sdk.resources import Resource
from opentelemetry.exporter.otlp.proto.http.metric_exporter import OTLPMetricExporter

from state_manager import DeviceState
from config import OTLP_ENDPOINT, OTLP_EXPORT_INTERVAL_MS

logger = logging.getLogger(__name__)


class OTLPExporter:
    """
    Configures an OpenTelemetry MeterProvider and pushes all 8 RTOS metrics
    to a configurable OTLP backend on a periodic interval.

    OTel uses push-based export (bridge → backend) on a timer, unlike
    Prometheus which is pull-based (Prometheus → bridge /metrics).

    Usage:
        exporter = OTLPExporter()           # Reads endpoint from env
        exporter.export_metrics("COM3", state, oom_seconds)

    Args:
        endpoint: OTLP HTTP endpoint URL. Defaults to OTEL_EXPORTER_OTLP_ENDPOINT
                  env variable (or http://localhost:4318/v1/metrics).
    """

    def __init__(self, endpoint: Optional[str] = None) -> None:
        resolved_endpoint = endpoint or OTLP_ENDPOINT
        logger.info("[otlp] Exporting to %s every %d ms",
                    resolved_endpoint, OTLP_EXPORT_INTERVAL_MS)

        # Resource tells the backend which service is sending the data
        resource = Resource(attributes={"service.name": "rtostwin-bridge"})

        # OTLPMetricExporter sends OTLP/HTTP protobuf to the configured endpoint
        self._exporter = OTLPMetricExporter(endpoint=resolved_endpoint)

        # PeriodicExportingMetricReader pushes metrics every N ms automatically
        reader = PeriodicExportingMetricReader(
            self._exporter,
            export_interval_millis=OTLP_EXPORT_INTERVAL_MS,
        )

        provider = MeterProvider(resource=resource, metric_readers=[reader])
        metrics.set_meter_provider(provider)

        # One Meter for the whole bridge — metrics are labelled by device_id
        self._meter = metrics.get_meter("rtostwin.bridge")

        # ---- Create all 8 instruments (TECH_SPEC §4.4) ----
        # OTel Gauges are "observable" — we register a callback that
        # the SDK calls just before each export.

        # Storage for the latest values: {device_id: value}
        self._heap_free:      Dict[str, int]   = {}
        self._heap_min:       Dict[str, int]   = {}
        self._heap_oom:       Dict[str, float] = {}
        self._cpu_util:       Dict[str, float] = {}
        self._packet_loss:    Dict[str, float] = {}
        # Per-task: {device_id: {task_name: value}}
        self._task_state:     Dict[str, Dict[str, int]]   = {}
        self._task_stack:     Dict[str, Dict[str, int]]   = {}
        self._task_cpu_ratio: Dict[str, Dict[str, float]] = {}

        # Register observable gauges with their callback functions
        self._meter.create_observable_gauge(
            "rtos.heap.free_bytes",
            callbacks=[self._observe_heap_free],
            description="Current free heap bytes",
            unit="By",
        )
        self._meter.create_observable_gauge(
            "rtos.heap.min_ever_bytes",
            callbacks=[self._observe_heap_min],
            description="Historical minimum free heap bytes",
            unit="By",
        )
        self._meter.create_observable_gauge(
            "rtos.heap.oom_projection_seconds",
            callbacks=[self._observe_heap_oom],
            description="Projected seconds to OOM (-1 = stable)",
            unit="s",
        )
        self._meter.create_observable_gauge(
            "rtos.cpu.utilization_ratio",
            callbacks=[self._observe_cpu_util],
            description="Total CPU utilization fraction 0.0-1.0",
            unit="1",
        )
        self._meter.create_observable_gauge(
            "rtos.telemetry.packet_loss_ratio",
            callbacks=[self._observe_packet_loss],
            description="Fraction of telemetry packets dropped",
            unit="1",
        )
        self._meter.create_observable_gauge(
            "rtos.task.state",
            callbacks=[self._observe_task_state],
            description="1 if task is in this state, 0 otherwise",
            unit="1",
        )
        self._meter.create_observable_gauge(
            "rtos.task.stack_watermark",
            callbacks=[self._observe_task_stack],
            description="Stack bytes remaining above the stack pointer",
            unit="By",
        )
        self._meter.create_observable_gauge(
            "rtos.task.cpu_ratio",
            callbacks=[self._observe_task_cpu_ratio],
            description="Per-task CPU utilization fraction 0.0-1.0",
            unit="1",
        )

    # ------------------------------------------------------------------
    # Public update method — called by main.py on every packet
    # ------------------------------------------------------------------

    def export_metrics(
        self,
        device_id: str,
        state: DeviceState,
        oom_projection_seconds: float = -1.0,
    ) -> None:
        """
        Update the local cache with the latest state. The OTel SDK will
        call the observation callbacks on its own export schedule.

        Args:
            device_id:              Unique device identifier.
            state:                  Latest DeviceState from StateManager.
            oom_projection_seconds: From OOMAnalyzer (-1.0 = stable).
        """
        total = state.packet_count
        loss  = (state.drop_count / total) if total > 0 else 0.0
        total_runtime = sum(t.runtime_ticks for t in state.tasks.values()) or 1

        self._heap_free[device_id]   = state.heap_free_bytes
        self._heap_min[device_id]    = state.heap_min_ever_bytes
        self._heap_oom[device_id]    = oom_projection_seconds
        self._cpu_util[device_id]    = state.cpu_utilization_pct / 100.0
        self._packet_loss[device_id] = loss

        self._task_state[device_id]     = {}
        self._task_stack[device_id]     = {}
        self._task_cpu_ratio[device_id] = {}

        for task in state.tasks.values():
            name = task.name or "unknown"
            self._task_state[device_id][name]     = task.state
            self._task_stack[device_id][name]     = task.stack_hwm_words * 4
            self._task_cpu_ratio[device_id][name] = task.runtime_ticks / total_runtime

    # ------------------------------------------------------------------
    # Observable callbacks — called by the OTel SDK on export
    # ------------------------------------------------------------------

    def _observe_heap_free(self, options: metrics.CallbackOptions):  # type: ignore[name-defined]
        for dev_id, val in self._heap_free.items():
            yield metrics.Observation(val, {"device_id": dev_id})

    def _observe_heap_min(self, options: metrics.CallbackOptions):  # type: ignore[name-defined]
        for dev_id, val in self._heap_min.items():
            yield metrics.Observation(val, {"device_id": dev_id})

    def _observe_heap_oom(self, options: metrics.CallbackOptions):  # type: ignore[name-defined]
        for dev_id, val in self._heap_oom.items():
            yield metrics.Observation(val, {"device_id": dev_id})

    def _observe_cpu_util(self, options: metrics.CallbackOptions):  # type: ignore[name-defined]
        for dev_id, val in self._cpu_util.items():
            yield metrics.Observation(val, {"device_id": dev_id})

    def _observe_packet_loss(self, options: metrics.CallbackOptions):  # type: ignore[name-defined]
        for dev_id, val in self._packet_loss.items():
            yield metrics.Observation(val, {"device_id": dev_id})

    def _observe_task_state(self, options: metrics.CallbackOptions):  # type: ignore[name-defined]
        for dev_id, tasks in self._task_state.items():
            for task_name, state_val in tasks.items():
                yield metrics.Observation(
                    state_val, {"device_id": dev_id, "task_name": task_name}
                )

    def _observe_task_stack(self, options: metrics.CallbackOptions):  # type: ignore[name-defined]
        for dev_id, tasks in self._task_stack.items():
            for task_name, stack_bytes in tasks.items():
                yield metrics.Observation(
                    stack_bytes, {"device_id": dev_id, "task_name": task_name}
                )

    def _observe_task_cpu_ratio(self, options: metrics.CallbackOptions):  # type: ignore[name-defined]
        for dev_id, tasks in self._task_cpu_ratio.items():
            for task_name, ratio in tasks.items():
                yield metrics.Observation(
                    ratio, {"device_id": dev_id, "task_name": task_name}
                )
