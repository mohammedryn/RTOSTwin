#ifndef RTOSTWIN_FRAMER_H
#define RTOSTWIN_FRAMER_H

#include <stdint.h>

/**
 * Build a complete wire-format packet from an encoded payload.
 *
 * @param payload Pointer to encoded payload bytes.
 * @param payload_len Number of bytes in payload.
 * @param out_buf Pointer to destination packet buffer.
 * @param out_buf_size Capacity of destination packet buffer in bytes.
 * @return Total packet length in bytes, or 0 if framing fails.
 */
uint16_t frame_packet(const uint8_t *payload, uint16_t payload_len,
                      uint8_t *out_buf, uint16_t out_buf_size);

/**
 * Compute CRC-16-CCITT over a byte buffer.
 *
 * @param data Pointer to input bytes.
 * @param len Number of input bytes.
 * @return Computed CRC-16-CCITT value.
 */
uint16_t crc16_ccitt(const uint8_t *data, uint16_t len);

#endif /* RTOSTWIN_FRAMER_H */
