# RTOSTwin — Task Queue (Ordered Implementation Tasks)

> Feed ONE task at a time to the AI. Always attach PRD.md + ARCHITECTURE.md + TECH_SPEC.md + FILE_STRUCTURE.md + CODING_RULES.md as context alongside the task.

---

## How To Use This File

1. Start at Task 1. Do not skip ahead.
2. For each task, give the AI: **this task description + all 5 spec files + any source files listed in "Context Files"**.
3. After the AI generates code, **verify the gate** before moving to the next task.
4. If a gate fails, fix it before proceeding. Dependencies are strict.

---

## Phase 0 — Foundation (No Hardware Needed)

### Task 1: Create `wire_format.h`
**Prompt:** "Create `agent/core/wire_format.h` containing all protocol constants as defined in TECH_SPEC.md Section 2.2. Include header guard. No functions, no structs — only `#define` constants."

**Context Files:** `TECH_SPEC.md`
**Output:** `agent/core/wire_format.h`
**Gate:** File contains all 13 constants from TECH_SPEC Section 2.2. Compiles with zero warnings.

---

### Task 2: Create Snapshot Structs
**Prompt:** "Create `agent/core/snapshot.h` with the exact struct definitions from TECH_SPEC.md Section 1.1. Include header guard, include `stdint.h` and `stdbool.h`. Add function prototypes for `snapshot_init()` and `snapshot_capture()`. Add Doxygen comments on every struct field and function."

**Context Files:** `TECH_SPEC.md`, `CODING_RULES.md`, `wire_format.h`
**Output:** `agent/core/snapshot.h`
**Gate:** Compiles with `-std=c99 -Wall -Wextra -Werror` (as a header inclusion test).

---

### Task 3: Implement CRC-16-CCITT (C)
**Prompt:** "Create `agent/core/framer.h` and `agent/core/framer.c` in separate chunks (one file per chunk). Implement `crc16_ccitt()` exactly as specified in TECH_SPEC.md Section 2.3. Also implement `frame_packet()` as specified in TECH_SPEC.md Section 3.3, including `packet_type` and `timestamp_ticks` inputs. All buffers are static. No malloc."

**Context Files:** `TECH_SPEC.md`, `CODING_RULES.md`, `wire_format.h`, `snapshot.h`
**Output:** `agent/core/framer.h`, `agent/core/framer.c`
**Gate:** CRC test vector passes: `crc16_ccitt("123456789", 9) == 0x29B1`.

---

### Task 4: CRC + Framer Unit Tests (C)
**Prompt:** "Create `agent/tests/test_crc.c` and `agent/tests/test_framer.c` using the Unity test framework in separate chunks (one file per chunk). Test `crc16_ccitt()` with the standard test vector (b'123456789' -> 0x29B1), with empty input, and with a single byte. Test `frame_packet()` with a known payload and verify all header fields and CRC in the output buffer, including TYPE and TIMESTAMP fields."

**Context Files:** `TECH_SPEC.md`, `CODING_RULES.md`, `framer.h`, `framer.c`, `wire_format.h`
**Output:** `agent/tests/test_crc.c`, `agent/tests/test_framer.c`
**Gate:** All tests pass with Unity test runner.

---

### Task 5: Implement CRC-16-CCITT (Python)
**Prompt:** "Create `bridge/decoder.py` with the `PacketDecoder` class as specified in TECH_SPEC.md Section 4.1. Implement `crc16_ccitt()` in Python as specified in Section 2.3. Implement the byte-by-byte state machine decoder with `feed_byte()` and `feed_bytes()`. Include `drop_count` and `sequence_gap_count` properties. Use dataclasses for `DecodedPacket` and `TaskSnapshot`. Full type hints on everything."

**Context Files:** `TECH_SPEC.md`, `CODING_RULES.md`, `wire_format.h` (for constants)
**Output:** `bridge/decoder.py`
**Gate:** `crc16_ccitt(b"123456789") == 0x29B1` — must match C output exactly.

---

### Task 6: Python Decoder Unit Tests
**Prompt:** "Create `bridge/tests/test_decoder.py` with pytest. Test: (1) CRC test vector matches C output, (2) Decode a manually constructed known-good packet, (3) CRC fail → packet discarded + drop_count incremented, (4) Sequence gap (seq 5 → seq 7) → sequence_gap_count incremented, (5) Garbage bytes before valid packet → decoder re-syncs. Create `bridge/tests/conftest.py` with fixtures for known-good packet bytes."

**Context Files:** `TECH_SPEC.md`, `decoder.py`, `wire_format.h`
**Output:** `bridge/tests/test_decoder.py`, `bridge/tests/conftest.py`
**Gate:** `pytest bridge/tests/test_decoder.py` all green.

---

### Task 7: Mock Device Generator
**Prompt:** "Create `bridge/mock_device.py` that generates valid RTOSTwin binary packets. It must follow the wire_format.h protocol exactly. Support three modes: `--mode normal` (steady state 4-task system), `--mode leak` (heap decreases by 10 bytes/sec), `--mode saturated` (all tasks running, 95% CPU). Output to stdout at 10 Hz. Each packet must have valid CRC and incrementing sequence numbers. Import and use the CRC function from decoder.py."

**Context Files:** `TECH_SPEC.md`, `decoder.py`, `wire_format.h`
**Output:** `bridge/mock_device.py`
**Gate:** `python mock_device.py --mode normal | python -c "import sys; print(len(sys.stdin.buffer.read(1000)))"` produces bytes. Decoder successfully parses mock output: `python mock_device.py --mode normal | python -c "from bridge.decoder import PacketDecoder; ..."`.

---

## Phase 1 — Agent Core (Requires STM32 Hardware)

### Task 8: DWT Profiler
**Prompt:** "Create `agent/core/profiler.h` and `agent/core/profiler.c` as specified in TECH_SPEC.md Section 3.5. Create `agent/hal/stm32/dwt.h` and `agent/hal/stm32/dwt.c` to enable `DWT->CYCCNT` on STM32. Handle 32-bit wrap-around correctly using unsigned subtraction. `profiler_report()` should printf to UART: `[PROFILER] label: min=X max=Y mean=Z cycles | min=Aµs max=Bµs mean=Cµs @ 168MHz`."

**Context Files:** `TECH_SPEC.md`, `CODING_RULES.md`, `snapshot.h`
**Output:** `agent/core/profiler.h`, `agent/core/profiler.c`, `agent/hal/stm32/dwt.h`, `agent/hal/stm32/dwt.c`
**Gate:** Compiles with arm-none-eabi-gcc. DWT counter increments on real hardware.

---

### Task 9: Snapshot Engine Implementation
**Prompt:** "Implement `agent/core/snapshot.c`. Use `static TaskStatus_t s_task_status_buf[MAX_TASKS]` at file scope — NOT inside the function, NOT on the stack, NOT with malloc. Use `taskENTER_CRITICAL()` around the `uxTaskGetSystemState()` call. Fill the output `full_snapshot_t` from `TaskStatus_t` array. Compute CPU utilization from idle hook counters (volatile static counters). Increment sequence_num on every call. Fill timestamp with `xTaskGetTickCount()`."

**Context Files:** `TECH_SPEC.md`, `CODING_RULES.md`, `snapshot.h`, `profiler.h`
**Output:** `agent/core/snapshot.c`
**Gate:** Compiles. Prints snapshot struct over UART on real STM32. WCET < 150µs verified by profiler.

---

### Task 10: FreeRTOS Hooks
**Prompt:** "Create `agent/freertos/hooks.c`. Implement `vApplicationIdleHook()` that increments a volatile uint32_t cycle counter. Implement `vApplicationStackOverflowHook()` that writes an error to a static buffer and halts. Provide `configUSE_IDLE_HOOK=1` in the FreeRTOSConfig.h guidance comments."

**Context Files:** `TECH_SPEC.md`, `CODING_RULES.md`, `snapshot.h`
**Output:** `agent/freertos/hooks.c`
**Gate:** Idle hook counter increments observed in snapshot output.

---

### Task 11: Delta Encoder
**Prompt:** "Implement `agent/core/encoder.h` and `agent/core/encoder.c` as specified in TECH_SPEC.md Sections 2.4 and 3.2. Maintain `static full_snapshot_t s_last_snapshot`. Compare field-by-field. Emit tag byte + new value for changed fields only. Force keyframe when `force_keyframe` is true. After encoding, copy current snapshot to `s_last_snapshot`."

**Context Files:** `TECH_SPEC.md`, `CODING_RULES.md`, `snapshot.h`, `wire_format.h`
**Output:** `agent/core/encoder.h`, `agent/core/encoder.c`
**Gate:** Keyframe produces full payload. Second call with identical data produces near-zero payload. Unit test verifies compression > 10x.

---

### Task 12: DMA Transport
**Prompt:** "Implement `agent/core/transport.h`, `agent/core/transport.c`, and `agent/hal/stm32/uart_dma.c`. Use `HAL_UART_Transmit_DMA(&huart2, buf, len)`. Check `HAL_UART_GetState()` before sending — if `HAL_UART_STATE_BUSY_TX`, return -1 and increment `static volatile uint32_t s_tx_drop_count`. Never block. `transport_init()` configures USART2 + DMA."

**Context Files:** `TECH_SPEC.md`, `CODING_RULES.md`, `wire_format.h`, `framer.h`
**Output:** `agent/core/transport.h`, `agent/core/transport.c`, `agent/hal/stm32/uart_dma.c`
**Gate:** Packets received by Python serial listener on host. < 0.1% packet loss over 10K packets.

---

### Task 13: Main Telemetry Task
**Prompt:** "Create `agent/main.c` with a FreeRTOS application. Create a telemetry task at `tskIDLE_PRIORITY + 1`. In the task loop: call `snapshot_capture()`, `encoder_encode()`, `frame_packet(payload, payload_len, packet_type, timestamp_ticks, ...)`, `transport_send()` at 10 Hz using `vTaskDelay(pdMS_TO_TICKS(100))`. Wrap `snapshot_capture()` with profiler calls. Report profiler stats every 100 iterations."

**Context Files:** `TECH_SPEC.md`, `CODING_RULES.md`, `snapshot.h`, `encoder.h`, `framer.h`, `transport.h`, `profiler.h`
**Output:** `agent/main.c`
**Gate:** Full end-to-end: MCU sends packets → Python decoder on host prints decoded task states.

---

## Phase 2 — Python Bridge Complete

### Task 14: State Manager
**Prompt:** "Create `bridge/state_manager.py` and `bridge/device_registry.py`. `StateManager` takes `DecodedPacket` and reconstructs full `DeviceState` by applying deltas to a stored keyframe. `DeviceRegistry` maps `device_id` → `DeviceState`. Support multiple simultaneous devices."

**Context Files:** `TECH_SPEC.md`, `CODING_RULES.md`, `decoder.py`
**Output:** `bridge/state_manager.py`, `bridge/device_registry.py`
**Gate:** Unit test: apply keyframe, then delta, verify reconstructed state matches expected.

---

### Task 15: Prometheus Exporter
**Prompt:** "Create `bridge/prometheus_exporter.py` using `prometheus_client`. Expose all 8 metrics listed in TECH_SPEC.md Section 4.3. HTTP server on port 8000. Labels: `device_id`, `task_name` (for per-task metrics), `state` (for task_state). Call `update_metrics()` on every decoded packet."

**Context Files:** `TECH_SPEC.md`, `CODING_RULES.md`, `state_manager.py`
**Output:** `bridge/prometheus_exporter.py`
**Gate:** `curl http://localhost:8000/metrics` returns valid Prometheus exposition format with all 8 metric names.

---

### Task 16: OTLP Exporter
**Prompt:** "Create `bridge/otlp_exporter.py` using `opentelemetry-sdk` and `opentelemetry-exporter-otlp-proto-http`. Register MeterProvider with PeriodicExportingMetricReader. Export all 8 metrics from TECH_SPEC.md Section 4.4 (dot-separated OTel names). OTLP endpoint configurable via `OTEL_EXPORTER_OTLP_ENDPOINT` env variable."

**Context Files:** `TECH_SPEC.md`, `CODING_RULES.md`, `state_manager.py`
**Output:** `bridge/otlp_exporter.py`
**Gate:** Metrics appear in a local OTel Collector.

---

### Task 17: OOM Analyzer
**Prompt:** "Create `bridge/oom_analyzer.py` as specified in TECH_SPEC.md Section 4.2. Implement both detectors: linear regression (`scipy.stats.linregress`) and rolling minimum comparison. Window size 600, R² threshold 0.7, rolling min threshold 10%. Return -1.0 when stable. Return projected seconds when leak detected."

**Context Files:** `TECH_SPEC.md`, `CODING_RULES.md`
**Output:** `bridge/oom_analyzer.py`
**Gate:** All 4 OOM pytest gates pass (monotonic, sawtooth, stable, insufficient data).

---

### Task 18: OOM Analyzer Tests
**Prompt:** "Create `bridge/tests/test_oom_analyzer.py`. Test: (1) Monotonic leak at 1 byte/sec over 600 samples → detected within 60s, (2) Sawtooth pattern with 5% net decrease → rolling min alert fires, (3) Stable 3600 samples → zero alerts, (4) Under 30 samples → returns -1.0."

**Context Files:** `TECH_SPEC.md`, `oom_analyzer.py`
**Output:** `bridge/tests/test_oom_analyzer.py`
**Gate:** `pytest bridge/tests/test_oom_analyzer.py` all green.

---

### Task 19: Bridge Main Entry Point
**Prompt:** "Create `bridge/main.py` and `bridge/config.py`. Main reads from serial port (configurable), feeds bytes to decoder, updates state manager, pushes to prometheus and otlp exporters, feeds heap data to OOM analyzer. config.py has all defaults: serial port, baud rate, prometheus port, OTLP endpoint, OOM thresholds. Use argparse for CLI overrides."

**Context Files:** `ARCHITECTURE.md`, `CODING_RULES.md`, `decoder.py`, `state_manager.py`, `prometheus_exporter.py`, `otlp_exporter.py`, `oom_analyzer.py`
**Output:** `bridge/main.py`, `bridge/config.py`, `bridge/requirements.txt`
**Gate:** `python bridge/main.py --serial COM3` connects and prints decoded packets.

---

## Phase 3 — Dashboard + Testing + Docs

### Task 20: Grafana Dashboard
**Prompt:** "Create `dashboard/rtostwin_dashboard.json`. A Grafana 10+ dashboard with 6 panels: (1) Task State table, (2) Stack Watermark bar chart, (3) Heap Free time series, (4) OOM Countdown stat panel, (5) CPU Utilization gauge, (6) Packet Loss time series. Data source: Prometheus. Use the exact metric names from TECH_SPEC.md."

**Context Files:** `TECH_SPEC.md`
**Output:** `dashboard/rtostwin_dashboard.json`
**Gate:** Imports into Grafana, all panels render with data from Prometheus.

---

### Task 21: Docker Compose
**Prompt:** "Create `docker-compose.yml` with Prometheus and Grafana containers. Prometheus pre-configured to scrape `host.docker.internal:8000/metrics` every 15s. Grafana pre-loaded with the RTOSTwin dashboard via provisioning."

**Context Files:** `ARCHITECTURE.md`
**Output:** `docker-compose.yml`
**Gate:** `docker-compose up` → Grafana accessible at localhost:3000 with pre-loaded dashboard.

---

### Task 22: GitHub Actions CI
**Prompt:** "Create `.github/workflows/ci.yml`. Jobs: (1) Build agent with arm-none-eabi-gcc, (2) Run C Unity tests, (3) Check agent .data+.bss < 10KB via arm-none-eabi-size, (4) Run pytest for bridge, (5) Lint Python with mypy. Use matrix for Python 3.9 and 3.11."

**Context Files:** `FILE_STRUCTURE.md`, `CODING_RULES.md`
**Output:** `.github/workflows/ci.yml`
**Gate:** Pipeline passes on push to main.

---

### Task 23: Example Apps
**Prompt:** "Create `examples/blinky_twin/main.c` — minimal 1-task FreeRTOS app with telemetry agent integrated. Create `examples/sensor_system/main.c` — multi-task realistic app with sensor_task, processing_task, comms_task + telemetry agent. Both must compile and run on STM32F4 Nucleo."

**Context Files:** `TECH_SPEC.md`, `CODING_RULES.md`, all `agent/core/*.h` files
**Output:** `examples/blinky_twin/`, `examples/sensor_system/`
**Gate:** Both compile. blinky_twin sends live packets to bridge.

---

### Task 24: README + Quick Start
**Prompt:** "Create `README.md` with: project description, architecture diagram (ASCII), features, quick-start (git clone → Grafana in 30 min), measured benchmarks table (placeholder until real data), license (MIT), contributing guidelines. Create `docs/quick_start.md` with step-by-step instructions including hardware setup, flashing, bridge setup, docker-compose, and Grafana import."

**Context Files:** `PRD.md`, `ARCHITECTURE.md`
**Output:** `README.md`, `docs/quick_start.md`
**Gate:** A new engineer can follow quick_start.md from zero to dashboard.

---

### Task 25: OTel Semantic Conventions Proposal
**Prompt:** "Create `semantic-conventions/rtos_metrics.md` following the OpenTelemetry semantic conventions format. Define all 8 RTOS metric names with: metric name, instrument type (Gauge), unit, description, required attributes (device_id, task_name, state), examples. Reference existing OTel conventions for naming patterns."

**Context Files:** `TECH_SPEC.md`, `PRD.md`
**Output:** `semantic-conventions/rtos_metrics.md`
**Gate:** Document follows OTel semantic conventions style. Ready for GitHub PR to `open-telemetry/semantic-conventions`.

---

## Summary — Task Dependencies

```
Task 1 (wire_format.h)
  ├── Task 2 (snapshot.h)
  │     └── Task 9 (snapshot.c) → Task 13 (main.c)
  ├── Task 3 (framer.c) → Task 4 (framer tests)
  │     └── Task 12 (transport.c) → Task 13 (main.c)
  ├── Task 5 (decoder.py) → Task 6 (decoder tests)
  │     └── Task 7 (mock_device.py)
  │           └── Task 14 (state_manager) → Task 15 (prometheus)
  │                                       → Task 16 (otlp)
  │                                       → Task 19 (bridge main)
  ├── Task 8 (profiler) → Task 9 (snapshot.c)
  ├── Task 11 (encoder.c) → Task 13 (main.c)
  └── Task 17 (oom_analyzer) → Task 18 (oom tests) → Task 19 (bridge main)

Independent: Task 20 (dashboard), Task 21 (docker), Task 22 (CI), Task 23 (examples), Task 24 (README), Task 25 (OTel proposal)
```
