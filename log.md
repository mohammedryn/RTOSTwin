# RTOSTwin — Project Audit Log

> **Purpose:** Factual record of the project's current state as of May 2026.
> Every claim in this file is verified against the actual files in the repository.
> No estimates, no opinions dressed as facts, no fabricated metrics.

---

## 1. What This Project Is

RTOSTwin is a lightweight, open-source RTOS telemetry agent and OpenTelemetry bridge. It has three components:

| Component | Runs On | Language | Role |
|-----------|---------|----------|------|
| Telemetry Agent | MCU (STM32F401RE / ESP32-P4 / Teensy 4.1) | C99 | Captures FreeRTOS state, delta-encodes, transmits via DMA |
| OTLP/Prometheus Bridge | Host PC / Raspberry Pi | Python 3.9+ | Decodes binary stream, emits Prometheus metrics + OTLP |
| Grafana Dashboard | Grafana instance | JSON / PromQL | Visualizes RTOS metrics |

**The gap it fills:** As of February 2026, no open-source, permissively licensed tool bridges FreeRTOS internal state (task scheduling, heap usage, stack watermarks) to the standard open observability stack (OTLP/Prometheus/Grafana). Memfault and Percepio Detect fill Mode C commercially. RTOSTwin targets the open-source alternative.

---

## 2. Repository File Inventory

### Root Level

| File | Status | Notes |
|------|--------|-------|
| `README.md` | Exists, readable | Project overview, architecture diagram, quick-start summary |
| `docker-compose.yml` | Valid, correct | Prometheus + Grafana, correct `host.docker.internal:host-gateway` extra_host |
| `log.md` | This file | — |

### `agent/` — C Firmware

| File | Valid C? | Implementation Status |
|------|---------|----------------------|
| `agent/main.c` | Compiles as C | `frame_packet()` called with 4 args; `framer.h` declares 7 — **will not compile** |
| `agent/core/wire_format.h` | Yes | Protocol constants correct: `SYNC_0=0xAA`, `SYNC_1=0x55`, `CRC_POLY=0x1021` |
| `agent/core/snapshot.h` | Yes | Structs correct. `full_snapshot_t.sequence_num` declared but **never set** by `snapshot_capture()` |
| `agent/core/snapshot.c` | Yes | Static buffer, no malloc, correct FreeRTOS API usage. `sequence_num` field left at zero |
| `agent/core/encoder.h` | Yes | Delta encoding interface correct |
| `agent/core/encoder.c` | Yes | Tag-byte delta protocol implemented. `TAG_MEM_HEAP_MIN` and `TAG_TASK_RUNTIME` defined but **never encoded** |
| `agent/core/framer.h` | Yes | Declares `frame_packet` with `seq_num` as external parameter — contradicts `TECH_SPEC.md:174` |
| `agent/core/framer.c` | Yes | CRC-16-CCITT with standard test vector 0x29B1 |
| `agent/core/transport.h` | Yes | Interface correct |
| `agent/core/transport.c` | Yes | Non-blocking DMA, drop counter, correct |
| `agent/core/profiler.h` | Yes | DWT interface correct |
| `agent/core/profiler.c` | Yes | Min/max/mean statistics, wrap-around handled |
| `agent/hal/stm32/dwt.h` | Yes | ARM DWT interface |
| `agent/hal/stm32/dwt.c` | Yes | TRCENA + LAR unlock + CYCCNT enable, wrap-around correct |
| `agent/hal/stm32/uart_dma.h` | Yes | Interface correct |
| `agent/hal/stm32/uart_dma.c` | Yes | `HAL_UART_GetState()` check before DMA, non-blocking |
| `agent/hal/esp32/` | **Does not exist** | ESP32-P4 HAL is a v1.0 requirement — missing entirely |
| `agent/freertos/hooks.c` | Yes | **Duplicate** `vApplicationIdleHook` — also defined in `snapshot.c:96` |
| `agent/tests/test_crc.c` | Yes | Standard CRC vector test |
| `agent/tests/test_encoder.c` | Yes | Keyframe and delta tests |
| `agent/tests/test_framer.c` | Yes | Packet framing tests |
| `agent/tests/test_profiler.c` | Yes | Min/max/mean and wrap-around |
| `agent/tests/test_snapshot.c` | Yes | Mocked FreeRTOS APIs |
| `agent/tests/unity_mock.h` | Yes | Lightweight Unity replacement macros |
| `agent/tests/mocks/FreeRTOS.h` | Yes | Type stubs for host-side testing |
| `agent/tests/mocks/task.h` | Yes | Mock FreeRTOS task API |

### `bridge/` — Python Host Bridge

> **Critical:** Seven Python files start with `/**` on line 1.
> `/**` is not valid Python syntax. Python will raise `SyntaxError` immediately on import or execution.
> Files using `"""` triple-quote docstrings are valid Python.

| File | Valid Python? | Implementation Status |
|------|--------------|----------------------|
| `bridge/decoder.py` | **Yes** (`"""` docstring) | State machine parser, CRC, sequence gap detection — fully functional |
| `bridge/mock_device.py` | **Yes** (`"""` docstring) | Generates valid framed packets at 10 Hz. No `--mode` CLI. One hardcoded mode (always leaking). Extra args silently ignored. |
| `bridge/tests/test_decoder.py` | **Yes** | 6 tests — runnable, correct |
| `bridge/tests/conftest.py` | **Yes** | `valid_packet_bytes()` fixture — correct |
| `bridge/tests/test_oom_analyzer.py` | **Yes** (file itself) | Tests exist but **cannot run** — `import bridge.oom_analyzer` fails due to `/**` on `oom_analyzer.py:1` |
| `bridge/config.py` | **No** — `/**` line 1 | Logic is correct but file is unparseable Python |
| `bridge/device_registry.py` | **No** — `/**` line 1 | Logic is correct but file is unparseable Python |
| `bridge/main.py` | **No** — `/**` line 1 | Pipeline wiring exists but file is unparseable Python |
| `bridge/state_manager.py` | **No** — `/**` line 1 | Also has payload field access bug (see §4) |
| `bridge/prometheus_exporter.py` | **No** — `/**` line 1 | Also has metric name mismatches (see §4) |
| `bridge/otlp_exporter.py` | **No** — `/**` line 1 | Also incomplete: only 1 of 8 metrics, callback not registered |
| `bridge/oom_analyzer.py` | **No** — `/**` line 1 | Also has API signature mismatch (see §4) |
| `bridge/requirements.txt` | N/A | Lists correct dependencies: `pyserial`, `scipy`, `prometheus-client`, `opentelemetry-sdk`, `opentelemetry-exporter-otlp` |

### `dashboard/` and Grafana Provisioning

| File | Status | Notes |
|------|--------|-------|
| `dashboard/rtostwin_dashboard.json` | Exists, valid JSON | Only 2 of 6 required panels: CPU gauge + Heap time series. Missing: task state table, stack watermark, OOM countdown, packet loss. |
| `grafana/provisioning/datasources/datasource.yml` | Correct | Datasource UID `P1849AB3917F44AF` matches the UID referenced in `dashboard/rtostwin_dashboard.json:27` |
| `grafana/provisioning/dashboards/provider.yml` | Correct | Points to `/etc/grafana/provisioning/dashboards` inside container |
| Dashboard JSON in provisioning path | **Missing** | `dashboard/rtostwin_dashboard.json` is NOT inside `grafana/provisioning/dashboards/`. `docker-compose up` will NOT auto-load the dashboard. |

### Infrastructure

| File | Status | Notes |
|------|--------|-------|
| `prometheus/prometheus.yml` | Correct | Scrapes `host.docker.internal:8000` every 5s — correct for Docker-to-host |
| `.github/workflows/ci.yml` | Broken | `cd digitaltwin-main` path does not exist. CI fails on every push. Missing: Unity C test runner, `arm-none-eabi-size` check, mypy. |

### Documentation and PRD

| File | Status |
|------|--------|
| `PRD/PRD.md` | Complete product requirements |
| `PRD/ARCHITECTURE.md` | Complete, but `frame_packet` data flow on line 66 shows external `seq` — contradicts `TECH_SPEC.md:174` |
| `PRD/TECH_SPEC.md` | Authoritative spec. Source of truth for metric names, API signatures, wire format |
| `PRD/CODING_RULES.md` | Complete |
| `PRD/FILE_STRUCTURE.md` | Complete |
| `PRD/TASK_QUEUE.md` | 25 tasks with verification gates |
| `PRD/WAY_TO_USE.md` | Complete |
| `PRD/AI_WORKFLOW_RULES.md` | Complete |
| `PRD/roles/ryn_role_assignment.md` | References `docs/wire_format_spec.md` — **file does not exist** |
| `PRD/roles/vnv_role_assignment.md` | Complete |
| `docs/quick_start.md` | Exists but inconsistent — see §4 Bug #8 |
| `docs/reports/RTOSTwin_Complete_Report.md` | Complete, detailed technical report |
| `docs/wire_format_spec.md` | **Does not exist** — required by `PRD/roles/ryn_role_assignment.md:66` |
| `semantic-conventions/rtos_metrics.md` | Exists but basic — not in full OTel proposal format |
| `examples/` | **Directory does not exist** — `blinky_twin` and `sensor_system` are v1.0 deliverables |

### Learning Materials

| Path | Status |
|------|--------|
| `rtos_twin_learning/roadmap/` | 12-week learning roadmap for both engineers |
| `rtos_twin_learning/ryn/` | Week 1 explanations, notes, theory answers, homework stubs |
| `rtos_twin_learning/vnv/` | Week 1 explanations, notes, theory answers, homework stubs |

---

## 3. Confirmed Bugs — Ordered by Severity

### Bug 1 — Seven Python Bridge Files Are Unparseable (Severity: CRITICAL)

**Files:** `bridge/config.py`, `bridge/device_registry.py`, `bridge/main.py`, `bridge/state_manager.py`, `bridge/prometheus_exporter.py`, `bridge/otlp_exporter.py`, `bridge/oom_analyzer.py`

**Cause:** Every file begins with `/**` on line 1 — a C-style block comment opener. In Python, `/` requires operands on both sides. `/**` is a `SyntaxError`.

**Effect:** `python -m py_compile bridge/config.py` fails. Any `import` of any of these modules fails. The entire Python bridge is non-functional. The OOM tests in `test_oom_analyzer.py` cannot run because the import fails.

**Fix:** Replace `/** ... */` with `"""..."""` (Python docstring) on every affected file.

---

### Bug 2 — `main.c` Calls `frame_packet()` With Wrong Argument Count (Severity: CRITICAL)

**Location:** `agent/main.c:54`

```c
// Current (wrong — 4 arguments):
uint16_t frame_len = frame_packet(s_payload_buffer, enc_len, s_framed_buffer, sizeof(s_framed_buffer));

// framer.h declares (7 arguments):
uint16_t frame_packet(const uint8_t *payload, uint16_t payload_len,
                      uint8_t pkt_type, uint16_t seq_num,
                      uint32_t timestamp_ms, uint8_t *out_buf, uint16_t out_buf_size);
```

**Effect:** Will not compile. The firmware cannot be built until this is corrected.

---

### Bug 3 — `vApplicationIdleHook` Defined Twice (Severity: HIGH)

**Locations:**
- `agent/core/snapshot.c:96` — increments `s_idle_ticks` and `s_period_ticks`
- `agent/freertos/hooks.c:39` — increments `s_idle_cycle_count`

**Effect:** If both translation units are linked into the same firmware binary (the intended build), the linker will fail with "multiple definition of `vApplicationIdleHook`." The two implementations also use different counter variables, so they are not equivalent — one cannot simply be removed without deciding which CPU measurement approach to keep.

---

### Bug 4 — `snapshot_capture()` Never Sets `sequence_num` (Severity: HIGH)

**Location:** `agent/core/snapshot.h:42` declares `sequence_num` in `full_snapshot_t`. `agent/core/snapshot.c:33` — `snapshot_capture()` never writes to `out->sequence_num`.

**Effect:** The field is always zero. Note: per `PRD/TECH_SPEC.md:174`, the framer owns packet sequence internally, so receiver gap detection uses the framer's counter — not the snapshot's. However, the `sequence_num` field in the struct is a dead field that misleads any reader of the code. The contract between snapshot and framer needs to be resolved: either remove the field from the struct, or have `snapshot_capture()` manage and increment it.

---

### Bug 5 — `frame_packet` Contract Drift Across Four Sources (Severity: HIGH)

The `frame_packet` function signature is inconsistent across four locations:

| Source | External `seq` param? | Parameter count |
|--------|----------------------|-----------------|
| `PRD/ARCHITECTURE.md:66` | Yes — shows `seq` in data flow | 7 |
| `PRD/TECH_SPEC.md:168–174` | No — framer owns seq internally | 6 |
| `agent/core/framer.h:40` | Yes — `seq_num` is an explicit parameter | 7 |
| `agent/main.c:54` (call site) | — | 4 (wrong regardless) |

`ARCHITECTURE.md` and `framer.h` agree with each other (external `seq`), but both contradict `TECH_SPEC.md` (framer-owned seq). The canonical source of truth is `TECH_SPEC.md`. Either `framer.h` must be updated to remove `seq_num` and manage it internally, or `TECH_SPEC.md` must be updated to reflect external seq. Either way, `main.c` must be fixed to pass the correct arguments.

---

### Bug 6 — `state_manager.py` Accesses Fields That Don't Exist on `DecodedPacket` (Severity: HIGH)

**Location:** `bridge/state_manager.py:44–57`

`DecodedPacket` in `bridge/decoder.py:54` has only four fields:
```python
packet_type: int
sequence_num: int
timestamp_ms: int      # named timestamp_ms
payload: bytes         # raw bytes only
```

`state_manager.py` accesses:
```python
packet.heap_free_bytes       # AttributeError
packet.heap_min_ever_bytes   # AttributeError
packet.cpu_utilization_pct   # AttributeError
packet.tasks                 # AttributeError
packet.task_deltas           # AttributeError
packet.timestamp_ticks       # AttributeError — field is named timestamp_ms in decoder
```

**Effect:** `StateManager.update()` raises `AttributeError` on every call. The end-to-end pipeline has no function that parses `payload: bytes` (the raw delta-encoded or keyframe bytes) into structured fields.

**Additional sub-bug:** The field is named `timestamp_ms` in `decoder.py` but `timestamp_ticks` in `state_manager.py`. These are not aliases — they are different names referring to the same field. Even if the other payload fields existed, this would still raise `AttributeError`.

---

### Bug 7 — Prometheus Metric Names Do Not Match `TECH_SPEC.md` (Severity: MEDIUM)

`PRD/TECH_SPEC.md:237–240` specifies these 8 metric names:

| Spec Metric Name | `prometheus_exporter.py` Has | Match? |
|-----------------|------------------------------|--------|
| `rtos_task_state` | `rtos_task_state` | ✅ |
| `rtos_task_stack_watermark_bytes` | `rtos_task_stack_hwm_bytes` | ❌ |
| `rtos_task_cpu_ratio` | — (missing entirely) | ❌ |
| `rtos_heap_free_bytes` | `rtos_heap_free_bytes` | ✅ |
| `rtos_heap_min_ever_bytes` | `rtos_heap_min_ever_bytes` | ✅ |
| `rtos_heap_oom_projection_seconds` | — (missing) | ❌ |
| `rtos_cpu_utilization_ratio` | `rtos_cpu_utilization_pct` | ❌ (wrong name + wrong unit: ratio vs percent) |
| `rtos_telemetry_packet_loss_ratio` | — (missing) | ❌ |

Also present but not in spec: `rtos_task_runtime_ticks`. 3 of 8 spec metrics are correctly named.

---

### Bug 8 — `OOMAnalyzer.add_sample()` API Mismatches `TECH_SPEC.md` (Severity: MEDIUM)

`PRD/TECH_SPEC.md:214–218` specifies:
```python
def __init__(self, window_size=600, min_r_squared=0.7,
             rolling_min_threshold=0.10, total_heap_bytes=131072): ...
def add_sample(self, timestamp_s: float, heap_free_bytes: int) -> None: ...
```

`bridge/oom_analyzer.py` implements:
```python
def __init__(self, window_size: int = 600, r_squared_threshold: float = 0.7):
    # no rolling_min_threshold, no total_heap_bytes
def add_sample(self, heap_bytes: int):
    # no timestamp_s parameter, wrong param name
```

`bridge/main.py` calls `oom.add_sample(state.heap_free_bytes)` — consistent with the current (wrong) implementation, not with the spec.

The rolling minimum detector (second of two required detection strategies from `RTOSTwin_Complete_Report.md §5.3`) is entirely absent. Only linear regression is implemented.

---

### Bug 9 — CI Pipeline Fails on Every Push (Severity: MEDIUM)

**Location:** `.github/workflows/ci.yml:25`

```yaml
- name: Run Python tests
  run: |
    cd digitaltwin-main        # This path does not exist in the repo
    python -m pytest bridge/tests/ -v
```

`digitaltwin-main` is a path from an old nested repository structure. The directory does not exist. Every push to `main` fails CI before running a single test.

**Additional gaps in CI:**
- No Unity C test runner job
- No `arm-none-eabi-size` size check against 10 KB limit
- No mypy type-checking job
- Only Python 3.10 tested (spec requires matrix of 3.9 and 3.11)

---

### Bug 10 — Grafana Dashboard Not in Auto-Provision Path (Severity: MEDIUM)

`docker-compose.yml` mounts `./grafana/provisioning` → `/etc/grafana/provisioning`.
`grafana/provisioning/dashboards/provider.yml` serves dashboards from `/etc/grafana/provisioning/dashboards`.

The dashboard JSON lives at `dashboard/rtostwin_dashboard.json` — this path is **not mounted** into the container. Running `docker-compose up` will not auto-load the dashboard. A user must manually import it through the Grafana UI.

**Note:** The Grafana datasource UID is correctly aligned. `dashboard/rtostwin_dashboard.json:27` and `grafana/provisioning/datasources/datasource.yml:9` both use `uid: P1849AB3917F44AF`. Once the dashboard is in the provisioning path, it will connect to the datasource correctly.

---

### Bug 11 — `docs/quick_start.md` Documents a Non-Existent CLI Flag (Severity: LOW)

`docs/quick_start.md:38`:
```bash
python bridge/mock_device.py --mode normal
```

`bridge/mock_device.py` has no `argparse` and does not read `sys.argv`. Extra arguments are **silently ignored** — no error is raised. The command runs, emits bytes, but always in the single hardcoded mode (heap slowly leaking). The guide is misleading — it implies mode selection works when it does not.

---

### Bug 12 — Missing Required Files (Severity: LOW–MEDIUM)

| File | Required By | Status |
|------|-------------|--------|
| `docs/wire_format_spec.md` | `PRD/roles/ryn_role_assignment.md:66` | Does not exist |
| `agent/hal/esp32/` (directory) | `PRD/PRD.md` (ESP32-P4 is a v1.0 platform) | Does not exist |
| `examples/blinky_twin/` | `PRD/TASK_QUEUE.md` Task 23 | Does not exist |
| `examples/sensor_system/` | `PRD/TASK_QUEUE.md` Task 23 | Does not exist |
| `semantic-conventions/rtos_metrics.md` | Exists but is a basic table, not in OTel proposal format |

---

## 4. Component Status — Honest Assessment

### C Agent (STM32 path only)

| Sub-component | Compiles? | Logic Correct? | Notes |
|---------------|-----------|----------------|-------|
| `wire_format.h` | Yes | Yes | — |
| `snapshot.h` | Yes | Yes (struct) | `sequence_num` field is declared but never written |
| `snapshot.c` | Yes | Mostly | `sequence_num` never set; `vApplicationIdleHook` duplicated with `hooks.c` |
| `encoder.h/c` | Yes | Yes | `heap_min_ever` and `task_runtime` are captured in snapshot but silently dropped in delta encoding |
| `framer.h/c` | Yes | Yes (logic) | Signature contradicts `TECH_SPEC.md` on `seq_num` ownership |
| `transport.h/c` | Yes | Yes | — |
| `profiler.h/c` | Yes | Yes | — |
| `hal/stm32/dwt.h/c` | Yes | Yes | — |
| `hal/stm32/uart_dma.h/c` | Yes | Yes | — |
| `freertos/hooks.c` | Yes | Yes (logic) | `vApplicationIdleHook` duplicated with `snapshot.c` |
| `main.c` | **No** | No | `frame_packet` called with 4 of 7 required arguments |
| **ESP32-P4 HAL** | **N/A** | **N/A** | **Does not exist** |

**No hardware measurements have been taken.** All performance targets (WCET < 150µs, CPU overhead < 2%, RAM < 10 KB) are targets only — none are verified.

### Python Bridge

| Sub-component | Valid Python? | Runnable? | Notes |
|---------------|--------------|-----------|-------|
| `decoder.py` | Yes | Yes | Solid state machine, CRC, sequence gap detection |
| `mock_device.py` | Yes | Yes | Single mode only; extra CLI args ignored silently |
| `config.py` | **No** | **No** | `/**` line 1 → SyntaxError |
| `device_registry.py` | **No** | **No** | `/**` line 1 → SyntaxError |
| `main.py` | **No** | **No** | `/**` line 1 → SyntaxError |
| `state_manager.py` | **No** | **No** | `/**` line 1 + `AttributeError` on every `update()` call |
| `prometheus_exporter.py` | **No** | **No** | `/**` line 1 + 5 of 8 metric names wrong or missing |
| `otlp_exporter.py` | **No** | **No** | `/**` line 1 + only 1 of 8 metrics + callback not registered |
| `oom_analyzer.py` | **No** | **No** | `/**` line 1 + API signature mismatch + no rolling min detector |
| `tests/test_decoder.py` | Yes | Yes | 6 tests, all runnable |
| `tests/conftest.py` | Yes | Yes | Fixture correct |
| `tests/test_oom_analyzer.py` | Yes (file) | **No** | Cannot run — import of `oom_analyzer` fails due to `/**` |

### Infrastructure

| Component | Status |
|-----------|--------|
| `docker-compose.yml` | Correct |
| `prometheus/prometheus.yml` | Correct — scrapes `host.docker.internal:8000` |
| Grafana datasource UID | Correct — aligned between `datasource.yml` and `dashboard.json` |
| Dashboard auto-provision | **Broken** — JSON not in mounted provisioning path |
| CI pipeline | **Broken** — wrong working directory, missing jobs |

---

## 5. Task Completion — Gate-Based Assessment

A task is "done" only when its verification gate from `PRD/TASK_QUEUE.md` passes, not merely when the file exists.

| # | Task | Gate | Gate Status |
|---|------|------|-------------|
| 1 | `wire_format.h` | All constants present, compiles | Gate likely passes |
| 2 | `snapshot.h` structs | Compiles with `-std=c99 -Wall -Wextra -Werror` | Gate likely passes |
| 3 | `framer.h/c` + CRC | `crc16_ccitt("123456789", 9) == 0x29B1` | CRC gate passes; signature contradicts spec |
| 4 | CRC + framer tests | All Unity tests pass | Tests exist; no confirmed run |
| 5 | `decoder.py` CRC | `crc16_ccitt(b"123456789") == 0x29B1` | Passes — verified by inspection |
| 6 | Decoder Python tests | `pytest` all green | `test_decoder.py` is runnable; 6 tests |
| 7 | `mock_device.py` | `--mode normal/leak/saturated` | **Fails** — no `--mode` implemented |
| 8 | DWT Profiler | Compiles, DWT increments on hardware | No hardware test run |
| 9 | `snapshot.c` | WCET < 150µs measured on NUCLEO-F401RE | **No hardware measurement** |
| 10 | FreeRTOS hooks | Idle hook counter increments in snapshot | **Duplicate definition** — won't link |
| 11 | Delta encoder | Compression > 10x measured | No measurement; `heap_min`/`runtime` not encoded |
| 12 | DMA transport | < 0.1% packet loss over 10K packets | No hardware test |
| 13 | `main.c` telemetry task | End-to-end: MCU sends → Python decodes | **Will not compile** — wrong `frame_packet` call |
| 14 | `state_manager.py` | Apply keyframe then delta, verify state | **Cannot run** — file is broken Python + payload parse missing |
| 15 | `prometheus_exporter.py` | `curl localhost:8000/metrics` returns all 8 spec metrics | **Cannot run** — broken Python; only 3/8 metrics correct |
| 16 | `otlp_exporter.py` | Metrics appear in OTel collector | **Cannot run** — broken Python; only 1/8 metrics |
| 17 | `oom_analyzer.py` | All 4 OOM pytest gates pass | **Cannot run** — broken Python; no rolling min detector |
| 18 | OOM tests | `pytest` all green | **Cannot run** — `oom_analyzer.py` import fails |
| 19 | `bridge/main.py` | `python bridge/main.py --port COM3` connects and decodes | **Cannot run** — broken Python |
| 20 | Grafana dashboard | 6 panels render with data | Only 2 panels; not in provisioning path |
| 21 | `docker-compose.yml` | `docker-compose up` → Grafana at localhost:3000 with preloaded dashboard | Grafana loads; dashboard not auto-loaded |
| 22 | GitHub Actions CI | Pipeline passes on push | **Fails** — wrong `cd digitaltwin-main` |
| 23 | Example apps | Both compile; `blinky_twin` sends live packets | **Does not exist** |
| 24 | README + quick_start | Engineer can reach live dashboard in 30 min | README exists; quick_start has `--mode` inconsistency |
| 25 | OTel semantic conventions | Follows OTel format, ready for PR | Exists but too basic for formal submission |

**Gates passing with confidence:** Tasks 1, 2, 5, 6
**Gates likely passing (no confirmed run):** Tasks 3, 4, 8
**Gates failing:** Tasks 7, 10, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23
**Gates not testable yet (hardware required):** Tasks 9, 11, 12

---

## 6. What Is Genuinely Working Right Now

The following can be run or used without modification:

1. **`bridge/decoder.py`** — A working Python packet decoder. Feed it bytes, get `DecodedPacket` objects back. CRC validation, sync detection, sequence gap tracking all functional.
2. **`bridge/mock_device.py`** — Generates valid framed binary packets at 10 Hz. Can be piped to test the decoder. Runs correctly (ignores extra CLI args silently).
3. **`bridge/tests/test_decoder.py`** — 6 runnable pytest tests against the decoder. These pass.
4. **`docker-compose.yml`** — Starts Prometheus and Grafana correctly. Prometheus scrapes the correct host target.
5. **C agent header files** — `wire_format.h`, `snapshot.h`, `encoder.h`, `framer.h`, `transport.h`, `profiler.h` — all compile cleanly and represent the correct interface contracts (modulo the `frame_packet` / `TECH_SPEC` disagreement on `seq_num`).
6. **`agent/core/encoder.c`** — Delta encoding logic is correct and compiles. Keyframe every 50 packets works.
7. **`agent/core/framer.c`** — CRC-16-CCITT implementation is correct. Standard test vector passes.
8. **`agent/hal/stm32/dwt.c`**, **`agent/hal/stm32/uart_dma.c`** — Compile and represent correct STM32 HAL patterns.

---

## 7. What Must Be Fixed to Reach End-to-End Runnable

Ordered by impact on unblocking the pipeline:

### Priority 1 — Fix Python Syntax (Unblocks Everything)
Replace `/** ... */` with `"""..."""` on line 1 of all seven affected bridge files. One-line change per file. This is the single fix with the most unblocking effect.

### Priority 2 — Fix `main.c` `frame_packet` Call
Resolve the 4-layer contract drift first (decide: does framer own `seq_num` internally per TECH_SPEC, or does the caller pass it per ARCHITECTURE.md and framer.h?). Then update `main.c` to pass the correct arguments and update `framer.h` or `TECH_SPEC.md` accordingly.

### Priority 3 — Fix the Duplicate `vApplicationIdleHook`
Decide which CPU measurement approach is canonical. Remove the duplicate from either `snapshot.c` or `hooks.c`. Wire the surviving counter correctly into `snapshot_capture()`.

### Priority 4 — Write the Payload Parser
Add a function (in `decoder.py` or `state_manager.py`) that parses `DecodedPacket.payload: bytes` using the tag-byte protocol from `encoder.c` (tags `0x01`–`0x12`, `0xFF` end marker). This is what converts raw bytes into `heap_free_bytes`, `task` states, etc. Without this, `StateManager.update()` cannot work.

### Priority 5 — Fix `state_manager.py` Field Names
Rename `packet.timestamp_ticks` → `packet.timestamp_ms` to match the `decoder.py` field name, once Priority 1 (syntax) and Priority 4 (payload parser) are resolved.

### Priority 6 — Fix Prometheus Metric Names
Rename metrics in `prometheus_exporter.py` to match `TECH_SPEC.md:237–240`. Add the three missing metrics: `rtos_task_cpu_ratio`, `rtos_heap_oom_projection_seconds`, `rtos_telemetry_packet_loss_ratio`.

### Priority 7 — Fix `OOMAnalyzer` API and Add Rolling Minimum
Update `oom_analyzer.py` to match the `TECH_SPEC.md:214` signature. Add the rolling minimum detector as the second detection strategy.

### Priority 8 — Fix CI Pipeline
Remove `cd digitaltwin-main`. Add Unity C test runner job. Add `arm-none-eabi-size` size check. Add Python version matrix (3.9, 3.11).

### Priority 9 — Move Dashboard to Provisioning Path
Copy `dashboard/rtostwin_dashboard.json` into `grafana/provisioning/dashboards/`. Dashboard will then auto-load on `docker-compose up`.

### Priority 10 — Complete the Dashboard
Add the 4 missing panels: task state table, stack watermark bar chart, OOM countdown stat panel, packet loss time series.

### Priority 11 — Create Example Apps
Create `examples/blinky_twin/main.c` and `examples/sensor_system/main.c`.

### Priority 12 — Create Missing Deliverables
- `docs/wire_format_spec.md`
- `agent/hal/esp32/` (ESP32-P4 HAL)
- Expand `semantic-conventions/rtos_metrics.md` to full OTel proposal format

---

## 8. Corrected Completion Metrics

| Component | Files Exist | Valid / Compiles | End-to-End Runnable |
|-----------|-------------|-----------------|---------------------|
| C Agent (STM32) | ~95% | ~80% (main.c won't compile) | 0% (no hardware, linker issues) |
| Python Bridge | ~90% | ~25% (decoder + mock only) | ~10% (decoder pipeline only) |
| Prometheus Pipeline | Partial | 0% (syntax-broken) | 0% |
| OTLP Pipeline | Partial | 0% (syntax-broken) | 0% |
| OOM Detection | Partial | 0% (syntax-broken) | 0% |
| Grafana Dashboard | Partial | Valid JSON | Manual import only, 2/6 panels |
| CI Pipeline | Exists | Runs | Fails (wrong path) |
| Example Apps | 0% | — | — |
| **End-to-End Demo** | — | — | **Not achievable in current state** |

---

## 9. What Is Genuinely Strong in This Project

These are design and specification quality observations, stated as such:

- The binary wire protocol design (sync bytes, CRC-16-CCITT, delta encoding, keyframe every N packets) is architecturally sound and correctly specified in `TECH_SPEC.md`.
- The C agent's zero-malloc constraint is correctly enforced in all implemented source files — `static` buffers throughout, no `pvPortMalloc` in the hot path.
- The choice of OTLP + Prometheus + Grafana as the target stack is well-reasoned: it integrates with infrastructure engineering teams already operate.
- The `PRD/` directory represents an unusually complete specification for a student project: wire format, function signatures, algorithm specifications, and verification gates are all written down before implementation.
- The 25-task queue with explicit gates is the correct way to drive AI-assisted development on a project of this complexity.
- `bridge/decoder.py` is the closest thing to production-quality code in the Python bridge — clean state machine, correct CRC, correct gap detection.

---

*Log compiled from three rounds of verified analysis. All file references are to the current repository state. No performance numbers are stated as fact — they are targets until measured on real hardware.*
