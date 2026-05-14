/**
 * @file rtostwin.c
 * @brief Embeddable lifecycle entry point for the RTOSTwin telemetry agent.
 */

#include "rtostwin.h"

#if RTOSTWIN_ENABLE

#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"
#include "../../agent/core/wire_format.h"
#include "core/encoder.h"
#include "core/framer.h"
#include "core/measurement.h"
#include "core/profiler.h"
#include "core/snapshot.h"
#include "core/transport.h"

#ifndef RTOSTWIN_TELEMETRY_TASK_NAME
#define RTOSTWIN_TELEMETRY_TASK_NAME "RTOSTwin"
#endif

#ifndef RTOSTWIN_TELEMETRY_TASK_STACK_WORDS
#define RTOSTWIN_TELEMETRY_TASK_STACK_WORDS 512
#endif

#ifndef RTOSTWIN_TELEMETRY_TASK_PRIORITY_OFFSET
#define RTOSTWIN_TELEMETRY_TASK_PRIORITY_OFFSET 2
#endif

#ifndef RTOSTWIN_TELEMETRY_PERIOD_MS
#define RTOSTWIN_TELEMETRY_PERIOD_MS 100
#endif

#ifndef RTOSTWIN_PAYLOAD_BUFFER_BYTES
#define RTOSTWIN_PAYLOAD_BUFFER_BYTES 512
#endif

#ifndef RTOSTWIN_FRAME_BUFFER_BYTES
#define RTOSTWIN_FRAME_BUFFER_BYTES 540
#endif

#ifndef RTOSTWIN_PROFILE_REPORT_INTERVAL
#define RTOSTWIN_PROFILE_REPORT_INTERVAL 100
#endif

#ifndef RTOSTWIN_ENABLE_PROFILING
#define RTOSTWIN_ENABLE_PROFILING 1
#endif

static full_snapshot_t s_current_snapshot;
static uint8_t s_payload_buffer[RTOSTWIN_PAYLOAD_BUFFER_BYTES];
static uint8_t s_framed_buffer[RTOSTWIN_FRAME_BUFFER_BYTES];
static measurement_stats_t s_cycle_stats;
static TaskHandle_t s_telemetry_task_handle;
static bool s_initialised;

static void rtostwin_telemetry_task(void *pvParameters)
{
    profiler_stats_t snap_stats = {0};
    uint32_t loop_count = 0U;

    (void)pvParameters;

    while (1) {
        uint32_t cycle_start = profiler_start();
        uint32_t t_start = profiler_start();

        snapshot_capture(&s_current_snapshot);
        profiler_record(&snap_stats, profiler_stop(t_start));

        {
            bool force_keyframe = ((loop_count % WF_KEYFRAME_INTERVAL) == 0U);
            uint16_t enc_len = encoder_encode(&s_current_snapshot,
                                              s_payload_buffer,
                                              (uint16_t)sizeof(s_payload_buffer),
                                              force_keyframe);

            if (enc_len > 0U) {
                uint8_t pkt_type = encoder_last_was_keyframe() ? WF_TYPE_KEYFRAME : WF_TYPE_DELTA;
                uint16_t frame_len = frame_packet(s_payload_buffer,
                                                  enc_len,
                                                  pkt_type,
                                                  s_current_snapshot.sequence_num,
                                                  s_current_snapshot.timestamp_ticks,
                                                  s_framed_buffer,
                                                  (uint16_t)sizeof(s_framed_buffer));

                if (frame_len > 0U) {
                    transport_send(s_framed_buffer, frame_len);
                }
            }
        }

        measurement_record(&s_cycle_stats, profiler_stop(cycle_start));

#if RTOSTWIN_ENABLE_PROFILING
        loop_count++;
        if ((loop_count % RTOSTWIN_PROFILE_REPORT_INTERVAL) == 0U) {
            profiler_report(&snap_stats, "snapshot_capture");
            printf("[MEASURE] telemetry_cycle min=%lu max=%lu mean=%lu cycles samples=%lu\n",
                   (unsigned long)s_cycle_stats.min_cycles,
                   (unsigned long)s_cycle_stats.max_cycles,
                   (unsigned long)measurement_mean_cycles(&s_cycle_stats),
                   (unsigned long)s_cycle_stats.sample_count);
            measurement_reset(&s_cycle_stats);
        }
#else
        loop_count++;
#endif

        vTaskDelay(pdMS_TO_TICKS(RTOSTWIN_TELEMETRY_PERIOD_MS));
    }
}

rtostwin_status_t rtostwin_init(void)
{
    if (s_initialised) {
        return RTOSTWIN_STATUS_OK;
    }

    profiler_init();
    snapshot_init();
    encoder_init();
    transport_init();
    measurement_reset(&s_cycle_stats);

    s_initialised = true;
    return RTOSTWIN_STATUS_OK;
}

rtostwin_status_t rtostwin_start(void)
{
    BaseType_t created;

    if (!s_initialised) {
        rtostwin_status_t init_status = rtostwin_init();
        if (init_status != RTOSTWIN_STATUS_OK) {
            return init_status;
        }
    }

    if (s_telemetry_task_handle != NULL) {
        return RTOSTWIN_STATUS_ALREADY_STARTED;
    }

    created = xTaskCreate(rtostwin_telemetry_task,
                          RTOSTWIN_TELEMETRY_TASK_NAME,
                          RTOSTWIN_TELEMETRY_TASK_STACK_WORDS,
                          NULL,
                          tskIDLE_PRIORITY + RTOSTWIN_TELEMETRY_TASK_PRIORITY_OFFSET,
                          &s_telemetry_task_handle);
    if (created != pdTRUE) {
        s_telemetry_task_handle = NULL;
        return RTOSTWIN_STATUS_ERROR;
    }

    return RTOSTWIN_STATUS_OK;
}

rtostwin_status_t rtostwin_stop(void)
{
    if (s_telemetry_task_handle == NULL) {
        return RTOSTWIN_STATUS_OK;
    }

    vTaskDelete(s_telemetry_task_handle);
    s_telemetry_task_handle = NULL;
    return RTOSTWIN_STATUS_OK;
}

bool rtostwin_is_running(void)
{
    return s_telemetry_task_handle != NULL;
}

const char *rtostwin_version(void)
{
    return RTOSTWIN_VERSION_STRING;
}

#if RTOSTWIN_ENABLE_COMPAT_API
void StartTelemetryAgent(void)
{
    (void)rtostwin_init();
    (void)rtostwin_start();
}
#endif

#endif /* RTOSTWIN_ENABLE */
