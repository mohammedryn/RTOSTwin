# VNV Checklist

## Purpose

This checklist tells `VNV` exactly what to do next so the project becomes a real end-to-end STM32 implementation, not just a protocol/spec prototype.

The target outcome is:

`NUCLEO-F401RE` -> `vnv_final/agent/*` sends live packets -> `vnv_final/bridge/main.py` decodes them -> Prometheus/Grafana shows live RTOS metrics.

This file is the practical handoff. Follow it in order. Do not skip steps.

---

## Before You Start

- [ ] Pull the latest `main`
- [ ] Create a new branch from latest `main`
- [ ] Work only in `vnv_final/` unless Rayan explicitly asks for a root-level change
- [ ] Read these files before coding:
  - `docs/wire_format_spec.md`
  - `agent/core/wire_format.h`
  - `roles/vnv_role_assignment.md`
  - `roles/ryn_role_assignment.md`
  - `roles/rayan_checklist.md`

---

## Non-Negotiable Rules

- [ ] Do **not** recreate `vnv_final/docs/wire_format_spec.md`
- [ ] Do **not** recreate `vnv_final/agent/core/wire_format.h`
- [ ] Do **not** invent a second packet format inside `vnv_final/`
- [ ] Do **not** parse raw packet bytes inside exporters or dashboards
- [ ] Do **not** change protocol field sizes, CRC rules, packet tags, or header layout without syncing with Rayan first
- [ ] Do **not** commit `__pycache__/`, `.pyc`, temp logs, random exports, or duplicate reports

The only protocol source of truth is:

- `docs/wire_format_spec.md`
- `agent/core/wire_format.h`

---

## What Is Already Done For You

- [x] `vnv_final/` has been cleaned
- [x] duplicate VNV-local protocol files were removed
- [x] VNV framer/encoder/bridge code was aligned to the frozen Phase 1 protocol
- [x] `python -m pytest vnv_final\bridge\tests -q` passes on current `main`

This means your next job is not “fix protocol drift.”

Your next job is to finish the **real STM32 baseline path** and make the observability stack run live.

---

## Your Ownership

You own these implementation paths inside `vnv_final/`:

- `agent/core/framer.*`
- `agent/core/encoder.*`
- `agent/core/transport.*`
- `agent/hal/stm32/uart_dma.*`
- `agent/main.c`
- `bridge/state_manager.py`
- `bridge/device_registry.py`
- `bridge/main.py`
- `bridge/prometheus_exporter.py`
- `bridge/otlp_exporter.py`
- `bridge/mock_device.py`
- `dashboard/rtostwin_dashboard.json`
- `docker-compose.yml`
- `prometheus/*`
- `grafana/*`
- `docs/quick_start.md`
- `README.md`

You do **not** own:

- `docs/wire_format_spec.md`
- `agent/core/wire_format.h`
- `agent/core/snapshot.*`
- `agent/core/profiler.*`
- `bridge/decoder.py`
- `bridge/oom_analyzer.py`

If you discover a bug in a non-owned file, write it down clearly and hand it back instead of silently redesigning it.

---

## Main Goal

Make `vnv_final/` demo-ready on the real `NUCLEO-F401RE`.

Success means all of this is true:

- [ ] STM32 firmware builds
- [ ] telemetry task runs at 10 Hz
- [ ] UART/DMA sends real framed packets
- [ ] host bridge reads them successfully
- [ ] state manager reconstructs device state correctly
- [ ] Prometheus exporter shows live metrics
- [ ] Grafana dashboard renders live values
- [ ] setup steps are documented cleanly enough that Rayan can reproduce them

---

## Phase A - Finish the MCU Data Path

Files:

- `vnv_final/agent/core/framer.*`
- `vnv_final/agent/core/encoder.*`
- `vnv_final/agent/core/transport.*`
- `vnv_final/agent/hal/stm32/uart_dma.*`
- `vnv_final/agent/main.c`

Checklist:

- [ ] Re-read `docs/wire_format_spec.md` and verify your code still matches the frozen header and CRC rules
- [ ] Make sure `frame_packet()` uses the frozen v1 signature exactly
- [ ] Make sure `encoder_encode()` and `encoder_last_was_keyframe()` behave correctly on keyframe vs delta paths
- [ ] Make sure transport is non-blocking and handles DMA-busy packet drops cleanly
- [ ] Make sure `vnv_final/agent/main.c` wires together:
  - snapshot capture
  - encoder
  - framer
  - transport
  - telemetry loop at 10 Hz
- [ ] Make sure the STM32 baseline target is `NUCLEO-F401RE`, not an imaginary generic STM32

What to prove before pushing:

- [ ] framed packets are actually transmitted over UART
- [ ] no duplicate protocol constants exist locally
- [ ] sequence numbers visible on the wire increase correctly
- [ ] packet type flips correctly between keyframe and delta

---

## Phase B - Make the Host Bridge Work From Live Data

Files:

- `vnv_final/bridge/state_manager.py`
- `vnv_final/bridge/device_registry.py`
- `vnv_final/bridge/main.py`
- `vnv_final/bridge/mock_device.py`

Checklist:

- [ ] `state_manager.py` must consume `DecodedPacket`, not raw framed bytes
- [ ] if any raw-payload interpretation remains, keep it strictly aligned with the current typed decoder contract
- [ ] `device_registry.py` must keep per-device state isolated correctly
- [ ] `main.py` must read from serial cleanly and continuously
- [ ] support a simple local demo path using either:
  - real serial port from STM32, or
  - `stdin` / mock mode for quick validation
- [ ] `mock_device.py` must stay aligned with the frozen protocol and keep these modes working:
  - `normal`
  - `leak`
  - `saturated`

What to prove before pushing:

- [ ] real or mock packets flow through `main.py` without crashing
- [ ] state reconstruction updates over time
- [ ] multiple packets in a row are handled correctly
- [ ] no hard-coded assumptions remain from the old packet layout

---

## Phase C - Finish Observability Output

Files:

- `vnv_final/bridge/prometheus_exporter.py`
- `vnv_final/bridge/otlp_exporter.py`
- `vnv_final/dashboard/rtostwin_dashboard.json`
- `vnv_final/docker-compose.yml`
- `vnv_final/prometheus/prometheus.yml`
- `vnv_final/grafana/provisioning/*`

Checklist:

- [ ] Prometheus exporter must expose stable metric names and labels
- [ ] bridge main loop must call the exporter update path correctly
- [ ] OTLP exporter must not break local development if endpoint config is absent
- [ ] dashboard panels must match the actual exposed metric names
- [ ] Docker/Prometheus/Grafana setup must boot without hand-editing five files

What to prove before pushing:

- [ ] `curl` or browser shows `/metrics` output
- [ ] Prometheus scrapes successfully
- [ ] Grafana dashboard loads and displays real values
- [ ] if OTLP is enabled, it does not crash the bridge

---

## Phase D - Real STM32 Demo Proof

This is the most important phase.

You are not done until you have evidence for this path:

1. Flash firmware to `NUCLEO-F401RE`
2. Connect board to host
3. Run bridge
4. Receive live packets
5. See live metrics in Prometheus/Grafana

Checklist:

- [ ] record exact board setup steps
- [ ] record exact serial port settings
- [ ] record exact command used to run the bridge
- [ ] record exact command used to start Prometheus/Grafana stack
- [ ] capture at least one screenshot of Grafana with live data
- [ ] capture at least one log snippet showing packets or decoded updates

This is the evidence Rayan needs before honestly claiming end-to-end STM32 delivery.

---

## Required Verification Before You Push

Run these first and include the results in your handoff note:

- [ ] `python -m pytest vnv_final\bridge\tests -q`
- [ ] `python -m compileall vnv_final\bridge`
- [ ] `python vnv_final\bridge\mock_device.py --help`
- [ ] `python vnv_final\bridge\main.py --help`

Also run any STM32 build/flash command you are using locally and include whether it passed.

If you add new tests, list them clearly.

---

## What To Push Back For Review

When you are ready, push:

- [ ] the branch with all code changes
- [ ] any updated docs needed to reproduce the demo
- [ ] dashboard/provisioning changes
- [ ] a short handoff note containing:
  - what files you changed
  - what commands you ran
  - what passed
  - what still does not work
  - whether the real `NUCLEO-F401RE` path was tested
  - what Rayan should review first

Do **not** send “done” without that evidence.

---

## Minimum Acceptable Handoff Message

Use this structure when you send work back:

```md
## VNV Handoff

### Branch
- <branch-name>

### Files changed
- <file 1>
- <file 2>

### Verification run
- `python -m pytest vnv_final\bridge\tests -q` -> <result>
- `python -m compileall vnv_final\bridge` -> <result>
- `<stm32 build command>` -> <result>
- `<flash/run/demo command>` -> <result>

### Real hardware status
- Tested on `NUCLEO-F401RE`: yes/no
- Serial packets received: yes/no
- Grafana live metrics visible: yes/no

### Known issues
- <issue 1>
- <issue 2>
```

---

## Final Definition Of Done

You are done only when Rayan can pull your branch and verify that:

- [ ] `vnv_final/` still obeys the root protocol truth
- [ ] bridge tests still pass
- [ ] live or reproducible demo path exists
- [ ] STM32 baseline path is real, not just mock-only
- [ ] Grafana/Prometheus integration is visible and reviewable

If any one of those is missing, the work is still in progress.
