#ifndef RTOSTWIN_FRAMER_H
#define RTOSTWIN_FRAMER_H

#include <stdint.h>

/**
 * @file framer.h
 * @brief Packet framing and CRC logic for the RTOSTwin agent.
 *
 * Provides functions to wrap encoded payloads into wire-format packets
 * and calculate the CRC-16-CCITT checksum for data integrity.
 *
 * NOTE (TECH_SPEC §3.3): The framer maintains an internal static uint16_t
 * sequence counter. Callers do NOT pass a sequence number — the framer
 * increments it automatically and wraps naturally at 65535 → 0.
 */

/**
 * @brief Calculate the CRC-16-CCITT checksum for a block of data.
 *
 * Uses polynomial 0x1021 and initial value 0xFFFF.
 * No bit reflection. No final XOR.
 *
 * Test vector: crc16_ccitt("123456789", 9) == 0x29B1
 *
 * @param data Pointer to the data buffer.
 * @param len  Length of the data in bytes.
 * @return     16-bit CRC checksum.
 */
uint16_t crc16_ccitt(const uint8_t *data, uint16_t len);

/**
 * @brief Construct a complete wire-format packet from a payload.
 *
 * Wraps the payload with sync bytes (0xAA 0x55), protocol version,
 * packet type, an auto-incremented sequence number, timestamp, payload
 * length, and a CRC-16-CCITT checksum over VERSION..PAYLOAD.
 *
 * The sequence number is managed internally by this function.
 * It starts at 0 and increments by 1 on every successful call,
 * wrapping from 65535 back to 0 via natural uint16_t overflow.
 *
 * @param payload         Pointer to the encoded payload buffer.
 * @param payload_len     Length of the payload in bytes.
 * @param packet_type     Packet type: WF_TYPE_DELTA or WF_TYPE_KEYFRAME.
 * @param timestamp_ticks Current RTOS tick count (xTaskGetTickCount()).
 * @param out_buf         Output buffer to store the complete framed packet.
 * @param out_buf_size    Size of the output buffer in bytes.
 * @return                Total size of the framed packet in bytes, or 0 on error.
 */
uint16_t frame_packet(const uint8_t *payload,
                      uint16_t       payload_len,
                      uint8_t        packet_type,
                      uint32_t       timestamp_ticks,
                      uint8_t       *out_buf,
                      uint16_t       out_buf_size);

/**
 * @brief Reset the internal sequence counter back to 0.
 *
 * Call this only when re-initialising the transport (e.g., after a
 * device reset). The bridge detects the counter jump as a gap, so
 * use sparingly.
 */
void framer_reset_sequence(void);

#endif /* RTOSTWIN_FRAMER_H */
