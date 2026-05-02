#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include "framer.h"
#include "wire_format.h"

/**
 * @file test_framer.c
 * @brief Unit test for the packet framing logic.
 * 
 * Target: Verify that frame_packet() builds the correct 14-byte 
 * header + CRC around a simple payload.
 */

int main(void) {
    uint8_t payload[] = {0xDE, 0xAD}; /* A simple 2-byte "letter" */
    uint8_t out_buf[64];
    uint16_t out_len;

    printf("--- Running Framer Unit Test ---\n");

    /* seq_num=500 (0x01F4), timestamp=1000 (0x000003E8) */
    out_len = frame_packet(payload, sizeof(payload), WF_TYPE_DELTA, 500, 1000, out_buf, sizeof(out_buf));

    /* 1. Check total length: 12 (header) + 2 (payload) + 2 (CRC) = 16 */
    assert(out_len == 16);
    printf("Step 1: Length is correct (16 bytes) ✅\n");

    /* 2. Check Sync Bytes (Bytes 0-1) */
    assert(out_buf[0] == 0xAA && out_buf[1] == 0x55);
    printf("Step 2: Sync Bytes (0xAA55) are correct ✅\n");

    /* 3. Check Sequence Number (Bytes 4-5) - Remember Little Endian! */
    /* 500 = 0x01F4. So out_buf[4] should be 0xF4, out_buf[5] should be 0x01. */
    assert(out_buf[4] == 0xF4);
    assert(out_buf[5] == 0x01);
    printf("Step 3: Sequence Number (500) is correct in Little Endian ✅\n");

    /* 4. Check Payload length (Bytes 10-11) */
    assert(out_buf[10] == 0x02);
    assert(out_buf[11] == 0x00);
    printf("Step 4: Payload length (2) is correct ✅\n");

    printf("RESULT: PASSED ✅\n");

    return 0;
}
