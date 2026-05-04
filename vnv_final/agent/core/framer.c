#include "framer.h"
#include "wire_format.h"
#include <stddef.h>

/**
 * @file framer.c
 * @brief Implementation of packet framing and CRC logic.
 *
 * TECH_SPEC §3.3: The framer owns the sequence counter.
 * It is a static file-scope variable, invisible to all other modules.
 */

/* ------------------------------------------------------------------
 * Internal state — NOT visible outside this file (static keyword)
 * ------------------------------------------------------------------ */

/** Auto-incrementing sequence number. Wraps at 65535 → 0. */
static uint16_t s_seq_num = 0u;

/* ------------------------------------------------------------------
 * crc16_ccitt()
 *
 * Algorithm: CRC-16-CCITT
 *   Polynomial : 0x1021
 *   Initial    : 0xFFFF
 *   Reflection : None
 *   Final XOR  : None
 *
 * Test vector: crc16_ccitt("123456789", 9) == 0x29B1
 * ------------------------------------------------------------------ */
uint16_t crc16_ccitt(const uint8_t *data, uint16_t len)
{
    uint16_t crc = WF_CRC_INIT; /* Start at 0xFFFF */

    for (uint16_t i = 0u; i < len; i++) {
        /* XOR the current byte into the high byte of the CRC register */
        crc ^= (uint16_t)((uint16_t)data[i] << 8u);

        /* Process each of the 8 bits */
        for (uint8_t bit = 0u; bit < 8u; bit++) {
            if (crc & 0x8000u) {
                /* MSB is 1: shift left and XOR with polynomial */
                crc = (uint16_t)((crc << 1u) ^ WF_CRC_POLY);
            } else {
                /* MSB is 0: just shift left */
                crc = (uint16_t)(crc << 1u);
            }
        }
    }

    return crc;
}

/* ------------------------------------------------------------------
 * frame_packet()
 *
 * Wire format (little-endian multi-byte fields):
 *
 *  Offset  Field        Size   Notes
 *  ------  -----------  ----   ----------------------------------
 *   0      SYNC_0        1     0xAA  (fixed)
 *   1      SYNC_1        1     0x55  (fixed)
 *   2      VERSION       1     0x01  (protocol v1)
 *   3      TYPE          1     0x01=delta, 0x02=keyframe
 *   4-5    SEQ_NUM       2     uint16_t, auto-incremented here
 *   6-9    TIMESTAMP     4     xTaskGetTickCount() value
 *  10-11   LENGTH        2     payload byte count
 *  12..N   PAYLOAD       N     encoded snapshot data
 *  N+1..N+2 CRC_16       2     CRC-CCITT over bytes 2..N
 *
 * CRC covers: VERSION (offset 2) through end of PAYLOAD.
 * ------------------------------------------------------------------ */
uint16_t frame_packet(const uint8_t *payload,
                      uint16_t       payload_len,
                      uint8_t        packet_type,
                      uint32_t       timestamp_ticks,
                      uint8_t       *out_buf,
                      uint16_t       out_buf_size)
{
    /* 1. Calculate total packet size: header(12) + payload(N) + CRC(2) */
    uint16_t total_len = (uint16_t)(WF_OVERHEAD + payload_len);

    /* 2. Guard: reject NULL buffer or insufficient size */
    if ((out_buf == NULL) || (out_buf_size < total_len)) {
        return 0u;
    }

    /* 3. Write sync bytes (offsets 0-1) */
    out_buf[0] = WF_SYNC_0;          /* 0xAA */
    out_buf[1] = WF_SYNC_1;          /* 0x55 */

    /* 4. Protocol version (offset 2) */
    out_buf[2] = WF_PROTOCOL_VERSION; /* 0x01 */

    /* 5. Packet type (offset 3) */
    out_buf[3] = packet_type;

    /* 6. Sequence number (offsets 4-5) — little-endian, then increment */
    out_buf[4] = (uint8_t)(s_seq_num & 0xFFu);
    out_buf[5] = (uint8_t)(s_seq_num >> 8u);
    s_seq_num++;  /* uint16_t overflow wraps 65535 → 0 automatically in C */

    /* 7. Timestamp in RTOS ticks (offsets 6-9) — little-endian */
    out_buf[6] = (uint8_t)(timestamp_ticks & 0xFFu);
    out_buf[7] = (uint8_t)(timestamp_ticks >> 8u);
    out_buf[8] = (uint8_t)(timestamp_ticks >> 16u);
    out_buf[9] = (uint8_t)(timestamp_ticks >> 24u);

    /* 8. Payload length (offsets 10-11) — little-endian */
    out_buf[10] = (uint8_t)(payload_len & 0xFFu);
    out_buf[11] = (uint8_t)(payload_len >> 8u);

    /* 9. Copy payload bytes starting at offset 12 */
    if ((payload != NULL) && (payload_len > 0u)) {
        for (uint16_t i = 0u; i < payload_len; i++) {
            out_buf[12u + i] = payload[i];
        }
    }

    /* 10. Compute CRC-16-CCITT over [VERSION..end of PAYLOAD]
     *     That is: bytes at offsets 2 through (12 + payload_len - 1)
     *     Size of that region = WF_HEADER_SIZE - 2 + payload_len = 10 + payload_len */
    uint16_t crc = crc16_ccitt(&out_buf[2],
                                (uint16_t)(WF_HEADER_SIZE - 2u + payload_len));

    /* 11. Append CRC at the tail — little-endian */
    out_buf[WF_HEADER_SIZE + payload_len]      = (uint8_t)(crc & 0xFFu);
    out_buf[WF_HEADER_SIZE + payload_len + 1u] = (uint8_t)(crc >> 8u);

    return total_len;
}

/* ------------------------------------------------------------------
 * framer_reset_sequence()
 * ------------------------------------------------------------------ */
void framer_reset_sequence(void)
{
    s_seq_num = 0u;
}
