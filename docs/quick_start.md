# Quick Start Guide

Follow these steps to set up the RTOSTwin digital twin.

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
Connect your MCU to your PC and run:

```bash
python bridge/main.py --serial COM3 --baud 115200
```
*(Replace `COM3` with your actual serial port)*

## 4. View the Dashboard
1. Open Grafana at [http://localhost:3000](http://localhost:3000).
2. Go to **Dashboards** → **RTOSTwin Digital Twin**.
3. You should see real-time task data and memory trends appearing immediately.

## Testing without Hardware
If you don't have an MCU handy, you can run the mock device script:

```bash
python bridge/mock_device.py
```
*(This will simulate a virtual serial port and stream fake telemetry to the bridge)*
