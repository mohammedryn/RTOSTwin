/**
 * @file test_encoder.c
 * @brief Unit tests for delta encoding logic.
 *
 * This test verifies that the encoder correctly produces keyframes and
 * compact delta packets using the nibble-based tagging system.
 */

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

/* We include the .c to test static variables/functions easily in a unit test */
#include "../core/encoder.c"

/* Mock structure mirroring snapshot.h if not available in include path */
/* (In a real build, we'd include snapshot.h) */

void test_encoder_keyframe(void) {
    encoder_init();
    full_snapshot_t snap;
    memset(&snap, 0, sizeof(full_snapshot_t));
    uint8_t buf[512];
    
    /* 1. First call without force_keyframe should STILL be a keyframe because it's unitialized */
    uint16_t size = encoder_encode(&snap, buf, 512, false);
    assert(size == sizeof(full_snapshot_t));
    
    /* 2. Force keyframe */
    size = encoder_encode(&snap, buf, 512, true);
    assert(size == sizeof(full_snapshot_t));
    
    printf("  [PASS] Keyframe logic correct (%d bytes)\n", size);
}

void test_encoder_delta_memory(void) {
    encoder_init();
    full_snapshot_t snap;
    memset(&snap, 0, sizeof(full_snapshot_t));
    uint8_t buf[512];
    
    /* 1. Establish ground truth */
    encoder_encode(&snap, buf, 512, true);
    
    /* 2. Change only the heap free value */
    snap.memory.heap_free_bytes = 0x12345678;
    uint16_t delta_size = encoder_encode(&snap, buf, 512, false);
    
    /* Expected: Tag (0xF5) + Value (4 bytes) = 5 bytes total */
    assert(delta_size == 5);
    assert(buf[0] == 0xF5);
    assert(buf[1] == 0x78); /* Little-endian check */
    assert(buf[4] == 0x12);
    
    /* 3. Change heap min ever */
    snap.memory.heap_min_ever_bytes = 0xAAAAAAAA;
    delta_size = encoder_encode(&snap, buf, 512, false);
    
    /* Expected: Tag (0xF6) + Value (4 bytes) = 5 bytes total */
    assert(delta_size == 5);
    assert(buf[0] == 0xF6);
    
    printf("  [PASS] Memory deltas correct (System nibble 0xF used)\n");
}

void test_encoder_delta_task(void) {
    encoder_init();
    full_snapshot_t snap;
    memset(&snap, 0, sizeof(full_snapshot_t));
    snap.task_count = 5;
    uint8_t buf[512];
    
    /* 1. Establish ground truth */
    encoder_encode(&snap, buf, 512, true);
    
    /* 2. Change Task 2 state and Task 4 priority */
    /* Task 2 State Tag: (2 << 4) | 0x01 = 0x21 */
    /* Task 4 Prio  Tag: (4 << 4) | 0x02 = 0x42 */
    snap.tasks[2].state = 3;
    snap.tasks[4].priority = 10;
    
    uint16_t delta_size = encoder_encode(&snap, buf, 512, false);
    
    /* Expected: (Tag 0x21 + 1 byte) + (Tag 0x42 + 1 byte) = 4 bytes */
    assert(delta_size == 4);
    
    /* We don't strictly enforce order in the encoder, but we check if both tags are present */
    bool found_t2 = false;
    bool found_t4 = false;
    for(int i=0; i<delta_size; i++) {
        if(buf[i] == 0x21) {
            assert(buf[i+1] == 3);
            found_t2 = true;
        }
        if(buf[i] == 0x42) {
            assert(buf[i+1] == 10);
            found_t4 = true;
        }
    }
    assert(found_t2 && found_t4);
    
    printf("  [PASS] Task deltas correct (Task indices preserved in nibbles)\n");
}

void test_encoder_runtime_delta(void) {
    encoder_init();
    full_snapshot_t snap;
    memset(&snap, 0, sizeof(full_snapshot_t));
    snap.task_count = 1;
    uint8_t buf[512];
    
    encoder_encode(&snap, buf, 512, true);
    
    /* Change Task 0 runtime: (0 << 4) | 0x04 = 0x04 */
    snap.tasks[0].runtime_ticks = 1000;
    uint16_t delta_size = encoder_encode(&snap, buf, 512, false);
    
    /* Expected: Tag (0x04) + 4 bytes = 5 bytes */
    assert(delta_size == 5);
    assert(buf[0] == 0x04);
    
    printf("  [PASS] Runtime deltas correct (4-byte ticks)\n");
}

int main(void) {
    printf("--- Running Encoder Unit Tests ---\n\n");
    
    test_encoder_keyframe();
    test_encoder_delta_memory();
    test_encoder_delta_task();
    test_encoder_runtime_delta();
    
    printf("\nALL ENCODER TESTS PASSED ✅\n");
    return 0;
}
