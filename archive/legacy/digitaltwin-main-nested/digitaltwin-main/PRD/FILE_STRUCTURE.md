# RTOSTwin — File Structure

## Root Directory

```
d:\digital_twin\
├── PRD/                          # AI vibecoding context (THIS folder)
│   ├── PRD.md
│   ├── ARCHITECTURE.md
│   ├── TECH_SPEC.md
│   ├── FILE_STRUCTURE.md         # (this file)
│   ├── CODING_RULES.md
│   └── TASK_QUEUE.md
│
├── agent/                        # C99 firmware — runs on MCU
│   ├── core/                     # Platform-independent agent logic
│   │   ├── snapshot.h            # full_snapshot_t structs + API
│   │   ├── snapshot.c            # snapshot_capture() implementation
│   │   ├── wire_format.h         # Protocol constants (shared with bridge)
│   │   ├── encoder.h             # Delta encoder API
│   │   ├── encoder.c             # Delta encoder implementation
│   │   ├── framer.h              # Packet framing API
│   │   ├── framer.c              # Packet framing + CRC implementation
│   │   ├── transport.h           # Transport abstraction API
│   │   ├── transport.c           # Transport task + queue logic
│   │   ├── profiler.h            # DWT profiler API
│   │   └── profiler.c            # DWT profiler implementation
│   │
│   ├── hal/                      # Hardware abstraction (platform-specific)
│   │   ├── stm32/
│   │   │   ├── uart_dma.h
│   │   │   ├── uart_dma.c        # STM32 HAL_UART_Transmit_DMA wrapper
│   │   │   ├── dwt.h
│   │   │   └── dwt.c             # DWT->CYCCNT enable + read
│   │   └── esp32/
│   │       ├── wifi_transport.h
│   │       └── wifi_transport.c  # ESP-IDF WiFi TCP send
│   │
│   ├── freertos/
│   │   └── hooks.c               # vApplicationIdleHook, trace hooks
│   │
│   ├── config/
│   │   └── FreeRTOSConfig.h      # configUSE_TRACE_FACILITY=1, configGENERATE_RUN_TIME_STATS=1
│   │
│   ├── tests/                    # Unity C unit tests
│   │   ├── test_snapshot.c
│   │   ├── test_encoder.c
│   │   ├── test_framer.c
│   │   ├── test_crc.c
│   │   └── test_runner.c
│   │
│   ├── main.c                    # Telemetry task creation, snapshot_init(), main loop
│   ├── CMakeLists.txt            # Build configuration
│   └── Makefile                  # Alternative build
│
├── bridge/                       # Python 3.9+ — runs on host
│   ├── __init__.py
│   ├── main.py                   # Entry point: serial reader + start exporters
│   ├── decoder.py                # Stateful binary packet decoder
│   ├── state_manager.py          # Reconstruct full state from deltas per device
│   ├── device_registry.py        # Multi-device registry (device_id → DeviceState)
│   ├── prometheus_exporter.py    # HTTP /metrics endpoint
│   ├── otlp_exporter.py          # OTLP/HTTP push exporter
│   ├── oom_analyzer.py           # Linear regression + rolling min OOM detector
│   ├── mock_device.py            # Generate valid packets without hardware
│   ├── config.py                 # All configurable parameters (ports, intervals, thresholds)
│   ├── requirements.txt          # Pinned dependencies
│   │
│   └── tests/                    # pytest test suite
│       ├── test_decoder.py
│       ├── test_oom_analyzer.py
│       ├── test_prometheus.py
│       ├── test_otlp.py
│       ├── test_state_manager.py
│       └── conftest.py           # Shared fixtures (known-good packets, mock data)
│
├── dashboard/
│   └── rtostwin_dashboard.json   # Grafana dashboard template (one-click import)
│
├── examples/
│   ├── blinky_twin/              # Minimal: 1 task + telemetry
│   │   ├── main.c
│   │   ├── FreeRTOSConfig.h
│   │   └── CMakeLists.txt
│   └── sensor_system/            # Realistic: multi-task sensor + processing + comms
│       ├── main.c
│       ├── sensor_task.c
│       ├── processing_task.c
│       ├── comms_task.c
│       ├── FreeRTOSConfig.h
│       └── CMakeLists.txt
│
├── docs/
│   ├── quick_start.md            # git clone → Grafana in 30 min
│   ├── architecture.md           # Public-facing architecture doc
│   ├── api_reference.md          # Doxygen-generated C API
│   ├── troubleshooting.md        # Common issues + fixes
│   └── learning/                 # RYN + VNV learning materials
│       └── roles/
│           ├── ryn_role_assignment.md
│           └── vnv_role_assignment.md
│
├── semantic-conventions/
│   └── rtos_metrics.md           # OTel semantic conventions proposal
│
├── tools/
│   └── overhead_profiler.py      # Script to measure + report agent overhead
│
├── .github/
│   └── workflows/
│       ├── ci.yml                # Build + test on every push
│       └── size_check.yml        # Verify agent < 10 KB
│
├── docker-compose.yml            # Prometheus + Grafana for local testing
├── README.md                     # Project README
├── LICENSE                       # MIT
└── .gitignore
```

---

## File Responsibility Rules

| File | What Goes Here | What Does NOT Go Here |
|---|---|---|
| `snapshot.h` | Struct definitions, `#define MAX_TASKS`, function prototypes | Any implementation code |
| `snapshot.c` | `snapshot_capture()` implementation, static buffers, idle hook counter reads | CRC code, framing code, transport code |
| `wire_format.h` | Protocol constants ONLY (`#define`). No functions, no structs. | Anything other than `#define` constants |
| `encoder.c` | Delta comparison, tag-byte encoding, keyframe serialization | Framing (no SYNC bytes), CRC, transport |
| `framer.c` | SYNC bytes, header fields, CRC computation, complete packet assembly | Snapshot capture, encoding, sending |
| `transport.c` | DMA initiation, busy check, drop counter, telemetry task loop | Packet encoding, framing, CRC |
| `decoder.py` | Byte-by-byte state machine, CRC validation, struct unpacking | Prometheus export, OTLP, analysis |
| `oom_analyzer.py` | Linear regression, rolling minimum, projection calculation | Packet decoding, HTTP serving |
| `prometheus_exporter.py` | Gauge creation, metric updates, HTTP server | Decoding, analysis, OTLP |
| `otlp_exporter.py` | OTel SDK meter, OTLP exporter setup, metric push | Decoding, analysis, Prometheus |
| `mock_device.py` | Generate binary packets to stdout/TCP for testing | Real serial port reading |

---

## Dependency Direction (Never Violate)

```
snapshot.h  ← snapshot.c
     ↓
wire_format.h ← encoder.c ← framer.c ← transport.c ← main.c
                                              ↓
                                        HAL (uart_dma.c)

wire_format.h → decoder.py → state_manager.py → prometheus_exporter.py
                                              → otlp_exporter.py
                                              → oom_analyzer.py
```

- `snapshot.c` NEVER includes `encoder.h` or `framer.h`.
- `encoder.c` NEVER includes `transport.h`.
- `decoder.py` NEVER imports `prometheus_exporter` or `otlp_exporter`.
- Dependencies flow ONE direction: capture → encode → frame → send → decode → export.
