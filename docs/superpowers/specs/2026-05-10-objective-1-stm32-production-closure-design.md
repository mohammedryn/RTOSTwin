# Objective 1 STM32 Production Closure Design

**Date:** 2026-05-10  
**Project:** `RTOSTwin`  
**Target objective:** `RTOSTwin_Complete_Report.md` Section 4.1 Objective 1  
**Scope:** `NUCLEO-F401RE` baseline only

## 1. Purpose

This spec defines how Objective 1 will be closed honestly and defensibly for
the validated STM32 baseline.

Objective 1 in the report currently reads as a multi-platform goal:

> Build a lightweight C99 telemetry agent for FreeRTOS (v10.5+ on
> NUCLEO-F401RE, ESP32-P4, and Teensy 4.1) that captures RTOS-internal state
> with under 2% CPU overhead at 10 Hz, under 10 KB static RAM on the baseline
> board, using no dynamic allocation in the hot path.

The project has already validated the end-to-end telemetry path on a real
`NUCLEO-F401RE`, but Objective 1 is not yet closed because the project still
lacks measured overhead evidence, measured RAM evidence, hot-path allocation
proof, and soak validation on the STM32 baseline.

This design closes Objective 1 as:

`Objective 1 achieved for the STM32 baseline, with ESP32-P4 and Teensy 4.1 explicitly deferred as future work.`

## 2. Goal Statement

The goal of this milestone is to prove that the existing STM32 telemetry agent
is not merely functional, but lightweight and stable enough to be described as
the production-baseline implementation for `NUCLEO-F401RE`.

At the end of this milestone, the project should be able to claim:

- telemetry runs at `10 Hz` on the validated STM32 baseline
- agent hot-path execution remains under the defined CPU-overhead target
- agent static RAM usage remains under the defined RAM target
- no dynamic allocation occurs in the telemetry hot path
- the firmware/bridge path remains stable through a defined soak run

## 3. In Scope

- `NUCLEO-F401RE` only
- `RTOSTwinF401RE_clean` as the only firmware baseline under test
- FreeRTOS telemetry agent behavior on the STM32 path
- measurement instrumentation needed to collect trustworthy overhead evidence
- bridge-side observation only insofar as it is needed to verify the agent and
  transport behavior during soak runs
- documentation changes needed to mark Objective 1 complete for STM32 and defer
  the other boards

## 4. Out of Scope

- validating `ESP32-P4`
- validating `Teensy 4.1`
- full production hardening of OTLP export
- dashboard redesign
- alert threshold tuning beyond what is needed to interpret the soak data
- broad stress/fault campaigns that belong to later objectives

## 5. Current Starting Point

The current validated baseline already proves:

- firmware builds on the clean STM32 project
- firmware flashes successfully to `NUCLEO-F401RE`
- packets stream over the ST-LINK virtual COM port
- the Python bridge decodes the live stream
- Prometheus exposes the decoded metrics
- Grafana renders the live device state

The current gap is not basic functionality. The current gap is evidence.

## 6. Closure Criteria

Objective 1 is closed only when all of the following are true for the STM32
baseline:

### 6.1 Telemetry Cadence

- telemetry is confirmed to run at `10 Hz`
- the tested build and runtime configuration are documented

### 6.2 CPU Overhead

- the telemetry agent overhead is measured on the real `NUCLEO-F401RE`
- the reported overhead is shown to be `< 2%`
- the method used to derive the percentage is documented

### 6.3 Static RAM

- the static RAM cost of the agent is measured on the STM32 baseline
- the reported agent RAM usage is shown to be `< 10 KB`
- the method used to isolate the agent contribution is documented

### 6.4 No Dynamic Allocation in Hot Path

- code review confirms no heap allocation is performed by the telemetry hot path
- if needed, a measurement or wrapper-based check confirms that assumption

### 6.5 Soak Stability

- the validated firmware and bridge run continuously for a defined soak duration
- the run shows no crashes, transport collapse, or evidence that invalidates
  the baseline claim

### 6.6 Documentation Closure

- the root report is updated to say Objective 1 is achieved for STM32 only
- `ESP32-P4` and `Teensy 4.1` are explicitly restated as future work
- a dedicated Objective 1 validation report is added to the repo

## 7. Measurement Strategy

### 7.1 Telemetry Frequency

Telemetry cadence will be confirmed using the existing task scheduling interval,
live packet timestamps, or profiler output from the STM32 baseline.

Accepted evidence:

- firmware configuration showing the intended cadence
- observed packet/timestamp behavior confirming `10 Hz`

### 7.2 CPU Overhead

The telemetry agent overhead will be measured from real STM32 execution time,
not estimated from host-side or paper design assumptions.

Preferred method:

- measure the time spent in the telemetry cycle on the real target
- compare that cost against the available CPU budget at the configured cadence
- compute percentage overhead from measured real-board timing

Required output:

- raw timing evidence
- calculation method
- final percentage

### 7.3 Static RAM

The project must distinguish between:

- whole-firmware memory usage
- telemetry-agent-specific static RAM cost

Accepted methods:

- section-size accounting using build artifacts
- symbol/section analysis for agent-owned `.data` and `.bss`
- an explicit documented approximation if the split cannot be made perfectly,
  provided the method is honest and reproducible

### 7.4 No-Malloc Proof

The hot path must be checked for:

- direct `malloc`
- `pvPortMalloc`
- any other dynamic allocation helpers

Accepted evidence:

- code audit of the hot path
- optional runtime/assertion hook if needed for stronger proof

### 7.5 Soak Run

A soak run will validate stability of the STM32 baseline over time.

Minimum acceptable scope:

- run the board + bridge + metrics path continuously for a meaningful duration
- capture packet-loss behavior, heap stability, and general runtime stability

Preferred target:

- a multi-hour run, ideally extending toward overnight or 24-hour validation

## 8. Workstreams

### Workstream A: Measurement Readiness

- inspect the current STM32 profiler and telemetry instrumentation
- decide whether existing instrumentation is sufficient
- add only the minimum extra measurement hooks needed

### Workstream B: CPU and Cadence Validation

- confirm real `10 Hz` behavior
- capture telemetry timing evidence
- calculate CPU overhead percentage

### Workstream C: RAM Accounting

- extract build-size evidence
- compute or isolate agent static RAM usage
- document the derivation clearly

### Workstream D: Hot-Path Allocation Audit

- inspect the hot path end to end
- document the proof that no dynamic allocation occurs

### Workstream E: Soak Validation

- run the baseline system continuously
- capture runtime behavior and anomalies
- summarize whether the run supports the baseline claim

### Workstream F: Documentation Closure

- update `docs/reports/RTOSTwin_Complete_Report.md`
- update milestone/status docs if needed
- add one dedicated Objective 1 validation report

## 9. Deliverables

At the end of this milestone, the repo should contain:

- updated `docs/reports/RTOSTwin_Complete_Report.md`
- one dedicated STM32 Objective 1 validation report
- updated milestone/status wording where necessary
- any minimal code changes needed to gather and preserve measurement evidence

## 10. Risks and Guardrails

### Risk 1: Measurement ambiguity

The project may have enough raw data to prove the agent works, but not enough
structured evidence to prove the overhead and RAM claims cleanly.

Guardrail:

- prefer explicit real-board measurements over inferred estimates
- document assumptions whenever isolation is imperfect

### Risk 2: Scope creep into later objectives

It would be easy to broaden this into dashboard tuning, stress campaigns, or
multi-board work.

Guardrail:

- stay focused on the exact Objective 1 closure criteria

### Risk 3: Overclaiming on non-STM32 boards

The original wording includes three boards, but this closure milestone is STM32
only.

Guardrail:

- every updated doc must state that `ESP32-P4` and `Teensy 4.1` remain future
  work

## 11. Final Definition of Done

Objective 1 is complete for this milestone if and only if the repo contains
evidence that:

- `NUCLEO-F401RE` telemetry runs at `10 Hz`
- telemetry overhead is measured and `< 2%`
- agent static RAM is measured and `< 10 KB`
- the telemetry hot path performs no dynamic allocation
- the STM32 baseline remains stable through a soak run
- the report and README family clearly state STM32 closure and future-board
  deferral

## 12. Recommended Next Step After Approval

Once this spec is approved, the next action is to create an implementation and
validation plan that breaks the work into:

1. measurement instrumentation
2. cadence and overhead capture
3. RAM accounting
4. hot-path allocation proof
5. soak-run execution
6. documentation closure
