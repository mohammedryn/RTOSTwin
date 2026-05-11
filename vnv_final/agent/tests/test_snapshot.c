/**
 * @file test_snapshot.c
 * @brief Unit tests for snapshot_capture() logic.
 *
 * Since FreeRTOS APIs don't exist on a PC, we mock every function
 * that snapshot.c calls. This lets us control exactly what data
 * "FreeRTOS" returns and verify our snapshot logic is correct.
 *
 * Run:
 *   gcc test_snapshot.c ../core/snapshot.c -I ../core -I mocks -o test_snapshot && ./test_snapshot
 */

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>

/*
 * Objective 1 audit anchor:
 * snapshot_capture() must continue using only fixed/static storage and must
 * not introduce malloc/pvPortMalloc into the telemetry hot path.
 */

/* ── FreeRTOS Type Stubs ────────────────────────────────────────── */
typedef uint32_t TickType_t;
typedef uint32_t UBaseType_t;

typedef enum {
    eRunning = 0,
    eReady,
    eBlocked,
    eSuspended,
    eDeleted
} eTaskState;

typedef struct {
    const char  *pcTaskName;
    eTaskState   eCurrentState;
    UBaseType_t  uxCurrentPriority;
    uint16_t     usStackHighWaterMark;
    uint32_t     ulRunTimeCounter;
} TaskStatus_t;

/* ── FreeRTOS API Mocks ─────────────────────────────────────────── */
static TaskStatus_t g_mock_tasks[3];
static uint32_t     g_mock_heap_free     = 40000U;
static uint32_t     g_mock_heap_min_ever = 38000U;

TickType_t xTaskGetTickCount(void)               { return 5000U; }
size_t     xPortGetFreeHeapSize(void)            { return g_mock_heap_free; }
size_t     xPortGetMinimumEverFreeHeapSize(void) { return g_mock_heap_min_ever; }
void       taskENTER_CRITICAL(void)              {}
void       taskEXIT_CRITICAL(void)               {}

UBaseType_t uxTaskGetSystemState(TaskStatus_t *buf, UBaseType_t max, uint32_t *total) {
    (void)max; (void)total;
    buf[0] = g_mock_tasks[0];
    buf[1] = g_mock_tasks[1];
    buf[2] = g_mock_tasks[2];
    return 3U;
}

/* Pull in real implementation */
#include "../core/snapshot.h"
#include "../core/snapshot.c"

/* ── Helpers ────────────────────────────────────────────────────── */
#define PASS(name) printf("  [PASS] %s\n", name)

/* ── Tests ──────────────────────────────────────────────────────── */

static void test_snapshot_timestamp(void) {
    full_snapshot_t snap = {0};
    snapshot_capture(&snap);
    assert(snap.timestamp_ticks == 5000U);
    PASS("timestamp_is_correct");
}

static void test_snapshot_task_count(void) {
    full_snapshot_t snap = {0};
    snapshot_capture(&snap);
    assert(snap.task_count == 3U);
    PASS("task_count_is_3");
}

static void test_snapshot_task_fields(void) {
    full_snapshot_t snap = {0};
    snapshot_capture(&snap);

    /* Check first task */
    assert(snap.tasks[0].state    == (uint8_t)eRunning);
    assert(snap.tasks[0].priority == 3U);
    assert(strcmp(snap.tasks[0].name, "LED_Task") == 0);
    PASS("task_fields_copied_correctly");
}

static void test_snapshot_heap_fields(void) {
    full_snapshot_t snap = {0};
    snapshot_capture(&snap);
    assert(snap.memory.heap_free_bytes     == 40000U);
    assert(snap.memory.heap_min_ever_bytes == 38000U);
    PASS("heap_fields_correct");
}

static void test_snapshot_cpu_utilisation(void) {
    /* Simulate 200 idle ticks out of 1000 total → CPU = 80% */
    /* 
     * vApplicationIdleHook() increments s_idle_ticks and s_period_ticks.
     * We call the hook manually to set up the counters.
     */
    snapshot_init();

    /* Simulate 1000 total ticks of which 200 are idle */
    for (int i = 0; i < 1000; i++) { s_period_ticks++; }
    for (int i = 0; i < 200;  i++) { s_idle_ticks++;   }

    full_snapshot_t snap = {0};
    snapshot_capture(&snap);

    /* cpu = 100 - (200*100/1000) = 100 - 20 = 80% */
    assert(snap.memory.cpu_utilization_pct == 80U);
    PASS("cpu_utilization_80_percent");
}

static void test_null_pointer_safety(void) {
    /* Must not crash */
    snapshot_capture(NULL);
    PASS("null_pointer_safe");
}

/* ── Main ───────────────────────────────────────────────────────── */
int main(void) {
    printf("--- Snapshot Engine Unit Tests ---\n");

    /* Set up mock task data */
    g_mock_tasks[0].pcTaskName          = "LED_Task";
    g_mock_tasks[0].eCurrentState       = eRunning;
    g_mock_tasks[0].uxCurrentPriority   = 3U;
    g_mock_tasks[0].usStackHighWaterMark = 180U;
    g_mock_tasks[0].ulRunTimeCounter    = 12345U;

    g_mock_tasks[1].pcTaskName          = "UART_Task";
    g_mock_tasks[1].eCurrentState       = eBlocked;
    g_mock_tasks[1].uxCurrentPriority   = 2U;
    g_mock_tasks[1].usStackHighWaterMark = 90U;
    g_mock_tasks[1].ulRunTimeCounter    = 5000U;

    g_mock_tasks[2].pcTaskName          = "Sensor_Task";
    g_mock_tasks[2].eCurrentState       = eReady;
    g_mock_tasks[2].uxCurrentPriority   = 1U;
    g_mock_tasks[2].usStackHighWaterMark = 120U;
    g_mock_tasks[2].ulRunTimeCounter    = 8800U;

    snapshot_init();
    test_snapshot_timestamp();
    test_snapshot_task_count();
    test_snapshot_task_fields();
    test_snapshot_heap_fields();
    test_snapshot_cpu_utilisation();
    test_null_pointer_safety();

    printf("\nAll snapshot tests PASSED ✅\n");
    return 0;
}
