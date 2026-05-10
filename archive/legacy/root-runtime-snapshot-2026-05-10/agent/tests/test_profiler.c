/**
 * @file test_profiler.c
 * @brief Unit tests for profiler_record() and profiler_stop() logic.
 *
 * Since we have no real STM32 hardware here, we:
 *  1. Mock dwt_get_cycles() with a variable we control.
 *  2. Feed known values to profiler_record() and assert the outcomes.
 *
 * Run:
 *   gcc test_profiler.c ../core/profiler.c -I ../core -I ../hal/stm32 -o test_profiler && ./test_profiler
 */

#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

/* ── Mock the hardware layer ──────────────────────────────────────── */
static uint32_t g_mock_cycles = 0;

/* These replace the real dwt_init / dwt_get_cycles from dwt.c */
bool     dwt_init(void)        { return true; }
uint32_t dwt_get_cycles(void)  { return g_mock_cycles; }

/* Now include the real profiler implementation */
#include "../core/profiler.h"
#include "../core/profiler.c"   /* Include .c so we compile one unit */

/* ── Helpers ──────────────────────────────────────────────────────── */
#define PASS(name) printf("  [PASS] %s\n", name)

/* ── Tests ────────────────────────────────────────────────────────── */

/* Test 1: First record initialises min and max correctly */
static void test_first_record_sets_min_max(void) {
    profiler_stats_t s = {0};
    profiler_record(&s, 100U);

    assert(s.min_cycles  == 100U);
    assert(s.max_cycles  == 100U);
    assert(s.call_count  == 1U);
    assert(s.total_cycles == 100U);
    PASS("first_record_sets_min_max");
}

/* Test 2: New maximum is detected */
static void test_new_maximum(void) {
    profiler_stats_t s = {0};
    profiler_record(&s, 100U);
    profiler_record(&s, 200U);   /* New max */

    assert(s.max_cycles == 200U);
    assert(s.min_cycles == 100U);
    assert(s.call_count == 2U);
    PASS("new_maximum_detected");
}

/* Test 3: New minimum is detected */
static void test_new_minimum(void) {
    profiler_stats_t s = {0};
    profiler_record(&s, 100U);
    profiler_record(&s, 50U);    /* New min */

    assert(s.min_cycles == 50U);
    assert(s.max_cycles == 100U);
    PASS("new_minimum_detected");
}

/* Test 4: Average (mean) calculation */
static void test_mean_calculation(void) {
    profiler_stats_t s = {0};
    profiler_record(&s, 100U);
    profiler_record(&s, 200U);
    profiler_record(&s, 300U);   /* total=600, count=3, mean=200 */

    uint32_t mean = (uint32_t)(s.total_cycles / s.call_count);
    assert(mean == 200U);
    PASS("mean_calculation");
}

/* Test 5: profiler_stop() wrap-around (simulated) */
static void test_stop_wrap_around(void) {
    /* start is close to 0xFFFFFFFF */
    uint32_t start = 0xFFFFFFF0U;
    uint32_t end   = 0x00000010U;

    /* Simulate: set mock clock to 'end', call profiler_stop(start) */
    g_mock_cycles = end;
    uint32_t elapsed = profiler_stop(start);

    /* Expected: 0x10 - 0xFFFFFFF0 = 0x20 = 32 (unsigned math) */
    assert(elapsed == 32U);
    PASS("stop_wrap_around");
}

/* ── Main ─────────────────────────────────────────────────────────── */
int main(void) {
    printf("--- Profiler Unit Tests ---\n");

    profiler_init();   /* Calls mock dwt_init, always returns true */

    test_first_record_sets_min_max();
    test_new_maximum();
    test_new_minimum();
    test_mean_calculation();
    test_stop_wrap_around();

    printf("\nAll profiler tests PASSED ✅\n");
    return 0;
}
