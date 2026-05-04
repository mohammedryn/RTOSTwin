#ifndef RTOSTWIN_WIRE_FORMAT_H
#define RTOSTWIN_WIRE_FORMAT_H

/**
 * @file wire_format.h
 * @brief Protocol constants for the RTOSTwin binary wire format.
 * 
 * This file defines the synchronization bytes, versioning, packet types,
 * and CRC parameters shared between the C Telemetry Agent and the 
 * Python Bridge.
 * 
 * @note Follows TECH_SPEC.md Section 2.2 exactly.
 */

/* Synchronization bytes */
#define WF_SYNC_0               0xAAU
#define WF_SYNC_1               0x55U

/* Protocol version */
#define WF_PROTOCOL_VERSION     0x01U

/* Packet types */
#define WF_TYPE_DELTA           0x01U
#define WF_TYPE_KEYFRAME        0x02U
#define WF_TYPE_DEVICE_INFO     0x03U

/* Header and framing sizes */
#define WF_HEADER_SIZE          12U
#define WF_CRC_SIZE             2U
#define WF_OVERHEAD             (WF_HEADER_SIZE + WF_CRC_SIZE)
#define WF_MAX_PACKET_SIZE      512U
#define WF_KEYFRAME_INTERVAL    50U

/* CRC-16-CCITT parameters */
#define WF_CRC_POLY             0x1021U
#define WF_CRC_INIT             0xFFFFU

/* Delta encoding field identifiers */
#define WF_DELTA_FIELD_TASK_STATE        0x01U
#define WF_DELTA_FIELD_TASK_PRIORITY     0x02U
#define WF_DELTA_FIELD_TASK_STACK_HWM    0x03U
#define WF_DELTA_FIELD_TASK_RUNTIME      0x04U
#define WF_DELTA_FIELD_HEAP_FREE         0x05U
#define WF_DELTA_FIELD_HEAP_MIN_EVER     0x06U
#define WF_DELTA_FIELD_CPU_UTILIZATION   0x07U

/* Reserved system-tag range */
#define WF_DELTA_SYSTEM_TAG_BASE         0xF0U

#endif /* RTOSTWIN_WIRE_FORMAT_H */
