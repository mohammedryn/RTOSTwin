#include "framer.h"

#include "wire_format.h"

uint16_t crc16_ccitt(const uint8_t *data, uint16_t len)
{
    uint16_t crc = WF_CRC_INIT;
    uint16_t i;
    uint8_t j;

    if ((data == (const uint8_t *)0) && (len > 0U)) {
        return 0U;
    }

    for (i = 0U; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (j = 0U; j < 8U; j++) {
            if ((crc & 0x8000U) != 0U) {
                crc = (uint16_t)((crc << 1) ^ WF_CRC_POLY);
            } else {
                crc <<= 1;
            }
        }
    }

    return crc;
}

uint16_t frame_packet(const uint8_t *payload, uint16_t payload_len,
                      uint8_t *out_buf, uint16_t out_buf_size)
{
    static uint16_t s_sequence_num = 0U;
    uint16_t i;
    uint16_t crc;
    uint16_t packet_len;
    uint32_t timestamp_ticks = 0U;

    if (out_buf == (uint8_t *)0) {
        return 0U;
    }

    if ((payload == (const uint8_t *)0) && (payload_len > 0U)) {
        return 0U;
    }

    if ((uint16_t)(WF_OVERHEAD + payload_len) > WF_MAX_PACKET_SIZE) {
        return 0U;
    }

    packet_len = (uint16_t)(WF_OVERHEAD + payload_len);
    if (out_buf_size < packet_len) {
        return 0U;
    }

    out_buf[0] = (uint8_t)WF_SYNC_0;
    out_buf[1] = (uint8_t)WF_SYNC_1;
    out_buf[2] = (uint8_t)WF_PROTOCOL_VERSION;
    out_buf[3] = (uint8_t)WF_TYPE_DELTA;

    out_buf[4] = (uint8_t)(s_sequence_num & 0x00FFU);
    out_buf[5] = (uint8_t)((s_sequence_num >> 8) & 0x00FFU);

    out_buf[6] = (uint8_t)(timestamp_ticks & 0x000000FFUL);
    out_buf[7] = (uint8_t)((timestamp_ticks >> 8) & 0x000000FFUL);
    out_buf[8] = (uint8_t)((timestamp_ticks >> 16) & 0x000000FFUL);
    out_buf[9] = (uint8_t)((timestamp_ticks >> 24) & 0x000000FFUL);

    out_buf[10] = (uint8_t)(payload_len & 0x00FFU);
    out_buf[11] = (uint8_t)((payload_len >> 8) & 0x00FFU);

    for (i = 0U; i < payload_len; i++) {
        out_buf[WF_HEADER_SIZE + i] = payload[i];
    }

    crc = crc16_ccitt(&out_buf[2], (uint16_t)((WF_HEADER_SIZE - 2U) + payload_len));
    out_buf[WF_HEADER_SIZE + payload_len] = (uint8_t)(crc & 0x00FFU);
    out_buf[WF_HEADER_SIZE + payload_len + 1U] = (uint8_t)((crc >> 8) & 0x00FFU);

    s_sequence_num++;

    return packet_len;
}
