#ifndef VNV_FINAL_AGENT_CORE_MEASUREMENT_H
#define VNV_FINAL_AGENT_CORE_MEASUREMENT_H

#include <stdint.h>

typedef struct {
    uint32_t min_cycles;
    uint32_t max_cycles;
    uint32_t sample_count;
    uint64_t total_cycles;
} measurement_stats_t;

void measurement_record(measurement_stats_t *stats, uint32_t elapsed_cycles);
uint32_t measurement_mean_cycles(const measurement_stats_t *stats);
void measurement_reset(measurement_stats_t *stats);

#endif /* VNV_FINAL_AGENT_CORE_MEASUREMENT_H */
