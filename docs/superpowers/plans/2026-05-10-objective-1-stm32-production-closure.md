# Objective 1 STM32 Production Closure Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Close Objective 1 for the `NUCLEO-F401RE` baseline by proving the STM32 telemetry agent runs at `10 Hz`, remains under `2%` CPU overhead, stays under `10 KB` static RAM, uses no dynamic allocation in the hot path, and stays stable through a defined soak run.

**Architecture:** Keep `vnv_final/` as the active implementation lane and treat the repo root as canonical docs plus shared protocol truth. Add only the minimum firmware instrumentation needed to collect reproducible STM32 evidence, preserve that evidence in a dedicated validation report, and then update the public project docs so Objective 1 is explicitly closed for STM32 while `ESP32-P4` and `Teensy 4.1` remain future work.

**Tech Stack:** C99, FreeRTOS, STM32 HAL, ARM DWT cycle counter, Python 3.9+, PowerShell, GCC host-side test builds, Markdown reports, STM32CubeIDE or equivalent STM32 build tooling.

---

## File Structure

### Existing files that remain central

- `vnv_final/agent/main.c`
  Owns the telemetry task loop, packet cadence, and UART send path.
- `vnv_final/agent/core/profiler.h`
  Declares the existing DWT-based timing interface.
- `vnv_final/agent/core/profiler.c`
  Owns low-level cycle measurement and stable reporting text.
- `vnv_final/agent/core/snapshot.h`
  Defines the fixed telemetry structures whose size directly affects RAM.
- `vnv_final/agent/core/snapshot.c`
  Owns `snapshot_capture()`, one of the main hot-path functions under audit.
- `vnv_final/agent/core/encoder.c`
  Owns delta/keyframe encoding and is part of the hot path.
- `vnv_final/agent/core/framer.c`
  Owns packet framing and is part of the hot path.
- `vnv_final/agent/core/transport.c`
  Owns non-blocking DMA send behavior and transport drop counting.
- `vnv_final/agent/freertos/hooks.c`
  Owns idle-hook counters and crash-safe static storage.
- `vnv_final/agent/tests/test_profiler.c`
  Existing host-side unit test anchor for profiler behavior.
- `vnv_final/agent/tests/test_snapshot.c`
  Existing host-side unit test anchor for snapshot behavior.
- `docs/reports/RTOSTwin_Complete_Report.md`
  Needs final Objective 1 wording changes.
- `docs/reports/RTOSTwin_Master_Milestone_Record.md`
  Needs a milestone entry for STM32 Objective 1 closure.
- `README.md`
  Needs the stronger STM32 production-baseline claim once evidence exists.
- `vnv_final/docs/vnv_repo_completion_status.md`
  Should reflect that Objective 1 is now closed on STM32 only.

### External artifact dependency that is not tracked in this repo

- `RTOSTwinF401RE_clean`
  This is the validated STM32CubeIDE project and the only firmware baseline
  allowed for this milestone. Its build artifacts, especially the `.map` file,
  are expected to exist on the local machine even though the project folder is
  not tracked inside this repository.

### New files to create

- `vnv_final/agent/core/measurement.h`
  Small rolling statistics API for telemetry-cycle timing.
- `vnv_final/agent/core/measurement.c`
  Implementation of min/max/mean/reset helpers for timing samples.
- `vnv_final/agent/tests/test_measurement.c`
  Host-side tests for the measurement helper.
- `tools/stm32/measure_agent_size.ps1`
  Reproducible static-RAM accounting helper that parses the STM32 linker map.
- `tools/stm32/no_malloc_audit.ps1`
  Repo-relative audit helper for allocation-free hot-path proof.
- `docs/reports/Objective_1_STM32_Validation_Report.md`
  Final evidence pack covering cadence, CPU overhead, static RAM, no-allocation proof, and soak validation.

The decomposition is:

- hot-path measurement logic stays isolated in `vnv_final/agent/core/measurement.*`
- timing integration touches only `vnv_final/agent/main.c` plus stable profiler output
- reproducible evidence collection lives under `tools/stm32/`
- final claims are made only after the validation report is filled with measured evidence

---

### Task 1: Add Rolling Measurement Support for the STM32 Telemetry Loop

**Files:**
- Create: `vnv_final/agent/core/measurement.h`
- Create: `vnv_final/agent/core/measurement.c`
- Create: `vnv_final/agent/tests/test_measurement.c`
- Modify: `vnv_final/agent/main.c`

- [ ] **Step 1: Write the failing host-side measurement test**

Create `vnv_final/agent/tests/test_measurement.c` with this full content:

```c
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "../core/measurement.h"

#define PASS(name) printf("  [PASS] %s\n", name)

static void test_record_initialises_min_max_count_and_total(void) {
    measurement_stats_t stats = {0};

    measurement_record(&stats, 120U);

    assert(stats.min_cycles == 120U);
    assert(stats.max_cycles == 120U);
    assert(stats.sample_count == 1U);
    assert(stats.total_cycles == 120U);
    PASS("record_initialises_min_max_count_and_total");
}

static void test_record_updates_running_min_max_and_mean(void) {
    measurement_stats_t stats = {0};

    measurement_record(&stats, 150U);
    measurement_record(&stats, 90U);
    measurement_record(&stats, 210U);

    assert(stats.min_cycles == 90U);
    assert(stats.max_cycles == 210U);
    assert(stats.sample_count == 3U);
    assert(stats.total_cycles == 450U);
    assert(measurement_mean_cycles(&stats) == 150U);
    PASS("record_updates_running_min_max_and_mean");
}

static void test_reset_clears_all_fields(void) {
    measurement_stats_t stats = {
        .min_cycles = 1U,
        .max_cycles = 2U,
        .sample_count = 3U,
        .total_cycles = 4U
    };

    measurement_reset(&stats);

    assert(stats.min_cycles == 0U);
    assert(stats.max_cycles == 0U);
    assert(stats.sample_count == 0U);
    assert(stats.total_cycles == 0U);
    PASS("reset_clears_all_fields");
}

int main(void) {
    printf("--- Measurement Unit Tests ---\n");
    test_record_initialises_min_max_count_and_total();
    test_record_updates_running_min_max_and_mean();
    test_reset_clears_all_fields();
    printf("\nAll measurement tests PASSED\n");
    return 0;
}
```

- [ ] **Step 2: Run the new test to verify it fails before implementation**

Run:

```bash
gcc -std=c99 -Wall -Wextra -Werror -Ivnv_final/agent/core vnv_final/agent/tests/test_measurement.c -o C:\tmp\test_measurement.exe
```

Expected:

- compile failure because `measurement.h`, `measurement_stats_t`,
  `measurement_record()`, `measurement_mean_cycles()`, and `measurement_reset()`
  do not exist yet

- [ ] **Step 3: Add the minimal measurement module**

Create `vnv_final/agent/core/measurement.h`:

```c
#ifndef VNV_FINAL_AGENT_CORE_MEASUREMENT_H
#define VNV_FINAL_AGENT_CORE_MEASUREMENT_H

#include <stdint.h>

typedef struct {
    uint32_t min_cycles;
    uint32_t max_cycles;
    uint32_t sample_count;
    uint64_t total_cycles;
} measurement_stats_t;

void measurement_record(measurement_stats_t *stats, uint32_t elapsed_cycles);
uint32_t measurement_mean_cycles(const measurement_stats_t *stats);
void measurement_reset(measurement_stats_t *stats);

#endif
```

Create `vnv_final/agent/core/measurement.c`:

```c
#include "measurement.h"

void measurement_record(measurement_stats_t *stats, uint32_t elapsed_cycles) {
    if (stats == 0) {
        return;
    }

    if (stats->sample_count == 0U) {
        stats->min_cycles = elapsed_cycles;
        stats->max_cycles = elapsed_cycles;
    } else {
        if (elapsed_cycles < stats->min_cycles) {
            stats->min_cycles = elapsed_cycles;
        }
        if (elapsed_cycles > stats->max_cycles) {
            stats->max_cycles = elapsed_cycles;
        }
    }

    stats->sample_count += 1U;
    stats->total_cycles += (uint64_t)elapsed_cycles;
}

uint32_t measurement_mean_cycles(const measurement_stats_t *stats) {
    if (stats == 0 || stats->sample_count == 0U) {
        return 0U;
    }

    return (uint32_t)(stats->total_cycles / stats->sample_count);
}

void measurement_reset(measurement_stats_t *stats) {
    if (stats == 0) {
        return;
    }

    stats->min_cycles = 0U;
    stats->max_cycles = 0U;
    stats->sample_count = 0U;
    stats->total_cycles = 0U;
}
```

- [ ] **Step 4: Wire the new measurement helper into the telemetry loop**

Update `vnv_final/agent/main.c` with these changes:

```c
#include "core/measurement.h"

static measurement_stats_t s_cycle_stats;

void vTelemetryTask(void *pvParameters)
{
    (void)pvParameters;

    profiler_init();
    snapshot_init();
    encoder_init();
    transport_init();

    profiler_stats_t snap_stats = {0};
    uint32_t loop_count = 0u;

    while (1) {
        uint32_t cycle_start = profiler_start();

        uint32_t snap_start = profiler_start();
        snapshot_capture(&s_current_snapshot);
        uint32_t snap_elapsed = profiler_stop(snap_start);
        profiler_record(&snap_stats, snap_elapsed);

        bool force_keyframe = ((loop_count % WF_KEYFRAME_INTERVAL) == 0u);
        uint16_t enc_len = encoder_encode(&s_current_snapshot,
                                          s_payload_buffer,
                                          sizeof(s_payload_buffer),
                                          force_keyframe);

        if (enc_len > 0u) {
            uint8_t pkt_type = encoder_last_was_keyframe() ? WF_TYPE_KEYFRAME : WF_TYPE_DELTA;
            uint16_t frame_len = frame_packet(s_payload_buffer,
                                              enc_len,
                                              pkt_type,
                                              s_current_snapshot.sequence_num,
                                              s_current_snapshot.timestamp_ticks,
                                              s_framed_buffer,
                                              sizeof(s_framed_buffer));

            if (frame_len > 0u) {
                transport_send(s_framed_buffer, frame_len);
            }
        }

        measurement_record(&s_cycle_stats, profiler_stop(cycle_start));

        if ((++loop_count % 100u) == 0u) {
            profiler_report(&snap_stats, "snapshot_capture");
            printf("[MEASURE] telemetry_cycle min=%lu max=%lu mean=%lu cycles samples=%lu\n",
                   (unsigned long)s_cycle_stats.min_cycles,
                   (unsigned long)s_cycle_stats.max_cycles,
                   (unsigned long)measurement_mean_cycles(&s_cycle_stats),
                   (unsigned long)s_cycle_stats.sample_count);
            measurement_reset(&s_cycle_stats);
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
```

- [ ] **Step 5: Run the host-side C tests**

Run:

```bash
gcc -std=c99 -Wall -Wextra -Werror -Ivnv_final/agent/core vnv_final/agent/tests/test_measurement.c vnv_final/agent/core/measurement.c -o C:\tmp\test_measurement.exe
C:\tmp\test_measurement.exe
```

Run:

```bash
gcc -std=c99 -Wall -Wextra -Werror vnv_final/agent/tests/test_profiler.c -o C:\tmp\test_profiler.exe
C:\tmp\test_profiler.exe
```

Expected:

- both test binaries compile
- both test binaries print only pass output

- [ ] **Step 6: Commit**

```bash
git add vnv_final/agent/core/measurement.h vnv_final/agent/core/measurement.c vnv_final/agent/tests/test_measurement.c vnv_final/agent/main.c
git commit -m "feat: add stm32 telemetry cycle measurement support"
```

---

### Task 2: Capture Reproducible Cadence and CPU-Overhead Evidence

**Files:**
- Modify: `vnv_final/agent/core/profiler.c`
- Create: `docs/reports/Objective_1_STM32_Validation_Report.md`

- [ ] **Step 1: Create the validation report skeleton before collecting evidence**

Create `docs/reports/Objective_1_STM32_Validation_Report.md`:

```md
# Objective 1 STM32 Validation Report

## Scope

This report closes Objective 1 for the STM32 baseline only:
- Board: `NUCLEO-F401RE`
- Firmware project: `RTOSTwinF401RE_clean`
- Other boards: `ESP32-P4` and `Teensy 4.1` remain future work

## Build Under Test

- Firmware project:
- Firmware build configuration:
- MCU clock:
- FreeRTOS tick rate:
- Telemetry task delay:
- Bridge command:

## Cadence Evidence

- Measurement window:
- Packet count delta:
- Measured cadence:
- Expected cadence:
- Verdict:

## CPU Overhead Evidence

- Snapshot min cycles:
- Snapshot max cycles:
- Snapshot mean cycles:
- Telemetry-cycle min cycles:
- Telemetry-cycle max cycles:
- Telemetry-cycle mean cycles:
- Mean telemetry-cycle microseconds:
- CPU overhead at 10 Hz:
- Verdict:

## Static RAM Evidence

## No-Allocation Proof

## Soak Run Outcome

## Final Objective 1 Verdict
```

- [ ] **Step 2: Make profiler text stable and report-friendly**

Update the format string in `vnv_final/agent/core/profiler.c` to this exact line:

```c
printf("[PROFILER] %s min=%lu max=%lu mean=%lu cycles samples=%lu\n",
       label,
       (unsigned long)stats->min_cycles,
       (unsigned long)stats->max_cycles,
       (unsigned long)mean,
       (unsigned long)stats->call_count);
```

This removes punctuation variance and makes copy-paste into the report easier.

- [ ] **Step 3: Build and flash the validated STM32 baseline**

Use the external STM32CubeIDE project `RTOSTwinF401RE_clean`:

1. build the firmware
2. flash the `NUCLEO-F401RE`
3. confirm the board enumerates as the ST-LINK virtual COM port

Evidence to preserve in the report:

- successful build summary
- successful flash summary
- exact board clock used for cycle conversion

- [ ] **Step 4: Capture cadence evidence over a fixed one-minute window**

Run the validated bridge command:

```bash
python vnv_final/bridge/main.py --port COM11 --baud 115200 --device-id nucleo-f401re
```

Then:

1. wait for steady packet flow
2. note the packet counter at `T0`
3. wait exactly `60` seconds
4. note the packet counter at `T1`
5. compute:

```text
packet_delta = packets_at_T1 - packets_at_T0
measured_hz = packet_delta / 60
```

Acceptance target:

- `measured_hz` must be within `9.5` to `10.5`

- [ ] **Step 5: Capture cycle evidence from the UART profiler output**

Collect at least one complete reporting window that contains both of these lines:

```text
[PROFILER] snapshot_capture min=... max=... mean=... cycles samples=100
[MEASURE] telemetry_cycle min=... max=... mean=... cycles samples=100
```

Record the captured numbers in the report.

- [ ] **Step 6: Convert cycles to overhead and write the exact formula into the report**

Write this exact formula in `docs/reports/Objective_1_STM32_Validation_Report.md`:

```text
microseconds = mean_cycles / 84
cpu_overhead_ratio = microseconds / 100000
cpu_overhead_percent = cpu_overhead_ratio * 100
```

Also write this explanation:

```text
The STM32F401RE validated baseline runs at 84 MHz, so 84 cycles equal 1 microsecond.
At 10 Hz, the telemetry cycle has a 100000 microsecond budget per iteration.
```

Acceptance target:

- `cpu_overhead_percent < 2.0`

- [ ] **Step 7: Commit**

```bash
git add vnv_final/agent/core/profiler.c docs/reports/Objective_1_STM32_Validation_Report.md
git commit -m "feat: add stm32 objective 1 cadence and timing evidence path"
```

---

### Task 3: Add Reproducible Static-RAM Accounting for the Agent

**Files:**
- Create: `tools/stm32/measure_agent_size.ps1`
- Modify: `docs/reports/Objective_1_STM32_Validation_Report.md`

- [ ] **Step 1: Create the STM32 map parser**

Create `tools/stm32/measure_agent_size.ps1`:

```powershell
param(
    [Parameter(Mandatory = $true)]
    [string]$MapFile
)

$ErrorActionPreference = "Stop"

if (-not (Test-Path -LiteralPath $MapFile)) {
    throw "Map file not found: $MapFile"
}

$agentObjectNames = @(
    'main.o',
    'snapshot.o',
    'encoder.o',
    'framer.o',
    'transport.o',
    'profiler.o',
    'hooks.o',
    'dwt.o',
    'uart_dma.o'
)

$sectionTotals = @{
    data = 0
    bss  = 0
}

$matchedRows = New-Object System.Collections.Generic.List[object]
$mapLines = Get-Content -Path $MapFile

foreach ($line in $mapLines) {
    if ($line -match '^\s*\.(data|bss)(\.[^\s]+)?\s+0x[0-9A-Fa-f]+\s+0x([0-9A-Fa-f]+)\s+(.+)$') {
        $sectionName = $matches[1]
        $sizeHex = $matches[3]
        $objectRef = $matches[4].Trim()

        foreach ($objectName in $agentObjectNames) {
            if ($objectRef -like "*$objectName*") {
                $sizeBytes = [Convert]::ToInt32($sizeHex, 16)
                $sectionTotals[$sectionName] += $sizeBytes
                $matchedRows.Add([pscustomobject]@{
                    Section = $sectionName
                    SizeBytes = $sizeBytes
                    Object = $objectRef
                }) | Out-Null
                break
            }
        }
    }
}

$total = $sectionTotals.data + $sectionTotals.bss

Write-Host "Agent .data bytes: $($sectionTotals.data)"
Write-Host "Agent .bss bytes:  $($sectionTotals.bss)"
Write-Host "Agent static RAM total bytes: $total"

$matchedRows | Sort-Object Section, Object | Format-Table -AutoSize
```

- [ ] **Step 2: Run the parser against the validated STM32 map file**

Run:

```bash
powershell -ExecutionPolicy Bypass -File tools/stm32/measure_agent_size.ps1 -MapFile "D:\path\to\RTOSTwinF401RE_clean\Debug\RTOSTwinF401RE_clean.map"
```

Expected:

- the script prints agent `.data`
- the script prints agent `.bss`
- the script prints agent static RAM total in bytes
- the table shows which agent object files contributed to the total

- [ ] **Step 3: Record the RAM-accounting method in the validation report**

Add this exact wording to the report:

```md
Static RAM is counted as:

`agent_static_ram_bytes = sum(.data for agent object files) + sum(.bss for agent object files)`

For this milestone, agent object files are:
- `main.o`
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
```

- [ ] **Step 4: Record the pass/fail fields in the report**

Add these fields:

```md
- Whole firmware `text`:
- Whole firmware `data`:
- Whole firmware `bss`:
- Agent `.data` bytes:
- Agent `.bss` bytes:
- Agent static RAM total:
- Pass/Fail against `< 10240 bytes`:
```

- [ ] **Step 5: Commit**

```bash
git add tools/stm32/measure_agent_size.ps1 docs/reports/Objective_1_STM32_Validation_Report.md
git commit -m "feat: add stm32 static ram accounting helper"
```

---

### Task 4: Prove the Telemetry Hot Path Uses No Dynamic Allocation

**Files:**
- Create: `tools/stm32/no_malloc_audit.ps1`
- Modify: `vnv_final/agent/tests/test_snapshot.c`
- Modify: `docs/reports/Objective_1_STM32_Validation_Report.md`

- [ ] **Step 1: Create the repo-relative audit helper**

Create `tools/stm32/no_malloc_audit.ps1`:

```powershell
$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$targets = @(
    (Join-Path $repoRoot 'vnv_final\agent\main.c'),
    (Join-Path $repoRoot 'vnv_final\agent\core\snapshot.c'),
    (Join-Path $repoRoot 'vnv_final\agent\core\encoder.c'),
    (Join-Path $repoRoot 'vnv_final\agent\core\framer.c'),
    (Join-Path $repoRoot 'vnv_final\agent\core\transport.c')
)

$patterns = @('malloc', 'calloc', 'realloc', 'free', 'pvPortMalloc', 'pvPortFree')
$matchesFound = $false

foreach ($file in $targets) {
    foreach ($pattern in $patterns) {
        $hits = Select-String -Path $file -Pattern $pattern -SimpleMatch
        if ($hits) {
            $matchesFound = $true
            $hits | ForEach-Object { Write-Host $_ }
        }
    }
}

if ($matchesFound) {
    throw "Allocation calls were found in the telemetry hot path."
}

Write-Host "No dynamic-allocation calls found in the telemetry hot path."
```

- [ ] **Step 2: Run the audit helper and verify it reports success**

Run:

```bash
powershell -ExecutionPolicy Bypass -File tools/stm32/no_malloc_audit.ps1
```

Expected:

- the script exits successfully
- the last line is `No dynamic-allocation calls found in the telemetry hot path.`

- [ ] **Step 3: Add an explicit audit anchor comment to the snapshot tests**

Add this comment near the top of `vnv_final/agent/tests/test_snapshot.c`:

```c
/*
 * Objective 1 audit anchor:
 * snapshot_capture() must continue using only fixed/static storage and must
 * not introduce malloc/pvPortMalloc into the telemetry hot path.
 */
```

- [ ] **Step 4: Record the no-allocation proof in the validation report**

Add this exact section:

```md
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
```

- [ ] **Step 5: Commit**

```bash
git add tools/stm32/no_malloc_audit.ps1 vnv_final/agent/tests/test_snapshot.c docs/reports/Objective_1_STM32_Validation_Report.md
git commit -m "test: add no-allocation audit evidence for stm32 objective 1"
```

---

### Task 5: Run and Record the STM32 Soak Validation

**Files:**
- Modify: `docs/reports/Objective_1_STM32_Validation_Report.md`
- Modify: `vnv_final/docs/reports/hardware_validation_2026-05-10.md`

- [ ] **Step 1: Write the soak protocol into the validation report before running it**

Add this exact section:

```md
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
```

- [ ] **Step 2: Run the soak and capture the minimum evidence set**

During the soak, record:

- start time
- end time
- total duration
- whether the firmware stayed alive the whole run
- whether the bridge stayed alive the whole run
- whether packet loss stayed at `0` or remained negligible
- whether `rtos_heap_free_bytes` stayed stable
- whether `rtos_heap_min_ever_bytes` stayed stable
- whether `rtos_heap_oom_projection_seconds` stayed `-1` absent an induced leak

- [ ] **Step 3: Add the exact soak-results table to the report**

```md
## Soak Run Outcome

| Field | Result |
|---|---|
| Start time | |
| End time | |
| Duration | |
| Firmware alive throughout | |
| Bridge alive throughout | |
| Packet loss result | |
| Heap free stability | |
| Heap min-ever stability | |
| OOM projection stability | |
| Final verdict | |
```

- [ ] **Step 4: Add a pointer from the hardware validation report**

Append this exact note to `vnv_final/docs/reports/hardware_validation_2026-05-10.md`:

```md
See also `docs/reports/Objective_1_STM32_Validation_Report.md` for the
follow-on production-baseline evidence covering cadence, CPU overhead, static
RAM, no-allocation proof, and soak validation for the STM32 baseline.
```

- [ ] **Step 5: Commit**

```bash
git add docs/reports/Objective_1_STM32_Validation_Report.md vnv_final/docs/reports/hardware_validation_2026-05-10.md
git commit -m "docs: add stm32 objective 1 soak validation evidence"
```

---

### Task 6: Close Objective 1 in the Public Project Docs

**Files:**
- Modify: `docs/reports/RTOSTwin_Complete_Report.md`
- Modify: `docs/reports/RTOSTwin_Master_Milestone_Record.md`
- Modify: `README.md`
- Modify: `vnv_final/docs/vnv_repo_completion_status.md`

- [ ] **Step 1: Update the formal Objective 1 wording in the complete report**

In `docs/reports/RTOSTwin_Complete_Report.md`, replace the current Objective 1
bullet with this wording:

```md
1. Build a lightweight C99 telemetry agent for FreeRTOS (v10.5+) and close the
   initial production-baseline objective on `NUCLEO-F401RE`, with
   `ESP32-P4` and `Teensy 4.1` retained as future expansion targets.
```

- [ ] **Step 2: Add an explicit Objective 1 status note near Section 4.1**

Insert this text near the objectives section:

```md
**Objective 1 status update (STM32 baseline):** Achieved on
`NUCLEO-F401RE` once the validation report in
`docs/reports/Objective_1_STM32_Validation_Report.md` records cadence, CPU
overhead, static RAM, no-allocation proof, and soak evidence.
```

- [ ] **Step 3: Add the milestone entry to the master milestone record**

Append this section to `docs/reports/RTOSTwin_Master_Milestone_Record.md`:

```md
### Objective 1 Closure - STM32 Production Baseline

- `NUCLEO-F401RE` baseline validated for cadence, CPU overhead, static RAM,
  no-allocation behavior, and soak stability
- `ESP32-P4` and `Teensy 4.1` remain future work
```

- [ ] **Step 4: Tighten both user-facing status docs**

Add this wording to `README.md` and `vnv_final/docs/vnv_repo_completion_status.md`:

```md
- Objective 1 is closed for the STM32 baseline on `NUCLEO-F401RE`
- `ESP32-P4` and `Teensy 4.1` remain planned expansion targets rather than
  completed baseline implementations
```

- [ ] **Step 5: Run a final docs consistency pass**

Run:

```bash
git diff -- docs/reports/RTOSTwin_Complete_Report.md docs/reports/RTOSTwin_Master_Milestone_Record.md README.md vnv_final/docs/vnv_repo_completion_status.md
```

Expected:

- all four docs agree that STM32 is complete
- no doc accidentally claims all three boards are complete
- the validation report path is spelled the same everywhere

- [ ] **Step 6: Commit**

```bash
git add docs/reports/RTOSTwin_Complete_Report.md docs/reports/RTOSTwin_Master_Milestone_Record.md README.md vnv_final/docs/vnv_repo_completion_status.md
git commit -m "docs: close objective 1 for stm32 production baseline"
```

---

## Self-Review

### Spec Coverage

The plan covers every closure requirement from the approved STM32 Objective 1 spec:

- telemetry cadence proof: Task 2
- CPU overhead proof: Task 2
- static RAM proof: Task 3
- no-allocation proof: Task 4
- soak validation: Task 5
- documentation closure: Task 6

### Placeholder Scan

This plan contains:

- exact file paths
- exact code for every new file
- explicit snippets for every required modification
- exact commands for verification and evidence collection
- explicit pass/fail thresholds for cadence, CPU overhead, and RAM

No step relies on `TODO`, `TBD`, or "figure this out later" wording.

### Type Consistency

The added measurement API is consistent end-to-end:

- `measurement_stats_t`
- `measurement_record()`
- `measurement_mean_cycles()`
- `measurement_reset()`

The final evidence file is named consistently everywhere:

- `docs/reports/Objective_1_STM32_Validation_Report.md`
