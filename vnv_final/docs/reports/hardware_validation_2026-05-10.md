# NUCLEO-F401RE Hardware Validation - 2026-05-10

This report records the first complete end-to-end validation of the RTOSTwin
hardware-to-dashboard path on a real STM32 target.

## Goal

Validate the full `NUCLEO-F401RE -> UART -> Python bridge -> Prometheus ->
Grafana` path on real hardware, including live RTOS metrics for:

- task states
- CPU usage
- stack watermark
- heap status
- packet loss
- OOM projection

## Validated setup

- Board: `NUCLEO-F401RE`
- MCU family detected by ST-LINK: `STM32F401xD/E`
- Firmware project: `RTOSTwinF401RE_clean`
- Host bridge path: `D:\digital_twin\vnv_final\bridge`
- Serial transport: `STMicroelectronics STLink Virtual COM Port (COM11)`
- Bridge command:

```bash
python bridge/main.py --port COM11 --baud 115200 --device-id nucleo-f401re
```

- Prometheus metrics endpoint: `http://localhost:8000/metrics`
- Prometheus UI: `http://localhost:9090`
- Grafana UI: `http://localhost:3000`

## Firmware build evidence

The clean STM32 firmware project built successfully with the telemetry agent
linked in:

```text
text = 32936
data = 108
bss = 23388
total dec = 56432
```

This confirms the real telemetry firmware was built, not the earlier tiny
placeholder image.

## Flash and transport evidence

The board was flashed successfully through ST-LINK with verification completed.
Observed identifiers during the successful run:

- Board: `NUCLEO-F401RE`
- Device: `STM32F401xD/E`
- Programming completed successfully
- Verification completed successfully

The host bridge opened `COM11 @ 115200` and continuously decoded packets from
the board. Representative bridge log output:

```text
110 packets received | drops=0 | seq_gaps=0
208 packets received | drops=0 | seq_gaps=0
```

Packet counts continued upward into the thousands with no observed sequence
gaps or packet loss.

## Verified live metrics

The Prometheus `/metrics` endpoint exposed live RTOS telemetry for
`device_id="nucleo-f401re"`, including:

- `rtos_cpu_utilization_ratio`
- `rtos_task_cpu_ratio`
- `rtos_heap_free_bytes`
- `rtos_heap_min_ever_bytes`
- `rtos_task_stack_watermark_bytes`
- `rtos_telemetry_packet_loss_ratio`
- `rtos_heap_oom_projection_seconds`
- task state metrics

The Grafana dashboard `RTOSTwin Digital Twin` rendered live data for:

- Total CPU Utilization
- Heap Memory Trends
- OOM Projection
- Stack High Watermarks
- Task CPU Distribution
- Packet Loss Ratio

## Observed metric values

Representative observed values from the validated hardware run:

- `rtos_cpu_utilization_ratio = 1`
- `rtos_heap_free_bytes = 12568`
- `rtos_heap_min_ever_bytes = 12568`
- `rtos_telemetry_packet_loss_ratio = 0`
- `rtos_heap_oom_projection_seconds = -1`

Per-task CPU and stack telemetry was confirmed for:

- `IDLE`
- `TelemetryTask`
- `defaultTask`
- `Tmr Svc`

Confirmed stack watermark values:

- `IDLE = 424 B`
- `TelemetryTask = 1560 B`
- `Tmr Svc = 856 B`
- `defaultTask = 344 B`

Interpretation:

- CPU profiling is flowing end to end.
- Heap usage is stable so far; the minimum-ever heap matches current free heap.
- No leak trend is currently detected, so OOM projection remains `-1`.
- No packet loss has been detected in the live transport path.
- Task names and task-level metrics are being exported correctly.

## Technical milestone

This validation proves the following components are working together on real
hardware:

- STM32 firmware task snapshot capture
- task/runtime profiling
- heap measurement
- stack watermark export
- telemetry framing and transport
- UART DMA output path
- serial ingest on the host
- packet decoding in Python
- device registry and state update
- Prometheus metric export
- Grafana dashboard visualization

## Project path decision

The STM32 project to keep as the validated hardware baseline is:

- `RTOSTwinF401RE_clean`

The following earlier STM32 projects should be treated as obsolete bring-up
artifacts:

- `RTOSTwinF401RE`
- `textCubeProject`
- `First`

## Final milestone statement

"The RTOSTwin system has been validated end to end on real hardware using the
STM32 NUCLEO-F401RE. Firmware was successfully built and flashed, telemetry
streamed over the ST-LINK virtual COM port, the Python bridge decoded packets
without loss, Prometheus exported live RTOS metrics, and the Grafana dashboard
rendered live CPU, heap, stack, packet-loss, and OOM-projection telemetry for
the device `nucleo-f401re`."
