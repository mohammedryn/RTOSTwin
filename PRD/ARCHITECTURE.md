# RTOSTwin — System Architecture

## System Diagram

```text
┌──────────────────────────────────────────────────────────────────────┐
│            MCU (STM32F401RE / ESP32-P4 / Teensy 4.1)               │
│                                                                      │
│   FreeRTOS / Board Runtime                                           │
│        │                                                             │
│        ▼                                                             │
│   snapshot.c -> encoder.c -> framer.c -> transport.c                │
│      |            |             |             |                      │
│      |            |             |             └─ UART / USB CDC / UDP│
│      |            |             └─ SYNC + header + CRC              │
│      |            └─ delta / keyframe encoding                      │
│      └─ task / heap / CPU capture                                   │
│                                                                      │
│   profiler.c measures WCET and timing around hot paths               │
└───────────────────────────────┬──────────────────────────────────────┘
                                │
                                ▼
                    Binary packet stream to host
                                │
                                ▼
┌──────────────────────────────────────────────────────────────────────┐
│                    HOST (PC / Raspberry Pi / Edge)                   │
│                                                                      │
│   decoder.py -> state_manager.py -> prometheus_exporter.py           │
│                     │                 ├─ /metrics                    │
│                     │                 └─ otlp_exporter.py            │
│                     └─ oom_analyzer.py                               │
│                                                                      │
│   mock_device.py provides the canonical simulated packet stream      │
└───────────────────────────────┬──────────────────────────────────────┘
                                │
                   ┌────────────┴────────────┐
                   ▼                         ▼
            Prometheus scrape          OTel Collector / OTLP backend
                   │                         │
                   └────────────┬────────────┘
                                ▼
                              Grafana
```

---

## Component Boundaries — Strict Rules

### Component 1: Telemetry Agent (C99, runs on MCU)

| Module | Responsibility | Input | Output |
|---|---|---|---|
| `snapshot.c` | Read RTOS state into `full_snapshot_t` | FreeRTOS / board APIs | `full_snapshot_t` |
| `profiler.c` | Measure cycle counts / timing | Any wrapped function | `profiler_stats_t` |
| `encoder.c` | Delta-encode current vs previous snapshot | `full_snapshot_t` | Encoded byte buffer |
| `framer.c` | Add SYNC, TYPE, SEQ, TIMESTAMP, LENGTH, CRC | Encoded bytes | Complete packet buffer |
| `transport.c` | Non-blocking transport dispatch | Complete packet | Bytes on wire |
| `agent/hal/stm32/*` | STM32-specific hardware path | Transport requests | UART DMA behavior |

**Data flow (one telemetry cycle):**

```text
snapshot_capture(&snap)
  -> encoder_encode(&snap, encoded_buf, size, is_keyframe)
    -> frame_packet(encoded_buf, encoded_len, packet_type, seq, timestamp, packet_buf, size)
      -> transport_send(packet_buf, packet_len)
```

**Critical constraints:**

- Telemetry task priority stays at `tskIDLE_PRIORITY + 1`.
- No `malloc` / `pvPortMalloc` in the agent data path.
- File-scope static buffers only.
- Critical sections stay around raw RTOS state reads only.

### Component 2: Python Bridge (Python 3.9+, runs on host)

| Module | Responsibility | Input | Output |
|---|---|---|---|
| `decoder.py` | Byte-stream decoding + CRC validation | Raw bytes | `DecodedPacket` |
| `state_manager.py` | Reconstruct full per-device state | `DecodedPacket` | `DeviceState` |
| `prometheus_exporter.py` | Serve `/metrics` for Prometheus | `DeviceState` | HTTP exposition |
| `otlp_exporter.py` | Push metrics to OTLP backends | `DeviceState` | OTLP export |
| `oom_analyzer.py` | Heap trend analysis / OOM projection | Heap samples | Projection seconds |
| `mock_device.py` | Generate canonical simulated telemetry | Config | Byte stream |

**Data flow:**

```text
read transport bytes
  -> decoder.feed_bytes()
    -> DecodedPacket
      -> state_manager.update()
        -> DeviceState
          -> prometheus_exporter.update_metrics()
          -> otlp_exporter.update()
          -> oom_analyzer.add_sample()
```

### Component 3: Grafana Dashboard

- Dashboard source file: `dashboard/rtostwin_dashboard.json`
- Provisioning files:
  - `grafana/provisioning/dashboards/provider.yml`
  - `grafana/provisioning/datasources/datasource.yml`
- Prometheus scrape config: `prometheus/prometheus.yml`

---

## Transport Options

| Transport | MCU Side | Host Side | When |
|---|---|---|---|
| UART + DMA | STM32 HAL `HAL_UART_Transmit_DMA` | `pyserial` | STM32F401RE baseline |
| USB CDC | Native board USB serial path | Host serial reader | ESP32-P4 / Teensy 4.1 preferred demo path |
| UDP | Board network stack | Python socket / `asyncio` receiver | Networked multi-board demos |

---

## Bandwidth Budget

- UART `115200 8N1` gives about `11,520 bytes/sec`.
- Full snapshots at `10 Hz` are too expensive to send continuously.
- Delta encoding is required for the steady-state path.
- Keyframes remain necessary for resync / recovery.

---

## Failure Behavior

- If transport is busy, the agent drops the packet and increments a drop counter.
- If UART / USB CDC / UDP disconnects, the agent keeps running; transport must not stall application tasks.
- CRC failure on host means the packet is discarded and counted.
- Sequence gaps are surfaced as packet-loss telemetry.

---

## Security (v1.0 Limitations)

- v1.0 does not guarantee secure transport by default.
- The bridge may expose internal firmware telemetry, so non-local deployments need additional controls.
- OTLP / networked deployments should add authentication, TLS, and network isolation outside the base v1 path.
