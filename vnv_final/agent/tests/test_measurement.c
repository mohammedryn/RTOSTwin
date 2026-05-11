#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "../core/measurement.h"

#define PASS(name) printf("  [PASS] %s\n", name)

static void test_record_initialises_min_max_count_and_total(void)
{
    measurement_stats_t stats = {0};

    measurement_record(&stats, 120U);

    assert(stats.min_cycles == 120U);
    assert(stats.max_cycles == 120U);
    assert(stats.sample_count == 1U);
    assert(stats.total_cycles == 120U);
    PASS("record_initialises_min_max_count_and_total");
}

static void test_record_updates_running_min_max_and_mean(void)
{
    measurement_stats_t stats = {0};

    measurement_record(&stats, 150U);
    measurement_record(&stats, 90U);
    measurement_record(&stats, 210U);

    assert(stats.min_cycles == 90U);
    assert(stats.max_cycles == 210U);
    assert(stats.sample_count == 3U);
    assert(stats.total_cycles == 450U);
    assert(measurement_mean_cycles(&stats) == 150U);
    PASS("record_updates_running_min_max_and_mean");
}

static void test_reset_clears_all_fields(void)
{
    measurement_stats_t stats = {
        .min_cycles = 1U,
        .max_cycles = 2U,
        .sample_count = 3U,
        .total_cycles = 4U
    };

    measurement_reset(&stats);

    assert(stats.min_cycles == 0U);
    assert(stats.max_cycles == 0U);
    assert(stats.sample_count == 0U);
    assert(stats.total_cycles == 0U);
    PASS("reset_clears_all_fields");
}

int main(void)
{
    printf("--- Measurement Unit Tests ---\n");
    test_record_initialises_min_max_count_and_total();
    test_record_updates_running_min_max_and_mean();
    test_reset_clears_all_fields();
    printf("\nAll measurement tests PASSED\n");
    return 0;
}
