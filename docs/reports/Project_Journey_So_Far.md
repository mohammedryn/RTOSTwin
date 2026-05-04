# RTOSTwin — Project Journey So Far
### A Complete Technical History & Learning Guide Through Phase 1

**Written for:** Mohammed Rayan (RYN) — Embedded Systems Engineer, Wire Protocol Architect  
**Date:** May 2026  
**Project Version:** v1.0-in-progress (Phase 1 complete)  
**Status:** Protocol Frozen ✅ | Snapshot Pending 🔄 | Decoder Pending 🔄 | OOM Pending 🔄

---

> *"Every single byte that flows through RTOSTwin originates from code you write. If your code is wrong, nothing works. If your code is right, everything works."*
> — RYN Role Assignment Document

---

## Table of Contents

- [Section 1 — The Technical Journey Log](#section-1--the-technical-journey-log)
  - [1.1 Where It All Started — The Problem](#11-where-it-all-started--the-problem)
  - [1.2 The Solution Architecture — What We Are Building](#12-the-solution-architecture--what-we-are-building)
  - [1.3 The Team and Ownership Model](#13-the-team-and-ownership-model)
  - [1.4 The Hardware Targets](#14-the-hardware-targets)
  - [1.5 Phase 1 — The Protocol Freeze (COMPLETE)](#15-phase-1--the-protocol-freeze-complete)
  - [1.6 The Wire Format Specification — Deep Dive](#16-the-wire-format-specification--deep-dive)
  - [1.7 The CRC-16-CCITT Algorithm — The Math Explained](#17-the-crc-16-ccitt-algorithm--the-math-explained)
  - [1.8 The Golden Packet Vectors — Mathematically Verified](#18-the-golden-packet-vectors--mathematically-verified)
  - [1.9 The wire_format.h Header — The Code Artefact](#19-the-wire_formath-header--the-code-artefact)
  - [1.10 The Sequence Ownership Decision](#110-the-sequence-ownership-decision)
  - [1.11 What Was Frozen and Why It Matters](#111-what-was-frozen-and-why-it-matters)
  - [1.12 What Comes Next — The Road Ahead](#112-what-comes-next--the-road-ahead)
- [Section 2 — Explain It Like I'm a Noob](#section-2--explain-it-like-im-a-noob)
  - [2.1 What Is RTOSTwin In Plain English?](#21-what-is-rtostwin-in-plain-english)
  - [2.2 The Problem We Are Solving](#22-the-problem-we-are-solving)
  - [2.3 The Two Halves of the System](#23-the-two-halves-of-the-system)
  - [2.4 What Is a Wire Format Spec?](#24-what-is-a-wire-format-spec)
  - [2.5 What Is a CRC-16 Checksum?](#25-what-is-a-crc-16-checksum)
  - [2.6 What Is Delta Encoding?](#26-what-is-delta-encoding)
  - [2.7 What Is a Keyframe?](#27-what-is-a-keyframe)
  - [2.8 What Did Phase 1 Actually Accomplish?](#28-what-did-phase-1-actually-accomplish)
  - [2.9 Why Did We Do Phase 1 Before Writing Any Real Code?](#29-why-did-we-do-phase-1-before-writing-any-real-code)
- [Section 3 — The Translation Map](#section-3--the-translation-map)
  - [3.1 Analogies → Technical Terms Master Table](#31-analogies--technical-terms-master-table)
  - [3.2 The Train Station Analogy — Full Breakdown](#32-the-train-station-analogy--full-breakdown)
  - [3.3 How the Wire Format Translates Into C](#33-how-the-wire-format-translates-into-c)
  - [3.4 How the CRC Translates Into Bytes](#34-how-the-crc-translates-into-bytes)
  - [3.5 How Delta Encoding Translates Into Tags and Bytes](#35-how-delta-encoding-translates-into-tags-and-bytes)
  - [3.6 File-by-File Role Map](#36-file-by-file-role-map)
  - [3.7 Interview Cheat Sheet](#37-interview-cheat-sheet)

---

---

# Section 1 — The Technical Journey Log

*A professional, textbook-style record of everything RTOSTwin has achieved from project inception through the completion of Phase 1.*

---

## 1.1 Where It All Started — The Problem

### The Observability Gap

Modern software engineers who build web apps, backend services, or cloud infrastructure take one thing completely for granted: **they can watch their system run in real time.** They open Grafana, see graphs of CPU usage, memory, request latency — all live, all historical, all free. The tools are called **Prometheus** (for storing metrics) and **Grafana** (for drawing beautiful dashboards from those metrics). Together they use the **OpenTelemetry (OTLP)** standard — an open protocol that any system can speak.

Embedded engineers — the people who write firmware for microcontrollers like the STM32, ESP32, or Teensy — do not have this. When a device ships to the field, the engineer goes blind. They cannot answer these questions without physically recovering the device:

- Is the heap (the pool of available memory) getting full?
- Is one task (a running thread inside the RTOS) consuming more CPU than it should?
- How close is the stack of any task to overflowing?
- Is there a slow memory leak that will cause a crash in three weeks?

The commercial tools that solve this (Memfault, Percepio Detect) cost money per device and lock your data into a proprietary cloud. **No open-source, free, standard alternative exists as of February 2026.** That is the gap this project fills.

### The Verified Market Gap

The RTOSTwin Complete Report (`docs/reports/RTOSTwin_Complete_Report.md`) conducted a thorough literature review of every existing tool:

| Tool | Open Source | Mode C (Production) | OTLP Output | Self-Hosted |
|---|---|---|---|---|
| SEGGER SystemView | No | ❌ No | ❌ No | No (needs probe) |
| Percepio Detect | No | ✅ Yes | ❌ No | ❌ No (SaaS) |
| Memfault | SDK partial | ✅ Yes | ❌ No | ❌ No (SaaS) |
| OpenTelemetry | Yes | ✅ Yes | ✅ Yes | ✅ Yes — but **no MCU agent** |
| **RTOSTwin (this project)** | **Yes (MIT)** | **✅ Yes** | **✅ Yes** | **✅ Yes** |

> **Conclusion:** RTOSTwin is the first open-source, self-hosted, OTLP-capable telemetry bridge for FreeRTOS. Every competitor either requires money, a proprietary cloud, or a debug probe attached to the device.

---

## 1.2 The Solution Architecture — What We Are Building

RTOSTwin has three independently useful components:

```
┌──────────────────────────────────────────────────────────────────────┐
│            MCU (STM32F401RE / ESP32-P4 / Teensy 4.1)                │
│                                                                      │
│   FreeRTOS / Board Runtime                                           │
│        │                                                             │
│        ▼                                                             │
│   snapshot.c → encoder.c → framer.c → transport.c                  │
│      |            |             |             |                      │
│      |            |             |             └─ UART / USB CDC / UDP│
│      |            |             └─ SYNC + header + CRC              │
│      |            └─ delta / keyframe encoding                      │
│      └─ task / heap / CPU capture                                   │
│                                                                      │
│   profiler.c measures WCET and timing around hot paths               │
└───────────────────────────────┬──────────────────────────────────────┘
                                │  Binary packet stream
                                ▼
┌──────────────────────────────────────────────────────────────────────┐
│                    HOST (PC / Raspberry Pi / Edge)                   │
│                                                                      │
│   decoder.py → state_manager.py → prometheus_exporter.py            │
│                     │                 ├─ /metrics (for Prometheus)  │
│                     │                 └─ otlp_exporter.py           │
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

### Component 1: The Telemetry Agent (C99, runs on the MCU)

This is firmware — real C code running on a microcontroller alongside FreeRTOS. It is a pipeline of five modules:

| Module | File | What It Does |
|---|---|---|
| Snapshot Engine | `agent/core/snapshot.c` | Reads all RTOS-internal state (tasks, heap, CPU) into a C struct |
| DWT Profiler | `agent/core/profiler.c` | Measures execution time of hot paths using the ARM cycle counter |
| Encoder | `agent/core/encoder.c` | Compresses the snapshot using delta encoding |
| Framer | `agent/core/framer.c` | Wraps encoded bytes in a packet with a header, sync bytes, and CRC checksum |
| Transport | `agent/core/transport.c` | Sends the packet over UART/USB/UDP using DMA (no CPU busy-waiting) |

**The data flow through the agent on every telemetry cycle (10 times per second):**

```
snapshot_capture(&snap)
  → encoder_encode(&snap, encoded_buf, size, is_keyframe)
    → frame_packet(encoded_buf, encoded_len, type, seq, timestamp, packet_buf, size)
      → transport_send(packet_buf, packet_len)
```

### Component 2: The Python Bridge (runs on the host PC or Raspberry Pi)

| Module | File | What It Does |
|---|---|---|
| Decoder | `bridge/decoder.py` | Reads raw bytes, validates CRC, and produces validated packet objects; Phase 3 upgrades that boundary to fully typed decoded fields |
| State Manager | `bridge/state_manager.py` | Reconstructs full device state from a sequence of keyframes and deltas |
| Prometheus Exporter | `bridge/prometheus_exporter.py` | Serves a `/metrics` HTTP endpoint for Prometheus to scrape |
| OTLP Exporter | `bridge/otlp_exporter.py` | Pushes metrics to any OpenTelemetry-compatible backend |
| OOM Analyzer | `bridge/oom_analyzer.py` | Phase 4 target: analyze heap trends and predict memory exhaustion |
| Mock Device | `bridge/mock_device.py` | Generates a simulated packet stream for testing without real hardware |

### Component 3: The Grafana Dashboard

A pre-built JSON dashboard (`dashboard/rtostwin_dashboard.json`) that any engineer can import into Grafana and immediately see all RTOS metrics from any RTOSTwin device.

---

## 1.3 The Team and Ownership Model

RTOSTwin is built by two engineers working in parallel. The strict ownership model prevents merge conflicts and protocol drift:

### RYN (Mohammed Rayan) — Protocol Architect, Snapshot Engine, Analytics

**RYN owns the data origin and the data meaning:**

| File | What It Is |
|---|---|
| `agent/core/snapshot.h` / `.c` | How data is captured from the RTOS |
| `agent/core/profiler.h` / `.c` | How performance is measured |
| `docs/wire_format_spec.md` | The protocol contract — the truth document |
| `agent/core/wire_format.h` | The C constants that implement the spec |
| `bridge/decoder.py` | How bytes are turned back into structured data |
| `bridge/oom_analyzer.py` | How memory leaks are detected and projected |

### VNV — Framer, Transport, Bridge Infrastructure

**VNV consumes RYN's output and delivers it to the world:**

| File | What It Is |
|---|---|
| `agent/core/framer.c` | Wraps RYN's encoded bytes into a packet |
| `agent/core/encoder.c` | Delta-encodes RYN's snapshot |
| `agent/core/transport.c` | DMA UART/USB/UDP transmission |
| `bridge/state_manager.py` | Reconstructs state from RYN's `DecodedPacket` |
| `bridge/prometheus_exporter.py` | Serves the `/metrics` endpoint |
| `bridge/otlp_exporter.py` | Pushes to OpenTelemetry |
| `dashboard/rtostwin_dashboard.json` | The Grafana dashboard |

### The Integration Boundary

The boundary between RYN and VNV is a contract, not a file. It is the `DecodedPacket` Python object produced by `bridge/decoder.py`. VNV's `state_manager.py` consumes it. RYN defines its shape. The intended steady-state design is that VNV never re-parses raw bytes; they should receive a typed object and work with that once the Phase 3 decoder boundary is complete.

---

## 1.4 The Hardware Targets

RTOSTwin runs on physical microcontroller boards. The rollout order is fixed:

| Board | Processor | Clock | Transport | Status |
|---|---|---|---|---|
| **STM32F401RE NUCLEO-64** | ARM Cortex-M4 | 84 MHz | UART + DMA | **First target — baseline** |
| ESP32-P4-Function-EV-Board | Dual Xtensa LX7 | up to 400 MHz | USB CDC or Ethernet | Second target |
| Teensy 4.1 | ARM Cortex-M7 | 600 MHz | USB CDC | Third target |

> **Important note:** The original report was written with `STM32F4 @ 168 MHz` as the reference. The actual NUCLEO-F401RE runs the STM32F401RE at **84 MHz** (not 168 MHz). All performance numbers (WCET, overhead) must be re-measured on the actual hardware — they cannot be assumed from spec-sheet estimates.

The NUCLEO-F401RE costs $15–20 USD and has an onboard ST-Link programmer. No J-Link needed. A logic analyzer ($15–25) is strongly recommended for debugging UART timing.

---

## 1.5 Phase 1 — The Protocol Freeze (COMPLETE)

### What Was Phase 1?

Phase 1 was about establishing the **single source of truth** for how every byte of every packet is structured. Before Phase 1, the project had:

- `PRD/TECH_SPEC.md` saying one thing about who owns sequence numbers
- `PRD/ARCHITECTURE.md` saying a different thing
- `agent/core/framer.h` implementing a third interpretation
- `agent/main.c` calling `frame_packet()` with the wrong number of arguments

This is called **contract drift** — the documentation, the headers, and the calling code all disagreed with each other. If RYN builds the Python decoder based on one interpretation, and VNV builds the C framer based on another, their code will never speak the same language.

Phase 1 fixed this permanently by producing two artefacts:

### The Two Artefacts of Phase 1

| Artefact | File | What It Does |
|---|---|---|
| The Specification | `docs/wire_format_spec.md` | The human-readable truth document. All protocol decisions live here. |
| The Constants Header | `agent/core/wire_format.h` | The machine-readable truth. Every C constant defined here and nowhere else. |

### What Was Committed on the Branch `fix/v1-protocol-freeze`

Three files changed, five commits:

1. **`docs/wire_format_spec.md` — Created (new file)**  
   The complete protocol specification. 118 lines. Covers: packet header layout, CRC algorithm, keyframe payload, delta payload, device-info type, and 3 golden byte-exact test vectors.

2. **`agent/core/wire_format.h` — Updated**  
   Added 8 new `#define` constants for delta field identifiers and the system-tag base. Updated comment to point at the new spec instead of the old `TECH_SPEC.md`.

3. **`roles/rayan_checklist.md` — Updated**  
   Every Phase 1 task marked complete (`[x]`). The next action item points to Phase 2.

---

## 1.6 The Wire Format Specification — Deep Dive

The file `docs/wire_format_spec.md` is the most important document Rayan produced in this phase. Let's go through every section.

### The Foundational Rule: Little-Endian

When a number is more than one byte long, you have to decide which end of the number to put first. RTOSTwin uses **little-endian**: the least significant byte (the small end) comes first.

Example: The number `0x1234` in little-endian is two bytes: `34` then `12` on the wire.

This matters for every multi-byte field: SEQ_NUM, TIMESTAMP_TICKS, PAYLOAD_LEN, stack_hwm_words, runtime_ticks, heap_free_bytes, heap_min_ever_bytes, CRC_16.

### The Packet Header Layout

Every single RTOSTwin packet starts with exactly the same 12-byte header, regardless of whether it is a keyframe, a delta, or a device-info packet:

```
Byte Offset  │ Field Name       │ Size  │ Value / Encoding            │ Notes
─────────────┼──────────────────┼───────┼─────────────────────────────┼────────────────────────────────
     0        │ SYNC_0          │ 1 byte│ Always 0xAA                 │ Start-of-packet marker
     1        │ SYNC_1          │ 1 byte│ Always 0x55                 │ Start-of-packet marker
     2        │ VERSION         │ 1 byte│ uint8, currently 0x01       │ Protocol version — v1
     3        │ TYPE            │ 1 byte│ uint8, 0x01/0x02/0x03       │ Delta / Keyframe / DeviceInfo
    4–5       │ SEQ_NUM         │ 2 bytes│ little-endian uint16       │ Packet counter (owned by snapshot)
    6–9       │ TIMESTAMP_TICKS │ 4 bytes│ little-endian uint32       │ Raw FreeRTOS tick count
   10–11      │ PAYLOAD_LEN     │ 2 bytes│ little-endian uint16       │ Length of payload ONLY (not CRC)
   12..       │ PAYLOAD         │ N bytes│ type-specific layout       │ Keyframe or delta data
 12+N..13+N   │ CRC_16          │ 2 bytes│ little-endian uint16       │ Checksum over bytes 2..11+N
```

**Key observations:**

1. **SYNC bytes are excluded from CRC.** The CRC protects everything from VERSION (byte 2) through the last byte of PAYLOAD. The two sync bytes are there purely for frame detection — if bytes are corrupted, the decoder uses these magic bytes to find where packets start.

2. **PAYLOAD_LEN describes only the payload, not the CRC.** The total packet size is `WF_OVERHEAD + PAYLOAD_LEN` where `WF_OVERHEAD = WF_HEADER_SIZE + WF_CRC_SIZE = 12 + 2 = 14 bytes`.

3. **TIMESTAMP_TICKS is a raw RTOS tick counter**, not wall-clock time. It comes directly from `xTaskGetTickCount()`. The Python bridge knows the tick rate and converts it.

4. **SEQ_NUM is snapshot-owned.** This was one of the key decisions of Phase 1 (see Section 1.10).

### The Three Packet Types

```c
#define WF_TYPE_DELTA        0x01U   // Only changed fields since last packet
#define WF_TYPE_KEYFRAME     0x02U   // Complete snapshot of all fields
#define WF_TYPE_DEVICE_INFO  0x03U   // Reserved — not implemented in v1
```

In practice, every 50th packet is a keyframe (`WF_KEYFRAME_INTERVAL = 50U`). The other 49 are deltas. This is the delta encoding strategy that keeps bandwidth under 2% of the UART capacity.

### The Keyframe Payload Layout

A keyframe is a **complete snapshot** — it contains every piece of data about every task, plus system-wide heap and CPU metrics. The Python decoder can reconstruct the full device state from just one keyframe.

```
Field                        │ Size   │ Encoding                    │ Notes
─────────────────────────────┼────────┼─────────────────────────────┼──────────────────────────────────
sequence_num                 │ 2 bytes│ little-endian uint16        │ Same as header SEQ_NUM
timestamp_ticks              │ 4 bytes│ little-endian uint32        │ Same as header TIMESTAMP_TICKS
task_count                   │ 1 byte │ uint8                       │ Number of tasks that follow
─────────────────────────────┼────────┼─────────────────────────────┼── Repeated task_count times ─────
  name[16]                   │16 bytes│ ASCII, null-padded          │ Max 15 chars + forced \0 at [15]
  state                      │ 1 byte │ uint8 (eTaskState enum)     │ 0=Running,1=Ready,2=Blocked...
  priority                   │ 1 byte │ uint8                       │ Current (not base) priority
  stack_hwm_words            │ 2 bytes│ little-endian uint16        │ Words remaining in stack
  runtime_ticks              │ 4 bytes│ little-endian uint32        │ Total ticks this task has run
─────────────────────────────┼────────┼─────────────────────────────┼──────────────────────────────────
heap_free_bytes              │ 4 bytes│ little-endian uint32        │ Current free heap bytes
heap_min_ever_bytes          │ 4 bytes│ little-endian uint32        │ Lowest heap has ever been
cpu_utilization_pct          │ 1 byte │ uint8                       │ 0-100 integer percent
```

**Payload size formula (for N tasks):**
```
payload_len = 2 + 4 + 1 + (N × (16 + 1 + 1 + 2 + 4)) + 4 + 4 + 1
            = 7 + (N × 24) + 9
            = 16 + (N × 24)
```

For the golden vector (1 task): `16 + (1 × 24) = 40 bytes = 0x28`. This matches the `28 00` in the header.

**Important design details:**

- **Task name is always exactly 16 bytes.** Even a 4-character name like "IDLE" takes 16 bytes on the wire (padded with 12 null bytes). This makes parsing simple — no length prefix needed.
- **Byte 15 of the name is forced to `\0`.** Even if someone accidentally creates a task with a 16-character name, the wire format guarantees null termination.
- **`stack_hwm_words` is in WORDS, not bytes.** On a 32-bit ARM (4-byte word), multiply by 4 to get bytes. A value of 32 words = 128 bytes of remaining stack space.
- **`heap_min_ever_bytes` is a monotonically decreasing historical minimum.** It is the lowest the heap has ever been since boot. This is critical for the OOM analyzer.

### The Delta Payload Layout

A delta packet says: "here are only the fields that changed since the last packet." This requires a clever encoding scheme.

**Every entry in a delta payload is a `[tag][value-bytes]` pair:**

- **The tag byte** encodes both **which object changed** and **which field of that object changed**
- **The value bytes** are the new value of that field (size depends on the field)

There are two kinds of entries:

**1. Per-task entry (for task-specific fields):**
```
tag = (task_index << 4) | field_id

Example: Task 0's state changed → tag = (0 << 4) | 0x01 = 0x01
Example: Task 2's priority changed → tag = (2 << 4) | 0x02 = 0x22
```

**2. System entry (for heap and CPU fields):**
```
tag = 0xF0 | field_id

Example: heap_free_bytes changed → tag = 0xF0 | 0x05 = 0xF5
Example: cpu_utilization_pct changed → tag = 0xF0 | 0x07 = 0xF7
```

**Field IDs and their value sizes:**

| Field ID | Field Name | Scope | Value Size |
|---|---|---|---|
| `0x01` | task state | Per-task only | 1 byte (uint8) |
| `0x02` | task priority | Per-task only | 1 byte (uint8) |
| `0x03` | task stack watermark words | Per-task only | 2 bytes (uint16 LE) |
| `0x04` | task runtime ticks | Per-task only | 4 bytes (uint32 LE) |
| `0x05` | heap free bytes | System only | 4 bytes (uint32 LE) |
| `0x06` | heap minimum ever bytes | System only | 4 bytes (uint32 LE) |
| `0x07` | CPU utilization percent | System only | 1 byte (uint8) |

**Why task index 0xF is reserved:** The high nibble `0xF` is what the decoder uses to detect system tags (`0xF0 | field_id`). If task index 15 (0xF) were allowed, its tag `0xF0 | field_id` would be indistinguishable from a system tag. Task indices 0x0 through 0xE (15 tasks) are valid.

**Delta termination:** There is no sentinel entry or end marker. The decoder reads entries until it has consumed exactly `PAYLOAD_LEN` bytes. This is why `PAYLOAD_LEN` must be exact.

### The Device Info Packet (Reserved)

`WF_TYPE_DEVICE_INFO = 0x03U` is a reserved packet type. In v1, the C agent does not generate it and the Python bridge does not parse its payload — but it **must not reject it either**. A v1 decoder must validate the framing and CRC, note the type, and ignore the payload. This ensures future versions adding device-info packets do not break v1 decoders.

---

## 1.7 The CRC-16-CCITT Algorithm — The Math Explained

### Why CRC at All?

When bytes travel over a UART wire, electrical noise can flip bits. A `0` becomes a `1`, or vice versa. If this happens undetected, the decoder reads wrong values — wrong heap numbers, wrong task states. The CRC is a **mathematical fingerprint** of the packet that allows the receiver to detect whether the data was corrupted in transit.

### Which CRC Algorithm?

RTOSTwin uses **CRC-16-CCITT-FALSE** (also called **CRC-16/IBM-3740**). The exact parameters are:

| Parameter | Value | What It Means |
|---|---|---|
| Polynomial | `0x1021` | The mathematical "divisor" used in the calculation |
| Initial value | `0xFFFF` | The starting value of the CRC register |
| Input reflection | Disabled | Do NOT reverse the bit order of each input byte |
| Output reflection | Disabled | Do NOT reverse the bit order of the final result |
| Final XOR | None | Do NOT XOR the result with anything at the end |

### The Algorithm Step by Step

The CRC algorithm treats a sequence of bytes as if it were a very large binary number, and computes the remainder when divided by the polynomial `0x1021`. Here is the actual computation:

```python
def crc16_ccitt(data: bytes, init: int = 0xFFFF) -> int:
    crc = init                          # Start at 0xFFFF
    for byte in data:
        crc ^= byte << 8               # XOR the byte into the top of the 16-bit register
        for _ in range(8):             # Process each bit
            if crc & 0x8000:           # If the top bit is 1...
                crc = (crc << 1) ^ 0x1021   # Shift left and XOR with polynomial
            else:
                crc <<= 1              # Just shift left
            crc &= 0xFFFF              # Keep to 16 bits
    return crc
```

### The Reference Vector

There is one universally accepted test: the ASCII string `"123456789"` must produce `0x29B1`.

```
Input bytes: 0x31 0x32 0x33 0x34 0x35 0x36 0x37 0x38 0x39
Expected output: 0x29B1
```

This was **verified independently by computation** during the Phase 1 audit:
```
crc16_ccitt(b"123456789") = 0x29B1 ✅ MATCHES
```

### Coverage: What Gets Checksummed

A critical detail: **the two sync bytes (0xAA 0x55) are NOT included in the CRC input.** The CRC covers:

```
[VERSION] [TYPE] [SEQ_NUM(2)] [TIMESTAMP(4)] [PAYLOAD_LEN(2)] [PAYLOAD(N)]
  byte 2    3       4, 5          6-9             10, 11          12..11+N

Total = 10 header bytes (excluding sync) + N payload bytes
```

The CRC itself is appended as the final 2 bytes of the packet, little-endian.

### Why Exclude the Sync Bytes?

The sync bytes `0xAA 0x55` are used by the decoder to find the start of a packet in a raw byte stream. If the sync bytes were covered by the CRC, you would have a chicken-and-egg problem: you need the sync bytes to find the packet, but you haven't validated the CRC yet to know the sync bytes are real. Excluding them is the clean solution.

---

## 1.8 The Golden Packet Vectors — Mathematically Verified

The spec defines three **golden vectors** — byte-exact reference packets that any implementation must match. These are the integration sanity check: if your C framer produces these bytes and your Python decoder reads them back correctly, the two sides are compatible.

All three CRCs were **independently verified by computation** during the Phase 1 audit.

### Vector 1: Keyframe Packet

**Scenario:** Sequence number `0x1234`, timestamp `0x01020304`, 1 task named "IDLE", stack HWM 32 words, runtime 100 ticks, heap free 8000 bytes, heap min-ever 7680 bytes, CPU 7%.

**Header breakdown:**
```
aa           → SYNC_0 = 0xAA
55           → SYNC_1 = 0x55
01           → VERSION = 0x01
02           → TYPE = 0x02 (WF_TYPE_KEYFRAME)
34 12        → SEQ_NUM = 0x1234 (little-endian)
04 03 02 01  → TIMESTAMP_TICKS = 0x01020304 (little-endian)
28 00        → PAYLOAD_LEN = 0x0028 = 40 bytes (little-endian)
```

**Payload breakdown (40 bytes):**
```
34 12                  → sequence_num = 0x1234 (mirrors header)
04 03 02 01            → timestamp_ticks = 0x01020304 (mirrors header)
01                     → task_count = 1
49 44 4c 45            → name = "IDLE" (I=0x49, D=0x44, L=0x4C, E=0x45)
00 00 00 00            → name padding
00 00 00 00            → name padding
00 00 00 00            → name padding (total 16 bytes)
00                     → state = 0 (Running)
00                     → priority = 0
20 00                  → stack_hwm_words = 0x0020 = 32 words (little-endian)
64 00 00 00            → runtime_ticks = 0x64 = 100 (little-endian)
40 1f 00 00            → heap_free_bytes = 0x1F40 = 8000 (little-endian)
00 1e 00 00            → heap_min_ever_bytes = 0x1E00 = 7680 (little-endian)
07                     → cpu_utilization_pct = 7
```

**CRC (computed over bytes 2..51, i.e., the header minus sync + payload):**
```
CRC = 0x8596  →  wire bytes: 96 85
```

**Full packet (54 bytes):**
```
aa 55 01 02 34 12 04 03 02 01 28 00
34 12 04 03 02 01 01 49 44 4c 45 00 00 00 00 00 00 00 00 00 00 00 00 00
00 20 00 64 00 00 00 40 1f 00 00 00 1e 00 00 07
96 85
```

**Audit result:** CRC verified ✅ Payload length verified (40 = 0x28) ✅

---

### Vector 2: Delta Packet

**Scenario:** Sequence number `0x1235` (one after the keyframe), timestamp `0x01020368` (100 ticks later), only two system fields changed: heap_free_bytes = 7712 and cpu_utilization_pct = 10.

**Header breakdown:**
```
aa           → SYNC_0
55           → SYNC_1
01           → VERSION = 0x01
01           → TYPE = 0x01 (WF_TYPE_DELTA)
35 12        → SEQ_NUM = 0x1235 (little-endian) — one after keyframe
68 03 02 01  → TIMESTAMP_TICKS = 0x01020368 (little-endian) — later than keyframe
07 00        → PAYLOAD_LEN = 7 bytes
```

**Payload breakdown (7 bytes):**
```
f5           → tag = 0xF5 = 0xF0 | 0x05 → system field ID 0x05 (heap_free_bytes)
20 1e 00 00  → new value: 0x00001E20 = 7712 (little-endian uint32)
f7           → tag = 0xF7 = 0xF0 | 0x07 → system field ID 0x07 (cpu_utilization_pct)
0a           → new value: 10 (uint8)
```

**CRC (computed over header bytes 2..11 + payload bytes):**
```
CRC = 0x2D36  →  wire bytes: 36 2d
```

**Full packet (21 bytes):**
```
aa 55 01 01 35 12 68 03 02 01 07 00 f5 20 1e 00 00 f7 0a 36 2d
```

**Audit result:** CRC verified ✅ Payload length 7 verified ✅ Tag decoding verified ✅

---

### Vector 3: Corrupted Packet

**Scenario:** Same as the delta vector, but the final CRC byte is flipped from `2d` to `d2`.

**Full packet:**
```
aa 55 01 01 35 12 68 03 02 01 07 00 f5 20 1e 00 00 f7 0a 36 d2
```

**Expected decoder behaviour:** Compute CRC over the data, compare to the on-wire value `0xD236`. The computed value is `0x2D36`. They do not match. The packet is **discarded**, the `drop_count` is incremented, and no `DecodedPacket` is emitted. This tests that the decoder's error handling path works correctly.

---

## 1.9 The wire_format.h Header — The Code Artefact

`agent/core/wire_format.h` is the machine-readable version of the spec. Every constant defined here is referenced by:
- The C framer (`agent/core/framer.c`) — VNV owns this
- The C encoder (`agent/core/encoder.c`) — VNV owns this
- The Python decoder (`bridge/decoder.py`) — read as reference constants

This header is **the only place** these values are defined. There must never be a magic number `0x1021` floating around in any other file.

### Full Header Content After Phase 1

```c
#ifndef RTOSTWIN_WIRE_FORMAT_H
#define RTOSTWIN_WIRE_FORMAT_H

/**
 * @file wire_format.h
 * @note Canonical v1 protocol truth is frozen in docs/wire_format_spec.md.
 */

/* Synchronization bytes */
#define WF_SYNC_0               0xAAU   // First byte of every packet
#define WF_SYNC_1               0x55U   // Second byte of every packet

/* Protocol version */
#define WF_PROTOCOL_VERSION     0x01U   // Must match VERSION field in header byte 2

/* Packet types — written into header byte 3 */
#define WF_TYPE_DELTA           0x01U   // Only changed fields
#define WF_TYPE_KEYFRAME        0x02U   // Full snapshot
#define WF_TYPE_DEVICE_INFO     0x03U   // Reserved, not implemented in v1

/* Header and framing sizes */
#define WF_HEADER_SIZE          12U     // Bytes 0..11 inclusive
#define WF_CRC_SIZE             2U      // CRC is always 2 bytes
#define WF_OVERHEAD             (WF_HEADER_SIZE + WF_CRC_SIZE)  // = 14
#define WF_MAX_PACKET_SIZE      512U    // Maximum total packet size in bytes
#define WF_KEYFRAME_INTERVAL    50U     // Send a keyframe every 50 packets

/* CRC-16-CCITT parameters */
#define WF_CRC_POLY             0x1021U // The polynomial divisor
#define WF_CRC_INIT             0xFFFFU // Starting value for CRC register

/* Delta encoding field identifiers (used in tag byte construction) */
#define WF_DELTA_FIELD_TASK_STATE        0x01U  // Per-task field
#define WF_DELTA_FIELD_TASK_PRIORITY     0x02U  // Per-task field
#define WF_DELTA_FIELD_TASK_STACK_HWM    0x03U  // Per-task field
#define WF_DELTA_FIELD_TASK_RUNTIME      0x04U  // Per-task field
#define WF_DELTA_FIELD_HEAP_FREE         0x05U  // System field
#define WF_DELTA_FIELD_HEAP_MIN_EVER     0x06U  // System field
#define WF_DELTA_FIELD_CPU_UTILIZATION   0x07U  // System field

/* Reserved system-tag range */
#define WF_DELTA_SYSTEM_TAG_BASE         0xF0U  // System tag = 0xF0 | field_id

#endif /* RTOSTWIN_WIRE_FORMAT_H */
```

### What Changed From the Old Header

Before Phase 1, `wire_format.h` was missing the entire delta encoding section. It had no constants for field IDs. Any code that needed to build or parse delta tags would have had to hardcode `0x01`, `0x05`, `0xF0` as magic numbers.

After Phase 1, every constant needed for tag construction is named and documented. The comment was updated to point at `docs/wire_format_spec.md` instead of the retired `TECH_SPEC.md`.

---

## 1.10 The Sequence Ownership Decision

One of the most important **decisions** made in Phase 1 was resolving who is responsible for the `sequence_num` field in each packet.

### Why This Was Ambiguous

Before Phase 1, three documents disagreed:

| Document | What It Said |
|---|---|
| `PRD/TECH_SPEC.md` | The framer owns sequence numbering internally |
| `PRD/ARCHITECTURE.md` | Sequence number is passed in from outside (external ownership) |
| `agent/core/framer.h` | The function signature takes `seq_num` as a parameter |

If ownership is ambiguous, you get bugs. Either the framer increments the counter AND the caller passes in a counter, resulting in garbled sequence numbers. Or neither increments it and every packet has sequence 0.

### The v1 Decision

**In v1, `sequence_num` is snapshot-owned.**

This means:
- The v1 contract assigns `sequence_num` ownership to the snapshot side rather than to the framer
- The `full_snapshot_t` struct contains `sequence_num` as a field
- The framer receives it as a parameter (it does NOT own or increment it)
- This is documented in `docs/wire_format_spec.md` Section 2
- The implementation work to populate and increment it in `agent/core/snapshot.c` remains a Phase 2 task

This decision is recorded in `docs/wire_format_spec.md`:
> `sequence_num` is snapshot-owned in v1 and is written into header bytes 4-5.

### Why Snapshot Ownership Makes Sense

The snapshot captures the RTOS state at a specific point in time. The sequence number is a monotonically increasing counter that identifies that point in time. It belongs with the data, not with the packaging layer. The framer's job is to frame data, not to generate it.

> ⚠️ **Current Blocker (Phase 2):** While the spec says snapshot owns `sequence_num`, the implementation (`agent/core/snapshot.c`) does not yet populate it. This is a known issue that Phase 2 must fix.

---

## 1.11 What Was Frozen and Why It Matters

Phase 1 created a **protocol freeze**. This means the following are now fixed and cannot change without a protocol version bump (`WF_PROTOCOL_VERSION` from `0x01` to `0x02` and a migration guide):

| Frozen Element | Value | Where |
|---|---|---|
| Sync byte 0 | `0xAA` | `wire_format_spec.md` §4, `wire_format.h` |
| Sync byte 1 | `0x55` | `wire_format_spec.md` §4, `wire_format.h` |
| Protocol version | `0x01` | `wire_format_spec.md` §4, `wire_format.h` |
| TYPE field values | `0x01`, `0x02`, `0x03` | `wire_format_spec.md` §4, `wire_format.h` |
| Header layout | 12-byte, fixed order | `wire_format_spec.md` §4 |
| Endianness | All multi-byte = little-endian | `wire_format_spec.md` §1 |
| CRC algorithm | CRC-16-CCITT-FALSE | `wire_format_spec.md` §3, `wire_format.h` |
| CRC coverage | Bytes 2 through 11+N | `wire_format_spec.md` §3 |
| CRC reference vector | `"123456789"` → `0x29B1` | `wire_format_spec.md` §3 |
| Keyframe payload layout | Fixed field order, no padding | `wire_format_spec.md` §5.1 |
| Delta tag scheme | `(idx<<4)|fid` and `0xF0|fid` | `wire_format_spec.md` §5.2 |
| Delta field IDs | `0x01`–`0x07` | `wire_format_spec.md` §5.2, `wire_format.h` |
| Keyframe interval | 50 packets | `wire_format.h` |
| Sequence ownership | Snapshot-owned | `wire_format_spec.md` §2 |

### What "Frozen" Means in Practice

- VNV can now build the framer and encoder against `docs/wire_format_spec.md` with confidence
- Rayan can now build the Python decoder against `docs/wire_format_spec.md` with confidence
- Both sides will produce/consume identical bytes
- If a bug is found in the spec, fixing it requires incrementing `WF_PROTOCOL_VERSION`

Without Phase 1, any code written on either side would be a guess. Two guesses rarely agree.

---

## 1.12 What Comes Next — The Road Ahead

Phase 1 is done. Here is the full project roadmap and what each phase delivers:

### Phase 2 — Stabilize Snapshot and Profiler (PENDING)
**Files:** `agent/core/snapshot.*`, `agent/core/profiler.*`, `agent/freertos/hooks.c`

**Goal:** Make the MCU-side data capture clean and internally consistent.

**Known blockers to fix:**
- `snapshot.h` declares `sequence_num` but `snapshot.c` never sets it
- `vApplicationIdleHook()` is defined in BOTH `snapshot.c` AND `hooks.c` (duplicate symbol error at link time)
- `snapshot_capture()` WCET must be measured on the NUCLEO-F401RE (not assumed from spec)

**Handoff to VNV:** Frozen snapshot struct semantics, sequence number policy confirmed, CPU-util accounting ownership resolved.

### Phase 3 — Define the Typed Decoder Boundary (PENDING)
**Files:** `bridge/decoder.py`

**Goal:** Convert the raw byte stream from the device into structured Python objects.

**Known blocker:** `bridge/decoder.py` currently returns a `DecodedPacket` that still carries raw payload bytes instead of fully decoded typed fields. `bridge/state_manager.py` expects fields like `heap_free_bytes`, `task_deltas`, and `timestamp_ticks`. Phase 3 must complete that typed decoder boundary.

**Handoff to VNV:** Exact `DecodedPacket` schema, field names, one decoded keyframe example, one decoded delta example.

### Phase 4 — Fix the OOM Analyzer Contract (PENDING)
**Files:** `bridge/oom_analyzer.py`

**Known blocker:** `bridge/oom_analyzer.py` does not match the API tested in `bridge/tests/test_oom_analyzer.py`.

**Deliverable:** Two-detector system (linear regression + rolling minimum) that detects memory leaks and projects time to OOM.

### Phase 5 — Baseline Integration Review (PENDING)
**Goal:** Review integration without touching VNV-owned files. File precise mismatch notes.

### Phase 6 — ESP32-P4 and Teensy 4.1 Expansion (PENDING)
**Goal:** Port to second and third hardware targets. Only after the STM32 baseline is proven end-to-end.

### Definition of Done for All of Rayan's Work

| Item | Status |
|---|---|
| `docs/wire_format_spec.md` exists | ✅ Complete |
| `agent/core/wire_format.h` matches written spec | ✅ Complete |
| `snapshot_capture()` WCET < 150 µs (measured on real hardware) | 🔄 Phase 2 |
| Zero `pvPortMalloc` inside agent functions (verified by mock) | 🔄 Phase 2 |
| Only one `vApplicationIdleHook()` implementation | 🔄 Phase 2 |
| `DecodedPacket` is typed and stable | 🔄 Phase 3 |
| `OOMAnalyzer` matches tests and spec | 🔄 Phase 4 |
| VNV can consume all outputs without guessing | 🔄 Phase 5 |

---

---

# Section 2 — Explain It Like I'm a Noob

*Everything about this project explained with plain language and fun analogies. No engineering degree required.*

---

## 2.1 What Is RTOSTwin In Plain English?

Imagine you have a tiny computer — about the size of a credit card — running in an industrial machine, a medical device, or a robot. This tiny computer (called a **microcontroller** or **MCU**) is running multiple programs at the same time. Each program is called a **task**. A piece of software called an **RTOS** (Real-Time Operating System) is responsible for managing all those tasks — deciding who gets to run, when they pause, and how much memory they get.

Now imagine this device ships to a factory floor in another city. A week later, the device starts behaving strangely. The engineers want to ask: "What is the device doing right now? Is it running low on memory? Is one task hogging all the CPU time?"

**But they can't.** The device is in another city. It has no screen. It's running in a sealed box. The traditional diagnostic tools require physically connecting a cable to the device — and that cable is obviously not there.

**RTOSTwin solves this.** It is a small program that runs *alongside* the RTOS on the device. Every 100 milliseconds, it:
1. Reads all the health data from the RTOS (how much memory is left, what each task is doing, etc.)
2. Packages that data into a compact message
3. Sends it over a serial cable (or Wi-Fi) to a nearby computer

That nearby computer (running the **Python bridge**) decodes the messages and feeds the data into **Grafana** — the same beautiful dashboard software that big tech companies use to monitor their servers.

The end result: an engineer can open a Grafana dashboard and see, in real time, exactly what is happening inside a device that is physically across the world.

---

## 2.2 The Problem We Are Solving

Think of it this way. You have a car. Modern cars have an OBD-II port. You can plug in a device, read engine codes, see the RPM, check the oil pressure, all while the car is running. If something breaks, you don't have to take the engine apart to guess what went wrong — you just read the data.

Microcontrollers running FreeRTOS are like cars that don't have an OBD-II port. The internal health data *exists* — the RTOS knows everything about every task — but there is no standard, open way to read it remotely.

Commercial tools exist (think of them like expensive OBD-II scanner apps that only work with their own cloud and charge per vehicle). RTOSTwin is the free, open-source, "plug it into any Grafana" version.

---

## 2.3 The Two Halves of the System

RTOSTwin has two halves that talk to each other:

### Half 1: The Tiny Spy (runs on the microcontroller)

Think of this as a very quiet spy living inside the device. Every 100 milliseconds, the spy:
1. **Looks around** (reads RTOS state — task status, memory levels, CPU usage)
2. **Takes notes** (writes everything into a struct in C)
3. **Compresses the notes** (only writes down what *changed* since last time)
4. **Folds the notes into an envelope** (adds a header, a stamp, and a checksum)
5. **Slides the envelope under the door** (sends it over UART/USB/Wi-Fi)

### Half 2: The Intelligence Hub (runs on the host computer)

Think of this as a detective office that receives envelopes from the spy:
1. **Opens the envelope and checks the stamp** (validates the CRC checksum)
2. **Reads the notes** (decodes the binary data into Python objects)
3. **Rebuilds the full picture** (combines delta packets with the last keyframe)
4. **Posts it on a big board** (exports to Prometheus/Grafana)
5. **Watches for trouble** (OOM analyzer checks if memory is slowly running out)

---

## 2.4 What Is a Wire Format Spec?

Imagine two people in different countries trying to exchange secret letters. They agree on a code:
- "The first word is always the secret greeting"
- "The second word is the message type"
- "The last three letters are the verification code"

Without agreeing on this code *before* writing any letters, the sender might put the greeting at the end, and the receiver would not understand a thing.

A **Wire Format Spec** is exactly this agreement, but for computers. It says:
- "The first byte is always `0xAA`"
- "The second byte is always `0x55`"
- "The 4th byte tells you what kind of message this is"
- "Bytes 4 and 5 are the message number, written smallest-byte-first"
- ...and so on for every byte

The file `docs/wire_format_spec.md` is the agreement between the C code on the microcontroller and the Python code on the host computer. Without it, the C code could put the data in any order, and the Python code would not be able to decode it.

**Why `0xAA` and `0x55`?** These two bytes are the "knock knock" at the start of every message. The receiving computer scans through all incoming bytes looking for the pattern `0xAA 0x55`. When it sees that pattern, it knows: "a new packet is starting here." These are called **sync bytes** or the **frame delimiter**.

In binary, `0xAA = 10101010` and `0x55 = 01010101` — they alternate perfectly, making them visually distinctive and easy to distinguish from data bytes. Smart design.

---

## 2.5 What Is a CRC-16 Checksum?

Imagine you ship a package and include a slip of paper that says: "The package weighs 2.3 kg." When the receiver gets it, they weigh the package. If the scale says 2.3 kg, great — nothing fell out. If it says 1.8 kg, something is missing.

A **CRC (Cyclic Redundancy Check)** is the same idea, but for bytes. Before sending a packet, the C code:
1. Takes all the bytes in the packet (except the sync bytes at the start)
2. Runs a specific mathematical calculation on them
3. Gets a 2-byte number (the CRC)
4. Appends those 2 bytes to the end of the packet

When the Python bridge receives the packet:
1. It takes all the same bytes
2. Runs the same mathematical calculation
3. Gets its own 2-byte number
4. Compares it to the 2 bytes at the end of the packet

If they match: ✅ "The package arrived intact."  
If they don't match: ❌ "Something got corrupted in transit — throw this packet away and count a drop."

The specific variant used here is called **CRC-16-CCITT**. The "16" means the result is 16 bits (2 bytes). The "CCITT" is the international standards body that defined this version. There are many CRC variants; each uses a different "polynomial" (the mathematical divisor). RTOSTwin uses polynomial `0x1021`.

**The magic number that proves your implementation is correct:** If you run CRC-16-CCITT on the nine bytes `"123456789"`, the answer must always be `0x29B1`. This is a famous test vector that every CRC-16-CCITT implementation agrees on. We verified it:
```
crc16_ccitt(b"123456789") = 0x29B1 ✅
```

---

## 2.6 What Is Delta Encoding?

Think about how traffic reporters work. Instead of describing every single car on every road every minute, they say: "Update: one new accident on the M6, lane 2. Everything else unchanged."

That "everything else unchanged" is delta encoding. You only transmit what **changed** since the last transmission.

In RTOSTwin, a full snapshot of all task data is about 350 bytes. That's a lot to send 10 times per second over a slow serial cable. At 115,200 baud, the cable can only carry about 11,520 bytes per second. Sending 3,500 bytes per second of snapshots would use 30% of the cable — leaving very little for anything else.

With delta encoding:
- The first packet (a **keyframe**) sends everything — all 350 bytes. This is the "baseline."
- Every subsequent packet (a **delta**) only sends what changed. If only the heap size and CPU% changed, that delta might be just 7 bytes.
- Every 50 packets, we force another full keyframe as a safety reset.

Typical bandwidth with delta encoding: **80–200 bytes per second** — less than 2% of the cable capacity. Delta encoding is not an optional optimization; it is what makes the system viable on standard UART.

**The clever part: the tag byte.** Each changed field in a delta packet is described by a **tag byte** followed by the new value. The tag byte has two pieces of information baked into it:
- **The top 4 bits (the nibble):** which task, or a special code for system-wide fields
- **The bottom 4 bits:** which specific field of that task changed

Example: `0xF5` means: top nibble = `F` (system field), bottom nibble = `5` (heap_free_bytes). The four bytes that follow are the new heap free value. Total: 5 bytes to update the heap reading.

---

## 2.7 What Is a Keyframe?

Think about how a video file works. Instead of storing every single pixel of every single frame, video compression picks certain **keyframes** (also called I-frames) that store the complete picture. Between keyframes, only the differences from the previous frame are stored. If you jump to the middle of a video, you need the nearest keyframe to reconstruct the full picture.

RTOSTwin uses the exact same concept:
- A **keyframe** contains the complete state of all tasks, heap, and CPU metrics. It is the "I-frame" of the telemetry stream.
- A **delta packet** only contains what changed since the last packet. It is the "P-frame" or "B-frame."

**Why do we need keyframes?** What happens if you connect the Python bridge to a device that has already been running for 10 minutes? Without a keyframe, you only receive delta packets, and you don't know the full picture to apply those deltas to. With periodic keyframes (every 50 packets = every 5 seconds at 10 Hz), the bridge at most has to wait 5 seconds to get a complete baseline.

**Also:** if a packet gets corrupted and dropped, the delta chain is broken. A keyframe resets everything to a known good state.

---

## 2.8 What Did Phase 1 Actually Accomplish?

Phase 1 was purely about **establishing the contract before wider integration continued.** Here is what was built:

### What Was Created

**1. `docs/wire_format_spec.md` — The Rule Book**

A 118-line document that defines every single byte of every single packet. Before this document existed, the framer code and the decoder code were working from different assumptions. After this document, they have one truth to both follow.

**2. `agent/core/wire_format.h` — The Translated Rule Book**

The same rules, but translated into C `#define` constants. Instead of `0x1021`, you write `WF_CRC_POLY`. Instead of `0xF0`, you write `WF_DELTA_SYSTEM_TAG_BASE`. This prevents magic numbers scattered across the codebase.

**3. Three Golden Packets — The Proof**

Three byte-exact example packets were computed and written into the spec:
- A complete keyframe (54 bytes) with CRC `0x8596` ✅ verified
- A delta packet (21 bytes) with CRC `0x2D36` ✅ verified
- A corrupted packet (same delta, wrong CRC) ✅ for testing error handling

**4. The Sequence Ownership Decision**

A formal decision was made and documented: `sequence_num` is owned by the snapshot engine (RYN), not the framer (VNV). This prevents a class of bugs where two components both try to manage the same counter.

### What Was NOT Done in Phase 1

Phase 1 deliberately avoided touching:
- Any C source file (`.c` files)
- Any Python source file (`.py` files)
- Any implementation that could cause bugs

Phase 1 was **specification only** for this branch — documentation and constants. The goal was to establish the truth before wider framer, snapshot, and bridge integration continued.

---

## 2.9 Why Did We Do Phase 1 Before Writing Any Real Code?

This is one of the most important lessons in this project, and it comes up in every embedded systems interview.

**The horror story without Phase 1:**

1. RYN writes a Python decoder assuming the packet looks like: `[sync] [type] [seq] [payload] [crc]`
2. VNV writes a C framer assuming the packet looks like: `[sync] [seq] [type] [payload] [crc]`
3. Both engineers spend 3 weeks writing 500 lines of code each
4. Integration day: nothing works. Every packet produces garbage output.
5. Now you have to figure out which of 1000 lines of code is wrong and fix both sides

**The story with Phase 1:**

1. RYN writes `docs/wire_format_spec.md` and both engineers agree on it
2. Golden vectors are computed and verified by hand
3. VNV writes the C framer against the spec. It produces byte-exact matches for the golden vectors.
4. RYN writes the Python decoder against the spec. It decodes the golden vectors correctly.
5. Integration day: it works on the first try.

**The principle:** In embedded systems, and especially in protocol design, **getting the contract right is more valuable than writing the code fast.** Code that implements the wrong contract is useless code. Code that implements a correct, shared contract is immediately useful.

This is also why the checklist says: "Do not trust drift between docs and code without resolving it explicitly."

---

---

# Section 3 — The Translation Map

*Every analogy from Section 2, mapped to the exact technical term, file, and C/Python code it corresponds to.*

---

## 3.1 Analogies → Technical Terms Master Table

| Analogy | Technical Term | File(s) | How It Works in Code |
|---|---|---|---|
| The Tiny Spy reading RTOS state | `snapshot_capture()` | `agent/core/snapshot.c` | Calls `uxTaskGetSystemState()`, `xPortGetFreeHeapSize()`, etc. Writes into `full_snapshot_t` struct |
| The notes the spy takes | `full_snapshot_t` struct | `agent/core/snapshot.h` | A C struct with fields: `sequence_num`, `timestamp_ticks`, `task_count`, `tasks[MAX_TASKS]`, `memory` |
| Compressing the notes to "only what changed" | Delta encoding | `agent/core/encoder.c` | Iterates all fields, compares current vs previous snapshot, emits tag+value pairs for changed fields |
| Folding notes into an envelope with a header | Packet framing | `agent/core/framer.c` | Writes SYNC(2)+VER(1)+TYPE(1)+SEQ(2)+TS(4)+LEN(2)+PAYLOAD(N)+CRC(2) |
| The knock-knock at the start of every envelope | Sync bytes `0xAA 0x55` | `wire_format.h: WF_SYNC_0, WF_SYNC_1` | `#define WF_SYNC_0 0xAAU` and `#define WF_SYNC_1 0x55U` |
| The verification stamp on the envelope | CRC-16-CCITT checksum | `wire_format.h: WF_CRC_POLY, WF_CRC_INIT` | 2-byte result of the polynomial division algorithm, appended to packet end |
| The magic password "123456789" → "29B1" | CRC reference vector | `docs/wire_format_spec.md §3` | `crc16_ccitt(b"123456789") == 0x29B1` — proves the algorithm is correct |
| The slide-under-the-door delivery | DMA UART/USB/UDP transmission | `agent/core/transport.c`, `agent/hal/stm32/uart_dma.c` | `HAL_UART_Transmit_DMA()` — CPU hands off to hardware, returns immediately |
| The detective office that opens envelopes | Python bridge decoder | `bridge/decoder.py` | `PacketDecoder.feed_bytes()` state machine — finds sync, validates CRC, unpacks fields |
| The big board where everything is posted | Grafana dashboard | `dashboard/rtostwin_dashboard.json` | PromQL queries against the Prometheus endpoint served by `bridge/prometheus_exporter.py` |
| The OBD-II port we are building | The entire RTOSTwin system | All files | The complete pipeline from RTOS → packet → Grafana |
| The rule book for exchanges | Wire format specification | `docs/wire_format_spec.md` | Markdown document defining every byte, field, size, encoding, and algorithm |
| The translated rule book (for computers) | `wire_format.h` constants | `agent/core/wire_format.h` | C `#define` constants that must match the spec byte-for-byte |
| A full traffic report (all roads, all cars) | Keyframe packet | `WF_TYPE_KEYFRAME = 0x02U` | Payload contains every task, heap, CPU metric — full snapshot |
| A traffic update ("accident on M6, all else unchanged") | Delta packet | `WF_TYPE_DELTA = 0x01U` | Payload contains only `[tag][value]` entries for changed fields |
| The video I-frame (baseline) | Keyframe | Every 50 packets (`WF_KEYFRAME_INTERVAL = 50U`) | Forces a full snapshot so the bridge can re-sync after corruption |
| The "what channel changed" part of a delta | Tag byte high nibble | `WF_DELTA_SYSTEM_TAG_BASE = 0xF0U` | High nibble = task index (0-14) or 0xF for system; low nibble = field ID |
| Memory slowly draining from the sink | Heap memory leak | `bridge/oom_analyzer.py` | Linear regression on `heap_free_bytes` sliding window; negative slope = leak |
| The detective projecting the crime will happen | OOM projection | `OOMAnalyzer.get_projection_seconds()` | `heap_current / abs(slope_bytes_per_second)` = seconds until heap = 0 |
| Measuring how long the spy takes to look around | WCET profiling | `agent/core/profiler.c` | ARM DWT cycle counter: `DWT->CYCCNT` measured before/after `snapshot_capture()` |

---

## 3.2 The Train Station Analogy — Full Breakdown

Think of RTOSTwin as a train system:

```
[Train = Packet]
[Locomotive = Framer: adds engine, wheels, brakes]
[Cargo = Payload: the actual task/heap data]
[Ticket = Sequence Number: packet #1234]
[Departure time = Timestamp: RTOS tick count]
[Customs seal = CRC: mathematical proof nothing was opened in transit]
[Destination board = Sync bytes: shows where the train starts]
[Train tracks = UART/USB/UDP transport]
[Arrival station = Python decoder]
[Cargo manifest = wire_format_spec.md: says exactly what goes where]
[Station rules = wire_format.h constants: the rules as machine-readable code]
```

**The "Rules of the Train" = `docs/wire_format_spec.md`**  
This document says: "Every train must have a destination board at the front (sync bytes), then a ticket (seq_num), then cargo (payload), then a customs seal (CRC) at the back."

**The "Station Rulebook Printed as Laminated Cards" = `agent/core/wire_format.h`**  
The same rules, but as quick-reference cards that any engineer (or code file) can look up instantly. `WF_SYNC_0 = 0xAAU` is the card that says "the front of every train is painted hexadecimal AA."

**The "Keyframe Train" = Full double-decker freight train**  
It carries everything: every task's state, all memory stats, CPU usage. Heavy but complete. Runs every 50 trains.

**The "Delta Train" = A lightweight courier van**  
Only carries what's different from the last delivery. Might be just two changes. Much smaller, much faster. Runs 49 out of every 50 times.

**The "Corrupted Packet" = A train that arrived with broken seals**  
The customs seal (CRC) doesn't match. The receiving station rejects the cargo, logs a drop, and waits for the next train. The agent knows how many trains got lost because the sequence numbers have a gap.

---

## 3.3 How the Wire Format Translates Into C

Here is a real-world translation: the first 12 bytes of a keyframe packet, shown three ways simultaneously:

```
   Wire (bytes on cable)       Meaning                    C constant (from wire_format.h)
   ─────────────────────────────────────────────────────────────────────────────────────────
   0xAA                         Sync byte 0               WF_SYNC_0
   0x55                         Sync byte 1               WF_SYNC_1
   0x01                         Protocol version 1        WF_PROTOCOL_VERSION
   0x02                         Packet type: keyframe     WF_TYPE_KEYFRAME
   0x34                         Seq_num low byte (0x1234) — little-endian
   0x12                         Seq_num high byte
   0x04                         Timestamp byte 0 (0x01020304) — little-endian
   0x03                         Timestamp byte 1
   0x02                         Timestamp byte 2
   0x01                         Timestamp byte 3
   0x28                         Payload length low byte (0x0028 = 40)
   0x00                         Payload length high byte
   ─────────────────────────────────────────────────────────────────────────────────────────
```

**In C, the framer writes these bytes like this:**
```c
// Writing the header (conceptually — actual framer code may use memcpy or struct packing)
out_buf[0] = WF_SYNC_0;                                 // 0xAA
out_buf[1] = WF_SYNC_1;                                 // 0x55
out_buf[2] = WF_PROTOCOL_VERSION;                       // 0x01
out_buf[3] = packet_type;                               // e.g., WF_TYPE_KEYFRAME = 0x02
out_buf[4] = (uint8_t)(sequence_num & 0xFF);            // Low byte of seq_num
out_buf[5] = (uint8_t)(sequence_num >> 8);              // High byte of seq_num
out_buf[6] = (uint8_t)(timestamp_ticks & 0xFF);         // Byte 0 of timestamp (little-endian)
out_buf[7] = (uint8_t)((timestamp_ticks >> 8) & 0xFF);  // Byte 1
out_buf[8] = (uint8_t)((timestamp_ticks >> 16) & 0xFF); // Byte 2
out_buf[9] = (uint8_t)((timestamp_ticks >> 24) & 0xFF); // Byte 3
out_buf[10] = (uint8_t)(payload_len & 0xFF);            // Low byte of payload length
out_buf[11] = (uint8_t)(payload_len >> 8);              // High byte of payload length
```

**In Python, the decoder reads them back:**
```python
# Reading the same header
sync0  = data[0]   # Should be 0xAA
sync1  = data[1]   # Should be 0x55
ver    = data[2]   # Protocol version
ptype  = data[3]   # Packet type
seq    = data[4] | (data[5] << 8)                             # Reconstruct uint16 LE
ts     = data[6] | (data[7]<<8) | (data[8]<<16) | (data[9]<<24)  # Reconstruct uint32 LE
plen   = data[10] | (data[11] << 8)                           # Payload length
```

Or with Python's `struct` module:
```python
import struct
# Unpack 12-byte header: 2 sync + 2 uint8 + uint16_LE + uint32_LE + uint16_LE
sync0, sync1, ver, ptype, seq, ts, plen = struct.unpack_from('<BBBBHIH', data, 0)
```

---

## 3.4 How the CRC Translates Into Bytes

The CRC function, shown completely:

```python
def crc16_ccitt(data: bytes, init: int = 0xFFFF) -> int:
    """
    CRC-16-CCITT-FALSE
    Poly=0x1021, Init=0xFFFF, RefIn=False, RefOut=False, XorOut=0x0000
    
    Constant names from wire_format.h:
        WF_CRC_POLY = 0x1021
        WF_CRC_INIT = 0xFFFF
    """
    crc = init                           # Start at WF_CRC_INIT = 0xFFFF
    for byte in data:
        crc ^= (byte << 8)               # XOR incoming byte into top 8 bits
        for _ in range(8):               # Process bit by bit
            if crc & 0x8000:             # If bit 15 is set...
                crc = (crc << 1) ^ 0x1021  # Shift left, XOR with WF_CRC_POLY
            else:
                crc = crc << 1           # Just shift left
            crc &= 0xFFFF                # Mask to 16 bits
    return crc
```

**What data goes into the CRC function for the delta vector?**

```
Input = header bytes [2..11] + all payload bytes
      = [01 01 35 12 68 03 02 01 07 00] + [f5 20 1e 00 00 f7 0a]
      = 17 bytes total

Output = 0x2D36

How it appears on wire (little-endian):
  Low byte first:  0x36
  High byte second: 0x2D
  → Wire bytes: 36 2d ✅
```

**The equivalent C code:**
```c
/* From agent/core (implemented by VNV in framer.c, spec defined by RYN) */
uint16_t compute_crc(const uint8_t *data, uint16_t len) {
    uint16_t crc = WF_CRC_INIT;            // 0xFFFF
    for (uint16_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (int j = 0; j < 8; j++) {
            if (crc & 0x8000) {
                crc = (uint16_t)((crc << 1) ^ WF_CRC_POLY);  // 0x1021
            } else {
                crc = (uint16_t)(crc << 1);
            }
        }
    }
    return crc;
}

/* Writing CRC at end of packet (little-endian) */
out_buf[header_size + payload_len]     = (uint8_t)(crc & 0xFF);   // Low byte
out_buf[header_size + payload_len + 1] = (uint8_t)(crc >> 8);     // High byte
```

---

## 3.5 How Delta Encoding Translates Into Tags and Bytes

### Building a Delta Tag — Per-Task Example

**Scenario:** Task index 2, field "priority" changed to 5.

```
field_id = WF_DELTA_FIELD_TASK_PRIORITY = 0x02U
task_index = 2

tag = (task_index << 4) | field_id
    = (2 << 4) | 0x02
    = 0x20 | 0x02
    = 0x22

Wire bytes: [0x22][0x05]
             tag   new value (1 byte, uint8)
```

### Building a Delta Tag — System Example

**Scenario:** heap_free_bytes changed to 7712 bytes (0x1E20).

```
field_id = WF_DELTA_FIELD_HEAP_FREE = 0x05U
tag = WF_DELTA_SYSTEM_TAG_BASE | field_id
    = 0xF0U | 0x05U
    = 0xF5

Value = 7712 = 0x00001E20 (little-endian uint32)
Wire bytes: [f5][20][1e][00][00]
             tag  byte0 byte1 byte2 byte3
```

This is exactly what appears in Vector 2 (the delta golden packet): `f5 20 1e 00 00`.

### Decoding a Delta in Python

```python
i = 0
while i < payload_len:
    tag = payload[i]
    i += 1
    
    high_nibble = tag >> 4     # Top 4 bits
    low_nibble  = tag & 0x0F   # Bottom 4 bits (field_id)
    
    if high_nibble == 0xF:
        # System field
        field_id = low_nibble
        if field_id == 0x05:   # WF_DELTA_FIELD_HEAP_FREE
            value = struct.unpack_from('<I', payload, i)[0]  # uint32 LE
            i += 4
            decoded.heap_free_bytes = value
        elif field_id == 0x07: # WF_DELTA_FIELD_CPU_UTILIZATION
            value = payload[i]  # uint8
            i += 1
            decoded.cpu_utilization_pct = value
        # ... handle 0x06 similarly
    else:
        # Per-task field
        task_index = high_nibble
        field_id   = low_nibble
        if field_id == 0x01:   # WF_DELTA_FIELD_TASK_STATE
            value = payload[i]; i += 1   # uint8
        elif field_id == 0x02: # WF_DELTA_FIELD_TASK_PRIORITY
            value = payload[i]; i += 1   # uint8
        elif field_id == 0x03: # WF_DELTA_FIELD_TASK_STACK_HWM
            value = struct.unpack_from('<H', payload, i)[0]; i += 2   # uint16 LE
        elif field_id == 0x04: # WF_DELTA_FIELD_TASK_RUNTIME
            value = struct.unpack_from('<I', payload, i)[0]; i += 4   # uint32 LE
```

---

## 3.6 File-by-File Role Map

Every file in the project, explained in one line, with its owner and phase:

| File | Owner | Phase | Plain-English Role |
|---|---|---|---|
| `docs/wire_format_spec.md` | RYN | ✅ Phase 1 | The rule book: every byte of every packet defined |
| `agent/core/wire_format.h` | RYN | ✅ Phase 1 | The rule book as C code constants |
| `agent/core/snapshot.h` | RYN | 🔄 Phase 2 | Defines the structs that hold RTOS state data |
| `agent/core/snapshot.c` | RYN | 🔄 Phase 2 | Actually reads the RTOS state into those structs |
| `agent/core/profiler.h/.c` | RYN | 🔄 Phase 2 | Times how long snapshot_capture() takes |
| `bridge/decoder.py` | RYN | 🔄 Phase 3 | Turns raw bytes back into Python objects |
| `bridge/oom_analyzer.py` | RYN | 🔄 Phase 4 | Detects memory leaks and predicts time to crash |
| `agent/core/framer.c` | VNV | Later | Wraps encoded bytes in a packet with header+CRC |
| `agent/core/encoder.c` | VNV | Later | Compresses snapshot into delta or keyframe bytes |
| `agent/core/transport.c` | VNV | Later | Sends packets via DMA without blocking the CPU |
| `agent/hal/stm32/uart_dma.c` | VNV | Later | STM32-specific DMA UART driver |
| `bridge/state_manager.py` | VNV | Later | Rebuilds full device picture from keyframes+deltas |
| `bridge/prometheus_exporter.py` | VNV | Later | Serves the `/metrics` page Prometheus scrapes |
| `bridge/otlp_exporter.py` | VNV | Later | Pushes metrics to Grafana/Datadog/etc. via OTLP |
| `bridge/mock_device.py` | VNV | Later | Simulates a real device for testing without hardware |
| `dashboard/rtostwin_dashboard.json` | VNV | Later | One-click Grafana dashboard showing all RTOS metrics |
| `docs/reports/RTOSTwin_Complete_Report.md` | Lead | Arch | The 800-line project bible — market analysis, architecture, timelines |
| `roles/rayan_checklist.md` | RYN | Working | RYN's personal task tracker and phase plan |
| `roles/ryn_role_assignment.md` | Lead | Arch | Formal definition of RYN's role, deliverables, timelines |
| `PRD/ARCHITECTURE.md` | Lead | Arch | Technical system architecture diagram and module boundaries |
| `PRD/TECH_SPEC.md` | Lead | Arch | Technical specification for all data structures and APIs |

---

## 3.7 Interview Cheat Sheet

The following questions are commonly asked in embedded systems and protocol design interviews. Here are precise answers drawn directly from this project:

---

**Q: What is a wire format specification and why is it important?**

A: A wire format spec defines the exact byte-level layout of every packet in a binary protocol. It specifies field names, byte offsets, sizes, endianness, encoding rules, and a CRC algorithm. Without a frozen spec, the encoder and decoder can drift apart — both sides implement a slightly different interpretation of the protocol, and integration fails. In RTOSTwin, `docs/wire_format_spec.md` is the single source of truth for the v1 protocol, frozen before wider implementation and integration work continued.

---

**Q: What is CRC-16-CCITT and how does it work?**

A: CRC-16-CCITT is an error-detection algorithm that produces a 2-byte (16-bit) checksum from an arbitrary byte sequence. The algorithm processes data one byte at a time, XOR-ing each byte into a 16-bit register and performing bit-by-bit polynomial division using the polynomial `0x1021`. The initial register value is `0xFFFF`. There is no bit reflection and no final XOR. The canonical test vector is `crc16_ccitt(b"123456789") == 0x29B1`. RTOSTwin places the CRC as the final 2 bytes of every packet, covers all bytes from VERSION through the end of the payload (excluding sync bytes), and appends the result in little-endian byte order.

---

**Q: What is delta encoding and why is it necessary in this project?**

A: Delta encoding is a compression technique where only changed values are transmitted rather than the full data set. In RTOSTwin, a full snapshot of 8 tasks is approximately 350 bytes. At 10 Hz over 115200-baud UART, that is 3500 bytes/second — 30% of channel capacity. With delta encoding, only changed fields are transmitted. Typical delta packets are 7–50 bytes, reducing bandwidth to under 2% of channel capacity. Every 50 packets, a full keyframe is forced to allow decoders to re-synchronize. Delta encoding is not optional — without it, the system would saturate the UART channel.

---

**Q: Why should there be no dynamic memory allocation in the agent hot path?**

A: In FreeRTOS, `pvPortMalloc()` acquires the heap mutex. If `snapshot_capture()` is called from the telemetry task while holding a critical section (which disables interrupts), and it then calls `pvPortMalloc()`, a priority inversion can occur: the heap mutex may be held by a higher-priority task that is now blocked, but interrupts are disabled so that task cannot run to release it. This causes a deadlock. The solution is to use `static TaskStatus_t s_task_status_buf[MAX_TASKS]` at file scope — allocated at compile time, never at runtime. All agent hot-path buffers must be statically allocated.

---

**Q: What is a keyframe and what is a delta packet?**

A: A keyframe (TYPE `0x02`) is a complete snapshot of all RTOS metrics — task states, priorities, stack watermarks, runtime counters, heap levels, and CPU utilization. It allows any new decoder (or a decoder that reconnected after a dropout) to reconstruct the full device state from a single packet. A delta packet (TYPE `0x01`) contains only the fields that changed since the previous packet, encoded as `[tag][value]` pairs. The tag byte encodes the task index (or system-field indicator) and the field ID. Keyframes are sent every 50 packets (5 seconds at 10 Hz) to bound the maximum re-synchronization time.

---

**Q: What is little-endian byte order?**

A: Multi-byte integers can be stored or transmitted with the least significant byte first (little-endian) or most significant byte first (big-endian). Little-endian is used by ARM Cortex-M (the STM32 and Teensy processors) and by most PC architectures (x86). In RTOSTwin, all multi-byte fields on the wire are little-endian. For example, the 16-bit sequence number `0x1234` is transmitted as the byte `0x34` followed by `0x12`. This matches the native representation on ARM, making field-by-field serialization straightforward without byte-swapping even though the packet still requires explicit framing and layout rules.

---

**Q: What does "WCET" mean and why does it matter for embedded systems?**

A: WCET stands for Worst-Case Execution Time. In a real-time system, tasks have deadlines — they must complete within a fixed time window. If `snapshot_capture()` sometimes takes 200 microseconds instead of its target 150 microseconds, it could cause a higher-priority task to miss its deadline, leading to system instability. WCET is measured by running the function 10,000 times under worst-case conditions (all tasks running, queues full) and recording the maximum time using the ARM DWT cycle counter (`DWT->CYCCNT`). The target is < 150 µs on the NUCLEO-F401RE (STM32F401RE at 84 MHz).

---

**Q: What is the purpose of the sequence number in the packet header?**

A: The sequence number is a monotonically incrementing counter that uniquely identifies each packet. The Python decoder already tracks the sequence number of each received packet. If packet #1234 is followed immediately by packet #1236, the decoder knows packet #1235 was lost (corrupted and discarded, or dropped by the agent's DMA transport), and it increments `sequence_gap_count`. Downstream bridge/exporter layers can turn that signal into packet-loss metrics. In RTOSTwin v1, sequence ownership is assigned to the snapshot side of the contract: it lives in the `full_snapshot_t` struct and is passed to the framer as an argument. Making `agent/core/snapshot.c` populate and increment it for real remains Phase 2 work.

---

**Q: Why are `0xAA` and `0x55` used as sync bytes?**

A: `0xAA = 10101010` in binary and `0x55 = 01010101` in binary. They alternate perfectly between 0 and 1 bits. This makes them visually distinctive and easy to identify in hex dumps. More importantly, this alternating pattern is unlikely to appear naturally as part of real payload data, reducing false sync detections. The decoder scans the byte stream for the pattern `0xAA 0x55` to locate the start of each packet, which is how it re-synchronizes after a corrupt packet or a mid-stream connection.

---

**Q: What is the OOM analyzer and how does it work?**

A: The OOM (Out Of Memory) analyzer is a Python module (`bridge/oom_analyzer.py`) intended to predict when the device will run out of heap memory. The Phase 4 target design uses two complementary detectors: (1) **Linear regression** on a sliding window of `heap_free_bytes` samples — if the slope is consistently negative with high R² (≥ 0.7), a steady leak is detected and time-to-OOM is projected as `current_heap / abs(slope)` seconds. (2) **Rolling minimum comparison** — if the minimum heap value in the window has dropped by more than 10% of total heap, a secondary alert fires even if linear regression is fooled by a sawtooth allocation pattern. Both detectors together catch monotonic leaks and bursty leaks. Exporting an OOM projection metric into Prometheus/Grafana remains part of that later integration work, not a completed Phase 1 capability.

---

*End of Project_Journey_So_Far.md*

---

**Document metadata:**

| Field | Value |
|---|---|
| File | `docs/reports/Project_Journey_So_Far.md` |
| Written by | Claude (acting as Embedded Systems Tutor) on request of Mohammed Rayan |
| Based on | `docs/wire_format_spec.md`, `docs/reports/RTOSTwin_Complete_Report.md`, `agent/core/wire_format.h`, `roles/rayan_checklist.md`, `roles/ryn_role_assignment.md`, `PRD/ARCHITECTURE.md` |
| Phase covered | Project inception through Phase 1 (Protocol Freeze) complete |
| CRC vectors verified | Independently computed and confirmed correct |
| Next update trigger | After Phase 2 (Snapshot Freeze) is merged to main |
