# RTOSTwin Project — Engineer Role Assignment

---

## 👤 Engineer: VNV
**Role Title:** Embedded Systems Engineer — Transport, Encoding & Observability  
**Project:** RTOSTwin — RTOS Telemetry Agent & OpenTelemetry Bridge  
**Assigned By:** Engineering Lead  
**Date:** March 2026  
**Project Version Target:** v1.0

---

> You own the path data takes from the moment it is encoded on the MCU, all the way through the wire, and out to Grafana. Without your work, the data captured by the snapshot engine never reaches anyone who needs it.

---

## 🏗️ Your Position in the System

```
[MCU - FreeRTOS]
      │
      ├── snapshot_capture()     ← (Partner's domain)
      │
      ▼
  ┌─────────────────────────┐
  │   PACKET FRAMER + CRC   │  ← YOU OWN THIS (C firmware side)
  │   DELTA ENCODER         │  ← YOU OWN THIS
  │   DMA UART TRANSPORT    │  ← YOU OWN THIS
  └──────────┬──────────────┘
             │ binary stream over UART / USB CDC / UDP
             ▼
  ┌─────────────────────────┐
  │   PROMETHEUS ENDPOINT   │  ← YOU OWN THIS (Python bridge side)
  │   OTLP/HTTP EXPORTER    │  ← YOU OWN THIS
  │   MULTI-DEVICE SUPPORT  │  ← YOU OWN THIS
  │   GRAFANA DASHBOARD     │  ← YOU OWN THIS
  └─────────────────────────┘
             │
             ▼
       [ Grafana Dashboard ]
```

---

## 🚨 Read This First — Current Project Baseline

- **Canonical implementation root:** repository root `d:\digital_twin\`
- **Do not split new work across trees:** the top-level `agent/` folder is partial reference code, not the canonical delivery path.
- **V1 hardware rollout order:** `NUCLEO-F401RE` first, `ESP32-P4-Function-EV-Board` second, `Teensy 4.1` third.
- **Transport rollout order:** get the STM32 UART baseline working first; then add USB CDC / UDP-style backends needed for `ESP32-P4` and `Teensy 4.1`.

## ✅ Your Exact Work Order (Start Here)

1. **Work in one tree only:** all new implementation goes under the repository root (`d:\digital_twin\`) folders such as `agent/`, `bridge/`, `dashboard/`, `docs/`, `grafana/`, and `prometheus/`.
2. **Follow RYN's protocol freeze for logic, but do not wait to clean infrastructure:** you can immediately fix syntax/scaffolding issues in your owned Python/YAML files, but packet/framing semantics must follow the frozen `docs/wire_format_spec.md` and `agent/core/wire_format.h`.
3. **Finish the MCU data path on the baseline board:** `agent/core/framer.*`, `agent/core/encoder.*`, `agent/core/transport.*`, and `agent/hal/stm32/uart_dma.c` must work together first on `NUCLEO-F401RE`.
4. **Own the bridge assembly path:** `bridge/state_manager.py`, `bridge/device_registry.py`, `bridge/main.py`, `bridge/prometheus_exporter.py`, `bridge/otlp_exporter.py`, and `bridge/mock_device.py` are your path from decoded packets to observability outputs.
5. **Keep one canonical mock stream for both engineers:** finish `bridge/mock_device.py` with `normal`, `leak`, and `saturated` modes and keep it aligned with the frozen wire format.
6. **Finish observability only after state flow is correct:** first state reconstruction, then Prometheus/OTLP, then dashboard, then Docker/Prometheus/Grafana wiring.
7. **After the STM32 baseline is stable, generalize transports for the other demo boards:** `ESP32-P4` and `Teensy 4.1` should reuse the same packet contract and bridge labels.

---

## 🤝 Integration Contract (Do Not Break)

1. **Single-owner rule (no overlap):**
      - VNV owns: `agent/core/framer.*`, `agent/core/encoder.*`, `agent/core/transport.*`, `agent/hal/stm32/uart_dma.c`, `bridge/state_manager.py`, `bridge/prometheus_exporter.py`, `bridge/otlp_exporter.py`, `bridge/device_registry.py`, `bridge/main.py`, `bridge/mock_device.py`, `dashboard/rtostwin_dashboard.json`
      - RYN owns: `agent/core/snapshot.*`, `agent/core/profiler.*`, `docs/wire_format_spec.md`, `agent/core/wire_format.h`, `bridge/decoder.py`, `bridge/oom_analyzer.py`
2. **Decoder ownership is fixed:** do not implement a second decoder/parser inside exporter, registry, or state-manager modules; consume `DecodedPacket` from `bridge/decoder.py`.
3. **State reconstruction ownership is fixed:** `bridge/state_manager.py` belongs to you, but it must reconstruct state from RYN's typed decoder output rather than from raw payload bytes.
4. **Protocol freeze gate:** `docs/wire_format_spec.md` + `agent/core/wire_format.h` must be frozen as `v1` by end of Week 3 and approved by both engineers.
5. **No breaking protocol change on main:** if packet layout, enum values, field sizes, CRC settings, or delta tags change, bump `WF_PROTOCOL_VERSION` and add backward-compat notes before merge.
6. **Merge gate (required):** every merge touching protocol/framing/decoder must pass:
      - 3 golden packet vectors (byte-for-byte exact)
      - Decoder compatibility test for current protocol version
      - End-to-end mock stream test (`mock_device.py` -> `decoder.py`)

---

## 📋 Deliverables — C Firmware Side (agent/)

These files run **on the microcontroller**. Written in C99. V1 rollout is `NUCLEO-F401RE` first, then `ESP32-P4`, then `Teensy 4.1`.

---

### Deliverable 1 — Packet Framer + CRC-16-CCITT
**Files:** `agent/core/framer.h`, `agent/core/framer.c`

**What it does:**  
Takes a raw snapshot payload (produced by your partner's `snapshot_capture()`) and wraps it in the agreed binary wire format so the Python bridge can synchronize, decode, and validate it.

**Wire Format (as agreed in wire format spec):**

| Field | Size | Value |
|---|---|---|
| `SYNC_0` | 1 byte | `0xAA` |
| `SYNC_1` | 1 byte | `0x55` |
| `TYPE` | 1 byte | `0x01` = snapshot |
| `SEQ_NUM` | 2 bytes | `uint16_t`, monotonically incrementing |
| `TIMESTAMP` | 4 bytes | `xTaskGetTickCount()` |
| `LENGTH` | 2 bytes | payload byte count |
| `PAYLOAD` | N bytes | encoded snapshot |
| `CRC_16` | 2 bytes | CRC-CCITT over `TYPE` through `PAYLOAD` |

**CRC Algorithm:** CRC-16-CCITT (polynomial `0x1021`, initial value `0xFFFF`)

**Function signatures to implement:**
```c
/* Wrap an encoded payload in a complete framed packet.
 * out_buf must be at least (12 + payload_len) bytes.
 * Returns total packet length in bytes. */
uint16_t frame_packet(const uint8_t *payload, uint16_t payload_len,
                      uint8_t *out_buf, uint16_t out_buf_size);

/* Compute CRC-16-CCITT over data[0..len-1]. */
uint16_t crc16_ccitt(const uint8_t *data, uint16_t len);
```

**Constraints:**
- Zero dynamic allocation. No `malloc`, no `pvPortMalloc`.
- `out_buf` is a static buffer declared at file scope in `transport.c`.
- Test vectors for CRC must be documented in the unit test file.

---

### Deliverable 2 — DMA UART Transport
**Files:** `agent/core/transport.h`, `agent/core/transport.c`, `agent/hal/stm32/uart_dma.c`

**What it does:**  
Sends a framed packet over UART using DMA (Direct Memory Access). The CPU initiates a DMA transfer and immediately returns — the hardware sends every byte without CPU involvement. If a previous DMA transfer is still in progress, the new packet is silently dropped and a drop counter is incremented.

**Key design rules:**
- The telemetry task runs at `tskIDLE_PRIORITY + 1` — it **never preempts** any application task.
- DMA TX uses the STM32 HAL: `HAL_UART_Transmit_DMA(&huart2, buf, len)`.
- Check `HAL_UART_GetState()` before re-initiating — if `HAL_UART_STATE_BUSY_TX`, drop and increment `tx_drop_count`.
- Packet drop rate must be < 0.1% at 10 Hz (verified by bridge via sequence number gaps).

**Function signatures:**
```c
/* Initialize UART + DMA peripheral. Called once at system start. */
void transport_init(void);

/* Attempt to transmit packet over DMA. Non-blocking.
 * Returns 0 on success, -1 if DMA busy (packet dropped). */
int transport_send(const uint8_t *packet, uint16_t len);

/* Returns number of packets dropped since last reset. */
uint32_t transport_get_drop_count(void);
```

**Verification gate:** < 0.1% packet loss over 10,000 packets with 8 FreeRTOS tasks running.

---

### Deliverable 3 — Delta Encoder
**Files:** `agent/core/encoder.h`, `agent/core/encoder.c`

**What it does:**  
Instead of sending a full snapshot every 100ms, the delta encoder compares the current snapshot against the previous one and emits only the bytes that changed. A keyframe (full snapshot) is sent every 50 packets to allow the bridge to resync.

**Why this matters:**  
At 115200 baud, UART throughput is 11,520 bytes/second. A full 8-task snapshot is ~350 bytes. At 10 Hz, that is 3,500 bytes/second (30% bandwidth). With delta encoding, typical usage drops to 80–200 bytes/second (under 2%).

**Design:**
- Maintain `static full_snapshot_t last_snapshot;` at file scope.
- Compare field-by-field with the new snapshot.
- Encode changed fields with a tag-byte identifying the field + new value.
- Every 50 packets (`SEQ_NUM % 50 == 0`), force a keyframe — send full snapshot.
- Bridge knows it is a keyframe from the `TYPE` field (`0x01` = delta, `0x02` = keyframe).

**Function signatures:**
```c
/* Produce a delta-encoded payload from current snapshot.
 * If force_keyframe is true, encode full snapshot.
 * Returns encoded byte count. */
uint16_t encoder_encode(const full_snapshot_t *current,
                        uint8_t *out_buf, uint16_t out_buf_size,
                        bool force_keyframe);

/* Reset encoder state (treat next packet as keyframe). */
void encoder_reset(void);
```

**Verification gate:** > 10x compression ratio on a typical 4-task steady-state workload. Measured and reported in the README.

---

## 📋 Deliverables — Python Bridge Side (bridge/)

These run **on a PC or Raspberry Pi**. Written in Python 3.9+. No embedded hardware required — you can develop entirely with mock packet data while the C side is being built.

---

### Deliverable 4 — Prometheus Exposition HTTP Endpoint
**Files:** `bridge/prometheus_exporter.py`

**What it does:**  
Hosts an HTTP endpoint at `http://localhost:8000/metrics` that Prometheus scrapes every 15 seconds. The endpoint returns all RTOS metrics in the [Prometheus exposition format](https://prometheus.io/docs/instrumenting/exposition_formats/).

**Metric names to implement** (as per OTel semantic conventions proposal):

| Metric | Type | Labels | Description |
|---|---|---|---|
| `rtos_task_state` | Gauge | `device_id`, `task_name`, `state` | 1 if task in this state, 0 otherwise |
| `rtos_task_stack_watermark_bytes` | Gauge | `device_id`, `task_name` | Stack bytes remaining |
| `rtos_task_cpu_ratio` | Gauge | `device_id`, `task_name` | CPU fraction 0.0–1.0 |
| `rtos_heap_free_bytes` | Gauge | `device_id` | Current free heap |
| `rtos_heap_min_ever_bytes` | Gauge | `device_id` | Historical minimum free heap |
| `rtos_cpu_utilization_ratio` | Gauge | `device_id` | Total CPU utilization 0.0–1.0 |
| `rtos_telemetry_packet_loss_ratio` | Gauge | `device_id` | Packet drop fraction |

**Library:** `prometheus_client` (`pip install prometheus-client`)

**Verification gate:** Prometheus scrapes successfully. All metrics appear in Grafana with correct labels.

---

### Deliverable 5 — OTLP/HTTP Exporter
**Files:** `bridge/otlp_exporter.py`

**What it does:**  
Pushes RTOS metrics as typed OTLP metrics to any OpenTelemetry-compatible backend (Grafana Cloud, Datadog, Splunk, self-hosted OTel Collector) on a configurable push interval (default: 30 seconds).

**Library:** `opentelemetry-sdk` + `opentelemetry-exporter-otlp-proto-http`

**Implementation notes:**
- Use `opentelemetry.sdk.metrics.MeterProvider`
- Register `OTLPMetricExporter` with `PeriodicExportingMetricReader`
- One `Meter` per device, metrics labeled with `device_id` attribute
- OTLP endpoint configurable via `OTEL_EXPORTER_OTLP_ENDPOINT` env variable

**Verification gate:** Metrics appear in Grafana Cloud free tier instance without any custom configuration beyond setting the OTLP endpoint.

---

### Deliverable 6 — Multi-Device Support
**Files:** `bridge/state_manager.py`, `bridge/device_registry.py`, updated `bridge/main.py`

**What it does:**  
Allows one bridge instance to handle telemetry from N devices simultaneously. Each device is identified by a unique `device_id` (configurable, default: serial port name or WiFi IP). Metrics from different devices carry different `device_id` labels and **never cross-contaminate**.

**Design:**
- `StateManager` reconstructs the full per-device state from RYN's `DecodedPacket`
- `DeviceRegistry` class: dict of `device_id → DeviceState`
- Each `DeviceState` holds: latest snapshot, packet counter, drop counter, OOM trend data
- Bridge main loop: reads from each serial port in async I/O loop (`asyncio`)

**Verification gate:** Two reference boards connected simultaneously. Grafana shows two separate sets of metrics, each correctly labeled.

---

### Deliverable 7 — Grafana Dashboard Template
**Files:** `dashboard/rtostwin_dashboard.json`

**What it does:**  
A ready-to-import Grafana dashboard JSON that works out-of-the-box with Prometheus as the data source. Any engineer can do: *Grafana → Dashboards → Import → paste JSON → live data*.

**Required panels:**
1. **Task State Timeline** — table showing current state (Running/Ready/Blocked/Suspended) per task
2. **Stack Watermark** — bar chart, one bar per task, showing bytes remaining (alert color below 256 bytes)
3. **Heap Free** — time-series graph showing `rtos_heap_free_bytes` over time
4. **OOM Countdown** — stat panel showing `rtos_heap_oom_projection_seconds` (-1 = stable)
5. **CPU Utilization** — gauge panel (0–100%) per task
6. **Packet Loss** — time-series alert panel, red if > 0.1%

**Verification gate:** Any engineer starting from `git clone` can `docker-compose up` and see a populated Grafana dashboard within 30 minutes.

---

### Deliverable 8 — Shared Mock Device Generator
**File:** `bridge/mock_device.py`

**What it does:**
Generates the canonical binary packet stream for integration testing without hardware. This is the only mock stream implementation used by both engineers for decoder, exporter, and dashboard validation.

**Required modes:**
- `--mode normal`
- `--mode leak`
- `--mode saturated`

**Verification gate:**
- `bridge/mock_device.py` output is parseable by partner-owned `bridge/decoder.py` in all required modes.
- Golden packet vectors generated from this tool match `wire_format.h` and `wire_format_spec.md`.

---

## 🚧 Critical Constraints (Apply to ALL Your Deliverables)

1. **Zero dynamic allocation in C firmware.** `malloc` and `pvPortMalloc` are forbidden inside any function you write in the agent. Use `static` buffers.
2. **Non-blocking transport.** `transport_send()` must never block. If DMA is busy, drop and return.
3. **ISR safety.** Any variable shared between your transport code and an ISR (e.g., the DMA complete callback) must be declared `volatile` and accessed inside a FreeRTOS critical section.
4. **Version pinning.** Your Python code targets Python 3.9+. Lock all dependencies in `requirements.txt` with exact versions.
5. **Test coverage.** Every function you write has at minimum one happy-path and one error-path unit test.

---

## ✅ Definition of Done

Your role is complete when every item in this checklist is checked off:

**C Firmware:**
- [ ] `frame_packet()` produces correct wire format bytes for a known input — verified by unit test
- [ ] `crc16_ccitt()` passes 3 known CRC test vectors
- [ ] `transport_send()` drops packets non-blocking when DMA is busy — verified by mock test
- [ ] `encoder_encode()` produces < 10% of full snapshot size on steady-state workload — measured
- [ ] Keyframe is forced every 50 packets — verified by unit test
- [ ] All C deliverables compile with `-Wall -Wextra -Werror` and zero warnings

**Python Bridge:**
- [ ] Prometheus endpoint returns correct Content-Type and all metric names — verified by pytest
- [ ] OTLP exporter sends metrics without error to a local OTel Collector — integration test
- [ ] Two simultaneous devices produce non-contaminated metrics — integration test
- [ ] Grafana dashboard imports and displays live data from a real device
- [ ] `bridge/mock_device.py` produces parseable streams for `normal`, `leak`, and `saturated` modes

---

## 🔗 Key Dependencies

| What You Need | Who Provides It | When |
|---|---|---|
| `full_snapshot_t` struct definition | Partner (snapshot engine) | Week 3 |
| Wire format byte layout spec | Partner (wire format spec, Week 3) | Week 3 |
| Mock packet data (for Python dev) | You own and maintain `bridge/mock_device.py` | Immediately |
| Prometheus running locally (testing) | You — `docker run prom/prometheus` | Phase 3 start |

> **Start the Python bridge immediately using mock packet data.** You do not need the physical MCU to build and test Deliverables 4–8. Maintain a single canonical `bridge/mock_device.py` used by both engineers to prevent integration drift.

---

## 📅 Timeline

| Milestone | Target Week | Gate |
|---|---|---|
| Packet framer + CRC unit tests pass | Week 4 | Wire format spec received |
| Shared `mock_device.py` ready for both tracks | Week 4 | Partner decoder parses all required modes |
| DMA transport verified on hardware | Week 5 | < 0.1% packet loss |
| Delta encoder verified | Week 7 | > 10x compression measured |
| Prometheus endpoint live | Week 10 | Grafana shows mock device data |
| OTLP exporter verified | Week 13 | Metrics in OTel Collector |
| Multi-device support | Week 15 | Two boards, zero cross-contamination |
| Grafana dashboard importable | Week 18 | 30-min quick-start complete |

---

*RTOSTwin v1.0 — Role Document | Engineering Lead | March 2026*
