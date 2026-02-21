# Week 1, Day 3 — MMIO, `volatile` & The 3 Faces of `static`
## RYN's Engineering Coach Notes

**Topic:** Memory-Mapped I/O, Compiler Optimization vs. Hardware Reality, Static Encapsulation  
**Platform:** STM32F407 (ARM Cortex-M4, 168 MHz, 192 KB SRAM, 1 MB Flash)  
**Project Connection:** Every hardware register access in RTOSTwin, telemetry flag safety, `snapshot_capture()` static internals

---

> [!IMPORTANT]
> **Today is the day you stop treating `volatile` as a mysterious keyword you add when things break.** After today, you will know exactly what the optimizer does, why it does it, what `volatile` contracts prevent it from doing, and how to verify your contract is in place by reading the generated assembly. This is senior firmware engineer territory and most developers never reach it.

---

# 1️⃣ Motivation & System Context

## The Silent Bug That Ships to Production

This exact pattern has caused firmware failures in industrial PLCs, medical devices, and automotive ECUs — systems where "it worked in debug mode" is not an acceptable answer:

```c
// main.c compiled with -O2 optimization:
uint8_t uart_byte_received = 0;  // Written by ISR, read by main

void uart_process_task(void) {
    while (!uart_byte_received) { }   // Waiting for ISR to set this flag
    process(received_byte);
}

// UART ISR (fires every time a byte arrives):
void USART2_IRQHandler(void) {
    received_byte = USART2->DR;
    uart_byte_received = 1;
}
```

In **debug mode (-O0):** Works perfectly. Every access is a real memory read. Developer tests it. Ships it.

In **release mode (-O2):** `uart_process_task()` **infinite loops and never processes any byte.** The optimizer looked at `uart_byte_received`, saw it was `0` at function entry and is never written ANYWHERE inside `uart_process_task()`, concluded it is always `0`, and replaced the `while (!uart_byte_received)` with `while (1)`. It discarded the condition entirely.

**The fix is one word: `volatile`. The cost of NOT knowing this is shipping broken firmware to customers.**

## RTOSTwin Hardware Register Context

Every time `snapshot_capture()` reads the UART status register or DWT cycle counter, it relies on this exact contract:

```c
// WITHOUT volatile on DWT->CYCCNT (from ARM CMSIS headers):
uint32_t start = DWT->CYCCNT;    // Optimizer: "I just read this, it can't have changed"
snapshot_capture(&current);      // Does not write CYCCNT
uint32_t end = DWT->CYCCNT;      // Optimizer: "Same read? I'll reuse start's value!"
uint32_t cycles = end - start;   // ALWAYS 0 — completely useless timing measurement

// WITH volatile (CMSIS declares DWT->CYCCNT as volatile uint32_t):
// Every read of DWT->CYCCNT is a real LDR instruction hitting the AHB bus
// The hardware updates CYCCNT every clock cycle — two reads ARE different
```

This is real. If ARM CMSIS didn't declare CYCCNT as `volatile uint32_t`, our profiling tool would return zero on every measurement.

---

# 2️⃣ Foundational Theory — First Principles

## 2.1 What a Compiler Optimizer Actually Does

The optimizer's job is to produce the **fastest, smallest** machine code that has the **same observable behavior** as your C source.

The key insight: The optimizer defines "observable behavior" as what a **single, uninterrupted thread** would see. It cannot know about:
- Hardware registers that change on their own (timer counts, UART data registers)
- Variables written by interrupt service routines executing in another "thread of control"
- Variables written by a DMA controller, which writes RAM without executing CPU instructions
- Variables shared between FreeRTOS tasks (the scheduler is not visible to the optimizer)

**The optimizer's three transformations that kill embedded code:**

### Transformation 1: Dead-Store Elimination

The optimizer removes writes to variables that are never subsequently read:

```c
void configure_uart(void) {
    uint32_t temp = USART2->CR1;    // Read CR1
    temp |= (1 << 13);              // Set UE bit
    USART2->CR1 = temp;             // Write CR1
    // Optimizer: "temp is written here and then the function returns.
    //             temp is never READ after this write. This is a 'dead store'.
    //             I will eliminate the write."
    // Result: UART is NEVER enabled. Peripheral stays off.
}
```

With `volatile uint32_t CR1` in the GPIO_TypeDef struct:
- The read of `USART2->CR1` is **observable** (reads from hardware)
- The write of `USART2->CR1` is **observable** (writes to hardware)
- Neither can be eliminated

### Transformation 2: Loop Invariant Code Motion (LICM)

The optimizer moves reads outside of loops if it determines the value doesn't change inside the loop:

```c
while (!(USART2->SR & (1 << 7))) { }  // Wait for TXE flag

// Optimizer analysis (without volatile):
// - USART2->SR is read on every iteration
// - Nothing INSIDE the loop writes to USART2->SR
// - Therefore: the value of USART2->SR is "loop invariant"
// - Move the read OUTSIDE the loop:

uint32_t sr_cached = USART2->SR;    // Read once before loop
while (!(sr_cached & (1 << 7))) { } // Loop forever using cached value!
```

Generated assembly (broken, without volatile):
```asm
LDR  R0, [R1, #0x08]   ; Read SR once into R0 (R1 = USART2 base address)
.loop:
  TST  R0, #0x80        ; Test bit 7 of R0 (THE CACHED VALUE)
  BEQ  .loop            ; Branch back if not set → INFINITE LOOP
```

Generated assembly (correct, with volatile):
```asm
.loop:
  LDR  R0, [R1, #0x08]  ; Read SR from memory EVERY iteration (LDR inside loop)
  TST  R0, #0x80        ; Test bit 7 of fresh value
  BEQ  .loop            ; Branch back if not set
  ; Eventually: hardware sets SR[7] → loop exits
```

### Transformation 3: Redundant Load/Store Elimination

The optimizer assumes if it wrote a value to a location, it can cache that write; if it read a value, it can reuse it:

```c
GPIOA->ODR  = 0x20;    // Write 1: Set bit 5 HIGH
// ... some code ...
GPIOA->ODR  = 0x00;    // Write 2: Set all bits LOW
// Optimizer: "Write 1 is immediately overwritten. It has no effect. Remove Write 1."
// Result: GPIO never goes HIGH — only Write 2 remains.
```

With `volatile`, **every write is a real side-effect** and cannot be merged or removed.

## 2.2 The `volatile` Type Qualifier — Formal Contract

`volatile` is a **type qualifier** (like `const`). It is part of the type of the variable:
- `volatile uint32_t x` — x is a volatile uint32_t
- `volatile uint32_t *p` — p is a pointer to a volatile uint32_t (the THING POINTED TO is volatile)
- `volatile uint32_t * volatile p` — BOTH the pointer itself AND what it points to are volatile

The contract `volatile` creates with the compiler:

| Guarantee | What It Means in Practice |
|-----------|--------------------------|
| **Every read is a real memory access** | No cached reads across reads. `LDR` emitted every time. |
| **Every write is a real memory access** | No dead-store elimination. `STR` emitted every time. |
| **No reordering relative to other volatile accesses** | Two consecutive volatile reads happen in source order |
| **No loop hoisting** | The read stays inside the loop body, not moved outside |

**What `volatile` does NOT guarantee:**

| Assumption | Why It's Wrong |
|-----------|---------------|
| Atomicity | `volatile uint64_t x; x = 0xDEADBEEF12345678;` is TWO 32-bit stores on ARM — NOT atomic |
| Memory ordering relative to non-volatile | Non-volatile accesses can be reordered around a volatile access |
| Thread safety | `volatile` is not a mutex. Two tasks reading/writing the same volatile variable have a race condition |
| ISR safety for compound operations | `x++` on a volatile variable is still read-modify-write: NOT atomic |

## 2.3 The 4 Mandatory `volatile` Rules — Derivation from First Principles

### Rule 1: Hardware Peripheral Registers

A peripheral register at address `0x40004414` is a flip-flop inside a silicon circuit. The hardware state machine that controls UART, GPIO, ADC, etc. modifies its own registers based on electrical events (bytes arriving, conversions completing, timer overflows). The CPU is not told when this happens.

```c
// CORRECT: The CMSIS typedef declares all fields volatile:
typedef struct {
    volatile uint32_t SR;    // Status Register — hardware modifies bits as data arrives
    volatile uint32_t DR;    // Data Register — hardware loads received byte here
    volatile uint32_t BRR;   // Baud Rate — ONLY modified by CPU, but still volatile
    volatile uint32_t CR1;   // Control Register 1
    volatile uint32_t CR2;   // Control Register 2
    volatile uint32_t CR3;   // Control Register 3
    volatile uint32_t GTPR;  // Guard Time and Prescaler
} USART_TypeDef;
```

Even CR1 and CR2 (control registers, only written by you) should be `volatile` — because the hardware uses these to drive the UART state machine. If you write them twice in sequence, both writes must reach the hardware.

### Rule 2: ISR-Shared Flags (Single-Writer / Single-Reader)

```
Timeline:
CPU running main():        ...polls uart_flag... (reads 0) ...polls... (reads 0)...
                                                          ↑
UART IRQ fires:           CPU switches to ISR → uart_flag = 1 → ISR returns
                                                          ↓
CPU returns to main():    ...polls uart_flag... (reads ???)
```

If `uart_flag` is not `volatile`:
- The compiler reads it once at the top of the function or the first iteration of the loop
- Subsequent "reads" just reuse the CPU register (`R0` still contains the original 0)
- The ISR writes to the memory ADDRESS of `uart_flag`, but `main()` never re-reads that address
- **`main()` sees stale data forever**

```c
volatile uint8_t uart_byte_received = 0;    // volatile REQUIRED

// ISR sets it:
void USART2_IRQHandler(void) {
    received_byte = USART2->DR;    // Read data register  
    uart_byte_received = 1;        // Signal main that a byte arrived
    // 'uart_byte_received = 1' is a volatile write → STR to memory → main will see it
}

// Main reads it:
while (!uart_byte_received) { }   // volatile read → LDR every iteration → sees the ISR's write
```

### Rule 3: Variables Shared Between FreeRTOS Tasks

FreeRTOS tasks are like threads — each has its own stack and runs independently. But unlike hardware ISRs, the optimizer's concern is different here:

```c
// Task A (producer, priority 1):
uint32_t shared_counter = 0;  // No volatile
void task_a(void *p) {
    while (1) {
        shared_counter++;   // Optimizer may keep shared_counter in a register
        vTaskDelay(10);
    }
}

// Task B (consumer, priority 2):
void task_b(void *p) {
    while (1) {
        if (shared_counter > 100) { ... }  // Reads memory — sees stale value
        vTaskDelay(5);
    }
}
```

The optimizer may keep `shared_counter` in a CPU register (R4) across multiple iterations of Task A's loop. When the scheduler switches to Task B, R4 is saved to the TCB — but `shared_counter`'s RAM address may not have been updated since Task A started. Task B's read of the RAM address shows an old value.

`volatile` forces the write to ACTUALLY reach RAM, and the read to ACTUALLY come from RAM.

> [!NOTE]
> **For simple flag patterns in RTOS, `volatile` is sufficient.** For compound operations (increment, check-and-set, swap), use a FreeRTOS mutex, critical section, or atomic operation. `volatile` alone is NOT sufficient there.

### Rule 4: DMA Destination Buffers

DMA (Direct Memory Access) is a hardware peripheral that copies data between two addresses without involving the CPU. When the UART receives bytes and DMA stores them in your buffer:

```c
uint8_t rx_buffer[256];         // DMA writes here while CPU is sleeping (vTaskDelay)

// DMA is configured to write received bytes to rx_buffer[]
// ...time passes, DMA writes 100 bytes...

// Main task wakes up:
if (rx_buffer[0] == 0xAA) {    // Is this a real memory read or a cached value?
    // WITHOUT volatile: compiler may cache rx_buffer[0] from before DMA wrote it
    // The CPU never wrote to rx_buffer during the delay — optimizer thinks it's unchanged
}
```

DMA writes RAM **without executing CPU instructions and without modifying any CPU registers**. The optimizer has zero visibility into what DMA wrote.

```c
volatile uint8_t rx_buffer[256];   // CORRECT: every access is a real memory read
```

**Performance consideration:** `volatile uint8_t rx_buffer[256]` means accessing ANY element requires an actual memory load. For a buffer you process in a loop, this can be slower than copying it first:

```c
// Performance-critical pattern for volatile DMA buffers:
volatile uint8_t rx_buffer[256];  // DMA destination: volatile

void process_received_packet(void) {
    uint8_t local_copy[256];
    // Single volatile read: copy all bytes to a local (non-volatile) buffer
    memcpy(local_copy, (uint8_t*)rx_buffer, 256);  // Cast away volatile for memcpy
    // Now process local_copy — optimizer can use registers freely
    parse_packet(local_copy, 256);
}
```

## 2.4 The 3 Faces of `static` — Formal Definitions

`static` is the most overloaded keyword in C. It means THREE different things depending on where it appears.

### Face 1: Storage Duration (Inside a Function)

```c
uint32_t call_counter(void) {
    static uint32_t count = 0;  // FACE 1: persistent storage between calls
    return ++count;
}
```

**What it does:**
- The variable `count` is allocated in `.bss` (NOT on the stack)
- It is initialized ONCE (at startup, to 0 by the `.bss` zeroing)
- It PERSISTS between calls — its value survives when the function returns
- It is NOT reinitialized on subsequent calls (the `= 0` runs only at program start)

**Why the initialization happens once and only once:**
```c
static uint32_t count = 0;
// At compile time, the compiler:
// 1. Places 'count' in .bss at some address (e.g., 0x200001A4)
// 2. Records that .bss should be zeroed at startup (which covers count = 0)
// The '= 0' is NOT a runtime assignment instruction!
// It is metadata for the linker to know the initial value
// There is NO 'STR #0, [count]' instruction generated at the call site
```

If you want to "reset" a static local for testing purposes, you must write to it explicitly:
```c
count = 0;  // Explicit reset — this IS a runtime instruction (STR)
```

**RTOSTwin use case:**
```c
void snapshot_capture(full_snapshot_t *out) {
    static uint32_t call_count = 0;          // Persistent call counter
    static uint64_t total_cycles_used = 0;   // Accumulated overhead tracking
    static full_snapshot_t prev;             // Previous snapshot for delta

    call_count++;

    uint32_t start = DWT->CYCCNT;
    // ... do work ...
    uint32_t elapsed = DWT->CYCCNT - start;

    total_cycles_used += elapsed;

    // Report efficiency: average overhead per call
    if (call_count % 100 == 0) {
        uint32_t avg_cycles = (uint32_t)(total_cycles_used / call_count);
        printf("Avg snapshot overhead: %lu cycles = %lu us\n",
               avg_cycles, avg_cycles / 168);
    }
}
```

### Face 2: File-Level Linkage (At File Scope)

```c
// encoder.c

static uint32_t keyframe_counter = 0;    // FACE 2: private to encoder.c
static uint8_t  prev_changed_fields = 0; // Also private to encoder.c

// Public interface: accessible from other files (note: NO static):
int32_t encode_delta(const full_snapshot_t *curr,
                     const full_snapshot_t *prev,
                     uint8_t *out, uint16_t *out_len) {
    keyframe_counter++;  // OK — in same file
    // ...
}

// Private helper — inaccessible from other files:
static uint8_t detect_task_changes(const full_snapshot_t *curr,
                                    const full_snapshot_t *prev) {
    return memcmp(&curr->tasks, &prev->tasks, sizeof(curr->tasks)) != 0;
}
```

If another file (`transport.c`) tries to call `detect_task_changes()`, the linker produces:
```
transport.c: error: undefined reference to 'detect_task_changes'
```

Even though the function EXISTS in `encoder.c`, it is **not visible outside that file**.

**The software engineering value of Face 2:**
- Prevents namespace pollution (no collision if two files both have `helper_fn()`)
- Enforces encapsulation — the module's internal state is protected
- Makes code analysis easier — a `static` function can only be called from within its file
- In RTOSTwin: `snapshot.c` should expose ONLY `snapshot_init()` and `snapshot_capture()` — all internal functions (`fill_task_data`, `compute_health`, etc.) should be `static`

### Face 3: Compile-Time Allocated Buffer (Inside a Function)

```c
void snapshot_capture(full_snapshot_t *out) {
    static TaskStatus_t task_buf[MAX_TASKS];  // FACE 3: compile-time allocated

    // task_buf is:
    //   - In .bss (not on the stack — 0 stack cost)
    //   - Always at the SAME address (0x20XXXXXX — fixed at link time)
    //   - Initialized to zero at startup (by .bss zeroing)
    //   - Shared across all calls (if called reentrantly, BOTH calls use the SAME task_buf!)
    
    UBaseType_t count = uxTaskGetSystemState(task_buf, MAX_TASKS, NULL);
    // ...
}
```

**The reentrance WARNING:** A `static` local buffer is shared across all calls. If `snapshot_capture()` is ever called from two different tasks simultaneously (or from a task and an ISR), both calls would write to the same `task_buf` simultaneously → **data corruption**.

In RTOSTwin, this is safe ONLY because:
1. `snapshot_capture()` is called from exactly ONE task (TelemetryTask)
2. We use a critical section during the call to prevent preemption

If you ever call it from two contexts: EITHER wrap the function body in a mutex, OR change to a stack-allocated buffer (if the stack can afford it).

## 2.5 Combining `volatile` + `static` + `const` — The Full Vocabulary

```c
// All combinations explained with RTOSTwin examples:

static volatile uint8_t flag;
// - static:   .bss, persistent, file-private
// - volatile: every read/write is a real memory access
// → Common pattern for ISR-shared flags (if declared at file scope)

static const uint8_t SYNC_BYTES[2] = {0xAA, 0x55};
// - static:   file-private (only used in transport.c)
// - const:    stored in .rodata (Flash), cannot be modified at runtime
// → Packet sync bytes

volatile const uint32_t * const DWT_CYCCNT = (volatile uint32_t *)0xE0001004;
// - volatile: hardware updates this register — re-read every access
// - const:    the pointer itself cannot change (always points to 0xE0001004)
// → Direct hardware register pointer

static const uint8_t CRC_LUT[256] = { /* precomputed */ };
// - static:   private to crc.c
// - const:    in .rodata (Flash)
// → CRC lookup table: 256 bytes in Flash, zero RAM
```

---

# 3️⃣ Deep Dive — Engineering Depth

## 3.1 Reading the Assembly to Verify `volatile` Contracts

The only way to be 100% certain `volatile` is doing its job is to inspect the generated assembly.

**Test case:**
```c
#include <stdint.h>

volatile uint32_t *UART_SR = (volatile uint32_t*)0x40004400;

void wait_for_txe(void) {
    while (!(*UART_SR & (1 << 7))) { }
}
```

Compile: `arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -O2 -S -o test.s test.c`

**Expected assembly (CORRECT — volatile works):**
```asm
wait_for_txe:
    LDR  R3, =0x40004400      ; Load address of UART_SR into R3
    LDR  R3, [R3]             ; Dereference: R3 = UART_SR pointer value (the address)
.L1:
    LDR  R2, [R3]             ; ← THIS LINE — inside the loop! Re-reads SR every iteration
    TST  R2, #128             ; Test bit 7
    BEQ  .L1                  ; Loop if not set
    BX   LR                   ; Return
```

If you see `LDR` INSIDE the `.L1` loop: ✅ `volatile` is working.

**If you see (BROKEN — volatile missing):**
```asm
wait_for_txe:
    LDR  R3, =0x40004400
    LDR  R3, [R3]
    LDR  R2, [R3]              ; ← This is OUTSIDE the loop! Reads once.
.L1:
    TST  R2, #128              ; Tests the same R2 forever
    BEQ  .L1                   ; Infinite loop
    BX   LR
```

**Commands to inspect assembly:**
```bash
# Generate assembly listing:
arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -O2 -S -o output.s source.c

# Disassemble compiled binary:
arm-none-eabi-objdump -d firmware.elf | grep -A 20 "wait_for_txe"
```

## 3.2 The ISR-Shared Flag Pattern — Full Production Implementation

### Pattern 1: Flag (Simple, Polling — Wastes CPU)
```c
// telemetry.h (public interface):
extern volatile uint8_t g_snapshot_request;

// main.c or timer ISR:
void TIM2_IRQHandler(void) {
    if (TIM2->SR & TIM_SR_UIF) {      // Timer update event
        TIM2->SR &= ~TIM_SR_UIF;      // Clear the flag
        g_snapshot_request = 1;        // Signal telemetry task
    }
}

// telemetry.c (telemetry task):
volatile uint8_t g_snapshot_request = 0;  // Definition

void telemetry_task(void *params) {
    while (1) {
        // CPU is BUSY-WAITING here — wastes power and CPU cycles:
        while (!g_snapshot_request) { }
        g_snapshot_request = 0;        // Must clear BEFORE processing (not after!)
        snapshot_capture(&current_snap);
        transmit_packet(&current_snap);
    }
}
```

**Problem:** `while (!g_snapshot_request)` consumes 100% CPU while waiting. In RTOS, this starves lower-priority tasks and prevents the CPU from entering low-power mode.

### Pattern 2: Semaphore (Correct RTOS Pattern)
```c
// Created in main() before vTaskStartScheduler():
SemaphoreHandle_t g_snapshot_sem;
g_snapshot_sem = xSemaphoreCreateBinary();

// Timer ISR:
void TIM2_IRQHandler(void) {
    if (TIM2->SR & TIM_SR_UIF) {
        TIM2->SR &= ~TIM_SR_UIF;
        BaseType_t higher_prio_woken = pdFALSE;
        xSemaphoreGiveFromISR(g_snapshot_sem, &higher_prio_woken);
        portYIELD_FROM_ISR(higher_prio_woken);  // Context switch if telemetry task unblocked
    }
}

// Telemetry task — SLEEPS while waiting (0% CPU usage during wait):
void telemetry_task(void *params) {
    while (1) {
        xSemaphoreTake(g_snapshot_sem, portMAX_DELAY);  // Blocks until ISR gives
        // WAKES IMMEDIATELY when ISR fires. Zero delay. Zero polling.
        snapshot_capture(&current_snap);
        transmit_packet(&current_snap);
    }
}
```

The semaphore also removes the need for `volatile` on the signaling mechanism — the FreeRTOS semaphore APIs internally use the correct memory barriers.

## 3.3 The Critical Section — Atomic Windows in RTOS

### What is Atomicity?

An **atomic operation** is one that cannot be interrupted mid-way. On ARM Cortex-M:
- A single 32-bit `STR` instruction is atomic — it completes in one bus cycle
- `x++` is NOT atomic: it is LDR (read) → ADD (increment) → STR (write) — 3 instructions

If an ISR fires between the LDR and the STR of `x++` and the ISR also modifies `x`, you get a **race condition**:
```
Main task:    LDR R0, [x_addr]   ; R0 = 5
ISR fires:    LDR R1, [x_addr]   ; R1 = 5
              ADD R1, R1, #1     ; R1 = 6
              STR R1, [x_addr]   ; x = 6
ISR returns:
Main task:    ADD R0, R0, #1     ; R0 = 6  (stale read! x was 5 when we read it, now it's 6)
              STR R0, [x_addr]   ; x = 6   (we overwrote ISR's write with the same value)
```

Expected: x = 7 (incremented twice). Actual: x = 6 (ISR's increment was lost).

### FreeRTOS Critical Sections

```c
// Protect a compound operation:
taskENTER_CRITICAL();
    // Everything here is ATOMIC from the perspective of other RTOS tasks and interrupts
    // below configMAX_SYSCALL_INTERRUPT_PRIORITY:
    shared_counter++;                 // Now safe
    if (shared_counter >= MAX) {
        reset_system();
    }
taskEXIT_CRITICAL();

// What taskENTER_CRITICAL actually does (ARM Cortex-M):
// CPSID I    — disable interrupts (sets PRIMASK = 1)
// Then on EXIT:
// CPSIE I    — re-enable interrupts (clears PRIMASK)
```

**Critical section nesting (FreeRTOS supports this correctly):**
```c
taskENTER_CRITICAL();  // Disables IRQs, sets nesting counter to 1
taskENTER_CRITICAL();  // Sets nesting counter to 2
taskEXIT_CRITICAL();   // Sets nesting counter to 1 — does NOT re-enable IRQs yet
taskEXIT_CRITICAL();   // Sets nesting counter to 0 — re-enables IRQs
```

Without nesting support, two paired critical sections that accidentally overlap would re-enable interrupts prematurely. FreeRTOS correctly handles nesting.

### `snapshot_capture()` — Why It Needs a Critical Section

During snapshot capture, the FreeRTOS task list must be read in a consistent state. If the scheduler performs a context switch between reading task 1's state and task 2's state, the snapshot could contain:
- Task 1's state from time T
- Task 3 (a newly created task that didn't exist at T) because its TCB was inserted into the list during the switch
- Task 2's state from time T + some delta

This inconsistency makes the delta encoder report false changes (the twin changes state that didn't actually change on the device).

```c
void snapshot_capture(full_snapshot_t *out) {
    static TaskStatus_t task_buf[MAX_TASKS];

    uint32_t start_cycles = DWT->CYCCNT;    // Start timing BEFORE critical section

    taskENTER_CRITICAL();                   // ← Disable interrupts/scheduler
    {
        // ALL OS state reads happen here — ATOMIC, consistent snapshot:
        UBaseType_t count = uxTaskGetSystemState(task_buf, MAX_TASKS, NULL);
        uint32_t free_heap = xPortGetFreeHeapSize();
        uint32_t tick_count = xTaskGetTickCount();

        // Fill output struct INSIDE the critical section while state is frozen:
        memset(out, 0, sizeof(*out));
        out->timestamp_us = tick_count * 1000ULL;  // ms → µs (approximate)
        out->memory.heap_free = free_heap;
        for (UBaseType_t i = 0; i < count && i < MAX_TASKS; i++) {
            out->tasks[i].state    = (uint8_t)task_buf[i].eCurrentState;
            out->tasks[i].priority = (uint8_t)task_buf[i].uxCurrentPriority;
        }
    }
    taskEXIT_CRITICAL();                    // ← Re-enable interrupts

    uint32_t elapsed = DWT->CYCCNT - start_cycles;
    // Verify: elapsed must be < 150 µs = 150 × 168 = 25,200 cycles
    configASSERT(elapsed < 25200);
}
```

**Critical section duration budget:** The critical section disables interrupt responses, which increases ISR latency. For safety-critical code, maximum critical section duration should be < 10 µs. For RTOSTwin, the `uxTaskGetSystemState()` call reads MAX_TASKS = 10 TCBs from the linked list. Each TCB read = ~10 memory accesses = ~20 cycles at 168 MHz. Total = ~200 cycles = 1.2 µs. Well within budget.

## 3.4 MMIO Pattern Library — The 5 Patterns Used in RTOSTwin

These 5 patterns appear in nearly every embedded firmware project. Memorize them.

### Pattern 1: Read-Modify-Write (Set a Bit)
```c
// Enable GPIOA clock in RCC:
RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
// Expands to:
// LDR R0, [RCC_BASE + AHB1ENR_OFFSET]  ; Read current value
// ORR R0, R0, #(1 << 0)               ; Set bit 0 (GPIOAEN)
// STR R0, [RCC_BASE + AHB1ENR_OFFSET]  ; Write back
```
**Risk:** NOT atomic. An ISR firing between LDR and STR could modify another bit in AHB1ENR, and our STR would overwrite the ISR's change. Solution: do clock enables before RTOS starts (in `main()` before `vTaskStartScheduler()`).

### Pattern 2: Read-Modify-Write (Clear a Bit)
```c
// Clear the TXE interrupt enable bit in USART_CR1:
USART2->CR1 &= ~USART_CR1_TXEIE;
// ~USART_CR1_TXEIE = ~(1 << 7) = 0xFFFFFF7F
// &= → clears bit 7, preserves all other bits
```

### Pattern 3: Atomic Single-Bit Set via BSRR
```c
// Set PA5 HIGH atomically (no read-modify-write):
GPIOA->BSRR = (1 << 5);         // Write to bits [15:0] → SET those pins
// Set PA5 LOW atomically:
GPIOA->BSRR = (1 << (5 + 16));  // Write to bits [31:16] → RESET those pins

// BSRR (Bit Set/Reset Register) is designed for atomic GPIO operations:
// - A write to bit N (0-15) SETS output N
// - A write to bit N+16 (16-31) RESETS output N
// - If both bit N and bit N+16 are set in the same write, SET wins
// - The hardware does this atomically in ONE bus cycle — no race condition possible
```

### Pattern 4: Status Register Polling (With `volatile`)
```c
// Wait for ADC conversion to complete:
while (!(ADC1->SR & ADC_SR_EOC)) { }   // EOC = End Of Conversion
// ADC1->SR is volatile uint32_t → re-read every iteration
// When conversion completes, hardware sets SR.EOC → loop exits
uint16_t adc_value = ADC1->DR;         // Read converted value (also clears EOC in some modes)
```

### Pattern 5: Write-Only Register (Must NOT read-modify-write)
```c
// Timer capture/compare registers: reading CCR while counting = undefined behavior
// Write directly without reading:
TIM2->CCR1 = 1000;  // Set compare value to generate PWM at 50% (if ARR = 2000)
// Do NOT do: TIM2->CCR1 |= 1000; — this reads CCR (unpredictable) then OR's it
```

---

# 4️⃣ Practical Firmware Implementation Insights

## 4.1 The Complete `volatile` Audit Checklist for RTOSTwin

Before shipping any firmware, run through this checklist:

```
☐ DWT->CYCCNT — IS volatile? YES (CMSIS declares it as volatile)
  → Verify: Check core_cm4.h, struct DWT_Type, CYCCNT field must be volatile uint32_t

☐ UART status register (USART2->SR) — IS volatile? YES (CMSIS GPIO_TypeDef all fields volatile)
  → Verify: Check stm32f4xx.h, USART_TypeDef has volatile before each field

☐ rx_buffer[] (DMA destination) — IS volatile? DEPENDS
  → If CPU reads rx_buffer only AFTER DMA transfer complete interrupt fires: NO volatile needed
     (The interrupt acts as a memory barrier — CPU knows DMA completed before reading)
  → If CPU and DMA concurrently access rx_buffer: YES, volatile required
  → RTOSTwin: Uses DMA Complete ISR callback → volatile NOT required for rx_buffer

☐ g_snapshot_request (ISR → task flag) — MUST be volatile
  → Without FreeRTOS semaphore: YES, volatile required
  → With FreeRTOS semaphore: NO, xSemaphoreGiveFromISR includes memory barriers

☐ shared_counter between two FreeRTOS tasks — volatile alone INSUFFICIENT
  → Need volatile for single-register types (read/write fits in one instruction: uint8_t, uint16_t, uint32_t)
  → For uint64_t: volatile is insufficient (two 32-bit operations — need mutex)
  → For compound operations (++, +=): need critical section or mutex regardless of volatile
```

## 4.2 The `static` Module Pattern — RTOSTwin's Internal Architecture

In production firmware, each `.c` file is a **module** with a public interface and private internals:

```c
// snapshot.h — PUBLIC INTERFACE (exposed to other modules):
#ifndef SNAPSHOT_H
#define SNAPSHOT_H

#include "types.h"

// Public functions — other modules CAN call these:
void     snapshot_init(void);
int32_t  snapshot_capture(full_snapshot_t *out);

#endif

// snapshot.c — PRIVATE IMPLEMENTATION:
#include "snapshot.h"

// Private state — ONLY this file can see or modify this:
static full_snapshot_t s_previous;          // Previous snapshot for delta
static TaskStatus_t    s_task_buf[MAX_TASKS]; // Static task status buffer
static uint32_t        s_call_count = 0;    // How many times captured
static uint32_t        s_budget_violations = 0; // Times we exceeded 150µs

// Private helper — the 'static' makes it invisible outside this file:
static uint32_t measure_start(void) {
    return DWT->CYCCNT;
}

static void measure_end(uint32_t start) {
    uint32_t elapsed = DWT->CYCCNT - start;
    if (elapsed > 25200) {  // 150 µs × 168 MHz
        s_budget_violations++;
    }
}

// Public implementation:
void snapshot_init(void) {
    memset(&s_previous, 0, sizeof(s_previous));
    s_call_count = 0;
}

int32_t snapshot_capture(full_snapshot_t *out) {
    uint32_t start = measure_start();
    // ... capture code ...
    measure_end(start);
    s_call_count++;
    return 0;
}
```

**Naming convention:** Static file-scope variables are often prefixed with `s_` (for static/state) or a module prefix (`snapshot_`) to visually mark them as module-private state. This is a common embedded coding standard.

## 4.3 `volatile` as a Code Review Red Flag and a Green Flag

### Red Flags (Wrong `volatile` Usage):

```c
// WRONG: volatile on a purely local variable (not shared with anyone)
void some_function(void) {
    volatile int i;
    for (volatile int i = 0; i < 1000; i++) { }  // Prevents loop optimization
    // If this loop is just a delay: use DWT delay or FreeRTOS delay instead!
    // 'volatile' for software delays is NOT portable and NOT accurate
}

// WRONG: volatile instead of a mutex for 64-bit shared variable
volatile uint64_t timestamp;  // Two 32-bit operations → STILL not atomic
// If ISR reads high word, then context switch occurs, main writes both words,
// then ISR reads low word → ISR sees mismatched halves → corrupted timestamp
```

### Green Flags (Correct `volatile` Usage):

```c
// CORRECT: ISR semaphore alternative (bare-metal, no RTOS):
volatile uint8_t  g_uart_rx_complete = 0;    // ISR → main, single-byte flag ✓
volatile uint32_t g_tick_ms = 0;             // SysTick ISR increments this ✓

// CORRECT: Hardware register pointer:
volatile uint32_t * const UART_DR = (volatile uint32_t*)0x40004404;  ✓

// CORRECT: DMA buffer (concurrent access, no completion interrupt):
volatile uint8_t g_adc_dma_buf[1024];    // ADC+DMA writes, CPU reads concurrently ✓
```

## 4.4 Common Pitfalls

### Pitfall 1: Casting Away `volatile` in `memcpy`
```c
volatile uint8_t rx_buf[256];
uint8_t process_buf[256];

// WRONG — undefined behavior:
memcpy(process_buf, rx_buf, 256);
// memcpy takes const void* — passing volatile uint8_t* discards volatile
// GCC will emit a warning: "passing argument from 'volatile uint8_t *' discards 'volatile' qualifier"
// Result: memcpy may read the buffer with cached values, missing DMA-written bytes

// CORRECT — explicit copy respecting volatile:
for (int i = 0; i < 256; i++) {
    process_buf[i] = rx_buf[i];     // Each read is a separate volatile load ✓
}

// OR: Cast carefully with full understanding:
memcpy(process_buf, (uint8_t*)rx_buf, 256);   // Cast if: DMA is COMPLETE before this runs
// Only safe if DMA completion interrupt fires FIRST (which acts as memory barrier)
```

### Pitfall 2: `static` Local Makes Functions Non-Reentrant
```c
// DANGER: NOT reentrant
char *format_snapshot(const full_snapshot_t *snap) {
    static char buf[512];    // Shared across ALL calls — no matter who calls it
    snprintf(buf, sizeof(buf), "Tasks: %d, Heap: %lu", snap->task_count, snap->memory.heap_free);
    return buf;              // Returns pointer to the static buffer
}

// If Task A calls format_snapshot(), saves the pointer...
// Then Task B preempts and calls format_snapshot()...
// Then Task A uses its pointer → reads Task B's formatted string, not its own!

// CORRECT: Caller provides buffer (reentrant):
void format_snapshot(const full_snapshot_t *snap, char *buf, size_t buf_size) {
    snprintf(buf, buf_size, "Tasks: %d, Heap: %lu", snap->task_count, snap->memory.heap_free);
    // buf is on the caller's stack → no sharing
}
```

### Pitfall 3: The Volatile-But-Not-Atomic Race
```c
volatile uint32_t shared_var = 0;

// Task A (runs at priority 1):
shared_var++;   // NOT atomic: LDR, ADD, STR

// Task B (priority 2, can preempt A):
shared_var++;

// If B preempts between A's LDR and STR: both read 0, both write 1
// Expected: 2. Actual: 1. RACE CONDITION.

// FIX:
taskENTER_CRITICAL();
shared_var++;  // Now atomic from RTOS perspective
taskEXIT_CRITICAL();
// OR: Use a FreeRTOS mutex
```

### Pitfall 4: `static` Variable Initialization Ordering
```c
// Module A (a.c):
static uint32_t A_count = B_get_initial_count();  // ERROR!
// Static variables are initialized BEFORE main() — B_get_initial_count() may not be safe to call yet

// CORRECT: Lazy initialization:
static uint32_t A_count = 0;
void A_init(void) {
    A_count = B_get_initial_count();  // Called explicitly during init phase
}
```

---

# 📝 Homework

## A. Theoretical Homework

Submit answers to: `ryn/homework/theoretical/week1/day3_theory_answers.md`

**Question 1: Compiler Optimization Analysis**

Given the following code compiled with `-O2`:
```c
uint32_t status_register;    // Address: 0x40004414 — NO volatile
// Hardware writes to this address asynchronously

void wait_ready(void) {
    while (!(status_register & 0x01)) { }  // Bit 0 = ready
    perform_action();
}
```

a) Describe the three-instruction sequence the CPU executes for the test `!(status_register & 0x01)` without optimization.  
b) With `-O2`, identify which transformation the optimizer applies and rewrite the loop in equivalent C showing the optimization. What does the generated infinite loop look like?  
c) Adding `volatile` prevents this. Prove this by showing the required assembly instruction that must be inside the loop body for correct behavior.  
d) If this function is called with optimization enabled and `status_register` is set by a timer ISR that fires 10ms later, what happens? Analyze the timing from power-on to function return (if it ever returns).

---

**Question 2: Static Scope and Lifetime Audit**

Audit this code and fix every error:
```c
// sensor.c
uint8_t sensor_error_count = 0;    // A
const float GRAVITY = 9.81;        // B
static float last_reading = 0.0f;  // C

float read_sensor(void) {
    float buffer[64];               // D
    static uint8_t call_num = 0;    // E
    call_num++;
    
    // ... fill buffer, return average ...
    last_reading = average;
    return average;
}

// main.c
extern float last_reading;          // F — will this link?
```

For each label A-F: (1) Which section (.text, .rodata, .data, .bss, stack)? (2) Is there a problem? (3) Fix it if needed.

---

**Question 3: Critical Section Overhead Formal Model**

RTOSTwin disables interrupts for the duration of `uxTaskGetSystemState()` + struct copy.

Given: 168 MHz CPU. `uxTaskGetSystemState()` with 10 tasks takes 420 cycles. Struct fill (memset + field copies) takes 180 cycles. Total critical section = 600 cycles.

a) Calculate the critical section duration in microseconds.  
b) If a UART ISR is awaiting service during this window, what is the added ISR latency?  
c) At 115200 baud (1 bit = 8.68 µs, 1 byte = 86.8 µs): how many bytes of incoming UART data could arrive during the critical section? Does the UART hardware buffer protect you? (Hint: UART has a 1-byte receive data register + 1-byte shift register = 2 bytes buffering)  
d) At what snapshot rate (Hz) does the cumulative critical section overhead exceed 1% of CPU time? Show the calculation.

---

**Question 4: `volatile` Without Thread Safety — Constructing the Race**

```c
volatile uint32_t packet_count = 0;
```

Task A runs every 10 ms and does: `packet_count++`  
Task B runs every 100 ms and does: `if (packet_count > 100) { reset_counter(); }`

a) Even though `packet_count` is `volatile`, describe a specific interleaving of Task A and Task B that produces an incorrect result. Show the step-by-step CPU instruction sequence that causes the race.  
b) The fix using critical sections: rewrite both tasks to be safe.  
c) Would a FreeRTOS `uint32_t` counter wrapped in `portATOMIC_ENTER_CRITICAL()` be sufficient? Or is a full mutex required? Justify.

---

## B. Code Homework

Submit to: `ryn/homework/code/week1/day3/`

**Assignment 1: Assembly Verification Lab (`volatile_asm_check.c`)**

Write a program with two versions of the same polling loop:
1. `volatile uint32_t *hw_reg = (volatile uint32_t*)some_address;`  — with volatile
2. `uint32_t *hw_reg = (uint32_t*)some_address;` — without volatile

Compile BOTH versions with `gcc -O2 -S`. Diff the assembly output files.  
Answer in a comment block:
1. Which specific assembly instruction is present inside the loop in version 1 but outside (or absent from) the loop in version 2?
2. How many MORE instructions does the volatile version execute per loop iteration?
3. What is the cycle cost of this overhead at 168 MHz if the loop runs 10,000 times?

**Assignment 2: Module Encapsulation Practice (`snapshot_module/`)**

Create a small 2-file module:
- `snapshot_module.h` — declares ONLY `snapshot_init()` and `snapshot_capture(desc_t *out)`
- `snapshot_module.c` — implements with `static` internal state:
  - `static uint32_t s_call_count`
  - `static uint32_t s_max_cycles`
  - `static char s_last_err[64]`
  - `static void s_update_stats(uint32_t cycles)` — private helper
- `main.c` — uses ONLY the public interface

Try in `main.c`: access `s_call_count` directly. Verify the compiler rejects it.  
Try: call `s_update_stats()` from `main.c`. Verify linker rejects it.  
Document the exact error messages in a comment.

**Assignment 3: Race Condition Demonstration (`race_demo.c`)**

Using POSIX threads (`pthread.h`) on Linux/Mac or Windows threads on Windows:
1. Create a `uint32_t counter = 0` (NOT volatile, NOT protected)
2. Launch 2 threads. Each increments `counter` 1,000,000 times in a loop
3. Join both threads. Print `counter`
4. Expected: 2,000,000. Actual: Much less (data loss from race)
5. Fix version 1: Add `volatile` — does it fix the race? (It will NOT — show why)
6. Fix version 2: Add a mutex (`pthread_mutex_t`) — now counter = 2,000,000 ✓
7. Write a clear comment explaining why `volatile` was insufficient and exactly what the mutex provides that volatile does not

---

# 📚 Reference Materials

1. **GCC Documentation — Volatiles** — Section 6.46 (When GCC Assumes No Side Effects)  
   → Formal specification of what GCC's `-O2` can and cannot do to volatile accesses  
   → [gcc.gnu.org/onlinedocs](https://gcc.gnu.org/onlinedocs/gcc/Volatiles.html)

2. **ARM IHI0042F — Procedure Call Standard for Arm Architecture (AAPCS)**  
   → The definitive rule book for which registers are callee-saved vs caller-saved  
   → How the compiler decides which registers to use for locals vs function arguments  
   → [developer.arm.com/documentation/ihi0042](https://developer.arm.com/documentation/ihi0042/latest/)

3. **FreeRTOS Source: `tasks.c`** — Read `vTaskEnterCritical()` and `vTaskExitCritical()`  
   → Exactly 30 lines of C. Shows the nesting counter and the PRIMASK manipulation.  
   → In your project: `Middlewares/Third_Party/FreeRTOS/Source/tasks.c` around line 2800

4. **"Embedded C Coding Standard" — Barr Group**  
   → Rule 4.2: volatile usage rules  
   → Rule 5.1: static local variables  
   → Free PDF: [barrgroup.com/embedded-systems/books/embedded-c-coding-standard](https://barrgroup.com/sites/default/files/barr_c_coding_standard_2018.pdf)

---

# 🔑 Key Points Summary

Copy this to your `ryn/notes/week1/day3_key_points.md`:

```
VOLATILE CONTRACT (The 4 Guarantees):
  1. Every read generates a real LDR instruction (no cached reads)
  2. Every write generates a real STR instruction (no dead-store elimination)
  3. Accesses stay in source code order (no LICM, no hoisting out of loops)
  4. Reads are never merged, writes are never merged

VOLATILE DOES NOT PROVIDE:
  - Atomicity for 64-bit types (two 32-bit operations)
  - Atomicity for read-modify-write (x++ is still 3 instructions)
  - Ordering guarantees relative to non-volatile accesses
  - Thread safety (not a mutex)

THE 4 MANDATORY volatile USES:
  1. Hardware peripheral registers (UART_SR, GPIO_ODR, etc.)
  2. Variables written in ISR, read in main/task (without semaphore)
  3. Variables shared between FreeRTOS tasks (for primitive types only)
  4. DMA destination buffers (concurrent DMA write + CPU read)

THE 3 FACES OF static:
  Face 1 — Inside function: persistent across calls, in .bss, NOT on stack
  Face 2 — File scope:      private to the .c file (linkage = file)
  Face 3 — In function:     compile-time allocated buffer, fixed address

THE MMIO 5 PATTERNS (Memorize):
  1. Set a bit:     reg |= (1 << n)
  2. Clear a bit:   reg &= ~(1 << n)
  3. Toggle a bit:  reg ^= (1 << n)
  4. BSRR set:      GPIOX->BSRR = (1 << n)         — atomic!
  5. BSRR reset:    GPIOX->BSRR = (1 << (n + 16))  — atomic!

CRITICAL SECTION RULES:
  - Max duration: < 50 µs (2 bytes of UART data at 115200 baud can arrive)
  - Use taskENTER_CRITICAL() (not __disable_irq()) in RTOS — supports nesting
  - Critical section disables IRQs ≤ configMAX_SYSCALL_INTERRUPT_PRIORITY
  - Higher priority IRQs (e.g., RTOS-exempt) still fire — they cannot use RTOS APIs

ASSEMBLY VERIFICATION COMMAND:
  arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -O2 -S -o out.s file.c
  → Look for LDR inside loop body to verify volatile is doing its job

INTERVIEW TALKING POINTS:
  - "volatile prevents three specific compiler transformations: dead-store elimination,
    loop invariant code motion, and redundant load/store elimination. Without it, the
    optimized binary may loop forever waiting for a hardware flag that the optimizer
    cached into a register once."
  - "volatile is not a mutex. For a uint32_t read and written by one ISR and one task,
    volatile is sufficient IF you don't do compound operations. For x++, you need a
    critical section because that's three ARM instructions, not one."
  - "Every static local buffer in snapshot_capture() is there to prevent stack overflow.
    The task stack is 2 KB. uxTaskGetSystemState() can use 400+ bytes internally.
    If I put TaskStatus_t task_buf[10] on the stack too, I'd overflow on the first call."
  - "RTOSTwin's critical section lasts < 4 µs (600 cycles at 168 MHz). At 115200 baud,
    one byte takes 86.8 µs — so no UART bytes are lost during our critical section."
```

---

**END OF WEEK 1 DAY 3 NOTES**

*Next: Day 4 — Self-Test, Teach Prep, and Complete Homework. You now have the full C foundation for embedded work. Weeks 2+ move to actual hardware.*
