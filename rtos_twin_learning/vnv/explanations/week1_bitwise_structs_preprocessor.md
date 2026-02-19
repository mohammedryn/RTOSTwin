# VNV — Week 1 Explanation
## Bitwise Operations, Structs, Padding & Preprocessor

**Topic:** C Programming Foundations — Part B (Data & Communication Focus)  
**Study Duration:** 3–4 days  
**Goal:** Master bit manipulation, data structure design, and compile-time configuration — the tools for encoding, decoding, and transmitting data between hardware and software  

---

> [!IMPORTANT]
> After studying this, you will teach RYN these concepts. Prepare to explain:
> 1. Live demo: `print_binary()` showing how bits change with each operation
> 2. The struct padding trap — why `sizeof` gives a bigger number than you expect
> 3. The delta encoder bitmask — how 1 byte tells you which fields changed
> 4. How `#ifdef` lets one codebase run on STM32 AND Teensy

---

# 1️⃣ Motivation

Every piece of hardware in an embedded system is controlled by writing individual **bits** in special registers. The temperature sensor sends data as **bytes** through a structured protocol. The telemetry packet your agent sends is a carefully designed **struct** serialized and checked with a **CRC**.

If you can't think in bits, you can't talk to hardware. If you can't design structs, you can't build protocols. If you can't use the preprocessor, you can't write portable code.

---

# 2️⃣ Bitwise Operations — The Language of Hardware

## The Six Operators

```c
// AND (&) — Masking: extract or clear bits
0b11001010 & 0b00001111 = 0b00001010  // Keep lower 4 bits

// OR (|) — Setting: turn on specific bits
0b00001010 | 0b11000000 = 0b11001010  // Set bits 7 and 6

// XOR (^) — Toggling: flip specific bits
0b00000001 ^ 0b00000001 = 0b00000000  // LED OFF
0b00000000 ^ 0b00000001 = 0b00000001  // LED ON

// NOT (~) — Invert all bits
~0b00100000 = 0b11011111

// LEFT SHIFT (<<) — Create bit position masks
(1 << 0) = 0b00000001  = 1
(1 << 5) = 0b00100000  = 32
(1 << 7) = 0b10000000  = 128

// RIGHT SHIFT (>>) — Extract bit fields
0b11001010 >> 4 = 0b00001100  // Get upper nibble
```

## The Four Patterns (Memorize These)

```c
// 1. SET a bit:     register |= (1 << bit);
// 2. CLEAR a bit:   register &= ~(1 << bit);
// 3. TOGGLE a bit:  register ^= (1 << bit);
// 4. CHECK a bit:   if (register & (1 << bit)) { /* set */ }
```

## Real Example: GPIO Pin Control

```c
// Turn ON LED on pin PA5:
GPIOA->ODR |= (1 << 5);    // SET bit 5

// Turn OFF LED:
GPIOA->ODR &= ~(1 << 5);   // CLEAR bit 5

// Toggle LED:
GPIOA->ODR ^= (1 << 5);    // TOGGLE bit 5

// Check if button on pin PA3 is pressed:
if (GPIOA->IDR & (1 << 3)) {  // CHECK bit 3
    // Button is HIGH
}
```

## RTOSTwin Delta Encoder — Your Design

The delta encoder uses a **bitmask** to track which parts of the snapshot changed:

```c
#define FIELD_TASKS       (1 << 0)    // Bit 0
#define FIELD_MEMORY      (1 << 1)    // Bit 1
#define FIELD_PERIPHERALS (1 << 2)    // Bit 2
#define FIELD_HEALTH      (1 << 3)    // Bit 3

uint8_t changed_fields = 0;           // Start: nothing changed

// Check each section:
if (tasks_changed)   changed_fields |= FIELD_TASKS;       // Set bit 0
if (memory_changed)  changed_fields |= FIELD_MEMORY;      // Set bit 1

// Decoder reads the bitmask:
if (changed_fields & FIELD_TASKS) {
    decode_task_data(packet);     // Only process if tasks changed
}
if (changed_fields & FIELD_MEMORY) {
    decode_memory_data(packet);   // Only process if memory changed
}

// If only tasks and health changed:
// changed_fields = 0b00001001 (bits 0 and 3 set)
// Packet contains ONLY task + health data → 80% smaller than full snapshot
```

## Multi-Bit Fields

Hardware registers often use 2, 3, or 4 bits for a field:

```c
// STM32 GPIO MODER register: 2 bits per pin
// Pin 5 mode at bits [11:10]: 00=Input, 01=Output, 10=AltFunc, 11=Analog

// Set Pin 5 as output (01):
GPIOA->MODER &= ~(0x3 << (5 * 2));    // Clear both bits first
GPIOA->MODER |=  (0x1 << (5 * 2));    // Set to 01 (output)

// Read Pin 5 mode:
uint8_t mode = (GPIOA->MODER >> (5 * 2)) & 0x3;  // Extract 2 bits
```

---

# 3️⃣ Structs — Modeling Data

## Basic Struct

```c
typedef struct {
    char     name[16];       // 16 bytes
    uint8_t  state;          // 1 byte
    uint8_t  priority;       // 1 byte
    uint32_t stack_used;     // 4 bytes
    uint32_t stack_total;    // 4 bytes
    uint32_t cpu_time_us;    // 4 bytes
} task_snapshot_t;           // Total: 30 bytes

// Create and use:
task_snapshot_t task;
strcpy(task.name, "SensorTask");
task.state = 1;   // Running
task.priority = 3;

// Access via pointer:
task_snapshot_t *p = &task;
p->state = 2;     // Blocked (→ is shorthand for (*p).state)
```

## The Padding Trap ⚠️

**The compiler inserts invisible bytes to align fields to their natural boundary:**

```c
typedef struct {
    uint8_t  a;      // 1 byte
    // 3 BYTES PADDING (to align b to 4-byte boundary)
    uint32_t b;      // 4 bytes
    uint16_t c;      // 2 bytes
    // 2 BYTES PADDING (to make struct size multiple of largest alignment)
} mystery_t;

// sizeof(mystery_t) = 12, NOT 7!
```

**Memory layout:**
```
Offset:  [0]  [1]  [2]  [3]  [4]  [5]  [6]  [7]  [8]  [9]  [10] [11]
Content:  a   PAD  PAD  PAD   b    b    b    b    c    c   PAD  PAD
```

### Why Padding Exists

ARM CPUs access 32-bit values most efficiently when they're at addresses divisible by 4. A `uint32_t` at address 0x03 requires two bus reads and a shift — slower and sometimes forbidden.

### Fix 1: Order fields largest → smallest

```c
typedef struct {
    uint32_t b;      // 4 bytes (offset 0 — aligned ✓)
    uint16_t c;      // 2 bytes (offset 4 — aligned ✓)
    uint8_t  a;      // 1 byte  (offset 6)
    // 1 byte padding (to make size = 8, multiple of 4)
} optimized_t;       // sizeof = 8 (was 12!)
```

### Fix 2: `__attribute__((packed))`

```c
typedef struct __attribute__((packed)) {
    uint8_t  a;
    uint32_t b;      // NOT aligned! May be slower or cause faults on some CPUs
    uint16_t c;
} packed_t;          // sizeof = 7 (exact)
```

**Use packed ONLY for wire protocol structs.** For in-memory structs, use natural alignment (fastest).

### Why Padding Breaks `memcmp`

```c
mystery_t x = {1, 100, 50};
mystery_t y = {1, 100, 50};
// x and y have IDENTICAL field values
// BUT: padding bytes may contain random garbage
// memcmp(&x, &y, sizeof(mystery_t)) may return NON-ZERO (different!)

// FIX: memset to zero before use
mystery_t x;
memset(&x, 0, sizeof(x));  // Zero ALL bytes including padding
x.a = 1; x.b = 100; x.c = 50;
// Now memcmp is safe
```

---

# 4️⃣ RTOSTwin Struct Hierarchy

```c
// Layer 1: Individual task state
typedef struct {
    uint32_t cpu_time_us;     // 4 bytes  — largest first!
    uint32_t stack_used;      // 4 bytes
    uint32_t stack_total;     // 4 bytes
    char     name[16];        // 16 bytes
    uint8_t  state;           // 1 byte
    uint8_t  priority;        // 1 byte
    // 2 bytes padding → 32 bytes total
} task_snapshot_t;

// Layer 2: Heap state
typedef struct {
    uint32_t heap_free;           // 4 bytes
    uint32_t heap_total;          // 4 bytes
    uint32_t heap_min_ever_free;  // 4 bytes
    uint16_t fragment_count;      // 2 bytes
    // 2 bytes padding → 16 bytes total
} memory_snapshot_t;

// Layer 3: System health
typedef struct {
    uint32_t uptime_sec;      // 4 bytes
    uint16_t error_count;     // 2 bytes
    int16_t  temperature_C;   // 2 bytes (°C × 10, e.g., 237 = 23.7°C)
    uint8_t  cpu_utilization;  // 1 byte (0-100%)
    // 3 bytes padding → 12 bytes total
} health_snapshot_t;

// Layer 4: Complete snapshot
typedef struct {
    uint64_t          timestamp_us;         // 8 bytes
    task_snapshot_t   tasks[MAX_TASKS];     // 32 × 10 = 320 bytes
    memory_snapshot_t memory;               // 16 bytes
    health_snapshot_t health;               // 12 bytes
    uint16_t          crc16;                // 2 bytes
    // padding → ~360 bytes total
} full_snapshot_t;
```

**Budget check:** ~360 bytes × 2 (current + previous for delta) = ~720 bytes. Well within our 10 KB agent budget. ✓

---

# 5️⃣ Enums & `#define`

```c
// #define — Zero runtime cost (text substitution at compile time)
#define MAX_TASKS       10
#define FIELD_TASKS     (1 << 0)
#define FIELD_MEMORY    (1 << 1)

// enum — Self-documenting, debugger-friendly
typedef enum {
    TASK_STATE_READY     = 0,
    TASK_STATE_RUNNING   = 1,
    TASK_STATE_BLOCKED   = 2,
    TASK_STATE_SUSPENDED = 3,
    TASK_STATE_COUNT             // = 4, useful for array sizing
} task_state_t;

// Use: task.state = TASK_STATE_RUNNING;
```

**Recommendation:**
- `#define` for bit masks, register offsets, sizes
- `enum` for named states and categories

---

# 6️⃣ Preprocessor — Compile-Time Configuration

```c
// Platform abstraction:
#ifdef TARGET_STM32F4
    #include "stm32f4xx_hal.h"
    #define LED_PORT  GPIOA
    #define LED_PIN   GPIO_PIN_5
#elif defined(TARGET_TEENSY41)
    #include "core_pins.h"
    #define LED_PIN   13
#else
    #error "No target platform defined!"
#endif

// Feature flags:
#define ENABLE_DELTA_ENCODING  1

#if ENABLE_DELTA_ENCODING
    encode_delta(&current, &previous, buffer, &len);
#else
    memcpy(buffer, &current, sizeof(current));
#endif

// Useful macros:
#define ARRAY_SIZE(arr)  (sizeof(arr) / sizeof((arr)[0]))
#define MIN(a, b)        ((a) < (b) ? (a) : (b))
#define MAX(a, b)        ((a) > (b) ? (a) : (b))

// Compile-time assertion:
#define STATIC_ASSERT(cond, msg) typedef char static_assert_##msg[(cond)?1:-1]
STATIC_ASSERT(sizeof(task_snapshot_t) <= 64, task_struct_too_large);
```

---

# 7️⃣ CRC-16 — Error Detection

```c
uint16_t crc16_ccitt(const uint8_t *data, uint16_t length) {
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < length; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x8000)
                crc = (crc << 1) ^ 0x1021;
            else
                crc <<= 1;
        }
    }
    return crc;
}

// Usage: compute CRC over snapshot, store in crc16 field
snapshot.crc16 = crc16_ccitt((uint8_t*)&snapshot, 
                             sizeof(snapshot) - sizeof(uint16_t));
```

Detects all single-bit errors, all burst errors up to 16 bits. Cost: ~16 cycles/byte.

---

# 📝 Homework for VNV

## Theoretical
1. Given `STATUS = 0b10110100`: which bits are set? Write C to clear bit 4 and toggle bit 7. What's the final value?
2. Draw the memory layout of `mystery_t` (uint8_t, uint32_t, uint16_t) byte-by-byte. Show padding.
3. If `changed_fields = 0b00001011`, which RTOSTwin sections changed? (TASKS, MEMORY, PERIPHERALS, HEALTH)

## Code
- [ ] Complete `bitwise_exercise.c` — implement `print_binary`, `set_bit`, `clear_bit`, `toggle_bit`, `check_bit`
- [ ] Complete `snapshot_struct.c` — define all structs, print sizeof, verify CRC
- [ ] Complete `crc16.c` — implement CRC-16-CCITT, verify with test vectors

---

**After completing this module, explain to RYN:**
- Live demo of `print_binary()` showing bits change with each operation
- The struct padding trap with two differently-ordered structs
- The delta encoder bitmask: how 1 byte saves 80% bandwidth

---

**END OF VNV WEEK 1 EXPLANATION**
