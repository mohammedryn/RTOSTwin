#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "../include/rtostwin.h"
#include "../core/snapshot.h"
#include "../core/profiler.h"
#include "../core/measurement.h"

static unsigned profiler_init_calls;
static unsigned snapshot_init_calls;
static unsigned encoder_init_calls;
static unsigned transport_init_calls;
static unsigned x_task_create_calls;
static unsigned v_task_delete_calls;
static TaskHandle_t created_task_handle = (TaskHandle_t)0x1234;

void profiler_init(void) { profiler_init_calls++; }
uint32_t profiler_start(void) { return 10U; }
uint32_t profiler_stop(uint32_t start_cycles) { return start_cycles + 1U; }
void profiler_record(profiler_stats_t *stats, uint32_t elapsed_cycles)
{
    (void)elapsed_cycles;
    stats->call_count++;
}
void profiler_report(const profiler_stats_t *stats, const char *label)
{
    (void)stats;
    (void)label;
}

void snapshot_init(void) { snapshot_init_calls++; }
void snapshot_capture(full_snapshot_t *out)
{
    if (out != NULL) {
        out->sequence_num++;
        out->timestamp_ticks++;
    }
}

void encoder_init(void) { encoder_init_calls++; }
uint16_t encoder_encode(const full_snapshot_t *current,
                        uint8_t *out_buf,
                        uint16_t out_buf_size,
                        bool force_keyframe)
{
    (void)current;
    (void)force_keyframe;
    if (out_buf_size < 1U) {
        return 0U;
    }
    out_buf[0] = 0xA5U;
    return 1U;
}
bool encoder_last_was_keyframe(void) { return true; }

uint16_t frame_packet(const uint8_t *payload,
                      uint16_t payload_len,
                      uint8_t packet_type,
                      uint16_t sequence_num,
                      uint32_t timestamp_ticks,
                      uint8_t *out_buf,
                      uint16_t out_buf_size)
{
    (void)payload;
    (void)payload_len;
    (void)packet_type;
    (void)sequence_num;
    (void)timestamp_ticks;
    if (out_buf_size < 1U) {
        return 0U;
    }
    out_buf[0] = 0x5AU;
    return 1U;
}

void transport_init(void) { transport_init_calls++; }
int transport_send(const uint8_t *packet, uint16_t len)
{
    (void)packet;
    return len > 0U ? 0 : -1;
}
uint32_t transport_get_drop_count(void) { return 0U; }

TickType_t xTaskGetTickCount(void) { return 0U; }
UBaseType_t uxTaskGetSystemState(TaskStatus_t *buf, UBaseType_t max, uint32_t *total)
{
    (void)buf;
    (void)max;
    if (total != NULL) {
        *total = 0U;
    }
    return 0U;
}
size_t xPortGetFreeHeapSize(void) { return 0U; }
size_t xPortGetMinimumEverFreeHeapSize(void) { return 0U; }

BaseType_t xTaskCreate(TaskFunction_t pxTaskCode,
                       const char * const pcName,
                       uint16_t usStackDepth,
                       void *pvParameters,
                       UBaseType_t uxPriority,
                       TaskHandle_t *pxCreatedTask)
{
    (void)pxTaskCode;
    (void)pvParameters;

    assert(strcmp(pcName, "RTOSTwinTest") == 0);
    assert(usStackDepth == 256U);
    assert(uxPriority == (tskIDLE_PRIORITY + 3U));

    x_task_create_calls++;
    *pxCreatedTask = created_task_handle;
    return pdTRUE;
}

void vTaskDelete(TaskHandle_t xTaskToDelete)
{
    assert(xTaskToDelete == created_task_handle);
    v_task_delete_calls++;
}

void vTaskDelay(TickType_t xTicksToDelay)
{
    (void)xTicksToDelay;
}

int main(void)
{
    assert(rtostwin_is_running() == false);
    assert(strcmp(rtostwin_version(), "test-lifecycle") == 0);

    assert(rtostwin_init() == RTOSTWIN_STATUS_OK);
    assert(profiler_init_calls == 1U);
    assert(snapshot_init_calls == 1U);
    assert(encoder_init_calls == 1U);
    assert(transport_init_calls == 1U);

    assert(rtostwin_init() == RTOSTWIN_STATUS_OK);
    assert(profiler_init_calls == 1U);

    assert(rtostwin_start() == RTOSTWIN_STATUS_OK);
    assert(rtostwin_is_running() == true);
    assert(x_task_create_calls == 1U);

    assert(rtostwin_start() == RTOSTWIN_STATUS_ALREADY_STARTED);
    assert(x_task_create_calls == 1U);

    assert(rtostwin_stop() == RTOSTWIN_STATUS_OK);
    assert(rtostwin_is_running() == false);
    assert(v_task_delete_calls == 1U);

    StartTelemetryAgent();
    assert(rtostwin_is_running() == true);
    assert(x_task_create_calls == 2U);

    printf("rtostwin lifecycle API test PASSED\n");
    return 0;
}
