/**
 * @file profiler.c
 * @brief Implementation of high-precision performance profiling.
 */

#include "profiler.h"
#include "../hal/stm32/dwt.h"
#include <stdio.h>

/* ------------------------------------------------------------------ */
/* Public API                                                           */
/* ------------------------------------------------------------------ */

void profiler_init(void) {
    /* Delegate entirely to the hardware layer */
    dwt_init();
}

uint32_t profiler_start(void) {
    /* Take a snapshot of the cycle counter right now */
    return dwt_get_cycles();
}

uint32_t profiler_stop(uint32_t start_cycles) {
    uint32_t end_cycles = dwt_get_cycles();

    /*
     * KEY INSIGHT: Unsigned subtraction automatically handles
     * 32-bit wrap-around (CYCCNT rolls from 0xFFFFFFFF → 0x00000000).
     *
     * Example:
     *   start = 0xFFFFFFF0  (close to overflow)
     *   end   = 0x00000010  (just after overflow)
     *   end - start = 0x10 - 0xFFFFFFF0
     *               = 0x00000020  (32 decimal) ✅ Correct!
     *
     * This works because C unsigned subtraction is modular arithmetic.
     */
    return (end_cycles - start_cycles);
}

void profiler_record(profiler_stats_t *stats, uint32_t elapsed_cycles) {
    /* First ever sample: initialise min/max to this reading */
    if (stats->call_count == 0U) {
        stats->min_cycles = elapsed_cycles;
        stats->max_cycles = elapsed_cycles;
    } else {
        if (elapsed_cycles < stats->min_cycles) {
            stats->min_cycles = elapsed_cycles;
        }
        if (elapsed_cycles > stats->max_cycles) {
            stats->max_cycles = elapsed_cycles;
        }
    }

    stats->total_cycles += (uint64_t)elapsed_cycles;
    stats->call_count   += 1U;
}

void profiler_report(const profiler_stats_t *stats, const char *label) {
    if (stats == NULL || label == NULL || stats->call_count == 0U) {
        return;
    }

    uint32_t mean = (uint32_t)(stats->total_cycles / stats->call_count);

    printf("[PROFILER] %s min=%lu max=%lu mean=%lu cycles samples=%lu\n",
           label,
           (unsigned long)stats->min_cycles,
           (unsigned long)stats->max_cycles,
           (unsigned long)mean,
           (unsigned long)stats->call_count);
}
