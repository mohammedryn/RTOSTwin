/*
 * Module 0.1 — Assignment 2: bitwise_exercise.c
 *
 * OBJECTIVE:
 *   Master bitwise operations — the language of hardware registers.
 *
 * INSTRUCTIONS:
 *   gcc -Wall -Wextra -std=c99 -o bitwise_exercise bitwise_exercise.c
 *   ./bitwise_exercise
 */

#include <stdio.h>
#include <stdint.h>

/* TODO: Implement this function
 * Print a 32-bit value in binary with spaces every 4 bits
 * Example output: "0000 0000 0000 0000 0000 0000 0010 1010"
 * 
 * HINT: Loop from bit 31 down to 0. 
 *       Use (value >> i) & 1 to extract each bit.
 *       Insert a space every 4 bits.
 */
void print_binary(uint32_t value) {
    // YOUR CODE HERE
}

/* TODO: Implement these four functions */

void set_bit(uint32_t *reg, uint8_t bit) {
    // YOUR CODE HERE
    // Set bit number 'bit' to 1
}

void clear_bit(uint32_t *reg, uint8_t bit) {
    // YOUR CODE HERE
    // Clear bit number 'bit' to 0
}

void toggle_bit(uint32_t *reg, uint8_t bit) {
    // YOUR CODE HERE
    // Flip bit number 'bit'
}

uint8_t check_bit(uint32_t reg, uint8_t bit) {
    // YOUR CODE HERE
    // Return 1 if bit is set, 0 if not
    return 0;
}


int main(void) {
    printf("=== Bitwise Operations Exercise ===\n\n");
    
    // Part 1: Test print_binary
    printf("Value 42 in binary: ");
    print_binary(42);
    printf("\n");
    // Expected: 0000 0000 0000 0000 0000 0000 0010 1010
    
    printf("Value 0xDEAD in binary: ");
    print_binary(0xDEAD);
    printf("\n\n");
    
    // Part 2: Test set/clear/toggle/check
    uint32_t reg = 0;
    printf("Initial register: ");
    print_binary(reg);
    printf("\n");
    
    // TODO: Set bits 0 and 3
    // set_bit(&reg, 0);
    // set_bit(&reg, 3);
    // printf("After setting bits 0,3: ");
    // print_binary(reg);
    // printf("\n");
    
    // TODO: Check if bit 2 is set (should be 0)
    // printf("Bit 2 is: %u\n", check_bit(reg, 2));
    
    // TODO: Check if bit 3 is set (should be 1)
    // printf("Bit 3 is: %u\n", check_bit(reg, 3));
    
    // TODO: Clear bit 0
    // clear_bit(&reg, 0);
    // printf("After clearing bit 0: ");
    // print_binary(reg);
    // printf("\n");
    
    // TODO: Toggle bit 7
    // toggle_bit(&reg, 7);
    // printf("After toggling bit 7: ");
    // print_binary(reg);
    // printf("\n");
    
    // Part 3: Delta encoder bitmask simulation
    printf("\n=== Delta Encoder Bitmask ===\n\n");
    
    // TODO: Simulate the RTOSTwin delta encoder changed_fields
    // Define: FIELD_TASKS = bit 0, FIELD_MEMORY = bit 1,
    //         FIELD_PERIPHERALS = bit 2, FIELD_HEALTH = bit 3
    // 
    // Set FIELD_TASKS and FIELD_HEALTH as "changed"
    // Print the bitmask in binary
    // Check each field and print which ones changed
    
    return 0;
}
