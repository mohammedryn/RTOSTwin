/*
 * ============================================================================
 *  RTOSTwin — FreeRTOS Hooks
 *  File: agent/freertos/hooks.c
 *  Platform: Any FreeRTOS-supported MCU
 * 
 *  Provides system-level trace hooks required by the RTOSTwin agent, 
 *  specifically for CPU utilization measurement and crash diagnostics.
 * 
 *  NOTE: To use the idle hook, you MUST set configUSE_IDLE_HOOK=1 
 *  in your projects's FreeRTOSConfig.h.
 * ============================================================================
 */

#include "FreeRTOS.h"
#include "task.h"
#include <stdint.h>

/* 
 * We declare this variable as volatile static because:
 * 1. static: It should only be accessed directly inside this file. The 
 *    snapshot_capture() function will access it via an extern declaration
 *    (or better, a getter function if we create one later).
 * 2. volatile: It is updated within an ISR-like context (the Idle task) and 
 *    read asynchronously by the Telemetry task. The compiler must not optimize 
 *    reads or writes to it.
 */
volatile uint32_t s_idle_cycle_count = 0;

/**
 * @brief FreeRTOS Application Idle Hook.
 * 
 * Called automatically by the FreeRTOS Scheduler whenever the Idle task runs 
 * (i.e., when no other task is in the Ready state). We use this to count 
 * how many cycles the CPU spends doing "nothing".
 * 
 * IMPORTANT: This function must never block.
 */
void vApplicationIdleHook(void) {
    /* 
     * We increment the counter every time the idle task spins.
     * The snapshot engine will read this counter periodically, reset it, 
     * and compare it against the total possible idle loops per period 
     * to calculate CPU utilization.
     */
    s_idle_cycle_count++;
}

/* 
 * A static buffer to hold crash information. We use a static array instead 
 * of malloc to guarantee that memory is available during a fatal error, 
 * and because malloc takes a mutex which is forbidden in ISRs/faults.
 */
#define CRASH_REASON_MAX_LEN 64
static char s_crash_reason[CRASH_REASON_MAX_LEN] = {0};

/**
 * @brief FreeRTOS Stack Overflow Hook.
 * 
 * Called automatically by FreeRTOS if it detects a task has blown past 
 * its allocated stack space. Requires configCHECK_FOR_STACK_OVERFLOW 
 * to be set to 1 or 2 in FreeRTOSConfig.h.
 * 
 * @param xTask The handle of the task that overflowed.
 * @param pcTaskName The name of the task that overflowed.
 */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    (void)xTask;  /* Suppress unused parameter warning */

    /* 
     * We manually copy the task name into our static buffer. We do not use 
     * snprintf or complex string functions here because we are in a fault 
     * state and must avoid stack usage. We do a simple byte-by-byte copy.
     */
    const char *prefix = "STACK OVERFLOW: ";
    size_t i = 0;
    
    while (prefix[i] != '\0' && i < (CRASH_REASON_MAX_LEN - 1)) {
        s_crash_reason[i] = prefix[i];
        i++;
    }

    /* Copy the task name into the buffer */
    size_t j = 0;
    while (pcTaskName != NULL && pcTaskName[j] != '\0' && i < (CRASH_REASON_MAX_LEN - 1)) {
        s_crash_reason[i] = pcTaskName[j];
        i++;
        j++;
    }

    /* Ensure null termination */
    s_crash_reason[i] = '\0';

    /* 
     * Halt execution. In a real system, you might trigger a hardware reset
     * or attempt to write s_crash_reason to persistent flash memory right before 
     * dying. For now, we spin in an infinite loop so a debugger can attach 
     * and inspect the s_crash_reason variable.
     */
    while (1) {
        /* Wait for debugger */
    }
}
