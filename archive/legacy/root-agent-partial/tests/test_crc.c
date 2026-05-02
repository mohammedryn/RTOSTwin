#include "unity.h"

#include "../core/framer.h"

#include <stdint.h>

void setUp(void)
{
}

void tearDown(void)
{
}

static void test_crc16_ccitt_standard_vector_returns_0x29B1(void)
{
    static const uint8_t k_test_vector[] = "123456789";
    uint16_t crc = crc16_ccitt(k_test_vector, 9U);

    TEST_ASSERT_EQUAL_HEX16(0x29B1U, crc);
}

static void test_crc16_ccitt_empty_input_returns_initial_value(void)
{
    uint16_t crc = crc16_ccitt((const uint8_t *)0, 0U);

    TEST_ASSERT_EQUAL_HEX16(0xFFFFU, crc);
}

static void test_crc16_ccitt_single_byte_input_matches_reference(void)
{
    static const uint8_t k_single_byte[] = { 0x00U };
    uint16_t crc = crc16_ccitt(k_single_byte, 1U);

    TEST_ASSERT_EQUAL_HEX16(0xE1F0U, crc);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_crc16_ccitt_standard_vector_returns_0x29B1);
    RUN_TEST(test_crc16_ccitt_empty_input_returns_initial_value);
    RUN_TEST(test_crc16_ccitt_single_byte_input_matches_reference);
    return UNITY_END();
}
