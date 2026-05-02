#ifndef RTOSTWIN_SNAPSHOT_H
#define RTOSTWIN_SNAPSHOT_H

#include "FreeRTOS.h"
#include "task.h"
#include <stdbool.h>
#include <stdint.h>

#define MAX_TASKS 16
#define TASK_NAME_MAX_LEN 16

/** Snapshot of one RTOS task. */
typedef struct {
    char name[TASK_NAME_MAX_LEN];   /**< Task name (null-terminated or padded). */
    uint8_t state;                  /**< FreeRTOS task state enum value. */
    uint8_t priority;               /**< Current task priority. */
    uint16_t stack_hwm_words;       /**< Stack high watermark in words. */
    uint32_t runtime_ticks;         /**< Runtime counter ticks. */
} task_snapshot_t;

/** Snapshot of system-wide memory and CPU state. */
typedef struct {
    uint32_t heap_free_bytes;       /**< Current free heap bytes. */
    uint32_t heap_min_ever_bytes;   /**< Minimum-ever free heap bytes. */
    uint8_t cpu_utilization_pct;    /**< CPU utilization percentage (0-100). */
} memory_snapshot_t;

/** Full telemetry snapshot for one capture cycle. */
typedef struct {
    uint16_t sequence_num;          /**< Monotonic packet sequence number. */
    uint32_t timestamp_ticks;       /**< FreeRTOS tick timestamp. */
    uint8_t task_count;             /**< Number of valid task entries. */
    task_snapshot_t tasks[MAX_TASKS]; /**< Per-task snapshots. */
    memory_snapshot_t memory;       /**< System memory and CPU snapshot. */
} full_snapshot_t;

/** Initialize snapshot subsystem state. */
void snapshot_init(void);

/**
 * Capture current RTOS state into the output snapshot.
 * @param out Destination snapshot pointer.
 */
void snapshot_capture(full_snapshot_t *out);

#endif /* RTOSTWIN_SNAPSHOT_H */
