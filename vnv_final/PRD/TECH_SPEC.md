# RTOSTwin — Technical Specification

## 1. Data Structures (C99 — Agent Side)

### 1.1 Snapshot Structs — `agent/core/snapshot.h`

```c
#include "FreeRTOS.h"
#include "task.h"
#include <stdint.h>
#include <stdbool.h>

#define MAX_TASKS              16
#define TASK_NAME_MAX_LEN      16

typedef struct {
    char     name[TASK_NAME_MAX_LEN];
    uint8_t  state;                   /* eTaskState enum */
    uint8_t  priority;                /* uxCurrentPriority */
    uint16_t stack_hwm_words;         /* uxTaskGetStackHighWaterMark() */
    uint32_t runtime_ticks;           /* ulRunTimeCounter */
} task_snapshot_t;                    /* 24 bytes per task */

typedef struct {
    uint32_t heap_free_bytes;
    uint32_t heap_min_ever_bytes;
    uint8_t  cpu_utilization_pct;     /* 0-100 */
} memory_snapshot_t;                  /* 9 bytes */

typedef struct {
    uint16_t         sequence_num;
    uint32_t         timestamp_ticks;
    uint8_t          task_count;
    task_snapshot_t   tasks[MAX_TASKS];
    memory_snapshot_t memory;
} full_snapshot_t;                    /* ~400 bytes max */
```

### 1.2 Profiler Stats — `agent/core/profiler.h`

```c
typedef struct {
    uint32_t min_cycles;
    uint32_t max_cycles;
    uint32_t call_count;
    uint64_t total_cycles;
} profiler_stats_t;
```

---

## 2. Wire Format — Binary Packet Layout
> Canonical v1 protocol truth now lives only in the root repo files `docs/wire_format_spec.md` and `agent/core/wire_format.h`. This VNV PRD may describe how VNV code consumes that contract, but it must not be treated as an independent packet-spec source.


### 2.1 Packet Structure — root `agent/core/wire_format.h`

All multi-byte fields are **little-endian** on the wire, independent of MCU architecture.

| Offset | Field | Size | Value/Range |
|---|---|---|---|
| 0 | SYNC_0 | 1 | `0xAA` (fixed) |
| 1 | SYNC_1 | 1 | `0x55` (fixed) |
| 2 | VERSION | 1 | `0x01` (protocol v1) |
| 3 | TYPE | 1 | `0x01`=delta, `0x02`=keyframe, `0x03`=device_info |
| 4-5 | SEQ_NUM | 2 | `uint16_t`, monotonic, wraps at 65535→0 |
| 6-9 | TIMESTAMP | 4 | `xTaskGetTickCount()` value |
| 10-11 | LENGTH | 2 | Payload byte count (excludes header + CRC) |
| 12..12+N-1 | PAYLOAD | N | Encoded snapshot data |
| 12+N..12+N+1 | CRC_16 | 2 | CRC-CCITT over bytes [2..12+N-1] (VERSION through PAYLOAD) |

### 2.2 Protocol Constants

```c
#define WF_SYNC_0               0xAAU
#define WF_SYNC_1               0x55U
#define WF_PROTOCOL_VERSION     0x01U
#define WF_TYPE_DELTA           0x01U
#define WF_TYPE_KEYFRAME        0x02U
#define WF_TYPE_DEVICE_INFO     0x03U
#define WF_KEYFRAME_INTERVAL    50U
#define WF_MAX_PACKET_SIZE      512U
#define WF_HEADER_SIZE          12U
#define WF_CRC_SIZE             2U
#define WF_OVERHEAD             (WF_HEADER_SIZE + WF_CRC_SIZE)
#define WF_CRC_POLY             0x1021U
#define WF_CRC_INIT             0xFFFFU
```

### 2.3 CRC-16-CCITT Algorithm

**Polynomial:** `0x1021` | **Initial:** `0xFFFF` | **No reflection** | **No final XOR**

C implementation:
```c
uint16_t crc16_ccitt(const uint8_t *data, uint16_t len) {
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (uint8_t j = 0; j < 8; j++) {
            crc = (crc & 0x8000) ? ((crc << 1) ^ 0x1021) : (crc << 1);
        }
    }
    return crc;
}
```

Python implementation (must produce identical output):
```python
def crc16_ccitt(data: bytes, initial: int = 0xFFFF) -> int:
    crc = initial
    for byte in data:
        crc ^= byte << 8
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021) if (crc & 0x8000) else (crc << 1)
    return crc & 0xFFFF
```

**Test vector:** `crc16_ccitt(b"123456789")` must equal `0x29B1`.

### 2.4 Delta Encoding Format

When TYPE=`0x01` (delta), payload contains only changed fields:

| Byte | Meaning |
|---|---|
| TAG byte | `(task_index << 4) | field_id` for per-task fields, or `0xF0 | field_id` for system fields |
| Value bytes | New value (size depends on field_id) |

Field IDs:
| field_id | Field | Size |
|---|---|---|
| 0x01 | task state | 1 byte |
| 0x02 | task priority | 1 byte |
| 0x03 | task stack_hwm | 2 bytes |
| 0x04 | task runtime | 4 bytes |
| 0x05 | heap_free_bytes | 4 bytes |
| 0x06 | heap_min_ever | 4 bytes |
| 0x07 | cpu_utilization | 1 byte |

When TYPE=`0x02` (keyframe), payload is the raw `full_snapshot_t` struct serialized field-by-field in the order defined above.

---

## 3. Function Signatures — Agent (C99)

### 3.1 Snapshot Engine

```c
void snapshot_init(void);
void snapshot_capture(full_snapshot_t *out);
```

**Constraints:** Uses `static TaskStatus_t s_task_status_buf[MAX_TASKS]` at file scope. Uses `taskENTER_CRITICAL()` / `taskEXIT_CRITICAL()`. Requires `configUSE_TRACE_FACILITY=1` and `configGENERATE_RUN_TIME_STATS=1`.

### 3.2 Encoder

```c
uint16_t encoder_encode(const full_snapshot_t *current,
                        uint8_t *out_buf, uint16_t out_buf_size,
                        bool force_keyframe);
void encoder_reset(void);
```

**Constraints:** Maintains `static full_snapshot_t s_last_snapshot`. Keyframe forced when `sequence_num % WF_KEYFRAME_INTERVAL == 0`. No malloc.

### 3.3 Framer

```c
uint16_t frame_packet(const uint8_t *payload, uint16_t payload_len,
                      uint8_t packet_type, uint16_t sequence_num,
                      uint32_t timestamp_ticks,
                      uint8_t *out_buf, uint16_t out_buf_size);
```

**Constraints:** `packet_type` must be one of `WF_TYPE_DELTA`, `WF_TYPE_KEYFRAME`, or `WF_TYPE_DEVICE_INFO`. Sequence ownership is snapshot-owned in v1, so callers pass `sequence_num` into the framer. CRC coverage remains bytes `VERSION..PAYLOAD` exactly as frozen in root `docs/wire_format_spec.md`.

### 3.4 Transport

```c
void transport_init(void);
int  transport_send(const uint8_t *packet, uint16_t len);  /* 0=ok, -1=busy/dropped */
uint32_t transport_get_drop_count(void);
```

**Constraints:** The transport layer must expose a non-blocking byte-stream backend. `STM32F401RE` may use `HAL_UART_Transmit_DMA`, while `ESP32-P4` and `Teensy 4.1` may use USB CDC or UDP-capable backends. If the backend is busy, return -1 and increment the drop counter. NEVER blocks.

### 3.5 Profiler

```c
void     profiler_init(void);
uint32_t profiler_start(void);
uint32_t profiler_stop(uint32_t start_cycles);
void     profiler_record(profiler_stats_t *stats, uint32_t elapsed_cycles);
void     profiler_report(const profiler_stats_t *stats, const char *label);
```

---

## 4. Function Signatures — Bridge (Python 3.9+)

### 4.1 Decoder

```python
class PacketDecoder:
    def feed_byte(self, byte: int) -> Optional[DecodedPacket]: ...
    def feed_bytes(self, data: bytes) -> List[DecodedPacket]: ...
    @property
    def drop_count(self) -> int: ...
    @property
    def sequence_gap_count(self) -> int: ...
```

### 4.2 OOM Analyzer

```python
class OOMAnalyzer:
    def __init__(self, window_size=600, min_r_squared=0.7,
                 rolling_min_threshold=0.10, total_heap_bytes=131072): ...
    def add_sample(self, timestamp_s: float, heap_free_bytes: int) -> None: ...
    def get_projection_seconds(self) -> float: ...  # -1.0 = stable
    @property
    def regression_slope_bytes_per_second(self) -> float: ...
    @property
    def rolling_minimum_bytes(self) -> int: ...
    @property
    def r_squared(self) -> float: ...
```

### 4.3 Prometheus Exporter

```python
class PrometheusExporter:
    def __init__(self, port: int = 8000): ...
    def update_metrics(self, device_id: str, state: DeviceState) -> None: ...
    def start(self) -> None: ...  # Start HTTP server
```

**Metric names (Prometheus format, underscores):**
`rtos_task_state`, `rtos_task_stack_watermark_bytes`, `rtos_task_cpu_ratio`,
`rtos_heap_free_bytes`, `rtos_heap_min_ever_bytes`, `rtos_heap_oom_projection_seconds`,
`rtos_cpu_utilization_ratio`, `rtos_telemetry_packet_loss_ratio`

### 4.4 OTLP Exporter

```python
class OTLPExporter:
    def __init__(self, endpoint: str = None): ...  # Default from env OTEL_EXPORTER_OTLP_ENDPOINT
    def export_metrics(self, device_id: str, state: DeviceState) -> None: ...
```

**Metric names (OTel format, dots):**
`rtos.task.state`, `rtos.task.stack_watermark`, `rtos.task.cpu_ratio`,
`rtos.heap.free_bytes`, `rtos.heap.min_ever_bytes`, `rtos.heap.oom_projection_seconds`,
`rtos.cpu.utilization_ratio`, `rtos.telemetry.packet_loss_ratio`

---

## 5. Algorithms

### 5.1 CPU Utilization via Idle Hook

```
idle_ratio = idle_ticks_in_period / total_ticks_in_period
cpu_utilization_pct = 100 - (idle_ratio × 100)
```

Measured using DWT cycle counter. Period = 1 second. Reset counters every period.

### 5.2 OOM Detection — Two Detectors

**Detector 1: Linear Regression** on sliding window of `heap_free_bytes` (600 samples, 60s).
- Slope < 0 AND R² > 0.7 → leak confirmed.
- Projection: `heap_free_current / abs(slope)` seconds to OOM.

**Detector 2: Rolling Minimum** over same window.
- `(rolling_min_start - rolling_min_now) / total_heap > 0.10` → secondary alert.
- Catches sawtooth/bursty leaks that have near-zero linear slope.

### 5.3 DWT Wrap-Around Handling

`DWT->CYCCNT` is 32-bit, wraps every ~25.6 seconds at 168 MHz.

```c
uint32_t elapsed = end - start;  /* Unsigned subtraction handles wrap correctly in C */
```

---

## 6. Known Hard Problems (Front-Loaded)

| Problem | Impact | Solution |
|---|---|---|
| `uxTaskGetSystemState()` suspends scheduler for 15-25µs | ISRs still fire but tasks don't switch | Keep telemetry task lowest priority. Measure actual suspend time. |
| `uxTaskGetStackHighWaterMark()` scans entire stack | ~9µs per task × 8 tasks = 72µs | If over budget: round-robin sampling (scan 2 tasks per cycle) |
| `malloc` inside snapshot causes priority inversion | Telemetry task holds heap mutex, higher-priority task blocks | **Static allocation only. No malloc.** |
| DWT CYCCNT wrap at 168 MHz = 25.6s | Timing measurement wraps | Unsigned subtraction handles wrap in C. Python: modular arithmetic. |
| UART 115200 = only 11.5 KB/s | Full snapshots at 10 Hz = 30% bandwidth | Delta encoding mandatory. Reduces to < 2%. |
| Linear regression misses sawtooth leaks | Slope ≈ 0 despite net leak | Second detector: rolling minimum comparison |
