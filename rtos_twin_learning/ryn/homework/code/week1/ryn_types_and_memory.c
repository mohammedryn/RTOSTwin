/*
 * RYN — Week 1 Homework: ryn_types_and_memory.c
 *
 * YOUR TOPICS: Types, Memory Layout, Pointers, volatile, static allocation
 *
 * INSTRUCTIONS:
 *   gcc -Wall -Wextra -std=c99 -o ryn_hw1 ryn_types_and_memory.c
 *   ./ryn_hw1
 *
 *   Complete all TODO sections. Be ready to explain your answers to VNV.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* ===== PART 1: sizeof exploration ===== */
void part1_sizeof(void) {
    printf("===== PART 1: sizeof =====\n\n");
    
    // TODO: Print sizeof for ALL stdint types
    printf("uint8_t:  %zu bytes\n", sizeof(uint8_t));
    // TODO: uint16_t, uint32_t, uint64_t
    // TODO: int8_t, int16_t, int32_t, int64_t
    
    // TODO: Print sizeof for non-portable types and explain WHY they're dangerous
    printf("\nNon-portable types:\n");
    printf("int:   %zu bytes (could be 2 on MSP430!)\n", sizeof(int));
    // TODO: short, long, long long, void*
    
    printf("\n");
}

/* ===== PART 2: Overflow ===== */
void part2_overflow(void) {
    printf("===== PART 2: Overflow =====\n\n");
    
    // TODO: Demonstrate unsigned overflow
    // Store 300 in uint8_t → what value?
    // Store 70000 in uint16_t → what value?
    // Show the modulo math in comments
    
    // TODO: Demonstrate signed overflow (undefined behavior on signed types!)
    // Store 200 in int8_t → what value? (hint: two's complement)
    // Store -1 in uint8_t → what value?
    
    // TODO: Demonstrate the USEFUL overflow for timestamps
    // uint16_t start = 65530;
    // uint16_t end = 5;    // After wrap
    // uint16_t elapsed = end - start;
    // printf("Elapsed: %u (should be 11)\n", elapsed);
    
    printf("\n");
}

/* ===== PART 3: Signed/Unsigned Comparison Trap ===== */
void part3_comparison_trap(void) {
    printf("===== PART 3: Signed vs Unsigned =====\n\n");
    
    // TODO: Show the trap
    // int8_t  a = -1;
    // uint8_t b = 1;
    // Compare them and explain WHY -1 appears greater than 1
    
    // TODO: Show how to fix it (cast to same type before comparing)
    
    printf("\n");
}

/* ===== PART 4: Memory Map Exploration ===== */

// A global variable (lives in .data or .bss)
static uint32_t global_var = 42;
static uint32_t global_zero;        // .bss (uninitialized)

void part4_memory_map(void) {
    printf("===== PART 4: Memory Addresses =====\n\n");
    
    // Stack variable
    uint32_t stack_var = 100;
    
    // Static local (lives in .bss, NOT on stack)
    static uint32_t static_local = 200;
    
    // Heap variable
    uint32_t *heap_var = (uint32_t *)malloc(sizeof(uint32_t));
    if (heap_var) *heap_var = 300;
    
    // TODO: Print the ADDRESS of each variable
    // Use %p for pointers
    printf("global_var  (.data):  %p\n", (void*)&global_var);
    printf("global_zero (.bss):   %p\n", (void*)&global_zero);
    printf("static_local (.bss):  %p\n", (void*)&static_local);
    printf("stack_var   (stack):  %p\n", (void*)&stack_var);
    printf("heap_var    (heap):   %p\n", (void*)heap_var);
    
    // TODO: Observe which addresses are close together
    // .data and .bss should be near each other
    // Stack should be in a different region
    // Heap should be in yet another region
    
    // TODO: Write a comment explaining what you observe about the address layout
    // Does it match the theoretical memory map (stack at top, heap below, globals at bottom)?
    
    free(heap_var);
    printf("\n");
}

/* ===== PART 5: volatile demonstration ===== */
void part5_volatile(void) {
    printf("===== PART 5: volatile =====\n\n");
    
    // TODO: Explain in a comment:
    // 1. What does volatile tell the compiler?
    // 2. When is it mandatory in embedded? (list 4 cases)
    // 3. What happens if you FORGET volatile on a hardware register read in a loop?
    
    // NOTE: On PC, volatile doesn't have visible effects because
    // there are no hardware registers. But on STM32:
    //
    // volatile uint32_t *GPIOA_IDR = (volatile uint32_t *)0x40020010;
    // while (!(*GPIOA_IDR & (1 << 3))) {
    //     // Wait for button press
    //     // WITHOUT volatile: compiler reads once, caches, infinite loop!
    //     // WITH volatile: compiler re-reads register every iteration ✓
    // }
    
    printf("(See comments in source code for volatile explanation)\n\n");
}

/* ===== PART 6: static keyword — 3 meanings ===== */
static uint32_t file_private_counter = 0;  // Meaning 1: file scope

void count_calls(void) {
    static uint32_t call_count = 0;  // Meaning 2: persistent local
    call_count++;
    printf("  count_calls() called %u times\n", call_count);
}

void part6_static(void) {
    printf("===== PART 6: static keyword =====\n\n");
    
    // Meaning 2: persistent local
    printf("Persistent local (call_count survives between calls):\n");
    count_calls();  // prints 1
    count_calls();  // prints 2
    count_calls();  // prints 3
    
    // TODO: Explain Meaning 3 in a comment:
    // Why does snapshot_capture() use:
    //   static TaskStatus_t task_buf[MAX_TASKS];
    // instead of:
    //   TaskStatus_t task_buf[MAX_TASKS];    (stack allocation)
    //   TaskStatus_t *task_buf = malloc(...); (heap allocation)
    
    printf("\n");
}


int main(void) {
    printf("╔══════════════════════════════════════╗\n");
    printf("║  RYN — Week 1: Types & Memory        ║\n");
    printf("╚══════════════════════════════════════╝\n\n");
    
    part1_sizeof();
    part2_overflow();
    part3_comparison_trap();
    part4_memory_map();
    part5_volatile();
    part6_static();
    
    printf("═══════════════════════════════════════\n");
    printf("Done! Review your answers and prepare to teach VNV.\n");
    return 0;
}
