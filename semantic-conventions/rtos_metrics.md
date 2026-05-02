# OTel Semantic Conventions Proposal: RTOS Metrics

Currently, OpenTelemetry (OTel) lacks specific semantic conventions for Real-Time Operating Systems (RTOS). To ensure RTOSTwin is future-proof and interoperable, we propose the following namespace patterns.

## 1. Task Namespacing
All task-level metrics should live under the `rtos.task` namespace.

| Metric Name | Type | Unit | Description |
|---|---|---|---|
| `rtos.task.state` | Gauge | Enum | Current execution state. |
| `rtos.task.stack_watermark` | Gauge | By | Remaining bytes in the task stack. |
| `rtos.task.cpu_ratio` | Gauge | Ratio | CPU fraction consumed by this task. |

**Required Attributes:**
- `rtos.task.name`: The string name of the task (e.g., `Idle`, `SensorTask`).
- `rtos.task.id`: (Optional) The memory address or numeric ID of the task handle.

## 2. Memory Namespacing
System-level memory metrics should live under `rtos.heap`.

| Metric Name | Type | Unit | Description |
|---|---|---|---|
| `rtos.heap.free_bytes` | Gauge | By | Current available heap size. |
| `rtos.heap.min_ever_bytes` | Gauge | By | Historical minimum free heap recorded. |
| `rtos.heap.oom_projection` | Gauge | s | Time until projected exhaustion. |

## 3. Telemetry Metadata
Metrics describing the health of the observability link.

| Metric Name | Type | Unit | Description |
|---|---|---|---|
| `rtos.telemetry.packet_loss` | Gauge | Ratio | Percentage of packets dropped by transport. |

## 4. Rationale
By adopting `rtos.*` prefixes instead of generic `system.memory.*`, we prevent collisions with the host OS metrics (Linux/Windows) that might be running the bridge. This allows a single Grafana dashboard to clearly distinguish between the "Embedded Twin" and its "Host Gateway."
