/*
 * Module 0.1 — Assignment 4: circular_buffer.c
 *
 * OBJECTIVE:
 *   Implement a lock-free circular buffer — the transmit queue in RTOSTwin.
 *
 * INSTRUCTIONS:
 *   gcc -Wall -Wextra -std=c99 -o circular_buffer circular_buffer.c
 *   ./circular_buffer
 *
 * KEY CONCEPT:
 *   BUFFER_SIZE must be a power of 2 (2, 4, 8, 16, 32, ...).
 *   This allows using bitwise AND instead of modulo for wrapping:
 *     index = (index + 1) & (BUFFER_SIZE - 1)
 *   This is faster and works in ISR context (no division).
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#define BUFFER_SIZE  8  // Power of 2! (small for easy testing)

typedef struct {
    uint8_t  data[BUFFER_SIZE];
    uint16_t head;  // Write index (producer writes here)
    uint16_t tail;  // Read index (consumer reads here)
} circular_buffer_t;


/* TODO: Implement all 4 functions below */

void cbuf_init(circular_buffer_t *cb) {
    // YOUR CODE HERE
    // Zero out head and tail
}

bool cbuf_is_full(const circular_buffer_t *cb) {
    // YOUR CODE HERE
    // Full when: (head + 1) % SIZE == tail
    // Use & (BUFFER_SIZE - 1) instead of %
    return false;
}

bool cbuf_is_empty(const circular_buffer_t *cb) {
    // YOUR CODE HERE
    // Empty when: head == tail
    return true;
}

bool cbuf_push(circular_buffer_t *cb, uint8_t byte) {
    // YOUR CODE HERE
    // 1. Check if full → return false
    // 2. Write byte to data[head]
    // 3. Advance head with wrap-around
    // 4. Return true
    return false;
}

bool cbuf_pop(circular_buffer_t *cb, uint8_t *byte) {
    // YOUR CODE HERE
    // 1. Check if empty → return false
    // 2. Read byte from data[tail]
    // 3. Advance tail with wrap-around
    // 4. Return true
    return false;
}

/* Helper: Count items in buffer */
uint16_t cbuf_count(const circular_buffer_t *cb) {
    // YOUR CODE HERE
    // HINT: (head - tail) & (BUFFER_SIZE - 1) gives the count
    return 0;
}


/* ===== TESTS ===== */

void test_basic(void) {
    printf("Test 1: Basic push/pop...\n");
    circular_buffer_t buf;
    cbuf_init(&buf);
    
    // Push 5 items
    for (uint8_t i = 0; i < 5; i++) {
        bool ok = cbuf_push(&buf, 'A' + i);
        assert(ok && "Push should succeed");
    }
    printf("  Pushed: A B C D E\n");
    printf("  Count: %u (expected 5)\n", cbuf_count(&buf));
    
    // Pop 3 items — should come out in FIFO order
    printf("  Popped: ");
    for (int i = 0; i < 3; i++) {
        uint8_t byte;
        bool ok = cbuf_pop(&buf, &byte);
        assert(ok && "Pop should succeed");
        printf("%c ", byte);
    }
    printf("(expected A B C)\n");
    printf("  Count: %u (expected 2)\n", cbuf_count(&buf));
    
    printf("  PASS ✓\n\n");
}

void test_full(void) {
    printf("Test 2: Buffer full...\n");
    circular_buffer_t buf;
    cbuf_init(&buf);
    
    // Push until full (can hold BUFFER_SIZE - 1 items)
    uint8_t pushed = 0;
    while (cbuf_push(&buf, pushed)) {
        pushed++;
    }
    printf("  Pushed %u items before full (expected %u)\n", 
           pushed, BUFFER_SIZE - 1);
    assert(pushed == BUFFER_SIZE - 1);
    assert(cbuf_is_full(&buf));
    
    // Try one more push — should fail
    bool overflow = cbuf_push(&buf, 99);
    assert(!overflow && "Push to full buffer should fail");
    printf("  Push to full buffer correctly returns false ✓\n");
    
    printf("  PASS ✓\n\n");
}

void test_empty(void) {
    printf("Test 3: Buffer empty...\n");
    circular_buffer_t buf;
    cbuf_init(&buf);
    
    assert(cbuf_is_empty(&buf));
    
    // Try pop from empty — should fail
    uint8_t byte;
    bool underflow = cbuf_pop(&buf, &byte);
    assert(!underflow && "Pop from empty buffer should fail");
    printf("  Pop from empty buffer correctly returns false ✓\n");
    
    printf("  PASS ✓\n\n");
}

void test_wrap_around(void) {
    printf("Test 4: Wrap-around...\n");
    circular_buffer_t buf;
    cbuf_init(&buf);
    
    // Fill → drain → fill again to test wrapping
    // Phase 1: Push 5, pop 5
    for (uint8_t i = 0; i < 5; i++) cbuf_push(&buf, i);
    for (int i = 0; i < 5; i++) { uint8_t b; cbuf_pop(&buf, &b); }
    
    // Now head and tail are at position 5 (past halfway in an 8-element buffer)
    
    // Phase 2: Push 7 (max capacity) — this wraps around the array
    for (uint8_t i = 10; i < 17; i++) {
        bool ok = cbuf_push(&buf, i);
        assert(ok);
    }
    
    // Pop all and verify order (FIFO)
    printf("  Popped after wrap: ");
    for (int i = 0; i < 7; i++) {
        uint8_t byte;
        cbuf_pop(&buf, &byte);
        printf("%u ", byte);
    }
    printf("(expected 10 11 12 13 14 15 16)\n");
    
    assert(cbuf_is_empty(&buf));
    printf("  PASS ✓\n\n");
}

int main(void) {
    printf("=== Circular Buffer Exercise ===\n\n");
    
    test_basic();
    test_full();
    test_empty();
    test_wrap_around();
    
    printf("ALL TESTS PASSED ✓\n");
    return 0;
}
