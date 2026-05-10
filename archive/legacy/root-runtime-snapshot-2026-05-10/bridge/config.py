/**
 * @file config.py
 * @brief Global configuration and defaults for the RTOSTwin Bridge.
 */

import os

# Serial Settings
DEFAULT_SERIAL_PORT = "COM3"
DEFAULT_BAUD_RATE = 115200

# Exporter Settings
PROMETHEUS_PORT = 8000
OTLP_ENDPOINT = os.getenv("OTEL_EXPORTER_OTLP_ENDPOINT", "http://localhost:4318/v1/metrics")

# OOM Analyzer Settings
OOM_WINDOW_SIZE = 600  # 60 seconds @ 10 Hz
OOM_CONFIDENCE_THRESHOLD = 0.7

# Protocol Settings
PROTOCOL_VERSION = 1
KEYFRAME_INTERVAL = 50
