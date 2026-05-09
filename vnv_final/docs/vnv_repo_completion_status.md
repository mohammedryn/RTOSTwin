# VNV Repo Completion Status

This document records what the `vnv_final/` lane can be verified to do from the
repository alone, and what still requires external STM32 hardware proof.

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

## Still not claimable without external evidence

These items require real board access and should not be claimed as complete from repo-only verification:

- flashing firmware to `NUCLEO-F401RE`
- proving UART/DMA packets are visible on the wire from the actual board
- confirming the real serial stream reaches the bridge
- capturing a Grafana screenshot with live hardware-backed data
- recording the exact STM32 build/flash command and serial-port setup used for the successful run

## Honest completion boundary

From this repository we can now claim:

- the VNV-owned `vnv_final/` software lane is locally reproducible and test-backed
- the mock-to-metrics observability path is in place
- the remaining gap is the final external hardware proof step

We cannot honestly claim:

- end-to-end `NUCLEO-F401RE -> UART -> Bridge -> Prometheus -> Grafana` has been physically demonstrated

until that evidence is produced.
