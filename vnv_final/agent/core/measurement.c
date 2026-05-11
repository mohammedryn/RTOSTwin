#include "measurement.h"

void measurement_record(measurement_stats_t *stats, uint32_t elapsed_cycles)
{
    if (stats == 0) {
        return;
    }

    if (stats->sample_count == 0U) {
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

    stats->sample_count += 1U;
    stats->total_cycles += (uint64_t)elapsed_cycles;
}

uint32_t measurement_mean_cycles(const measurement_stats_t *stats)
{
    if (stats == 0 || stats->sample_count == 0U) {
        return 0U;
    }

    return (uint32_t)(stats->total_cycles / stats->sample_count);
}

void measurement_reset(measurement_stats_t *stats)
{
    if (stats == 0) {
        return;
    }

    stats->min_cycles = 0U;
    stats->max_cycles = 0U;
    stats->sample_count = 0U;
    stats->total_cycles = 0U;
}
