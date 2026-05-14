/**
 * @file task.h
 * @brief Minimal FreeRTOS task.h stub for PC unit testing.
 */

#ifndef TASK_H
#define TASK_H

#include <stdint.h>
#include "FreeRTOS.h"

typedef void *TaskHandle_t;
typedef void (*TaskFunction_t)(void *);

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

#define tskIDLE_PRIORITY 0U
#define pdMS_TO_TICKS(ms) ((TickType_t)(ms))

TickType_t   xTaskGetTickCount(void);
UBaseType_t  uxTaskGetSystemState(TaskStatus_t *buf, UBaseType_t max, uint32_t *total);
size_t       xPortGetFreeHeapSize(void);
size_t       xPortGetMinimumEverFreeHeapSize(void);

BaseType_t xTaskCreate(TaskFunction_t pxTaskCode,
                       const char * const pcName,
                       uint16_t usStackDepth,
                       void *pvParameters,
                       UBaseType_t uxPriority,
                       TaskHandle_t *pxCreatedTask);

void vTaskDelete(TaskHandle_t xTaskToDelete);
void vTaskDelay(TickType_t xTicksToDelay);

#endif /* TASK_H */
