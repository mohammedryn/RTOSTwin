/**
 * @file encoder.h
 * @brief Delta compression for RTOS snapshots.
 *
 * Compares current snapshot to the last sent snapshot and 
 * encodes only the differences (deltas).
 */

#ifndef AGENT_CORE_ENCODER_H
#define AGENT_CORE_ENCODER_H

#include "snapshot.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Initialize or reset the encoder state.
 * 
 * Clears the 'last_snapshot' buffer so the next call to 
 * encoder_encode() results in a full keyframe.
 */
void encoder_init(void);

/**
 * @brief Encode a snapshot into the output buffer.
 * 
 * @param current Pointer to the fresh snapshot from the engine.
 * @param out_buf Buffer to store the encoded bytes.
 * @param out_buf_size Size of out_buf.
 * @param force_keyframe If true, ignores last state and sends full data.
 * @return uint16_t Number of bytes written to out_buf.
 */
uint16_t encoder_encode(const full_snapshot_t *current,
                        uint8_t *out_buf, uint16_t out_buf_size,
                        bool force_keyframe);

bool encoder_last_was_keyframe(void);

#endif /* AGENT_CORE_ENCODER_H */
