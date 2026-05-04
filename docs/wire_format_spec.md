# v1 Wire Format Specification

## 1. Scope

This document freezes the exact v1 on-wire packet contract shared by the C telemetry agent and the Python bridge.

All multi-byte fields on the wire are little-endian.

## 2. `frame_packet()` v1 contract

```c
uint16_t frame_packet(const uint8_t *payload,
                      uint16_t payload_len,
                      uint8_t packet_type,
                      uint16_t sequence_num,
                      uint32_t timestamp_ticks,
                      uint8_t *out_buf,
                      uint16_t out_buf_size);
```

- `sequence_num` is snapshot-owned in v1 and is written into header bytes 4-5.
- `timestamp_ticks` is the raw `xTaskGetTickCount()` value and is written into header bytes 6-9.
- `packet_type` must be one of `WF_TYPE_DELTA`, `WF_TYPE_KEYFRAME`, or `WF_TYPE_DEVICE_INFO`.
- The framed packet always uses the fixed v1 header layout defined below, followed by the payload bytes and a trailing CRC.

## 3. CRC-16-CCITT rules

- Polynomial: `0x1021`
- Initial value: `0xFFFF`
- Input reflection: disabled
- Output reflection: disabled
- Final XOR: none
- Coverage: bytes `VERSION` through the end of `PAYLOAD` inclusive
- Header sync bytes `0xAA 0x55` are excluded from the CRC input
- Reference vector: `crc16_ccitt(b"123456789") == 0x29B1`

## 4. Packet header layout

| Offset | Field | Size | Encoding | Notes |
|---|---|---:|---|---|
| 0 | `SYNC_0` | 1 | `0xAA` | fixed |
| 1 | `SYNC_1` | 1 | `0x55` | fixed |
| 2 | `VERSION` | 1 | `uint8` | `WF_PROTOCOL_VERSION` |
| 3 | `TYPE` | 1 | `uint8` | delta, keyframe, or device info |
| 4-5 | `SEQ_NUM` | 2 | little-endian `uint16` | snapshot-owned |
| 6-9 | `TIMESTAMP_TICKS` | 4 | little-endian `uint32` | raw RTOS tick count |
| 10-11 | `PAYLOAD_LEN` | 2 | little-endian `uint16` | payload only |
| 12.. | `PAYLOAD` | N | bytes | type-specific |
| 12+N..13+N | `CRC_16` | 2 | little-endian `uint16` | over bytes `2..11+N` |

## 5. Payload layouts

### 5.1 Keyframe payload

Keyframes serialize fields in this order with no padding bytes:

1. `sequence_num` (`uint16`, little-endian)
2. `timestamp_ticks` (`uint32`, little-endian)
3. `task_count` (`uint8`)
4. Repeated `task_count` times:
   - `name[16]` (`char[16]`, always emitted as 16 single-byte ASCII task-name bytes on the wire; copy at most 15 bytes from the source name, write `\0` in byte 15, null-pad shorter names)
   - `state` (`uint8`)
   - `priority` (`uint8`)
   - `stack_hwm_words` (`uint16`, little-endian)
   - `runtime_ticks` (`uint32`, little-endian)
5. `heap_free_bytes` (`uint32`, little-endian)
6. `heap_min_ever_bytes` (`uint32`, little-endian)
7. `cpu_utilization_pct` (`uint8`)

### 5.2 Delta payload

- Delta payload bytes are encoded as a linear sequence of `[tag][value-bytes]` entries packed back-to-back with no separator bytes.
- Delta parsing terminates exactly at `PAYLOAD_LEN`; there is no sentinel entry.
- Each system field ID may appear at most once per packet, and each `(task_index, field_id)` pair may appear at most once per packet.
- Entry ordering on the wire should be preserved exactly as emitted by the encoder.
- Per-task delta tag: `(task_index << 4) | field_id`
- System delta tag: `0xF0 | field_id`
- Task indices `0x0` through `0xE` are valid for per-task delta entries. Task index `0xF` is reserved because high nibble `0xF` identifies system tags.
- Field-ID validity is part of the v1 contract: `0x01` through `0x04` are valid only for per-task tags, `0x05` through `0x07` are valid only for system tags, and all other tag/field-ID combinations are invalid.
- Field IDs:
  - `0x01` task state (`uint8`)
  - `0x02` task priority (`uint8`)
  - `0x03` task stack watermark words (`uint16`, little-endian)
  - `0x04` task runtime ticks (`uint32`, little-endian)
  - `0x05` heap free bytes (`uint32`, little-endian)
  - `0x06` heap minimum ever bytes (`uint32`, little-endian)
  - `0x07` CPU utilization percent (`uint8`)
- Task topology changes, task renames, and `task_count` changes require a keyframe.

### 5.3 Device info payload

`WF_TYPE_DEVICE_INFO` remains a reserved v1 packet type. The constant and framing behavior are part of the frozen v1 contract, but payload generation and payload parsing are out of scope for Phase 1 implementation. A v1 bridge must still validate framing and CRC, surface the packet type, and ignore device-info payload semantics rather than rejecting the transport stream.

## 6. Golden packet vectors

### 6.1 Keyframe vector

- Header: `aa 55 01 02 34 12 04 03 02 01 28 00`
- Payload: `34 12 04 03 02 01 01 49 44 4c 45 00 00 00 00 00 00 00 00 00 00 00 00 00 00 20 00 64 00 00 00 40 1f 00 00 00 1e 00 00 07`
- CRC: `96 85` (`0x8596`, little-endian on wire)
- Full packet: `aa 55 01 02 34 12 04 03 02 01 28 00 34 12 04 03 02 01 01 49 44 4c 45 00 00 00 00 00 00 00 00 00 00 00 00 00 00 20 00 64 00 00 00 40 1f 00 00 00 1e 00 00 07 96 85`

### 6.2 Delta vector

- Header: `aa 55 01 01 35 12 68 03 02 01 07 00`
- Payload: `f5 20 1e 00 00 f7 0a`
- `f5`: system delta tag for field ID `0x05` (`heap_free_bytes`)
- `20 1e 00 00`: `heap_free_bytes = 0x00001e20` (`7712`)
- `f7`: system delta tag for field ID `0x07` (`cpu_utilization_pct`)
- `0a`: `cpu_utilization_pct = 10`
- CRC: `36 2d` (`0x2D36`, little-endian on wire)
- Full packet: `aa 55 01 01 35 12 68 03 02 01 07 00 f5 20 1e 00 00 f7 0a 36 2d`

### 6.3 Corrupted vector

- Start from the delta vector and flip the final CRC byte from `2d` to `d2`.
- Full packet: `aa 55 01 01 35 12 68 03 02 01 07 00 f5 20 1e 00 00 f7 0a 36 d2`
- Expected decoder behavior: reject packet, increment drop count, do not emit `DecodedPacket`
