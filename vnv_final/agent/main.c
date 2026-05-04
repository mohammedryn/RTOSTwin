/**
 * @file main.c
 * @brief Main entry point for the RTOSTwin Telemetry Agent.
 *
 * This file implements the "Heartbeat" task (Task 13). It orchestrates
 * the capture, encoding, framing, and transmission of telemetry.
 */

#include "FreeRTOS.h"
#include "task.h"
#include "core/wire_format.h"
#include "core/snapshot.h"
#include "core/encoder.h"
#include "core/framer.h"
#include "core/transport.h"
#include "core/profiler.h"

/* 
 * Shared static buffers.
 * These are placed in the .bss section to keep task stack usage low.
 */
static full_snapshot_t s_current_snapshot;
static uint8_t s_payload_buffer[512];
static uint8_t s_framed_buffer[540];

/**
 * @brief Telemetry Task (Task 13)
 * 
 * Runs at 10 Hz. Coordinates the VNV pipeline.
 */
void vTelemetryTask(void *pvParameters) {
    (void)pvParameters;

    /* Initialize all modules */
    snapshot_init();
    encoder_init();
    framer_reset_sequence(); /* Ensure seq starts at 0 */
    transport_init();
    
    profiler_stats_t snap_stats = {0};
    uint32_t loop_count = 0;

    while (1) {
        /* 1. Capture Snapshot + Profile Performance */
        uint32_t t_start = profiler_start();
        snapshot_capture(&s_current_snapshot);
        uint32_t t_elapsed = profiler_stop(t_start);
        profiler_record(&snap_stats, t_elapsed);

        /* 2. Delta Encoding (Force keyframe every 50 iterations) */
        bool force_keyframe = (loop_count % 50 == 0);
        uint16_t enc_len = encoder_encode(&s_current_snapshot, 
                                          s_payload_buffer, 
                                          sizeof(s_payload_buffer), 
                                          force_keyframe);

        if (enc_len > 0) {
            /* 
             * Determine Packet Type:
             * If enc_len matches full struct size, it's a KEYFRAME.
             * Otherwise, it's a DELTA.
             */
            uint8_t pkt_type = (enc_len == sizeof(full_snapshot_t)) ? 
                                WF_TYPE_KEYFRAME : WF_TYPE_DELTA;

            /* 3. Framing (Sync + Header + CRC) 
             * Added missing arguments: pkt_type and xTaskGetTickCount()
             */
            uint16_t frame_len = frame_packet(s_payload_buffer, 
                                              enc_len, 
                                              pkt_type,
                                              xTaskGetTickCount(),
                                              s_framed_buffer, 
                                              sizeof(s_framed_buffer));

            /* 4. Asynchronous Transmission (DMA) */
            if (frame_len > 0) {
                transport_send(s_framed_buffer, frame_len);
            }
        }

        /* 5. Performance Reporting (every 10 seconds) */
        if (++loop_count % 100 == 0) {
            profiler_report(&snap_stats, "snapshot_pipeline");
        }

        /* Sleep for 100ms (10 Hz rate) */
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/**
 * @brief Main function (stub)
 * 
 * Launch the task. In a real project, this is called from your main().
 */
void StartTelemetryAgent(void) {
    xTaskCreate(vTelemetryTask, 
                "TelemetryTask", 
                512, /* Increased stack size for safety */
                NULL, 
                tskIDLE_PRIORITY + 2, /* Slightly higher than other app tasks */
                NULL);
}
