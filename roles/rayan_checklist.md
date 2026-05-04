# Rayan Checklist

## Immediate Things To Do

- [ ] Read `PRD/roles/ryn_role_assignment.md`
- [ ] Read `PRD/ARCHITECTURE.md`
- [ ] Read `PRD/TECH_SPEC.md`
- [ ] Read `PRD/TASK_QUEUE.md`
- [ ] Inspect `agent/core/snapshot.h`
- [ ] Inspect `agent/core/snapshot.c`
- [ ] Inspect `agent/freertos/hooks.c`
- [ ] Inspect `agent/core/framer.h`
- [ ] Inspect `agent/main.c`
- [ ] Inspect `bridge/decoder.py`
- [ ] Inspect `bridge/tests/test_oom_analyzer.py`
- [x] Create `docs/wire_format_spec.md`
- [x] Freeze packet/framer contract before any wider fixes

## Next thing to do

Phase 2 - stabilize `agent/core/snapshot.*`, remove duplicate idle-hook ownership, and make `sequence_num` population real in the implementation.

---

## Purpose

This checklist is the working plan for Rayan's ownership in `RTOSTwin`.

Rayan owns the protocol truth, snapshot/profiler baseline, typed decoder boundary, and OOM analytics contract. The goal is to make the `RYN` side stable enough that `VNV` can build the rest of the bridge and dashboard without guessing.

---

## Your Ownership

Rayan owns these files and interfaces:

- `agent/core/snapshot.*`
- `agent/core/profiler.*`
- `docs/wire_format_spec.md`
- `agent/core/wire_format.h`
- `bridge/decoder.py`
- `bridge/oom_analyzer.py`

Rayan does **not** own these files:

- `agent/core/framer.*`
- `agent/core/encoder.*`
- `agent/core/transport.*`
- `agent/hal/stm32/uart_dma.c`
- `bridge/state_manager.py`
- `bridge/prometheus_exporter.py`
- `bridge/otlp_exporter.py`
- `bridge/device_registry.py`
- `bridge/main.py`
- `bridge/mock_device.py`
- `dashboard/rtostwin_dashboard.json`

If a fix touches a non-owned file, first convert it into a contract handoff or review note unless the change is explicitly coordinated.

---

## Current Reality You Must Work From

- Canonical repo root: `d:\digital_twin`
- First real hardware target: `NUCLEO-F401RE`
- Next targets after baseline works:
  - `ESP32-P4-Function-EV-Board`
  - `Teensy 4.1`

Important current blockers already known:

- `docs/wire_format_spec.md` does not exist
- `agent/core/snapshot.h` declares `sequence_num`, but `agent/core/snapshot.c` does not populate it
- `vApplicationIdleHook()` exists in both:
  - `agent/core/snapshot.c`
  - `agent/freertos/hooks.c`
- `agent/main.c` and `agent/core/framer.h` disagree on `frame_packet()` usage
- `bridge/decoder.py` returns raw payload bytes, while `bridge/state_manager.py` expects typed fields
- `bridge/oom_analyzer.py` does not match the tested API in `bridge/tests/test_oom_analyzer.py`

---

## Read Order

Read these in this exact order before making large changes.

### 1. Role and ownership

- `PRD/roles/ryn_role_assignment.md`

Read this for:

- exact ownership
- work order
- protocol freeze expectations
- handoff boundaries with `VNV`

### 2. System architecture

- `PRD/ARCHITECTURE.md`

Read this for:

- MCU-side module flow
- host-side module flow
- transport expectations
- where your output is consumed next

### 3. Technical spec

- `PRD/TECH_SPEC.md`

Read this for:

- snapshot struct truth
- packet header layout
- CRC rules
- delta encoding format
- decoder API expectations
- OOM analyzer API expectations

### 4. Implementation order

- `PRD/TASK_QUEUE.md`

Read this for:

- original intended build order
- gate definitions
- what counts as done

### 5. Current agent-side implementation

- `agent/core/snapshot.h`
- `agent/core/snapshot.c`
- `agent/freertos/hooks.c`
- `agent/core/profiler.h`
- `agent/core/profiler.c`
- `agent/core/wire_format.h`
- `agent/core/framer.h`
- `agent/main.c`

Read these for:

- current contract vs current implementation
- missing fields
- duplicated ownership
- signature drift

### 6. Current host-side contract boundary

- `bridge/decoder.py`
- `bridge/tests/test_decoder.py`
- `bridge/tests/test_oom_analyzer.py`
- `bridge/oom_analyzer.py`

Read these for:

- current decoded packet shape
- actual tested expectations
- current API drift

---

## What You Must Understand Before Coding

Before you start editing, you should be able to answer these clearly:

- Who owns packet sequence numbering: snapshot or framer?
- What is the exact `frame_packet()` signature for v1?
- What is the exact layout of a keyframe payload?
- What is the exact layout of a delta payload?
- What fields must `DecodedPacket` expose to the rest of the bridge?
- What exact API must `OOMAnalyzer` implement?
- Where should idle-hook CPU accounting live?

If you cannot answer these, do not jump into broad code edits yet.

---

## Work Plan

### Phase 1 - Freeze the Protocol Contract

### Goal

Create the protocol truth so nobody else has to guess.

### Files

- `docs/wire_format_spec.md`
- `agent/core/wire_format.h`

### Tasks

- [x] Create `docs/wire_format_spec.md`
- [x] Document packet sync bytes
- [x] Document version field
- [x] Document packet types
- [x] Document endianness
- [x] Document CRC settings and test vector
- [x] Document header field offsets and sizes
- [x] Document keyframe payload layout
- [x] Document delta payload layout
- [x] Document `DEVICE_INFO` packet
- [ ] Document units:
  - stack watermark
  - timestamp
  - runtime counters
- [x] Decide and document sequence ownership: snapshot-owned in v1
- [x] Align `agent/core/wire_format.h` with the written spec
- [x] Record 3 golden packet vectors: keyframe, delta, corrupted

### Key decision to resolve here

There is current contract drift:

- `PRD/TECH_SPEC.md` says framer owns sequence internally
- `PRD/ARCHITECTURE.md` shows external `seq`
- `agent/core/framer.h` currently takes external `seq_num`
- `agent/main.c` currently calls `frame_packet()` incorrectly

You must freeze the v1 truth here before wider integration.

### Handoff to VNV after Phase 1

Give `VNV`:

- final `docs/wire_format_spec.md`
- updated `agent/core/wire_format.h`
- final `frame_packet()` contract
- 1 golden keyframe example
- 1 golden delta example

Do **not** let wider framer or bridge integration continue on guesswork.

---

### Phase 2 - Stabilize Snapshot and Profiler Baseline

### Goal

Make the MCU truth model clean and internally consistent on `NUCLEO-F401RE`.

### Files

- `agent/core/snapshot.h`
- `agent/core/snapshot.c`
- `agent/core/profiler.h`
- `agent/core/profiler.c`
- `agent/freertos/hooks.c`

### Tasks

- [ ] Ensure `snapshot_capture()` fully matches `full_snapshot_t`
- [ ] Decide whether `sequence_num` remains in snapshot
- [ ] If it remains, populate and increment it correctly
- [ ] Fix duplicate idle-hook ownership
- [ ] Keep only one `vApplicationIdleHook()` implementation
- [ ] Make CPU-util accounting coherent
- [ ] Keep static allocation only
- [ ] Keep profiler API aligned with spec
- [ ] Keep STM32 baseline timing measurable

### Immediate issues to resolve here

- `agent/core/snapshot.h` has `sequence_num`
- `agent/core/snapshot.c` never sets it
- `agent/core/snapshot.c` defines `vApplicationIdleHook()`
- `agent/freertos/hooks.c` also defines `vApplicationIdleHook()`

### Handoff to VNV after Phase 2

Give `VNV`:

- final snapshot field semantics
- final CPU-util semantics
- decision on `sequence_num`
- confirmation that the snapshot side is stable enough for encoder/framer integration

---

### Phase 3 - Define the Typed Decoder Boundary

### Goal

Convert raw bytes into a stable structured packet contract.

### Files

- `bridge/decoder.py`

### Tasks

- [ ] Keep CRC and framing logic correct
- [ ] Define typed `DecodedPacket` output for keyframes
- [ ] Define typed `DecodedPacket` output for deltas
- [ ] Make timestamp naming consistent
- [ ] Define how task deltas are represented
- [ ] Define how system-field deltas are represented
- [ ] Keep compatibility with the frozen v1 protocol

### `DecodedPacket` must not remain only raw payload

Right now `bridge/state_manager.py` expects fields such as:

- `heap_free_bytes`
- `heap_min_ever_bytes`
- `cpu_utilization_pct`
- `tasks`
- `task_deltas`
- `timestamp_ticks`

Your decoder contract must make this possible in a clean, typed way.

### Handoff to VNV after Phase 3

Give `VNV`:

- exact `DecodedPacket` schema
- one parsed keyframe example
- one parsed delta example
- exact field names to consume in `state_manager.py`

After this, `VNV` should implement state reconstruction against your object model, not raw bytes.

---

### Phase 4 - Fix the OOM Analyzer Contract

### Goal

Make the OOM analyzer match the actual tested API and intended design.

### Files

- `bridge/oom_analyzer.py`
- `bridge/tests/test_oom_analyzer.py`

### Tasks

- [ ] Align constructor with tested/spec API
- [ ] Support `min_r_squared`
- [ ] Support `rolling_min_threshold`
- [ ] Support `total_heap_bytes`
- [ ] Make `add_sample(timestamp_s, heap_free_bytes)` match the spec
- [ ] Implement linear regression detector
- [ ] Implement rolling minimum detector
- [ ] Expose:
  - `regression_slope_bytes_per_second`
  - `rolling_minimum_bytes`
  - `r_squared`
- [ ] Return `-1.0` when stable or insufficient data

### Handoff to VNV after Phase 4

Give `VNV`:

- final OOM API
- expected return semantics
- exactly what state field to pass into OOM
- whether any derived OOM metric should go into Prometheus/Grafana

---

### Phase 5 - Baseline Integration Review

### Goal

Review the integration points without taking over VNV-owned files.

### Tasks

- [ ] Check that `state_manager.py` consumes your `DecodedPacket` schema correctly
- [ ] Check that `bridge/main.py` feeds OOM with the correct API
- [ ] Check that metric names remain consistent with `PRD/TECH_SPEC.md`
- [ ] File precise mismatch notes when VNV-side code disagrees with your contracts

### Rule

Do not casually rewrite VNV-owned modules unless directly coordinated.

---

### Phase 6 - Only Then Extend Toward ESP32-P4 and Teensy 4.1

### Goal

Generalize only after the baseline works.

### Tasks

- [ ] Add protocol notes needed for `ESP32-P4`
- [ ] Add protocol notes needed for `Teensy 4.1`
- [ ] Keep extensions additive and version-aware
- [ ] Avoid breaking the `NUCLEO-F401RE` baseline

### Do not do this early

Do not start with:

- ESP32 dual-core handling
- Teensy transport specifics
- protocol expansion for future boards

until the baseline path is stable end-to-end.

---

## Work Intervals and Handoffs

### Interval 1 - Protocol Freeze

You finish:

- protocol spec
- `wire_format.h`
- packet contract decisions

You give VNV:

- final packet layout
- final `frame_packet()` contract
- keyframe/delta rules
- [x] Record 3 golden packet vectors: keyframe, delta, corrupted

### Interval 2 - Snapshot Freeze

You finish:

- snapshot consistency
- profiler baseline
- one idle-hook owner only

You give VNV:

- final snapshot semantics
- sequence ownership decision
- CPU-util semantics

### Interval 3 - Decoder Freeze

You finish:

- typed `DecodedPacket`
- keyframe parse shape
- delta parse shape

You give VNV:

- exact packet object schema
- parsed examples
- state-manager input contract

### Interval 4 - OOM Freeze

You finish:

- tested OOM API
- regression + rolling minimum behavior

You give VNV:

- final analyzer API
- return semantics
- input cadence and fields

### Interval 5 - Integration Review

You do:

- contract review
- mismatch reporting

You give VNV:

- exact bug notes with file names and expected behavior

---

### Day 1 Plan

- [ ] Read role doc
- [ ] Read architecture doc
- [ ] Read technical spec
- [ ] Read task queue
- [ ] Read snapshot files
- [ ] Read hooks file
- [ ] Read framer header
- [ ] Read decoder and OOM test files
- [ ] Write a notes page answering the protocol questions

### Day 2 Plan

- [x] Create `docs/wire_format_spec.md`
- [x] Align `agent/core/wire_format.h`
- [x] Resolve sequence ownership
- [x] Prepare protocol freeze handoff for VNV

### Day 3 Plan

- [ ] Fix snapshot contract issues
- [ ] Resolve duplicate idle hook
- [ ] Align profiler and CPU-util ownership
- [ ] Prepare snapshot freeze handoff for VNV

---

## Definition of Done for Rayan Core Work

Rayan core work is done when:

- [x] `docs/wire_format_spec.md` exists
- [x] `agent/core/wire_format.h` matches the written spec
- [ ] snapshot semantics are frozen
- [ ] only one idle-hook implementation remains
- [ ] `DecodedPacket` is typed and stable
- [ ] `OOMAnalyzer` matches tests and spec
- [ ] `VNV` can consume the outputs without guessing

---

## Notes To Yourself

- Do not confuse “file exists” with “task done”
- Do not trust drift between docs and code without resolving it explicitly
- Do not broaden scope before the baseline works
- Freeze contracts before integration
- Hand off decisions clearly, not just code
