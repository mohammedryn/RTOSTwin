/**
 * @file dwt.c
 * @brief Implementation of DWT Hardware Access Layer.
 */

#include "dwt.h"

/* ARM Cortex-M Core Debug and DWT Register Definitions */
/* These are fixed memory addresses defined by the ARM Architecture */
#define DWT_CONTROL             (*((volatile uint32_t*)0xE0001000))
#define DWT_CYCCNT              (*((volatile uint32_t*)0xE0001004))
#define DEMCR                   (*((volatile uint32_t*)0xE000EDFC))
#define LAR                     (*((volatile uint32_t*)0xE0001FB0))

/* Bit definitions for controlling the hardware */
#define DEMCR_TRCENA            (1UL << 24)
#define DWT_CONTROL_CYCCNTENA   (1UL << 0)
#define LAR_UNLOCK_KEY          0xC5ACCE55

bool dwt_init(void) {
    /* 1. Enable TRCENA bit in Debug Exception and Monitor Control Register.
     * This turns on the "Global" clock for the debug unit. */
    DEMCR |= DEMCR_TRCENA;

    /* 2. Unlock DWT (Required on newer Cortex-M7 devices like STM32F7/H7).
     * Think of this like putting in the PIN for a safe. */
    LAR = LAR_UNLOCK_KEY;

    /* 3. Reset the cycle counter to zero. */
    DWT_CYCCNT = 0;

    /* 4. Enable the cycle counter bit in DWT control register.
     * This starts the clock ticking! */
    DWT_CONTROL |= DWT_CONTROL_CYCCNTENA;

    /* 5. Verification: Check if the counter is actually running. */
    uint32_t start = DWT_CYCCNT;
    for(volatile int i = 0; i < 100; i++);
    
    // If the count changed, the hardware is alive!
    return (DWT_CYCCNT > start);
}

uint32_t dwt_get_cycles(void) {
    /* Read the raw hardware register directly. 
     * This takes only 1 or 2 CPU instructions—super fast! */
    return DWT_CYCCNT;
}
