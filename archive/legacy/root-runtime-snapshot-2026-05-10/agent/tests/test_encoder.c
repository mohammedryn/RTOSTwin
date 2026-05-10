/**
 * @file test_encoder.c
 * @brief Unit tests for delta encoding logic.
 */

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "../core/encoder.h"
#include "../core/encoder.c"

void test_encoder_keyframe() {
    encoder_init();
    full_snapshot_t snap = {0};
    uint8_t buf[512];
    
    /* Force a keyframe */
    uint16_t size = encoder_encode(&snap, buf, 512, true);
    
    /* Keyframe should be the full raw struct size */
    assert(size == sizeof(full_snapshot_t));
    printf("  [PASS] Keyframe size matches full struct (%d bytes)\n", size);
}

void test_encoder_delta_memory() {
    encoder_init();
    full_snapshot_t snap = {0};
    uint8_t buf[512];
    
    /* 1. Establish ground truth with keyframe */
    encoder_encode(&snap, buf, 512, true);
    
    /* 2. Change only the heap value */
    snap.memory.heap_free_bytes = 0x12345678;
    uint16_t delta_size = encoder_encode(&snap, buf, 512, false);
    
    /* 
     * Expected: TAG_MEM_HEAP_FREE (1) + Value (4) + TAG_END_OF_PACKET (1) = 6 bytes
     */
    assert(delta_size == 6);
    assert(buf[0] == TAG_MEM_HEAP_FREE);
    assert(buf[5] == TAG_END_OF_PACKET);
    printf("  [PASS] Delta encoding for memory is correct (6 bytes)\n");
}

void test_encoder_delta_task() {
    encoder_init();
    full_snapshot_t snap = {0};
    snap.task_count = 1;
    uint8_t buf[512];
    
    /* 1. Establish ground truth */
    encoder_encode(&snap, buf, 512, true);
    
    /* 2. Change task 0 state */
    snap.tasks[0].state = 2; /* eBlocked */
    uint16_t delta_size = encoder_encode(&snap, buf, 512, false);
    
    /*
     * Expected: 
     * TAG_TASK_INDEX (1) + Index 0 (1) 
     * + TAG_TASK_STATE (1) + Value 2 (1) 
     * + TAG_END_OF_PACKET (1) 
     * = 5 bytes
     */
    assert(delta_size == 5);
    assert(buf[0] == TAG_TASK_INDEX);
    assert(buf[1] == 0);
    assert(buf[2] == TAG_TASK_STATE);
    assert(buf[3] == 2);
    printf("  [PASS] Delta encoding for tasks is correct (5 bytes)\n");
}

int main() {
    printf("--- Encoder Unit Tests ---\n");
    test_encoder_keyframe();
    test_encoder_delta_memory();
    test_encoder_delta_task();
    printf("\nAll encoder tests PASSED ✅\n");
    return 0;
}
