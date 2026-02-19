# 📋 VNV's Complete Learning Timeline
## RTOSTwin — Individual Study Track (Ultra-Detailed Edition)

**Study Partner:** RYN  
**Method:** Solo study for 3–4 days → Teach each other for 3–4 days  
**Your Track:** Data, Peripherals & Communication (bit manipulation, data encoding, sensors, protocols, UI)  

---

> [!TIP]
> **Your identity in this project:** You are the **data person**. You understand how to structure, encode, verify, and visualize data — how bits become structs, structs become packets, packets become dashboards. RYN is the **systems person** — he understands what happens inside the CPU. Together you cover the full stack.

---

## Master Schedule

| Week | VNV Solo Topics | RYN Solo Topics | Teach Session Focus |
|------|----------------|----------------|-------------------|
| 1 | Bitwise Ops, Structs, Padding, Preprocessor, CRC | C Types, Memory Model, Pointers, volatile, static | Full C foundations |
| 2 | Build Systems, Toolchain, CubeIDE Setup, Makefiles | Number Systems, Two's Complement, Memory Map | Dev environment + Binary thinking |
| 3 | GPIO Modes, Registers, HAL vs Bare Metal, Debouncing | ARM Cortex-M4 Architecture, Registers, Bus, Clocks | First I/O + Hardware model |
| 4 | Timers, SysTick, PWM, DWT Cycle Counter, Watchdog | Interrupts, NVIC, Vector Table, Critical Sections | Timing + Real-time events |
| 5 | SPI Protocol, I2C Protocol, Sensor Communication | UART Protocol, Baud Rate, Polling/IRQ/DMA modes | All serial protocols |
| 6 | ADC Fundamentals, Sampling, DMA-driven ADC | DMA Engine, Channels, Circular Mode, UART+DMA | Analog I/O + Zero-copy |
| 7 | FreeRTOS Queues, xQueueSend/Receive, ISR Queues | Why RTOS, Super-loop vs Preemptive, Task Creation | RTOS core mechanics |
| 8 | FreeRTOS Heap (1-5), pvPortMalloc, Memory Pools | Semaphores, Mutexes, Priority Inversion, Deadlock | Memory + Synchronization |
| 9 | Stack Overflow Detection, Debugging, Trace Hooks | CPU Usage Measurement, Idle Hook, Runtime Stats | RTOS diagnostics |
| 10 | Delta Encoder: Bitmask, Change Detection, Keyframes | Snapshot Engine: snapshot_capture(), Critical Path | RTOSTwin Agent Core |
| 11 | PC Receiver: Serial Decode, State Reconstruction | Transport Layer: Packet Framing, CRC, Seq Numbers | End-to-end telemetry |
| 12 | Dashboard v1: React, WebSocket, Task Timeline Chart | Twin State Manager (C++), Ring Buffer, Thread Safety | Host-side twin |

---

# WEEK 1 — Bitwise Operations, Structs, Padding, Preprocessor, CRC

## Day 1: Bitwise Operations — The Language of Hardware (3–4 hours)

### Topics to Cover

**1. The Six Bitwise Operators (60 min)**

| Operator | Symbol | What It Does | Hardware Use |
|----------|--------|-------------|-------------|
| AND | `&` | Both bits must be 1 → 1 | Masking: extract bits, clear bits |
| OR | `\|` | Either bit is 1 → 1 | Setting: turn bits ON |
| XOR | `^` | Bits differ → 1 | Toggling: flip bits, checksums |
| NOT | `~` | Invert all bits | Creating clear masks |
| Left Shift | `<<` | Move bits left, fill with 0 | Multiply by 2^n, create bit positions |
| Right Shift | `>>` | Move bits right | Divide by 2^n, extract upper bits |

- Work through each operator with two 8-bit binary numbers by hand on paper
- AND truth table: `0&0=0, 0&1=0, 1&0=0, 1&1=1`
- XOR truth table: `0^0=0, 0^1=1, 1^0=1, 1^1=0` (same=0, different=1)
- **Exercise:** Compute by hand: `0b11001010 & 0b00001111`, `0b00001010 | 0b11000000`, `0b10110100 ^ 0b10000100`, `~0b00100000` (8-bit). Verify with a C program.

**2. The Four Sacred Patterns — Memorize These Forever (45 min)**
```c
// 1. SET bit n:     register |= (1 << n);
//    Example: GPIOA->ODR |= (1 << 5);    // Turn ON LED on pin PA5

// 2. CLEAR bit n:   register &= ~(1 << n);
//    Example: GPIOA->ODR &= ~(1 << 5);   // Turn OFF LED

// 3. TOGGLE bit n:  register ^= (1 << n);
//    Example: GPIOA->ODR ^= (1 << 5);    // Flip LED state

// 4. CHECK bit n:   if (register & (1 << n))
//    Example: if (GPIOA->IDR & (1 << 3)) { /* button pressed */ }
```
- Write each pattern 5 times from memory without looking
- **Why `~(1 << n)` for clear?** `(1 << 5)` = `0b00100000`. `~` inverts = `0b11011111`. AND with this preserves all bits EXCEPT bit 5. This is the ONLY safe way to clear one bit.
- **Exercise:** Starting from `reg = 0x00`: set bits 0, 3, 7. Print binary after each. Clear bit 3. Toggle bit 0 twice (should return to original). Check each bit 0-7 and print which are set.

**3. Multi-Bit Fields (60 min)**
- Many hardware registers use 2, 3, or 4 bits for one setting
- STM32 GPIO MODER: 2 bits per pin (00=Input, 01=Output, 10=AltFunc, 11=Analog)
- **Read a field:** `value = (reg >> start_bit) & mask;` where `mask = (1 << width) - 1`
- **Write a field:** `reg &= ~(mask << start); reg |= (value << start);`
- Example: Set pin 5 to output mode (01) in MODER:
  ```c
  GPIOA->MODER &= ~(0x3 << (5 * 2));    // Clear bits [11:10] → 00
  GPIOA->MODER |=  (0x1 << (5 * 2));    // Set to 01 (output)
  ```
- **Exercise:** Write `uint32_t read_field(uint32_t reg, uint8_t start, uint8_t width)` and `uint32_t write_field(uint32_t reg, uint8_t start, uint8_t width, uint32_t value)`. Test: insert value 0b10 at bits [7:6] of `0x00000000`, then extract it back.

**4. Implement `print_binary()` — Your #1 Debug Tool (30 min)**
```c
void print_binary(uint32_t value) {
    for (int i = 31; i >= 0; i--) {
        printf("%d", (value >> i) & 1);
        if (i % 4 == 0 && i != 0) printf(" ");
    }
}
// Output: "0000 0000 0000 0000 0000 0000 0010 1010"
```
- You will use this function in EVERY future exercise to visualize register changes
- **Exercise:** Print binary of: 0, 1, 42, 255, 0xDEAD, 0xFFFFFFFF, (1 << 31)

### Deliverables for Day 1
- [ ] All 6 operators computed by hand (8 examples)
- [ ] 4 sacred patterns written from memory 5 times
- [ ] `print_binary()` implemented and tested
- [ ] `read_field()` / `write_field()` utility functions working

---

## Day 2: Structs, Padding & Memory Layout (3–4 hours)

### Topics to Cover

**1. Struct Basics (45 min)**
- `struct` groups related data into a single unit
- Access: `task.state = 1;` (direct) or `ptr->state = 1;` (pointer, shorthand for `(*ptr).state`)
- `typedef struct { ... } name_t;` — defines a type you can use like `int`
- Nested structs: `full_snapshot_t` contains `task_snapshot_t tasks[10]`, `memory_snapshot_t memory`, etc.
- **Exercise:** Define a `student_t` struct with name[32], age (uint8), gpa (uint16, ×100 scale), id (uint32). Create 3 instances, fill data, print each.

**2. The Padding Trap — Why `sizeof` Lies (60 min)**

ARM CPUs access memory most efficiently when values sit at addresses divisible by their size:
- `uint32_t` (4 bytes) → address must be divisible by 4
- `uint16_t` (2 bytes) → address must be divisible by 2
- `uint8_t` (1 byte) → any address is fine

The compiler inserts invisible **padding bytes** to ensure this alignment:

```c
typedef struct {
    uint8_t  a;        // offset 0:  1 byte
                       // offset 1-3: 3 BYTES PADDING (align b to ÷4)
    uint32_t b;        // offset 4:  4 bytes
    uint16_t c;        // offset 8:  2 bytes
                       // offset 10-11: 2 BYTES PADDING (struct size must be ÷4)
} bad_t;               // sizeof = 12 (fields only use 7 bytes!)
```

Draw this on paper byte-by-byte:
```
Offset: [0]  [1]  [2]  [3]  [4]  [5]  [6]  [7]  [8]  [9]  [10] [11]
         a   PAD  PAD  PAD   b    b    b    b    c    c   PAD  PAD
```

**Fix — order fields largest → smallest:**
```c
typedef struct {
    uint32_t b;        // offset 0:  4 bytes (aligned ✓)
    uint16_t c;        // offset 4:  2 bytes (aligned ✓)
    uint8_t  a;        // offset 6:  1 byte
                       // offset 7:  1 byte padding (struct ÷ 4)
} good_t;              // sizeof = 8 (saved 4 bytes = 33% reduction!)
```

- **Exercise:** For each struct below, draw the byte-by-byte layout with padding, calculate sizeof, then reorder to minimize:
  1. `{uint8_t a, uint32_t b, uint8_t c}` — predict sizeof before running
  2. `{uint16_t x, uint8_t y, uint32_t z, uint8_t w}` — predict sizeof
  3. `{char name[5], uint32_t id, uint16_t score}` — predict sizeof (tricky!)

**3. `__attribute__((packed))` — Wire Protocol Structs (30 min)**
```c
typedef struct __attribute__((packed)) {
    uint8_t  sync1;     // 0xAA
    uint8_t  sync2;     // 0x55
    uint8_t  type;      // packet type
    uint8_t  seq;       // sequence number
    uint16_t length;    // payload length
} packet_header_t;      // sizeof = 6 (exact, no padding)
```
- Packed removes ALL padding — fields may be unaligned
- Unaligned access is slower on ARM (may even fault on Cortex-M0!)
- **Rule:** Use packed ONLY for structs that go over the wire (UART packets). Use natural alignment for in-memory structs.
- **Exercise:** Create both packed and unpacked versions of the same struct. Print sizeof of each. Compare.

**4. The `memcmp` Trap (30 min)**
```c
bad_t x = {1, 100, 50};
bad_t y = {1, 100, 50};
// Fields are IDENTICAL, but padding bytes contain random garbage
// memcmp(&x, &y, sizeof(bad_t)) → may return NON-ZERO!

// FIX: Always memset before use
bad_t x;
memset(&x, 0, sizeof(x));   // Zero ALL bytes including padding
x.a = 1; x.b = 100; x.c = 50;
// Now padding is 0x00, memcmp works correctly
```
- RTOSTwin delta encoder uses `memcmp` to detect changes between snapshots. Without `memset`, it reports false changes every single time.
- **Exercise:** Create two structs with identical field values but WITHOUT memset. Run `memcmp`. Then repeat WITH memset. Verify the difference.

### Deliverables for Day 2
- [ ] 3 struct padding analyses drawn by hand (with verification by sizeof)
- [ ] Packed vs unpacked comparison program
- [ ] memcmp trap demonstration
- [ ] Written rule: "Order fields largest → smallest, memset before use"

---

## Day 3: Preprocessor, Enums & CRC-16 (3–4 hours)

### Topics to Cover

**1. `#define` — Compile-Time Constants (30 min)**
- `#define MAX_TASKS 10` — text substitution, zero runtime cost
- `#define FIELD_TASKS (1 << 0)` — bit position constants for the delta encoder
- `#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))` — useful macro
- **Danger:** No type checking. `#define PI 3.14` treats PI as a `double` everywhere.
- Prefer `static const uint32_t MAX_TASKS = 10;` when type safety matters (but `#define` for bit masks and array sizes).
- **Exercise:** Define macros for: `MIN(a,b)`, `MAX(a,b)`, `CLAMP(x, lo, hi)`, `BIT(n)` (expands to `(1U << n)`). Test each with 5 cases.

**2. Conditional Compilation — `#ifdef` (45 min)**
```c
// Platform abstraction — ONE codebase, multiple targets:
#ifdef TARGET_STM32F4
    #include "stm32f4xx_hal.h"
    #define LED_PORT  GPIOA
    #define LED_PIN   GPIO_PIN_5
    #define UART_HANDLE huart2
#elif defined(TARGET_TEENSY41)
    #include "core_pins.h"
    #define LED_PIN   13
#else
    #error "Define TARGET_STM32F4 or TARGET_TEENSY41!"
#endif

// Feature flags — enable/disable at compile time:
#define ENABLE_DELTA_ENCODING  1
#define ENABLE_CRC_CHECK       1
#define DEBUG_PRINT_SNAPSHOTS  0   // Set to 1 during development

#if ENABLE_DELTA_ENCODING
    encode_delta(&current, &previous, buffer, &len);
#else
    memcpy(buffer, &current, sizeof(current));
    len = sizeof(current);
#endif
```
- Compile with `-DTARGET_STM32F4` flag to define the target from the command line
- **Exercise:** Write a program with `#ifdef DEBUG` that prints extra info. Compile once with `gcc -DDEBUG` and once without. Verify different output.

**3. Compile-Time Assertions (15 min)**
```c
// Catch struct size bugs at COMPILE TIME, not runtime:
#define STATIC_ASSERT(cond, msg) typedef char static_assert_##msg[(cond)?1:-1]

STATIC_ASSERT(sizeof(task_snapshot_t) <= 64, task_snapshot_too_large);
STATIC_ASSERT(sizeof(full_snapshot_t) * 2 <= 10240, snapshot_exceeds_budget);
```
- If the condition is false, the compiler emits a cryptic error about a negative-size array — but it STOPS the build, which is the point.
- **Exercise:** Add static assertions to your RTOSTwin structs. Try making a struct too big and observe the compile error.

**4. Enums — Named States (30 min)**
```c
typedef enum {
    TASK_STATE_READY     = 0,
    TASK_STATE_RUNNING   = 1,
    TASK_STATE_BLOCKED   = 2,
    TASK_STATE_SUSPENDED = 3,
    TASK_STATE_COUNT            // = 4, useful for array sizing and validation
} task_state_t;

// Use: snapshot.tasks[0].state = TASK_STATE_RUNNING;
// Validate: if (state >= TASK_STATE_COUNT) { /* invalid! */ }
```
- Enums give names to numbers — makes code self-documenting
- The debugger shows `TASK_STATE_RUNNING` instead of meaningless `1`
- **Rule:** Use `enum` for states/categories, `#define` for bit masks and sizes
- **Exercise:** Define enums for: packet types (FULL_SNAPSHOT, DELTA, ACK, HEARTBEAT, NACK), error codes (OK, CRC_FAIL, TIMEOUT, OVERFLOW), and GPIO modes (INPUT, OUTPUT, ALT_FUNC, ANALOG).

**5. CRC-16-CCITT Implementation (60 min)**

CRC (Cyclic Redundancy Check) detects data corruption in transit. RTOSTwin appends CRC-16 to every telemetry packet.

Algorithm step-by-step:
```
1. Initialize CRC = 0xFFFF
2. For each byte in data:
   a. XOR the byte into the UPPER 8 bits of CRC:
      crc ^= (uint16_t)byte << 8
   b. Process 8 bits (one bit at a time):
      - If MSB (bit 15) of crc is 1:
          crc = (crc << 1) XOR 0x1021
      - Else:
          crc = crc << 1
3. Return final 16-bit CRC
```

The polynomial `0x1021` = x^16 + x^12 + x^5 + 1 (CRC-16-CCITT standard). Detects:
- ALL single-bit errors
- ALL burst errors up to 16 bits long
- 99.998% of all random errors

- **Exercise:** Implement `crc16_ccitt()`. Test with:
  - `"Hello"` → compute CRC, note value
  - Empty data → should return 0xFFFF (initial value)
  - Append CRC to data → recompute over data+CRC → result should be 0 (residue property)
  - Flip one bit in data → CRC changes (corruption detected!)

**6. CRC in RTOSTwin Packets (30 min)**
```c
// Sender: compute CRC over everything except the CRC field itself
snapshot.crc16 = crc16_ccitt(
    (const uint8_t *)&snapshot,
    sizeof(full_snapshot_t) - sizeof(uint16_t)  // exclude crc16 field
);

// Receiver: recompute and compare
uint16_t check = crc16_ccitt(
    (const uint8_t *)&received,
    sizeof(full_snapshot_t) - sizeof(uint16_t)
);
if (check != received.crc16) {
    error_count++;  // Packet corrupted!
    return;         // Discard
}
```
- **Exercise:** Create a full_snapshot_t, fill with data, compute CRC, "transmit" (memcpy to another buffer), verify CRC on the "receiver" side. Then corrupt one byte and verify detection.

### Deliverables for Day 3
- [ ] Macro library: MIN, MAX, CLAMP, BIT, ARRAY_SIZE
- [ ] Conditional compilation demo (DEBUG flag)
- [ ] Enums for 3 categories (task states, packet types, error codes)
- [ ] Working CRC-16 with corruption detection test

---

## Day 4: RTOSTwin Structs + Delta Bitmask + Review (3 hours)

**1. Build the Complete RTOSTwin Struct Hierarchy (60 min)**

Define all 4 structs with optimal field ordering:
```c
task_snapshot_t    → ~32 bytes × 10 tasks = 320 bytes
memory_snapshot_t  → ~16 bytes
health_snapshot_t  → ~12 bytes
full_snapshot_t    → ~360 bytes total (timestamp + tasks + memory + health + crc)
```
- Verify: `sizeof(full_snapshot_t) * 2 < 10240` (need two copies for delta comparison)
- Fill a snapshot with realistic test data (3+ tasks, memory stats, health metrics)
- Compute CRC, verify integrity

**2. Delta Encoder Bitmask Design (45 min)**
```c
#define FIELD_TASKS       (1 << 0)    // Bit 0: tasks[] changed
#define FIELD_MEMORY      (1 << 1)    // Bit 1: memory changed
#define FIELD_PERIPHERALS (1 << 2)    // Bit 2: peripherals changed
#define FIELD_HEALTH      (1 << 3)    // Bit 3: health changed

uint8_t detect_changes(const full_snapshot_t *curr, const full_snapshot_t *prev) {
    uint8_t changed = 0;
    if (memcmp(&curr->tasks, &prev->tasks, sizeof(curr->tasks)) != 0)
        changed |= FIELD_TASKS;
    if (memcmp(&curr->memory, &prev->memory, sizeof(curr->memory)) != 0)
        changed |= FIELD_MEMORY;
    if (memcmp(&curr->health, &prev->health, sizeof(curr->health)) != 0)
        changed |= FIELD_HEALTH;
    return changed;
}
```
- If only memory changed: send 1 byte (bitmask) + 16 bytes (memory) + 2 bytes (CRC) = 19 bytes instead of 360. **95% compression!**
- Keyframe: send full snapshot every 10th packet for resynchronization
- **Exercise:** Implement `detect_changes()`. Create two snapshots where only the memory section differs. Verify only FIELD_MEMORY bit is set. Then create one where tasks AND health changed. Verify both bits are set.

**3. Teach Prep (60 min)**

Prepare 3 live demos for RYN:

| Demo | What to Show | Key Takeaway |
|------|-------------|-------------|
| **print_binary** | `set_bit`, `clear_bit`, `toggle_bit` with binary output after each | Bits are physical wires on real hardware |
| **Padding trap** | Same fields, different order → different `sizeof` | Field order matters for RAM budget |
| **Delta bitmask** | 360 bytes → 19 bytes with one changed section | 1 byte saves 95% bandwidth |

- [ ] Self-test: Write the 4 bit patterns from memory (no notes)
- [ ] Self-test: Draw padding for `{uint8_t, uint32_t, uint16_t}` from memory
- [ ] Complete `vnv_bitwise_and_structs.c`
- [ ] Fill in `week1_theory_answers.md`

---

# WEEK 2 — Build Systems, Toolchain, CubeIDE, Makefiles

## Day 1: The Compilation Pipeline (3 hours)

**1. From .c to Binary — 4 Stages (60 min)**
```
source.c → [Preprocessor] → source.i  (expanded macros, includes)
source.i → [Compiler]     → source.s  (assembly language)
source.s → [Assembler]    → source.o  (machine code, relocatable)
*.o      → [Linker]       → firmware.elf (final binary with addresses resolved)
firmware.elf → [objcopy]  → firmware.bin (raw flash image)
```
- **Preprocessor:** Expands `#include`, `#define`, `#ifdef`. You can see output: `gcc -E source.c -o source.i`
- **Compiler:** Translates C to assembly. Optimizes. See output: `gcc -S source.c -o source.s`
- **Assembler:** Converts assembly mnemonics to binary machine code
- **Linker:** Combines all `.o` files, resolves function addresses, applies linker script
- **Exercise:** Run each stage manually on a simple "Hello World". Examine each intermediate file. Count how many lines the preprocessor adds from `#include <stdio.h>` alone.

**2. Cross-Compilation (45 min)**
- Your PC runs x86. Your STM32 runs ARM. You can't run PC-compiled code on STM32.
- `gcc` → compiles for YOUR machine (x86 PC)
- `arm-none-eabi-gcc` → compiles for ARM Cortex-M (no OS, no standard library)
  - `arm` = ARM architecture
  - `none` = no operating system (bare metal)
  - `eabi` = Embedded Application Binary Interface
- Key flags:
  ```
  -mcpu=cortex-m4          # Target CPU
  -mthumb                  # Use Thumb-2 instruction set
  -mfloat-abi=hard         # Use hardware FPU
  -mfpu=fpv4-sp-d16        # Single-precision FPU variant
  -Wall -Wextra            # Enable all warnings
  -Os                      # Optimize for size (common for flash-limited MCUs)
  -O2                      # Optimize for speed (when code size permits)
  -g                       # Include debug symbols
  --specs=nosys.specs      # Don't link system calls (no OS!)
  -T stm32f407.ld          # Use this linker script
  ```
- **Exercise:** Install `arm-none-eabi-gcc`. Compile a simple C file with it. Try to run the output on your PC — it won't work (wrong architecture). This is the proof you're cross-compiling.

**3. Understanding Compiler Output Sizes (45 min)**
```bash
arm-none-eabi-size firmware.elf
#   text    data     bss     dec     hex filename
#  12340     124    8192   20656    50b0 firmware.elf
```
- `text` = code + constants (lives in FLASH)
- `data` = initialized globals (stored in FLASH, copied to RAM at boot)
- `bss` = uninitialized globals (only needs RAM, NOT Flash — just zeroed at boot)
- `dec` = text + data + bss = total memory footprint
- **Flash usage:** `text + data` (everything stored in Flash)
- **RAM usage:** `data + bss` (everything that needs RAM), PLUS stack + heap at runtime
- **Exercise:** Add a `uint8_t big_buffer[4096]` as a global. Observe which column changes (`bss`). Add `= {1}` initializer → observe `data` increases AND `text` increases (initial values stored in Flash!). Remove the initializer → `bss` only. This is why `.bss` saves Flash.

## Day 2: STM32CubeIDE & CubeMX Setup (3–4 hours)

**1. Installing STM32CubeIDE (30 min)**
- Download from st.com (free, includes compiler + debugger + CubeMX)
- CubeIDE = Eclipse-based IDE + CubeMX code generator + GDB debugger
- Install STM32F4 support package when prompted

**2. CubeMX Pin Configuration (60 min)**
- CubeMX is a graphical tool that generates initialization code
- Configure: clock tree (168 MHz), GPIO pins, UART, SPI, I2C, DMA, FreeRTOS
- It generates: `main.c`, `stm32f4xx_hal_msp.c`, clock config, GPIO init, peripheral init
- **Never edit generated files directly** — your changes get overwritten on re-generation. Put custom code between `/* USER CODE BEGIN */` and `/* USER CODE END */` markers.
- **Exercise:** Create a new STM32F407 project. Configure PA5 as GPIO output (LED). Configure USART2 (115200 baud). Generate code. Build. Flash. LED should not blink yet (no code in main loop).

**3. Project Structure (45 min)**
```
MyProject/
├── Core/
│   ├── Inc/         → Header files (main.h, stm32f4xx_hal_conf.h)
│   ├── Src/         → Source files (main.c, stm32f4xx_it.c, syscalls.c)
│   └── Startup/     → startup_stm32f407xx.s (vector table, boot code)
├── Drivers/
│   ├── CMSIS/       → ARM core definitions (core_cm4.h)
│   └── STM32F4xx_HAL_Driver/  → HAL library (hal_gpio.c, hal_uart.c, etc.)
├── Middlewares/     → FreeRTOS (when enabled)
├── STM32F407VGTX_FLASH.ld  → Linker script
└── .project, .cproject      → IDE config
```
- **Exercise:** Navigate the generated project. Open `startup_stm32f407xx.s` and find the vector table. Open the linker script and find FLASH/RAM origins. Open `main.c` and find the USER CODE markers.

**4. First Flash & Debug (45 min)**
- Connect ST-Link debugger to Nucleo board (USB cable)
- Build (Ctrl+B) → Flash (Run → Debug As → STM32 C/C++ Application)
- Set breakpoints, step through code, watch variables
- **Exercise:** Add `HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5)` and `HAL_Delay(500)` in the main loop. Build, flash, verify LED blinks at 1 Hz. Set a breakpoint at the toggle line and step through.

## Day 3: Makefiles & Build Automation (2–3 hours)

**1. Makefile Fundamentals (60 min)**
```makefile
CC = gcc
CFLAGS = -Wall -Wextra -std=c99

# Rule: target: dependencies
#        recipe (must start with TAB, not spaces!)
types_exercise: types_exercise.c
	$(CC) $(CFLAGS) -o $@ $<

bitwise_exercise: bitwise_exercise.c
	$(CC) $(CFLAGS) -o $@ $<

all: types_exercise bitwise_exercise crc16 circular_buffer snapshot_struct

clean:
	rm -f types_exercise bitwise_exercise crc16 circular_buffer snapshot_struct

.PHONY: all clean
```
- `$@` = target name, `$<` = first dependency, `$^` = all dependencies
- Pattern rules: `%.o: %.c` → compile any `.c` to `.o`
- **Exercise:** Write a Makefile that builds all 5 Week 1 code homework files. Add a `test` target that runs all 5 executables sequentially.

**2. Compiler Flags Deep Dive (30 min)**

| Flag | What It Does | Why Use It |
|------|-------------|-----------|
| `-Wall` | Enable most warnings | Catch bugs early |
| `-Wextra` | Even more warnings | Catch subtle bugs |
| `-Werror` | Treat warnings as errors | Force clean code |
| `-std=c99` | Use C99 standard | `//` comments, `for(int i...)` |
| `-O0` | No optimization | Best for debugging (code maps 1:1 to source) |
| `-O2` | Full optimization | Production: faster, smaller |
| `-Os` | Optimize for size | Flash-limited MCUs |
| `-g` | Debug symbols | Required for GDB/debugger |
| `-DDEBUG` | Define `DEBUG` macro | Enable debug prints |

- **Exercise:** Compile the same file with `-O0`, `-O2`, and `-Os`. Compare output sizes (`ls -la` or `size`). Then step through each in GDB — notice `-O2` skips lines and reorders. This is why you debug with `-O0`.

## Day 4: Review & Teach RYN
- [ ] Explain the 4 compilation stages with a diagram
- [ ] Show the cross-compiler flag meanings
- [ ] Demo: CubeIDE project creation → LED blink → debug session
- [ ] Explain `arm-none-eabi-size` output: what goes to Flash vs RAM

---

# WEEK 3 — GPIO: Your First Hardware Interaction

## Day 1: GPIO Theory & Registers (3 hours)

**1. What is GPIO? (30 min)**
- General Purpose Input/Output — the simplest peripheral
- Each pin can be: Input (read voltage), Output (drive voltage), Alternate Function (UART/SPI/I2C), or Analog (ADC)
- STM32F4 has GPIO ports A through K, each with 16 pins (PA0-PA15, PB0-PB15, etc.)
- **Physical:** A GPIO pin is a tiny MOSFET transistor connected to a pad on the chip. Setting ODR bit = gate voltage changes = output voltage changes.

**2. GPIO Registers (60 min)**

| Register | Name | Purpose | Read/Write |
|----------|------|---------|-----------|
| MODER | Mode Register | 2 bits/pin: 00=Input, 01=Output, 10=AltFunc, 11=Analog | RW |
| OTYPER | Output Type | 1 bit/pin: 0=Push-Pull, 1=Open-Drain | RW |
| OSPEEDR | Output Speed | 2 bits/pin: 00=Low, 01=Medium, 10=Fast, 11=Very Fast | RW |
| PUPDR | Pull-Up/Down | 2 bits/pin: 00=None, 01=Pull-Up, 10=Pull-Down | RW |
| IDR | Input Data Register | Read pin states (1 bit per pin, read-only) | R |
| ODR | Output Data Register | Set pin outputs (read-modify-write!) | RW |
| BSRR | Bit Set/Reset | Atomic set/reset (bits [15:0] SET, bits [31:16] RESET) | W |

- **ODR vs BSRR:** `ODR |= (1<<5)` is a read-modify-write — NOT atomic. If an interrupt fires between read and write, it can corrupt other bits. `BSRR = (1<<5)` is a single atomic write — always safe.
- **Exercise:** Configure PA5 as push-pull output using ONLY register writes (no HAL):
  ```c
  RCC->AHB1ENR |= (1 << 0);              // Enable GPIOA clock
  GPIOA->MODER &= ~(0x3 << (5 * 2));     // Clear mode bits for pin 5
  GPIOA->MODER |=  (0x1 << (5 * 2));     // Set to output (01)
  GPIOA->BSRR = (1 << 5);                // Set pin HIGH (LED ON)
  GPIOA->BSRR = (1 << (5 + 16));         // Reset pin LOW (LED OFF)
  ```

**3. HAL vs Bare Metal (45 min)**
```c
// HAL (easy, portable, ~20 lines of generated init code):
HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
GPIO_PinState state = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_3);

// Bare Metal (fast, direct, 1 instruction):
GPIOA->BSRR = (1 << 5);         // Set pin — 1 clock cycle
GPIOA->BSRR = (1 << (5 + 16));  // Reset pin — 1 clock cycle

// HAL_GPIO_TogglePin is ~10 cycles (function call + read ODR + XOR + write ODR)
// Direct BSRR is 1 cycle
// For RTOSTwin snapshot timing, 1 cycle matters when you have 150 µs budget
```
- **Exercise:** Measure toggle speed: Toggle a pin in a tight loop with HAL vs bare metal. Use an oscilloscope or logic analyzer to measure the frequency. HAL: ~4 MHz. Bare metal: ~42 MHz (at 168 MHz SYSCLK).

## Day 2: Input, Pull-Ups, Debouncing (3 hours)

**1. Reading a Button (45 min)**
- Button connects pin to GND when pressed (active low)
- Need pull-up resistor: when button is NOT pressed, pin is pulled HIGH
- Internal pull-up: `GPIOA->PUPDR |= (0x1 << (3 * 2));` (pin PA3, pull-up)
- Read: `if (!(GPIOA->IDR & (1 << 3))) { /* pressed (LOW) */ }`
- **Exercise:** Configure PA3 as input with pull-up. Poll in main loop. Toggle LED when button is pressed.

**2. Button Debouncing (60 min)**
- Mechanical buttons "bounce" — the contact makes/breaks rapidly for 1-10 ms
- Without debouncing: one press registers as 5-20 presses
- **Software debounce strategies:**
  1. Simple delay: Read button → wait 20 ms → read again → if still pressed, it's real
  2. Counter: Require N consecutive identical readings (e.g., 5 reads at 5 ms intervals)
  3. Timer-based: Start 20 ms timer on first edge, ignore changes until timer expires
- **Exercise:** Implement all 3 debouncing methods. Connect LED to button. Verify each press = exactly 1 toggle.

**3. LED Patterns (45 min)**
- Blink at different rates: 1 Hz, 2 Hz, 10 Hz, 100 Hz (at 100 Hz your eye sees constant brightness ~50%)
- SOS pattern in Morse code: ... --- ... (dot = 200 ms, dash = 600 ms, gap = 200 ms, letter gap = 600 ms)
- Binary counter: Use 4 LEDs to display a 4-bit counter (0000 to 1111)
- **Exercise:** Implement SOS blinker AND 4-bit binary counter (you'll need 4 GPIO pins configured as output)

## Day 3: Alternate Functions & Practice (2–3 hours)

**1. Alternate Function Configuration (60 min)**
- Pins have multiple functions. PA2 can be: GPIO, USART2_TX, TIM5_CH3, etc.
- To use UART on PA2: set MODER to Alternate Function (10), then set AF register to AF7 (USART2)
- `GPIOA->AFR[0] |= (7 << (2 * 4));` — AFR[0] covers pins 0-7, 4 bits each
- CubeMX handles this automatically, but you MUST understand it for debugging
- **Exercise:** Configure PA2 as USART2_TX using only register writes. Compare to what CubeMX generates.

**2. Open-Drain Output (30 min)**
- Push-pull: Can drive HIGH and LOW (the default)
- Open-drain: Can only pull LOW. HIGH state is floating (needs external pull-up)
- Required for I2C (SDA/SCL are open-drain with pull-ups)
- **Exercise:** Configure a pin as open-drain. Measure voltage with multimeter in HIGH vs LOW state.

## Day 4: Review & Teach RYN
- [ ] Explain all 7 GPIO registers and their roles
- [ ] Demo: Bare-metal LED blink (no HAL, just register writes)
- [ ] Demo: Button with debouncing (show bounce problem first, then fix)
- [ ] Explain: ODR (read-modify-write, unsafe) vs BSRR (atomic, safe)

---

# WEEK 4 — Timers, SysTick, PWM, DWT, Watchdog

## Day 1: Hardware Timer Fundamentals (3–4 hours)

**1. Timer Architecture (60 min)**
- A timer is a counter that counts up (or down) at a configurable rate
- Components: Prescaler (divides clock), Counter (the count value), Auto-Reload (max value → resets to 0)
- Frequency = Timer_Clock / ((Prescaler + 1) × (AutoReload + 1))
- **Example:** APB1 = 84 MHz (timer clock is 2× APB1 if prescaler ≠ 1 → 84 MHz). Prescaler = 8399, AutoReload = 9999 → Frequency = 84,000,000 / (8400 × 10000) = 1 Hz exactly
- **Exercise:** Calculate prescaler and auto-reload for: 1 Hz, 100 Hz, 1 kHz, 10 kHz, 1 MHz. Verify each calculation produces exact frequency (no rounding).

**2. Timer Modes (45 min)**
- **Up-counting:** Counter counts 0 → ARR, generates event, resets to 0
- **Down-counting:** Counter counts ARR → 0, generates event, reloads ARR
- **Center-aligned:** Counts up to ARR then down to 0 (used for symmetric PWM)
- **One-pulse:** Counts once, stops (useful for precise delays)
- **Exercise:** Configure TIM2 in up-counting mode. Set to 1 kHz. Enable update interrupt. In ISR, increment a counter. Print counter value — should be 1000 after 1 second.

**3. SysTick — ARM's Built-In Timer (45 min)**
- 24-bit down-counter, built into every Cortex-M core
- FreeRTOS uses SysTick as its heartbeat (default: 1 kHz = 1 ms tick)
- `SysTick->LOAD = SystemCoreClock / 1000 - 1;` (168000 - 1 for 1 ms at 168 MHz)
- SysTick_Handler ISR fires every tick → FreeRTOS increments tick count, checks if context switch needed
- `HAL_Delay()` uses SysTick ticks (but blocks the CPU — don't use in RTOS tasks!)
- **Exercise:** Configure SysTick manually (no HAL). Verify 1 ms interrupt rate by toggling a GPIO in the ISR and measuring with logic analyzer.

## Day 2: PWM & DWT (3 hours)

**1. PWM — Pulse Width Modulation (60 min)**
- PWM = digital signal with variable duty cycle: fraction of time spent HIGH
- Duty cycle 0% = always LOW, 50% = half HIGH/half LOW, 100% = always HIGH
- Used for: LED brightness, motor speed, servo position, audio
- Timer counts 0 → ARR. While counter < CCR (Capture Compare Register): output HIGH. While counter ≥ CCR: output LOW.
- Duty cycle = CCR / (ARR + 1) × 100%
- **Example:** ARR = 999 (1000 steps), CCR = 250 → 25% duty cycle → LED at 25% brightness
- **Exercise:** Set up TIM3 CH1 on PA6 as PWM output. Fade LED from 0% to 100% brightness over 2 seconds. Then implement a breathing effect (smooth fade in/out).

**2. DWT Cycle Counter — Your Profiling Tool (60 min)**
- Data Watchpoint and Trace unit — ARM debug peripheral
- `DWT->CYCCNT` is a 32-bit counter, increments every CPU clock cycle (168 MHz)
- Wraps every 2^32 / 168,000,000 = 25.6 seconds
- Setup:
  ```c
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;  // Enable trace
  DWT->CYCCNT = 0;                                    // Reset counter
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;               // Start counting
  ```
- Usage:
  ```c
  uint32_t start = DWT->CYCCNT;
  snapshot_capture(&snap);    // The function we're measuring
  uint32_t elapsed = DWT->CYCCNT - start;  // Works even if wrapped!
  uint32_t us = elapsed / (SystemCoreClock / 1000000);
  printf("snapshot_capture: %lu cycles = %lu us\n", elapsed, us);
  ```
- **This is THE tool RTOSTwin uses to verify < 150 µs snapshot overhead**
- **Exercise:** Measure execution time of: `memcpy(dst, src, 100)`, `HAL_GPIO_TogglePin()`, CRC-16 over 350 bytes, `sprintf()`. Convert all to microseconds. Which is surprisingly slow?

**3. Watchdog Timers (45 min)**
- **IWDG (Independent Watchdog):** Runs on separate low-speed oscillator (32 kHz). Must be "kicked" periodically or it resets the MCU. Uses: detect firmware hang / infinite loop.
- **WWDG (Window Watchdog):** Must be kicked within a TIME WINDOW (not too early, not too late). Detects timing violations.
- Setup: Set timeout period (e.g., 1 second). In your main loop or a dedicated task, call `HAL_IWDG_Refresh()` regularly. If firmware hangs, watchdog triggers reset.
- **Exercise:** Enable IWDG with 1-second timeout. Verify normal operation (LED blinks). Then add a `while(1){}` infinite loop to simulate a hang. Observe: MCU resets after 1 second, LED pattern restarts.

## Day 3: Timer Applications (2 hours)

**1. Input Capture — Measuring External Signals (45 min)**
- Timer captures the counter value when an external signal edge occurs
- Used for: Measuring frequency, period, pulse width of external signals
- **Exercise:** Connect a function generator (or another timer's PWM output) to a capture input. Measure its frequency.

**2. Precise Periodic Execution (45 min)**
- RTOSTwin captures snapshots at exactly 10 Hz (every 100 ms)
- Using `vTaskDelayUntil()` with FreeRTOS gives ~ms precision
- Using a timer interrupt gives µs precision
- **Exercise:** Set up a timer to fire every 100 ms. In the ISR, set a flag. In the task, wait for the flag, capture snapshot, clear flag.

## Day 4: Review & Teach RYN
- [ ] Explain timer architecture: prescaler → counter → auto-reload → interrupt
- [ ] Calculate prescaler/ARR for any target frequency (live calculation)
- [ ] Demo: LED PWM breathing effect
- [ ] Demo: DWT measuring function execution time
- [ ] Explain: IWDG watchdog reset behavior

---

# WEEK 5 — SPI & I2C Protocols

## Day 1: I2C Protocol (3–4 hours)

**1. I2C Fundamentals (60 min)**
- 2 wires: SDA (data), SCL (clock) — both open-drain with pull-up resistors
- Master initiates all communication. Slaves respond when addressed.
- Each slave has a 7-bit address (e.g., BME280 sensor = 0x76 or 0x77)
- Transaction: START → Address + R/W bit → ACK → Data bytes → ACK each → STOP
- Clock stretching: Slave holds SCL LOW to pause master (slave needs more time)
- Multi-master: Possible but complex (arbitration). RTOSTwin uses single-master.

**2. I2C Registers & Transactions (60 min)**
- **Write transaction:** Master sends: [START] [ADDR+W] [REG_ADDR] [DATA] [STOP]
- **Read transaction:** Master sends: [START] [ADDR+W] [REG_ADDR] [RESTART] [ADDR+R] → slave sends [DATA] [STOP]
- HAL functions: `HAL_I2C_Mem_Write(&hi2c1, ADDR, REG, 1, &data, 1, timeout)` and `HAL_I2C_Mem_Read()`
- **Exercise:** Read the WHO_AM_I register (0xD0) of a BME280 sensor. Expected value: 0x60. If you get 0x60, the sensor is alive.

**3. Reading a Temperature Sensor (60 min)**
- BME280/BMP280: Temperature + Pressure + Humidity
- Steps: Read calibration data (stored in sensor Flash) → read raw ADC values → apply compensation formula
- Raw temperature = 20-bit value in registers 0xFA-0xFC
- Compensation involves 3 calibration coefficients and integer math
- **Exercise:** Read temperature from BME280, compensate, display as °C (×10 fixed-point for RTOSTwin `health.temperature_C`)

## Day 2: SPI Protocol (3 hours)

**1. SPI Fundamentals (60 min)**
- 4 wires: MOSI (Master Out Slave In), MISO (Master In Slave Out), SCK (Clock), CS (Chip Select, active LOW)
- Full-duplex: Master sends and receives simultaneously on every clock edge
- 4 modes based on CPOL (clock polarity) and CPHA (clock phase) — check sensor datasheet for which mode
- Much faster than I2C: 1-50 MHz typical (I2C: 100-400 kHz standard)
- No addressing: CS pin selects which slave. Each slave needs its own CS pin.
- **Exercise:** Draw the timing diagram for SPI Mode 0 (CPOL=0, CPHA=0) sending byte 0x55.

**2. SPI Flash Memory (60 min)**
- W25Q64: 8 MB external Flash via SPI. Used for data logging, firmware updates.
- Operations: Read (any address), Write (must be page-aligned, 256 bytes), Erase (sector = 4 KB minimum)
- Must erase before write (can only change 1→0, erase resets all to 1)
- **Exercise:** Write "Hello RTOSTwin" to address 0x000000 in SPI Flash. Read it back. Verify match.

**3. I2C vs SPI Comparison (30 min)**

| Feature | I2C | SPI |
|---------|-----|-----|
| Wires | 2 (SDA, SCL) | 4 + 1 CS per slave |
| Speed | 100/400 kHz standard | 1-50 MHz |
| Addressing | Built-in (7-bit) | CS pin per device |
| Multi-slave | Yes (shared bus) | Yes (separate CS) |
| Complexity | Medium (ACK, stretch) | Simple (no ACK) |
| Use for | Sensors, EEPROMs | Flash, displays, fast ADC |

## Day 3: Sensor Integration Practice (2 hours)
- **Exercise:** Read temperature (I2C sensor) AND write it to SPI Flash log entry
- **Exercise:** Create a `peripheral_snapshot_t` struct tracking: `i2c_transactions`, `spi_transactions`, `i2c_errors`, `spi_errors`. Increment counters during each transaction.

## Day 4: Review & Teach RYN
- [ ] Draw I2C write and read transactions (START, ADDR, ACK, DATA, STOP)
- [ ] Draw SPI timing diagram for Mode 0
- [ ] Demo: Read temperature from I2C sensor, display over UART
- [ ] Explain: Why RTOSTwin tracks peripheral transaction counts

---

# WEEKS 6–12 — Condensed Roadmap (Will Expand When You Reach Them)

## Week 6: ADC — Analog Input
- ADC architecture: SAR conversion, sample-and-hold, resolution (12-bit = 0-4095)
- Voltage calculation: `V = ADC_value × VREF / 4095` (VREF typically 3.3V)
- Sampling time, conversion time, total acquisition time calculation
- ADC modes: single, continuous, scan (multiple channels), injected
- DMA-driven ADC: Fill buffer automatically, process when complete
- Signal conditioning: voltage dividers (measure 12V with 3.3V ADC), RC filter (anti-aliasing)
- **Deliverable:** Read potentiometer + current sensor via ADC+DMA, compute power

## Week 7: FreeRTOS Queues — Inter-Task Communication
- `xQueueCreate(length, item_size)` — typed FIFO buffer
- `xQueueSend()` / `xQueueReceive()` — block if full/empty (with timeout)
- `xQueueSendFromISR()` — safe for ISR context (no blocking)
- Queue sets: wait on multiple queues simultaneously
- Mailboxes: queue of length 1 (latest value only, overwrite mode)
- Practical pattern: ISR → Queue → Processing Task → Queue → Logging Task
- **Deliverable:** Multi-task pipeline: sensor ISR → queue → filter task → queue → UART task

## Week 8: FreeRTOS Memory Management
- 5 heap implementations: heap_1 (alloc only), heap_2 (no coalescing), heap_3 (wraps malloc), heap_4 (first-fit with coalescing, DEFAULT), heap_5 (multiple memory regions)
- `pvPortMalloc()` / `vPortFree()` — FreeRTOS wrappers
- `xPortGetFreeHeapSize()` — current free bytes (→ `memory_snapshot_t.heap_free`)
- `xPortGetMinimumEverFreeHeapSize()` — lowest free ever (→ `heap_min_ever_free`)
- Memory pools: Pre-allocate N fixed-size blocks. Alloc/free = O(1), zero fragmentation.
- **Deliverable:** Monitor heap usage in real-time, detect simulated leak

## Week 9: FreeRTOS Debugging & Stack Overflow Detection
- Stack overflow detection: `configCHECK_FOR_STACK_OVERFLOW` (methods 1 and 2)
- `vApplicationStackOverflowHook()` — callback when overflow detected
- `uxTaskGetStackHighWaterMark()` — minimum remaining stack bytes
- FreeRTOS trace macros: `traceTASK_SWITCHED_IN()`, `traceTASK_SWITCHED_OUT()`
- SystemView / TraceRecorder integration for visual debugging
- Common bugs: wrong task priority, missed ISR `FromISR` variant, stack too small
- **Deliverable:** Stack watermark reporting for all tasks, overflow detection active

## Week 10: RTOSTwin — Delta Encoder
- `delta_encode(current, previous, output, &len)` implementation
- Changed-fields bitmask: 1 byte tells decoder which sections to expect
- Field-level comparison using `memcmp` (must `memset` structs!)
- Keyframe strategy: full snapshot every N packets for resync
- Compression ratio measurement: full=350B, typical delta=20-50B (85-95% savings)
- **Deliverable:** Working delta encoder/decoder with compression ratio logging

## Week 11: PC Receiver — Serial Decode & State Reconstruction
- Python serial receiver: `pyserial` library, sync byte detection, packet parsing
- CRC verification, sequence number gap detection
- Delta decoder: maintain running state, apply deltas, handle keyframes
- State database: SQLite for history, in-memory dict for current state
- **Deliverable:** Python script receiving live telemetry, reconstructing full state

## Week 12: Dashboard v1 — React + WebSocket
- React project setup (`create-react-app` or Vite)
- WebSocket connection: Python backend → browser
- Task Timeline component: Gantt chart showing task states over time
- Memory Graph component: Plotly.js chart with prediction line
- Alert Panel: Real-time warnings for high stack usage, low heap, CPU spikes
- **Deliverable:** Live web dashboard displaying telemetry data

---

## Progress Tracker

| Week | Topic | Status | Started | Completed | Taught RYN? |
|------|-------|--------|---------|-----------|-------------|
| 1 | Bitwise, Structs, CRC | ⬜ Not Started | — | — | ⬜ |
| 2 | Build Systems, CubeIDE | ⬜ Not Started | — | — | ⬜ |
| 3 | GPIO & LED Blink | ⬜ Not Started | — | — | ⬜ |
| 4 | Timers, PWM, DWT | ⬜ Not Started | — | — | ⬜ |
| 5 | SPI & I2C | ⬜ Not Started | — | — | ⬜ |
| 6 | ADC | ⬜ Not Started | — | — | ⬜ |
| 7 | FreeRTOS Queues | ⬜ Not Started | — | — | ⬜ |
| 8 | FreeRTOS Memory | ⬜ Not Started | — | — | ⬜ |
| 9 | Debugging & Stack | ⬜ Not Started | — | — | ⬜ |
| 10 | Delta Encoder | ⬜ Not Started | — | — | ⬜ |
| 11 | PC Receiver | ⬜ Not Started | — | — | ⬜ |
| 12 | Dashboard v1 | ⬜ Not Started | — | — | ⬜ |

---

**END OF VNV'S TIMELINE**
