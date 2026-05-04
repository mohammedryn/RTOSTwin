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

/* Synchronization Bytes */
#define WF_SYNC_0               0xAAU
#define WF_SYNC_1               0x55U

/* Protocol Version */
#define WF_PROTOCOL_VERSION     0x01U

/* Packet Types */
#define WF_TYPE_DELTA           0x01U
#define WF_TYPE_KEYFRAME        0x02U
#define WF_TYPE_DEVICE_INFO     0x03U

/* Protocol Constraints */
#define WF_KEYFRAME_INTERVAL    50U
#define WF_MAX_PACKET_SIZE      512U

/* Header and Framing Sizes */
#define WF_HEADER_SIZE          12U
#define WF_CRC_SIZE             2U
#define WF_OVERHEAD             (WF_HEADER_SIZE + WF_CRC_SIZE)

/* CRC-16-CCITT Parameters */
#define WF_CRC_POLY             0x1021U
#define WF_CRC_INIT             0xFFFFU

#endif /* RTOSTWIN_WIRE_FORMAT_H */
