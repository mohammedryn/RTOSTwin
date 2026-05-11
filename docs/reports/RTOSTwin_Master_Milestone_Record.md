# RTOSTwin Master Milestone Record
## Complete Goal, Milestone, Validation, and Superseded-Path History

**Date:** 2026-05-10  
**Project:** `RTOSTwin`  
**Primary baseline board:** `NUCLEO-F401RE`  
**Authoritative purpose:** This file is the single dense record of what the
project set out to achieve, which milestones were completed, how they were
completed, what evidence exists for each milestone, which paths were
superseded, and what still remains before broader v1.0 claims can be made.

---

## 1. Why This Project Exists

RTOSTwin exists to close a specific observability gap:

- modern backend and cloud systems can be monitored continuously with
  `Prometheus`, `Grafana`, and `OpenTelemetry`
- embedded FreeRTOS systems usually cannot expose their internal RTOS state to
  that same open observability stack once deployed in the field
- existing commercial products such as Memfault and Percepio Detect prove the
  demand is real, but they are not the open, self-hosted, standards-based path
  this project set out to build

The project's central thesis is:

> a real RTOS target should be observable with the same open metrics stack used
> for servers and distributed systems

That thesis drove the entire architecture:

- a lightweight MCU-side telemetry agent
- a host-side Python bridge
- a Prometheus and Grafana observability path

---

## 2. Original End-to-End Goal

The concrete project goal for the baseline hardware lane was:

> get the full RTOSTwin hardware-to-dashboard pipeline working end to end on a
> real `NUCLEO-F401RE` board

That goal broke down into these sub-goals:

1. build STM32 firmware that runs FreeRTOS and emits telemetry packets over
   UART
2. flash that firmware successfully through ST-LINK
3. receive packets on the PC through the ST-LINK virtual COM port
4. decode those packets in the Python bridge
5. expose the decoded state as Prometheus metrics
6. visualize those metrics in Grafana
7. verify that the actual RTOS signals look sane:
   task states, CPU usage, stack watermark, heap status, packet loss, and OOM
   projection

As of **2026-05-10**, that baseline goal is **achieved on real hardware**.

---

## 3. Evidence Model Used In This Record

This document separates milestones into three evidence classes:

### 3.1 Achieved and Verified

These are backed by direct repository evidence, test evidence, or explicit
hardware validation evidence.

### 3.2 Historically Important but Superseded

These were real phases, attempts, or project paths that mattered to progress
but are no longer the canonical baseline.

### 3.3 Not Yet Claimable

These may exist in plans, requirements, or design reports, but they are not yet
validated strongly enough to be presented as completed project milestones.

---

## 4. High-Level Milestone Summary

| Milestone | Status | Why It Matters |
|---|---|---|
| Problem definition and market-gap validation | Achieved and documented | Proved the project solves a real open-source observability gap |
| Core three-part architecture defined | Achieved | Established the MCU agent -> bridge -> dashboard system model |
| Baseline board strategy fixed | Achieved | Chose `NUCLEO-F401RE` as the first real target |
| Ownership and integration boundary defined | Achieved | Prevented protocol drift between MCU and bridge work |
| Phase 1 protocol freeze | Achieved | Created the packet contract the whole system depends on |
| Golden vectors and CRC contract | Achieved | Gave byte-exact validation anchors for encoder/decoder work |
| Host-side mock-to-metrics bridge lane | Achieved and locally verified | Proved the software pipeline could run before hardware proof |
| Clean STM32 project baseline created | Achieved | Replaced mixed bring-up projects with a real hardware path |
| Real STM32 firmware build with telemetry linked | Achieved and hardware-verified | Proved the firmware side was no longer placeholder-only |
| ST-LINK flash success on `NUCLEO-F401RE` | Achieved and hardware-verified | Proved the board could actually run the firmware |
| Live serial telemetry over ST-LINK VCP | Achieved and hardware-verified | Proved transport and framing were good enough for sustained decoding |
| Python bridge decoding live board packets | Achieved and hardware-verified | Proved the PC-side ingest worked on real data |
| Prometheus metrics endpoint with live RTOS data | Achieved and hardware-verified | Proved observability export worked |
| Grafana dashboard rendering live hardware metrics | Achieved and hardware-verified | Proved the full digital twin path |
| Final baseline path consolidation around `RTOSTwinF401RE_clean` | Achieved | Clarified which STM32 project is the real validated one |

---

## 4.1 Objective 1 Closure Snapshot - STM32 Baseline

The STM32 interpretation of `Objective 1` is now supported by direct measured
evidence on `NUCLEO-F401RE`:

- cadence: `9.52 Hz` over a `63 second` bridge capture window, with `drops=0`
  and `seq_gaps=0`
- CPU overhead: `72987` mean telemetry-cycle cycles at `84 MHz`
  = `868.9 us` = `0.869%` of a `100 ms` loop budget
- static RAM: `2543 bytes` agent-specific `.data + .bss`
- dynamic-allocation audit: no `malloc`, `calloc`, `realloc`, `free`,
  `pvPortMalloc`, or `pvPortFree` calls found in the telemetry hot path

Saved evidence bundle:

- [objective1_stm32](/D:/digital_twin/evidence/objective1_stm32)

Against the explicit Objective 1 thresholds:

- `< 2%` CPU overhead: `PASS`
- `< 10 KB` static RAM: `PASS`
- no dynamic allocation in hot path: `PASS`

This closes the core engineering acceptance criteria for the STM32 baseline.
`ESP32-P4` and `Teensy 4.1` are not covered by this milestone, and a long soak
record is still recommended before claiming perfect formal signoff against the
full closure plan.

---

## 5. Chronological Master Milestone History

### 5.1 Milestone 0 - The Problem Was Identified and Framed Correctly

### Goal

Define a project that solves a real technical problem rather than building a
demo without a defensible use case.

### What was established

- embedded systems teams lack open, self-hosted RTOS-internal observability
  using standard tools like `Prometheus`, `Grafana`, and `OTLP`
- field monitoring of task state, heap, stack margin, and CPU distribution is
  useful and commercially validated
- existing alternatives either require proprietary backends, paid services, or
  attached debug probes

### How this milestone was achieved

- a full technical report was produced to document the problem, architecture,
  standards alignment, and literature review
- the project was explicitly positioned as an open-source bridge between RTOS
  internals and the standard observability stack

### Evidence

- `docs/reports/RTOSTwin_Complete_Report.md`
- `vnv_final/docs/reports/RTOSTwin_Complete_Report.md`
- `docs/reports/Project_Journey_So_Far.md`

### Why it mattered

This milestone prevented the project from drifting into a generic "embedded
dashboard" demo. It anchored the effort to a specific open-source product gap.

---

### 5.2 Milestone 1 - The System Architecture Was Defined

### Goal

Decide what the system is, where each responsibility lives, and what the data
path should be from hardware to dashboard.

### What was built conceptually

The project settled on three major components:

1. an MCU-side telemetry agent in C
2. a host-side Python bridge
3. an observability stack using Prometheus and Grafana

### Core architecture

`FreeRTOS state -> snapshot capture -> encoder -> framer -> transport -> serial stream -> decoder -> device state -> Prometheus/OTLP exporters -> Grafana`

### How this milestone was achieved

- the architecture was documented in the main project reports
- the intended data flow and module boundaries were laid out before wider
  integration work continued

### Evidence

- `docs/reports/Project_Journey_So_Far.md`
- `docs/reports/RTOSTwin_Complete_Report.md`

### Why it mattered

Without this architecture milestone, the project would have had no stable
 boundary between embedded runtime capture and host-side observability export.

---

### 5.3 Milestone 2 - The Baseline Hardware Strategy Was Chosen

### Goal

Pick a realistic first board and rollout order for the multi-platform project.

### Decision

The baseline first target became:

- `NUCLEO-F401RE`

Follow-on targets were planned later:

- `ESP32-P4`
- `Teensy 4.1`

### Why `NUCLEO-F401RE` mattered

- low-cost and accessible
- onboard ST-LINK programmer
- appropriate for proving the full UART-based telemetry path
- strong baseline for performance and observability validation

### Evidence

- `docs/reports/Project_Journey_So_Far.md`
- `docs/reports/RTOSTwin_Complete_Report.md`

### Why it mattered

This forced the project to target a real, constrained, deployable MCU path
instead of staying abstract.

---

### 5.4 Milestone 3 - Ownership and the Integration Boundary Were Defined

### Goal

Prevent two contributors from implementing incompatible assumptions.

### What was decided

- RYN owned the data meaning, protocol definition, snapshot semantics, decoder
  expectations, and analytical interpretation
- VNV owned framing, transport, bridge infrastructure, exporters, and dashboard
  plumbing
- the integration boundary centered on the packet contract and decoded packet
  semantics

### How this milestone was achieved

- role and responsibility boundaries were written down
- the packet contract became a shared interface rather than an informal idea

### Evidence

- `docs/reports/Project_Journey_So_Far.md`
- `roles/`
- `PRD/`

### Why it mattered

This milestone is what made later protocol freeze work necessary and useful.

---

### 5.5 Milestone 4 - Phase 1 Protocol Freeze Was Completed

### Goal

Freeze the v1 packet contract so firmware and bridge code could converge on the
same byte-level truth.

### What was wrong before

Before the freeze, the project had contract drift:

- documentation disagreed with code
- sequence ownership was inconsistent across files
- `frame_packet()` expectations diverged across architecture docs, technical
  spec, headers, and call sites

### What was built

Two critical artefacts became the protocol truth:

- `docs/wire_format_spec.md`
- `agent/core/wire_format.h`

The protocol freeze also covered:

- packet header layout
- little-endian encoding rules
- CRC-16-CCITT definition
- packet types
- keyframe and delta layout
- golden test vectors

### How this milestone was achieved

- the wire format was documented line by line
- constants were centralized in a machine-readable header
- role tracking marked the phase complete

### Evidence

- `docs/wire_format_spec.md`
- `agent/core/wire_format.h`
- `docs/reports/Project_Journey_So_Far.md`

### Why it mattered

This was the milestone that stopped the firmware and bridge from speaking two
different dialects of the protocol.

---

### 5.6 Milestone 5 - Golden Vectors and CRC Validation Were Locked Down

### Goal

Ensure that packet validation and byte-level correctness had hard anchors that
could be checked independently.

### What was achieved

- CRC-16-CCITT-FALSE was fixed as the checksum algorithm
- the standard `"123456789"` vector mapped to `0x29B1`
- golden packet vectors were established for validation

### How this milestone was achieved

- the CRC contract was written into the protocol spec
- testable vectors were documented for later encoder/decoder checks

### Evidence

- `docs/wire_format_spec.md`
- `docs/reports/Project_Journey_So_Far.md`

### Why it mattered

This milestone turned "I think the protocol is right" into "we can prove a
packet is right byte-for-byte."

---

### 5.7 Milestone 6 - The Host-Side Bridge Lane Became Locally Reproducible

### Goal

Prove that the host-side observability path could ingest valid telemetry, keep
device state, and expose metrics even before real-board proof.

### What was achieved

Within the `vnv_final/` lane, the bridge path became locally testable and
reproducible using canonical mock telemetry.

Local repo-verified capabilities included:

- decoder behavior against golden vectors
- state reconstruction from keyframe and delta packets
- OOM analyzer test coverage gates
- Prometheus metric rendering
- bridge smoke-path coverage from framed mock data through exporters
- Docker-based Prometheus and Grafana local stack

### How this milestone was achieved

- a separate `vnv_final/` delivery lane was used for the cleaner software path
- the bridge accepted `stdin` for local demo use
- mock-device and tests supported reproducible host-side validation

### Evidence

- `vnv_final/docs/vnv_repo_completion_status.md`
- `vnv_final/docs/quick_start.md`

### Why it mattered

This milestone reduced hardware risk. It meant the project did not need the
board to prove every part of the host-side path.

---

### 5.8 Milestone 7 - The Project Reached an Honest Verification Boundary

### Goal

Separate what was genuinely verified from what was still only planned or only
locally mocked.

### What was achieved

The project explicitly documented that:

- mock-to-metrics software verification existed
- full real-hardware proof had not yet been completed at that earlier stage
- certain claims should not yet be made without external evidence

### How this milestone was achieved

- status reporting was tightened in the VNV completion status document
- repo-only proof and hardware proof were separated explicitly

### Evidence

- `vnv_final/docs/vnv_repo_completion_status.md`

### Why it mattered

This prevented inflated claims before the real board milestone was reached.

---

### 5.9 Milestone 8 - The Clean STM32 Hardware Baseline Was Established

### Goal

Replace mixed or broken STM32 bring-up attempts with one clean project that
could actually carry the telemetry agent to real hardware.

### What was achieved

A clean STM32 project was created:

- `RTOSTwinF401RE_clean`

It was configured for:

- board: `NUCLEO-F401RE`
- `USART2` asynchronous serial
- DMA TX
- FreeRTOS with `CMSIS_V2`
- SWD debug through ST-LINK

### How this milestone was achieved

- the telemetry agent was integrated into the clean STM32 project
- obsolete mixed project paths were left behind as bring-up artefacts

### Evidence

- `vnv_final/docs/reports/hardware_validation_2026-05-10.md`

### Why it mattered

This milestone created the first trustworthy firmware baseline for the hardware
validation run.

---

### 5.10 Milestone 9 - Real Telemetry Firmware Was Built Successfully

### Goal

Build real firmware with the telemetry agent linked in, not a tiny placeholder
or empty shell.

### What was achieved

The successful firmware build for `RTOSTwinF401RE_clean` produced:

```text
text = 32936
data = 108
bss = 23388
total dec = 56432
```

### How this milestone was achieved

- the clean STM32 project was compiled successfully
- the resulting binary size confirmed that the telemetry firmware was actually
  linked into the image

### Evidence

- `vnv_final/docs/reports/hardware_validation_2026-05-10.md`

### Why it mattered

This was the milestone that proved the project had moved beyond protocol and
mocking into a genuine embedded runtime image.

---

### 5.11 Milestone 10 - The Real Board Was Flashed Successfully Through ST-LINK

### Goal

Program the real `NUCLEO-F401RE` board successfully and verify the download.

### What was achieved

The board was flashed and verified successfully with:

- board: `NUCLEO-F401RE`
- device family: `STM32F401xD/E`
- flash download completed
- verification completed successfully

### How this milestone was achieved

- the clean STM32 firmware project was programmed through ST-LINK
- successful device detection and verification closed the firmware deployment
  loop

### Evidence

- `vnv_final/docs/reports/hardware_validation_2026-05-10.md`

### Why it mattered

Without this milestone, every downstream "real hardware" claim would still have
been hypothetical.

---

### 5.12 Milestone 11 - Live Serial Telemetry Worked Over the ST-LINK Virtual COM Port

### Goal

Prove that the board was not only flashed, but actively transmitting valid
telemetry that the PC could receive continuously.

### What was achieved

Windows exposed:

- `STMicroelectronics STLink Virtual COM Port (COM11)`

The bridge was run as:

```bash
python bridge/main.py --port COM11 --baud 115200 --device-id nucleo-f401re
```

Representative bridge logs showed:

```text
110 packets received | drops=0 | seq_gaps=0
208 packets received | drops=0 | seq_gaps=0
```

Packet counts continued upward into the thousands.

### How this milestone was achieved

- UART transport on the MCU side was functioning
- framing and CRC were valid enough for sustained decode
- the host-side bridge opened the board's virtual COM port successfully

### Evidence

- `vnv_final/docs/reports/hardware_validation_2026-05-10.md`

### Why it mattered

This was the decisive transport milestone. It proved that the device and host
were actually exchanging live telemetry.

---

### 5.13 Milestone 12 - The Python Bridge Decoded Real Hardware Data

### Goal

Show that the Python bridge could decode real telemetry from the board rather
than only synthetic mock packets.

### What was achieved

The bridge continuously decoded live board packets with:

- sustained packet counts
- `drops=0`
- `seq_gaps=0`

### How this milestone was achieved

- the live serial feed was ingested by the bridge
- packet framing, integrity, and sequence behavior were healthy enough for
  continuous operation

### Evidence

- `vnv_final/docs/reports/hardware_validation_2026-05-10.md`
- `vnv_final/docs/vnv_repo_completion_status.md`

### Why it mattered

This milestone is where the project crossed from "hardware emits bytes" to
"hardware emits meaningful telemetry the software stack can use."

---

### 5.14 Milestone 13 - Prometheus Exposed Live RTOS Metrics

### Goal

Turn live decoded board telemetry into queryable Prometheus metrics.

### What was achieved

The metrics endpoint became live at:

- `http://localhost:8000/metrics`

Metrics confirmed for `device_id="nucleo-f401re"` included:

- `rtos_cpu_utilization_ratio`
- `rtos_task_cpu_ratio`
- `rtos_heap_free_bytes`
- `rtos_heap_min_ever_bytes`
- `rtos_task_stack_watermark_bytes`
- `rtos_telemetry_packet_loss_ratio`
- `rtos_heap_oom_projection_seconds`
- task state metrics

### How this milestone was achieved

- the bridge maintained device state from incoming packets
- exporter logic surfaced those values through the Prometheus exposition format

### Evidence

- `vnv_final/docs/reports/hardware_validation_2026-05-10.md`
- `vnv_final/docs/vnv_repo_completion_status.md`

### Why it mattered

This milestone converted device telemetry into standard observability data,
which is the core point of the project.

---

### 5.15 Milestone 14 - Grafana Rendered the Live Digital Twin

### Goal

Visualize the real hardware telemetry through the provisioned dashboard.

### What was achieved

The `RTOSTwin Digital Twin` dashboard rendered live data for:

- Total CPU Utilization
- Heap Memory Trends
- OOM Projection
- Stack High Watermarks
- Task CPU Distribution
- Packet Loss Ratio

### How this milestone was achieved

- Prometheus served the live metrics
- Grafana queried those metrics successfully
- the provisioned dashboard displayed real board signals rather than mock-only
  data

### Evidence

- `vnv_final/docs/reports/hardware_validation_2026-05-10.md`

### Why it mattered

This is the milestone that completed the project's flagship promise:

`real MCU -> open metrics stack -> live dashboard`

---

### 5.16 Milestone 15 - The Actual RTOS Signals Looked Sane

### Goal

Verify not just transport and rendering, but that the actual runtime values from
the board were credible and useful.

### What was observed

Representative observed values:

- `rtos_cpu_utilization_ratio = 1`
- `rtos_heap_free_bytes = 12568`
- `rtos_heap_min_ever_bytes = 12568`
- `rtos_telemetry_packet_loss_ratio = 0`
- `rtos_heap_oom_projection_seconds = -1`

Per-task telemetry was confirmed for:

- `IDLE`
- `TelemetryTask`
- `defaultTask`
- `Tmr Svc`

Confirmed stack watermark values:

- `IDLE = 424 B`
- `TelemetryTask = 1560 B`
- `Tmr Svc = 856 B`
- `defaultTask = 344 B`

### Interpretation

- CPU accounting was flowing end to end
- heap usage was stable
- no memory leak trend was currently detected
- packet loss was zero
- task names and task-level measurements were present and sane

### Evidence

- `vnv_final/docs/reports/hardware_validation_2026-05-10.md`

### Why it mattered

This milestone proved the observability output was not just "connected" but
actually informative.

---

### 5.17 Milestone 16 - The Full Baseline Hardware-to-Dashboard Goal Was Achieved

### Goal

Complete the whole target pipeline on a real board.

### Final achieved baseline

The following path is now proven on real hardware:

`NUCLEO-F401RE -> FreeRTOS telemetry firmware -> UART via ST-LINK VCP -> Python bridge -> Prometheus -> Grafana`

### What this proves technically

The project has demonstrated all of the following working together:

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

### Authoritative milestone statement

> The RTOSTwin system has been validated end to end on real hardware using the
> STM32 NUCLEO-F401RE. Firmware was successfully built and flashed, telemetry
> streamed over the ST-LINK virtual COM port, the Python bridge decoded packets
> without loss, Prometheus exported live RTOS metrics, and the Grafana
> dashboard rendered live CPU, heap, stack, packet-loss, and OOM-projection
> telemetry for the device `nucleo-f401re`.

### Evidence

- `vnv_final/docs/reports/hardware_validation_2026-05-10.md`
- `vnv_final/docs/vnv_repo_completion_status.md`

---

## 6. Superseded, Failed, Mixed, or Obsolete Paths

This section is intentionally included because the project history is not just a
list of successes. Several paths were necessary for learning and bring-up but
are not the final validated baseline.

### 6.1 Superseded STM32 Project Paths

The hardware validation record explicitly marks these older STM32 projects as
obsolete bring-up artefacts:

- `RTOSTwinF401RE`
- `textCubeProject`
- `First`

### What these paths represent

- earlier experimentation
- mixed configuration states
- non-canonical firmware bring-up attempts

### Why they are not the baseline anymore

The validated hardware path is:

- `RTOSTwinF401RE_clean`

That is the project that built successfully, flashed successfully, and produced
the validated telemetry stream.

---

### 6.2 Earlier State Where Hardware Proof Had Not Yet Been Earned

Before the 2026-05-10 validation, the project had an explicit "honest boundary"
phase where it was correct to claim:

- the mock-to-metrics pipeline existed
- the software lane was locally reproducible
- real-board proof was still external

### Why this matters historically

It shows that the project did not jump straight from idea to hardware success.
There was an intermediate milestone where the team deliberately separated local
verification from real-board proof.

### Evidence

- `vnv_final/docs/vnv_repo_completion_status.md`

---

### 6.3 The Audit State Captured a Mixed and Imperfect Repository Snapshot

The audit log in `log.md` is a factual historical snapshot of repository state,
not the final validated baseline. It records issues such as:

- contract drift around `frame_packet()`
- duplicate `vApplicationIdleHook` ownership
- missing or incomplete deliverables in some repo lanes
- older bridge or dashboard limitations
- differences between planned deliverables and actual current files

### How to interpret that audit correctly

- it is useful as an honesty document
- it should not be confused with the final clean hardware validation path
- it reflects repository state at audit time, including mixed legacy and
  in-progress paths
- the clean validated baseline in `vnv_final/` plus `RTOSTwinF401RE_clean`
  supersedes the idea that the project is only a mock/demo lane

### Why this matters

It captures an important project milestone of a different kind:

`the team established a truthful understanding of what was actually working,
what was incomplete, and what needed to be cleaned up`

That honesty is part of how the final hardware validation became credible.

---

## 7. What Was Missing From Earlier Milestone Files and Is Now Captured Here

This master record deliberately fills gaps that were split across older files:

### Gap 1 - Phase 1 history existed, but later milestones were missing

`Project_Journey_So_Far.md` is excellent for the early protocol and learning
history, but it stops at the "Phase 1 complete" era and does not capture the
later real-board success.

### Gap 2 - Current VNV status existed, but not as a full project history

`vnv_repo_completion_status.md` captures what is verified now, but it is a
status summary, not a full narrative of how the project got there.

### Gap 3 - Hardware validation existed, but only as a single milestone report

`hardware_validation_2026-05-10.md` captures the crucial real-board milestone,
but only that milestone.

### Gap 4 - The audit existed, but not as a milestone story

`log.md` is about current-state truthfulness and repository assessment, not a
goal-to-milestone history.

### What this file adds

This file unifies:

- the original problem
- the target architecture
- the early protocol milestone
- the software-lane validation milestone
- the clean-board bring-up milestone
- the real hardware validation milestone
- the obsolete/superseded path history
- the current honest completion boundary

---

## 8. Current Verified State

The project can now honestly claim all of the following:

- the project solves a clearly defined, real observability problem
- the core architecture is established
- the protocol contract is frozen
- the host-side software lane is locally reproducible and test-backed
- the baseline `NUCLEO-F401RE -> UART -> Bridge -> Prometheus -> Grafana` path
  has been validated on real hardware
- live RTOS metrics including CPU, heap, stack watermark, task distribution,
  packet loss, and OOM projection are flowing through the full pipeline
- `RTOSTwinF401RE_clean` is the correct STM32 baseline to keep

---

## 9. What Is Still Not Safe to Claim

Even after the successful hardware milestone, the following broader claims
should still be treated carefully unless separately measured or validated:

- that every STM32 project variant in the repository is hardware-validated
- that all historical repo lanes are equally clean or canonical
- that long-duration soak reliability has been fully proven
- that CPU overhead, WCET, memory overhead, and timing targets generalize beyond
  the validated baseline without fresh measurement
- that the `ESP32-P4` and `Teensy 4.1` targets are validated to the same level
  as the `NUCLEO-F401RE`

---

## 10. Remaining Milestones Beyond the Baseline Hardware Victory

The baseline end-to-end goal is achieved, but broader project-completion work
still remains if the project wants a stronger v1.0 story.

Important next milestones include:

1. long-duration soak validation on the real board
2. measured overhead and timing characterization on the exact baseline firmware
3. alert-threshold definition for stack, heap, and packet-loss health
4. stress-scenario validation to prove the dashboard reveals bad RTOS states
5. cleanup of legacy paths and documentation around the validated baseline
6. future board-port validation for `ESP32-P4` and `Teensy 4.1`

---

## 11. Final Authoritative Conclusion

RTOSTwin started as a response to a real and verified observability gap in the
embedded systems world: FreeRTOS devices did not have an open, self-hosted,
standards-based path into the same observability stack used for servers. The
project first had to define that problem, then design the architecture, then
freeze the byte-level protocol, then establish a locally reproducible bridge
path, then create a clean STM32 baseline, and finally prove the complete system
on a real `NUCLEO-F401RE`.

That final proof now exists.

The most important completed milestone is:

> the full `NUCLEO-F401RE -> FreeRTOS telemetry firmware -> ST-LINK virtual COM
> port -> Python bridge -> Prometheus -> Grafana` pipeline has been validated
> end to end on real hardware as of 2026-05-10

Everything before that milestone was preparation, alignment, cleanup,
simulation, or bring-up. Everything after it is characterization, hardening,
and expansion.

That is the true project state.

---

## 12. Source Documents Consolidated Into This Master Record

- `docs/reports/Project_Journey_So_Far.md`
- `docs/reports/RTOSTwin_Complete_Report.md`
- `docs/wire_format_spec.md`
- `vnv_final/docs/vnv_repo_completion_status.md`
- `vnv_final/docs/reports/hardware_validation_2026-05-10.md`
- `log.md`
