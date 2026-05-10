/**
 * @file otlp_exporter.py
 * @brief Pushes metrics to OpenTelemetry backends via OTLP/HTTP.
 */

import time
from opentelemetry import metrics
from opentelemetry.sdk.metrics import MeterProvider
from opentelemetry.sdk.resources import Resource
from opentelemetry.sdk.metrics.export import PeriodicExportingMetricReader
from opentelemetry.exporter.otlp.proto.http.metric_exporter import OTLPMetricExporter
from .state_manager import DeviceState

class OTLPManager:
    def __init__(self, endpoint: str, service_name: str = "rtostwin-bridge"):
        resource = Resource(attributes={"service.name": service_name})
        self.exporter = OTLPMetricExporter(endpoint=endpoint)
        self.reader = PeriodicExportingMetricReader(self.exporter, export_interval_millis=30000)
        self.provider = MeterProvider(resource=resource, metric_readers=[self.reader])
        metrics.set_meter_provider(self.provider)
        self.meter = metrics.get_meter("rtostwin.metrics")

        # Define OTel Gauges
        self.heap_gauge = self.meter.create_observable_gauge(
            name="rtos.heap.free",
            description="Current free heap",
            unit="by"
        )
        # (Internal storage for values to be read by observable gauges)
        self._current_data = {}

    def update(self, device_id: str, state: DeviceState):
        """Updates the local cache for OTel exporters."""
        self._current_data[device_id] = state.heap_free_bytes

    # (OTel requires callbacks for observable instruments)
    def _observe_heap(self, observer):
        for dev_id, val in self._current_data.items():
            yield metrics.Observation(val, {"device_id": dev_id})
