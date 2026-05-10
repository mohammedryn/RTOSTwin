/**
 * @file dwt.h
 * @brief Hardware Access Layer for ARM Cortex-M DWT Cycle Counter.
 *
 * Provides high-precision cycle-accurate timing using the processor's
 * internal hardware counter.
 */

#ifndef AGENT_HAL_STM32_DWT_H
#define AGENT_HAL_STM32_DWT_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Initializes and enables the DWT Cycle Counter.
 * 
 * Must be called before dwt_get_cycles().
 * Handles unlocking the DWT on Cortex-M7 and enabling the TRCENA bit.
 * 
 * @return true if successfully enabled, false if DWT is not available.
 */
bool dwt_init(void);

/**
 * @brief Reads the current processor cycle count.
 * 
 * @return 32-bit cycle count (DWT->CYCCNT).
 */
uint32_t dwt_get_cycles(void);

#endif /* AGENT_HAL_STM32_DWT_H */
