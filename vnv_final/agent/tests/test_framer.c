#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include "framer.h"
#include "wire_format.h"

/**
 * @file test_framer.c
 * @brief Unit tests for frame_packet() — Task 4 of TASK_QUEUE.md.
 *
 * Changes from original:
 *   - seq_num parameter REMOVED from frame_packet() call (framer manages it).
 *   - Tests now verify the internally-managed sequence counter behaviour.
 *   - Added: sequence counter wraps 65535 → 0 boundary test.
 *   - Added: NULL/small-buffer error-path test.
 */

int main(void)
{
    uint8_t  payload[] = {0xDE, 0xAD};  /* 2-byte test payload */
    uint8_t  out_buf[64];
    uint16_t out_len;
    uint16_t crc_received;
    uint16_t crc_calculated;

    printf("--- Running Framer Unit Tests ---\n\n");

    /* ----------------------------------------------------------------
     * Reset sequence counter so tests are deterministic regardless of
     * which other test ran before this binary.
     * --------------------------------------------------------------- */
    framer_reset_sequence();

    /* ----------------------------------------------------------------
     * Test 1 — Happy path: correct total length
     *   12 (header) + 2 (payload) + 2 (CRC) = 16 bytes
     * --------------------------------------------------------------- */
    out_len = frame_packet(payload, (uint16_t)sizeof(payload),
                           WF_TYPE_DELTA, 1000u,
                           out_buf, (uint16_t)sizeof(out_buf));

    assert(out_len == 16u);
    printf("Test 1 — Total length is 16 bytes:                   PASSED\n");

    /* ----------------------------------------------------------------
     * Test 2 — Sync bytes at offsets 0 and 1
     * --------------------------------------------------------------- */
    assert(out_buf[0] == WF_SYNC_0);   /* 0xAA */
    assert(out_buf[1] == WF_SYNC_1);   /* 0x55 */
    printf("Test 2 — Sync bytes (0xAA, 0x55) correct:           PASSED\n");

    /* ----------------------------------------------------------------
     * Test 3 — Protocol version at offset 2
     * --------------------------------------------------------------- */
    assert(out_buf[2] == WF_PROTOCOL_VERSION);  /* 0x01 */
    printf("Test 3 — Protocol version (0x01) correct:           PASSED\n");

    /* ----------------------------------------------------------------
     * Test 4 — Packet type at offset 3
     * --------------------------------------------------------------- */
    assert(out_buf[3] == WF_TYPE_DELTA);         /* 0x01 */
    printf("Test 4 — Packet type (DELTA=0x01) correct:          PASSED\n");

    /* ----------------------------------------------------------------
     * Test 5 — Sequence number starts at 0 after reset (little-endian)
     *   First call after framer_reset_sequence() → seq=0
     *   0 in little-endian: byte[4]=0x00, byte[5]=0x00
     * --------------------------------------------------------------- */
    assert(out_buf[4] == 0x00u);
    assert(out_buf[5] == 0x00u);
    printf("Test 5 — First seq number is 0 (little-endian):     PASSED\n");

    /* ----------------------------------------------------------------
     * Test 6 — Second call increments sequence to 1
     * --------------------------------------------------------------- */
    out_len = frame_packet(payload, (uint16_t)sizeof(payload),
                           WF_TYPE_DELTA, 2000u,
                           out_buf, (uint16_t)sizeof(out_buf));
    assert(out_len == 16u);
    assert(out_buf[4] == 0x01u);
    assert(out_buf[5] == 0x00u);
    printf("Test 6 — Second call seq incremented to 1:          PASSED\n");

    /* ----------------------------------------------------------------
     * Test 7 — Timestamp at offsets 6-9 (little-endian)
     *   We pass timestamp_ticks = 2000 = 0x000007D0
     *   Little-endian: D0 07 00 00
     * --------------------------------------------------------------- */
    assert(out_buf[6] == 0xD0u);
    assert(out_buf[7] == 0x07u);
    assert(out_buf[8] == 0x00u);
    assert(out_buf[9] == 0x00u);
    printf("Test 7 — Timestamp (2000) little-endian correct:    PASSED\n");

    /* ----------------------------------------------------------------
     * Test 8 — Payload length at offsets 10-11 (little-endian)
     *   2 bytes payload → byte[10]=0x02, byte[11]=0x00
     * --------------------------------------------------------------- */
    assert(out_buf[10] == 0x02u);
    assert(out_buf[11] == 0x00u);
    printf("Test 8 — Payload length (2) little-endian correct:  PASSED\n");

    /* ----------------------------------------------------------------
     * Test 9 — Payload bytes at offsets 12-13
     * --------------------------------------------------------------- */
    assert(out_buf[12] == 0xDEu);
    assert(out_buf[13] == 0xADu);
    printf("Test 9 — Payload bytes (0xDE, 0xAD) correct:        PASSED\n");

    /* ----------------------------------------------------------------
     * Test 10 — CRC integrity check
     *   Re-compute CRC over [VERSION..PAYLOAD] and compare with the
     *   two CRC bytes the framer placed at the tail of the packet.
     * --------------------------------------------------------------- */
    crc_received   = (uint16_t)out_buf[14] | ((uint16_t)out_buf[15] << 8u);
    crc_calculated = crc16_ccitt(&out_buf[2], (uint16_t)(WF_HEADER_SIZE - 2u + sizeof(payload)));
    assert(crc_received == crc_calculated);
    printf("Test 10 — CRC-16 appended at tail is valid:         PASSED\n");

    /* ----------------------------------------------------------------
     * Test 11 — Error path: NULL output buffer → returns 0
     * --------------------------------------------------------------- */
    out_len = frame_packet(payload, (uint16_t)sizeof(payload),
                           WF_TYPE_DELTA, 0u,
                           NULL, 64u);
    assert(out_len == 0u);
    printf("Test 11 — NULL buffer returns 0 (error path):       PASSED\n");

    /* ----------------------------------------------------------------
     * Test 12 — Error path: output buffer too small → returns 0
     * --------------------------------------------------------------- */
    uint8_t tiny_buf[4];
    out_len = frame_packet(payload, (uint16_t)sizeof(payload),
                           WF_TYPE_DELTA, 0u,
                           tiny_buf, (uint16_t)sizeof(tiny_buf));
    assert(out_len == 0u);
    printf("Test 12 — Too-small buffer returns 0 (error path):  PASSED\n");

    /* ----------------------------------------------------------------
     * Test 13 — Boundary: sequence wraps from 65535 → 0
     *   Drive the counter to 65535, call once more, expect 0.
     * --------------------------------------------------------------- */
    framer_reset_sequence();
    /* Burn through 65535 increments using a zero-length payload (fastest) */
    {
        uint8_t dummy_buf[WF_OVERHEAD]; /* 14 bytes: just header + CRC, no payload */
        for (uint32_t n = 0u; n < 65535u; n++) {
            frame_packet(NULL, 0u, WF_TYPE_DELTA, 0u,
                         dummy_buf, (uint16_t)sizeof(dummy_buf));
        }
        /* Next call: seq was 65535, increments to 0 (uint16 wrap) */
        out_len = frame_packet(NULL, 0u, WF_TYPE_DELTA, 0u,
                               dummy_buf, (uint16_t)sizeof(dummy_buf));
        /* seq used for THIS packet was 65535 (0xFFFF) */
        assert(dummy_buf[4] == 0xFFu);
        assert(dummy_buf[5] == 0xFFu);
        /* internal counter is now 0 — verify on the next call */
        out_len = frame_packet(NULL, 0u, WF_TYPE_DELTA, 0u,
                               dummy_buf, (uint16_t)sizeof(dummy_buf));
        assert(dummy_buf[4] == 0x00u);
        assert(dummy_buf[5] == 0x00u);
    }
    printf("Test 13 — Seq wraps 65535 → 0 correctly:            PASSED\n");

    printf("\n--- ALL FRAMER TESTS PASSED ---\n");
    return 0;
}
