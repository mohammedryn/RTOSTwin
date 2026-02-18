# Module 0.1: C Programming for Embedded Systems
## Complete Deep-Dive Explanation

**Topic:** C Programming for Embedded  
**Subtopic:** Everything a firmware engineer must know about C that a desktop/web programmer doesn't  
**Focus Level:** Beginner → Intermediate  
**Time Available:** 4 Hours  
**Target Platforms:** STM32F4 (ARM Cortex-M4, 168 MHz), Teensy 4.1 (ARM Cortex-M7, 600 MHz)  
**Connection to RTOSTwin:** This module teaches every C concept used in the telemetry agent firmware  

---

# 1️⃣ Motivation & System Context

## Why "Embedded C" Is a Different Language

You may have written C programs on your PC — `printf("Hello")`, `scanf`, `malloc`, string manipulation. In embedded, **most of that is either unavailable, dangerous, or works completely differently.**

Here's why:

| Feature | Desktop C | Embedded C |
|---------|-----------|------------|
| `printf()` | Prints to terminal instantly | May not exist. If it does, it's SLOW (blocks UART for ms) |
| `malloc()` | Almost always succeeds (virtual memory) | Can fail. Fragments memory. Causes priority inversion in RTOS |
| `int` size | Always 32 bits (modern x86/x64) | 16 bits on MSP430, 32 bits on ARM. **Non-portable!** |
| Stack size | 1–8 MB (OS-managed) | 256 bytes – 4 KB per task (YOU manage it) |
| Floating point | Always available | Cortex-M0 has NO FPU. Cortex-M4/M7 do (STM32F4 ✓, Teensy 4.1 ✓) |
| OS | Linux/Windows manages memory, files, I/O | **There is no OS** (until you add FreeRTOS) |
| Crash behavior | Segfault → OS kills your program | Hard Fault → device freezes forever (or resets via watchdog) |

## Real-World Failure: Toyota's Unintended Acceleration (2005–2010)

In 2005–2010, Toyota vehicles experienced **unintended acceleration** that killed 89 people. NASA and embedded experts analyzed the firmware and found:

1. **Stack overflow** — Toyota allocated too little stack for some tasks. Under rare conditions (specific combination of inputs), the call depth exceeded the stack, corrupting adjacent memory.
2. **Global variable abuse** — Over 10,000 global variables with no access protection. Race conditions between tasks corrupted throttle control data.
3. **No MISRA compliance** — Toyota's C code violated basic safety rules.

**The lesson:** In embedded, a C bug doesn't just crash your program. It crashes a MACHINE — a car, a ventilator, an industrial robot. The C you write in this project WILL be the kind of C that controls physical systems.

## How This Connects to RTOSTwin

Every line of our telemetry agent runs on a resource-constrained MCU:

- **STM32F4:** 168 MHz, 192 KB RAM, 1 MB Flash
- **Teensy 4.1:** 600 MHz, 1 MB RAM, 8 MB Flash (much more powerful, but same principles)

The agent must:
- Capture RTOS state in < 150 µs (25,000 CPU cycles on STM32F4)
- Use < 10 KB of RAM
- NEVER call `malloc` in the hot path
- NEVER corrupt application data
- NEVER cause a priority inversion

You cannot achieve these constraints without mastering the C topics in this module.

---

# 2️⃣ Foundational Theory (First Principles)

## 2.1 The Type System: Exact Widths or Death

### The Problem

```c
int x = 70000;
```

**What value does `x` hold?**

- On your PC (32-bit `int`): `70000` ✓
- On an MSP430 MCU (16-bit `int`): **`4464`** ✗ (overflow! 70000 mod 65536 = 4464)

Your code "works" on your PC, passes tests, ships to production, and **silently computes wrong values on the target hardware.** A sensor reading, a motor speed, a dosage calculation — all wrong.

### The Solution: `stdint.h`

The C99 standard introduced **exact-width integer types.** These are MANDATORY in embedded.

```c
#include <stdint.h>

// Unsigned (always ≥ 0)
uint8_t   a;   // Exactly 8 bits.  Range: 0 to 255
uint16_t  b;   // Exactly 16 bits. Range: 0 to 65,535
uint32_t  c;   // Exactly 32 bits. Range: 0 to 4,294,967,295
uint64_t  d;   // Exactly 64 bits. Range: 0 to 18,446,744,073,709,551,615

// Signed (can be negative)
int8_t    e;   // Exactly 8 bits.  Range: -128 to +127
int16_t   f;   // Exactly 16 bits. Range: -32,768 to +32,767
int32_t   g;   // Exactly 32 bits. Range: -2,147,483,648 to +2,147,483,647
int64_t   h;   // Exactly 64 bits. Range: ±9.2 quintillion
```

### When to Use Each Type

| Type | Use Case | Example in RTOSTwin |
|------|----------|-------------------|
| `uint8_t` | Flags, states, small enums, register bytes | `task_snapshot_t.state` (0=Ready, 1=Running, 2=Blocked) |
| `int8_t` | Small signed values | Temperature offset in °C |
| `uint16_t` | ADC readings (12-bit), moderate counters, CRC | `full_snapshot_t.crc16` |
| `int16_t` | Temperature (°C × 10 for 0.1° resolution) | `health_snapshot_t.temperature_C` |
| `uint32_t` | Addresses, tick counts, byte sizes, timestamps | `task_snapshot_t.stack_used`, `memory_snapshot_t.heap_free` |
| `int32_t` | General signed, difference calculations | Time differences |
| `uint64_t` | Microsecond timestamps (wraps after 584,942 years) | `full_snapshot_t.timestamp_us` |

### The `sizeof` Operator — Your Best Friend

**Always verify your assumptions:**

```c
#include <stdio.h>
#include <stdint.h>

int main(void) {
    printf("char:     %zu bytes\n", sizeof(char));      // Always 1
    printf("short:    %zu bytes\n", sizeof(short));     // Usually 2
    printf("int:      %zu bytes\n", sizeof(int));       // 2 or 4 (DANGER!)
    printf("long:     %zu bytes\n", sizeof(long));      // 4 or 8 (DANGER!)
    printf("float:    %zu bytes\n", sizeof(float));     // Always 4
    printf("double:   %zu bytes\n", sizeof(double));    // Always 8
    printf("uint32_t: %zu bytes\n", sizeof(uint32_t));  // Always 4 (SAFE)
    printf("void*:    %zu bytes\n", sizeof(void*));     // 4 on 32-bit ARM
    return 0;
}
```

### Type Casting — Silent Killer

```c
uint8_t sensor_raw = 200;
int8_t temperature = (int8_t)sensor_raw;  
// temperature = -56 (NOT 200!)
// Because int8_t range is -128 to +127
// 200 interpreted as signed 8-bit = 200 - 256 = -56

// FIX: Use the correct type from the start
uint8_t temperature_raw = 200;  // Raw ADC value
int16_t temperature_C = (int16_t)temperature_raw - 50;  // Apply offset in wider type
```

### Fixed-Point Arithmetic (When You Have No FPU)

The Cortex-M0 and M3 have **no floating-point unit.** Even on Cortex-M4/M7 (which have FPUs), fixed-point is faster and deterministic.

```c
// FLOATING POINT (slow on M0/M3, fast on M4/M7):
float temp = 23.7f;
float result = temp * 1.5f;  // ~14 cycles on M4 FPU, ~200 cycles on M0 (software)

// FIXED POINT (fast everywhere):
// Represent temperature as integer × scaling factor
// "Q8.8" format: 8 integer bits, 8 fractional bits
// Scale factor = 256 (2^8)

int16_t temp_fixed = (int16_t)(23.7f * 256);  // = 6067
int16_t factor_fixed = (int16_t)(1.5f * 256); // = 384
int32_t result_fixed = (int32_t)temp_fixed * factor_fixed;  // = 2,329,728
// To get actual value: result_fixed / (256 * 256) = 2329728 / 65536 = 35.55

// Common scale factors:
// × 10     for 0.1 resolution   (temperature: 237 = 23.7°C)
// × 100    for 0.01 resolution  (voltage: 330 = 3.30V)
// × 256    for Q8.8 format      (efficient on binary CPUs)
// × 1024   for Q6.10 format     (more fractional precision)
```

**RTOSTwin uses integer arithmetic in the agent** — the MCU-side code avoids `float` to ensure deterministic execution time.

---

## 2.2 Memory Layout: Understanding Every Byte

### The Four Sections of a C Program

When the compiler processes your code, it produces four distinct sections:

```
┌─────────────────────────────────┐
│          .text (Code)           │  ← Your compiled functions
│  Stored in FLASH (read-only)   │     Machine instructions
│  Survives power-off             │
├─────────────────────────────────┤
│         .rodata (Constants)     │  ← const char msg[] = "Hello";
│  Stored in FLASH (read-only)   │     const int TABLE[] = {1,2,3};
│                                 │
├─════════════════════════════════┤  ← boundary between Flash and RAM
│                                 │
│          .data (Initialized)    │  ← int counter = 42;
│  Copied from Flash to RAM      │     Startup code copies initial values
│  at boot by startup code       │     from Flash to RAM
│                                 │
├─────────────────────────────────┤
│          .bss (Uninitialized)   │  ← int buffer[100];
│  In RAM, zeroed at startup     │     static uint32_t tick_count;
│  Does NOT consume Flash space  │     (saved Flash space!)
│                                 │
├─────────────────────────────────┤
│          Heap ↑                 │  ← malloc() allocates here
│          (grows upward)         │     pvPortMalloc() in FreeRTOS
│                                 │
│         (free space)            │  ← If heap meets stack = CRASH
│                                 │
│          Stack ↓                │  ← Local variables, return addresses
│          (grows downward)       │     Each RTOS task has its OWN stack
│                                 │
└─────────────────────────────────┘
```

### Why This Matters — Calculating Your Memory Budget

**STM32F4 Memory Budget:**

```
Flash (1 MB):
  Your firmware code (usually 50–200 KB)
  + Constants, strings, lookup tables
  + FreeRTOS kernel code (~20 KB)
  Available: Plenty. Not usually the bottleneck.

RAM (192 KB = 196,608 bytes):
  .data section:             ~2 KB  (initialized globals)
  .bss section:              ~5 KB  (uninitialized globals)
  FreeRTOS heap:            ~80 KB  (configTOTAL_HEAP_SIZE)
    ├─ Task stacks:         ~40 KB  (8 tasks × 2 KB each + idle + timer)
    ├─ Queues:               ~5 KB
    └─ Free:                ~35 KB  (this is what heap_free measures!)
  RTOSTwin agent:           <10 KB  (OUR BUDGET)
    ├─ Snapshot buffers:     ~1 KB  (current + previous)
    ├─ TX queue:             ~4 KB  (32 packets × 128 bytes)
    ├─ Task status buffer:   ~1 KB  (static, for uxTaskGetSystemState)
    └─ Misc:                 ~4 KB
  System stack (MSP):        ~2 KB  (used before RTOS starts, and in ISRs)
  ─────────────────────────────────
  Total:                   ~99 KB
  Remaining:               ~97 KB (safety margin)
```

**Teensy 4.1 Memory Budget:**

```
Flash (8 MB):  Enormous. Not a concern.
RAM (1 MB):    Luxurious by embedded standards.
  Same structure, but much more headroom.
  Agent can afford larger buffers, more history.
```

### The `static` Keyword — Three Meanings

`static` is the most overloaded keyword in C. It means different things in different contexts:

```c
// MEANING 1: File-scoped global (internal linkage)
// Only visible within this .c file. Other files can't see it.
static uint32_t module_counter = 0;  // Private to this file

// MEANING 2: Persistent local variable
// Retains its value between function calls (NOT on the stack).
void count_calls(void) {
    static uint32_t call_count = 0;  // Initialized ONCE, persists forever
    call_count++;
    // After 100 calls, call_count == 100 (not 1)
}

// MEANING 3: Static allocation (no malloc)
void snapshot_capture(full_snapshot_t *out) {
    static TaskStatus_t task_buffer[MAX_TASKS];  // Allocated in .bss, not stack
    // This buffer exists for the entire program lifetime
    // It's NOT allocated on the (tiny) stack
    // It's NOT allocated with malloc (no fragmentation)
    // It's always at the same address (deterministic)
    
    UBaseType_t n = uxTaskGetSystemState(task_buffer, MAX_TASKS, NULL);
    // ... use task_buffer ...
}
```

**Why static matters for RTOSTwin:**
- Our `snapshot_capture()` function needs a `TaskStatus_t` array to receive data from FreeRTOS.
- **Stack allocation:** `TaskStatus_t buf[10];` — puts ~300 bytes on the stack. If the telemetry task's stack is small, this risks overflow.
- **Dynamic allocation:** `TaskStatus_t *buf = malloc(...)` — forbidden in the hot path.
- **Static allocation:** `static TaskStatus_t buf[10];` — lives in `.bss`, always available, zero overhead. **This is our approach.**

---

## 2.3 Pointers: The Foundation of Hardware Access

### Pointers as Memory Addresses

```c
// In desktop C, pointers are abstract — you rarely care about the actual address.
int x = 42;
int *p = &x;    // p holds "some address" — you don't care what it is
*p = 100;       // x is now 100

// In embedded C, pointers ARE the hardware:
// The LED on STM32F4 Nucleo is connected to GPIO Port A, Pin 5.
// GPIO Port A's Output Data Register (ODR) is at address 0x40020014.

// This is how you PHYSICALLY turn on an LED:
volatile uint32_t *GPIOA_ODR = (volatile uint32_t *)0x40020014;
*GPIOA_ODR |= (1 << 5);  // Set bit 5 → Pin PA5 goes to 3.3V → LED turns ON

// You just wrote to a PHYSICAL ADDRESS that connects to a PHYSICAL WIRE
// that connects to a PHYSICAL LED. That's the magic of embedded.
```

### Memory-Mapped I/O (MMIO)

On ARM Cortex-M, hardware peripherals are accessed through memory addresses. There is NO special "I/O instruction" — you just read/write normal memory addresses, and the bus connects those addresses to hardware circuits.

```
Address Range          Connected To
─────────────────────  ──────────────────────────────
0x0000_0000–0x07FF_FFFF  Flash memory (your code)
0x2000_0000–0x2002_FFFF  SRAM (your variables)
0x4000_0000–0x4000_03FF  TIM2 (Timer 2 peripheral)
0x4000_1000–0x4000_13FF  TIM3 (Timer 3 peripheral)
0x4001_1000–0x4001_13FF  USART1 (serial port)
0x4001_1400–0x4001_17FF  USART6
0x4002_0000–0x4002_03FF  GPIOA (GPIO Port A)
0x4002_0400–0x4002_07FF  GPIOB (GPIO Port B)
0xE000_E010–0xE000_E01F  SysTick (System Timer)
0xE000_ED00–0xE000_ED3F  SCB (System Control Block)
0xE000_1000–0xE000_1FFF  DWT (Debug Watchpoint & Trace)
```

**When you write `GPIOA->ODR |= (1 << 5);`, the compiler generates:**

```assembly
LDR   R0, =0x40020014    ; Load address of GPIOA_ODR into R0
LDR   R1, [R0]           ; Read current value of ODR
ORR   R1, R1, #(1<<5)    ; Set bit 5
STR   R1, [R0]           ; Write back — LED physically turns ON
```

That `STR` instruction doesn't just write to RAM. The ARM bus routes the write to the GPIO peripheral hardware, which physically changes the voltage on pin PA5.

### `volatile` — The Most Important Keyword in Embedded

```c
// WITHOUT volatile (BROKEN):
uint32_t *status = (uint32_t *)0x40001000;  // UART status register

while (*status & (1 << 5) == 0) {
    // Wait for "transmit complete" flag
}
// COMPILER THINKS: "status never changes in this loop.
//                   I'll read it once and cache the value."
// RESULT: INFINITE LOOP (compiler optimized away the re-read!)

// WITH volatile (CORRECT):
volatile uint32_t *status = (volatile uint32_t *)0x40001000;

while (*status & (1 << 5) == 0) {
    // Wait for "transmit complete" flag
}
// COMPILER KNOWS: "This address can change at any time (by hardware).
//                  I MUST re-read it every iteration."
// RESULT: Loop exits when hardware sets the flag ✓
```

**You MUST use `volatile` when:**

| Scenario | Why |
|----------|-----|
| Hardware registers | Hardware changes values independently of CPU |
| Variables shared between ISR and main code | ISR modifies while main loop reads |
| Variables shared between RTOS tasks | Another task may modify between context switches |
| DMA buffers | DMA controller writes without CPU knowledge |

**Example — ISR + Main Sharing a Variable:**

```c
// BROKEN (no volatile):
uint8_t button_pressed = 0;

void button_ISR(void) {
    button_pressed = 1;  // ISR sets the flag
}

void main(void) {
    while (1) {
        if (button_pressed) {  // Compiler caches this read!
            handle_press();     // NEVER REACHED (compiler bug!)
            button_pressed = 0;
        }
    }
}

// FIXED (with volatile):
volatile uint8_t button_pressed = 0;  // Compiler will ALWAYS re-read

void button_ISR(void) {
    button_pressed = 1;
}

void main(void) {
    while (1) {
        if (button_pressed) {  // Re-reads from RAM every iteration ✓
            handle_press();
            button_pressed = 0;
        }
    }
}
```

### Pointer Arithmetic (How Arrays Really Work)

```c
uint32_t buffer[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

uint32_t *p = buffer;        // p points to buffer[0]
// p      == address of buffer[0], e.g., 0x20001000
// p + 1  == address of buffer[1], e.g., 0x20001004 (NOT 0x20001001!)
//           Because uint32_t is 4 bytes, p+1 advances by 4 bytes

// These are IDENTICAL:
buffer[3]    == *(buffer + 3)    == *(p + 3)
&buffer[3]   == buffer + 3       == p + 3

// Iterating with pointers (faster than index on some architectures):
uint32_t *end = buffer + 10;  // One past the last element
for (uint32_t *ptr = buffer; ptr < end; ptr++) {
    *ptr = 0;  // Clear the buffer
}
```

**RTOSTwin Usage — Copying Snapshot Data:**

```c
void encode_delta(full_snapshot_t *current, full_snapshot_t *previous,
                  uint8_t *output, uint16_t *output_len) {
    uint8_t *write_ptr = output;  // Pointer walks through output buffer
    
    // Check each task for changes
    for (int i = 0; i < MAX_TASKS; i++) {
        if (memcmp(&current->tasks[i], &previous->tasks[i], 
                   sizeof(task_snapshot_t)) != 0) {
            // Task changed — write index + new data
            *write_ptr++ = (uint8_t)i;  // Write task index, advance pointer
            memcpy(write_ptr, &current->tasks[i], sizeof(task_snapshot_t));
            write_ptr += sizeof(task_snapshot_t);  // Advance by struct size
        }
    }
    
    *output_len = (uint16_t)(write_ptr - output);  // Calculate bytes written
}
```

---

## 2.4 Bitwise Operations: The Language of Hardware

### Why Bits Matter

Hardware peripherals are controlled by **registers** — 32-bit values where each individual bit has a specific meaning.

Example: STM32F4 GPIOA MODER register (Mode Register):

```
Bit 31:30  Bit 29:28  Bit 27:26  ...  Bit 11:10  ...  Bit 3:2  Bit 1:0
 Pin 15     Pin 14     Pin 13          Pin 5           Pin 1    Pin 0
  Mode       Mode       Mode           Mode            Mode     Mode

Each pin's mode is 2 bits:
  00 = Input
  01 = General purpose output
  10 = Alternate function (UART, SPI, etc.)
  11 = Analog (ADC)
```

To set **Pin 5 as output** without changing any other pin:

```c
// We need to set bits [11:10] to 01
// Step 1: Clear bits [11:10]
GPIOA->MODER &= ~(0x3 << (5 * 2));    // AND with ~(11 << 10) = AND with 1111..0011..1111
// Step 2: Set bits [11:10] to 01
GPIOA->MODER |=  (0x1 << (5 * 2));    // OR with  (01 << 10) = OR  with 0000..0100..0000
```

### All Six Bitwise Operators

```c
// AND (&) — Mask/Clear bits
// "Keep only the bits I want"
uint8_t reg = 0b11001010;
uint8_t mask = 0b00001111;
uint8_t result = reg & mask;    // 0b00001010 (upper 4 bits cleared)

// OR (|) — Set bits
// "Turn on specific bits"
uint8_t reg = 0b00001010;
reg |= 0b11000000;              // 0b11001010 (set bits 7 and 6)

// XOR (^) — Toggle bits
// "Flip specific bits"
uint8_t led_state = 0b00000001;
led_state ^= 0b00000001;        // 0b00000000 (LED OFF)
led_state ^= 0b00000001;        // 0b00000001 (LED ON again)

// NOT (~) — Invert all bits
uint8_t mask = 0b00100000;
uint8_t inverted = ~mask;        // 0b11011111

// LEFT SHIFT (<<) — Multiply by 2^n, or create bit masks
(1 << 0)  = 0b00000001  = 1
(1 << 1)  = 0b00000010  = 2
(1 << 5)  = 0b00100000  = 32
(1 << 7)  = 0b10000000  = 128

// RIGHT SHIFT (>>) — Divide by 2^n, or extract bit fields
uint8_t reg = 0b11001010;
uint8_t upper_nibble = reg >> 4;   // 0b00001100 = 12

// COMBINED: Extract bits [11:10] from a 32-bit register
uint32_t moder = GPIOA->MODER;
uint8_t pin5_mode = (moder >> 10) & 0x3;  // Shift right 10, mask to 2 bits
```

### Common Bit Manipulation Patterns (Memorize These)

```c
// 1. SET a single bit
register |= (1 << bit_number);

// 2. CLEAR a single bit
register &= ~(1 << bit_number);

// 3. TOGGLE a single bit
register ^= (1 << bit_number);

// 4. CHECK if a bit is set
if (register & (1 << bit_number)) { /* bit is 1 */ }

// 5. SET a multi-bit field (e.g., 2-bit mode field)
register &= ~(0x3 << field_start);      // Clear field first
register |=  (new_value << field_start); // Then set new value

// 6. READ a multi-bit field
uint8_t value = (register >> field_start) & field_mask;

// 7. Create a bitmask for N bits starting at position P
#define BITMASK(N, P)  (((1 << (N)) - 1) << (P))
// BITMASK(3, 4) = 0b01110000 (3 bits starting at bit 4)
```

### RTOSTwin's Delta Encoder — Bitmask in Action

```c
// The delta encoder uses a bitmask to track which parts of the snapshot changed

#define FIELD_TASKS       (1 << 0)    // Bit 0: task data changed
#define FIELD_MEMORY      (1 << 1)    // Bit 1: memory data changed
#define FIELD_PERIPHERALS (1 << 2)    // Bit 2: peripheral data changed
#define FIELD_HEALTH      (1 << 3)    // Bit 3: health metrics changed

uint8_t changed_fields = 0;

// Compare current snapshot to previous
if (memcmp(&current.tasks, &previous.tasks, sizeof(current.tasks)) != 0)
    changed_fields |= FIELD_TASKS;

if (current.memory.heap_free != previous.memory.heap_free)
    changed_fields |= FIELD_MEMORY;

if (current.health.cpu_utilization != previous.health.cpu_utilization)
    changed_fields |= FIELD_HEALTH;

// Now changed_fields tells the decoder exactly what to expect
// If changed_fields = 0b00001001 (9), then TASKS and HEALTH changed
// The delta packet only contains those two sections → 80% bandwidth savings
```

---

## 2.5 Structs: Modeling the Physical World

### Basic Struct Usage

```c
// Define a struct type
typedef struct {
    char     name[16];
    uint8_t  state;
    uint8_t  priority;
    uint32_t stack_used;
    uint32_t stack_total;
    uint32_t cpu_time_us;
} task_snapshot_t;

// Create an instance
task_snapshot_t task;

// Access fields
task.state = 1;               // Running
task.priority = 5;
task.stack_used = 512;

// Access via pointer (->)
task_snapshot_t *p = &task;
p->state = 2;                 // Blocked
p->stack_used = 600;

// Arrays of structs
task_snapshot_t tasks[10];     // 10 task snapshots
tasks[0].state = 0;            // Task 0 is Ready
tasks[3].priority = 10;        // Task 3 has priority 10
```

### Struct Padding & Alignment (The Trap)

**The compiler inserts invisible padding bytes to align fields to their natural boundaries:**

```c
// You THINK this is 7 bytes:
typedef struct {
    uint8_t  a;      // 1 byte
    uint32_t b;      // 4 bytes
    uint16_t c;      // 2 bytes
} mystery_t;         // Total: 7 bytes? WRONG!

// Actually 12 bytes! Here's why:
// Offset 0: a (1 byte)
// Offset 1-3: PADDING (3 bytes, to align b to 4-byte boundary)
// Offset 4-7: b (4 bytes)
// Offset 8-9: c (2 bytes)  
// Offset 10-11: PADDING (2 bytes, to make struct size multiple of 4)
// Total: 12 bytes

// VERIFY:
printf("sizeof(mystery_t) = %zu\n", sizeof(mystery_t));  // Prints 12, not 7!
```

**Why This Matters for RTOSTwin:**

1. **Memory waste:** If your snapshot struct has bad field ordering, padding wastes precious RAM.
2. **Protocol mismatch:** If the MCU and PC have different padding rules, the receiver decodes garbage.
3. **`memcmp` correctness:** Padding bytes contain random data. `memcmp` on two structs might say "different" even if all real fields are equal.

**Solution 1: Order fields by decreasing size**

```c
// GOOD: Minimal padding (natural alignment)
typedef struct {
    uint64_t timestamp_us;   // 8 bytes (offset 0)
    uint32_t stack_used;     // 4 bytes (offset 8)
    uint32_t stack_total;    // 4 bytes (offset 12)
    uint32_t cpu_time_us;    // 4 bytes (offset 16)
    uint16_t crc16;          // 2 bytes (offset 20)
    uint8_t  state;          // 1 byte  (offset 22)
    uint8_t  priority;       // 1 byte  (offset 23)
} task_snapshot_t;           // Total: 24 bytes, ZERO padding!
```

**Solution 2: `__attribute__((packed))` (use cautiously)**

```c
// PACKED: No padding at all
typedef struct __attribute__((packed)) {
    uint8_t  a;      // 1 byte
    uint32_t b;      // 4 bytes (NOT aligned to 4-byte boundary!)
    uint16_t c;      // 2 bytes
} packed_t;          // Total: 7 bytes (exactly as declared)

// WARNING: On ARM Cortex-M, unaligned 32-bit access is SLOWER (2 cycles → 3 cycles)
//          On some architectures (Cortex-M0), it causes a HARD FAULT!
// Use packed only for wire protocol structs, NOT for in-memory data structures.
```

**Best Practice for RTOSTwin:**
- **In-memory structs:** Natural alignment (order fields by size). Used for fast access.
- **Wire protocol structs:** Packed, with explicit field order. Used only for serialization/deserialization.

---

## 2.6 Enums and #define: Naming Your Constants

```c
// METHOD 1: #define (no type safety, but zero runtime cost)
#define TASK_STATE_READY     0
#define TASK_STATE_RUNNING   1
#define TASK_STATE_BLOCKED   2
#define TASK_STATE_SUSPENDED 3

task.state = TASK_STATE_RUNNING;  // Clear and readable


// METHOD 2: enum (type-checked by some compilers, self-documenting)
typedef enum {
    TASK_STATE_READY     = 0,
    TASK_STATE_RUNNING   = 1,
    TASK_STATE_BLOCKED   = 2,
    TASK_STATE_SUSPENDED = 3,
    TASK_STATE_COUNT     = 4   // Useful for array sizing!
} task_state_t;

task_state_t state = TASK_STATE_BLOCKED;


// METHOD 3: const (type safe, but may use RAM)
const uint8_t TASK_STATE_READY = 0;
// On embedded, this may waste 1 byte of RAM (compiler-dependent)
// #define wastes ZERO RAM — it's a text substitution at compile time
```

**Recommendation:** Use `#define` for bit positions and masks (hardware). Use `enum` for state machines and named constants (readability).

---

## 2.7 Function Pointers: Callbacks & ISR Dispatch

```c
// A function pointer holds the ADDRESS of a function
// Just like a data pointer holds the address of a variable

// Declare a function pointer type
typedef void (*callback_t)(uint32_t event_data);

// A function that matches the signature
void on_button_press(uint32_t pin_number) {
    toggle_led();
}

// Store and call the function pointer
callback_t my_callback = on_button_press;
my_callback(5);  // Calls on_button_press(5)

// REAL USE CASE: Interrupt vector table on ARM
// The startup code contains an array of function pointers:
void (* const vector_table[])(void) = {
    (void (*)(void))&_estack,     // Initial stack pointer
    Reset_Handler,                 // Reset → your main() eventually
    NMI_Handler,
    HardFault_Handler,
    // ...
    EXTI0_IRQHandler,             // GPIO interrupt → your ISR
    USART1_IRQHandler,            // UART interrupt → your ISR
    DMA1_Stream0_IRQHandler,      // DMA complete → your ISR
};
// When hardware triggers interrupt N, the CPU jumps to vector_table[N]
```

**RTOSTwin usage:** FreeRTOS trace hooks are function pointers. When a task switches, FreeRTOS calls your tracking function via a function pointer you registered.

---

## 2.8 The Preprocessor: Compile-Time Configuration

```c
// Conditional compilation — include/exclude code at compile time
#define MAX_TASKS  10
#define ENABLE_DELTA_ENCODING  1
#define TARGET_STM32F4

// Platform-specific code (HAL abstraction)
#ifdef TARGET_STM32F4
    #include "stm32f4xx_hal.h"
    #define UART_HANDLE  huart2
    #define LED_PORT     GPIOA
    #define LED_PIN      GPIO_PIN_5
#elif defined(TARGET_TEENSY41)
    #include "core_pins.h"
    #define LED_PIN      13
#else
    #error "No target platform defined!"
#endif

// Feature flags
#if ENABLE_DELTA_ENCODING
    encode_delta(&current, &previous, buffer, &len);
#else
    memcpy(buffer, &current, sizeof(full_snapshot_t));
    len = sizeof(full_snapshot_t);
#endif

// USEFUL MACROS (you'll see these in production firmware)

// Array element count (safer than hardcoding)
#define ARRAY_SIZE(arr)  (sizeof(arr) / sizeof((arr)[0]))

uint32_t buffer[32];
for (int i = 0; i < ARRAY_SIZE(buffer); i++) { ... }

// MIN / MAX (with type safety via GCC extension)
#define MIN(a, b)  ((a) < (b) ? (a) : (b))
#define MAX(a, b)  ((a) > (b) ? (a) : (b))

// COMPILE-TIME ASSERTIONS (catch errors before running)
#define STATIC_ASSERT(cond, msg)  typedef char static_assert_##msg[(cond) ? 1 : -1]

STATIC_ASSERT(sizeof(task_snapshot_t) == 24, task_snapshot_size_mismatch);
// If the struct size changes (due to padding), this FAILS at compile time!
```

---

## 2.9 CRC — Error Detection for Data Integrity

### What Is CRC?

CRC (Cyclic Redundancy Check) is a checksum algorithm that detects if data was corrupted during transmission. It works by treating the data as a giant binary polynomial and computing the remainder when divided by a generator polynomial.

**You don't need to understand the polynomial math.** You just need to implement it.

### CRC-16-CCITT (Used in RTOSTwin)

```c
// CRC-16-CCITT with polynomial 0x1021
// This is the exact function used in the telemetry agent

uint16_t crc16_ccitt(const uint8_t *data, uint16_t length) {
    uint16_t crc = 0xFFFF;  // Initial value
    
    for (uint16_t i = 0; i < length; i++) {
        crc ^= (uint16_t)data[i] << 8;  // XOR byte into high 8 bits of CRC
        
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x8000) {          // If MSB is set
                crc = (crc << 1) ^ 0x1021;  // Shift left, XOR with polynomial
            } else {
                crc = crc << 1;          // Just shift left
            }
        }
    }
    
    return crc;
}

// USAGE:
full_snapshot_t snapshot;
// ... fill in snapshot data ...

// Calculate CRC over everything EXCEPT the CRC field itself
snapshot.crc16 = crc16_ccitt(
    (const uint8_t *)&snapshot,
    sizeof(full_snapshot_t) - sizeof(uint16_t)  // Exclude CRC field
);

// RECEIVER SIDE: Verify integrity
uint16_t received_crc = received_packet.crc16;
uint16_t computed_crc = crc16_ccitt(
    (const uint8_t *)&received_packet,
    sizeof(full_snapshot_t) - sizeof(uint16_t)
);

if (received_crc != computed_crc) {
    // DATA CORRUPTED! Discard packet.
    error_count++;
}
```

**Properties of CRC-16-CCITT:**
- Detects all single-bit errors
- Detects all double-bit errors
- Detects any odd number of bit errors
- Detects all burst errors up to 16 bits
- Computation cost: ~16 cycles per byte on Cortex-M4

---

## 2.10 Circular Buffers: The Foundation of Embedded Communication

### What Is a Circular Buffer?

A fixed-size array used as a FIFO (First-In, First-Out) queue. When the write pointer reaches the end, it wraps to the beginning.

```
Initial (empty):
  Read ↓
  [ ][ ][ ][ ][ ][ ][ ][ ]
  Write ↓

After 3 writes (A, B, C):
  Read ↓
  [A][B][C][ ][ ][ ][ ][ ]
           Write ↓

After 2 reads:
        Read ↓
  [ ][ ][C][ ][ ][ ][ ][ ]
           Write ↓

After 7 more writes (wrapping!):
        Read ↓
  [K][ ][C][D][E][F][G][H]  ← Write wrapped around!
     Write ↓
```

### Implementation (Lock-Free, ISR-Safe)

```c
#define BUFFER_SIZE  32  // MUST be power of 2 for mask trick

typedef struct {
    uint8_t data[BUFFER_SIZE];
    volatile uint16_t head;  // Write index (producer)
    volatile uint16_t tail;  // Read index (consumer)
} circular_buffer_t;

// Initialize
void cbuf_init(circular_buffer_t *cb) {
    cb->head = 0;
    cb->tail = 0;
}

// Check if buffer is full
static inline bool cbuf_is_full(const circular_buffer_t *cb) {
    return ((cb->head + 1) & (BUFFER_SIZE - 1)) == cb->tail;
    // The & mask trick works because BUFFER_SIZE is a power of 2
    // (head + 1) % 32 == (head + 1) & 31
}

// Check if buffer is empty
static inline bool cbuf_is_empty(const circular_buffer_t *cb) {
    return cb->head == cb->tail;
}

// Push one byte (called by PRODUCER — e.g., snapshot task)
bool cbuf_push(circular_buffer_t *cb, uint8_t byte) {
    if (cbuf_is_full(cb)) {
        return false;  // Buffer full — cannot push
    }
    cb->data[cb->head] = byte;
    cb->head = (cb->head + 1) & (BUFFER_SIZE - 1);  // Wrap around
    return true;
}

// Pop one byte (called by CONSUMER — e.g., DMA transmit ISR)
bool cbuf_pop(circular_buffer_t *cb, uint8_t *byte) {
    if (cbuf_is_empty(cb)) {
        return false;  // Buffer empty — nothing to read
    }
    *byte = cb->data[cb->tail];
    cb->tail = (cb->tail + 1) & (BUFFER_SIZE - 1);  // Wrap around
    return true;
}
```

### Why This Is Lock-Free and ISR-Safe

This circular buffer works without mutexes or disabling interrupts IF:
- **Exactly one producer** writes to `head` (the telemetry task)
- **Exactly one consumer** reads from `tail` (the DMA completion ISR or transmit task)
- `head` and `tail` are `volatile` (compiler won't cache them)
- Writes to `head`/`tail` are atomic (uint16_t writes are atomic on 32-bit ARM)

No locking needed → No priority inversion → No deadline violations. 

### Connection to RTOSTwin

Your transmit queue in the transport layer IS a circular buffer. The snapshot task pushes encoded packets. The transport task (or DMA ISR) pops and sends them over UART.

---

# 3️⃣ Deep Dive (Engineering Depth)

## Common Pitfalls (What Will Bite You)

### Pitfall 1: Integer Overflow

```c
uint8_t counter = 250;
counter += 10;  // counter = 4 (NOT 260! 260 mod 256 = 4)

// DANGEROUS: Timing calculation
uint16_t start = get_tick();     // 65530
// ... time passes ...
uint16_t end = get_tick();       // 5 (wrapped!)
uint16_t elapsed = end - start;  // 5 - 65530 = ??? 

// ACTUALLY: On unsigned arithmetic, this WORKS correctly!
// 5 - 65530 = -65525, but as uint16_t, = 65536 - 65525 = 11 ✓
// This is WHY we use unsigned types for timestamps!
```

### Pitfall 2: Signed vs. Unsigned Comparison

```c
int8_t a = -1;
uint8_t b = 1;

if (a < b) {
    printf("a is less\n");    // You expect this
} else {
    printf("b is less\n");    // THIS RUNS! -1 converted to unsigned = 255
}
// Fix: Compare same types, or cast explicitly.
```

### Pitfall 3: Macro Side Effects

```c
#define MAX(a, b)  ((a) > (b) ? (a) : (b))

int x = 5;
int result = MAX(x++, 3);
// Expands to: ((x++) > (3) ? (x++) : (3))
// x is incremented TWICE if x > 3!
// result = 6 or 7 depending on evaluation order (UNDEFINED BEHAVIOR!)

// FIX: Use GCC statement expressions:
#define MAX_SAFE(a, b) ({ \
    __typeof__(a) _a = (a); \
    __typeof__(b) _b = (b); \
    _a > _b ? _a : _b; \
})
```

### Pitfall 4: `memcmp` on Padded Structs

```c
typedef struct {
    uint8_t  state;     // 1 byte
    // 3 bytes padding here!
    uint32_t stack;     // 4 bytes
} task_t;

task_t a = {1, 100};
task_t b = {1, 100};

// a and b have the SAME field values
// BUT: padding bytes may contain random garbage
// memcmp(&a, &b, sizeof(task_t)) might return NON-ZERO!

// FIX 1: Compare field by field
if (a.state == b.state && a.stack == b.stack) { /* equal */ }

// FIX 2: Zero the struct before use
task_t a;
memset(&a, 0, sizeof(task_t));  // Zero ALL bytes including padding
a.state = 1;
a.stack = 100;
// Now memcmp works because padding is deterministically zero
```

---

# 4️⃣ Practical Firmware Implementation Insights

## How These C Concepts Map to RTOSTwin Agent Code

| C Concept | Where in RTOSTwin | Why |
|-----------|------------------|-----|
| `uint32_t`, `uint8_t` | Every struct field | Portable, exact width |
| `volatile` | Shared counters between ISR and telemetry task | Prevents compiler caching |
| `static` allocation | `snapshot_capture()` internal buffers | No malloc, deterministic |
| Struct packing | Wire protocol packet format | Byte-exact serialization |
| Bitwise OR/AND | Delta encoder `changed_fields` | Efficient change tracking |
| `(1 << N)` | Field bitmasks, GPIO control | Individual bit manipulation |
| CRC-16 | Packet integrity field | Detect UART bit errors |
| Circular buffer | Transmit queue | Lock-free ISR-safe FIFO |
| Pointer arithmetic | Delta encoder output walking | Efficient serialization |
| `#define` / `enum` | Config constants, task states | Zero-cost abstraction |
| `sizeof` | Memory budget calculation, memcpy lengths | Correctness |
| `memset` / `memcmp` | Struct initialization, change detection | Bulk operations |

---

# 5️⃣ Homework

## A. Theoretical Homework (Advanced Level)

**Submission path:** `/homework/theoretical/tier0/module_0_1_answers.md`

### Question 1: Type Width and Overflow

A temperature sensor returns a 12-bit ADC value (0–4095). You store it in a `uint8_t`. What is the maximum value you can represent? If the ADC reads 3000, what value is stored? Show the math.

### Question 2: Memory Layout

Given this struct:
```c
typedef struct {
    uint8_t   flags;        // ?
    uint32_t  timestamp;    // ?
    uint16_t  value;        // ?
    uint8_t   type;         // ?
    uint32_t  sequence;     // ?
} packet_t;
```

a) Calculate `sizeof(packet_t)` assuming default ARM alignment (4-byte boundary).  
b) Draw the memory layout showing padding bytes.  
c) Reorder the fields to minimize padding. What is the new `sizeof`?  
d) If you transmit this struct as raw bytes over UART, what problems might the receiver have?

### Question 3: Bitwise Operations

You have an 8-bit register `STATUS = 0b10110100`. 

a) Which bits are set? (list their positions)  
b) Write a C expression to check if bit 5 is set.  
c) Write a C expression to clear bit 4 without changing other bits.  
d) Write a C expression to toggle bits 7 and 2 simultaneously.  
e) After all three operations (b, c, d), what is the final value of `STATUS`?

### Question 4: CRC Overhead Estimation

The CRC-16 function processes one byte in approximately 16 CPU cycles on a Cortex-M4 at 168 MHz.

a) How many cycles to compute CRC over a 350-byte snapshot?  
b) How many microseconds does that take?  
c) If `snapshot_capture()` takes 85 µs and CRC takes X µs, what is the total? Does it meet the < 150 µs target?  
d) The CRC computation does NOT need to happen inside a critical section (interrupts disabled). Why not?

### Question 5: Circular Buffer Capacity

Your transmit circular buffer has `BUFFER_SIZE = 32` entries, each entry is a `packet_t` of 128 bytes.

a) What is the total RAM consumed by the buffer?  
b) If the producer pushes 10 packets/sec and the consumer pops 8 packets/sec, after how many seconds does the buffer fill up?  
c) What should your code do when the buffer is full? (List at least 3 options and their tradeoffs.)

---

## B. Code Homework

**Submission path:** `/homework/code/tier0/module_0_1/`

### Assignment 1: `types_exercise.c`
Write a program (runs on PC) that:
1. Declares one variable of each `stdint.h` type (`uint8_t` through `uint64_t`, signed and unsigned).
2. Prints `sizeof()` for each.
3. Demonstrates overflow: store 300 in a `uint8_t`, print the result. Store -1 in a `uint8_t`, print the result.
4. Demonstrates signed/unsigned comparison trap: compare `(int8_t)-1` with `(uint8_t)1`. Print which is "larger."

### Assignment 2: `bitwise_exercise.c`
Write a program that:
1. Implements `void print_binary(uint32_t value)` — prints a 32-bit value in binary format with spaces every 4 bits (e.g., `0000 0000 0000 0000 0000 0000 0010 1010`).
2. Implements `set_bit()`, `clear_bit()`, `toggle_bit()`, `check_bit()` functions.
3. Demonstrates: Create a `uint8_t changed_fields = 0`. Set bits 0 and 3. Check if bit 2 is set. Clear bit 0. Print the binary after each operation.

### Assignment 3: `crc16.c`
Implement the CRC-16-CCITT function from section 2.9 above. Test it with:
- Input: `"Hello"` → Expected CRC: `0xD26E` (verify this!)
- Input: 5 bytes of `0x00` → compute and print.
- Input: Empty (length 0) → should return the initial value (`0xFFFF`).

### Assignment 4: `circular_buffer.c`
Implement the full circular buffer from section 2.10 above. Test it with:
1. Push 5 items. Pop 3. Push 4 more. Pop all remaining. Verify order (FIFO).
2. Try pushing when full — verify it returns `false`.
3. Try popping when empty — verify it returns `false`.
4. Fill the buffer completely, pop all, fill again — verify the wrap-around works.

### Assignment 5: `snapshot_struct.c`
Define the `full_snapshot_t` structure hierarchy from the RTOSTwin report:
1. `task_snapshot_t` (name, state, priority, stack_used, stack_total, cpu_time_us)
2. `memory_snapshot_t` (heap_free, heap_total, heap_largest_block, fragment_percent)
3. `health_snapshot_t` (cpu_utilization, temperature, uptime_sec, interrupt_rate, error_count)
4. `full_snapshot_t` (timestamp, tasks[10], memory, health, crc16)

Then:
- Print `sizeof()` for each struct. 
- Create a dummy snapshot with made-up values. 
- Calculate and store the CRC (using your CRC function from Assignment 3). 
- Verify the CRC by recomputing and comparing.

---

# Supplementary Learning

## Recommended Reading

| Resource | What | Why Read It |
|----------|------|-------------|
| **"Making Embedded Systems" by Elecia White, Ch. 2–3** | C for embedded, memory model | Best beginner intro. Written by an actual embedded engineer. |
| **FreeRTOS Coding Standard** (freertos.org) | Style guide | Shows how production RTOS code uses stdint, naming, etc. |
| **MISRA-C:2012 Guidelines** (summary) | Safety-critical C rules | Industry standard for automotive/medical C code. Learn the rules. |
| **STM32F4 Reference Manual (RM0090), Section 2** | Memory map | See the actual addresses used in this module. |
| **Barr Group Embedded C Coding Standard** (free PDF) | Coding rules | Concise, practical style guide for embedded teams. |

---

**END OF MODULE 0.1**
