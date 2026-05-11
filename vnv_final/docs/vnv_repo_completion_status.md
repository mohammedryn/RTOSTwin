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
- Prometheus queries return live values for CPU, heap, per-task CPU, stack watermark, packet loss, and OOM projection
- the provisioned Grafana dashboard renders live hardware-backed panels successfully
- measured cadence evidence on the STM32 baseline is `9.52 Hz`
- measured telemetry-cycle CPU overhead on the STM32 baseline is `0.869%`
- measured agent static RAM on the STM32 baseline is `2543 bytes`
- the telemetry hot path passes the no-allocation audit
- supporting screenshots and terminal captures are stored under
  [`evidence/objective1_stm32/`](/D:/digital_twin/evidence/objective1_stm32)

See [`reports/hardware_validation_2026-05-10.md`](reports/hardware_validation_2026-05-10.md)
for the detailed record.

## What still requires caution

These items should still be treated carefully even though the baseline
end-to-end validation is complete:

- the validated run is currently tied to the clean STM32 baseline project `RTOSTwinF401RE_clean`
- other historical STM32 project folders are not part of the validated path
- timing, overhead, and long-duration soak results should only be claimed when re-measured on the exact firmware/hardware revision under test

## Honest completion boundary

From this repository we can now claim:

- the VNV-owned `vnv_final/` software lane is locally reproducible and test-backed
- the mock-to-metrics observability path is in place
- the baseline end-to-end `NUCLEO-F401RE -> UART -> Bridge -> Prometheus -> Grafana` path has been validated on real hardware as of `2026-05-10`
- the STM32 baseline now has measured Objective 1 evidence for cadence, CPU overhead, static RAM, and no-allocation behavior

We cannot honestly claim:

- that every STM32 project variant in the repo is hardware-validated
- that performance, overhead, or reliability results generalize beyond the validated baseline without fresh measurement
