# ESP32 HAL for RTOSTwin
**Status:** Planned / Under Development

This directory will contain the hardware abstraction layer for ESP32 devices using the ESP-IDF framework.

## Planned Features
- **Transport**: WiFi (UDP/TCP) and USB Serial support.
- **Timing**: Use of ESP-IDF High Resolution Timer (esp_timer) for profiling.
- **Optimization**: Leveraging the dual-core architecture of ESP32 to offload telemetry encoding.
