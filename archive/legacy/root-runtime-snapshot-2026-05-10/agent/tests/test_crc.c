#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include "framer.h"

/**
 * @file test_crc.c
 * @brief Unit test for the CRC-16-CCITT implementation.
 * 
 * Target: Verify that crc16_ccitt("123456789") == 0x29B1.
 */

int main(void) {
    const char *test_str = "123456789";
    uint16_t result;

    printf("--- Running CRC-16 Unit Test ---\n");

    /* Call our function from framer.c */
    result = crc16_ccitt((const uint8_t*)test_str, (uint16_t)strlen(test_str));

    printf("Input: '%s'\n", test_str);
    printf("Expected: 0x29B1\n");
    printf("Actual:   0x%04X\n", result);

    /* The 'assert' macro crashes the program if the condition is false.
     * This is our "Pass/Fail" gate. */
    assert(result == 0x29B1);

    printf("RESULT: PASSED ✅\n");

    return 0;
}
