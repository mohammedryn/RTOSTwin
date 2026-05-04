/**
 * @file profiler.h
 * @brief High-precision performance profiling for RTOS tasks.
 *
 * Uses the ARM DWT cycle counter to measure execution time with
 * cycle-level accuracy.
 */

#ifndef AGENT_CORE_PROFILER_H
#define AGENT_CORE_PROFILER_H

#include <stdint.h>

/**
 * @brief Statistics for a single profiling point.
 *
 * Stores accumulated Min/Max/Average cycle counts for one
 * measured section of code. Caller is responsible for zeroing
 * this struct before the first call to profiler_record().
 */
typedef struct {
    uint32_t min_cycles;    /**< Minimum observed cycles */
    uint32_t max_cycles;    /**< Maximum observed cycles */
    uint32_t call_count;    /**< How many times recorded */
    uint64_t total_cycles;  /**< Running sum (used to compute mean) */
} profiler_stats_t;

/** @brief Enable DWT hardware. Must be called once at startup. */
void     profiler_init(void);

/** @brief Read the current cycle count. Call this just BEFORE the code you want to measure. */
uint32_t profiler_start(void);

/** @brief Calculate elapsed cycles. Call this just AFTER the code you measured. */
uint32_t profiler_stop(uint32_t start_cycles);

/** @brief Update a stats struct with a new elapsed value. */
void     profiler_record(profiler_stats_t *stats, uint32_t elapsed_cycles);

/** @brief Print "[PROFILER] label: min=X max=Y mean=Z cycles" over UART/printf. */
void     profiler_report(const profiler_stats_t *stats, const char *label);

#endif /* AGENT_CORE_PROFILER_H */
