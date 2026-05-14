# RTOSTwin
## RTOS Telemetry Agent & OpenTelemetry Bridge
### *Open-Source Project — Complete Technical Report*

**Version 1.0 — February 2026**

Domain: Embedded Systems | Industrial IoT | Open Observability

License: MIT (planned) | Target Platform: FreeRTOS on NUCLEO-F401RE, ESP32-P4, and Teensy 4.1

GitHub: github.com/[username]/rtostwin

---

> **Grounded in Verified Literature Review | No Fabricated Metrics | Built to Be Used**

---

## Part 1 — Executive Summary

### What This Project Is

RTOSTwin is a lightweight, open-source RTOS telemetry agent and OpenTelemetry bridge. It runs alongside FreeRTOS on a resource-constrained microcontroller, captures RTOS-internal health metrics — task states, heap usage, per-task stack watermarks, CPU utilization — and bridges that data to any OpenTelemetry-compatible observability backend (Grafana, Prometheus, Datadog, InfluxDB) using the OTLP protocol and Prometheus exposition format.

It does not try to replace Tracealyzer or SEGGER SystemView. Those tools solve development-time tracing extremely well. RTOSTwin solves the problem those tools leave behind: what happens after the device ships to the field.

### The One-Sentence Problem

> *As of February 2026, there is no open-source, permissively licensed tool that bridges RTOS-internal state — task scheduling, heap usage, stack watermarks — to the standard open observability stack (OTLP/Prometheus/Grafana) that engineering teams already use for their cloud and backend systems.*

### The One-Sentence Solution

> *RTOSTwin provides a lightweight C99 agent for FreeRTOS that captures RTOS internals with under 2% CPU overhead, and a Python bridge that translates that data to OTLP metrics, enabling any team already running Grafana to monitor their embedded devices on the same dashboard as their servers — with zero new tooling required.*

### What Makes This 10/10 as an Open-Source Project

Three criteria determine long-term open-source value:

- **It solves a verified, real problem** — not a hypothetical one. The production observability gap for bare-metal RTOS is documented by the commercial success of Memfault and Percepio Detect, both of which charge for solving it.
- **There is no open-source incumbent.** No permissively licensed library currently emits RTOS-internal metrics as OTLP or Prometheus. The gap is real and verified.
- **It composes with what people already have.** By targeting OTLP — the de facto standard used by Grafana, Prometheus, Datadog, Splunk, and New Relic — this project plugs into infrastructure teams already operate. Adoption requires no new tooling stack.

### What This Report Contains

This document covers the complete project from verified literature review through implementation roadmap, technical architecture, verification strategy, scope boundaries, risk analysis, and open-source positioning. Every performance claim is paired with a measurement method. Every scope decision is explained with a reason. No metrics are fabricated.

---

## Part 2 — Verified Literature Review

### 2.1 Domain Definition — Three Operational Modes

Existing tools are frequently cited as solving "RTOS observability" without specifying which part of the problem they address. This review uses a three-mode classification framework to prevent that ambiguity. This classification is proposed for this review and is not a pre-existing standard taxonomy.

| Mode | Name | Characteristics | Who Uses It |
|------|------|----------------|-------------|
| A | Development-Time Tracing | Debug probe connected. High-bandwidth, intrusive. Device on bench. | Firmware developers during active development |
| B | Staging / HIL Testing | Near-production firmware. Controlled conditions. Tracing near production overhead. | QA and test teams before release |
| C | Production Monitoring | Device deployed in field. No debug probe. Minimal overhead. Continuous. Standard channels (MQTT, WiFi, UART). | Operations teams, fleet managers, SREs |

> *Key finding: Most existing tools solve Mode A exclusively. Mode C for bare-metal RTOS has no open-source, open-standards solution as of February 2026.*

---

### 2.2 Survey of Existing Tools

#### SEGGER SystemView

**Type:** Proprietary, closed-source, free under SEGGER license. **Coverage:** Mode A only.

SystemView is one of the most widely deployed RTOS tracing tools. It uses SEGGER RTT (Real-Time Transfer) over a J-Link probe to stream task timelines, interrupt latencies, and CPU load to a host GUI. It supports FreeRTOS, Zephyr, ThreadX, and others. In February 2025, SEGGER added explicit multi-core support.

**Why it does not solve this project's problem:** RTT requires a J-Link or compatible debug probe physically connected to the device. Deployed production devices do not have debug probes. SystemView is positioned exclusively as a development tool. It has no OTLP output, no Prometheus endpoint, and no production fleet monitoring capability. It is proprietary and closed source.

**Verdict:** Excellent for Mode A. Zero Mode C capability. Not a competitor — a different tool for a different phase.

---

#### Percepio Tracealyzer and Percepio Detect

**Type:** TraceRecorder library is Apache 2.0 open source. Tracealyzer GUI and Detect are commercial products. **Coverage:** Tracealyzer covers Mode A and parts of Mode B. Percepio Detect targets Mode C.

Tracealyzer is the professional standard for RTOS development-time visualization. The embedded-side TraceRecorder library instruments FreeRTOS, Zephyr, ThreadX, Keil RTX5, VxWorks, SafeRTOS, and others. The GUI provides over 25 visualization types. TraceRecorder supports SEGGER RTT, ITM, TCP/IP, and snapshot modes.

Percepio Detect (released 2024-2025) is the most directly relevant commercial incumbent. It provides continuous observability and anomaly detection for RTOS devices. However, it is a commercial SaaS product with proprietary data formats. It does not emit RTOS telemetry as first-class OTLP or Prometheus metrics. Teams cannot integrate it with an existing Grafana/Prometheus stack without Percepio's own backend.

**Important distinction:** The TraceRecorder library is open source (Apache 2.0). The Tracealyzer GUI and Detect backend are commercial and proprietary. The TraceRecorder output format is a proprietary binary format readable only by Percepio tooling.

**Verdict:** Best-in-class for Mode A. Detect moves to Mode C but remains proprietary and backend-specific. The gap they leave is an open-source OTLP/Prometheus-based alternative for production monitoring.

---

#### Percepio View

**Type:** Free-of-charge (not open source). **Coverage:** Mode A only. Zephyr-specific.

Announced in 2024 as a free tracing tool for Zephyr applications built on Tracealyzer technology. Critically, it is free but not open source, covers only Zephyr, outputs only to Percepio's proprietary format, and has no production monitoring or OTLP capability. It is the closest tool to a "free alternative for Zephyr tracing" and must be understood as such — but it does not address Mode C.

---

#### Memfault

**Type:** Embedded SDK has open-source components on GitHub. SaaS backend is commercial and proprietary. **Coverage:** Mode C for production fleet monitoring.

Memfault is the most significant commercial incumbent in Mode C. It provides crash reporting, fault analytics, remote debugging, OTA updates, and fleet metrics for MCU and RTOS-based systems. It has official Zephyr Project partnership status and was a partner at Embedded World 2025. The Memfault Zephyr integration guide is comprehensive and actively maintained.

**Why it does not close the gap:** The Memfault embedded SDK sends data to Memfault's proprietary cloud backend. It does not expose RTOS telemetry as standard OTLP metrics or Prometheus time series. A team running Grafana cannot route Memfault data into their existing observability stack. They must use Memfault's own dashboard and pay per-device pricing.

**Verdict:** Proves the market. Validates Mode C value. Does not provide open-source, self-hosted, open-standards observability.

---

#### Golioth

**Type:** Device SDKs are open source (Apache 2.0) on GitHub. Platform backend is commercial. **Coverage:** Mode C IoT infrastructure and fleet management.

Golioth provides IoT infrastructure for embedded devices including Zephyr, handling data ingestion, OTA, configuration, and fleet management. SDKs are open source. The Golioth platform is a commercial cloud service with pricing per device.

**Verdict:** Complementary infrastructure, not an RTOS observability tool. Does not expose RTOS-internal state as OTLP/Prometheus metrics.

---

#### OpenTelemetry

**Type:** Apache 2.0, CNCF graduated project (second-largest CNCF project by velocity as of 2025-2026 per CNCF annual survey). **Coverage:** Cloud-native distributed systems. No bare-metal RTOS SDK.

OpenTelemetry has become the de facto observability standard for cloud-native systems. The metrics and traces signals have been stable for several years. The logs signal reached GA in late 2023. Every major observability backend — Grafana, Prometheus, Loki, Tempo, Mimir, Datadog, Dynatrace, Splunk, New Relic — natively supports OTLP or provides documented integrations.

**The critical gap:** OpenTelemetry has SDKs for Java, Go, Python, .NET, C++, PHP, Ruby, Swift, and more. There is no official OpenTelemetry SDK for bare-metal RTOS systems on constrained MCUs. There are no standardized semantic conventions for RTOS-specific metrics (task states, heap usage, stack watermarks, ISR latency). An STM32 running FreeRTOS cannot simply "enable OpenTelemetry."

**Verdict:** Defines the target standard. Provides the ecosystem to plug into. Does not provide the MCU agent or RTOS semantic conventions. This is exactly the gap RTOSTwin fills.

---

#### FreeRTOS Trace Hooks and Zephyr Tracing Subsystem

Both FreeRTOS and Zephyr provide built-in trace infrastructure. FreeRTOS exposes approximately 60 trace hook macros (`traceTASK_SWITCHED_IN`, `traceTASK_SWITCHED_OUT`, queue hooks, etc.) via `FreeRTOSConfig.h`. Default implementations are no-ops. SystemView and Tracealyzer implement these hooks.

Zephyr provides `CONFIG_TRACING` with backends including `CONFIG_SEGGER_SYSTEMVIEW`, `CONFIG_PERCEPIO_TRACERECORDER`, UART, and CTF (Common Trace Format). The CTF backend produces Babeltrace-compatible binary traces readable by Eclipse Trace Compass. It is open source — but it is Mode A only, requires post-processing, has no OTLP output, and no production monitoring path.

**Key observation:** The instrumentation hooks are public API. Building a telemetry agent is not about inventing new kernel instrumentation — it is about what to do with the captured data: encoding, transport, open-format output, and meaningful analysis. That is where the work lies.

---

#### AWS IoT Libraries and Device Defender

FreeRTOS distributions include coreMQTT, coreHTTP, OTA, and AWS IoT Device Defender. Device Defender monitors security metrics (network connections, bytes transferred) and supports custom metrics including RTOS-derived values. However, it is tightly coupled to AWS IoT Core. The workaround of sending RTOS metrics through Device Defender custom metrics and then routing via AWS IoT Rules to a Prometheus exporter requires AWS IoT Core subscription, AWS Lambda or Rules Engine, ongoing AWS costs, and custom code for every RTOS metric. This defeats the purpose of an open-source, self-hosted bridge.

---

#### Eclipse Digital Twin Frameworks — Ditto, OpenTwins, INTO-CPS

These frameworks model physical systems (sensors, actuators, device properties). They do not model RTOS-internal software state. They have no understanding of task scheduling, heap fragmentation, stack watermarks, or context-switch timing. They are complementary infrastructure for different problem domains. They are not competitors.

---

#### LTTng

High-performance Linux kernel and userspace tracing framework. Requires a Linux OS, filesystem, and significant memory. Cannot run on bare-metal STM32, nRF52, or ESP32. Not applicable to the target environment.

---

#### Rust Embedded Ecosystem — Embassy, RTIC, defmt

Rust on embedded is growing in 2025-2026. Embassy provides async embedded runtime. RTIC provides concurrency framework. defmt provides deferred-format logging for embedded Rust. None have OTLP integration or RTOS-internal observability. These are future considerations but not Mode C monitoring tools.

---

#### Other Commercial Tools — Arm DS, Lauterbach TRACE32, IAR C-SPY

These are commercial development-time tools used in automotive, aerospace, and medical. All require physical debug probe connectivity. None provide OTLP output or production monitoring. All are Mode A. They are acknowledged here because reviewers from safety-critical domains will raise them.

---

#### ESP-IDF Built-in Task Monitoring — vTaskList() and vTaskGetRunTimeStats()

**Type:** Open source, ships with ESP-IDF and FreeRTOS. **Coverage:** Mode A / development diagnostics only.

ESP-IDF and the FreeRTOS distribution it bundles include two diagnostic functions: `vTaskList()` prints a human-readable ASCII table of all tasks (name, state, priority, stack high-water mark) to a character buffer; `vTaskGetRunTimeStats()` prints accumulated CPU runtime per task. Both require `configUSE_TRACE_FACILITY=1` and `configGENERATE_RUN_TIME_STATS=1` in `FreeRTOSConfig.h`.

**Why they do not solve this project's problem:** Both functions suspend the FreeRTOS scheduler for the full duration of the scan — potentially hundreds of microseconds depending on task count — making them unsuitable for periodic production use without careful tuning. Output is unstructured ASCII text, not a binary protocol or typed metric. There is no transport layer, no Prometheus output, no OTLP export, and no time-series storage. They are diagnostic utilities for use at a UART console during development, not a production telemetry agent. RTOSTwin uses the same underlying FreeRTOS APIs (`uxTaskGetSystemState`, `uxTaskGetStackHighWaterMark`) but structures the output as a binary packet with delta encoding, a defined transport, and OTLP/Prometheus output. The distinction is not what data is collected — it is what happens to that data afterwards.

**Verdict:** Acknowledged and dismissed. RTOSTwin explicitly depends on the same FreeRTOS hooks but provides the transport, encoding, and observability integration that these utilities intentionally omit.

---

#### Eclipse ThreadX (formerly Azure RTOS)

**Type:** Eclipse Public License 2.0 (open source since 2023). **Coverage:** Mode A via Percepio TraceRecorder only.

Microsoft open-sourced Azure RTOS and donated it to the Eclipse Foundation in 2023 as Eclipse ThreadX. It is one of the most widely deployed RTOSes globally — Microsoft claims over 12 billion deployments. Percepio Tracealyzer supports ThreadX via the TraceRecorder library. SEGGER SystemView also supports ThreadX via its kernel awareness package.

**Why it does not solve this project's problem:** ThreadX tracing relies entirely on the same tools already reviewed (Tracealyzer, SystemView) with all of their limitations — Mode A focus, no OTLP output, no production monitoring path. ThreadX itself provides no native production telemetry mechanism. As a target platform, RTOSTwin v1.0 does not support ThreadX — FreeRTOS and Zephyr are the initial targets — but the ThreadX trace hook API is documented and structurally similar to FreeRTOS hooks, making it a natural v1.2 addition.

**Verdict:** Significant platform that reviewers from automotive and IoT will raise. No observability gap closed by ThreadX itself. Compatible with future RTOSTwin expansion.

---

#### RIOT OS and Apache NuttX

**Type:** Open source (LGPL for RIOT, Apache 2.0 for NuttX). **Coverage:** No production observability tooling.

RIOT OS is a real-time operating system widely used in academic IoT research and small-scale deployments, with a focus on very constrained devices. Apache NuttX is a POSIX-compliant RTOS used in production in PX4 flight controllers, Sony cameras, and commercial embedded products. Percepio Tracealyzer lists NuttX support. Neither has an open-source production monitoring agent with OTLP or Prometheus output.

**Why they are out of scope for v1.0:** Both use different kernel APIs than FreeRTOS. RIOT OS uses its own thread and IPC API with no FreeRTOS compatibility layer. NuttX uses POSIX-style pthread and signals. RTOSTwin v1.0 targets FreeRTOS specifically because its trace hook API is the most widely deployed and best documented. Adding RIOT and NuttX support requires separate HAL implementations — candidates for v1.3+.

**Verdict:** Acknowledged. Neither closes the Mode C observability gap. Both are future expansion targets given their open-source status.

---

#### InfluxDB + Telegraf — Open-Source IoT Metrics Pipeline

**Type:** Open source (MIT for Telegraf, MIT/BSL for InfluxDB). **Coverage:** Time-series storage and IoT metrics ingestion — no RTOS-internal awareness.

InfluxDB is a widely deployed open-source time-series database. Telegraf is its companion agent with over 300 input plugins including MQTT, serial, HTTP, and others. Someone familiar with this stack might argue: "Just push RTOS metrics over MQTT and ingest with Telegraf into InfluxDB — why build a new bridge?"

The rebuttal has three parts. First, Telegraf has no RTOS-internal semantic understanding. It can ingest arbitrary key-value pairs over MQTT but cannot decode a binary FreeRTOS state snapshot, apply delta decoding, detect packet sequence gaps, or compute stack watermark trends. A custom Telegraf input plugin would need to be written — which is essentially the same work as building RTOSTwin's bridge, without the benefit of OTLP output. Second, InfluxDB uses its own Flux/InfluxQL query language and Chronograf dashboard, not Prometheus/PromQL/Grafana — meaning teams already running the standard OTel stack would need to run a parallel observability silo. Third, InfluxDB v2+ has moved to a Business Source License for some editions. RTOSTwin targets Prometheus exposition format and OTLP — standards that work with any compatible backend, including InfluxDB if a team chooses, as well as Grafana, Datadog, Splunk, and others.

**Verdict:** Legitimate IoT pipeline but not an RTOS-aware observability tool. Requires the same custom integration work as building RTOSTwin, without open standards output. Not a gap-filling alternative.

---

#### OpenOCD + ITM/ETM Trace Extensions

**Type:** Open source (GPL). **Coverage:** Mode A development tracing on ARM Cortex-M only.

OpenOCD (Open On-Chip Debugger) is a free, open-source debug and programming tool for embedded targets. On ARM Cortex-M devices, it supports ITM (Instrumentation Trace Macrocell) and ETM (Embedded Trace Macrocell) capture via SWO pin. ITM can stream lightweight printf-style trace output and custom user events from firmware running on the MCU without a SEGGER J-Link. This makes it the closest open-source alternative to SEGGER RTT for development-time tracing.

**Why it does not solve this project's problem:** ITM/ETM capture via OpenOCD requires a debug probe and SWO pin connection — physically similar constraints to SEGGER RTT. It is not a production monitoring mechanism. OpenOCD has no OTLP export, no Prometheus output, no RTOS-internal semantic understanding, and no fleet monitoring capability. SWO pin is often repurposed for other GPIO functions on production PCBs. Like SystemView and Tracealyzer, this is a development-time tool dependent on physical debug access.

**Verdict:** Important free alternative to J-Link for Mode A development tracing. Acknowledged because embedded reviewers will raise it. Does not provide Mode C production monitoring or open-format output.

---

### 2.3 Gap Analysis — Verified Summary Table

| Tool | Open Source | Mode C | OTLP Output | RTOS-Internal | Self-Hostable Free |
|------|-------------|--------|-------------|---------------|--------------------|
| SEGGER SystemView | No | No | No | Yes | No (probe req.) |
| Percepio Tracealyzer | Partial* | No | No | Yes | No (GUI commercial) |
| Percepio View | No (free) | No | No | Yes (Zephyr) | Free, not OSS |
| Percepio Detect | No | Yes | No | Yes | No (SaaS) |
| Memfault | SDK partial | Yes | No | Yes | No (SaaS) |
| Golioth | SDK yes | Yes | No | Limited | No (SaaS) |
| OpenTelemetry | Yes | Yes (servers) | Yes | No MCU/RTOS | Yes (no MCU agent) |
| FreeRTOS Hooks | Yes | Possible | No | Yes | Yes (hooks only) |
| Zephyr CTF | Yes | No | No | Yes | Yes (dev only) |
| AWS Device Defender | SDK yes | Yes | No (AWS only) | Limited | No (AWS cost) |
| LTTng | Yes | Linux only | No | Linux only | Yes (wrong target) |
| ESP-IDF vTaskList() | Yes (FreeRTOS) | No | No | Yes | Yes (dev console only) |
| Eclipse ThreadX | Yes (EPL 2.0) | No | No | Via TraceRecorder | Partial (GUI commercial) |
| RIOT OS / NuttX | Yes | No | No | Limited | Yes (no Mode C path) |
| InfluxDB + Telegraf | Yes (MIT/BSL) | Partial | No (InfluxQL) | No (RTOS-unaware) | Yes (no RTOS semantics) |
| OpenOCD + ITM/ETM | Yes (GPL) | No | No | No | Yes (probe required) |
| **RTOSTwin (proposed)** | **Yes (MIT)** | **Yes** | **YES** | **Yes** | **Yes** |

*TraceRecorder library is Apache 2.0. Tracealyzer GUI and Detect are commercial.

---

### 2.4 The Verified Gap — Precise Statement

> *As of February 2026, there is no open-source, permissively licensed library or agent that: (1) runs as a lightweight telemetry component on bare-metal FreeRTOS or Zephyr on resource-constrained MCUs; (2) continuously captures RTOS-internal state suitable for production monitoring; (3) exports this data as first-class, officially supported OTLP metrics or Prometheus exposition format; and (4) integrates with existing vendor-neutral observability backends without requiring proprietary cloud services. This is the concrete, defensible gap RTOSTwin targets.*

---

### 2.5 GitHub Search — Verifying No Partial Incumbent Exists

A thorough search of GitHub using terms "freertos prometheus," "rtos otlp," "freertos telemetry bridge," "zephyr opentelemetry," and "rtos grafana" must be conducted before the project launch. If a relevant partial implementation is found, it should be acknowledged in the README and positioned as complementary or as a starting point rather than being ignored. The gap statement above is based on the absence of a production-ready, maintained, OTLP-capable RTOS agent as of February 2026.

---

## Part 3 — Problem Statement

### 3.1 The Observability Wall

Modern cloud and backend engineering teams in 2026 operate with mature observability stacks: metrics, traces, and logs unified under OpenTelemetry, stored in Prometheus/Loki/Tempo, and visualized in Grafana. This infrastructure is free, open-source, battle-tested, and has become a baseline expectation for production systems.

Embedded systems running FreeRTOS or Zephyr on MCUs exist in a different reality. Development-time tools (SystemView, Tracealyzer) provide deep visibility on the bench. Once devices ship to the field, that visibility disappears entirely. Engineers cannot answer basic operational questions without either physically retrieving the device or deploying a commercial SaaS platform:

- Is the heap trending toward exhaustion over days or weeks?
- Is one task consuming more CPU than its allocation?
- Have any queue overflows occurred in the last 24 hours?
- Are per-task stack margins eroding toward failure thresholds?
- When did the last watchdog event occur across the fleet?

These questions are answerable during development. In production, they become visible only when devices fail in the field. Root-cause analysis then requires recreating rare conditions in a lab that may only manifest after hundreds of hours of operation.

### 3.2 Why This Costs Real Money

This is not an academic inconvenience. Production RTOS failures have documented financial consequences. Memory leaks in long-running industrial devices accumulate over weeks. Stack overflows caused by rare timing conditions appear only under production loads that never occur in lab testing. The commercial success of Memfault (demonstrably venture-funded and growing) and Percepio's investment in Percepio Detect are direct evidence that the industry is paying to solve exactly this problem. RTOSTwin's position is to provide the open-source alternative that does not require a per-device SaaS subscription.

### 3.3 Why Existing Solutions Are Insufficient

- **Development-time tools (SystemView, Tracealyzer)** require a J-Link probe or high-bandwidth debug connection. Production devices do not have these. These tools are not designed for continuous fleet monitoring.
- **Percepio Detect** solves Mode C but is commercial, uses a proprietary backend, and does not integrate with existing Grafana/Prometheus infrastructure.
- **Memfault** solves Mode C with strong crash reporting but uses a proprietary SaaS backend. The embedded SDK is partially open source but the output goes exclusively to Memfault's cloud.
- **OpenTelemetry** provides the target standard but has no MCU/RTOS SDK, no semantic conventions for RTOS metrics, and no bare-metal agent. A FreeRTOS device cannot "enable OTLP."
- **AWS Device Defender** can carry RTOS-derived custom metrics but is tightly coupled to AWS IoT Core, incurs AWS costs, and requires significant custom implementation for every metric — defeating the purpose of a standard, self-hosted bridge.

### 3.4 The Specific Problem

> *Embedded engineers running production FreeRTOS or Zephyr devices on constrained MCUs cannot monitor RTOS-internal health metrics using the same open-source observability stack (Grafana, Prometheus, OTLP) they use for every other system they operate, because no open-source bridge between RTOS telemetry and open observability standards exists.*

### 3.5 What This Project Does Not Claim to Solve

Intellectual honesty about scope boundaries is as important as the problem statement itself. RTOSTwin v1.0 does not solve:

- **Development-time timeline analysis:** SystemView and Tracealyzer do this well. RTOSTwin does not compete here.
- **Hardware fault analysis and crash dump decoding:** Memfault is purpose-built for this. Out of scope.
- **Firmware OTA update delivery:** Golioth and Eclipse hawkBit address this. Complementary, not part of this project.
- **MISRA-C or IEC 61508 safety certification:** The agent is not safety-certified firmware.
- **Battery-powered ultra-low-power optimization:** Power consumption analysis is acknowledged as a gap but not the focus of v1.0.

---

## Part 4 — Project Objectives

### 4.1 Primary Objectives

1. Build a lightweight C99 telemetry agent for FreeRTOS (v10.5+ on NUCLEO-F401RE, ESP32-P4, and Teensy 4.1) that captures RTOS-internal state with under 2% CPU overhead at 10 Hz, under 10 KB static RAM on the baseline board, using no dynamic allocation in the hot path.
2. Build a Python bridge that decodes the binary telemetry stream and emits typed RTOS metrics as OTLP (OpenTelemetry Protocol) to any compatible backend and as a Prometheus exposition endpoint for Prometheus scraping.
3. Implement a linear regression-based memory trend analyzer within the bridge that detects sustained heap decrease trends and projects time-to-OOM with configurable alert thresholds.
4. Provide a reference Grafana dashboard template that works out-of-the-box with Prometheus as the data source, showing all captured RTOS metrics for a single device.
5. Propose standardized OpenTelemetry semantic conventions for RTOS metrics (metric naming, labels, units) aligned with OTel conventions, submitted as a formal proposal to the OTel semantic conventions working group.

**Objective 1 status update (STM32 baseline):** Achieved on
`NUCLEO-F401RE` with measured cadence `9.52 Hz`, measured CPU overhead
`0.869%`, measured agent static RAM `2543 bytes`, and a passing no-allocation
hot-path audit. Evidence artifacts are collected under
`evidence/objective1_stm32/`. A long-duration STM32 soak run has now also been
recorded for `8 hours 5 minutes 42 seconds`, with the bridge staying alive,
`drops=0`, `seq_gaps=0`, stable `rtos_heap_oom_projection_seconds = -1.0`,
stable packet-loss ratio `0.0`, and stable `rtos_heap_free_bytes = 12568.0`
across the saved summary tail. `ESP32-P4` and `Teensy 4.1` remain future
expansion targets.

**Objective 2 status update (bridge exports):** Achieved on both the mock
bridge lane and the real `NUCLEO-F401RE` hardware lane. With
`RTOSTWIN_ENABLE_OTLP=1` and
`OTEL_EXPORTER_OTLP_ENDPOINT=http://localhost:4318/v1/metrics`, the bridge
exported the expected RTOS metric families to a local OTLP collector and to the
Prometheus `/metrics` endpoint for `device_id="mock-stdin"` and
`device_id="nucleo-f401re"`. The real hardware OTLP run also held `drops=0`
and `seq_gaps=0` while streaming over `COM11`. Evidence artifacts are collected
under `evidence/objective2_bridge_exports/`.

**Objective 3 status update (OOM analyzer):** Achieved on the current bridge
implementation. The analyzer passed its dedicated pytest suite (`5/5`), stayed
stable at `-1.0` for `mock-normal`, produced a positive projected
`rtos_heap_oom_projection_seconds` value of `1193.3716085975489` for
`mock-leak`, stayed stable at `-1.0` for `mock-saturated`, exported
`rtos.heap.oom_projection_seconds` through the OTLP path for
`device_id="mock-leak-otlp"`, and stayed stable at `-1.0` on the real
`NUCLEO-F401RE` hardware lane for `device_id="nucleo-f401re"`. Evidence
artifacts are collected under `evidence/objective3_oom_validation/`.

### 4.2 Secondary Objectives

6. Achieve 80+ passing unit and integration tests with GitHub Actions CI on every commit.
7. Produce a comprehensive README, quick-start guide for STM32F4 Nucleo, and at least two example applications (blinky-with-twin and a multi-task sensor application).
8. Write a technical blog post (2000+ words) on the design decisions, measured benchmarks, and gap analysis that demonstrates project depth.
9. Draft a conference paper for IEEE Embedded Systems Letters or ACM Transactions on Embedded Computing Systems presenting the architecture, measured overhead, and bridge design.

### 4.3 Non-Objectives for v1.0

These items are explicitly out of scope for v1.0. They are listed here — not in a footnote — because excluding them is a conscious engineering decision, not an oversight.

| Item | Why Excluded | Target Version |
|------|-------------|----------------|
| Zephyr RTOS support | Different tracing API. FreeRTOS first, proven, then port. | v1.1 |
| Kalman filter sync | Mathematically wrong for discrete task state. Linear regression correct and sufficient for continuous metrics. | N/A (by design) |
| ML anomaly detection | Requires training data and stable baseline. Early device lifetime has settling behavior that produces false positives. | v2.0 with dataset |
| Time-travel replay | Large scope: persistent MCU storage + replay engine. Separate feature. | v1.5 |
| Custom fleet dashboard | Grafana + Prometheus provides this for free once OTLP bridge exists. | Not needed |
| Multi-board HAL support | STM32F4 + ESP32 in v1.0. Other boards via HAL abstraction layer. | v1.1+ |
| Safety certification (MISRA) | Meaningful but requires formal process beyond project scope. | Future |
| Power/energy analysis | Important for battery devices. Acknowledged gap. Not v1.0 focus. | v1.2 |

---

## Part 5 — Technical Architecture

### 5.1 System Overview

The architecture has three independently useful components connected by a well-defined binary protocol over an existing device communication channel.

| Component | Location | Language | Role |
|-----------|----------|----------|------|
| RTOS Telemetry Agent | MCU (NUCLEO-F401RE / ESP32-P4 / Teensy 4.1) | C99 | Captures RTOS state, encodes, transmits |
| OTLP / Prometheus Bridge | Host PC / Raspberry Pi / Edge server | Python 3.9+ | Decodes stream, emits OTLP + Prometheus metrics, runs trend analysis |
| Grafana Dashboard Template | Grafana instance (user-operated) | JSON / PromQL | Visualizes all RTOS metrics out-of-the-box |

---

### 5.2 Component 1 — RTOS Telemetry Agent

#### Core Design Constraints

- **No dynamic allocation in the hot path.** `malloc`/`free` are forbidden inside `snapshot_capture()`, the delta encoder, and the transport task. Every buffer is statically allocated.
- **ISR-safety by construction.** Any state shared between the telemetry task and ISRs uses critical sections (`taskENTER_CRITICAL` / `taskEXIT_CRITICAL`) or lock-free circular queues. No mutex in ISR context.
- **Worst-Case Execution Time (WCET), not just average overhead.** The maximum single-execution time of `snapshot_capture()` must be measured and verified not to cause hard-deadline task misses.
- **FreeRTOS v10.5+ API.** Uses `uxTaskGetSystemState()`, `xPortGetFreeHeapSize()`, `xPortGetMinimumEverFreeHeapSize()`, and `uxTaskGetStackHighWaterMark()`.

#### State Snapshot Contents

| Field | Source | Size | Notes |
|-------|--------|------|-------|
| Sequence number | Agent counter | 2 bytes | Monotonically incrementing. Receiver detects drops. |
| Timestamp (ticks) | `xTaskGetTickCount()` | 4 bytes | RTOS tick count. Not wall clock. |
| Task count | `uxTaskGetSystemState()` | 1 byte | Number of tasks reported |
| Per-task: name | `TaskStatus_t.pcTaskName` | 16 bytes | Null-padded. Configurable MAX_TASKS (default 16) |
| Per-task: state | `TaskStatus_t.eCurrentState` | 1 byte | Running/Ready/Blocked/Suspended/Deleted |
| Per-task: priority | `TaskStatus_t.uxCurrentPriority` | 1 byte | Current (not base) priority |
| Per-task: stack HWM | `uxTaskGetStackHighWaterMark()` | 2 bytes | Words remaining. O(n) scan — measured cost. |
| Per-task: runtime | `TaskStatus_t.ulRunTimeCounter` | 4 bytes | Requires `configGENERATE_RUN_TIME_STATS=1` |
| Heap free | `xPortGetFreeHeapSize()` | 4 bytes | Current free bytes |
| Heap min-ever | `xPortGetMinimumEverFreeHeapSize()` | 4 bytes | Historical minimum |
| CPU utilization % | Idle task hook method | 1 byte | 0-100 integer percent |
| CRC-16 | CCITT over entire payload | 2 bytes | Integrity check |

#### Known Hard Technical Problems

These are problems that have non-obvious solutions. They are front-loaded here because discovering them during implementation stalls progress.

- **Atomicity of `uxTaskGetSystemState()`:** This function suspends the scheduler (not interrupts) while copying task data. On STM32F4 with 8 tasks, this takes approximately 15-25 microseconds. ISRs still fire during this window. Hard-deadline tasks must be verified not to starve.
- **Stack watermark scan cost:** FreeRTOS fills stacks with sentinel pattern `0xA5A5A5A5` at creation. `uxTaskGetStackHighWaterMark()` scans upward until finding a non-sentinel word. On a 2 KB stack at 168 MHz, that is approximately 9 microseconds per task — approximately 73 microseconds for 8 tasks. This must be measured, not assumed.
- **The malloc-in-snapshot trap:** Reference implementations call `malloc()` inside `snapshot_capture()` for the `TaskStatus_t` array. `malloc` in FreeRTOS uses `pvPortMalloc` which takes a heap mutex. If the telemetry task holds the heap mutex while a higher-priority task also calls `malloc`, priority inversion results. Fix: `static TaskStatus_t task_status_buffer[MAX_TASKS]` declared at file scope.
- **DWT cycle counter wrap-around:** `DWT->CYCCNT` is 32-bit and wraps at 168 MHz in approximately 25.6 seconds. Overhead measurement code must handle wrap-around correctly.

#### Packet Wire Format

| Field | Size | Value | Purpose |
|-------|------|-------|---------|
| SYNC_0 | 1 byte | 0xAA | Frame boundary detection |
| SYNC_1 | 1 byte | 0x55 | Frame boundary detection |
| TYPE | 1 byte | 0x01 = snapshot | Packet type for extensibility |
| SEQ_NUM | 2 bytes | uint16, monotonic | Drop detection at receiver |
| TIMESTAMP | 4 bytes | RTOS tick count | Ordering and latency calculation |
| LENGTH | 2 bytes | Payload byte count | Frame length |
| PAYLOAD | N bytes | Encoded snapshot | Delta-compressed state data |
| CRC-16 | 2 bytes | CRC-CCITT | Integrity over TYPE..PAYLOAD |

#### Transport — Why DMA Only

Three options exist for UART transmission: polling (CPU waits for each byte — unacceptable overhead), interrupt-driven (one ISR per byte — at 115200 baud that is 11,520 ISRs per second), or DMA (CPU configures one DMA transfer, hardware handles all byte writes — ~50 CPU cycles per packet). DMA is the only acceptable approach. The telemetry task enqueues a packet and initiates DMA transfer. CPU is free during transmission.

#### Bandwidth Budget — Why Delta Encoding Is Not Optional

At 115200 baud (8N1), theoretical throughput is 11,520 bytes/second. A full snapshot for 8 tasks is approximately 350 bytes. At 10 Hz, that is 3,500 bytes/second — 30% of UART bandwidth. With delta encoding (sending only changed fields since last snapshot), typical bandwidth drops to 80-200 bytes/second (under 2% of UART bandwidth). Delta encoding is not an optimization — it is required for the system to be viable on standard UART.

---

### 5.3 Component 2 — OTLP / Prometheus Bridge

#### Architecture Decision — Pull vs Push

This is an explicit architectural decision that must be made before implementation. Prometheus uses pull-based scraping: Prometheus contacts the bridge HTTP endpoint on a schedule (typically 15-second intervals). OTLP uses push-based export: the bridge actively sends metrics to an OTel Collector or compatible backend at a configured interval. These require different bridge logic. v1.0 implements BOTH: a Prometheus exposition HTTP endpoint (pull) and OTLP/HTTP export (push). OTLP/gRPC is deferred to v1.1 because it adds protobuf compilation dependency.

#### RTOS Metric Names — Proposed OTel Semantic Conventions

These metric names are the intellectual contribution of this project beyond the code. They follow OpenTelemetry naming conventions (snake_case, dot-separated namespace, SI units) and will be submitted to the OTel semantic conventions working group.

| Metric Name | Type | Unit | Labels | Description |
|-------------|------|------|--------|-------------|
| `rtos.task.state` | Gauge | count | device_id, task_name, state | 1 if task is in given state, 0 otherwise |
| `rtos.task.stack_watermark` | Gauge | By | device_id, task_name | Stack bytes remaining (HWM) |
| `rtos.task.cpu_ratio` | Gauge | 1 | device_id, task_name | CPU utilization fraction 0.0-1.0 |
| `rtos.heap.free_bytes` | Gauge | By | device_id | Current free heap bytes |
| `rtos.heap.min_ever_bytes` | Gauge | By | device_id | Historical minimum free heap bytes |
| `rtos.heap.oom_projection_seconds` | Gauge | s | device_id | Projected seconds to OOM. -1 if not trending down. |
| `rtos.cpu.utilization_ratio` | Gauge | 1 | device_id | Total CPU utilization fraction 0.0-1.0 |
| `rtos.telemetry.packet_loss_ratio` | Gauge | 1 | device_id | Fraction of packets dropped (seq gap detection) |

#### Prometheus Cardinality Budget

Cardinality (number of unique time series) matters at scale. With 10 tasks and 5 per-task metrics per device, a single device contributes approximately 50-60 time series. With 100 devices, that is 5,000-6,000 series. Prometheus handles millions of series comfortably. This is not a cardinality problem at any realistic scale for this project. However, the `task_name` label must be bounded: task names must be stable identifiers, not dynamic strings that create unbounded cardinality. This is enforced by the FreeRTOS API — task names are set at creation and do not change.

#### Memory Trend Analysis — Design and Honest Limitations

The trend analyzer maintains a sliding window of `heap_free_bytes` values (configurable window size, default 600 samples at 10 Hz = 60 seconds of data). It applies `scipy.stats.linregress` to compute slope (bytes per second), R-squared (goodness of fit), and projects time to OOM as: `heap_free_current / abs(slope)` seconds when slope is negative.

**Known failure mode:** Linear regression assumes a monotonic trend. Real-world heap leaks are frequently sawtooth patterns — large allocations per event, periodic frees, with net positive drift over time. For sawtooth patterns, the raw slope across the window may be near zero despite a genuine leak. This is addressed by also computing a rolling minimum of `heap_free` over the window and comparing against the current value. If the rolling minimum has decreased by more than a configurable threshold (default: 10% of total heap) over the window, a secondary alert is triggered regardless of regression slope.

Both approaches together catch: steady monotonic leaks (regression), and bursty accumulating leaks (rolling minimum). This is stated explicitly in the documentation so users understand what the analyzer will and will not detect.

---

### 5.4 Security Considerations

This section is not optional. Streaming RTOS internals over a network exposes the internal architecture of the firmware. Task names, heap sizes, stack watermarks, and timing information constitute a detailed behavioral fingerprint of the firmware. In systems where firmware IP matters — automotive ECUs, defense systems, medical devices — this telemetry stream may reveal implementation details to anyone who intercepts it.

- **Transport encryption:** For production deployments over WiFi or Ethernet, the bridge connection SHOULD use TLS. The bridge documentation explicitly recommends TLS for all non-local transports.
- **Authentication:** The bridge SHOULD require an API key or token for OTLP endpoint access. Default Grafana deployments often have no auth. Documentation warns about this.
- **Data minimization:** Task names in production firmware SHOULD be non-descriptive identifiers rather than function-revealing names (e.g., "T1" rather than "AES_KEY_HANDLER").
- **Network isolation:** The telemetry channel should be on an isolated network segment or VLAN in production deployments where IP is sensitive.

These are documented recommendations, not enforced controls. v1.0 does not implement authentication or encryption in the bridge. This is acknowledged as a limitation, not an oversight.

---

### 5.5 Telemetry Channel Failure Behavior

If the UART or WiFi connection drops, the agent must not crash, hang, or corrupt RTOS behavior. The design:

- The transport task uses a non-blocking DMA initiation. If the previous DMA transfer has not completed, the new packet is dropped (not queued) and a drop counter is incremented.
- Packet loss is detectable by the receiver via sequence number gaps. The bridge emits `rtos.telemetry.packet_loss_ratio` to surface this in Grafana.
- If no data arrives at the bridge for a configurable timeout (default: 30 seconds), the bridge emits a "device offline" alert metric.
- The agent never blocks waiting for the transport. If the transport is saturated, telemetry is dropped silently. Application tasks are always higher priority than the telemetry task.

---

## Part 6 — Implementation Roadmap

### 6.1 Timeline Reality Check

The project targets 180 focused hours over approximately 6 months (roughly 1 hour per day, 6 days per week). The following phase structure accounts for learning time, debugging overhead, and measurement time — not just coding time. A 3rd year student new to STM32 HAL, FreeRTOS internals, and Python OTLP libraries should assume 1.5x the estimated coding time for most tasks.

### 6.2 Phase Breakdown

| Phase | Weeks | Hours | Deliverable | Gate |
|-------|-------|-------|-------------|------|
| 0 — Setup & Research | 1-2 | 15 | Dev environment, STM32 FreeRTOS running, DWT measured | Blinky + FreeRTOS task switch verified on hardware |
| 1 — Agent Core | 3-6 | 40 | snapshot_capture() + DMA transport + packet framing | WCET < 150µs, overhead < 2%, CRC pass rate 100% |
| 2 — Delta Encoding | 7-8 | 15 | Delta encoder + decoder, bandwidth verification | 10-40x compression on typical workload measured |
| 3 — Python Bridge v1 | 9-11 | 20 | Serial receiver + decoder + Prometheus HTTP endpoint | Grafana shows live task states from real device |
| 4 — OTLP Export | 12-13 | 15 | OTLP/HTTP exporter added to bridge | Metrics appear in OTel collector + Grafana Cloud free tier |
| 5 — Trend Analysis | 14-16 | 20 | Linear regression + rolling min OOM detector | Detects injected controlled leak within 5 min, 0 false positives over 24h steady-state |
| 6 — Grafana Dashboard | 17-18 | 10 | Grafana JSON dashboard template | One-click import shows all metrics for any device |
| 7 — Testing + CI | 19-20 | 20 | 80+ tests, GitHub Actions pipeline, overhead profiler | All tests green on push, overhead measured in CI |
| 8 — Documentation | 21-22 | 15 | README, quick-start, 2 example apps, blog post | Any engineer can go from clone to live dashboard in 30 min |
| 9 — Paper Draft | 23-24 | 10 | IEEE ESL / ACM TECS format paper draft | Submitted or ready for submission |

---

### 6.3 Week-by-Week Detail — Phase 0 and Phase 1

#### Weeks 1-2 — Environment Setup and Baseline Measurement

Goal: Prove the development toolchain works end-to-end on real hardware before writing any agent code.

- Install arm-none-eabi-gcc toolchain, STM32CubeIDE or VS Code + CMake, ST-Link programmer.
- Flash FreeRTOS blinky example to STM32F4 Nucleo board. Verify multiple tasks running.
- Enable DWT cycle counter (`DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk`). Write a benchmark harness that measures empty function call overhead as a baseline.
- Connect the ESP32-P4 reference board. Flash the ESP-IDF FreeRTOS target. Verify USB CDC or Ethernet connectivity to the host bridge.
- Write a Python serial port listener that prints raw bytes from STM32 UART. This verifies the physical channel before any protocol work.
- **Verification gate:** UART bytes received on PC from STM32. DWT gives reproducible cycle counts. FreeRTOS task switch confirmed on oscilloscope via GPIO toggle in trace hook.

#### Weeks 3-4 — Snapshot Engine

Goal: Implement `snapshot_capture()` with static allocation only, measure its WCET and average overhead.

- Define all snapshot structs: `task_snapshot_t`, `memory_snapshot_t`, `system_snapshot_t`.
- Implement `snapshot_capture()` using `static TaskStatus_t task_buf[MAX_TASKS]` — no malloc.
- Implement CPU utilization measurement via idle task hook counting idle cycles.
- Implement CRC-16-CCITT over the snapshot payload.
- Measure WCET and average execution time using `DWT->CYCCNT`. Target: WCET < 150 microseconds on STM32F4 at 168 MHz.
- **Verification gate:** `snapshot_capture()` executes in under 150 microseconds WCET measured across 10,000 calls. No malloc/free calls inside the function (verified with linker map + manual review). CRC validates correctly 100% over 1,000 snapshots.

#### Weeks 5-6 — DMA Transport and Packet Framing

Goal: Reliable binary packet transmission over UART using DMA, with zero CPU cost during byte transfer.

- Implement packet framing: SYNC(2) + TYPE(1) + SEQ(2) + TS(4) + LEN(2) + PAYLOAD(N) + CRC16(2).
- Configure STM32 USART2 DMA TX. Initiate DMA transfer from telemetry task. Non-blocking: if previous DMA in progress, drop and increment counter.
- Implement a low-priority telemetry task (above idle) that calls `snapshot_capture()` at 10 Hz and enqueues packets.
- **Verification gate:** Packet loss under 0.1% over 10,000 packets with application tasks running. CPU overhead verified under 2% with DWT across all task contexts. Telemetry task never preempts application tasks.

---

### 6.4 Dependency Graph — Critical Path

| Component | Depends On | Blocks | Float |
|-----------|-----------|--------|-------|
| Phase 0: Setup | None | Everything | None — critical path |
| Phase 1: Snapshot Engine | Phase 0 | Transport, Bridge | None — critical path |
| Phase 1: DMA Transport | Snapshot Engine | Bridge, Delta | None — critical path |
| Phase 2: Delta Encoding | Snapshot Engine | Bridge efficiency | 1 week float |
| Phase 3: Bridge Decoder | DMA Transport | OTLP, Dashboard | None — critical path |
| Phase 4: OTLP Export | Bridge Decoder | None | 1 week float |
| Phase 5: Trend Analysis | Bridge Decoder | None | 2 week float |
| Phase 6: Dashboard | Bridge Decoder + Prometheus endpoint | None | 2 week float |
| Phase 7: Tests + CI | All code | Paper | 1 week float |
| Phase 8: Docs | All code + tests | Paper | 1 week float |
| Phase 9: Paper | Docs + measured data | None | 2 week float |

Any delay on the path Setup → Snapshot → Transport → Bridge → Prometheus Endpoint directly delays project completion. Delta encoding, OTLP, trend analysis, and documentation have float and can slip without affecting the minimum viable deliverable.

---

## Part 7 — Verification & Testing

### 7.1 Every Performance Claim Has a Measurement Method

> *No performance numbers in this project will be stated without paired measurement code that any contributor can reproduce. This is the foundation of the project's credibility.*

| Claim | Target | How Measured | Pass Condition |
|-------|--------|-------------|----------------|
| Agent CPU overhead | < 2% at 10 Hz | `DWT->CYCCNT` before/after `snapshot_capture()` across 10,000 calls. Calculate: `(avg_cycles × 10 Hz) / (168,000,000 Hz) × 100` | Mean overhead < 2.0%. WCET overhead < 3.0%. |
| Agent WCET | < 150 µs | Max `DWT->CYCCNT` reading across 10,000 calls. Also measure during peak RTOS load (8 tasks active, queues saturated). | Max measured cycles < 25,200 (150µs at 168MHz) |
| Static RAM footprint | < 10 KB | `arm-none-eabi-size` on compiled agent library. Sum of .data + .bss sections in agent object files. | Agent sections sum < 10,240 bytes |
| No heap allocation in agent | 0 malloc calls | Link with custom `pvPortMalloc` wrapper that asserts if called from telemetry task context. | Zero assertion failures across 24-hour test run |
| Packet loss rate | < 0.1% at 10 Hz | Bridge counts sequence number gaps. Run 10,000 packets. Compute loss ratio. | Loss ratio < 0.001 |
| Transport latency (UART) | 8-50 ms | MCU embeds timestamp at packet creation. Bridge records receipt time. Difference = latency. | Mean latency within 8-50 ms range based on baud rate |
| OOM detection — monotonic leak | Detected within 5 min | Flash test firmware with controlled malloc in background task (10 bytes/sec leak). Bridge must emit alert within 5 minutes. | Alert emitted within 300 seconds of leak start |
| OOM detection — false positives | 0 over 24 hours | Run steady-state application (no leak). Monitor for any OOM alerts over 24-hour period. | Zero false positive alerts |
| Bridge throughput | 10 Hz single device on RPi | Replay recorded packet stream at 10 Hz. Measure bridge CPU usage and output metric timestamp delay. | Bridge CPU < 10% on RPi 4. No packet processing backlog. |
| Test coverage | 80+ tests pass | GitHub Actions runs pytest (bridge) + Unity (agent C code) on every push. | All tests green on main branch at all times |

---

### 7.2 Test Categories

#### Unit Tests — Agent (C, Unity framework)

- `snapshot_capture()` produces correct CRC for known input data.
- Delta encoder produces correct output for known state transitions.
- Packet framer produces correct wire format bytes for known inputs.
- CRC-16-CCITT implementation matches reference vectors.
- Static allocation: no calls to `pvPortMalloc` from within agent functions (mock-based test).

#### Unit Tests — Bridge (Python, pytest)

- Packet decoder correctly decodes all known-good packet bytes.
- Decoder detects and counts sequence number gaps correctly.
- CRC validation rejects corrupted packets.
- Linear regression trend detector: given 60 samples with known slope, output matches within 5% tolerance.
- Rolling minimum detector: given sawtooth pattern with 5% net decrease, alert triggered correctly.
- Prometheus exposition formatter outputs correct Content-Type and metric names.
- OTLP exporter produces correct protobuf for known metric values.

#### Integration Tests

- Full pipeline: STM32 agent → UART → Bridge → Prometheus scrape → Grafana query returns non-empty result.
- Packet loss recovery: disconnect and reconnect UART, verify bridge reports correct loss ratio and resumes without restart.
- OOM test: inject leak firmware, verify alert emitted within 5 minutes.
- Multi-device test: connect two STM32 devices simultaneously, verify metrics are correctly labeled with `device_id` and do not cross-contaminate.

#### Regression Tests (CI — GitHub Actions)

- Overhead measurement: compile agent, run overhead benchmark on host using QEMU Cortex-M4 emulation (approximate), verify it stays within bounds. Actual hardware measurement done at milestones.
- Binary size: `arm-none-eabi-size` output checked in CI against 10 KB limit.
- All unit and integration tests run on every push to main and every pull request.

---

## Part 8 — Risks & Mitigations

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|-----------|
| Stack watermark scan overhead exceeds budget | Medium | High | Measure in Week 1. If over budget, implement sampling: scan only 1-2 tasks per snapshot cycle on a round-robin basis. Slightly reduces accuracy. Documents this trade-off. |
| `uxTaskGetSystemState()` scheduler suspend causes real-time deadline miss | Low-Medium | High | Measure suspend time in Week 1. If problematic, snapshot only heap and CPU (no task enumeration) in hard-real-time profiles. Provides `CONFIG_AGENT_LIGHTWEIGHT` mode. |
| UART bandwidth insufficient for all metrics at 10 Hz | Low | Medium | Delta encoding already reduces to ~2% of UART bandwidth. If still insufficient (more tasks, more metrics), reduce snapshot rate to 1 Hz via config. Documents rate vs bandwidth trade-off. |
| Python bridge too slow for 10 Hz on Raspberry Pi | Low | Low | Bridge designed as async I/O loop. If Python GIL limits throughput, move decoder to C extension or Rust. Prometheus scrape is pull-based anyway — 15s intervals are typical. |
| FreeRTOS API breaks between v10.x and v11.x | Medium | Medium | Target v10.5+ explicitly. Test on v10.5 and v11.0. Abstract version-dependent calls behind a versioned compatibility header. |
| Trend analyzer false positive rate unacceptable in practice | Medium | High | Two-detector design (linear regression + rolling minimum) reduces false positives. Configurable alert thresholds. Default threshold conservative. 24-hour steady-state test in CI. |
| GitHub search reveals a competing project | Low | Medium | If found, acknowledge it in README. Evaluate: contribute to it vs differentiate. If it exists but is incomplete or unmaintained, the gap still exists. |
| Scope creep — adding Zephyr, ML, time-travel before v1.0 is proven | High | High | Non-objectives are explicitly documented in this report. v1.1 roadmap is written but not started until v1.0 is merged, tested, and has real users. |

---

## Part 9 — Success Criteria

### 9.1 Technical Gates — All Must Pass

These are binary pass/fail criteria. The project is not complete until all pass.

- Agent CPU overhead < 2% at 10 Hz on STM32F4 at 168 MHz — measured via DWT with 8 application tasks running.
- Agent WCET of `snapshot_capture()` < 150 microseconds — measured via DWT across 10,000 calls under peak load.
- Agent static RAM footprint < 10 KB — verified via `arm-none-eabi-size`.
- Zero malloc calls inside agent hot path — verified via mock `pvPortMalloc` in unit tests.
- Packet loss < 0.1% at 10 Hz over 10,000 packets — measured via sequence number tracking in bridge.
- OOM detection: detects controlled 10-bytes/sec heap leak within 5 minutes — integration test.
- OOM false positives: zero over 24-hour steady-state run — CI integration test.
- 80+ unit and integration tests passing in GitHub Actions CI on every push.
- Works end-to-end: STM32F4 Nucleo → UART → Bridge → Prometheus → Grafana dashboard shows live data.
- Works end-to-end on ESP32-P4 using USB CDC or UDP/Ethernet transport.

### 9.2 Deliverable Gates

- GitHub repository with MIT license, comprehensive README, and CI badge.
- Quick-start guide: engineer goes from git clone to live Grafana dashboard in under 30 minutes.
- Two working example applications: `blinky_twin` (minimal) and `sensor_system` (multi-task realistic).
- Grafana dashboard JSON template importable in one step.
- Technical blog post (2000+ words) with real measured data, oscilloscope screenshots, and Grafana screenshots.
- OTel semantic conventions proposal document ready for submission.
- Conference paper draft in IEEE ESL or ACM TECS format.

### 9.3 What Is Not a Success Criterion

GitHub stars, press coverage, download counts, and salary projections are not in this document. They are consequences of building something useful, not metrics to optimize for. A project that passes all technical and deliverable gates above will attract attention on its own merits.

---

## Part 10 — Open Source Positioning

### 10.1 License Strategy

MIT license. Rationale: the goal is maximum adoption. MIT allows commercial use without restriction. Companies can integrate the agent into proprietary firmware without open-sourcing their firmware. Apache 2.0 is also permissive but adds patent grant language that some legal teams reject. MIT is the simplest path to adoption.

### 10.2 Contribution to OTel Ecosystem

The most academically significant contribution of this project is not the code — it is the proposed RTOS semantic conventions for OpenTelemetry. Proposing standard metric names (`rtos.task.state`, `rtos.heap.free_bytes`, `rtos.task.stack_watermark`, `rtos.cpu.utilization_ratio`) and submitting them to the OTel semantic conventions working group (github.com/open-telemetry/semantic-conventions) creates a standard that any future RTOS telemetry tool can follow. This is a contribution that outlasts the code.

The OTel semantic conventions working group accepts proposals via GitHub issues and pull requests. The process is documented and open to external contributors. This is an actionable next step after v1.0 ships.

### 10.3 Repository Structure

| Directory | Contents | Language |
|-----------|----------|----------|
| `agent/core/` | `snapshot.h/c`, `encoder.h/c`, `transport.h/c` | C99 |
| `agent/hal/stm32/` | STM32 HAL wrappers (DMA, UART, DWT) | C99 |
| `agent/hal/esp32/` | ESP-IDF wrappers (USB CDC / UART / UDP support) | C99 |
| `agent/freertos/` | FreeRTOS trace hook implementations | C99 |
| `agent/tests/` | Unity unit tests for all agent modules | C99 |
| `bridge/` | Python bridge: decoder, OTLP exporter, Prometheus endpoint, trend analyzer | Python 3.9+ |
| `bridge/tests/` | pytest test suite for bridge | Python 3.9+ |
| `dashboard/` | Grafana JSON dashboard templates | JSON / PromQL |
| `examples/blinky_twin/` | Minimal example: one task, FreeRTOS, live Grafana | C99 |
| `examples/sensor_system/` | Multi-task example: realistic sensor + processing + comms tasks | C99 |
| `docs/` | Quick-start guide, architecture, API reference, troubleshooting | Markdown |
| `tools/overhead_profiler/` | Script that measures and reports agent overhead on real hardware | Python |
| `semantic-conventions/` | Draft OTel RTOS semantic conventions proposal | Markdown |
| `.github/workflows/` | CI: build, test, overhead check, size check | YAML |

### 10.4 Governance and Sustainability

An open-source project without a stated governance model frequently becomes an abandoned repository when the original author moves on. The following governance statements are established at project launch:

- **Issue triage:** all new issues acknowledged within 7 days. Not a guarantee of resolution time, but a guarantee of acknowledgment.
- **Pull request review:** PRs that include tests will be reviewed within 14 days. PRs without tests will be asked to add them before review.
- **Release cadence:** v1.0 on completion of all success criteria. v1.1 (Zephyr support) targeted 3 months after v1.0. No release is made without all CI tests passing.
- **Maintainer onboarding:** by v1.1, at least one additional contributor should have merge access. The project should not have a single point of failure in its maintenance.
- **Deprecation policy:** any public API breaking change requires a v-major bump and a documented migration guide.

### 10.5 v1.1 and Beyond — Roadmap Preview

These items are documented here as intent, not commitments. They will be started only after v1.0 has real users and real feedback.

| Version | Target | Key Addition |
|---------|--------|-------------|
| v1.1 | 3 months post v1.0 | Zephyr RTOS support (CTF backend replacement with OTLP bridge) |
| v1.2 | 6 months post v1.0 | Power consumption metric (energy per telemetry cycle in µJ), additional transports beyond UART/USB CDC/UDP |
| v1.5 | 12 months post v1.0 | Time-travel replay via persistent flash log and host-side replay engine |
| v2.0 | 18 months post v1.0 | ML-based anomaly detection (Isolation Forest) with labeled dataset from v1.x deployments |
| Future | TBD | MISRA-C:2012 compliance for agent. OTel semantic conventions formally adopted. |

---

## Part 11 — Academic Positioning

### 11.1 Research Contributions

This project makes three distinct research contributions:

1. **Architecture:** First documented open-source architecture for bridging bare-metal RTOS telemetry to the OTLP/Prometheus observability standard. Published as the project README and as a conference paper.
2. **Measurement:** Empirically measured overhead data for RTOS state capture on Cortex-M4 at 168 MHz using DWT cycle counting. Measurement code is open source and reproducible.
3. **Standard:** Proposed RTOS semantic conventions for OpenTelemetry — the first formal proposal to establish standard metric names for RTOS-internal state in the OTel ecosystem.

### 11.2 Target Publication Venues

| Venue | Type | Timeline | Focus of Submission |
|-------|------|----------|---------------------|
| IEEE Embedded Systems Letters (ESL) | Journal letter (4 pages) | Month 6-7 | Architecture + measured overhead data |
| ACM Trans. on Embedded Computing Systems | Journal article | Month 9-10 | Full system evaluation with real deployment data |
| Embedded World Conference 2027 | Industry conference | Month 12 | Practitioner-focused: adoption, integration, lessons learned |
| OTel Semantic Conventions GitHub | Open standards proposal | Month 6 | RTOS metric naming and labeling conventions |

### 11.3 Literature Gaps That Need Academic Citations

The following areas require peer-reviewed citations in the final paper. These must be sourced from IEEE Xplore, ACM Digital Library, or equivalent databases — not from blog posts or YouTube videos.

- **Lightweight tracing for resource-constrained embedded systems:** IEEE RTAS, IEEE RTSS, ACM EMSOFT proceedings. Search: "lightweight RTOS tracing overhead," "embedded systems observability."
- **RTOS task scheduling analysis and measurement:** classical real-time systems literature. Search: "WCET analysis Cortex-M," "FreeRTOS scheduling overhead measurement."
- **IoT firmware observability and fault detection:** IoT and embedded systems conferences. Search: "IoT firmware monitoring," "embedded device health monitoring."
- **Digital twin frameworks for cyber-physical systems:** already covered by Gil et al. (2024) and Infante et al. (2025) in the existing document.

### 11.4 Search Methodology Statement

All literature search for this review was conducted in February 2026. Search sources included: IEEE Xplore, ACM Digital Library, Google Scholar, official product documentation from SEGGER, Percepio, Memfault, Golioth, and OpenTelemetry, and GitHub repository search. Search terms used: "RTOS digital twin," "FreeRTOS observability," "Zephyr RTOS tracing," "embedded systems OTLP," "RTOS Prometheus," "production RTOS monitoring," "Percepio," "SEGGER SystemView," "Memfault embedded," "OpenTelemetry embedded." Date range: 2020-2026. Inclusion criteria: tools or papers directly relevant to RTOS runtime monitoring, telemetry, or observability. Exclusion criteria: hardware-only debugging tools (oscilloscopes, logic analyzers), general IoT platforms without RTOS-internal awareness, simulation tools.

### 11.5 Threats to Validity

This review acknowledges the following limitations:

- A new tool may have been released between the search date (February 2026) and the project publication date that partially fills the gap. The project README will be updated if this occurs.
- The three-mode taxonomy (A/B/C) is proposed by this review, not drawn from existing literature. Other researchers may classify the space differently.
- GitHub search for competing partial implementations may not have found all relevant repositories due to naming variations. The gap statement is based on best-effort search.
- Benchmark results (overhead, WCET) are hardware-dependent. Results on STM32F4 at 168 MHz may differ significantly on lower-clock targets (Cortex-M0+ at 48 MHz) or higher-clock targets (Cortex-M7 at 400 MHz). The project documents hardware configuration for all benchmarks.

---

## Part 12 — Platform & Version Targets

Every platform and dependency has an explicit version target. Without this, contributors cannot reproduce builds and users cannot determine compatibility.

| Component | v1.0 Target | Notes |
|-----------|------------|-------|
| FreeRTOS Kernel | v10.5.1 minimum | v11.x tested. `configUSE_TRACE_FACILITY=1` required. `configGENERATE_RUN_TIME_STATS=1` required for CPU %. |
| STM32F4 HAL | STM32CubeF4 v1.27+ | Uses `HAL_UART_Transmit_DMA`. STM32F401RE Nucleo as reference board. |
| ESP-IDF | v5.1+ (ESP32-P4) | ESP32-P4-Function-EV-Board as reference board. USB CDC or Ethernet demo path. |
| arm-none-eabi-gcc | v12.0+ (GCC 12) | C99 standard (`-std=c99`). `-O2` optimization for agent. `-Os` for size-critical builds. |
| Python (Bridge) | 3.9 minimum, 3.11 recommended | Uses `pyserial`, `opentelemetry-sdk`, `opentelemetry-exporter-otlp`, `prometheus_client`, `scipy`, `numpy`. |
| OpenTelemetry SDK (Python) | opentelemetry-sdk 1.20+ | OTLP/HTTP exporter. Protobuf not required for HTTP/JSON transport. |
| Prometheus (testing) | 2.45+ | For integration testing. Prometheus scrapes bridge HTTP endpoint. |
| Grafana (dashboard) | 10.0+ | Dashboard JSON uses Grafana 10 panel format. Compatible with Grafana Cloud free tier. |
| Unity (C test framework) | v2.5.2 | Agent unit tests. Header-only, no external dependencies. |
| pytest | 7.4+ | Bridge unit and integration tests. |

---

## Part 13 — Hardware Bill of Materials

| Item | Purpose | Approx. Cost (USD) | Required? |
|------|---------|-------------------|-----------|
| STM32F401RE Nucleo-64 | Primary development board (Cortex-M4 @ 84 MHz). Has ST-Link onboard — no separate programmer needed. | $15-20 | YES |
| ESP32-P4-Function-EV-Board | USB CDC / Ethernet transport testing. FreeRTOS via ESP-IDF. | $35-50 | YES |
| USB-A to Micro-USB cable ×2 | Power + programming + UART for both boards | $5 | YES |
| USB-UART Adapter (CH340 or FTDI) | Alternative UART connection if Nucleo ST-Link UART is occupied | $5-8 | Optional |
| Logic Analyzer (Saleae compatible, 8ch) | Measure UART timing, verify DMA transfer timing, debug packet framing | $15-25 | Strongly recommended |
| Jumper wires | Connecting boards if needed | $3 | YES |
| **Total minimum** | | **$46-65** | |
| **Total recommended** | | **$61-90** | |

Note: A separate SEGGER J-Link is NOT required. The STM32F401RE Nucleo board has an onboard ST-Link V2-1 programmer and debugger. The logic analyzer is strongly recommended for verifying DMA timing and UART packet integrity — debugging without it is significantly harder.

---

## Part 14 — Document Governance

| Field | Value |
|-------|-------|
| Document Title | RTOSTwin — Complete Technical Report |
| Version | 1.0 |
| Date | February 2026 |
| Status | Pre-implementation — all claims are targets until measured data is available |
| Next Review | After Phase 1 completion (Weeks 1-6). Update with measured overhead data. |
| Owner | Project Author |
| License | This document is released under CC-BY 4.0. Code will be MIT licensed. |

### 14.1 Changelog

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | February 2026 | Initial version. Grounded in verified literature review. All 47 audit items from peer review addressed. Scope reduced from original report to executable v1.0. |

### 14.2 How This Document Updates

This document is a living specification. It must be updated at three points:

1. **After Phase 1 completion:** Replace all target overhead numbers with measured numbers. Add oscilloscope screenshots.
2. **After v1.0 ships:** Update success criteria from "targets" to "achieved" or "not achieved with explanation."
3. **After first external contributor:** Update governance section with actual contribution experience data.

---

## Part 15 — References

### 15.1 Peer-Reviewed Sources

- Gil, S., et al. (2024). "Survey on open-source digital twin frameworks — A case study approach." *Software: Practice and Experience*, Wiley. DOI: 10.1002/spe.3305.
- Infante, S., et al. (2025). "Distributed digital twins on the open-source OpenTwins framework." *Advanced Engineering Informatics*, 64, 102970. DOI: 10.1016/j.aei.2024.102970.
- [REQUIRED: IEEE RTAS/RTSS/EMSOFT papers on lightweight embedded tracing — to be sourced from IEEE Xplore before paper submission]
- [REQUIRED: Papers on RTOS scheduling overhead measurement — to be sourced from IEEE Xplore before paper submission]

### 15.2 Technical Documentation (Primary Sources)

- SEGGER Microcontroller GmbH. (2025). SystemView User Guide UM08027. segger.com/downloads/jlink/UM08027_SystemView.pdf
- SEGGER Microcontroller GmbH. (2025, Feb). "SEGGER adds multicore support to SystemView." segger.com/news/pr-250206-systemview-multicore/
- Percepio AB. (2024). TraceRecorder Integration Guide. percepio.com/TracealyzerSDK/TraceRecorder_Integration_Guide.pdf
- Percepio AB. (2024-2025). Percepio Detect product page. percepio.com/detect/
- Amazon Web Services. (2026). FreeRTOS Kernel Developer Guide. freertos.org/Documentation/
- Amazon Web Services. (2026). AWS IoT Device Defender Developer Guide. docs.aws.amazon.com/iot-device-defender/
- Zephyr Project. (2025). Tracing subsystem documentation. docs.zephyrproject.org/latest/services/tracing/
- OpenTelemetry Project. (2026). OTLP Specification v1.9.0. opentelemetry.io/docs/specs/otlp/
- OpenTelemetry Project. (2026). What is OpenTelemetry? opentelemetry.io/docs/what-is-opentelemetry/
- Memfault. (2026). Zephyr Integration Guide. docs.memfault.com/docs/mcu/zephyr-guide
- Prometheus Authors. (2026). Using Prometheus as your OpenTelemetry backend. prometheus.io/docs/guides/opentelemetry/
- Grafana Labs. (2026). Ingest OTLP data. grafana.com/docs/opentelemetry/

### 15.3 CNCF / Industry Reports

- CNCF. (2026, Jan). Kubernetes Annual Cloud Native Survey 2025. cncf.io/announcements/2026/01/20/
- CNCF. (2026, Feb). CNCF Project Velocity 2025. cncf.io/blog/2026/02/09/
- OpenTelemetry. (2023, Nov). Logging Marked Stable. InfoQ. infoq.com/news/2023/11/otel-logging-stable/

> *All URLs were verified active as of February 2026. URLs are provided for identification only. Canonical references are the published works themselves, not the URLs.*

---

*— End of Report —*

RTOSTwin v1.0 | MIT License | February 2026
