# RTOSTwin Project — Engineer Role Assignment

---

## 👤 Engineer: RYN
**Role Title:** Embedded Systems Engineer — Snapshot Engine, Wire Protocol Architect & Analytics  
**Project:** RTOSTwin — RTOS Telemetry Agent & OpenTelemetry Bridge  
**Assigned By:** Engineering Lead  
**Date:** March 2026  
**Project Version Target:** v1.0

---

> You are the foundation of this entire system. Every single byte that flows through RTOSTwin originates from code you write. The snapshot engine is the heartbeat of the agent, the wire format spec is the contract the entire project is built on, and the OOM analyzer is the intelligence that makes the tool actually useful in production. If your code is wrong, nothing works. If your code is right, everything works.

---

## 🏗️ Your Position in the System

```
[MCU - FreeRTOS Hardware]
         │
         │  (reads directly from FreeRTOS API)
         ▼
  ┌──────────────────────────────────┐
  │      SNAPSHOT ENGINE             │ ← YOU OWN THIS (C firmware side)
  │  snapshot_capture()              │
  │  CPU utilization (idle hook)     │
  │  DWT performance profiler        │
  └───────────────┬──────────────────┘
                  │ full_snapshot_t struct
                  ▼
  ┌──────────────────────────────────┐
  │    WIRE FORMAT SPECIFICATION     │ ← YOU DEFINE THIS (Week 3 deliverable)
  │    (Partner uses to build        │   This is the contract. Both tracks depend on it.
  │     framer + CRC + encoder)      │
  └───────────────┬──────────────────┘
                  │ binary packet stream (UART / USB CDC / UDP)
                  ▼
  ┌──────────────────────────────────┐
  │      PACKET DECODER (Python)     │ ← YOU OWN THIS (Python bridge side)
  │      CRC validator               │
  │      Sequence gap detector       │
  └───────────────┬──────────────────┘
                  │ decoded metric values
                  ▼
  ┌──────────────────────────────────┐
  │    OOM TREND ANALYZER (Python)   │ ← YOU OWN THIS
  │    Linear regression             │
  │    Rolling minimum detector      │
  │    rtos.heap.oom_projection_s    │
  └──────────────────────────────────┘
```

---

## 🚨 Read This First — Current Project Baseline

- **Canonical implementation root:** repository root `d:\digital_twin\`
- **Do not split new work across trees:** the top-level `agent/` folder is partial reference code, not the canonical delivery path.
- **V1 hardware rollout order:** `NUCLEO-F401RE` first, `ESP32-P4-Function-EV-Board` second, `Teensy 4.1` third.
- **Performance numbers below:** historical `STM32F4 @ 168 MHz` notes are reference sizing guidance only; accept/reject decisions must be re-measured on the active target board, starting with `NUCLEO-F401RE`.

## ✅ Your Exact Work Order (Start Here)

1. **Create and freeze the protocol contract first:** write `docs/wire_format_spec.md`, then align `agent/core/wire_format.h` with it exactly. Include packet types, CRC settings, delta tags, `DEVICE_INFO`, normalized units, and board metadata.
2. **Stabilize the snapshot/profiler baseline next:** make `agent/core/snapshot.*`, `agent/core/profiler.*`, and the FreeRTOS hook path work cleanly on the STM32 baseline with no heap allocation, one idle-hook implementation, valid `sequence_num`, and measured timing.
3. **Finish the typed decoder boundary:** `bridge/decoder.py` must output structured packet data that the rest of the bridge can consume without re-parsing payload bytes.
4. **Finish the OOM analytics layer:** implement `bridge/oom_analyzer.py` against the tested API and validate it using the shared `bridge/mock_device.py` stream owned by VNV.
5. **Validate end-to-end on one board before porting:** baseline success is `NUCLEO-F401RE -> decoder -> exporter -> Grafana`. Only after that should you extend protocol/snapshot semantics for `ESP32-P4` and `Teensy 4.1`.

---

## 🤝 Integration Contract (Do Not Break)

1. **Single-owner rule (no overlap):**
    - RYN owns: `agent/core/snapshot.*`, `agent/core/profiler.*`, `docs/wire_format_spec.md`, `agent/core/wire_format.h`, `bridge/decoder.py`, `bridge/oom_analyzer.py`
    - VNV owns: `agent/core/framer.*`, `agent/core/encoder.*`, `agent/core/transport.*`, `agent/hal/stm32/uart_dma.c`, `bridge/state_manager.py`, `bridge/prometheus_exporter.py`, `bridge/otlp_exporter.py`, `bridge/device_registry.py`, `bridge/main.py`, `bridge/mock_device.py`, `dashboard/rtostwin_dashboard.json`
2. **State reconstruction boundary is fixed:** your boundary ends at `DecodedPacket` from `bridge/decoder.py`; `bridge/state_manager.py` belongs to VNV and must consume your typed output rather than re-parsing wire bytes.
3. **Protocol freeze gate:** `docs/wire_format_spec.md` + `agent/core/wire_format.h` must be frozen as `v1` by end of Week 3 and approved by both engineers.
4. **No breaking protocol change on main:** if packet layout, enum values, field sizes, CRC settings, or delta tags change, bump `WF_PROTOCOL_VERSION` and add backward-compat notes before merge.
5. **Merge gate (required):** every merge touching protocol/framing/decoder must pass:
    - 3 golden packet vectors (byte-for-byte exact)
    - Decoder compatibility test for current protocol version
    - End-to-end mock stream test (`mock_device.py` -> `decoder.py`)
6. **Branch policy:** no direct commits to main for protocol or decoder/framer changes; use PR with both engineers as reviewers.

---

## 📋 Deliverables — C Firmware Side (agent/)

These files run **on the microcontroller**. Written in **C99**. V1 rollout is `NUCLEO-F401RE` first, then `ESP32-P4`, then `Teensy 4.1`. No C++, no stdlib heap functions.

---

### Deliverable 1 — Snapshot Engine
**Files:** `agent/core/snapshot.h`, `agent/core/snapshot.c`

**What it does:**  
This is the most critical function in the entire project. `snapshot_capture()` reads all relevant RTOS-internal state in a single atomic operation and writes it into a static struct. It is called by the telemetry task at 10 Hz (every 100 ms). It must complete in under 150 microseconds on STM32F4 at 168 MHz — always, even under worst-case RTOS load with 8 tasks running.

#### Data Structures to Define in `snapshot.h`

```c
#include "FreeRTOS.h"
#include "task.h"
#include <stdint.h>
#include <stdbool.h>

#define MAX_TASKS              16
#define TASK_NAME_MAX_LEN      16   /* Must match configMAX_TASK_NAME_LEN */

/* Per-task snapshot. One entry per active FreeRTOS task. */
typedef struct {
    char     name[TASK_NAME_MAX_LEN]; /* Null-padded task name                    */
    uint8_t  state;                   /* eTaskState: Running/Ready/Blocked/etc.   */
    uint8_t  priority;                /* uxCurrentPriority (not base priority)    */
    uint16_t stack_hwm_words;         /* uxTaskGetStackHighWaterMark() result      */
    uint32_t runtime_ticks;           /* ulRunTimeCounter (needs RUNTIME_STATS=1) */
} task_snapshot_t;

/* System-wide memory and timing snapshot. */
typedef struct {
    uint32_t heap_free_bytes;         /* xPortGetFreeHeapSize()                   */
    uint32_t heap_min_ever_bytes;     /* xPortGetMinimumEverFreeHeapSize()        */
    uint8_t  cpu_utilization_pct;     /* 0-100, computed from idle hook           */
} memory_snapshot_t;

/* Full snapshot written once per telemetry cycle. */
typedef struct {
    uint16_t         sequence_num;              /* Monotonically incrementing     */
    uint32_t         timestamp_ticks;           /* xTaskGetTickCount()            */
    uint8_t          task_count;                /* Number of active tasks         */
    task_snapshot_t  tasks[MAX_TASKS];          /* Per-task data                  */
    memory_snapshot_t memory;                    /* Heap + CPU                    */
} full_snapshot_t;
```

#### Functions to Implement in `snapshot.c`

```c
/* Initialize snapshot engine (register idle hook, enable DWT counter).
 * Call ONCE from main() before starting the RTOS scheduler. */
void snapshot_init(void);

/* Capture a full RTOS state snapshot into the provided struct.
 * - Must complete in < 150 µs WCET on STM32F4 @ 168 MHz.
 * - Zero dynamic allocation. No malloc, no pvPortMalloc.
 * - Caller-provided output struct (no hidden static state in this function).
 * - Uses taskENTER_CRITICAL / taskEXIT_CRITICAL for atomic capture. */
void snapshot_capture(full_snapshot_t *out);
```

#### Implementation Requirements

**1. Static-only allocation.** The `TaskStatus_t` array that `uxTaskGetSystemState()` requires must be declared `static` at file scope, NOT inside `snapshot_capture()`:

```c
/* snapshot.c — file scope, NOT inside snapshot_capture() */
static TaskStatus_t s_task_status_buf[MAX_TASKS];
```

If you declare it inside the function, it goes on the stack — for MAX_TASKS=16, that is 16 × ~40 bytes = 640 bytes of stack consumed every call (stack overflow risk). More dangerously, if you use `malloc` inside `snapshot_capture()`, you enter the FreeRTOS heap internals while potentially holding a critical section — this causes priority inversion and deadlocks.

**2. CPU Utilization via Idle Hook.** FreeRTOS provides a hook `vApplicationIdleHook()` that is called every time the idle task runs. You count idle cycles using DWT:

```c
/* Global counters — volatile because written by idle hook, read by snapshot_capture */
static volatile uint32_t s_idle_cycle_count  = 0;
static volatile uint32_t s_total_cycle_count = 0;

/* Called by FreeRTOS idle task every idle cycle.
 * configUSE_IDLE_HOOK must be 1 in FreeRTOSConfig.h */
void vApplicationIdleHook(void) {
    s_idle_cycle_count++;
}

/* In snapshot_capture(): compute CPU% from idle/total ratio */
```

**3. DWT Wrap-Around on 32-bit Counter.** `DWT->CYCCNT` wraps at 168,000,000 × 25.6 s. Elapsed cycles computation must handle wrap-around:

```c
uint32_t elapsed = (end >= start) ? (end - start) : (0xFFFFFFFF - start + end + 1);
```

**4. `configUSE_TRACE_FACILITY` = 1 and `configGENERATE_RUN_TIME_STATS` = 1 must be set in `FreeRTOSConfig.h`.** Without these, `uxTaskGetSystemState()` will not compile.

#### Critical Timing Constraints

On STM32F4 @ 168 MHz:
- `uxTaskGetSystemState()` on 8 tasks: ~15–25 µs (suspends scheduler, not interrupts)
- `uxTaskGetStackHighWaterMark()` per task: ~9 µs worst case (scans 0xA5A5A5A5 sentinel pattern upward through 2 KB stack)
- Total for 8 tasks: ~15 + (8 × 9) = ~87 µs average, up to ~150 µs under peak load

Your WCET target is **< 150 µs**. This is tight. You must verify it with DWT, not assume it.

**Verification Gate:** Print WCET over 10,000 consecutive calls under 8-task load. If any single call exceeds 150 µs, implement **stack watermark sampling** (scan only 1–2 tasks per snapshot on round-robin basis) and document the trade-off.

---

### Deliverable 2 — Wire Format Specification (The Contract)
**File:** `docs/wire_format_spec.md` + `agent/core/wire_format.h`

**Deadline:** End of Week 3 — both tracks block on this.

**What it does:**  
This is the single most important document you will produce in the first month. It defines the exact binary layout every byte of every packet follows. Your partner uses it to build the framer and encoder. You use it to build the Python decoder. It is the contract that makes parallel development possible.

#### What the Spec Must Define

1. **Complete byte-by-byte packet layout** with field name, offset, size, endianness, and allowed values for every field.
2. **CRC algorithm:** polynomial, initial value, input/output reflection settings, test vector (input bytes + expected CRC output).
3. **Packet TYPE field values:** `0x01` = delta snapshot, `0x02` = keyframe snapshot, `0x03` = device info (future).
4. **Delta encoding format:** tag-byte schema for each field type.
5. **Keyframe rule:** keyframe forced every N packets (define N — default 50).
6. **Versioning:** a 1-byte `PROTOCOL_VERSION` field for future compatibility.

#### `wire_format.h` — Shared Constants

```c
/* agent/core/wire_format.h
 * Single source of truth for protocol constants.
 * Include this in BOTH the C agent and reference in the Python decoder. */

#define WF_SYNC_0               0xAAU
#define WF_SYNC_1               0x55U
#define WF_PROTOCOL_VERSION     0x01U

#define WF_TYPE_DELTA           0x01U
#define WF_TYPE_KEYFRAME        0x02U
#define WF_TYPE_DEVICE_INFO     0x03U

#define WF_KEYFRAME_INTERVAL    50U     /* Force keyframe every N packets */
#define WF_MAX_PACKET_SIZE      512U    /* Maximum framed packet size, bytes */

#define WF_HEADER_SIZE          12U     /* SYNC(2)+VER(1)+TYPE(1)+SEQ(2)+TS(4)+LEN(2) */
#define WF_CRC_SIZE             2U
#define WF_OVERHEAD             (WF_HEADER_SIZE + WF_CRC_SIZE)

/* CRC-16-CCITT parameters */
#define WF_CRC_POLY             0x1021U
#define WF_CRC_INIT             0xFFFFU
```

**Verification Gate:** Produce 3 known-good packet byte sequences by hand (or with a Python script). Your partner's C framer must produce the same bytes. Your Python decoder must decode them correctly. All three must match. This is the integration sanity check.

---

### Deliverable 3 — DWT Performance Profiler
**Files:** `agent/core/profiler.h`, `agent/core/profiler.c`

**What it does:**  
A lightweight instrumentation layer that uses the ARM DWT cycle counter to measure WCET and average execution time of any function. Used to verify all performance claims in the final report.

```c
/* Enable DWT cycle counter. Call once in snapshot_init(). */
void profiler_init(void);

/* Begin timing a section. Returns the current CYCCNT value. */
uint32_t profiler_start(void);

/* End timing a section. Returns elapsed cycles (handles 32-bit wrap-around). */
uint32_t profiler_stop(uint32_t start_cycles);

/* Profiler statistics — updated on every call to profiler_stop(). */
typedef struct {
    uint32_t min_cycles;   /* Minimum ever measured */
    uint32_t max_cycles;   /* Maximum ever measured (WCET) */
    uint32_t call_count;   /* Total calls measured */
    uint64_t total_cycles; /* Sum of all measured cycles (for mean) */
} profiler_stats_t;

/* Update a stats struct with a new measurement. */
void profiler_record(profiler_stats_t *stats, uint32_t elapsed_cycles);

/* Print stats to UART (for development verification). */
void profiler_report(const profiler_stats_t *stats, const char *label);
```

**Usage example in telemetry task:**
```c
profiler_stats_t snapshot_stats = {0};

void telemetry_task(void *param) {
    full_snapshot_t snap;
    while (1) {
        uint32_t t = profiler_start();
        snapshot_capture(&snap);
        profiler_record(&snapshot_stats, profiler_stop(t));

        /* Every 100 calls: report to UART */
        if (snapshot_stats.call_count % 100 == 0) {
            profiler_report(&snapshot_stats, "snapshot_capture");
        }
        vTaskDelay(pdMS_TO_TICKS(100)); /* 10 Hz */
    }
}
```

**Output format you must produce (for README benchmarks):**

```
[PROFILER] snapshot_capture: min=14203 max=24187 mean=16842 cycles | min=84µs max=144µs mean=100µs @ 168MHz
```

**Verification Gate:** Run 10,000 calls with 8 tasks actively running. Report min/max/mean. All numbers become the real benchmark data in your paper and README.

---

## 📋 Deliverables — Python Bridge Side (bridge/)

Runs on a **PC or Raspberry Pi**. Python 3.9+. You can develop and test this entirely without the MCU using the shared `bridge/mock_device.py` owned by VNV.

---

### Deliverable 4 — Binary Packet Decoder
**Files:** `bridge/decoder.py`

**What it does:**  
This is the inverse of your partner's packet framer. It reads raw bytes from the serial port, finds packet boundaries using the sync bytes (`0xAA 0x55`), validates the CRC, unpacks all fields, and exposes a clean Python dataclass as output.

#### Decoder State Machine

The decoder is a state machine because binary data arrives as a byte stream — you cannot assume packet boundaries are aligned with read boundaries. Your decoder must handle:
- **Mid-packet disconnect/reconnect** (re-sync on next `0xAA 0x55` pair)
- **Corrupted packets** (CRC mismatch → discard, log drop count)
- **Sequence number gaps** (bridge knows how many packets were dropped in the channel)

```python
from dataclasses import dataclass, field
from typing import List, Optional
import struct

@dataclass
class TaskSnapshot:
    name: str
    state: int          # eTaskState enum value
    priority: int
    stack_hwm_words: int
    runtime_ticks: int

@dataclass
class DecodedPacket:
    protocol_version: int
    packet_type: int    # 0x01=delta, 0x02=keyframe
    sequence_num: int
    timestamp_ticks: int
    task_count: int
    tasks: List[TaskSnapshot]
    heap_free_bytes: int
    heap_min_ever_bytes: int
    cpu_utilization_pct: int
    crc_valid: bool


class PacketDecoder:
    """
    Stateful binary stream decoder for RTOSTwin packets.

    Usage:
        decoder = PacketDecoder()
        for byte in serial_stream:
            result = decoder.feed_byte(byte)
            if result is not None:
                process(result)  # DecodedPacket
    """

    def feed_byte(self, byte: int) -> Optional[DecodedPacket]:
        """Feed one byte. Returns DecodedPacket when a complete valid packet is assembled."""
        ...

    def feed_bytes(self, data: bytes) -> List[DecodedPacket]:
        """Feed multiple bytes. Returns list of all complete packets decoded."""
        ...

    @property
    def drop_count(self) -> int:
        """Number of packets dropped due to CRC failure or framing error."""
        ...

    @property
    def sequence_gap_count(self) -> int:
        """Number of sequence number gaps detected (packets dropped in the channel)."""
        ...
```

#### CRC Validation

Must implement the same CRC-16-CCITT as the C side in pure Python:

```python
def crc16_ccitt(data: bytes, initial: int = 0xFFFF) -> int:
    """
    CRC-16-CCITT (polynomial 0x1021, no reflection).
    Must produce identical output to the C crc16_ccitt() implementation.

    Test vector: crc16_ccitt(b'\\x01\\x00\\x01') == 0x???? (exact value in wire_format_spec.md)
    """
    crc = initial
    for byte in data:
        crc ^= byte << 8
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021) if (crc & 0x8000) else (crc << 1)
    return crc & 0xFFFF
```

**Verification Gate:**
- `pytest test_decoder.py` passes 100% including:
  - Correct decode of 5 known-good packet byte sequences
  - CRC failure → packet discarded, `drop_count` incremented
  - Sequence gap (seq 5 → seq 7) → `sequence_gap_count` incremented by 1
  - Mid-stream garbage bytes → decoder re-syncs correctly after next `0xAA 0x55`

---

### Deliverable 5 — OOM Trend Analyzer
**Files:** `bridge/oom_analyzer.py`

**What it does:**  
The intelligence of the bridge. Maintains a sliding window of `heap_free_bytes` measurements and uses two statistically complementary detectors to identify memory leaks and project time-to-OOM. This gets exported as the `rtos_heap_oom_projection_seconds` Prometheus metric and the matching OTLP metric.

#### Detector 1 — Linear Regression (Monotonic Leak Detection)

```python
from collections import deque
import numpy as np
from scipy.stats import linregress

class OOMAnalyzer:
    """
    Two-detector OOM prediction system.

    Detector 1: Linear regression on heap_free_bytes sliding window.
                Catches steady monotonic leaks.
    Detector 2: Rolling minimum comparison.
                Catches bursty/sawtooth leaks that fool linear regression.
    """

    def __init__(self,
                 window_size: int = 600,        # 600 samples @ 10 Hz = 60 seconds
                 min_r_squared: float = 0.7,    # Minimum R² for regression to be trusted
                 rolling_min_threshold: float = 0.10,  # 10% drop triggers secondary alert
                 total_heap_bytes: int = 131072):       # Device total heap (configurable)
        self.window_size = window_size
        self.min_r_squared = min_r_squared
        self.rolling_min_threshold = rolling_min_threshold
        self.total_heap_bytes = total_heap_bytes

        self._samples: deque[tuple[float, int]] = deque(maxlen=window_size)
        # (timestamp_seconds, heap_free_bytes)

    def add_sample(self, timestamp_s: float, heap_free_bytes: int) -> None:
        """Add a new heap measurement. Called on every decoded packet."""
        self._samples.append((timestamp_s, heap_free_bytes))

    def get_projection_seconds(self) -> float:
        """
        Returns projected seconds until OOM.
        -1.0 means stable (no leak detected by either detector).
        0.0 means already at OOM.

        Logic:
        1. If window has < 30 samples: return -1.0 (not enough data yet)
        2. Run linear regression on window.
           - If slope >= 0: heap is stable or growing → return -1.0
           - If slope < 0 and R² < min_r_squared: noisy data, not confident → check rolling min only
           - If slope < 0 and R² >= min_r_squared: leak confirmed → project: heap_current / abs(slope)
        3. Rolling minimum check (independent):
           - rolling_min = min(heap in window)
           - if (rolling_min_start - rolling_min_now) / total_heap > threshold: secondary alert
        """
        ...

    @property
    def regression_slope_bytes_per_second(self) -> float:
        """Current regression slope. Negative = leak. Positive = stable/recovering."""
        ...

    @property
    def rolling_minimum_bytes(self) -> int:
        """Rolling minimum of heap within the current window."""
        ...

    @property
    def r_squared(self) -> float:
        """Goodness-of-fit of the current regression. 0.0–1.0."""
        ...
```

#### Why Two Detectors Are Necessary

| Leak Pattern | Linear Regression | Rolling Minimum |
|---|---|---|
| Steady monotonic leak (10 bytes/sec) | ✅ Catches it | ✅ Catches it |
| Bursty: allocate 10KB every 30s, free 9KB | ❌ Slope ≈ 0, misses it | ✅ Rolling min drifts down |
| Normal allocation/free churn (no leak) | ✅ Slope ≈ 0, correctly ignores | ✅ Rolling min stable |
| Large one-time alloc (not a leak) | ⚠️ Might see negative slope briefly | ⚠️ Might trigger briefly (threshold tuning needed) |

**Both detectors together** catch either pattern. Document this design decision with this table in the README.

#### Verification Gates

```bash
# Gate 1: Monotonic leak detection
pytest test_oom_analyzer.py::test_monotonic_leak
# Inject 600 samples at 1 byte/sec drop. Projection must be detected within 60 seconds.

# Gate 2: Bursty leak detection
pytest test_oom_analyzer.py::test_sawtooth_leak
# Inject sawtooth pattern with 5% net decrease per window. Rolling min alert fires.

# Gate 3: No false positives
pytest test_oom_analyzer.py::test_stable_no_alert
# Inject 3600 samples (1 hour) of realistic allocation churn. Zero alerts fired.

# Gate 4: Integration — real device
# Run for 24 hours on steady-state firmware. Zero false positive alerts.
```

---

## 🚧 Critical Constraints (Apply to ALL Your Deliverables)

1. **Zero dynamic allocation in C firmware.** `malloc` and `pvPortMalloc` are absolutely forbidden inside `snapshot_capture()` and `profiler_record()`. Violating this causes priority inversion. This is not negotiable.
2. **`volatile` on every cross-context shared variable.** The idle hook counter `s_idle_cycle_count` is written in idle task context and read by `snapshot_capture()`. It must be `volatile`. Without it, the optimizer can cache or eliminate reads.
3. **Critical sections must be minimal.** The critical section inside `snapshot_capture()` disables IRQs. Keep it under 50 µs. Do not do any computation (CRC, encoding) inside the critical section — only the raw reads.
4. **Python decoder must be stateful and stream-safe.** Never assume a `read()` call aligns with packet boundaries. Feed one byte at a time in your state machine.
5. **OOM analyzer defaults must be conservative.** A false positive OOM alert from a deployed device that causes an engineer to fly out to a field site at 2 AM is infinitely worse than a missed alert. Keep `min_r_squared` at 0.7 and `rolling_min_threshold` at 10% by default. Make both configurable.

---

## ✅ Definition of Done

Your role is complete when every item in this checklist is checked off:

**C Firmware:**
- [ ] `snapshot_capture()` completes in < 150 µs WCET over 10,000 calls — verified by `profiler_report()`
- [ ] Zero `pvPortMalloc` calls inside any of your functions — verified by mock `pvPortMalloc` wrapper in unit tests
- [ ] CPU utilization percentage tracks correctly against a known-load test task — within ±5%
- [ ] Wire format spec document signed off by both engineers before Week 4
- [ ] `wire_format.h` committed and referenced by both C framer and Python decoder
- [ ] All C files compile with `-std=c99 -Wall -Wextra -Werror` — zero warnings

**Python Bridge:**
- [ ] Decoder correctly decodes 5 known-good packet byte sequences — pytest
- [ ] Decoder correctly rejects 3 corrupted packets (CRC fail) and increments drop count — pytest
- [ ] Decoder re-syncs correctly after arbitrary garbage bytes inserted mid-stream — pytest
- [ ] Sequence gap detection: gaps of 1, 5, and 100 packets detected accurately — pytest
- [ ] OOM analyzer detects a 1 byte/sec monotonic leak within 60 seconds — pytest
- [ ] OOM analyzer detects sawtooth leak with 5% net decrease per window — pytest
- [ ] Zero false positive OOM alerts over 3600 steady-state mock samples — pytest
- [ ] Decoder integration passes against shared `bridge/mock_device.py` in both `--mode normal` and `--mode leak`

---

## 🔗 Key Dependencies

| What You Need | Who Provides It | When |
|---|---|---|
| `full_snapshot_t` struct — from yourself | You defined it in `snapshot.h` | Week 3 |
| Wire format byte layout | **You define it** -> partner uses it | **Week 3 — you go first** |
| CRC test vectors | **You define them** in wire format spec | Week 3 |
| Shared mock packet stream (`bridge/mock_device.py`) | Partner (VNV) | Week 4 |
| DMA transport working (to test end-to-end) | Partner (Deliverable: DMA transport) | Week 5 |
| Prometheus endpoint live (to feed OOM data into Grafana) | Partner (Deliverable: Prometheus) | Week 10 |

> Use partner-owned `bridge/mock_device.py` from Week 4 onward for decoder and OOM integration tests. Do not fork protocol behavior in a second local mock.

---

## 📅 Timeline

| Milestone | Target Week | Gate |
|---|---|---|
| `snapshot_capture()` compiles, prints to UART | Week 3 | Visual verify of UART output |
| Wire format spec finalized + `wire_format.h` committed | **Week 3** | **Both engineers sign off before Week 4** |
| `snapshot_capture()` WCET < 150 µs — measured | Week 4 | `profiler_report()` printed over 10,000 calls |
| Decoder + shared mock integration functional | Week 4 | `bridge/mock_device.py` parses in both modes |
| Python decoder passes all pytest cases | Week 5 | `pytest test_decoder.py` green |
| End-to-end: MCU → UART → decoder → decoded struct in Python | Week 6 | First live data from real device |
| OOM analyzer passes all pytest cases | Week 12 | All 4 OOM test gates pass |
| OOM projection appears in Grafana | Week 15 | `rtos_heap_oom_projection_seconds` visible |
| 24-hour steady-state zero false-positive validation | Week 18 | CI integration test green |

---

## 📚 Required Reading (Before You Write a Line of Code)

These are the exact sections of the RTOSTwin Complete Report you must have read and understood before starting each deliverable:

| Deliverable | Report Sections |
|---|---|
| Snapshot Engine | Part 5.2: Component 1 — entire section (esp. "Known Hard Technical Problems") |
| Wire Format Spec | Part 5.2: "Packet Wire Format" table |
| DWT Profiler | Part 7.1: "Agent CPU overhead" and "Agent WCET" rows |
| Python Decoder | Part 5.2: "Packet Wire Format" + Part 5.3: "Pull vs Push" decision |
| OOM Analyzer | Part 5.3: "Memory Trend Analysis — Design and Honest Limitations" |

---

*RTOSTwin v1.0 — Role Document | Engineering Lead | March 2026*
