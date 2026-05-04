# RTOSTwin — File Structure

## Root Directory

This document reflects the **current canonical repo layout** after consolidation.

```text
d:\digital_twin\
├── .github/
│   └── workflows/
│       └── ci.yml                        # Current CI workflow
│
├── PRD/                                  # Project specs + implementation instructions
│   ├── AI_WORKFLOW_RULES.md
│   ├── ARCHITECTURE.md
│   ├── CODING_RULES.md
│   ├── FILE_STRUCTURE.md                 # This file
│   ├── PRD.md
│   ├── TASK_QUEUE.md
│   ├── TECH_SPEC.md
│   ├── WAY_TO_USE.md
│   └── roles/
│       ├── ryn_role_assignment.md
│       └── vnv_role_assignment.md
│
├── agent/                                # Embedded firmware agent (C)
│   ├── core/
│   │   ├── encoder.c
│   │   ├── encoder.h
│   │   ├── framer.c
│   │   ├── framer.h
│   │   ├── profiler.c
│   │   ├── profiler.h
│   │   ├── snapshot.c
│   │   ├── snapshot.h
│   │   ├── transport.c
│   │   ├── transport.h
│   │   └── wire_format.h
│   ├── freertos/
│   │   └── hooks.c
│   ├── hal/
│   │   └── stm32/
│   │       ├── dwt.c
│   │       ├── dwt.h
│   │       ├── uart_dma.c
│   │       └── uart_dma.h
│   ├── tests/
│   │   ├── mocks/
│   │   │   ├── FreeRTOS.h
│   │   │   └── task.h
│   │   ├── test_crc.c
│   │   ├── test_encoder.c
│   │   ├── test_framer.c
│   │   ├── test_profiler.c
│   │   ├── test_snapshot.c
│   │   └── unity_mock.h
│   └── main.c
│
├── bridge/                               # Host bridge (Python)
│   ├── config.py
│   ├── decoder.py
│   ├── device_registry.py
│   ├── main.py
│   ├── mock_device.py
│   ├── oom_analyzer.py
│   ├── otlp_exporter.py
│   ├── prometheus_exporter.py
│   ├── requirements.txt
│   ├── state_manager.py
│   └── tests/
│       ├── conftest.py
│       ├── test_decoder.py
│       └── test_oom_analyzer.py
│
├── dashboard/
│   └── rtostwin_dashboard.json
│
├── docs/
│   ├── ai_context/                       # Reserved for extra context docs
│   ├── quick_start.md
│   └── reports/
│       ├── RTOSTwin_Complete_Report.md
│       └── RTOSTwin_Complete_Report_v2.pdf
│
├── grafana/
│   └── provisioning/
│       ├── dashboards/
│       │   └── provider.yml
│       └── datasources/
│           └── datasource.yml
│
├── prometheus/
│   └── prometheus.yml
│
├── rtos_twin_learning/                   # Learning / roadmap / teaching content
│   ├── explanations/
│   ├── homework/
│   ├── notes/
│   ├── roadmap/
│   ├── ryn/
│   └── vnv/
│
├── semantic-conventions/
│   └── rtos_metrics.md
│
├── archive/                              # Preserved old or duplicate repo material
│   └── legacy/
│       ├── digitaltwin-main-nested/
│       └── root-agent-partial/
│
├── artifacts/                            # Generated local binaries / scratch outputs
│   └── local-validation/
│
├── docker-compose.yml
└── README.md
```

---

## Canonical Source Rules

- The **repo root** is the canonical project root.
- All active implementation work belongs in root folders such as `agent/`, `bridge/`, `dashboard/`, `docs/`, `grafana/`, and `prometheus/`.
- `archive/legacy/` is **not** active source; it is preserved only for reference/recovery.
- `artifacts/local-validation/` is **not** source; it stores generated binaries, temporary test harnesses, and scratch outputs.
- `rtos_twin_learning/` is supporting learning content, not the production runtime path.

---

## File Responsibility Rules

| Path | What Goes Here | What Does NOT Go Here |
|---|---|---|
| `agent/core/snapshot.*` | RTOS state capture structs and logic | Framing, CRC, transport |
| `root agent/core/wire_format.h` | Shared wire constants only | Runtime logic, structs unrelated to protocol constants |
| `agent/core/encoder.*` | Delta/keyframe encoding | UART/HAL code, snapshot capture |
| `agent/core/framer.*` | Packet assembly and CRC | Transport/HAL code |
| `agent/core/transport.*` | Transport abstraction logic | STM32 HAL register/HAL details |
| `agent/hal/stm32/*` | STM32-specific hardware support | Protocol logic, Python bridge logic |
| `bridge/decoder.py` | Byte-stream decoding and CRC validation | Exporters, OOM analysis |
| `bridge/state_manager.py` | Reconstruct full device state from decoded packets | Raw byte parsing |
| `bridge/prometheus_exporter.py` | Prometheus metrics exposure | Decoder logic |
| `bridge/otlp_exporter.py` | OTLP metric export | Decoder logic |
| `bridge/oom_analyzer.py` | Heap trend/OOM analysis | Export serving, byte parsing |
| `dashboard/rtostwin_dashboard.json` | Grafana dashboard definition | Prometheus scrape config |
| `docs/reports/*` | Long-form reports and evaluation material | Runtime config |
| `semantic-conventions/rtos_metrics.md` | OTel semantic conventions proposal | Product quick-start steps |
| `archive/legacy/*` | Old duplicated or superseded material | New active implementation |
| `artifacts/local-validation/*` | Generated outputs and local test artifacts | Source files meant for active development |

---

## Dependency Direction (Current Intended Flow)

```text
agent/core/snapshot.* -> agent/core/encoder.* -> agent/core/framer.* -> agent/core/transport.*
                                                               |
                                                               v
                                                          agent/main.c

root agent/core/wire_format.h -> bridge/decoder.py -> bridge/state_manager.py
                                                   -> bridge/prometheus_exporter.py
                                                   -> bridge/otlp_exporter.py
                                                   -> bridge/oom_analyzer.py
```

- `snapshot` must not depend on `encoder`, `framer`, or `transport`.
- `decoder` must stay the single wire parser used by the bridge.
- Exporters must consume reconstructed state, not raw packet bytes.
- Archived and generated folders must never become part of the active dependency path.
