# VNV Repo Completion Status

This document records both:

- what the `vnv_final/` lane can be verified to do from the repository alone
- what has been verified externally on real STM32 hardware

## Verified locally from the repo

The following checks were run against the current `vnv_final/` implementation:

- `python -m pytest vnv_final\bridge\tests -q`
- `python -m compileall vnv_final\bridge`
- `python vnv_final\bridge\mock_device.py --help`
- `python vnv_final\bridge\main.py --help`

The bridge test suite now covers:

- decoder golden-vector behavior
- OOM analyzer gates
- state reconstruction from keyframe and delta packets
- Prometheus metric rendering with the required metric names
- a bridge smoke path from framed mock packet -> decoder -> state manager -> exporters

## Verified behavior

- `mock_device.py` produces valid framed packets for `normal`, `leak`, and `saturated` modes.
- `main.py` accepts `stdin` as a local demo transport source.
- `PrometheusExporter` exposes all 8 required Prometheus metric names.
- OTLP fan-out is wired through the bridge path and is only enabled when `RTOSTWIN_ENABLE_OTLP=1`.
- Docker Compose provisions Prometheus and Grafana for local development in `vnv_final/`.

## Verified externally on real hardware

As of `2026-05-10`, the full hardware-backed path has been demonstrated on a
real `NUCLEO-F401RE`.

Validated hardware evidence includes:

- STM32 firmware project `RTOSTwinF401RE_clean` builds successfully with the telemetry agent linked in
- firmware flashes successfully through ST-LINK with verification completed
- the board streams live packets over `STMicroelectronics STLink Virtual COM Port (COM11)`
- `python bridge/main.py --port COM11 --baud 115200 --device-id nucleo-f401re` decodes packets continuously with `drops=0` and `seq_gaps=0`
- `http://localhost:8000/metrics` exposes live RTOS metrics for `device_id="nucleo-f401re"`
- the bridge OTLP path has been explicitly validated with `RTOSTWIN_ENABLE_OTLP=1` and `OTEL_EXPORTER_OTLP_ENDPOINT=http://localhost:4318/v1/metrics`
- a local OTLP collector received the expected RTOS metric families for both `device_id="mock-stdin"` and `device_id="nucleo-f401re"`
- Prometheus queries return live values for CPU, heap, per-task CPU, stack watermark, packet loss, and OOM projection
- the provisioned Grafana dashboard renders live hardware-backed panels successfully
- measured cadence evidence on the STM32 baseline is `9.52 Hz`
- measured telemetry-cycle CPU overhead on the STM32 baseline is `0.869%`
- measured agent static RAM on the STM32 baseline is `2543 bytes`
- the telemetry hot path passes the no-allocation audit
- the OOM analyzer test suite passes `5/5`
- the OOM analyzer stays stable at `-1.0` for `mock-normal`, detects `mock-leak` with a positive projection, and stays stable at `-1.0` for `mock-saturated`
- the OOM projection metric is exported through OTLP for `device_id="mock-leak-otlp"`
- the real `nucleo-f401re` hardware lane stays stable with `rtos_heap_oom_projection_seconds = -1.0`
- the STM32 baseline also has a recorded soak run from `2026-05-12 03:06:58`
  to `2026-05-12 11:12:40` (`8 hours 5 minutes 42 seconds`) with `97`
  metrics snapshots, `drops=0`, `seq_gaps=0`, stable
  `rtos_heap_oom_projection_seconds = -1.0`, stable packet-loss ratio `0.0`,
  and stable `rtos_heap_free_bytes = 12568.0`
- supporting screenshots and terminal captures are stored under
  [`evidence/objective1_stm32/`](/D:/digital_twin/evidence/objective1_stm32)
- supporting OTLP and Prometheus export evidence is stored under
  [`evidence/objective2_bridge_exports/`](/D:/digital_twin/evidence/objective2_bridge_exports)
- supporting OOM-analyzer validation evidence is stored under
  [`evidence/objective3_oom_validation/`](/D:/digital_twin/evidence/objective3_oom_validation)

See [`reports/hardware_validation_2026-05-10.md`](reports/hardware_validation_2026-05-10.md)
for the detailed record.

## What still requires caution

These items should still be treated carefully even though the baseline
end-to-end validation is complete:

- the validated run is currently tied to the clean STM32 baseline project `RTOSTwinF401RE_clean`
- other historical STM32 project folders are not part of the validated path
- timing, overhead, and soak results should only be claimed for the exact
  firmware/hardware revision under test; broader generalization still needs
  fresh measurement

## Honest completion boundary

From this repository we can now claim:

- the VNV-owned `vnv_final/` software lane is locally reproducible and test-backed
- the mock-to-metrics observability path is in place
- the baseline end-to-end `NUCLEO-F401RE -> UART -> Bridge -> Prometheus -> Grafana` path has been validated on real hardware as of `2026-05-10`
- the STM32 baseline now has measured Objective 1 evidence for cadence, CPU overhead, static RAM, and no-allocation behavior
- the STM32 baseline now also has a saved `8 hour 5 minute` soak evidence set
- the bridge-side Objective 2 export path now has saved OTLP and Prometheus evidence for both mock and real hardware lanes
- the bridge-side Objective 3 OOM analyzer now has saved unit, mock-scenario, OTLP, and real-hardware stability evidence

We cannot honestly claim:

- that every STM32 project variant in the repo is hardware-validated
- that performance, overhead, or reliability results generalize beyond the validated baseline without fresh measurement
