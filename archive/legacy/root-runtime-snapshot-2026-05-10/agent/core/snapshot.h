#ifndef RTOSTwin_SNAPSHOT_H
#define RTOSTwin_SNAPSHOT_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @file snapshot.h
 * @brief FreeRTOS task and memory telemetry structures.
 * 
 * Defines the data structures used by the Snapshot Engine to capture 
 * internal RTOS state and passed to the Encoder.
 */

#define MAX_TASKS              16
#define TASK_NAME_MAX_LEN      16

/**
 * @brief State summary for a single FreeRTOS task.
 */
typedef struct {
    char     name[TASK_NAME_MAX_LEN]; /**< Task name (from FreeRTOS) */
    uint8_t  state;                   /**< Task state (eTaskState enum) */
    uint8_t  priority;                /**< Current task priority */
    uint16_t stack_hwm_words;         /**< Stack high watermark (words remaining) */
    uint32_t runtime_ticks;           /**< Total CPU runtime ticks for this task */
} task_snapshot_t;                    /**< Size: 24 bytes per task */

/**
 * @brief Current status of the system heap and CPU.
 */
typedef struct {
    uint32_t heap_free_bytes;      /**< Current available heap in bytes */
    uint32_t heap_min_ever_bytes;  /**< Historical minimum free heap seen */
    uint8_t  cpu_utilization_pct;  /**< Total CPU utilization (0-100%) */
} memory_snapshot_t;               /**< Size: 9 bytes */

/**
 * @brief A complete frame containing all system telemetry.
 */
typedef struct {
    uint16_t         sequence_num;    /**< Monotonically incrementing packet ID */
    uint32_t         timestamp_ticks; /**< Value of xTaskGetTickCount() */
    uint8_t          task_count;      /**< Number of active tasks in this snapshot */
    task_snapshot_t   tasks[MAX_TASKS]; /**< Fixed array of task snapshots */
    memory_snapshot_t memory;          /**< Heap and CPU metrics */
} full_snapshot_t;                    /**< Size: ~400 bytes max */

/**
 * @brief Initialize the snapshot engine internals.
 */
void snapshot_init(void);

/**
 * @brief Capture a fresh telemetry snapshot from FreeRTOS.
 * 
 * @param out Pointer to the full_snapshot_t buffer to fill.
 */
void snapshot_capture(full_snapshot_t *out);

#endif /* RTOSTwin_SNAPSHOT_H */
