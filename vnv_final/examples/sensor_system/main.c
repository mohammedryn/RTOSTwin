/**
 * @file main.c
 * @brief Sensor System — A multi-task example of RTOSTwin integration.
 * 
 * This example simulates a realistic embedded system with three tasks:
 * 1. SensorTask: Reads data (high frequency).
 * 2. ProcessingTask: Processes data (CPU intensive).
 * 3. CommsTask: Sends results (IO intensive).
 */

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "../../agent/include/rtostwin.h"

static QueueHandle_t xSensorQueue;

void vSensorTask(void *pvParameters) {
    uint32_t val = 0;
    for (;;) {
        val++;
        xQueueSend(xSensorQueue, &val, portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(50)); /* 20 Hz */
    }
}

void vProcessingTask(void *pvParameters) {
    uint32_t val;
    for (;;) {
        if (xQueueReceive(xSensorQueue, &val, portMAX_DELAY)) {
            /* Simulate CPU intensive work */
            for (volatile int i = 0; i < 50000; i++);
        }
    }
}

void vCommsTask(void *pvParameters) {
    for (;;) {
        /* Simulate IO wait */
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

int main(void) {
    xSensorQueue = xQueueCreate(10, sizeof(uint32_t));
    
    xTaskCreate(vSensorTask,     "Sensor",     256, NULL, tskIDLE_PRIORITY + 2, NULL);
    xTaskCreate(vProcessingTask, "Processor",  256, NULL, tskIDLE_PRIORITY + 2, NULL);
    xTaskCreate(vCommsTask,      "Comms",      256, NULL, tskIDLE_PRIORITY + 1, NULL);
    
    (void)rtostwin_init();
    (void)rtostwin_start();
    
    vTaskStartScheduler();
    for (;;);
    return 0;
}
