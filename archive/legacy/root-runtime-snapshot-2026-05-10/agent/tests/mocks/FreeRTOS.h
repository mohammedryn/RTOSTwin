/**
 * @file FreeRTOS.h
 * @brief Minimal FreeRTOS stub to allow snapshot.c to compile on a PC.
 *
 * When running unit tests on a host machine (not the real STM32),
 * we provide just enough type and macro definitions to satisfy the
 * compiler. The real implementations are provided by mock functions
 * in the test file itself.
 */

#ifndef FREERTOS_H
#define FREERTOS_H

#include <stdint.h>
#include <stddef.h>

typedef uint32_t TickType_t;
typedef uint32_t UBaseType_t;
typedef int      BaseType_t;

#define pdTRUE  ((BaseType_t)1)
#define pdFALSE ((BaseType_t)0)

/* Critical section macros — do nothing in the test environment */
#define taskENTER_CRITICAL()
#define taskEXIT_CRITICAL()

#endif /* FREERTOS_H */
