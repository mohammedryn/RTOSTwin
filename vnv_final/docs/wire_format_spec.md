# RTOSTwin Wire Format Specification
Version 1.0 — February 2026

This document defines the binary wire protocol used for communication between the RTOSTwin Telemetry Agent (MCU) and the OTLP/Prometheus Bridge (Host).

## 1. Design Principles
- **Minimal Overhead**: Designed for 115200 baud UART.
- **Robustness**: Sync bytes and CRC-16-CCITT for frame boundary detection and integrity.
- **Efficiency**: Delta encoding reduces bandwidth usage by >90% in typical steady-state operation.
- **Portability**: Little-endian byte order for all multi-byte fields.

## 2. Packet Structure
A complete packet consists of a Header, a Payload, and a Footer (CRC).

| Offset | Field | Size (Bytes) | Description |
| :--- | :--- | :--- | :--- |
| 0 | `SYNC_0` | 1 | Fixed value `0xAA` |
| 1 | `SYNC_1` | 1 | Fixed value `0x55` |
| 2 | `VERSION` | 1 | Protocol version (currently `0x01`) |
| 3 | `TYPE` | 1 | `0x01` (Delta), `0x02` (Keyframe), `0x03` (Device Info) |
| 4-5 | `SEQ_NUM` | 2 | Monotonic sequence number (wraps at 65535) |
| 6-9 | `TIMESTAMP` | 4 | RTOS tick count (`xTaskGetTickCount`) |
| 10-11 | `LENGTH` | 2 | Length of the `PAYLOAD` field only |
| 12..N-1| `PAYLOAD` | N | Encoded data (Delta or Keyframe) |
| N..N+1 | `CRC_16` | 2 | CRC-16-CCITT over bytes [2 .. N-1] |

## 3. Payload Formats

### 3.1 Keyframe (`TYPE = 0x02`)
The keyframe contains a full state snapshot. It is a raw dump of the `full_snapshot_t` structure.

- **Tasks**: `MAX_TASKS` (16) records of 24 bytes each.
- **Memory**: 9 bytes (Heap Free 4, Heap Min 4, CPU Util 1).

Total Keyframe Payload Size: **~400 bytes**.

### 3.2 Delta (`TYPE = 0x01`)
The delta payload contains only fields that have changed since the last packet.

Each field is encoded as:
`[TAG (1 byte)] [VALUE (N bytes)]`

The `TAG` byte is composed of two nibbles:
- **Upper Nibble (4 bits)**: Task Index (0-14). `0xF` indicates a system-level field.
- **Lower Nibble (4 bits)**: Field ID.

#### Field IDs
| ID | Field Name | Size | Description |
| :--- | :--- | :--- | :--- |
| `0x01` | Task State | 1 | `eTaskState` enum value |
| `0x02` | Task Priority | 1 | `uxCurrentPriority` |
| `0x03` | Stack HWM | 2 | Words remaining (High Watermark) |
| `0x04` | Task Runtime | 4 | Total ticks consumed |
| `0x05` | Heap Free | 4 | Current free heap bytes |
| `0x06` | Heap Min | 4 | Historical minimum free heap |
| `0x07` | CPU Util | 1 | 0-100 percentage |

## 4. Checksum
The protocol uses **CRC-16-CCITT**:
- **Polynomial**: `0x1021`
- **Initial Value**: `0xFFFF`
- **Reflection**: None
- **Final XOR**: None

**Test Vector**: `crc16_ccitt("123456789") == 0x29B1`
