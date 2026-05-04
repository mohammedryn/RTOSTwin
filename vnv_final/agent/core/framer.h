#ifndef RTOSTWIN_FRAMER_H
#define RTOSTWIN_FRAMER_H

#include <stdint.h>

/**
 * @file framer.h
 * @brief Packet framing and CRC logic for the RTOSTwin agent.
 *
 * The canonical v1 protocol contract is frozen in the root repo files
 * docs/wire_format_spec.md and agent/core/wire_format.h.
 */

uint16_t crc16_ccitt(const uint8_t *data, uint16_t len);

uint16_t frame_packet(const uint8_t *payload,
                      uint16_t payload_len,
                      uint8_t packet_type,
                      uint16_t sequence_num,
                      uint32_t timestamp_ticks,
                      uint8_t *out_buf,
                      uint16_t out_buf_size);

#endif /* RTOSTWIN_FRAMER_H */
