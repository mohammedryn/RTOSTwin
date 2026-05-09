# Quick Start Guide

Follow these steps from the `vnv_final/` directory to bring up the local
observability stack first, then swap in the real `NUCLEO-F401RE` serial stream
when hardware is available.

## 1. Start in the correct working tree

```bash
cd vnv_final
```

The protocol source of truth still lives at the repository root:

- `../docs/wire_format_spec.md`
- `../agent/core/wire_format.h`

## 2. Start the observability stack

Launch Prometheus and Grafana using Docker Compose:

```bash
docker-compose up -d
```

- **Grafana:** [http://localhost:3000](http://localhost:3000)
- **Prometheus:** [http://localhost:9090](http://localhost:9090)

Grafana is provisioned for anonymous local access in this subtree.

## 3. Install Python dependencies

Ensure you have Python 3.9+ installed, then:

```bash
pip install -r bridge/requirements.txt
```

## 4. Local repo-verified demo path (no hardware)

Run the bridge and feed it the canonical mock telemetry stream:

```bash
python bridge/mock_device.py --mode leak | python bridge/main.py --port stdin --device-id mock-stdin
```

Once the bridge is running:

1. Open `http://localhost:8000/metrics` in a browser, or
2. Run:

```bash
curl http://localhost:8000/metrics
```

You should see RTOS metrics such as:

- `rtos_heap_free_bytes`
- `rtos_task_state`
- `rtos_cpu_utilization_ratio`
- `rtos_telemetry_packet_loss_ratio`

Then open Grafana and load the provisioned dashboard to watch live mock data.

## 5. Real hardware path (`NUCLEO-F401RE`)

Connect your telemetry source to your PC and run:

```bash
python bridge/main.py --port COM3 --baud 115200 --device-id nucleo-f401re
```

Replace `COM3` with the actual serial or USB CDC port exposed by your board.

## 6. Optional OTLP export

Local development keeps OTLP disabled by default so a missing collector does not
spam errors or interrupt the bridge loop.

Enable OTLP explicitly only when you have a collector or backend ready:

```bash
set RTOSTWIN_ENABLE_OTLP=1
set OTEL_EXPORTER_OTLP_ENDPOINT=http://localhost:4318/v1/metrics
python bridge/main.py --port stdin
```

## 7. What is verified in-repo vs. external

- **Verified in this repo:** mock-device decoding, state reconstruction, metrics rendering, bridge tests, and CLI/compile checks.
- **Still external:** flashing firmware to a real `NUCLEO-F401RE` and capturing live serial-to-Grafana evidence.

See [vnv_repo_completion_status.md](vnv_repo_completion_status.md) for the current verified status.
