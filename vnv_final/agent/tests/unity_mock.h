#ifndef UNITY_MOCK_H
#define UNITY_MOCK_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

/**
 * @file unity_mock.h
 * @brief Lightweight macros to simulate a subset of the Unity test framework.
 * 
 * We use this because the actual Unity library is not present in the workspace.
 * It follows the naming conventions of the real Unity framework. (Rule 13)
 */

/* Red color for failure, green for success (ANSI escape codes) */
#define TEST_PASS_COLOR "\033[0;32m"
#define TEST_FAIL_COLOR "\033[0;31m"
#define TEST_RESET_COLOR "\033[0m"

#define TEST_ASSERT_EQUAL_HEX16(expected, actual) \
    if ((uint16_t)(expected) != (uint16_t)(actual)) { \
        printf(TEST_FAIL_COLOR "  FAIL at %s:%d: Expected 0x%04X, Got 0x%04X\n" TEST_RESET_COLOR, \
               __FILE__, __LINE__, (uint16_t)(expected), (uint16_t)(actual)); \
        return; \
    }

#define TEST_ASSERT_EQUAL_UINT8(expected, actual) \
    if ((uint8_t)(expected) != (uint8_t)(actual)) { \
        printf(TEST_FAIL_COLOR "  FAIL at %s:%d: Expected 0x%02X, Got 0x%02X\n" TEST_RESET_COLOR, \
               __FILE__, __LINE__, (uint8_t)(expected), (uint8_t)(actual)); \
        return; \
    }

#define TEST_ASSERT_EQUAL_INT(expected, actual) \
    if ((int)(expected) != (int)(actual)) { \
        printf(TEST_FAIL_COLOR "  FAIL at %s:%d: Expected %d, Got %d\n" TEST_RESET_COLOR, \
               __FILE__, __LINE__, (int)(expected), (int)(actual)); \
        return; \
    }

#define RUN_TEST(test_func) \
    printf("Running %s... ", #test_func); \
    test_func(); \
    printf(TEST_PASS_COLOR "PASS\n" TEST_RESET_COLOR);

#endif /* UNITY_MOCK_H */
