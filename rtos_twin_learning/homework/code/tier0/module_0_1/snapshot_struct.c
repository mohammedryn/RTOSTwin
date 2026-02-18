/*
 * Module 0.1 — Assignment 5: snapshot_struct.c
 *
 * OBJECTIVE:
 *   Define the RTOSTwin data structures and verify memory layout.
 *   This is the EXACT code structure your telemetry agent will use.
 *
 * INSTRUCTIONS:
 *   gcc -Wall -Wextra -std=c99 -o snapshot_struct snapshot_struct.c
 *   ./snapshot_struct
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define MAX_TASKS  10

/* ===== TODO: Define these structures ===== */

/* Task Snapshot: State of one RTOS task
 * Fields needed:
 *   - name:       char[16]   — Human-readable task name
 *   - state:      uint8_t    — 0=Ready, 1=Running, 2=Blocked, 3=Suspended
 *   - priority:   uint8_t    — Task priority (0-255)
 *   - stack_used:  uint32_t  — Bytes of stack currently used
 *   - stack_total: uint32_t  — Total bytes allocated for stack
 *   - cpu_time_us: uint32_t  — Cumulative CPU time in microseconds
 *
 * CHALLENGE: Order the fields to minimize padding!
 */
typedef struct {
    // YOUR CODE HERE
} task_snapshot_t;


/* Memory Snapshot: Heap state
 * Fields:
 *   - heap_free:          uint32_t — Bytes free in heap
 *   - heap_total:         uint32_t — Total heap size
 *   - heap_min_ever_free: uint32_t — Lowest heap_free value ever recorded
 *   - fragment_count:     uint16_t — Number of free blocks (fragmentation indicator)
 */
typedef struct {
    // YOUR CODE HERE
} memory_snapshot_t;


/* Health Snapshot: System health metrics
 * Fields:
 *   - cpu_utilization:  uint8_t  — CPU usage percentage (0-100)
 *   - temperature_C:    int16_t  — Board temperature in (°C × 10) for 0.1° resolution
 *   - uptime_sec:       uint32_t — Seconds since boot
 *   - error_count:      uint16_t — Cumulative error counter
 */
typedef struct {
    // YOUR CODE HERE
} health_snapshot_t;


/* Full Snapshot: The complete system state at one instant
 * Fields:
 *   - timestamp_us: uint64_t              — Microseconds since boot
 *   - tasks:        task_snapshot_t[MAX_TASKS]  — All task states
 *   - memory:       memory_snapshot_t     — Heap state
 *   - health:       health_snapshot_t     — Health metrics
 *   - crc16:        uint16_t             — CRC over all preceding fields
 */
typedef struct {
    // YOUR CODE HERE
} full_snapshot_t;


/* ===== CRC Function (copy from your crc16.c Assignment 3) ===== */
uint16_t crc16_ccitt(const uint8_t *data, uint16_t length) {
    // TODO: Paste your working CRC function here
    // Or implement it fresh
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < length; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x8000)
                crc = (crc << 1) ^ 0x1021;
            else
                crc = crc << 1;
        }
    }
    return crc;
}


int main(void) {
    printf("=== RTOSTwin Snapshot Structure Exercise ===\n\n");
    
    /* Part 1: Print sizes of each struct */
    printf("--- Struct Sizes ---\n");
    printf("task_snapshot_t:    %3zu bytes (× %d tasks = %zu)\n",
           sizeof(task_snapshot_t), MAX_TASKS, sizeof(task_snapshot_t) * MAX_TASKS);
    printf("memory_snapshot_t:  %3zu bytes\n", sizeof(memory_snapshot_t));
    printf("health_snapshot_t:  %3zu bytes\n", sizeof(health_snapshot_t));
    printf("full_snapshot_t:    %3zu bytes\n", sizeof(full_snapshot_t));
    printf("\n");
    
    // TODO: Does full_snapshot_t fit in our 10 KB RAM budget? 
    //       Print: "Fits in budget: YES/NO"
    // if (sizeof(full_snapshot_t) <= 10240) { ... }
    
    /* Part 2: Create a dummy snapshot with realistic values */
    full_snapshot_t snapshot;
    memset(&snapshot, 0, sizeof(snapshot));  // IMPORTANT: zero padding bytes!
    
    snapshot.timestamp_us = 1234567890ULL;  // ~20 minutes after boot
    
    // TODO: Fill in at least 3 tasks with realistic values
    // Example for task 0:
    // strncpy(snapshot.tasks[0].name, "SensorTask", 15);
    // snapshot.tasks[0].state = 1;       // Running
    // snapshot.tasks[0].priority = 3;
    // snapshot.tasks[0].stack_used = 512;
    // snapshot.tasks[0].stack_total = 1024;
    // snapshot.tasks[0].cpu_time_us = 450000;
    
    // TODO: Fill in memory snapshot
    // snapshot.memory.heap_free = 80000;
    // snapshot.memory.heap_total = 131072;   // 128 KB
    // snapshot.memory.heap_min_ever_free = 75000;
    // snapshot.memory.fragment_count = 5;
    
    // TODO: Fill in health snapshot
    // snapshot.health.cpu_utilization = 35;    // 35%
    // snapshot.health.temperature_C = 237;     // 23.7°C
    // snapshot.health.uptime_sec = 1234;
    // snapshot.health.error_count = 0;
    
    /* Part 3: Calculate and store CRC */
    // The CRC should cover everything EXCEPT the crc16 field itself
    // snapshot.crc16 = crc16_ccitt(
    //     (const uint8_t *)&snapshot,
    //     sizeof(full_snapshot_t) - sizeof(uint16_t)
    // );
    // printf("CRC-16: 0x%04X\n", snapshot.crc16);
    
    /* Part 4: Verify CRC */
    // uint16_t verify = crc16_ccitt(
    //     (const uint8_t *)&snapshot,
    //     sizeof(full_snapshot_t) - sizeof(uint16_t)
    // );
    // printf("CRC verify: %s\n", (verify == snapshot.crc16) ? "PASS ✓" : "FAIL ✗");
    
    /* Part 5: Print the snapshot contents */
    printf("\n--- Snapshot Contents ---\n");
    printf("Timestamp: %llu us\n", (unsigned long long)snapshot.timestamp_us);
    // TODO: Print each task's details
    // TODO: Print memory details
    // TODO: Print health details
    
    return 0;
}
