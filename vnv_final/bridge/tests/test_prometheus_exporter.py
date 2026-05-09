import os
import sys

from prometheus_client import CollectorRegistry

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))

from decoder import TaskSnapshot
from prometheus_exporter import PrometheusExporter
from state_manager import DeviceState


def test_prometheus_exporter_renders_required_metrics() -> None:
    exporter = PrometheusExporter(port=0, registry=CollectorRegistry())
    state = DeviceState(
        timestamp_ticks=0x01020304,
        heap_free_bytes=7_712,
        heap_min_ever_bytes=7_680,
        cpu_utilization_pct=10,
        tasks={
            0: TaskSnapshot(
                name="IDLE",
                state=2,
                priority=0,
                stack_hwm_words=32,
                runtime_ticks=150,
            )
        },
        packet_count=4,
        drop_count=1,
    )

    exporter.update_metrics("demo-board", state, oom_projection_seconds=12.5)
    rendered = exporter.render_metrics().decode("utf-8")

    for metric_name in (
        "rtos_task_state",
        "rtos_task_stack_watermark_bytes",
        "rtos_task_cpu_ratio",
        "rtos_heap_free_bytes",
        "rtos_heap_min_ever_bytes",
        "rtos_heap_oom_projection_seconds",
        "rtos_cpu_utilization_ratio",
        "rtos_telemetry_packet_loss_ratio",
    ):
        assert metric_name in rendered

    assert 'device_id="demo-board"' in rendered
    assert 'task_name="IDLE"' in rendered
    assert 'state="blocked"' in rendered
