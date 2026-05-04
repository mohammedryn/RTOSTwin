#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../../../agent/core/wire_format.h"
#include "../core/framer.h"

static void test_crc_vector(void)
{
    const uint8_t vector[] = "123456789";
    assert(crc16_ccitt(vector, 9u) == 0x29B1u);
}

static void test_delta_golden_packet(void)
{
    const uint8_t payload[] = {0xF5u, 0x20u, 0x1Eu, 0x00u, 0x00u, 0xF7u, 0x0Au};
    const uint8_t expected[] = {
        0xAAu, 0x55u, 0x01u, 0x01u, 0x35u, 0x12u, 0x68u, 0x03u, 0x02u, 0x01u,
        0x07u, 0x00u, 0xF5u, 0x20u, 0x1Eu, 0x00u, 0x00u, 0xF7u, 0x0Au, 0x36u, 0x2Du
    };
    uint8_t out_buf[32];
    uint16_t out_len = frame_packet(payload,
                                    (uint16_t)sizeof(payload),
                                    WF_TYPE_DELTA,
                                    0x1235u,
                                    0x01020368u,
                                    out_buf,
                                    (uint16_t)sizeof(out_buf));

    assert(out_len == (uint16_t)sizeof(expected));
    assert(memcmp(out_buf, expected, sizeof(expected)) == 0);
}

static void test_null_payload_rules(void)
{
    uint8_t out_buf[16];

    assert(frame_packet(NULL, 0u, WF_TYPE_DELTA, 0u, 0u, out_buf, (uint16_t)sizeof(out_buf)) == 14u);
    assert(frame_packet(NULL, 1u, WF_TYPE_DELTA, 0u, 0u, out_buf, (uint16_t)sizeof(out_buf)) == 0u);
    assert(frame_packet((const uint8_t *)"A", 1u, WF_TYPE_DELTA, 0u, 0u, NULL, 16u) == 0u);
}

int main(void)
{
    printf("--- Running Framer Unit Tests ---\n\n");

    test_crc_vector();
    printf("  [PASS] CRC reference vector\n");

    test_delta_golden_packet();
    printf("  [PASS] Golden delta packet bytes\n");

    test_null_payload_rules();
    printf("  [PASS] NULL/error path rules\n");

    printf("\nALL FRAMER TESTS PASSED\n");
    return 0;
}
