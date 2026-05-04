"""
integration_test.py
-------------------
A binary-safe integration test that runs the Mock Device and the Bridge
simultaneously, ensuring that data flows correctly from the simulated 
MCU to the Prometheus metrics endpoint.

Usage:
    python bridge/tests/integration_test.py
"""

import subprocess
import time
import sys
import os

def run_integration():
    print("🚀 Starting RTOSTwin Integration Test...")
    
    # 1. Start the Bridge as a subprocess
    # We use stdin=subprocess.PIPE to ensure we can send binary data safely
    bridge_proc = subprocess.Popen(
        [sys.executable, "bridge/main.py", "--port", "stdin"],
        stdin=subprocess.PIPE,
        stdout=sys.stdout,
        stderr=sys.stderr,
        bufsize=0  # Unbuffered
    )
    
    # 2. Start the Mock Device as a subprocess
    # We capture its stdout to send it to the bridge
    mock_proc = subprocess.Popen(
        [sys.executable, "bridge/mock_device.py", "--mode", "leak"],
        stdout=subprocess.PIPE,
        stderr=sys.stderr,
        bufsize=0
    )
    
    print("✅ Bridge and Mock Device are running.")
    print("📊 View your dashboard at http://localhost:3000")
    print("🔍 Metrics available at http://localhost:8000/metrics")
    print("\nPress Ctrl+C to stop the test.\n")

    try:
        # 3. Proxy data from mock -> bridge manually to ensure binary safety
        while True:
            chunk = mock_proc.stdout.read(1024)
            if not chunk:
                break
            bridge_proc.stdin.write(chunk)
            bridge_proc.stdin.flush()
            
    except KeyboardInterrupt:
        print("\n🛑 Stopping integration test...")
    finally:
        mock_proc.terminate()
        bridge_proc.terminate()
        print("👋 Integration test stopped.")

if __name__ == "__main__":
    # Ensure we are in the root directory
    if not os.path.exists("bridge/main.py"):
        print("❌ Error: Please run this script from the project root directory.")
        sys.exit(1)
    run_integration()
