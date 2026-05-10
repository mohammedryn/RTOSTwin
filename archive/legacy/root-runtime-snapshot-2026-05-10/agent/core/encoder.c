/**
 * @file encoder.c
 * @brief Implementation of delta encoding logic.
 */

#include "encoder.h"
#include <string.h>

/* Persistent state - strictly no malloc */
static full_snapshot_t s_last_snapshot;
static bool s_is_initialized = false;

/* Delta Tags */
#define TAG_TASK_INDEX      0x01
#define TAG_TASK_STATE      0x02
#define TAG_TASK_PRIO       0x03
#define TAG_TASK_STACK      0x04
#define TAG_TASK_RUNTIME    0x05
#define TAG_MEM_HEAP_FREE   0x10
#define TAG_MEM_HEAP_MIN    0x11
#define TAG_MEM_CPU_UTIL    0x12
#define TAG_END_OF_PACKET   0xFF

void encoder_init(void) {
    memset(&s_last_snapshot, 0, sizeof(full_snapshot_t));
    s_is_initialized = true;
}

uint16_t encoder_encode(const full_snapshot_t *current,
                        uint8_t *out_buf, uint16_t out_buf_size,
                        bool force_keyframe) 
{
    if (!current || !out_buf) return 0;

    uint16_t ptr = 0;

    /* 
     * Keyframe: Send the full struct raw.
     * This is the simplest way to ensure the PC has a "ground truth"
     * to apply future deltas to.
     */
    if (force_keyframe || !s_is_initialized) {
        if (out_buf_size < sizeof(full_snapshot_t)) return 0;
        memcpy(out_buf, current, sizeof(full_snapshot_t));
        memcpy(&s_last_snapshot, current, sizeof(full_snapshot_t));
        return sizeof(full_snapshot_t);
    }

    /* 
     * Delta Logic: Compare current vs last.
     * Only write to out_buf if the value has changed.
     */

    /* 1. Heap Free Bytes (4 bytes) */
    if (current->memory.heap_free_bytes != s_last_snapshot.memory.heap_free_bytes) {
        if (ptr + 5 > out_buf_size) return 0;
        out_buf[ptr++] = TAG_MEM_HEAP_FREE;
        memcpy(&out_buf[ptr], &current->memory.heap_free_bytes, 4);
        ptr += 4;
    }
    
    /* 2. CPU Utilization (1 byte) */
    if (current->memory.cpu_utilization_pct != s_last_snapshot.memory.cpu_utilization_pct) {
        if (ptr + 2 > out_buf_size) return 0;
        out_buf[ptr++] = TAG_MEM_CPU_UTIL;
        out_buf[ptr++] = current->memory.cpu_utilization_pct;
    }

    /* 3. Task-specific deltas */
    for (uint8_t i = 0; i < current->task_count; i++) {
        const task_snapshot_t *curr_t = &current->tasks[i];
        const task_snapshot_t *last_t = &s_last_snapshot.tasks[i];
        
        bool task_changed = (curr_t->state != last_t->state) || 
                            (curr_t->priority != last_t->priority) ||
                            (curr_t->stack_hwm_words != last_t->stack_hwm_words);

        if (task_changed) {
            if (ptr + 2 > out_buf_size) return 0;
            out_buf[ptr++] = TAG_TASK_INDEX;
            out_buf[ptr++] = i;

            if (curr_t->state != last_t->state) {
                if (ptr + 2 > out_buf_size) return 0;
                out_buf[ptr++] = TAG_TASK_STATE;
                out_buf[ptr++] = curr_t->state;
            }
            if (curr_t->priority != last_t->priority) {
                if (ptr + 2 > out_buf_size) return 0;
                out_buf[ptr++] = TAG_TASK_PRIO;
                out_buf[ptr++] = curr_t->priority;
            }
            if (curr_t->stack_hwm_words != last_t->stack_hwm_words) {
                if (ptr + 3 > out_buf_size) return 0;
                out_buf[ptr++] = TAG_TASK_STACK;
                memcpy(&out_buf[ptr], &curr_t->stack_hwm_words, 2);
                ptr += 2;
            }
        }
    }

    /* End of Packet Marker */
    if (ptr + 1 > out_buf_size) return 0;
    out_buf[ptr++] = TAG_END_OF_PACKET;

    /* Update last known state for the next call */
    memcpy(&s_last_snapshot, current, sizeof(full_snapshot_t));
    
    return ptr;
}
