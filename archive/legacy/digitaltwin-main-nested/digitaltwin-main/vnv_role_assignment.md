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
             │ binary stream over UART / WiFi
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

## 📋 Deliverables — C Firmware Side (agent/)

These files run **on the microcontroller**. Written in C99. Target: STM32F4 (ARM Cortex-M4 @ 168 MHz), later ESP32.

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
**Files:** `bridge/device_registry.py`, updated `bridge/main.py`

**What it does:**  
Allows one bridge instance to handle telemetry from N devices simultaneously. Each device is identified by a unique `device_id` (configurable, default: serial port name or WiFi IP). Metrics from different devices carry different `device_id` labels and **never cross-contaminate**.

**Design:**
- `DeviceRegistry` class: dict of `device_id → DeviceState`
- Each `DeviceState` holds: latest snapshot, packet counter, drop counter, OOM trend data
- Bridge main loop: reads from each serial port in async I/O loop (`asyncio`)

**Verification gate:** Two STM32 Nucleo boards connected simultaneously. Grafana shows two separate sets of metrics, each correctly labeled.

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

---

## 🔗 Key Dependencies

| What You Need | Who Provides It | When |
|---|---|---|
| `full_snapshot_t` struct definition | Partner (snapshot engine) | Week 3 |
| Wire format byte layout spec | Partner (wire format spec, Week 3) | Week 3 |
| Mock packet data (for Python dev) | You — write a `mock_device.py` generator | Immediately |
| Prometheus running locally (testing) | You — `docker run prom/prometheus` | Phase 3 start |

> **Start the Python bridge immediately using mock packet data.** You do not need the physical MCU to build and test Deliverables 4–7. Write `mock_device.py` that generates fake but correctly framed binary packets, feed it into your decoder, and build the entire bridge in parallel.

---

## 📅 Timeline

| Milestone | Target Week | Gate |
|---|---|---|
| Packet framer + CRC unit tests pass | Week 4 | Wire format spec received |
| DMA transport verified on hardware | Week 5 | < 0.1% packet loss |
| Delta encoder verified | Week 7 | > 10x compression measured |
| Prometheus endpoint live | Week 10 | Grafana shows mock device data |
| OTLP exporter verified | Week 13 | Metrics in OTel Collector |
| Multi-device support | Week 15 | Two boards, zero cross-contamination |
| Grafana dashboard importable | Week 18 | 30-min quick-start complete |

---

*RTOSTwin v1.0 — Role Document | Engineering Lead | March 2026*
