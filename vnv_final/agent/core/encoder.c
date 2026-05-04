/**
 * @file encoder.c
 * @brief Implementation of delta encoding logic.
 *
 * This module compares the current RTOS state against the last sent state
 * and produces either a compact delta stream or a field-serialised keyframe.
 */

#include "encoder.h"
#include "../../../agent/core/wire_format.h"
#include <string.h>

static full_snapshot_t s_last_snapshot;
static bool s_is_initialized = false;
static bool s_last_encode_was_keyframe = false;

static uint8_t clamp_task_count(uint8_t task_count)
{
    return (task_count <= MAX_TASKS) ? task_count : (uint8_t)MAX_TASKS;
}

static void write_u16_le(uint8_t *out_buf, uint16_t value)
{
    out_buf[0] = (uint8_t)(value & 0xFFu);
    out_buf[1] = (uint8_t)(value >> 8u);
}

static void write_u32_le(uint8_t *out_buf, uint32_t value)
{
    out_buf[0] = (uint8_t)(value & 0xFFu);
    out_buf[1] = (uint8_t)((value >> 8u) & 0xFFu);
    out_buf[2] = (uint8_t)((value >> 16u) & 0xFFu);
    out_buf[3] = (uint8_t)((value >> 24u) & 0xFFu);
}

static uint16_t keyframe_payload_size(const full_snapshot_t *current)
{
    return (uint16_t)(7u + ((uint16_t)clamp_task_count(current->task_count) * 24u) + 9u);
}

static bool task_name_differs(const task_snapshot_t *current, const task_snapshot_t *previous)
{
    for (uint8_t i = 0u; i < TASK_NAME_MAX_LEN; i++) {
        if (current->name[i] != previous->name[i]) {
            return true;
        }
        if ((current->name[i] == '\0') && (previous->name[i] == '\0')) {
            break;
        }
    }
    return false;
}

static bool topology_requires_keyframe(const full_snapshot_t *current)
{
    uint8_t task_count = clamp_task_count(current->task_count);

    if (task_count != clamp_task_count(s_last_snapshot.task_count)) {
        return true;
    }

    for (uint8_t i = 0u; i < task_count; i++) {
        if (task_name_differs(&current->tasks[i], &s_last_snapshot.tasks[i])) {
            return true;
        }
    }

    return false;
}

static uint16_t encode_keyframe(const full_snapshot_t *current,
                                uint8_t *out_buf,
                                uint16_t out_buf_size)
{
    uint8_t task_count = clamp_task_count(current->task_count);
    uint16_t required = keyframe_payload_size(current);
    uint16_t ptr = 0u;

    if (out_buf_size < required) {
        return 0u;
    }

    write_u16_le(&out_buf[ptr], current->sequence_num);
    ptr += 2u;

    write_u32_le(&out_buf[ptr], current->timestamp_ticks);
    ptr += 4u;

    out_buf[ptr++] = task_count;

    for (uint8_t i = 0u; i < task_count; i++) {
        uint8_t copied = 0u;
        const task_snapshot_t *task = &current->tasks[i];

        memset(&out_buf[ptr], 0, TASK_NAME_MAX_LEN);
        while ((copied < (TASK_NAME_MAX_LEN - 1u)) && (task->name[copied] != '\0')) {
            out_buf[ptr + copied] = (uint8_t)task->name[copied];
            copied++;
        }
        ptr += TASK_NAME_MAX_LEN;

        out_buf[ptr++] = task->state;
        out_buf[ptr++] = task->priority;
        write_u16_le(&out_buf[ptr], task->stack_hwm_words);
        ptr += 2u;
        write_u32_le(&out_buf[ptr], task->runtime_ticks);
        ptr += 4u;
    }

    write_u32_le(&out_buf[ptr], current->memory.heap_free_bytes);
    ptr += 4u;
    write_u32_le(&out_buf[ptr], current->memory.heap_min_ever_bytes);
    ptr += 4u;
    out_buf[ptr++] = current->memory.cpu_utilization_pct;

    return ptr;
}

void encoder_init(void)
{
    memset(&s_last_snapshot, 0, sizeof(full_snapshot_t));
    s_is_initialized = false;
    s_last_encode_was_keyframe = false;
}

bool encoder_last_was_keyframe(void)
{
    return s_last_encode_was_keyframe;
}

uint16_t encoder_encode(const full_snapshot_t *current,
                        uint8_t *out_buf,
                        uint16_t out_buf_size,
                        bool force_keyframe)
{
    uint16_t ptr = 0u;
    uint8_t task_count;

    if ((current == NULL) || (out_buf == NULL)) {
        return 0u;
    }

    task_count = clamp_task_count(current->task_count);

    if (force_keyframe || !s_is_initialized || topology_requires_keyframe(current)) {
        uint16_t size = encode_keyframe(current, out_buf, out_buf_size);
        if (size == 0u) {
            return 0u;
        }

        memcpy(&s_last_snapshot, current, sizeof(full_snapshot_t));
        s_last_snapshot.task_count = task_count;
        s_is_initialized = true;
        s_last_encode_was_keyframe = true;
        return size;
    }

    s_last_encode_was_keyframe = false;

    if (current->memory.heap_free_bytes != s_last_snapshot.memory.heap_free_bytes) {
        if ((ptr + 5u) > out_buf_size) {
            return 0u;
        }
        out_buf[ptr++] = (uint8_t)(WF_DELTA_SYSTEM_TAG_BASE | WF_DELTA_FIELD_HEAP_FREE);
        write_u32_le(&out_buf[ptr], current->memory.heap_free_bytes);
        ptr += 4u;
    }

    if (current->memory.heap_min_ever_bytes != s_last_snapshot.memory.heap_min_ever_bytes) {
        if ((ptr + 5u) > out_buf_size) {
            return 0u;
        }
        out_buf[ptr++] = (uint8_t)(WF_DELTA_SYSTEM_TAG_BASE | WF_DELTA_FIELD_HEAP_MIN_EVER);
        write_u32_le(&out_buf[ptr], current->memory.heap_min_ever_bytes);
        ptr += 4u;
    }

    if (current->memory.cpu_utilization_pct != s_last_snapshot.memory.cpu_utilization_pct) {
        if ((ptr + 2u) > out_buf_size) {
            return 0u;
        }
        out_buf[ptr++] = (uint8_t)(WF_DELTA_SYSTEM_TAG_BASE | WF_DELTA_FIELD_CPU_UTILIZATION);
        out_buf[ptr++] = current->memory.cpu_utilization_pct;
    }

    for (uint8_t i = 0u; i < task_count; i++) {
        const task_snapshot_t *curr_t = &current->tasks[i];
        const task_snapshot_t *last_t = &s_last_snapshot.tasks[i];
        uint8_t task_tag_high = (uint8_t)(i << 4u);

        if (i >= 15u) {
            break;
        }

        if (curr_t->state != last_t->state) {
            if ((ptr + 2u) > out_buf_size) {
                return 0u;
            }
            out_buf[ptr++] = (uint8_t)(task_tag_high | WF_DELTA_FIELD_TASK_STATE);
            out_buf[ptr++] = curr_t->state;
        }

        if (curr_t->priority != last_t->priority) {
            if ((ptr + 2u) > out_buf_size) {
                return 0u;
            }
            out_buf[ptr++] = (uint8_t)(task_tag_high | WF_DELTA_FIELD_TASK_PRIORITY);
            out_buf[ptr++] = curr_t->priority;
        }

        if (curr_t->stack_hwm_words != last_t->stack_hwm_words) {
            if ((ptr + 3u) > out_buf_size) {
                return 0u;
            }
            out_buf[ptr++] = (uint8_t)(task_tag_high | WF_DELTA_FIELD_TASK_STACK_HWM);
            write_u16_le(&out_buf[ptr], curr_t->stack_hwm_words);
            ptr += 2u;
        }

        if (curr_t->runtime_ticks != last_t->runtime_ticks) {
            if ((ptr + 5u) > out_buf_size) {
                return 0u;
            }
            out_buf[ptr++] = (uint8_t)(task_tag_high | WF_DELTA_FIELD_TASK_RUNTIME);
            write_u32_le(&out_buf[ptr], curr_t->runtime_ticks);
            ptr += 4u;
        }
    }

    memcpy(&s_last_snapshot, current, sizeof(full_snapshot_t));
    s_last_snapshot.task_count = task_count;
    return ptr;
}
