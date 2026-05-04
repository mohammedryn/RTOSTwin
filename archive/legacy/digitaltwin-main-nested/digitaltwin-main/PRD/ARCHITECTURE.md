# RTOSTwin — System Architecture

## System Diagram

```
┌─────────────────────────────────────────────────────────────────────┐
│                        MCU (STM32F4 / ESP32)                        │
│                                                                     │
│   ┌─────────────────────┐   ┌──────────────────┐                   │
│   │   FreeRTOS Kernel   │   │  Application     │                   │
│   │   (Tasks, Heap,     │   │  Tasks (user's   │                   │
│   │    Scheduler)       │   │  firmware)       │                   │
│   └────────┬────────────┘   └──────────────────┘                   │
│            │ FreeRTOS API calls                                     │
│            ▼                                                        │
│   ┌─────────────────────────────────────────┐                      │
│   │          TELEMETRY AGENT (C99)          │                      │
│   │                                         │                      │
│   │  snapshot.c ──► encoder.c ──► framer.c  │                      │
│   │   (capture)     (delta)      (CRC+hdr)  │                      │
│   │                      │                   │                      │
│   │                      ▼                   │                      │
│   │              transport.c                 │                      │
│   │           (DMA UART / WiFi)              │                      │
│   └──────────────┬──────────────────────────┘                      │
└──────────────────┼──────────────────────────────────────────────────┘
                   │ Binary packet stream (UART 115200 / WiFi TCP)
                   ▼
┌──────────────────────────────────────────────────────────────────────┐
│                  HOST (PC / Raspberry Pi / Edge)                     │
│                                                                      │
│   ┌────────────────────────────────────────────────────────────┐    │
│   │               PYTHON BRIDGE (Python 3.9+)                  │    │
│   │                                                            │    │
│   │  decoder.py ──► state_manager.py ──┬──► prometheus.py      │    │
│   │  (sync, CRC,    (reconstruct       │   (HTTP /metrics)     │    │
│   │   unframe)       full state from   │                       │    │
│   │                  deltas)           ├──► otlp_exporter.py   │    │
│   │                                    │   (OTLP/HTTP push)    │    │
│   │                                    │                       │    │
│   │                                    └──► oom_analyzer.py    │    │
│   │                                        (regression +       │    │
│   │                                         rolling min)       │    │
│   └────────────────────────────────────────────────────────────┘    │
└──────────────────┬───────────────────────────┬───────────────────────┘
                   │                           │
                   ▼                           ▼
         ┌─────────────────┐        ┌──────────────────┐
         │   Prometheus    │        │  OTel Collector   │
         │  (scrape :8000) │        │  (receive OTLP)   │
         └────────┬────────┘        └────────┬─────────┘
                  │                          │
                  ▼                          ▼
         ┌──────────────────────────────────────────┐
         │              GRAFANA                      │
         │   (Dashboard JSON template provided)      │
         └──────────────────────────────────────────┘
```

---

## Component Boundaries — Strict Rules

### Component 1: Telemetry Agent (C99, runs on MCU)

| Module | Responsibility | Input | Output |
|---|---|---|---|
| `snapshot.c` | Read all FreeRTOS state into `full_snapshot_t` | FreeRTOS API | `full_snapshot_t` struct |
| `profiler.c` | DWT cycle counter measurement | Any function | `profiler_stats_t` |
| `encoder.c` | Delta-encode current vs previous snapshot | Two `full_snapshot_t` | Encoded byte buffer |
| `framer.c` | Add SYNC, SEQ, TYPE, TIMESTAMP, LENGTH, CRC to payload | Encoded bytes | Complete packet buffer |
| `transport.c` | Non-blocking DMA UART transmit | Complete packet | Bytes on wire |
| `uart_dma.c` | STM32 HAL-specific DMA UART | Packet buffer | Hardware UART TX |

**Data flow (one telemetry cycle):**
```
snapshot_capture(&snap)
  → encoder_encode(&snap, buf, sizeof(buf), is_keyframe)
    → frame_packet(encoded_buf, encoded_len, packet_buf, sizeof(packet_buf))
      → transport_send(packet_buf, packet_len)
        → HAL_UART_Transmit_DMA() [non-blocking, hardware handles bytes]
```

**Critical constraints:**
- Telemetry task priority: `tskIDLE_PRIORITY + 1` — NEVER preempts application tasks.
- Zero `malloc`/`pvPortMalloc` in entire data path.
- All buffers are `static` at file scope.
- Critical section (IRQ disabled) only around the raw FreeRTOS API reads — under 50 µs.

### Component 2: Python Bridge (Python 3.9+, runs on host)

| Module | Responsibility | Input | Output |
|---|---|---|---|
| `decoder.py` | Byte-by-byte stateful packet decoder + CRC check | Raw serial bytes | `DecodedPacket` dataclass |
| `state_manager.py` | Reconstruct full state from deltas, maintain per-device state | `DecodedPacket` | `DeviceState` |
| `prometheus_exporter.py` | Serve `/metrics` HTTP endpoint for Prometheus scraping | `DeviceState` | HTTP response |
| `otlp_exporter.py` | Push metrics via OTLP/HTTP to OTel Collector | `DeviceState` | OTLP/HTTP POST |
| `oom_analyzer.py` | Sliding window regression + rolling min on heap data | Heap time series | OOM projection seconds |
| `mock_device.py` | Generate valid binary packet stream without hardware | Config | Byte stream to stdout/TCP |

**Data flow:**
```
serial.read() → decoder.feed_bytes()
  → DecodedPacket → state_manager.update()
    → DeviceState → prometheus_exporter.update_metrics()
                  → otlp_exporter.export()
                  → oom_analyzer.add_sample() → projection_seconds
```

### Component 3: Grafana Dashboard (JSON, runs in Grafana)

- Single JSON file importable via Grafana UI.
- Data source: Prometheus (preconfigured).
- 6 panels: Task States, Stack Watermarks, Heap Free, OOM Countdown, CPU Utilization, Packet Loss.

---

## Transport Options

| Transport | MCU Side | Host Side | When |
|---|---|---|---|
| UART + DMA | STM32 HAL `HAL_UART_Transmit_DMA` at 115200 8N1 | `pyserial` | v1.0 primary |
| WiFi TCP | ESP-IDF `esp_transport` or raw socket | Python `asyncio` TCP server | v1.0 secondary |
| USB CDC | Future | Future | v1.1 |

---

## Bandwidth Budget

- UART 115200 baud (8N1) = 11,520 bytes/sec max throughput.
- Full snapshot (8 tasks) ≈ 350 bytes.
- At 10 Hz full snapshots = 3,500 bytes/sec = **30% bandwidth** — too much.
- With delta encoding: 80–200 bytes/sec = **< 2% bandwidth** — viable.
- **Delta encoding is REQUIRED, not optional.**

---

## Failure Behavior

- DMA busy when new packet ready → **drop packet silently**, increment `tx_drop_count`.
- UART/WiFi disconnects → agent continues capturing, packets dropped, no crash/hang.
- Bridge receives no data for 30s → emit `device_offline` alert metric.
- Corrupted packet (CRC fail) → discard, increment bridge `drop_count`.
- Sequence gap detected → bridge computes `packet_loss_ratio` metric.
- **Agent NEVER blocks waiting for transport. Application tasks always have priority.**

---

## Security (v1.0 Limitations)

- v1.0 does NOT implement TLS or authentication on the bridge.
- Telemetry stream exposes firmware internals (task names, heap sizes, timing).
- Documentation recommends: TLS for non-local transports, API key for OTLP endpoint, non-descriptive task names in production, network isolation.
- These are documented warnings, not enforced controls in v1.0.
