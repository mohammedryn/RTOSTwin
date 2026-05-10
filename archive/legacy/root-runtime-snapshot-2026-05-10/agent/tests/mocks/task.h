/**
 * @file task.h
 * @brief Minimal FreeRTOS task.h stub for PC unit testing.
 */

#ifndef TASK_H
#define TASK_H

#include <stdint.h>
#include "FreeRTOS.h"

typedef enum {
    eRunning = 0,
    eReady,
    eBlocked,
    eSuspended,
    eDeleted
} eTaskState;

typedef struct {
    const char  *pcTaskName;
    eTaskState   eCurrentState;
    UBaseType_t  uxCurrentPriority;
    uint16_t     usStackHighWaterMark;
    uint32_t     ulRunTimeCounter;
} TaskStatus_t;

/* Declarations — defined as mocks in test_snapshot.c */
TickType_t   xTaskGetTickCount(void);
UBaseType_t  uxTaskGetSystemState(TaskStatus_t *buf, UBaseType_t max, uint32_t *total);
size_t       xPortGetFreeHeapSize(void);
size_t       xPortGetMinimumEverFreeHeapSize(void);

#endif /* TASK_H */
