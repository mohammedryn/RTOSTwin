#include "framer.h"
#include "../../../agent/core/wire_format.h"
#include <stddef.h>

uint16_t crc16_ccitt(const uint8_t *data, uint16_t len)
{
    uint16_t crc = WF_CRC_INIT;

    for (uint16_t i = 0u; i < len; i++) {
        crc ^= (uint16_t)((uint16_t)data[i] << 8u);
        for (uint8_t bit = 0u; bit < 8u; bit++) {
            if (crc & 0x8000u) {
                crc = (uint16_t)((crc << 1u) ^ WF_CRC_POLY);
            } else {
                crc = (uint16_t)(crc << 1u);
            }
        }
    }

    return crc;
}

uint16_t frame_packet(const uint8_t *payload,
                      uint16_t payload_len,
                      uint8_t packet_type,
                      uint16_t sequence_num,
                      uint32_t timestamp_ticks,
                      uint8_t *out_buf,
                      uint16_t out_buf_size)
{
    uint16_t total_len = (uint16_t)(WF_OVERHEAD + payload_len);

    if ((out_buf == NULL) || (out_buf_size < total_len)) {
        return 0u;
    }

    if ((payload_len > 0u) && (payload == NULL)) {
        return 0u;
    }

    out_buf[0] = WF_SYNC_0;
    out_buf[1] = WF_SYNC_1;
    out_buf[2] = WF_PROTOCOL_VERSION;
    out_buf[3] = packet_type;

    out_buf[4] = (uint8_t)(sequence_num & 0xFFu);
    out_buf[5] = (uint8_t)(sequence_num >> 8u);

    out_buf[6] = (uint8_t)(timestamp_ticks & 0xFFu);
    out_buf[7] = (uint8_t)((timestamp_ticks >> 8u) & 0xFFu);
    out_buf[8] = (uint8_t)((timestamp_ticks >> 16u) & 0xFFu);
    out_buf[9] = (uint8_t)((timestamp_ticks >> 24u) & 0xFFu);

    out_buf[10] = (uint8_t)(payload_len & 0xFFu);
    out_buf[11] = (uint8_t)(payload_len >> 8u);

    for (uint16_t i = 0u; i < payload_len; i++) {
        out_buf[WF_HEADER_SIZE + i] = payload[i];
    }

    {
        uint16_t crc = crc16_ccitt(&out_buf[2], (uint16_t)(WF_HEADER_SIZE - 2u + payload_len));
        out_buf[WF_HEADER_SIZE + payload_len] = (uint8_t)(crc & 0xFFu);
        out_buf[WF_HEADER_SIZE + payload_len + 1u] = (uint8_t)(crc >> 8u);
    }

    return total_len;
}
