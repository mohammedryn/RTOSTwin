/*
 * ============================================================
 *  ADVANCED: STRUCTS, BIT-FIELDS & UNIONS — Mastery Edition
 *  File: 04_advanced_structs.c
 *  Path: C:\Users\Dell\Downloads\digitaltwin-main\04_advanced_structs.c
 * ============================================================
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>

/* ─── SECTION A: Bit-Fields (The Register Model) ─────────── */
typedef struct {
    uint8_t task_id    : 4;  /* Bits 0-3: ID 0 to 15 */
    uint8_t is_ready   : 1;  /* Bit 4:  Boolean */
    uint8_t is_blocked : 1;  /* Bit 5:  Boolean */
    uint8_t priority   : 2;  /* Bits 6-7: Priority 0-3 */
} task_control_t;

void demo_bit_fields(void) {
    printf("\n=== SECTION A: BIT-FIELDS ===\n");
    printf("sizeof(task_control_t) = %zu\n", sizeof(task_control_t));
    
    task_control_t tc;
    tc.task_id = 9;
    tc.is_ready = 1;
    tc.is_blocked = 0;
    tc.priority = 3;

    uint8_t raw = *(uint8_t*)&tc;
    printf("Raw byte in memory: 0x%02X\n", raw);
    printf("Bit layout: [Prio:%d] [Blocked:%d] [Ready:%d] [ID:%d]\n", 
           tc.priority, tc.is_blocked, tc.is_ready, tc.task_id);
}

/* ─── SECTION B: Unions (The Dual-Identity Model) ────────── */
typedef union {
    float    value_f;
    uint32_t value_u32;
    uint8_t  bytes[4];
} data_packet_t;

void demo_unions(void) {
    printf("\n=== SECTION B: UNIONS ===\n");
    data_packet_t pkt;
    pkt.value_f = 23.5f;
    
    printf("Float: %.2f | Hex: 0x%08X\n", pkt.value_f, pkt.value_u32);
    printf("Bytes: [0]=0x%02X, [1]=0x%02X, [2]=0x%02X, [3]=0x%02X\n", 
           pkt.bytes[0], pkt.bytes[1], pkt.bytes[2], pkt.bytes[3]);
}

/* ─── SECTION C: Nested Structs & Alignment ──────────────── */
typedef struct {
    uint8_t  a;
    uint16_t b;
} small_t; 

typedef struct {
    uint8_t  x;      
    small_t  nested; 
    uint32_t y;
} parent_t;

void demo_nested(void) {
    printf("\n=== SECTION C: NESTED ALIGNMENT ===\n");
    printf("sizeof(parent_t) = %zu\n", sizeof(parent_t));
    printf("Offsets: x=%zu, nested=%zu, y=%zu\n", 
           offsetof(parent_t, x), offsetof(parent_t, nested), offsetof(parent_t, y));
}

int main(void) {
    printf("==========================================\n");
    printf("  ADVANCED STRUCT MASTERY — DEMO\n");
    printf("==========================================\n");
    demo_bit_fields();
    demo_unions();
    demo_nested();
    return 0;
}
