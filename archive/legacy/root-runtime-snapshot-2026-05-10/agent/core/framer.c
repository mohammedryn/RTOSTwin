#include "framer.h"
#include "wire_format.h"
#include <stddef.h>

/**
 * @file framer.c
 * @brief Implementation of packet framing and CRC logic.
 */

/* ------------------------------------------------------------------
 * crc16_ccitt()
 * ------------------------------------------------------------------ */
uint16_t crc16_ccitt(const uint8_t *data, uint16_t len)
{
    /* Use the initial value and polynomial defined in wire_format.h */
    uint16_t crc = WF_CRC_INIT;

    for (uint16_t i = 0; i < len; i++) {
        /* XOR the incoming byte into the upper byte of the CRC register */
        crc ^= (uint16_t)((uint16_t)data[i] << 8u);

        /* Process each bit in the byte */
        for (uint8_t bit = 0; bit < 8u; bit++) {
            /* If the highest bit is 1, XOR with the polynomial after shifting */
            if (crc & 0x8000u) {
                crc = (uint16_t)((crc << 1u) ^ WF_CRC_POLY);
            } else {
                /* Otherwise, just shift left */
                crc = (uint16_t)(crc << 1u);
            }
        }
    }

    return crc;
}

/* ------------------------------------------------------------------
 * frame_packet()
 * ------------------------------------------------------------------ */
uint16_t frame_packet(const uint8_t *payload,
                      uint16_t       payload_len,
                      uint8_t        pkt_type,
                      uint16_t       seq_num,
                      uint32_t       timestamp_ms,
                      uint8_t       *out_buf,
                      uint16_t       out_buf_size)
{
    /* 1. Calculate the total size of the final packet */
    uint16_t total_len = (uint16_t)(WF_OVERHEAD + payload_len);

    /* 2. Safety checks (CODING_RULES.md §4) */
    if ((out_buf == NULL) || (out_buf_size < total_len)) {
        return 0u;
    }

    /* 3. Fill the header fields (Offset 0-11) */
    out_buf[0] = WF_SYNC_0;
    out_buf[1] = WF_SYNC_1;
    out_buf[2] = WF_PROTOCOL_VERSION;
    out_buf[3] = pkt_type;
    
    /* Sequence number (Offset 4-5) - Little Endian */
    out_buf[4] = (uint8_t)(seq_num & 0xFFu);
    out_buf[5] = (uint8_t)(seq_num >> 8u);

    /* Timestamp (Offset 6-9) - Little Endian */
    out_buf[6] = (uint8_t)(timestamp_ms & 0xFFu);
    out_buf[7] = (uint8_t)(timestamp_ms >> 8u);
    out_buf[8] = (uint8_t)(timestamp_ms >> 16u);
    out_buf[9] = (uint8_t)(timestamp_ms >> 24u);

    /* Payload length (Offset 10-11) - Little Endian */
    out_buf[10] = (uint8_t)(payload_len & 0xFFu);
    out_buf[11] = (uint8_t)(payload_len >> 8u);

    /* 4. Copy the payload (Offset 12+) */
    if ((payload != NULL) && (payload_len > 0u)) {
        for (uint16_t i = 0; i < payload_len; i++) {
            out_buf[12u + i] = payload[i];
        }
    }

    /* 5. Calculate and append the CRC-16-CCITT at the end (Last 2 bytes) */
    /* CRC covers from VERSION (offset 2) through end of PAYLOAD */
    uint16_t crc = crc16_ccitt(&out_buf[2], (uint16_t)(WF_HEADER_SIZE - 2u + payload_len));
    
    /* CRC (Little Endian) */
    out_buf[WF_HEADER_SIZE + payload_len]      = (uint8_t)(crc & 0xFFu);
    out_buf[WF_HEADER_SIZE + payload_len + 1u] = (uint8_t)(crc >> 8u);

    return total_len;
}
