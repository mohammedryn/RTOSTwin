import os
import sys

from prometheus_client import CollectorRegistry

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))

from decoder import PacketDecoder
from device_registry import DeviceRegistry
from main import handle_packets
from oom_analyzer import OOMAnalyzer
from prometheus_exporter import PrometheusExporter


class RecordingOTLPExporter:
    def __init__(self) -> None:
        self.calls = []

    def export_metrics(self, device_id, state, oom_projection_seconds=-1.0) -> None:
        self.calls.append((device_id, state.packet_count, oom_projection_seconds))


def test_handle_packets_updates_metrics_from_keyframe(valid_keyframe_packet_bytes: bytes) -> None:
    decoder = PacketDecoder()
    packets = decoder.feed_bytes(valid_keyframe_packet_bytes)
    registry = DeviceRegistry()
    prom = PrometheusExporter(port=0, registry=CollectorRegistry())
    otlp = RecordingOTLPExporter()
    oom = OOMAnalyzer(total_heap_bytes=131_072)

    processed = handle_packets(
        device_id="mock-stdin",
        packets=packets,
        decoder=decoder,
        registry=registry,
        prom=prom,
        otlp=otlp,
        oom=oom,
        otlp_enabled=True,
    )

    rendered = prom.render_metrics().decode("utf-8")

    assert processed == 1
    assert len(otlp.calls) == 1
    assert "rtos_heap_free_bytes" in rendered
    assert 'device_id="mock-stdin"' in rendered
    assert registry.get_state("mock-stdin").heap_free_bytes == 8_000
