/**
 * @file snapshot.c
 * @brief Snapshot Engine — captures live FreeRTOS state into snapshot structs.
 *
 * DESIGN NOTES
 * - No malloc. All buffers are static.
 * - snapshot_capture() enters a FreeRTOS critical section for the minimum
 *   time needed. Stack watermarks are read outside the critical section to
 *   reduce scheduler suspension time.
 * - Requires FreeRTOSConfig.h:
 *     configUSE_TRACE_FACILITY        = 1
 *     configGENERATE_RUN_TIME_STATS   = 1
 */

#include "snapshot.h"
#include "FreeRTOS.h"
#include "task.h"

/* ── Static buffer — no malloc ever ─────────────────────────────── */
static TaskStatus_t s_task_status_buf[MAX_TASKS];

/* ── Counters ────────────────────────────────────────────────────── */
extern volatile uint32_t s_idle_ticks;
extern volatile uint32_t s_period_ticks;
static uint16_t s_snap_seq = 0U;

/* ── Public API ──────────────────────────────────────────────────── */

void snapshot_init(void) {
    s_idle_ticks   = 0U;
    s_period_ticks = 0U;
}

void snapshot_capture(full_snapshot_t *out) {
    if (out == NULL) {
        return;
    }

    /* 1. Sequence and Timestamp */
    out->sequence_num    = s_snap_seq++;
    out->timestamp_ticks = xTaskGetTickCount();

    /* 2. Task list — suspends scheduler briefly (~15-25 µs typical) */
    UBaseType_t n = uxTaskGetSystemState(s_task_status_buf,
                                         MAX_TASKS,
                                         NULL /* total runtime unused here */);

    /* Clamp to our fixed array size */
    out->task_count = (n <= MAX_TASKS) ? (uint8_t)n : (uint8_t)MAX_TASKS;

    /* 3. Copy fields for each task */
    for (uint8_t i = 0U; i < out->task_count; i++) {
        TaskStatus_t *src  = &s_task_status_buf[i];
        task_snapshot_t *dst = &out->tasks[i];

        /* Copy name safely (null-terminate in case FreeRTOS name is short) */
        uint8_t j;
        for (j = 0U; j < (TASK_NAME_MAX_LEN - 1U); j++) {
            dst->name[j] = src->pcTaskName[j];
            if (src->pcTaskName[j] == '\0') { break; }
        }
        dst->name[TASK_NAME_MAX_LEN - 1U] = '\0'; /* Guarantee null terminator */

        dst->state         = (uint8_t)src->eCurrentState;
        dst->priority      = (uint8_t)src->uxCurrentPriority;
        dst->stack_hwm_words = (uint16_t)src->usStackHighWaterMark;
        dst->runtime_ticks = src->ulRunTimeCounter;
    }

    /* 4. Memory snapshot */
    out->memory.heap_free_bytes     = (uint32_t)xPortGetFreeHeapSize();
    out->memory.heap_min_ever_bytes = (uint32_t)xPortGetMinimumEverFreeHeapSize();

    /* 5. CPU utilisation — calculated from idle hook counters */
    uint32_t idle   = s_idle_ticks;
    uint32_t period = s_period_ticks;

    if (period > 0U) {
        uint32_t busy_pct = 100U - (uint32_t)((idle * 100U) / period);
        out->memory.cpu_utilization_pct = (busy_pct <= 100U) ? (uint8_t)busy_pct : 100U;
    } else {
        out->memory.cpu_utilization_pct = 0U;
    }

    /* Reset counters for next measurement window */
    taskENTER_CRITICAL();
    s_idle_ticks   = 0U;
    s_period_ticks = period; /* keep denominator consistent */
    taskEXIT_CRITICAL();
}

/* ── Idle Hook (called by FreeRTOS from the Idle task) ───────────── */
/**
 * This function is defined in agent/freertos/hooks.c to avoid duplicate 
 * definitions. It increments the idle counters used for CPU utilization.
 */
extern void snapshot_update_idle_counters(void);
