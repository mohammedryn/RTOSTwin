/*
 * VNV — Week 1 Homework: vnv_bitwise_and_structs.c
 *
 * YOUR TOPICS: Bitwise operations, Structs, Padding, Preprocessor, CRC
 *
 * INSTRUCTIONS:
 *   gcc -Wall -Wextra -std=c99 -o vnv_hw1 vnv_bitwise_and_structs.c
 *   ./vnv_hw1
 *
 *   Complete all TODO sections. Be ready to explain your answers to RYN.
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>

/* ===== PART 1: print_binary ===== */
/*
 * TODO: Print a 32-bit value in binary with spaces every 4 bits
 * Example: print_binary(42) → "0000 0000 0000 0000 0000 0000 0010 1010"
 *
 * HINT: Loop from bit 31 down to 0.
 *       Use (value >> i) & 1 to extract each bit.
 *       Print a space after every 4 bits.
 */
void print_binary(uint32_t value) {
    // YOUR CODE HERE
    for (int i = 31; i >= 0; i--) {
        // TODO: print each bit
        // TODO: print space every 4 bits (when i % 4 == 0 and i != 0)
    }
}

/* ===== PART 2: Bit manipulation functions ===== */
void set_bit(uint32_t *reg, uint8_t bit) {
    // TODO: Set bit number 'bit' to 1
}

void clear_bit(uint32_t *reg, uint8_t bit) {
    // TODO: Clear bit number 'bit' to 0
}

void toggle_bit(uint32_t *reg, uint8_t bit) {
    // TODO: Flip bit number 'bit'
}

uint8_t check_bit(uint32_t reg, uint8_t bit) {
    // TODO: Return 1 if bit is set, 0 if not
    return 0;
}

/* ===== PART 3: CRC-16-CCITT ===== */
/*
 * TODO: Implement CRC-16-CCITT (polynomial 0x1021)
 * Initial value: 0xFFFF
 * For each byte: XOR into upper 8 bits of CRC, then process 8 bits
 */
uint16_t crc16_ccitt(const uint8_t *data, uint16_t length) {
    // YOUR CODE HERE
    return 0xFFFF;
}

/* ===== PART 4: Struct padding analysis ===== */

// BAD ORDER (lots of padding):
typedef struct {
    uint8_t  flags;       // 1 byte
    uint32_t timestamp;   // 4 bytes
    uint16_t value;       // 2 bytes
    uint8_t  type;        // 1 byte
    uint32_t sequence;    // 4 bytes
} bad_order_t;

// TODO: Define GOOD order (minimize padding)
typedef struct {
    // YOUR CODE HERE — reorder the same 5 fields to minimize sizeof
} good_order_t;

// PACKED variant (for wire protocol)
typedef struct __attribute__((packed)) {
    uint8_t  flags;
    uint32_t timestamp;
    uint16_t value;
    uint8_t  type;
    uint32_t sequence;
} packed_t;


/* ===== PART 5: RTOSTwin Structs ===== */

#define MAX_TASKS  10

// TODO: Define task_snapshot_t
// Fields: name[16], state (uint8), priority (uint8), 
//         stack_used (uint32), stack_total (uint32), cpu_time_us (uint32)
// Order for minimal padding!
typedef struct {
    // YOUR CODE HERE
} task_snapshot_t;

// TODO: Define memory_snapshot_t
// Fields: heap_free (uint32), heap_total (uint32), 
//         heap_min_ever_free (uint32), fragment_count (uint16)
typedef struct {
    // YOUR CODE HERE
} memory_snapshot_t;

// TODO: Define health_snapshot_t
// Fields: uptime_sec (uint32), error_count (uint16), 
//         temperature_C (int16), cpu_utilization (uint8)
typedef struct {
    // YOUR CODE HERE
} health_snapshot_t;

// TODO: Define full_snapshot_t
// Fields: timestamp_us (uint64), tasks[MAX_TASKS], 
//         memory, health, crc16 (uint16)
typedef struct {
    // YOUR CODE HERE
} full_snapshot_t;


/* ===== PART 6: Delta encoder bitmask ===== */

#define FIELD_TASKS       (1 << 0)
#define FIELD_MEMORY      (1 << 1)
#define FIELD_PERIPHERALS (1 << 2)
#define FIELD_HEALTH      (1 << 3)


int main(void) {
    printf("╔══════════════════════════════════════════╗\n");
    printf("║  VNV — Week 1: Bitwise, Structs, CRC     ║\n");
    printf("╚══════════════════════════════════════════╝\n\n");
    
    /* Part 1: print_binary test */
    printf("===== PART 1: print_binary =====\n");
    printf("42 = "); print_binary(42); printf("\n");
    printf("255 = "); print_binary(255); printf("\n");
    printf("0xDEAD = "); print_binary(0xDEAD); printf("\n\n");
    
    /* Part 2: Bit manipulation */
    printf("===== PART 2: Bit Operations =====\n");
    uint32_t reg = 0;
    printf("Start:          "); print_binary(reg); printf("\n");
    
    // TODO: Set bits 0 and 3, print after each
    // TODO: Check bit 2 (should be 0)
    // TODO: Check bit 3 (should be 1)
    // TODO: Clear bit 0, print
    // TODO: Toggle bit 7, print
    printf("\n");
    
    /* Part 3: CRC test */
    printf("===== PART 3: CRC-16 =====\n");
    const char *test = "Hello";
    uint16_t crc = crc16_ccitt((const uint8_t*)test, strlen(test));
    printf("CRC(\"Hello\") = 0x%04X\n", crc);
    printf("CRC(empty)   = 0x%04X (expect 0xFFFF)\n", crc16_ccitt(NULL, 0));
    printf("\n");
    
    /* Part 4: Struct padding */
    printf("===== PART 4: Struct Padding =====\n");
    printf("bad_order_t:  %zu bytes (fields = 12 bytes, rest is padding!)\n", sizeof(bad_order_t));
    printf("good_order_t: %zu bytes (should be <= bad_order_t)\n", sizeof(good_order_t));
    printf("packed_t:     %zu bytes (no padding, exact field sum)\n", sizeof(packed_t));
    printf("\n");
    
    /* Part 5: RTOSTwin structs */
    printf("===== PART 5: RTOSTwin Structs =====\n");
    printf("task_snapshot_t:   %3zu bytes (× %d = %zu)\n",
           sizeof(task_snapshot_t), MAX_TASKS, sizeof(task_snapshot_t) * MAX_TASKS);
    printf("memory_snapshot_t: %3zu bytes\n", sizeof(memory_snapshot_t));
    printf("health_snapshot_t: %3zu bytes\n", sizeof(health_snapshot_t));
    printf("full_snapshot_t:   %3zu bytes\n", sizeof(full_snapshot_t));
    printf("Budget: %s (need < 10240 bytes)\n",
           sizeof(full_snapshot_t) * 2 < 10240 ? "✓ PASS" : "✗ FAIL");
    printf("\n");
    
    /* Part 6: Delta encoder bitmask */
    printf("===== PART 6: Delta Bitmask =====\n");
    uint8_t changed = 0;
    
    // Simulate: tasks and health changed
    changed |= FIELD_TASKS;
    changed |= FIELD_HEALTH;
    
    printf("changed_fields = "); print_binary((uint32_t)changed); printf("\n");
    printf("TASKS changed:       %s\n", (changed & FIELD_TASKS) ? "YES" : "NO");
    printf("MEMORY changed:      %s\n", (changed & FIELD_MEMORY) ? "YES" : "NO");
    printf("PERIPHERALS changed: %s\n", (changed & FIELD_PERIPHERALS) ? "YES" : "NO");
    printf("HEALTH changed:      %s\n", (changed & FIELD_HEALTH) ? "YES" : "NO");
    
    printf("\n═══════════════════════════════════════\n");
    printf("Done! Review your answers and prepare to teach RYN.\n");
    return 0;
}
