/*
 * Module 0.1 — Assignment 1: types_exercise.c
 * 
 * OBJECTIVE:
 *   Understand stdint.h types, sizeof, overflow, and signed/unsigned traps.
 *
 * INSTRUCTIONS:
 *   Compile and run on your PC:
 *     gcc -Wall -Wextra -std=c99 -o types_exercise types_exercise.c
 *     ./types_exercise
 *
 *   Fill in the TODO sections. Do NOT look at the answers until you've tried.
 *
 * EXPECTED LEARNING:
 *   - Why uint32_t is safer than int
 *   - What happens when you overflow an unsigned type
 *   - The signed/unsigned comparison trap
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

int main(void) {
    printf("=== Part 1: sizeof() for each stdint type ===\n\n");
    
    // TODO: Declare one variable of each type and print sizeof()
    // Example:
    uint8_t  var_u8  = 0;
    int8_t   var_i8  = 0;
    // uint16_t var_u16 = ...
    // int16_t  var_i16 = ...
    // uint32_t var_u32 = ...
    // int32_t  var_i32 = ...
    // uint64_t var_u64 = ...
    // int64_t  var_i64 = ...
    
    printf("uint8_t:  %zu bytes\n", sizeof(var_u8));
    printf("int8_t:   %zu bytes\n", sizeof(var_i8));
    // TODO: Print sizeof for the rest
    
    printf("\n\n=== Part 2: Overflow Demonstration ===\n\n");
    
    // TODO: Store 300 in a uint8_t. What value gets printed?
    // uint8_t overflow_test = 300;
    // printf("300 stored in uint8_t = %u\n", overflow_test);
    // EXPLAIN: Why is it this value? (Show the math: 300 mod 256 = ?)
    
    // TODO: Store -1 in a uint8_t. What value gets printed?
    // uint8_t neg_test = -1;
    // printf("-1 stored in uint8_t = %u\n", neg_test);
    // EXPLAIN: Why is it 255? (Hint: two's complement)
    
    printf("\n\n=== Part 3: Signed/Unsigned Comparison Trap ===\n\n");
    
    // TODO: Compare (int8_t)-1 with (uint8_t)1
    // int8_t  signed_val   = -1;
    // uint8_t unsigned_val = 1;
    //
    // if (signed_val < unsigned_val) {
    //     printf("-1 < 1 (correct!)\n");
    // } else {
    //     printf("-1 >= 1 (TRAP! signed promoted to unsigned)\n");
    // }
    //
    // EXPLAIN: Why does this happen? What does -1 become when converted
    //          to unsigned? Use the formula: (uint8_t)(-1) = 256 + (-1) = 255
    
    printf("\n\n=== Part 4: Platform Danger ===\n\n");
    
    // Print sizeof for the non-portable types
    printf("sizeof(char)   = %zu\n", sizeof(char));
    printf("sizeof(short)  = %zu\n", sizeof(short));
    printf("sizeof(int)    = %zu\n", sizeof(int));     // 2 or 4???
    printf("sizeof(long)   = %zu\n", sizeof(long));    // 4 or 8???
    printf("sizeof(void*)  = %zu\n", sizeof(void*));   // 4 or 8???
    
    // TODO: Write a comment explaining why using 'int sensor_value;' 
    //       is dangerous on an MSP430 (16-bit int) vs STM32 (32-bit int).
    
    return 0;
}
