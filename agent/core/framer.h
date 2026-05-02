#ifndef RTOSTWIN_FRAMER_H
#define RTOSTWIN_FRAMER_H

#include <stdint.h>

/**
 * @file framer.h
 * @brief Packet framing and CRC logic for the RTOSTwin agent.
 * 
 * Provides functions to wrap encoded payloads into wire-format packets
 * and calculate the CRC-16-CCITT checksum for data integrity.
 */

/**
 * @brief Calculate the CRC-16-CCITT checksum for a block of data.
 * 
 * Uses polynomial 0x1021 and initial value 0xFFFF.
 * 
 * @param data Pointer to the data buffer.
 * @param len Length of the data in bytes.
 * @return 16-bit CRC checksum.
 */
uint16_t crc16_ccitt(const uint8_t *data, uint16_t len);

/**
 * @brief Construct a complete wire-format packet from a payload.
 * 
 * Wraps the payload with sync bytes, header info (type, seq, timestamp, length),
 * and appends the calculated CRC-16-CCITT checksum.
 * 
 * @param payload Pointer to the encoded payload buffer.
 * @param payload_len Length of the payload in bytes.
 * @param pkt_type Packet type (Delta/Keyframe).
 * @param seq_num Sequence number for this packet.
 * @param timestamp_ms Current system timestamp in ms.
 * @param out_buf Output buffer to store the framed packet.
 * @param out_buf_size Size of the output buffer.
 * @return Total size of the framed packet in bytes, or 0 on error.
 */
uint16_t frame_packet(const uint8_t *payload,
                      uint16_t       payload_len,
                      uint8_t        pkt_type,
                      uint16_t       seq_num,
                      uint32_t       timestamp_ms,
                      uint8_t       *out_buf,
                      uint16_t       out_buf_size);

#endif /* RTOSTWIN_FRAMER_H */
