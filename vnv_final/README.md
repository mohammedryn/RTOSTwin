# RTOSTwin — RTOS Digital Twin & Observability Bridge

**RTOSTwin** is a low-overhead observability agent for FreeRTOS and a Python bridge that transforms embedded telemetry into real-time Grafana dashboards across `NUCLEO-F401RE`, `ESP32-P4`, and `Teensy 4.1`.

![RTOSTwin Architecture](dashboard/architecture_diagram.png)

## Features

- **🚀 Low-overhead agent:** Optimized C agent with delta encoding and board-specific transport backends.
- **📉 95% Bandwidth Reduction:** Only sends changed fields between keyframes.
- **🚨 OOM Prediction:** Python-side linear regression predicts heap exhaustion before it happens.
- **📊 Modern Dashboards:** Pre-configured Grafana dashboard for task states, stack health, and memory.
- **🔌 Multi-Exporter:** Push metrics to Prometheus or any OpenTelemetry (OTLP) backend.

## Architecture

1. **MCU Agent (C):** Hooks into FreeRTOS, encodes deltas, and streams over UART, USB CDC, or UDP depending on the board.
2. **Python Bridge:** Decodes packets, maintains device state, and runs OOM analysis for one or more devices.
3. **Observability Stack:** Prometheus stores metrics; Grafana visualizes the "Digital Twin".

## Quick Start

See [docs/quick_start.md](docs/quick_start.md) to get running in 5 minutes.

## Tech Stack

- **Embedded:** C11, FreeRTOS, STM32 HAL, ESP-IDF, and Teensy/i.MX RT platform support.
- **Bridge:** Python 3.9+, pyserial, scipy, prometheus-client, opentelemetry-sdk.
- **Infrastructure:** Docker, Prometheus, Grafana.
