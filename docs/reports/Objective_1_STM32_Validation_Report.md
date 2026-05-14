# Objective 1 STM32 Validation Report

## Scope

This report closes Objective 1 for the STM32 baseline only:
- Board: `NUCLEO-F401RE`
- Firmware project: `RTOSTwinF401RE_clean`
- Other boards: `ESP32-P4` and `Teensy 4.1` remain future work

## Build Under Test

- Firmware project: `RTOSTwinF401RE_clean`
- Firmware build configuration: `Debug`
- MCU clock: `84 MHz`
- FreeRTOS tick rate: not separately re-recorded in this validation pass
- Telemetry task delay: `vTaskDelay(pdMS_TO_TICKS(100))`
- Bridge command: `python vnv_final/bridge/main.py --port COM11 --baud 115200 --device-id nucleo-f401re`

## Cadence Evidence

- Measurement window: `63 seconds` from saved bridge logs
- Packet count delta: `600 packets`
- Measured cadence: `9.52 Hz`
- Expected cadence: `10 Hz` with acceptance band `9.5 Hz` to `10.5 Hz`
- Verdict: `PASS`

Bridge evidence used:

- `T0 = 110 packets` at `22:49:07`
- `T1 = 710 packets` at `22:50:10`
- `drops=0`
- `seq_gaps=0`

Evidence artifacts:

- Screenshot: [03_cadence_60s.png](/D:/digital_twin/evidence/objective1_stm32/03_cadence_60s.png)
- Terminal log: [03_cadence_60s.txt](/D:/digital_twin/evidence/objective1_stm32/03_cadence_60s.txt)

## CPU Overhead Evidence

- Snapshot min cycles: `58644`
- Snapshot max cycles: `70770`
- Snapshot mean cycles: `58998`
- Telemetry-cycle min cycles: `71750`
- Telemetry-cycle max cycles: `98624`
- Telemetry-cycle mean cycles: `72987`
- Mean telemetry-cycle microseconds: `868.9 us`
- CPU overhead at 10 Hz: `0.869%`
- Verdict: `PASS`

Evidence artifacts:

- Screenshot: [04_timing_capture.png](/D:/digital_twin/evidence/objective1_stm32/04_timing_capture.png)
- Terminal log: [04_timing_capture.txt](/D:/digital_twin/evidence/objective1_stm32/04_timing_capture.txt)

Cycle conversion formula:

```text
microseconds = mean_cycles / 84
cpu_overhead_ratio = microseconds / 100000
cpu_overhead_percent = cpu_overhead_ratio * 100
```

The STM32F401RE validated baseline runs at 84 MHz, so 84 cycles equal
1 microsecond. At 10 Hz, the telemetry cycle has a 100000 microsecond budget
per iteration.

## Static RAM Evidence

Static RAM is counted as:

`agent_static_ram_bytes = sum(.data for agent object files) + sum(.bss for agent object files)`

For this milestone, agent object files are:
- `telemetry_agent.o`
- `measurement.o`
- `snapshot.o`
- `encoder.o`
- `framer.o`
- `transport.o`
- `profiler.o`
- `hooks.o`
- `dwt.o`
- `uart_dma.o`

Whole-firmware `.data` and `.bss` are recorded separately for context, but the
Objective 1 pass/fail check is performed against the agent-specific total.

- Whole firmware `text`: `32468`
- Whole firmware `data`: `96`
- Whole firmware `bss`: `23416`
- Agent `.data` bytes: `0`
- Agent `.bss` bytes: `2543`
- Agent static RAM total: `2543`
- Pass/Fail against `< 10240 bytes`: `PASS`

Evidence artifacts:

- Screenshot: [05_ram_parser.png](/D:/digital_twin/evidence/objective1_stm32/05_ram_parser.png)
- Terminal log: [05_ram_parser.txt](/D:/digital_twin/evidence/objective1_stm32/05_ram_parser.txt)

## No-Allocation Proof

Hot-path audit files:
- `vnv_final/agent/main.c`
- `vnv_final/agent/core/snapshot.c`
- `vnv_final/agent/core/encoder.c`
- `vnv_final/agent/core/framer.c`
- `vnv_final/agent/core/transport.c`

Audit command:
- `powershell -ExecutionPolicy Bypass -File tools/stm32/no_malloc_audit.ps1`

Audit result:
- no `malloc`, `calloc`, `realloc`, `free`, `pvPortMalloc`, or `pvPortFree`
  calls were found in the telemetry hot path
- Verdict: `PASS`

Evidence artifacts:

- Screenshot: [06_no_malloc_audit.png](/D:/digital_twin/evidence/objective1_stm32/06_no_malloc_audit.png)
- Terminal log: [06_no_malloc_audit.txt](/D:/digital_twin/evidence/objective1_stm32/06_no_malloc_audit.txt)

## Soak Run Protocol

- Board: `NUCLEO-F401RE`
- Firmware: `RTOSTwinF401RE_clean`
- Serial path: `STMicroelectronics STLink Virtual COM Port`
- Bridge command: `python vnv_final/bridge/main.py --port COM11 --baud 115200 --device-id nucleo-f401re`
- Metrics endpoint: `http://localhost:8000/metrics`
- Prometheus UI: `http://localhost:9090`
- Grafana UI: `http://localhost:3000`
- Minimum duration for closure: `6 hours`
- Preferred duration for stronger evidence: `24 hours`

## Soak Run Outcome

| Field | Result |
|---|---|
| Start time | `2026-05-12 03:06:58` |
| End time | `2026-05-12 11:12:40` |
| Duration | `8 hours 5 minutes 42 seconds` |
| Firmware alive throughout | `PASS` |
| Bridge alive throughout | `PASS` |
| Packet loss result | `PASS` (`drops=0`, `seq_gaps=0`, metrics packet-loss ratio `0.0`) |
| Heap free stability | `PASS` (`12568.0` throughout the sampled summary tail) |
| Heap min-ever stability | `Not separately recorded in the compact soak summary` |
| OOM projection stability | `PASS` (`-1.0` throughout the sampled summary tail) |
| Final verdict | `PASS` |

Soak evidence highlights:

- bridge log remained active until manual stop, with packet counts still
  increasing near the end of the run
- bridge tail remained at `drops=0` and `seq_gaps=0`
- compact metrics summary stayed stable across the tail window:
  - `rtos_heap_oom_projection_seconds{device_id="nucleo-f401re"} = -1.0`
  - `rtos_telemetry_packet_loss_ratio{device_id="nucleo-f401re"} = 0.0`
  - `rtos_heap_free_bytes{device_id="nucleo-f401re"} = 12568.0`
- snapshot capture count: `97` files in `metrics_snapshots/`

Soak evidence artifacts:

- Metadata: [00_soak_metadata.txt](/D:/digital_twin/evidence/objective1_stm32/soak_2026-05-12/00_soak_metadata.txt)
- Bridge log: [01_bridge_soak_log.txt](/D:/digital_twin/evidence/objective1_stm32/soak_2026-05-12/01_bridge_soak_log.txt)
- Metrics summary: [02_metrics_snapshot_summary.txt](/D:/digital_twin/evidence/objective1_stm32/soak_2026-05-12/02_metrics_snapshot_summary.txt)
- Start bridge screenshot: [01_bridge_started.png](/D:/digital_twin/evidence/objective1_stm32/soak_2026-05-12/screenshots/01_bridge_started.png)
- Start metrics-loop screenshot: [02_metrics_snapshot_loop.png](/D:/digital_twin/evidence/objective1_stm32/soak_2026-05-12/screenshots/02_metrics_snapshot_loop.png)
- Start metrics-summary screenshot: [03_metrics_summary_started.png](/D:/digital_twin/evidence/objective1_stm32/soak_2026-05-12/screenshots/03_metrics_summary_started.png)
- Soak metadata screenshot: [04_soak_metadata.png](/D:/digital_twin/evidence/objective1_stm32/soak_2026-05-12/screenshots/04_soak_metadata.png)
- End bridge screenshot: [05_bridge_end.png](/D:/digital_twin/evidence/objective1_stm32/soak_2026-05-12/screenshots/05_bridge_end.png)
- End metrics-summary screenshot: [06_metrics_summary_end.png](/D:/digital_twin/evidence/objective1_stm32/soak_2026-05-12/screenshots/06_metrics_summary_end.png)
- Snapshot-count screenshot: [07_snapshot_count.png](/D:/digital_twin/evidence/objective1_stm32/soak_2026-05-12/screenshots/07_snapshot_count.png)

## Final Objective 1 Verdict

`Objective 1` is technically achieved for the STM32 baseline on
`NUCLEO-F401RE`.

Measured closure evidence:

- cadence: `9.52 Hz` over the captured bridge window, `PASS`
- CPU overhead: `0.869%` at `10 Hz`, `PASS`
- static RAM: `2543 bytes`, `PASS`
- dynamic-allocation audit: `PASS`

This report closes the core engineering acceptance criteria for the STM32
baseline only. `ESP32-P4` and `Teensy 4.1` remain future work. The minimum
`6-hour` soak requirement is now satisfied by the recorded `8 hour 5 minute`
STM32 soak run, so the STM32 baseline can be treated as formally signoff-ready
within the current closure runbook.
