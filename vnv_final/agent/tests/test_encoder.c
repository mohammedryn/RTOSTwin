#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../core/encoder.c"

static void zero_snapshot(full_snapshot_t *snapshot)
{
    memset(snapshot, 0, sizeof(full_snapshot_t));
}

static void test_keyframe_matches_root_golden_payload(void)
{
    full_snapshot_t snap;
    uint8_t buf[64];
    const uint8_t expected[] = {
        0x34u, 0x12u, 0x04u, 0x03u, 0x02u, 0x01u, 0x01u,
        0x49u, 0x44u, 0x4Cu, 0x45u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
        0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
        0x00u, 0x00u, 0x20u, 0x00u, 0x64u, 0x00u, 0x00u, 0x00u,
        0x40u, 0x1Fu, 0x00u, 0x00u, 0x00u, 0x1Eu, 0x00u, 0x00u, 0x07u
    };

    encoder_init();
    zero_snapshot(&snap);

    snap.sequence_num = 0x1234u;
    snap.timestamp_ticks = 0x01020304u;
    snap.task_count = 1u;
    memcpy(snap.tasks[0].name, "IDLE", 5u);
    snap.tasks[0].stack_hwm_words = 32u;
    snap.tasks[0].runtime_ticks = 100u;
    snap.memory.heap_free_bytes = 8000u;
    snap.memory.heap_min_ever_bytes = 7680u;
    snap.memory.cpu_utilization_pct = 7u;

    assert(encoder_encode(&snap, buf, (uint16_t)sizeof(buf), true) == (uint16_t)sizeof(expected));
    assert(encoder_last_was_keyframe() == true);
    assert(memcmp(buf, expected, sizeof(expected)) == 0);
}

static void test_delta_memory_tags(void)
{
    full_snapshot_t snap;
    uint8_t buf[64];

    encoder_init();
    zero_snapshot(&snap);
    assert(encoder_encode(&snap, buf, (uint16_t)sizeof(buf), true) == 16u);

    snap.memory.heap_free_bytes = 0x12345678u;
    assert(encoder_encode(&snap, buf, (uint16_t)sizeof(buf), false) == 5u);
    assert(encoder_last_was_keyframe() == false);
    assert(buf[0] == (uint8_t)(WF_DELTA_SYSTEM_TAG_BASE | WF_DELTA_FIELD_HEAP_FREE));
    assert(buf[1] == 0x78u);
    assert(buf[4] == 0x12u);
}

static void test_task_topology_change_forces_keyframe(void)
{
    full_snapshot_t snap;
    uint8_t buf[128];

    encoder_init();
    zero_snapshot(&snap);
    snap.task_count = 1u;
    memcpy(snap.tasks[0].name, "IDLE", 5u);
    assert(encoder_encode(&snap, buf, (uint16_t)sizeof(buf), true) == 40u);

    snap.task_count = 2u;
    memcpy(snap.tasks[1].name, "COMM", 5u);
    assert(encoder_encode(&snap, buf, (uint16_t)sizeof(buf), false) == 64u);
    assert(encoder_last_was_keyframe() == true);
}

int main(void)
{
    printf("--- Running Encoder Unit Tests ---\n\n");

    test_keyframe_matches_root_golden_payload();
    printf("  [PASS] Golden keyframe payload\n");

    test_delta_memory_tags();
    printf("  [PASS] Delta memory tags\n");

    test_task_topology_change_forces_keyframe();
    printf("  [PASS] Topology change forces keyframe\n");

    printf("\nALL ENCODER TESTS PASSED\n");
    return 0;
}
