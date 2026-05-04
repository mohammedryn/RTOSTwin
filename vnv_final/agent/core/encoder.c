/**
 * @file encoder.c
 * @brief Implementation of delta encoding logic.
 *
 * This module compares the current RTOS state against the last sent state
 * and produces a compact delta-encoded stream.
 *
 * TAG FORMAT (TECH_SPEC §2.4):
 *   Upper Nibble: Task Index (0x0 to 0xE). 0xF = System-level field.
 *   Lower Nibble: Field ID.
 *
 * Ownership: VNV (ARCHITECTURE.md — Component 1)
 */

#include "encoder.h"
#include <string.h>

/* Persistent state — strictly no malloc per CODING_RULES.md */
static full_snapshot_t s_last_snapshot;
static bool s_is_initialized = false;

/* ------------------------------------------------------------------
 * Delta Field IDs (Low Nibble)
 * ------------------------------------------------------------------ */
#define FIELD_TASK_STATE    0x01
#define FIELD_TASK_PRIO     0x02
#define FIELD_TASK_STACK    0x03
#define FIELD_TASK_RUNTIME  0x04

#define FIELD_HEAP_FREE     0x05
#define FIELD_HEAP_MIN      0x06
#define FIELD_CPU_UTIL      0x07

/* ------------------------------------------------------------------
 * encoder_init()
 * ------------------------------------------------------------------ */
void encoder_init(void) {
    memset(&s_last_snapshot, 0, sizeof(full_snapshot_t));
    s_is_initialized = false; /* Force keyframe on first call */
}

/* ------------------------------------------------------------------
 * encoder_encode()
 * ------------------------------------------------------------------ */
uint16_t encoder_encode(const full_snapshot_t *current,
                        uint8_t *out_buf, 
                        uint16_t out_buf_size,
                        bool force_keyframe) 
{
    if (!current || !out_buf) return 0u;

    uint16_t ptr = 0u;

    /* 
     * 1. KEYFRAME (FORCE_RESET)
     * If forced or never initialized, copy the whole struct raw.
     */
    if (force_keyframe || !s_is_initialized) {
        if (out_buf_size < sizeof(full_snapshot_t)) return 0u;
        
        memcpy(out_buf, current, sizeof(full_snapshot_t));
        memcpy(&s_last_snapshot, current, sizeof(full_snapshot_t));
        s_is_initialized = true;
        
        return (uint16_t)sizeof(full_snapshot_t);
    }

    /* 
     * 2. DELTA ENCODING
     * Compare current vs last. Only write if changed.
     */

    /* --- System Fields (Tag High Nibble = 0xF) --- */
    
    /* Heap Free (4 bytes) */
    if (current->memory.heap_free_bytes != s_last_snapshot.memory.heap_free_bytes) {
        if ((ptr + 5u) > out_buf_size) return 0u;
        out_buf[ptr++] = 0xF0 | FIELD_HEAP_FREE;
        memcpy(&out_buf[ptr], &current->memory.heap_free_bytes, 4);
        ptr += 4u;
    }

    /* Heap Min Ever (4 bytes) */
    if (current->memory.heap_min_ever_bytes != s_last_snapshot.memory.heap_min_ever_bytes) {
        if ((ptr + 5u) > out_buf_size) return 0u;
        out_buf[ptr++] = 0xF0 | FIELD_HEAP_MIN;
        memcpy(&out_buf[ptr], &current->memory.heap_min_ever_bytes, 4);
        ptr += 4u;
    }

    /* CPU Utilization (1 byte) */
    if (current->memory.cpu_utilization_pct != s_last_snapshot.memory.cpu_utilization_pct) {
        if ((ptr + 2u) > out_buf_size) return 0u;
        out_buf[ptr++] = 0xF0 | FIELD_CPU_UTIL;
        out_buf[ptr++] = current->memory.cpu_utilization_pct;
    }

    /* --- Per-Task Fields (Tag High Nibble = i) --- */
    for (uint8_t i = 0u; i < current->task_count; i++) {
        if (i >= 15u) break; /* Guard: 0xF is reserved for system */
        
        const task_snapshot_t *curr_t = &current->tasks[i];
        const task_snapshot_t *last_t = &s_last_snapshot.tasks[i];
        uint8_t task_tag_high = (uint8_t)(i << 4u);

        /* Task State (1 byte) */
        if (curr_t->state != last_t->state) {
            if ((ptr + 2u) > out_buf_size) return 0u;
            out_buf[ptr++] = task_tag_high | FIELD_TASK_STATE;
            out_buf[ptr++] = curr_t->state;
        }

        /* Priority (1 byte) */
        if (curr_t->priority != last_t->priority) {
            if ((ptr + 2u) > out_buf_size) return 0u;
            out_buf[ptr++] = task_tag_high | FIELD_TASK_PRIO;
            out_buf[ptr++] = curr_t->priority;
        }

        /* Stack High Watermark (2 bytes) */
        if (curr_t->stack_hwm_words != last_t->stack_hwm_words) {
            if ((ptr + 3u) > out_buf_size) return 0u;
            out_buf[ptr++] = task_tag_high | FIELD_TASK_STACK;
            memcpy(&out_buf[ptr], &curr_t->stack_hwm_words, 2);
            ptr += 2u;
        }

        /* Runtime Counter (4 bytes) */
        if (curr_t->runtime_ticks != last_t->runtime_ticks) {
            if ((ptr + 5u) > out_buf_size) return 0u;
            out_buf[ptr++] = task_tag_high | FIELD_TASK_RUNTIME;
            memcpy(&out_buf[ptr], &curr_t->runtime_ticks, 4);
            ptr += 4u;
        }
    }

    /* Update last known state for the next call */
    memcpy(&s_last_snapshot, current, sizeof(full_snapshot_t));
    
    return ptr;
}
