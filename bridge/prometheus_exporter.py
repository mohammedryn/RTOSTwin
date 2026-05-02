/**
 * @file prometheus_exporter.py
 * @brief Exports RTOS telemetry to Prometheus format.
 */

from prometheus_client import start_http_server, Gauge
from .state_manager import DeviceState

# 1. Memory Metrics
RTOS_HEAP_FREE = Gauge('rtos_heap_free_bytes', 
                       'Current available heap in bytes', 
                       ['device_id'])

RTOS_HEAP_MIN = Gauge('rtos_heap_min_ever_bytes', 
                      'Historical minimum free heap seen', 
                      ['device_id'])

# 2. CPU Metrics
RTOS_CPU_UTIL = Gauge('rtos_cpu_utilization_pct', 
                       'Total CPU utilization (0-100%)', 
                       ['device_id'])

# 3. Task Metrics
RTOS_TASK_STATE = Gauge('rtos_task_state', 
                        'Current task state (0=Running, 1=Ready, 2=Blocked, etc)', 
                        ['device_id', 'task_name'])

RTOS_TASK_STACK = Gauge('rtos_task_stack_hwm_bytes', 
                        'Stack high watermark (bytes remaining)', 
                        ['device_id', 'task_name'])

RTOS_TASK_RUNTIME = Gauge('rtos_task_runtime_ticks', 
                          'Total CPU runtime ticks for this task', 
                          ['device_id', 'task_name'])

def start_metrics_server(port: int = 8000):
    """Starts the HTTP server that Prometheus scrapes."""
    start_http_server(port)

def update_prometheus_metrics(device_id: str, state: DeviceState):
    """
    Updates the Prometheus client with the latest device state.
    This is called every time a new packet is fully reconstructed.
    """
    # Update System Metrics
    RTOS_HEAP_FREE.labels(device_id=device_id).set(state.heap_free_bytes)
    RTOS_HEAP_MIN.labels(device_id=device_id).set(state.heap_min_ever_bytes)
    RTOS_CPU_UTIL.labels(device_id=device_id).set(state.cpu_utilization_pct)

    # Update Task Metrics
    for _, task in state.tasks.items():
        RTOS_TASK_STATE.labels(device_id=device_id, task_name=task.name).set(task.state)
        
        # Convert words to bytes (assuming 4-byte words for ARM)
        stack_bytes = task.stack_hwm_words * 4
        RTOS_TASK_STACK.labels(device_id=device_id, task_name=task.name).set(stack_bytes)
        
        RTOS_TASK_RUNTIME.labels(device_id=device_id, task_name=task.name).set(task.runtime_ticks)
