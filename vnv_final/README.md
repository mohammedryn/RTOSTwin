# Validated Implementation Subtree

This subtree contains the current validated RTOSTwin implementation lane.

It is retained under `vnv_final/` for compatibility with:

- validated bridge commands
- local Docker/Grafana/Prometheus bring-up
- hardware validation documents
- Objective 1/2/3 evidence and runbooks

If you want the project overview, milestone status, or evidence links, start at
the repository root [README.md](../README.md).

If you want the currently supported implementation workflow, continue below.

![RTOSTwin Architecture](dashboard/architecture_diagram.png)

## What Lives Here

- `agent/` - MCU-side telemetry implementation
- `bridge/` - Python bridge, exporters, and tests
- `dashboard/` - dashboard assets
- `docs/` - validated implementation quick start and VNV-scoped status
- `semantic-conventions/` - RTOS OpenTelemetry proposal draft
- `docker-compose.yml` - local observability stack bring-up

## Start Here

- [Embeddable agent guide](agent/README.md)
- [Validated quick start](docs/quick_start.md)
- [Hardware validation record](docs/reports/hardware_validation_2026-05-10.md)
- [VNV verification boundary](docs/vnv_repo_completion_status.md)

## Hardware Validation Status

The baseline hardware-to-dashboard path is validated end to end on a real
`NUCLEO-F401RE` as of `2026-05-10`. The validated STM32 firmware project is
`RTOSTwinF401RE_clean`, and the recorded proof path is `ST-LINK VCP -> Python
bridge -> Prometheus -> Grafana`.

See
[`docs/reports/hardware_validation_2026-05-10.md`](docs/reports/hardware_validation_2026-05-10.md)
for the concrete build, flash, serial, and metrics evidence.

## Implementation Features

- Low-overhead embedded telemetry agent with delta encoding
- Prometheus and OTLP exporter support in the host bridge
- OOM trend analysis and projected out-of-memory metrics
- Local Grafana and Prometheus bring-up for reproducible validation

## Workflow Scope

Use this subtree when you want to:

- run the bridge locally
- exercise the mock device path
- bring up the observability stack
- follow the validated STM32 bridge path
- inspect the current semantic-conventions proposal draft
