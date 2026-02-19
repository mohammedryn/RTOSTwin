# Week 1, Day 1 — C Types, Memory Layout & Pointers
## RYN's Engineering Coach Notes

**Topic:** C for Embedded Systems — The Type System, Memory Model, Pointers, `volatile`  
**Platform:** STM32F407 (ARM Cortex-M4, 168 MHz, 192 KB RAM, 1 MB Flash)  
**Project Connection:** RTOSTwin Telemetry Agent — `snapshot_capture()`, `full_snapshot_t`, RAM budget  

---

> [!IMPORTANT]
> **Before you read a single line of this:** Open a blank sheet of paper. At the end of each section, close this document and write what you just learned from memory. If you can't write it, you don't know it yet. Real embedded engineers don't "sort of understand" their type system — they know it cold.

---

# 1️⃣ Motivation & System Context — Why This Kills Production Systems

## The Real Cost of Getting This Wrong

In 1996, the **Ariane 5 rocket** exploded 37 seconds after launch. A 64-bit floating-point number was converted to a 16-bit signed integer. The value overflowed. The inertial reference system crashed. The rocket self-destructed.

Loss: **$370 million and 10 years of work**.

In 2003, a **Toyota Camry** accelerator stuck open. Root cause: a local variable on a task stack corrupted adjacent stack memory. Integer type mismatch caused incorrect motor commands. People died.

In embedded firmware, **wrong types and wrong memory decisions are not compiler warnings. They are silent killers.** The code compiles. The program runs. The bug appears three months later on a production unit in a factory — or in a car — and you don't know why.

## RTOSTwin Connection — Why You Must Master This Now

Your telemetry agent has a **non-negotiable production constraint:**

```
CPU overhead:   < 2% of processor time
RAM budget:     < 10 KB total for the entire agent
Capture time:   < 150 µs for snapshot_capture()
```

Every type decision you make directly affects these numbers:

- `full_snapshot_t` with wrong types → struct balloons from 360 bytes to 1 KB → RAM budget violated
- `uint32_t timestamp` instead of `uint64_t` → wraps every 71 minutes → telemetry corrupts after one hour
- Missing `volatile` on a flag → compiler optimizes it away → telemetry task never wakes up
- `malloc()` instead of `static` → 200 extra CPU cycles per call → > 2% overhead

You are not writing a homework program. You are writing firmware that will run on real hardware, sending real data to a real digital twin. **Get the types right.**

---

# 2️⃣ Foundational Theory — First Principles

## 2.1 What a Type Is, Physically

A **type** is not just a label. It is a contract with the CPU about:
1. **How many bytes** to allocate
2. **How to interpret** those bytes (signed vs unsigned, integer vs float)
3. **Which CPU instructions** to emit (signed division vs unsigned division are different instructions)
4. **What values are valid** (the domain)

A `uint8_t` variable occupies exactly **8 physical flip-flops** on the silicon die inside the STM32. Each flip-flop stores one bit. The number `200` stored in a `uint8_t` is physically represented as:

```
Bit:   7   6   5   4   3   2   1   0
Val:   1   1   0   0   1   0   0   0
       └── MSB                  LSB ─┘
```

This is not an abstraction. These are actual voltage levels — HIGH (≈3.3V) or LOW (≈0V) — stored in a circuit. When you write `uint8_t x = 200;`, you are telling the compiler exactly which 8 bits of the CPU's register to put those voltages into.

**When you write `int x = 200;` instead**, you're telling the compiler "use however many bits `int` is on this platform." On your STM32: 32 bits. On an MSP430: 16 bits. The SAME `.c` file compiles to different machine code and stores x in a different number of flip-flops depending on the processor.

## 2.2 The Binary Number System — Building Integers from First Principles

A N-bit unsigned integer can represent values in the range **[0, 2ᴺ − 1]**.

### Unsigned Integer Value Formula

For an N-bit unsigned binary number with bits $b_{N-1}, b_{N-2}, ..., b_1, b_0$:

$$V = \sum_{i=0}^{N-1} b_i \cdot 2^i$$

Example: 8-bit value `0b11001000` (= `0xC8`):

$$V = 0 \cdot 2^0 + 0 \cdot 2^1 + 0 \cdot 2^2 + 1 \cdot 2^3 + 0 \cdot 2^4 + 0 \cdot 2^5 + 1 \cdot 2^6 + 1 \cdot 2^7$$
$$V = 0 + 0 + 0 + 8 + 0 + 0 + 64 + 128 = 200$$

This formula is the foundation of everything. Understand it, don't just memorize it.

### Unsigned Overflow — Modular Arithmetic

When a value exceeds the maximum:

$$V_{stored} = V_{actual} \mod 2^N$$

Storing 300 in `uint8_t` (N=8, max=255):
$$V_{stored} = 300 \mod 256 = 300 - 256 = 44$$

Storing 70,000 in `uint16_t` (N=16, max=65535):
$$V_{stored} = 70000 \mod 65536 = 70000 - 65536 = 4464$$

This is **not a bug in C**. It is defined, deterministic, mathematical behavior. It kills you when you don't expect it. It saves you when you use it correctly (timestamp wraparound — see below).

### Signed Integers — Two's Complement

For a **signed** N-bit integer, the MSB (bit N-1) is the **sign bit**. The value formula changes:

$$V = -b_{N-1} \cdot 2^{N-1} + \sum_{i=0}^{N-2} b_i \cdot 2^i$$

For `int8_t` with bits `1 1 1 1 1 1 1 1` (= `0xFF`):

$$V = -(1 \cdot 2^7) + (1 \cdot 64) + (1 \cdot 32) + (1 \cdot 16) + (1 \cdot 8) + (1 \cdot 4) + (1 \cdot 2) + (1 \cdot 1)$$
$$V = -128 + 127 = -1$$

**This is why `0xFF` as `int8_t` = -1.** And it's why `(uint8_t)(-1) = 255` — the same bit pattern `0xFF` interpreted as unsigned.

### Two's Complement Conversion (Manual)

To negate a number using two's complement:
1. Start with the positive binary representation
2. Invert ALL bits (ones' complement)
3. Add 1

Example: -42 in 8 bits:
```
42  = 0b00101010
~42 = 0b11010101   (invert)
+1  → 0b11010110   = 0xD6
```
Verify: Using the signed formula above with `0b11010110`:
$$V = -128 + 64 + 16 + 4 + 2 = -42 ✓$$

## 2.3 The stdint.h Type Table — The Complete Reference

| Type | Width | Minimum | Maximum | ARM Register Usage |
|------|-------|---------|---------|-------------------|
| `uint8_t` | 8 bits | 0 | 255 | 32-bit register, upper 24 bits ignored |
| `int8_t` | 8 bits | −128 | +127 | 32-bit register, sign-extended |
| `uint16_t` | 16 bits | 0 | 65,535 | 32-bit register, upper 16 bits ignored |
| `int16_t` | 16 bits | −32,768 | +32,767 | 32-bit register, sign-extended |
| `uint32_t` | 32 bits | 0 | 4,294,967,295 | One 32-bit register exactly |
| `int32_t` | 32 bits | −2,147,483,648 | +2,147,483,647 | One 32-bit register exactly |
| `uint64_t` | 64 bits | 0 | 18,446,744,073,709,551,615 | Two 32-bit registers |
| `int64_t` | 64 bits | −9.2×10¹⁸ | +9.2×10¹⁸ | Two 32-bit registers |

> [!NOTE]
> On ARM Cortex-M4, **all arithmetic happens in 32-bit registers**. Storing `uint8_t x = 200` occupies a full 32-bit register (R0-R15) in the CPU — but only the lowest 8 bits are "visible" to your code. The compiler generates instructions to mask off the upper bits as needed. This means `uint8_t` doesn't save CPU registers — it only saves RAM when stored in a struct or array.

### Why the Non-Portable Types Are Dangerous

| Type | MSP430 (16-bit) | STM32F4 (32-bit) | PC x86-64 |
|------|-----------------|------------------|-----------|
| `int` | 2 bytes | 4 bytes | 4 bytes |
| `long` | 4 bytes | 4 bytes | 8 bytes |
| `long long` | 8 bytes | 8 bytes | 8 bytes |
| `void*` | 2 bytes | 4 bytes | 8 bytes |

```c
// This code is CORRECT on STM32 and BROKEN on MSP430:
int sensor_value = 3000;   // int is 16-bit on MSP430 (max 32767 — wait, 3000 fits)
int adc_reading = 65000;   // 65000 > 32767 → OVERFLOW ON MSP430 → value = -536
int product = 1000 * 40;   // 40,000 > 32,767 → OVERFLOW ON MSP430 → wrong
```

**The rule is absolute: never use `int`, `long`, or `short` in embedded code. Use `stdint.h` types exclusively.**

## 2.4 The Memory Model — Where Your Variables Live

The ARM Cortex-M4 memory is physically divided into distinct regions. Understanding this is not optional — it determines your RAM budget, boot sequence, and every static vs dynamic allocation decision.

### Physical Memory Regions on STM32F407

```
Address:        Region:              Contents:
──────────────────────────────────────────────────────────────────
0x0800_0000     FLASH (1 MB)
                ├── .text            Machine code (your compiled functions)
                ├── .rodata          Read-only data (const variables, string literals)
                └── .data_init       Initial values for .data variables (copied to RAM at boot)
──────────────────────────────────────────────────────────────────
0x2000_0000     SRAM1 (112 KB)
                ├── .data            Initialized global/static variables
                ├── .bss             Uninitialized global/static variables (zeroed at boot)
                ├── Heap      ↑      pvPortMalloc/malloc grows upward from here
                │   (free)
                │   (free)
                └── Stack     ↓      Local variables, function call frames grow DOWNWARD
0x2001_FFFF     (end of SRAM1)
──────────────────────────────────────────────────────────────────
0x4000_0000     Peripheral Registers (hardware, not RAM)
```

### What Goes Where

| Code Construct | Section | Memory Type |
|---------------|---------|-------------|
| `void foo(void) { ... }` | `.text` | Flash |
| `const char msg[] = "Hello"` | `.rodata` | Flash |
| `int global = 42;` | `.data` | RAM (value in Flash too) |
| `int global;` or `static int x;` | `.bss` | RAM only |
| `int local_var;` (inside function) | Stack | RAM |
| `malloc(100)` | Heap | RAM |

### The Critical Insight: `.bss` vs `.data` Flash Cost

```c
// This uses 4 bytes of FLASH + 4 bytes of RAM:
int counter = 0;   // .data: the "0" initial value MUST be stored in Flash
                   //        and copied to RAM by startup_stm32f407xx.s

// This uses 0 bytes of FLASH + 4 bytes of RAM:
int counter;       // .bss: startup just memsets to 0 — no Flash copy needed

// This uses 4096 bytes of FLASH + 4096 bytes of RAM:
uint8_t big_buf[4096] = {0};   // .data! Every 0x00 stored in Flash.

// This uses 0 bytes of FLASH + 4096 bytes of RAM:
uint8_t big_buf[4096];         // .bss: startup zeros it. 4 KB of Flash saved.
// OR:
static uint8_t big_buf[4096];  // Same: .bss
```

**For RTOSTwin:** The 4 KB transmit queue buffer (`static uint8_t tx_queue[4096]`) must be in `.bss`, not `.data`. If someone accidentally adds `= {0}`, the firmware binary grows by 4 KB and may not fit in Flash.

### The Stack — How Function Calls Really Work

Every function call creates a **stack frame**: a block of memory on the stack containing:
- Return address (where to jump back to when function returns)
- Saved registers (the previous function's workspace)
- Local variables (the current function's workspace)

```
           ┌─────────────────────────┐  ← SP before main() (initial SP = top of RAM)
           │   main() frame          │
           │   - local vars          │
           │   - saved LR            │
           ├─────────────────────────┤  ← SP after calling foo()
           │   foo() frame           │
           │   - local vars          │
           │   - saved R4-R11        │
           ├─────────────────────────┤  ← SP after calling bar() from foo()
           │   bar() frame           │
           │   - local vars          │
           │   - saved LR            │
           │   - saved R4-R8         │
           ├─────────────────────────┤  ← Current SP (Stack Pointer = R13)
           │   (free space)          │
           │   ...                   │
           │   (heap grows up ↑)     │
           └─────────────────────────┘  ← Bottom of RAM (0x2000_0000)
```

**Stack grows DOWN** (from high addresses to low). Every `push` decrements SP. Every `pop` increments SP.

**What stack overflow looks like:** SP decrements past the bottom of the task's allocated stack. Now it writes into an adjacent RTOS task's stack. Both tasks appear to work — until one reads corrupted data hours later. This is one of the most difficult bugs in embedded systems to diagnose.

**FreeRTOS task stack sizes:** Each task gets its OWN private stack, NOT shared:
```c
xTaskCreate(
    telemetry_task,   // Function to run
    "TelTask",        // Name for debugging
    512,              // Stack depth in WORDS (512 × 4 = 2048 bytes)
    NULL,             // Parameter
    2,                // Priority
    NULL              // Handle
);
// This allocates 2048 bytes in the FreeRTOS heap for this task's stack
```

## 2.5 RTOSTwin RAM Budget — The Formal Calculation

Starting from 192 KB (196,608 bytes) total on STM32F407:

```
Component                              Size (bytes)    Notes
─────────────────────────────────────────────────────────────────────
.data + .bss (global vars)            ~7,168           Varies by code
FreeRTOS system overhead              ~1,024           Scheduler lists, OS state
FreeRTOS heap (for tasks + queues):   ~81,920
  ├── SensorTask stack (2 KB)          2,048
  ├── TelemetryTask stack (2 KB)       2,048
  ├── ControlTask stack (4 KB)         4,096
  ├── MonitorTask stack (1 KB)         1,024
  ├── IdleTask stack (512 B)             512
  ├── TimerTask stack (1 KB)           1,024
  ├── RTOS queues + semaphores         ~2,048
  └── Free heap (margin)              ~69,120
RTOSTwin Agent:                        <10,240  ← YOUR BUDGET
  ├── full_snapshot_t current           ~360
  ├── full_snapshot_t previous          ~360    (for delta comparison)
  ├── tx_queue circular buffer        ~4,096    (32 × 128-byte slots)
  ├── task_status_buf[10]               ~320    (temp for uxTaskGetSystemState)
  └── misc (packet framing buffers)    ~5,104
ISR stacks (MSP):                      ~2,048
─────────────────────────────────────────────
Total used:                           ~102,400
Free (safety margin):                  ~94,208
```

**Constraint check:** `sizeof(full_snapshot_t) * 2 + 4096 + 320 < 10240` → `360*2 + 4416 < 10240` → `5136 < 10240` ✓

---

# 3️⃣ Deep Dive — Engineering Depth

## 3.1 The Signed/Unsigned Comparison Trap — Dissected

This is one of the most common bugs in production embedded code. It doesn't always crash — it just computes wrong.

```c
int8_t  signed_val   = -1;
uint8_t unsigned_val = 1;

if (signed_val < unsigned_val) {
    printf("Correct: -1 < 1\n");
} else {
    printf("WRONG: -1 appears >= 1\n");  // THIS EXECUTES
}
```

**What happens in the CPU (the C integer promotion rules):**
1. C compares the two values by first **promoting** both to a common type
2. `int8_t` (-1) and `uint8_t` (1) → both promoted to `int` (32-bit)
3. Promotion of `uint8_t` 1 → `int` 1 (zero-extended: MSB=0, so positive)
4. Promotion of `int8_t` -1 → `int` -1 (sign-extended: `0xFFFFFFFF`)
5. Compare -1 and 1 in 32-bit signed space → -1 < 1, **correct**

Wait — that actually gives the right answer? Let's try the real trap:

```c
int8_t  signed_val   = -1;
uint32_t unsigned_val = 1;

if (signed_val < unsigned_val) {
    // WRONG: THIS DOES NOT EXECUTE
} else {
    printf("BUG: -1 appears >= 1\n");  // THIS EXECUTES
}
```

**With `uint32_t`:**
1. Promotion: `int8_t` -1 → `int32_t` -1 → then promoted to `uint32_t` because the other operand is unsigned
2. `(uint32_t)(-1)` = `0xFFFFFFFF` = 4,294,967,295
3. Compare 4,294,967,295 with 1 → 4,294,967,295 > 1 → **signed_val appears to be greater!**

**The real attack vector in RTOSTwin:**
```c
// Bug: array index comparison with signed variable
int8_t task_id = -1;   // Error indicator
if (task_id < MAX_TASKS) {   // MAX_TASKS is uint8_t = 10
    process_task(task_id);   // RUNS when task_id = -1 → array underflow!
}

// Fix:
if (task_id >= 0 && (uint8_t)task_id < MAX_TASKS) {
    process_task((uint8_t)task_id);
}
```

## 3.2 Overflow as a Tool — Timestamp Wraparound

Don't just fear overflow. Learn to USE it:

```c
// Problem: 32-bit tick counter wraps every 2^32 ms = ~49.7 days
// uint32_t ticks = HAL_GetTick(); → wraps at midnight on day 50

// WRONG way to measure elapsed time:
uint32_t start = HAL_GetTick();
do_work();
uint32_t elapsed = HAL_GetTick() - start;
// BUG: What if start = 4294967290 and end = 5 (after wrap)?
// elapsed = 5 - 4294967290 → huge number with int, CORRECT with uint32_t!

// CORRECT: Unsigned subtraction ALWAYS gives correct elapsed time, even across wrap
// Proof:
// start = 4,294,967,290 = 0xFFFFFFFA
// end   = 5             = 0x00000005
// end - start = 0x00000005 - 0xFFFFFFFA = 0x0000000B = 11 ← CORRECT!
// (works because subtraction in 32-bit unsigned space wraps correctly)
```

**The RTOSTwin timestamp:**
```c
uint64_t timestamp_us;  // 64-bit microsecond timestamp
// Wraps every 2^64 / 1,000,000 / 3,600 / 24 / 365 = 584,942 YEARS
// You will never see this wrap. uint64_t is the correct choice.

// If you used uint32_t:
uint32_t timestamp_us;
// Wraps every 2^32 / 1,000,000 = 4,294 seconds = 71.6 MINUTES
// Telemetry corrupts every 71 minutes. Ship to customer. Never seen in testing.
```

## 3.3 `volatile` — The Compiler Optimization Battlefield

To understand `volatile`, you must first understand what a compiler optimizer does.

**The optimizer's job:** Transform your C source code into the FASTEST, SMALLEST possible machine code, while preserving the program's OBSERVABLE BEHAVIOR.

**Critical insight:** The compiler defines "observable behavior" as the behavior from the perspective of the CURRENT THREAD. It has no concept of:
- Hardware registers that change on their own
- Variables modified by interrupt service routines
- Variables modified by DMA controllers
- Variables accessed by other CPU cores

**Example 1 — Hardware Status Register (No volatile → infinite loop):**

```c
// C source (buggy):
uint32_t *UART_SR = (uint32_t*)0x40004400;
while ((*UART_SR & 0x80) == 0) { }  // Wait for TXE flag

// What the compiler SEES (without volatile):
// "You read *UART_SR once, then loop forever comparing the same value.
//  I'll just read it once and store in a register."

// Generated assembly (BROKEN):
LDR R0, [R1]       ; Read UART_SR once into R0
.loop:
  TST R0, #0x80    ; Test R0 (ALWAYS the same cached value!)
  BEQ .loop        ; Branch back if zero → INFINITE LOOP
```

The hardware changes `0x40004400`, but the CPU never re-reads it — it uses the cached copy in R0 forever.

```c
// FIX: volatile tells compiler "this memory can change behind my back"
volatile uint32_t *UART_SR = (volatile uint32_t*)0x40004400;
while ((*UART_SR & 0x80) == 0) { }

// Generated assembly (CORRECT):
.loop:
  LDR R0, [R1]     ; Re-read UART_SR from memory EVERY iteration
  TST R0, #0x80    ; Test the fresh value
  BEQ .loop        ; Branch back if not set
```

**Example 2 — ISR-Shared Flag (No volatile → dead code elimination):**

```c
// In main():
uint8_t uart_received = 0;  // Set by ISR when byte arrives

while (!uart_received) { }  // Wait for ISR to set flag

// The compiler looks at main() in isolation:
// "uart_received is always 0 (nothing in main sets it).
//  This while loop never exits. I'll optimize it to while(1)."
// It REMOVES the check and replaces with an infinite loop!

// OR with -O2:
// "uart_received is never changed in main(), so (uart_received == 0) is always true.
//  I'll just emit BL ... ; B . (infinite spin) and remove the condition entirely."

// FIX:
volatile uint8_t uart_received = 0;  // Every access is a real memory read
```

**The 4 Rules for `volatile` — Memorize These:**

| Situation | Reason |
|-----------|--------|
| Hardware register accessed via pointer | Hardware changes the value without the CPU executing code |
| Variable written in ISR, read in main | Two execution contexts — compiler only sees one |
| Variable shared between FreeRTOS tasks | Compiler doesn't know about the scheduler |
| DMA destination buffer | DMA writes memory without CPU executing stores |

**What `volatile` does NOT do:**
- It does NOT prevent race conditions (you still need critical sections for atomicity)
- It does NOT guarantee a specific ORDER of reads/writes relative to other operations
- It does NOT substitute for a mutex

## 3.4 Static Allocation — The Performance Model

### Stack Allocation vs. Static Allocation — Cycle Count Analysis

```c
// Stack allocation (inside a function):
void snapshot_capture(void) {
    TaskStatus_t task_buf[10];  // Stack allocation
    // ...
}
// Cost: `SUB SP, SP, #320` — 1 ARM instruction, 1 cycle
// But: 320 bytes consumed from the task stack permanently

// Heap allocation (dangerous in hot path):
void snapshot_capture(void) {
    TaskStatus_t *task_buf = pvPortMalloc(10 * sizeof(TaskStatus_t));
    // ...
    vPortFree(task_buf);
}
// Cost: pvPortMalloc takes ~200 cycles in best case (heap_4)
//       In worst case (fragmented heap): ~2000+ cycles — NON-DETERMINISTIC
//       vPortFree: ~100-500 cycles

// Static allocation (CORRECT for RTOSTwin):
void snapshot_capture(full_snapshot_t *out) {
    static TaskStatus_t task_buf[MAX_TASKS];  // .bss — always present, zero cost
    UBaseType_t count = uxTaskGetSystemState(task_buf, MAX_TASKS, NULL);
    // task_buf is ALWAYS available, at a FIXED address, for FREE
}
// Cost: accessing task_buf = 1 LDR instruction (load its static address from .bss)
//       Allocation cost at runtime: ZERO
```

### The 3 Faces of `static` — Precisely Defined

**Face 1: Storage duration (inside a function)**
```c
void count_me(void) {
    static uint32_t count = 0;  // In .bss, NOT on stack
    count++;
    printf("Called %lu times\n", count);
}
// count persists between calls. Lives for the ENTIRE program lifetime.
// NOT on the stack (no stack growth, no stack overflow risk)
// First call: count = 0 (zeroed by startup)
// Fifth call: count = 4
```

**Face 2: Linkage (at file scope)**
```c
// snapshot.c
static uint32_t snapshot_count = 0;  // Private to snapshot.c
// Other .c files CANNOT access or extern this variable
// Prevents namespace pollution, enforces encapsulation
```

**Face 3: Compile-time buffer (in functions)**
```c
void get_system_snapshot(full_snapshot_t *out) {
    static full_snapshot_t previous;  // 360 bytes in .bss, guaranteed to exist
    // 'previous' is ALWAYS available — no allocation, no NULL check needed
    // Stores the last snapshot for delta comparison
    compute_delta(&previous, out);
    memcpy(&previous, out, sizeof(full_snapshot_t));
}
```

## 3.5 Fixed-Point Arithmetic — The Math Behind Sensor Encoding

When Cortex-M0/M3 lacks an FPU, or when you need deterministic timing (no float variability):

**The Q format:** Represent a real number as an integer by scaling by a power of 2.

Q15 format: Multiply the real value by 2¹⁵ = 32768, store as `int16_t`
- Range: −1.0 to +0.99997 (one integer bit, 15 fractional bits)
- Resolution: 1/32768 ≈ 0.00003

**RTOSTwin uses simpler decimal scaling:**

$T_{stored} = T_{real} \times 10$

So temperature 23.7°C is stored as 237 in `health_snapshot_t.temperature_C` (`int16_t`).

**Multiplication with fixed-point (CRITICAL: use wider type to avoid overflow):**

```c
// WRONG: Overflow if a * b > 32767
int16_t a = 237;   // 23.7°C
int16_t b = 15;    // Scale factor
int16_t result = (a * b) / 10;  // 237 * 15 = 3555 → overflows int16_t! (max 32767 — wait, 3555 fits)
// Barely works here, but try a = 3000 (300.0°C), b = 100:
// 3000 * 100 = 300,000 → OVERFLOWS int16_t (max 32767)

// CORRECT: Widen to int32_t BEFORE multiplying
int32_t result_wide = (int32_t)a * (int32_t)b;  // 300000 fits in int32_t (max 2.1B)
int16_t result = (int16_t)(result_wide / 10);    // Then narrow back
```

**Rule:** When multiplying two N-bit numbers, the result can need 2N bits. Always widen BEFORE multiplying.

---

# 4️⃣ Practical Firmware Implementation Insights

## 4.1 How Types Connect to RTOSTwin's Structs

Every field in your snapshot structs has a TYPE REASON. Not habit. Not guessing. Reason:

```c
typedef struct {
    uint64_t timestamp_us;      // MUST be uint64_t: 32-bit wraps every 71 min
    uint32_t stack_used;        // 0 to 4 KB. uint16_t enough, but uint32_t = no padding needed
    uint32_t stack_total;       // Matches FreeRTOS uxTaskGetStackHighWaterMark return type
    uint32_t cpu_time_us;       // Cumulative. Can reach billions after hours → uint32_t (71 min)
    char     name[16];          // Fixed 16 bytes: pcTaskGetName returns up to 15 chars + null
    uint8_t  state;             // Only values 0-3 (Ready/Running/Blocked/Suspended) → uint8_t
    uint8_t  priority;          // FreeRTOS: 0-31 typically → uint8_t
    // 2 bytes of compiler padding here → must memset struct before use!
} task_snapshot_t;
// sizeof = 4+4+4+4+16+1+1+2 = 36 bytes (with padding)
// WITHOUT memset: the 2 padding bytes contain garbage → memcmp gives false positives!
```

```c
typedef struct {
    uint32_t heap_free;             // Current free: 0 to 192 KB → uint32_t
    uint32_t heap_total;            // Total heap: fixed → uint32_t
    uint32_t heap_min_ever_free;    // The "watermark" — minimum heap during runtime
    uint16_t fragment_count;        // Number of free blocks: rarely > 65535 → uint16_t
    // 2 bytes padding
} memory_snapshot_t;
// sizeof = 4+4+4+2+2 = 16 bytes

typedef struct {
    uint32_t uptime_sec;       // Could be days: > 65535 sec, need uint32_t
    int16_t  temperature_C;    // ×10 fixed-point. Range: -40.0 to +125.0 → -400 to +1250 → int16_t ✓
    uint16_t error_count;      // Errors since boot: > 255 possible → uint16_t
    uint8_t  cpu_utilization;  // Percentage 0-100 → uint8_t
    // 3 bytes padding
} health_snapshot_t;
// sizeof = 4+2+2+1+3 = 12 bytes
```

## 4.2 The `memset` Mandate — Protecting the Delta Encoder

The delta encoder works by comparing `current_snapshot` to `previous_snapshot` using `memcmp`. This compares every byte, including padding.

**Without `memset`:** Padding bytes contain whatever was on the stack previously (garbage). Two structs with identical fields will show different bytes in padding positions → `memcmp` returns nonzero → delta encoder thinks something changed → sends unnecessary full snapshot → wastes bandwidth.

**With `memset`:**
```c
full_snapshot_t snapshot;
memset(&snapshot, 0, sizeof(snapshot));   // FIRST: zero all bytes including padding
// NOW fill fields:
snapshot.timestamp_us = get_microseconds();
// etc...
// All padding bytes are guaranteed 0x00
// memcmp will correctly identify only real field changes
```

**This is a production rule, not a suggestion.** Add it to every struct initialization.

## 4.3 Common Pitfalls — Production Bugs from Type Errors

### Pitfall 1: `char` for Sensor Data
```c
// WRONG (bug waiting to happen):
char sensor_value = read_adc();  // char is signed! Value 200 → stored as -56
if (sensor_value > 100) { alarm(); }  // -56 > 100 is FALSE → alarm never triggers

// CORRECT:
uint8_t sensor_value = read_adc();
```

### Pitfall 2: Integer Division Truncation
```c
// WRONG:
uint8_t cpu_percent = cpu_cycles_used / total_cycles;  // Integer division: 0 when < 100%
// If cpu_cycles_used = 15000, total_cycles = 1000000:
// 15000 / 1000000 = 0 in integer math → cpu_percent = 0 (wrong! it's 1.5%)

// CORRECT: Multiply by 100 FIRST to preserve precision:
uint8_t cpu_percent = (cpu_cycles_used * 100) / total_cycles;
// 15000 * 100 = 1,500,000 / 1,000,000 = 1 → cpu_percent = 1% (acceptable rounding)
// Watch for overflow: if cpu_cycles_used > 42,949,672 → product overflows uint32_t!
// Safe fix: use uint64_t for intermediate:
uint8_t cpu_percent = (uint8_t)(((uint64_t)cpu_cycles_used * 100ULL) / total_cycles);
```

### Pitfall 3: Missing `volatile` on Telemetry Flag
```c
// task.c — runs in a FreeRTOS task:
uint8_t new_snapshot_ready = 0;  // No volatile!

void telemetry_task(void *params) {
    while (1) {
        if (new_snapshot_ready) {  // Compiler: "this is always 0, optimize away"
            transmit_snapshot();
            new_snapshot_ready = 0;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// snapshot_timer_isr() in another context:
void snapshot_timer_isr(void) {
    new_snapshot_ready = 1;  // Compiler doesn't know this ISR exists!
}

// WITH -O2 optimization, telemetry never runs. Bug invisible at -O0.
// FIX:
volatile uint8_t new_snapshot_ready = 0;
// BETTER FIX (proper RTOS pattern):
// Use xSemaphoreGiveFromISR() → xSemaphoreTake() instead of polling a flag
```

### Pitfall 4: Stack Overflow from Large Local Buffer
```c
// WRONG (will overflow a 2 KB task stack):
void snapshot_capture(full_snapshot_t *out) {
    TaskStatus_t task_buf[MAX_TASKS];   // 10 × 36 = 360 bytes on STACK
    uint8_t temp_packet[512];           // 512 bytes on STACK
    full_snapshot_t scratch;            // 360 bytes on STACK
    // Total stack usage: 360 + 512 + 360 = 1,232 bytes (60% of a 2 KB stack!)
    // Plus: function's own saved registers + other local vars → OVERFLOW
}

// CORRECT: Static buffers never touch the stack
void snapshot_capture(full_snapshot_t *out) {
    static TaskStatus_t task_buf[MAX_TASKS];  // .bss: 360 bytes, exists once, forever
    static uint8_t temp_packet[512];          // .bss: 512 bytes, not on ANY task stack
    // Stack usage: 0 bytes (only the pointer 'out' is on stack, which is 4 bytes)
}
```

---

# 📝 Homework

## A. Theoretical Homework

Submit answers to: `ryon/homework/theoretical/week1/day1_theory_answers.md`

**Question 1: Two's Complement & Overflow Analysis**

An ADC peripheral delivers temperature readings as a 12-bit unsigned value (0–4095). Your code reads it into a `uint16_t`, then the senior engineer tells you to store it in a `uint8_t` to "save RAM."

a) What is the exact stored value when the ADC reads 3000? Show the modular arithmetic.  
b) At what ADC reading does the first silent overflow occur?  
c) What is the consequence to the digital twin — what temperature does the analytics engine receive vs what the sensor actually measured?  
d) Calculate: How many bytes of RAM does storing 10 ADC readings in `uint8_t[]` vs `uint16_t[]` actually save? Is this trade-off ever worth it?

---

**Question 2: RAM Budget Formal Analysis**

The RTOSTwin agent must fit in < 10,240 bytes of RAM. Given:
- `full_snapshot_t` = 360 bytes
- Circular TX queue = 32 slots × 128 bytes each
- `TaskStatus_t` buffer [10] = 360 bytes  
- Packet framing overhead = 264 bytes

a) Calculate the total agent RAM usage. Does it fit in the budget?  
b) If snapshot rate increases to 20 Hz, what changes in the RAM budget? (hint: nothing — rate is timing, not memory). Explain WHY sampling rate does not affect static RAM usage.  
c) The Teensy 4.1 has 1 MB RAM. Would it be safe to remove the < 10 KB constraint on that platform? Give TWO reasons why you should keep it anyway.

---

**Question 3: Real-Time Constraint — Formal `volatile` Requirement**

The following pattern is used in RTOSTwin:
```
ISR sets volatile flag → Main telemetry task reads flag → sends packet
```

a) Explain mathematically why, without `volatile`, a compiler at `-O2` optimization level may eliminate the `if (flag)` check entirely. What code does the optimizer prove about the flag's value?  
b) Does adding `volatile` alone guarantee that the flag check is safe from race conditions? If not, what additional mechanism is needed?  
c) The FreeRTOS alternative uses `xSemaphoreGiveFromISR()` and `xSemaphoreTake()`. What advantage does this provide over a `volatile uint8_t` flag?

---

**Question 4: Timestamp Overflow — Formal Proof**

Prove mathematically that unsigned subtraction `(end - start)` gives the correct elapsed time even when `end < start` (wraparound occurred), assuming both are `uint32_t` and at most one wraparound occurred.

(Hint: work in modular arithmetic. Let wrap = 2³², show that `(end - start) mod wrap` gives the true elapsed time regardless of whether overflow occurred.)

---

## B. Code Homework

Submit to: `ryn/homework/code/week1/day1/`

**Assignment 1: The Complete Types Explorer (`types_explorer.c`)**

Write a C program that:
1. Prints `sizeof()` for every `stdint.h` type AND every built-in type (`char`, `short`, `int`, `long`, `long long`, `void*`)
2. Demonstrates overflow: predict the stored values for storing 300, 256, 257, -1, -128 in `uint8_t` — print computed vs actual. PREDICT BEFORE COMPILING.
3. Demonstrates the signed/unsigned comparison trap: compare `int8_t -1` vs `uint32_t 1`. Print which branch executes and WHY in a comment.
4. Prints the address of: a global variable (`.data`), a `static` local variable (`.bss`), a stack variable, a heap variable. Use `%p` format. In a comment, explain which group of addresses are close together and why.

Compile and run: `gcc -Wall -Wextra -std=c99 -O2 -o types_explorer types_explorer.c`

**Assignment 2: The `volatile` Demonstration (`volatile_demo.c`)**

Write a C program that:
1. Creates a global `uint8_t ready_flag = 0`
2. Uses `<pthread.h>` (POSIX threads, available on Linux/Mac) OR a simple timed loop to simulate an ISR setting `ready_flag = 1` after 1 second
3. In the main thread, spin-wait on `ready_flag` (poll it in a while loop)
4. Compile with `-O2` and WITHOUT `volatile`. Observe behavior (may hang forever)
5. Add `volatile` to `ready_flag`. Compile again. Observe correct termination.
6. Add a comment block explaining what changed in the generated assembly (use `gcc -S` to see)

Note: On Windows use `<windows.h>` `CreateThread()` instead of `<pthread.h>`.

---

# 📚 Reference Materials

**Read This Week:**

1. **ARM Architecture Reference Manual** — Section A2.4 (Data Types and Alignment)  
   → How ARM defines integer types, alignment requirements  
   → Free from: [developer.arm.com](https://developer.arm.com/documentation/ddi0487/latest/)

2. **FreeRTOS Reference Manual** — Chapter 3 (Task Management) — Pages 1-15  
   → `xTaskCreate()` parameters: stack depth, priority  
   → `uxTaskGetStackHighWaterMark()` — how FreeRTOS tracks stack usage  
   → Free from: [freertos.org/Documentation](https://www.freertos.org/Documentation/02-Kernel/07-Books-and-manual/01-RTOS_book)

3. **ISO/IEC 9899:1999 (C99 Standard)** — Section 6.2.5 (Types) — Paragraph 7-9  
   → The formal definition of signed/unsigned overflow  
   → Signed overflow = undefined behavior. Unsigned overflow = defined modular.

4. **MISRA-C:2012** — Rule 10.1 through 10.7 (Essential Type Model)  
   → Industry rules for safe arithmetic in embedded C  
   → Why automotive firmware bans implicit conversions

---

# 🔑 Key Points Summary

Copy this to your `ryn/notes/week1/day1_key_points.md`:

```
CORE RULES (Production Non-Negotiables):
1. Never use int, short, long, char for numeric data → always stdint.h
2. volatile on: hardware registers, ISR-shared vars, RTOS-shared vars, DMA buffers
3. static for: persistent buffers, file-private data, large local arrays
4. memset(&struct, 0, sizeof(struct)) before EVERY struct initialization
5. Order struct fields: largest type first → smallest → minimize padding
6. Widen to int32_t/int64_t BEFORE multiplying to avoid overflow

FORMULAS:
- uint8_t overflow:       stored = value mod 256
- uint16_t overflow:      stored = value mod 65536
- int8_t from uint8_t:    if MSB set, value = uint_value - 256
- Two's complement -N:    invert bits, add 1
- Fixed-point ×10:        store (real_value × 10) as int16_t
- Multiply safety:        (int32_t)a * (int32_t)b → then narrow

RAM BUDGET (STM32F407):
- Total: 192 KB
- FreeRTOS heap: ~80 KB (tasks, queues, semaphores)
- RTOSTwin agent: < 10 KB
- full_snapshot_t: ~360 bytes × 2 = 720 bytes
- TX queue: 32 × 128 = 4,096 bytes
- Total agent: ~5,136 bytes ✓ within budget

HARDWARE CONSTANTS (STM32F407):
- SRAM base:  0x20000000
- Flash base: 0x08000000
- Stack grows: DOWNWARD (from high to low address)
- ARM Cortex-M4: all arithmetic in 32-bit registers

INTERVIEW TALKING POINTS:
- "I use stdint.h because int width is platform-defined — I've seen code silently overflow
  on MSP430 because int is 16-bit there but the developer assumed 32-bit"
- "volatile doesn't prevent race conditions — it only prevents the compiler from caching
  a value. For atomicity you still need a critical section or a FreeRTOS primitive"
- "We use static allocation in the telemetry agent because malloc is non-deterministic,
  can fragment, and requires a mutex — which risks priority inversion in a real-time system"
```

---

**END OF WEEK 1 DAY 1 NOTES**

*Next: Day 2 — Pointers as Hardware Wires, Memory-Mapped I/O, and the `volatile` Register Pattern*
