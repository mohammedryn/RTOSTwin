# Quick Start Guide

Follow these steps to bring up the current `RTOSTwin` stack on the baseline `NUCLEO-F401RE` path first. The same bridge architecture is intended to expand to `ESP32-P4` and `Teensy 4.1` after the host-side bridge is stable.

## 1. Start the Observability Stack
Launch Prometheus and Grafana using Docker Compose:

```bash
docker-compose up -d
```
- **Grafana:** [http://localhost:3000](http://localhost:3000) (Login: admin/admin)
- **Prometheus:** [http://localhost:9090](http://localhost:9090)

## 2. Install Python Dependencies
Ensure you have Python 3.9+ installed, then:

```bash
pip install -r bridge/requirements.txt
```

## 3. Run the Bridge
Connect your telemetry source to your PC and run:

```bash
python bridge/main.py --port COM3 --baud 115200
```
*(Replace `COM3` with your actual serial or USB CDC port.)*

## 4. View the Dashboard
1. Open Grafana at [http://localhost:3000](http://localhost:3000).
2. Go to **Dashboards** → **RTOSTwin Digital Twin**.
3. You should see real-time task data and memory trends appearing immediately.

## Testing without Hardware
If you don't have hardware handy, you can run the mock device script:

```bash
python bridge/mock_device.py --mode normal
```
*(This generates valid telemetry bytes for local decoder and bridge testing.)*
