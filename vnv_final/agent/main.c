/**
 * @file main.c
 * @brief Main entry point for the RTOSTwin Telemetry Agent.
 */

#include "FreeRTOS.h"
#include "task.h"
#include "../../agent/core/wire_format.h"
#include "core/snapshot.h"
#include "core/encoder.h"
#include "core/framer.h"
#include "core/transport.h"
#include "core/profiler.h"

static full_snapshot_t s_current_snapshot;
static uint8_t s_payload_buffer[512];
static uint8_t s_framed_buffer[540];

void vTelemetryTask(void *pvParameters)
{
    (void)pvParameters;

    snapshot_init();
    encoder_init();
    transport_init();

    profiler_stats_t snap_stats = {0};
    uint32_t loop_count = 0u;

    while (1) {
        uint32_t t_start = profiler_start();
        snapshot_capture(&s_current_snapshot);
        uint32_t t_elapsed = profiler_stop(t_start);
        profiler_record(&snap_stats, t_elapsed);

        bool force_keyframe = ((loop_count % WF_KEYFRAME_INTERVAL) == 0u);
        uint16_t enc_len = encoder_encode(&s_current_snapshot,
                                          s_payload_buffer,
                                          sizeof(s_payload_buffer),
                                          force_keyframe);

        if (enc_len > 0u) {
            uint8_t pkt_type = encoder_last_was_keyframe() ? WF_TYPE_KEYFRAME : WF_TYPE_DELTA;
            uint16_t frame_len = frame_packet(s_payload_buffer,
                                              enc_len,
                                              pkt_type,
                                              s_current_snapshot.sequence_num,
                                              s_current_snapshot.timestamp_ticks,
                                              s_framed_buffer,
                                              sizeof(s_framed_buffer));

            if (frame_len > 0u) {
                transport_send(s_framed_buffer, frame_len);
            }
        }

        if ((++loop_count % 100u) == 0u) {
            profiler_report(&snap_stats, "snapshot_pipeline");
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void StartTelemetryAgent(void)
{
    xTaskCreate(vTelemetryTask,
                "TelemetryTask",
                512,
                NULL,
                tskIDLE_PRIORITY + 2,
                NULL);
}
