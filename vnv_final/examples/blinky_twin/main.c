/**
 * @file main.c
 * @brief Blinky Twin — A minimal example of RTOSTwin integration.
 * 
 * This example demonstrates how to integrate the RTOSTwin Telemetry Agent
 * into a simple single-task FreeRTOS application.
 */

#include "FreeRTOS.h"
#include "task.h"
#include "../../agent/include/rtostwin.h"

/* 
 * Task 1: Blinky
 * Toggles a virtual LED and does some "work" to use the CPU.
 */
void vBlinkyTask(void *pvParameters) {
    (void)pvParameters;
    for (;;) {
        /* Simulate some CPU work */
        for (volatile int i = 0; i < 10000; i++);
        
        /* Toggle LED (Pseudo-code) */
        // HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
        
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

int main(void) {
    /* 1. Hardware Initialization (HAL_Init, SystemClock_Config, etc.) */
    // HAL_Init();
    
    /* 2. Create Application Tasks */
    xTaskCreate(vBlinkyTask, "Blinky", 128, NULL, tskIDLE_PRIORITY + 1, NULL);
    
    /* 3. Start the RTOSTwin Telemetry Agent */
    /* This starts a separate low-priority task to handle telemetry */
    (void)rtostwin_init();
    (void)rtostwin_start();
    
    /* 4. Start Scheduler */
    vTaskStartScheduler();
    
    /* Should never reach here */
    for (;;);
    return 0;
}
