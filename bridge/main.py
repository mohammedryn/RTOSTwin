/**
 * @file main.py
 * @brief Main entry point for the Python Bridge.
 */

import time
import argparse
import serial
from .decoder import PacketDecoder
from .device_registry import DeviceRegistry
from .prometheus_exporter import start_metrics_server, update_prometheus_metrics
from .oom_analyzer import OOMAnalyzer
from .config import *

def run_bridge(port: str, baud: int):
    print(f"🚀 RTOSTwin Bridge starting on {port} @ {baud}...")
    
    registry = DeviceRegistry()
    decoder = PacketDecoder()
    oom = OOMAnalyzer(window_size=OOM_WINDOW_SIZE)
    
    # Start the web server for Grafana
    start_metrics_server(PROMETHEUS_PORT)
    
    try:
        # Open the physical serial port
        ser = serial.Serial(port, baud, timeout=0.1)
        
        while True:
            # 1. Read raw bytes from the wire
            data = ser.read(256)
            if not data:
                continue
                
            # 2. Decode into packets
            packets = decoder.feed_bytes(data)
            
            for packet in packets:
                # 3. Reconstruct full state from deltas
                # Using port name as the unique device_id
                manager = registry.get_manager(port)
                state = manager.update(packet)
                
                # 4. Analyze for memory leaks
                oom.add_sample(state.heap_free_bytes)
                time_to_oom = oom.get_projection_seconds()
                
                if time_to_oom > 0:
                    print(f"⚠️  WARNING: Memory leak detected! Crash in {time_to_oom:.1f}s")
                
                # 5. Push to Dashboards
                update_prometheus_metrics(port, state)
                
    except KeyboardInterrupt:
        print("\n🛑 Bridge stopped by user.")
    except Exception as e:
        print(f"❌ ERROR: {e}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="RTOSTwin Python Bridge")
    parser.add_argument("--port", default=DEFAULT_SERIAL_PORT, help="Serial port (e.g. COM3 or /dev/ttyUSB0)")
    parser.add_argument("--baud", type=int, default=DEFAULT_BAUD_RATE, help="Baud rate")
    
    args = parser.parse_args()
    run_bridge(args.port, args.baud)
