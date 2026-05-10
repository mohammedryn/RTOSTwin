"""
manual_integration_demo.py
--------------------------
Manual binary-safe integration harness for the RTOSTwin bridge.

This script starts the bridge and the mock device as subprocesses, then
forwards raw bytes from the mock device into the bridge process. It is a
human-run demo tool, not an automated pytest test.

Run from the vnv_final project root:
    python bridge/manual_integration_demo.py
"""

import os
import subprocess
import sys


def run_integration() -> None:
    print("Starting RTOSTwin integration demo...")

    bridge_proc = subprocess.Popen(
        [sys.executable, "bridge/main.py", "--port", "stdin"],
        stdin=subprocess.PIPE,
        stdout=sys.stdout,
        stderr=sys.stderr,
        bufsize=0,
    )

    mock_proc = subprocess.Popen(
        [sys.executable, "bridge/mock_device.py", "--mode", "leak"],
        stdout=subprocess.PIPE,
        stderr=sys.stderr,
        bufsize=0,
    )

    print("Bridge and mock device are running.")
    print("View Grafana at http://localhost:3000")
    print("View Prometheus metrics at http://localhost:8000/metrics")
    print("Press Ctrl+C to stop the demo.")

    try:
        while True:
            chunk = mock_proc.stdout.read(1024)
            if not chunk:
                break
            if bridge_proc.poll() is not None:
                print("Bridge process exited; stopping integration demo.")
                break
            bridge_proc.stdin.write(chunk)
            bridge_proc.stdin.flush()
    except KeyboardInterrupt:
        print("\nStopping integration demo...")
    except OSError as exc:
        print(f"Integration demo stopping after pipe error: {exc}")
    finally:
        mock_proc.terminate()
        bridge_proc.terminate()
        print("Integration demo stopped.")


if __name__ == "__main__":
    if not os.path.exists("bridge/main.py"):
        print("Error: run this script from the vnv_final project root.")
        sys.exit(1)
    run_integration()
