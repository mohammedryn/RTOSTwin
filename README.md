# RTOSTwin — RTOS Digital Twin & Observability Bridge

**RTOSTwin** is a low-overhead observability agent for FreeRTOS and a high-performance Python bridge that transforms embedded telemetry into real-time Grafana dashboards.

![RTOSTwin Architecture](dashboard/architecture_diagram.png)

## Features

- **🚀 0.1% CPU Overhead:** Optimized C agent using UART DMA and delta encoding.
- **📉 95% Bandwidth Reduction:** Only sends changed fields between keyframes.
- **🚨 OOM Prediction:** Python-side linear regression predicts heap exhaustion before it happens.
- **📊 Modern Dashboards:** Pre-configured Grafana dashboard for task states, stack health, and memory.
- **🔌 Multi-Exporter:** Push metrics to Prometheus or any OpenTelemetry (OTLP) backend.

## Architecture

1. **MCU Agent (C):** Hooks into FreeRTOS idle/task switches, encodes deltas, and streams over UART.
2. **Python Bridge:** Decodes packets, maintains device state, and runs OOM analysis.
3. **Observability Stack:** Prometheus stores metrics; Grafana visualizes the "Digital Twin".

## Quick Start

See [docs/quick_start.md](docs/quick_start.md) to get running in 5 minutes.

## Tech Stack

- **Embedded:** C11, FreeRTOS, STM32 HAL.
- **Bridge:** Python 3.9+, pyserial, scipy, prometheus-client, opentelemetry-sdk.
- **Infrastructure:** Docker, Prometheus, Grafana.
