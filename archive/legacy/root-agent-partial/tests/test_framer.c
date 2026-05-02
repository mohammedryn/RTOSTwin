#include "unity.h"

#include "../core/framer.h"
#include "../core/wire_format.h"

#include <stdint.h>

void setUp(void)
{
}

void tearDown(void)
{
}

static uint16_t read_u16_le(const uint8_t *buf)
{
    return (uint16_t)buf[0] | ((uint16_t)buf[1] << 8);
}

static uint32_t read_u32_le(const uint8_t *buf)
{
    return (uint32_t)buf[0]
         | ((uint32_t)buf[1] << 8)
         | ((uint32_t)buf[2] << 16)
         | ((uint32_t)buf[3] << 24);
}

static void test_frame_packet_happy_path_sets_header_payload_and_crc(void)
{
    static const uint8_t k_payload[] = { 0x10U, 0x20U, 0x30U };
    uint8_t out_buf[64] = { 0U };
    uint16_t packet_len = frame_packet(k_payload, (uint16_t)sizeof(k_payload),
                                       out_buf, (uint16_t)sizeof(out_buf));
    uint16_t expected_crc;
    uint16_t packet_crc;

    TEST_ASSERT_EQUAL_UINT16((uint16_t)(WF_OVERHEAD + sizeof(k_payload)), packet_len);

    TEST_ASSERT_EQUAL_UINT8((uint8_t)WF_SYNC_0, out_buf[0]);
    TEST_ASSERT_EQUAL_UINT8((uint8_t)WF_SYNC_1, out_buf[1]);
    TEST_ASSERT_EQUAL_UINT8((uint8_t)WF_PROTOCOL_VERSION, out_buf[2]);

    /* Current implementation always emits delta packets. */
    TEST_ASSERT_EQUAL_UINT8((uint8_t)WF_TYPE_DELTA, out_buf[3]);

    /* Current implementation keeps timestamp at 0. */
    TEST_ASSERT_EQUAL_UINT32(0UL, read_u32_le(&out_buf[6]));

    TEST_ASSERT_EQUAL_UINT16((uint16_t)sizeof(k_payload), read_u16_le(&out_buf[10]));

    TEST_ASSERT_EQUAL_UINT8(k_payload[0], out_buf[WF_HEADER_SIZE + 0U]);
    TEST_ASSERT_EQUAL_UINT8(k_payload[1], out_buf[WF_HEADER_SIZE + 1U]);
    TEST_ASSERT_EQUAL_UINT8(k_payload[2], out_buf[WF_HEADER_SIZE + 2U]);

    expected_crc = crc16_ccitt(&out_buf[2], (uint16_t)((WF_HEADER_SIZE - 2U) + sizeof(k_payload)));
    packet_crc = read_u16_le(&out_buf[WF_HEADER_SIZE + sizeof(k_payload)]);
    TEST_ASSERT_EQUAL_HEX16(expected_crc, packet_crc);
}

static void test_frame_packet_sequence_increments_between_calls(void)
{
    static const uint8_t k_payload[] = { 0xAAU };
    uint8_t out_first[32] = { 0U };
    uint8_t out_second[32] = { 0U };
    uint16_t seq_first;
    uint16_t seq_second;
    uint16_t len_first = frame_packet(k_payload, (uint16_t)sizeof(k_payload),
                                      out_first, (uint16_t)sizeof(out_first));
    uint16_t len_second = frame_packet(k_payload, (uint16_t)sizeof(k_payload),
                                       out_second, (uint16_t)sizeof(out_second));

    TEST_ASSERT_NOT_EQUAL(0U, len_first);
    TEST_ASSERT_NOT_EQUAL(0U, len_second);

    seq_first = read_u16_le(&out_first[4]);
    seq_second = read_u16_le(&out_second[4]);
    TEST_ASSERT_EQUAL_UINT16((uint16_t)(seq_first + 1U), seq_second);
}

static void test_frame_packet_null_output_buffer_returns_zero(void)
{
    static const uint8_t k_payload[] = { 0x01U };
    uint16_t packet_len = frame_packet(k_payload, (uint16_t)sizeof(k_payload),
                                       (uint8_t *)0, 16U);

    TEST_ASSERT_EQUAL_UINT16(0U, packet_len);
}

static void test_frame_packet_null_payload_with_nonzero_length_returns_zero(void)
{
    uint8_t out_buf[32] = { 0U };
    uint16_t packet_len = frame_packet((const uint8_t *)0, 1U,
                                       out_buf, (uint16_t)sizeof(out_buf));

    TEST_ASSERT_EQUAL_UINT16(0U, packet_len);
}

static void test_frame_packet_output_buffer_too_small_returns_zero(void)
{
    static const uint8_t k_payload[] = { 0x11U, 0x22U, 0x33U, 0x44U };
    uint8_t out_buf[16] = { 0U };
    uint16_t required_len = (uint16_t)(WF_OVERHEAD + sizeof(k_payload));
    uint16_t packet_len = frame_packet(k_payload, (uint16_t)sizeof(k_payload),
                                       out_buf, (uint16_t)(required_len - 1U));

    TEST_ASSERT_EQUAL_UINT16(0U, packet_len);
}

static void test_frame_packet_payload_exceeding_max_packet_size_returns_zero(void)
{
    static uint8_t k_big_payload[WF_MAX_PACKET_SIZE] = { 0U };
    uint8_t out_buf[WF_MAX_PACKET_SIZE] = { 0U };
    uint16_t too_large_payload_len = (uint16_t)((WF_MAX_PACKET_SIZE - WF_OVERHEAD) + 1U);
    uint16_t packet_len = frame_packet(k_big_payload, too_large_payload_len,
                                       out_buf, (uint16_t)sizeof(out_buf));

    TEST_ASSERT_EQUAL_UINT16(0U, packet_len);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_frame_packet_happy_path_sets_header_payload_and_crc);
    RUN_TEST(test_frame_packet_sequence_increments_between_calls);
    RUN_TEST(test_frame_packet_null_output_buffer_returns_zero);
    RUN_TEST(test_frame_packet_null_payload_with_nonzero_length_returns_zero);
    RUN_TEST(test_frame_packet_output_buffer_too_small_returns_zero);
    RUN_TEST(test_frame_packet_payload_exceeding_max_packet_size_returns_zero);
    return UNITY_END();
}
