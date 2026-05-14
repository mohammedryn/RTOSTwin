# RTOS Semantic Conventions
**Status:** Experimental

This document defines semantic conventions for Real-Time Operating System (RTOS) metrics. These conventions allow for standardized monitoring of task-level and system-level health on microcontrollers running RTOS kernels like FreeRTOS, Zephyr, or ThreadX.

## Proposal Surface Decision

This proposal standardizes **OpenTelemetry-style semantic attribute names**,
not the bridge's current implementation labels.

The current bridge implementation emits attributes such as `device_id` and
`task_name` in its local OTLP and Prometheus exporters. Those names are treated
as an implementation detail of the first RTOSTwin bridge, not as the proposed
cross-project standard.

For the formal OpenTelemetry proposal, the standardized attribute names are:

- `device.id`
- `task.name`
- `state`

This keeps the proposal aligned with OpenTelemetry semantic naming style and
makes it suitable for future RTOS exporters beyond this repository.

### Compatibility Mapping

| Current RTOSTwin Implementation | Proposed Semantic Convention |
| :--- | :--- |
| `device_id` | `device.id` |
| `task_name` | `task.name` |
| task state numeric value in exporter output | `rtos.task.state` with `state` attribute |

## 1. Metric Namespaces
All RTOS metrics MUST be prefixed with `rtos.`.

## 2. System Metrics
These metrics track the overall health of the RTOS kernel and memory.

| Name | Instrument | Unit | Description |
| :--- | :--- | :--- | :--- |
| `rtos.heap.free_bytes` | Gauge | `By` | The amount of free memory currently available in the RTOS heap. |
| `rtos.heap.min_ever_bytes` | Gauge | `By` | The historical minimum amount of free heap memory observed since boot. |
| `rtos.heap.oom_projection_seconds` | Gauge | `s` | Projected seconds until the heap is exhausted, based on trend analysis. `-1` indicates stability. |
| `rtos.cpu.utilization_ratio` | Gauge | `1` | Total CPU utilization as a fraction between `0.0` and `1.0`. |
| `rtos.telemetry.packet_loss_ratio` | Gauge | `1` | Fraction of telemetry packets dropped due to transport saturation or CRC errors. |

## 3. Task Metrics
These metrics provide visibility into individual RTOS tasks. All task metrics
MUST include the `task.name` attribute.

| Name | Instrument | Unit | Description |
| :--- | :--- | :--- | :--- |
| `rtos.task.state` | Gauge | `1` | A value of `1` if the task is in the state specified by the `state` attribute, `0` otherwise. |
| `rtos.task.stack_watermark` | Gauge | `By` | The high-water mark of stack usage. This represents the minimum free stack space ever observed for this task. |
| `rtos.task.cpu_ratio` | Gauge | `1` | The fraction of CPU time consumed by this task relative to the total system runtime. |

## 4. Attributes

### 4.1 `device.id` (Required)
The unique identifier for the physical device sending the telemetry.
- **Type**: `string`
- **Example**: `COM3`, `STM32-01-FF22`

### 4.2 `task.name` (Required for Task Metrics)
The human-readable name of the task.
- **Type**: `string`
- **Example**: `Blinky`, `SensorTask`, `IDLE`

### 4.3 `state` (Required for `rtos.task.state`)
The operational state of the task.
- **Type**: `enum`
- **Values**:
  - `running`: The task is currently executing.
  - `ready`: The task is ready to run but waiting for the scheduler.
  - `blocked`: The task is waiting for an event (semaphore, queue, etc.).
  - `suspended`: The task has been explicitly suspended.
  - `deleted`: The task has been terminated.

### 4.4 State Encoding Rule

`rtos.task.state` is proposed as a **one-hot gauge family**:

- each emitted data point MUST include `device.id`, `task.name`, and `state`
- the gauge value MUST be `1` for the active task state
- the gauge value MUST be `0` for all other enumerated states

This form is preferred over exporting a raw numeric task-state enum because it
is easier to query, aggregate, and visualize consistently across backends.

## 5. Example
An OTLP metric payload for `rtos.task.stack_watermark`:
```json
{
  "name": "rtos.task.stack_watermark",
  "unit": "By",
  "data": {
    "dataPoints": [
      {
        "attributes": [
          { "key": "device.id", "value": { "stringValue": "STM32-01" } },
          { "key": "task.name", "value": { "stringValue": "Blinky" } }
        ],
        "asInt": 512
      }
    ]
  }
}
```
