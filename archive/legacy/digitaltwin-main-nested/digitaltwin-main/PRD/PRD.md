# RTOSTwin — Product Requirements Document (PRD)

## Product Identity
- **Name:** RTOSTwin
- **Tagline:** RTOS Telemetry Agent & OpenTelemetry Bridge
- **License:** MIT
- **Version Target:** v1.0
- **Platforms:** FreeRTOS 10.5+ on STM32F4 / ESP32

---

## Problem Statement

> As of February 2026, there is no open-source, permissively licensed tool that bridges FreeRTOS/Zephyr internal state (task scheduling, heap usage, stack watermarks) to the standard open observability stack (OTLP/Prometheus/Grafana) that engineering teams already use for cloud and backend systems.

### Why This Matters
- **Development tools** (SystemView, Tracealyzer) require a J-Link debug probe. Production devices don't have probes.
- **Commercial solutions** (Memfault, Percepio Detect) are proprietary SaaS with per-device pricing and closed backends.
- **OpenTelemetry** has SDKs for Java, Go, Python, C++ — but zero coverage for bare-metal MCUs running RTOS.
- The gap is **verified by commercial success**: Memfault and Percepio charge money to solve exactly this. No free alternative exists.

### Who Is the User
1. **Embedded firmware engineers** who want production visibility into devices they've already shipped.
2. **DevOps/SRE teams** who already run Grafana+Prometheus and want embedded devices on the same dashboards.
3. **Small IoT companies** that cannot afford per-device SaaS monitoring.

---

## Solution — One Sentence

> A lightweight C99 agent for FreeRTOS that captures RTOS internals with < 2% CPU overhead, paired with a Python bridge that translates that data to OTLP metrics and Prometheus format — enabling any team with Grafana to monitor embedded devices with zero new tooling.

---

## Product Components

| Component | Runs On | Language | Purpose |
|---|---|---|---|
| **Telemetry Agent** | MCU (STM32F4 / ESP32) | C99 | Capture RTOS state, encode, transmit |
| **OTLP/Prometheus Bridge** | PC / RPi / Edge server | Python 3.9+ | Decode stream, emit OTLP + Prometheus metrics, OOM analysis |
| **Grafana Dashboard** | Grafana instance | JSON/PromQL | One-click import, visualize all RTOS metrics |

---

## Success Criteria (All Must Pass)

### Performance Gates
| Metric | Target | Measurement Method |
|---|---|---|
| Agent CPU overhead | < 2% at 10 Hz | DWT cycle counter, 10K calls, 8 tasks running |
| Agent WCET | < 150 µs | Max DWT reading across 10K calls under peak load |
| Agent static RAM | < 10 KB | `arm-none-eabi-size` (.data + .bss) |
| Heap allocs in hot path | Exactly 0 | Mock pvPortMalloc wrapper, 24h test |
| Packet loss | < 0.1% at 10 Hz | Sequence number gap tracking, 10K packets |
| OOM detection (monotonic) | < 5 minutes | Inject 10 bytes/sec controlled leak |
| OOM false positives | 0 over 24 hours | Steady-state firmware, no leaks |
| Test count | 80+ passing | GitHub Actions CI on every push |

### Deliverable Gates
- GitHub repo with MIT license, CI badge, comprehensive README.
- Quick-start guide: `git clone` → live Grafana dashboard in 30 minutes.
- Two example apps: `blinky_twin` (minimal) and `sensor_system` (multi-task realistic).
- Grafana JSON dashboard template — one-click import.
- Technical blog post (2000+ words) with real benchmarks and screenshots.
- OTel semantic conventions proposal document.
- Conference paper draft (IEEE ESL or ACM TECS format).

---

## Non-Goals for v1.0 (Explicit Scope Exclusions)

| Excluded Item | Reason | Future Version |
|---|---|---|
| Zephyr RTOS support | Different tracing API. FreeRTOS first. | v1.1 |
| Kalman filter / state estimation | Mathematically wrong for discrete task state. Linear regression is correct. | Never (by design) |
| ML anomaly detection | Needs training data from v1.x deployments | v2.0 |
| Time-travel replay | Large scope: persistent flash + replay engine | v1.5 |
| Custom fleet dashboard | Grafana + Prometheus already does this for free | Not needed |
| MISRA-C safety certification | Formal process beyond project scope | Future |
| Power/energy analysis | Important for battery devices, not v1.0 focus | v1.2 |

---

## Key Metrics to Capture (RTOS State)

| Field | FreeRTOS API | Size |
|---|---|---|
| Task name | `TaskStatus_t.pcTaskName` | 16 bytes |
| Task state | `TaskStatus_t.eCurrentState` | 1 byte |
| Task priority | `TaskStatus_t.uxCurrentPriority` | 1 byte |
| Task stack watermark | `uxTaskGetStackHighWaterMark()` | 2 bytes |
| Task runtime counter | `TaskStatus_t.ulRunTimeCounter` | 4 bytes |
| Heap free bytes | `xPortGetFreeHeapSize()` | 4 bytes |
| Heap min-ever bytes | `xPortGetMinimumEverFreeHeapSize()` | 4 bytes |
| CPU utilization % | Idle task hook counting | 1 byte |
| Sequence number | Agent counter | 2 bytes |
| Timestamp | `xTaskGetTickCount()` | 4 bytes |
| CRC-16 | CCITT over payload | 2 bytes |

---

## Hardware Requirements

| Item | Required? | Approx Cost |
|---|---|---|
| STM32F401RE Nucleo-64 | YES | $15-20 |
| ESP32-WROOM-32 DevKit V1 | YES | $8-12 |
| USB-A to Micro-USB cable ×2 | YES | $5 |
| USB-UART Adapter (CH340/FTDI) | Optional | $5-8 |
| Logic Analyzer (8ch Saleae-compatible) | Strongly recommended | $15-25 |
| **Total minimum** | | **$46-65** |
