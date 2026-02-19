# RYN — Week 1 Explanation
## C Types, Memory Layout, Pointers & Static Allocation

**Topic:** C Programming Foundations — Part A (Systems & Architecture Focus)  
**Study Duration:** 3–4 days  
**Goal:** Master the type system, memory model, and pointer mechanics that underpin all embedded firmware  

---

> [!IMPORTANT]
> After studying this, you will teach VNV these concepts. Prepare to explain:
> 1. Why `int` is dangerous and `uint32_t` is safe
> 2. Draw the stack vs heap diagram from memory
> 3. Why `volatile` is life-or-death in embedded
> 4. Why we NEVER use `malloc` in the agent hot path

---

# 1️⃣ Motivation

You're building firmware that runs on a microcontroller with **192 KB of RAM**. Every byte matters. Every type decision has consequences. A wrong type can overflow silently and produce incorrect sensor readings. A missing `volatile` can cause an infinite loop. A careless `malloc` can fragment memory and crash the system hours later.

Desktop programmers never worry about this — their OS gives them gigabytes of virtual memory. **You don't have an OS.** You ARE the OS.

---

# 2️⃣ Fixed-Width Types (`stdint.h`)

## The Problem

```c
int x = 70000;
// PC (32-bit int): x = 70000 ✓
// MSP430 (16-bit int): x = 4464 ✗ (70000 mod 65536)
```

Your code "works" on your PC, ships to production, and **silently computes wrong values**.

## The Solution

```c
#include <stdint.h>

uint8_t   flags;           // Exactly 8 bits.  0 to 255
uint16_t  adc_value;       // Exactly 16 bits. 0 to 65,535
uint32_t  timestamp;       // Exactly 32 bits. 0 to 4.29 billion
uint64_t  timestamp_us;    // Exactly 64 bits. Microsecond timestamps
int8_t    temp_offset;     // Signed 8-bit.    -128 to +127
int16_t   temperature;     // Signed 16-bit.   -32,768 to +32,767
int32_t   error_value;     // Signed 32-bit.   ±2.14 billion
```

## Complete Type Reference

| Type | Bytes | Range | Use In RTOSTwin |
|------|-------|-------|----------------|
| `uint8_t` | 1 | 0–255 | Task state, flags, register bytes |
| `int8_t` | 1 | -128 to +127 | Small signed offsets |
| `uint16_t` | 2 | 0–65,535 | ADC readings, CRC, counters |
| `int16_t` | 2 | ±32,767 | Temperature (°C × 10) |
| `uint32_t` | 4 | 0–4.29B | Addresses, tick counts, stack sizes |
| `int32_t` | 4 | ±2.14B | Time differences |
| `uint64_t` | 8 | 0–18.4 quintillion | Microsecond timestamps |

## `sizeof` — Always Verify

```c
printf("uint8_t:  %zu bytes\n", sizeof(uint8_t));   // 1
printf("uint16_t: %zu bytes\n", sizeof(uint16_t));  // 2
printf("uint32_t: %zu bytes\n", sizeof(uint32_t));  // 4
printf("int:      %zu bytes\n", sizeof(int));        // 2 or 4 — DANGER
printf("long:     %zu bytes\n", sizeof(long));       // 4 or 8 — DANGER
```

## Overflow

```c
uint8_t x = 250;
x += 10;  // x = 4, NOT 260! (260 mod 256 = 4)

// This is USEFUL for timestamps:
uint16_t start = 65530;  // Near wrap
uint16_t end = 5;         // Wrapped
uint16_t elapsed = end - start;  // = 11 ✓ (unsigned arithmetic wraps correctly)
```

## Signed/Unsigned Trap

```c
int8_t  a = -1;
uint8_t b = 1;
if (a < b) { /* You expect this */ }
else { /* THIS runs! -1 → unsigned → 255 > 1 */ }
```

**Rule:** Never compare signed and unsigned types directly.

---

# 3️⃣ Memory Layout

## The Four Sections

```
┌────────────────────────┐  FLASH (1 MB) — survives power off
│  .text    (your code)  │  Machine instructions from your .c files
│  .rodata  (constants)  │  const char msg[] = "Hello";
├════════════════════════┤  ← Flash / RAM boundary
│  .data    (init'd)     │  int counter = 42;  (copied from Flash at boot)
│  .bss     (uninit'd)   │  static uint32_t buf[100];  (zeroed at boot)
├────────────────────────┤  RAM (192 KB)
│  Heap     ↑            │  malloc/pvPortMalloc grows upward
│  (free)                │  ← if these meet = CRASH
│  Stack    ↓            │  Local variables, return addresses grow downward
└────────────────────────┘
```

## RAM Budget (STM32F4)

```
Total RAM: 192 KB = 196,608 bytes

.data + .bss:            ~7 KB
FreeRTOS heap:          ~80 KB
  ├─ Task stacks:       ~40 KB (8 tasks × ~2-4 KB each)
  ├─ Queues/timers:      ~5 KB
  └─ Free heap:         ~35 KB ← this is what heap_free measures
RTOSTwin agent:         <10 KB ← OUR BUDGET
  ├─ Snapshot buffers:   ~1 KB
  ├─ TX queue:           ~4 KB
  └─ Misc:               ~5 KB
System stack (ISR):      ~2 KB
───────────────────────────
Used:                   ~99 KB
Remaining:              ~97 KB (margin)
```

---

# 4️⃣ Pointers & `volatile`

## Pointers as Hardware Addresses

```c
// Desktop: pointer is abstract
int x = 42;
int *p = &x;

// Embedded: pointer IS the hardware
volatile uint32_t *GPIOA_ODR = (volatile uint32_t *)0x40020014;
*GPIOA_ODR |= (1 << 5);  // PHYSICALLY turns on LED
```

Writing to `0x40020014` sends an electrical signal through the bus to the GPIO peripheral, which changes the voltage on a physical pin. **Pointers are wires.**

## `volatile` — Mandatory for Hardware

```c
// WITHOUT volatile — BROKEN:
uint32_t *status = (uint32_t *)0x40001000;
while ((*status & (1 << 5)) == 0) { }  // Compiler caches the read → infinite loop

// WITH volatile — CORRECT:
volatile uint32_t *status = (volatile uint32_t *)0x40001000;
while ((*status & (1 << 5)) == 0) { }  // Re-reads every iteration ✓
```

**Use `volatile` for:**
- Hardware registers
- Variables shared between ISR and main code
- Variables shared between RTOS tasks
- DMA buffers

## Pointer Arithmetic

```c
uint32_t buf[10];
uint32_t *p = buf;      // p → buf[0]
p + 1;                   // → buf[1] (advances by sizeof(uint32_t) = 4 bytes)
*(p + 3) == buf[3];      // Same thing
```

---

# 5️⃣ Static vs Dynamic Allocation

```c
// DYNAMIC (dangerous in embedded):
int *buf = malloc(100 * sizeof(int));  // Can fail, fragments, needs mutex
free(buf);                               // Can double-free, use-after-free

// STATIC (safe, deterministic):
static int buf[100];  // Always available. Zero overhead. Never fails.
```

**Why `malloc` is forbidden in the agent hot path:**
1. **Non-deterministic:** Time to allocate varies (searches free list)
2. **Fragmentation:** Repeated alloc/free creates unusable holes
3. **Failure:** Returns NULL if no memory — most code doesn't check
4. **Mutex:** FreeRTOS heap uses a mutex → priority inversion risk
5. **Determinism:** Static memory is known at compile time → verifiable

```c
// RTOSTwin snapshot capture — correct approach:
void snapshot_capture(full_snapshot_t *out) {
    static TaskStatus_t task_buf[MAX_TASKS];  // In .bss, not stack, not heap
    UBaseType_t n = uxTaskGetSystemState(task_buf, MAX_TASKS, NULL);
    // ...
}
```

## The 3 Meanings of `static`

```c
// 1. File-scope (private to this .c file):
static uint32_t counter = 0;

// 2. Persistent local (survives function returns):
void foo(void) {
    static int call_count = 0;
    call_count++;  // Remembers value between calls
}

// 3. Static buffer (compile-time allocation):
static uint8_t buffer[256];  // Lives in .bss forever
```

---

# 6️⃣ Fixed-Point Arithmetic

When you have no FPU (Cortex-M0/M3), or want deterministic timing:

```c
// Float: 23.7°C
float temp = 23.7f;  // ~14 cycles on M4 FPU, ~200 cycles on M0

// Fixed-point (×10): 237 represents 23.7°C
int16_t temp_fixed = 237;  // 1 cycle. Always.
// Display: printf("Temp: %d.%d C", temp_fixed/10, temp_fixed%10);
```

---

# 📝 Homework for RYN

## Theoretical
1. If `uint8_t x = 200` and `int8_t y = (int8_t)x`, what is `y`? Show the two's complement math.
2. Draw the STM32F4 RAM layout from memory (stack, heap, .bss, .data). Label addresses.
3. Explain in your own words why `volatile` prevents the compiler from optimizing away hardware reads.

## Code
- [ ] Complete `types_exercise.c` from the shared homework folder
- [ ] Write a function `void print_memory_map(void)` that prints the address of a global, a static local, a stack variable, and a heap variable (on PC). Observe which addresses are close together.

---

**After completing this module, explain to VNV:**
- The `uint8_t` overflow demo (300 → 44)
- Draw the memory layout on a whiteboard/paper
- Show why `volatile` matters with a concrete ISR example

---

**END OF RYN WEEK 1 EXPLANATION**
