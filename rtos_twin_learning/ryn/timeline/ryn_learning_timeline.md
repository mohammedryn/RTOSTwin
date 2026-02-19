# 📋 RYN's Complete Learning Timeline
## RTOSTwin — Individual Study Track (Ultra-Detailed Edition)

**Study Partner:** VNV  
**Method:** Solo study for 3–4 days → Teach each other for 3–4 days  
**Your Track:** Systems & Architecture (CPU internals, memory, interrupts, RTOS scheduling, protocols)  

---

> [!TIP]
> **Your identity in this project:** You are the **systems person**. You understand what happens INSIDE the CPU — how memory is organized, how interrupts steal control, how the scheduler decides which task runs next, how bytes travel over UART with DMA. VNV is the **data person** — she understands how to encode, structure, and verify that data. Together you cover the full stack.

---

## Master Schedule

| Week | RYN Solo Topics | VNV Solo Topics | Teach Session Focus |
|------|----------------|----------------|-------------------|
| 1 | C Types, Memory Model, Pointers, volatile, static | Bitwise Ops, Structs, Padding, Preprocessor, CRC | Full C foundations |
| 2 | Number Systems, Two's Complement, Memory Map | Build Systems, Toolchain, CubeIDE Setup | Binary thinking + Dev environment |
| 3 | ARM Cortex-M4 Architecture, Registers, Bus, Clocks | GPIO Modes, Registers, HAL vs Bare Metal, Debouncing | Hardware model + First I/O |
| 4 | Interrupts, NVIC, Vector Table, Critical Sections | Timers, SysTick, PWM, DWT Cycle Counter, Watchdog | Real-time event handling + Timing |
| 5 | UART Protocol, Baud Rate, Polling/IRQ/DMA modes | SPI Protocol, I2C Protocol, Sensor Communication | All serial protocols |
| 6 | DMA Engine, Channels, Circular Mode, UART+DMA | ADC Fundamentals, Sampling, DMA-driven ADC | Zero-copy I/O |
| 7 | Why RTOS, Super-loop vs Preemptive, Task Creation | FreeRTOS Queues, xQueueSend/Receive, ISR Queues | RTOS core mechanics |
| 8 | Semaphores, Mutexes, Priority Inversion, Deadlock | FreeRTOS Heap (1-5), pvPortMalloc, Memory Pools | Synchronization + Memory |
| 9 | CPU Usage Measurement, Idle Hook, Runtime Stats | Stack Overflow Detection, Debugging, Trace Hooks | RTOS diagnostics |
| 10 | Snapshot Engine: `snapshot_capture()`, Critical Path | Delta Encoder: Bitmask, Change Detection, Keyframes | RTOSTwin Agent Core |
| 11 | Transport Layer: Packet Framing, CRC, Sequence Numbers | PC Receiver: Serial Decode, State Reconstruction | End-to-end telemetry |
| 12 | Twin State Manager (C++), Ring Buffer, Thread Safety | Dashboard v1: React, WebSocket, Task Timeline Chart | Host-side twin |

---

# WEEK 1 — C Types, Memory Model, Pointers, `volatile`, Static Allocation

## Day 1: The Type System (3–4 hours)

### Topics to Cover

**1. Why `int` Is Dangerous (30 min)**
- `int` is 16-bit on MSP430, 32-bit on ARM, 64-bit on some x64 compilers
- Code that "works" on your PC silently overflows on the target MCU
- The fix: `#include <stdint.h>` — use `uint8_t`, `uint16_t`, `uint32_t`, `uint64_t`
- Signed variants: `int8_t`, `int16_t`, `int32_t`, `int64_t`
- **Exercise:** Write a program that prints `sizeof()` for every `stdint` type AND every built-in type (`char`, `short`, `int`, `long`, `long long`, `void*`). Compare on your PC.

**2. Unsigned Integer Overflow (45 min)**
- `uint8_t` range: 0–255. Storing 300 gives 300 mod 256 = 44
- `uint16_t` range: 0–65535. Storing 70000 gives 70000 mod 65536 = 4464
- This is NOT a bug — it's defined behavior for unsigned types
- **Why this is useful:** Timestamp wraparound. `uint16_t end = 5, start = 65530; elapsed = end - start = 11` ← correct! Unsigned subtraction handles wrap naturally
- **Exercise:** Store 300, 256, 255, 1000 in `uint8_t`. Print each. Predict results FIRST, then verify.
- **Exercise:** Time delta with wraparound: `start=65530, end=5`. Show that `end-start = 11`.

**3. Signed Integer Representation — Two's Complement (45 min)**
- `int8_t` range: -128 to +127
- How -1 is stored: all bits set → `0xFF` (unsigned 255)
- How -128 is stored: `0x80` (MSB set, all others zero)
- Conversion: `uint8_t x = 200; int8_t y = (int8_t)x;` → y = 200 - 256 = -56
- **The comparison trap:** `int8_t a = -1; uint8_t b = 1; if (a < b)` → FALSE! Because -1 is promoted to unsigned (255), and 255 > 1
- **Exercise:** Cast every value 0–255 from `uint8_t` to `int8_t`. At which value does the sign flip? Verify it's at 128.

**4. Fixed-Point Arithmetic (30 min)**
- Problem: Cortex-M0/M3 have no FPU. `float` operations take 200+ CPU cycles
- Solution: Represent 23.7°C as integer 237 (×10 scaling)
- Common scales: ×10 (0.1 resolution), ×100 (0.01), ×256 (Q8.8 binary format)
- Multiplication: `int16_t a = 237; int16_t b = 15; int32_t result = (int32_t)a * b / 10;` (must widen to 32-bit BEFORE multiplying to avoid overflow)
- RTOSTwin usage: `health_snapshot_t.temperature_C` uses ×10 scale (237 = 23.7°C)
- **Exercise:** Implement `fixed_multiply(int16_t a, int16_t b, uint8_t scale)` that returns `(a * b) / scale` using 32-bit intermediate.

### Deliverables for Day 1
- [ ] Completed program printing all `sizeof()` values
- [ ] Overflow demonstration with predictions
- [ ] Two's complement conversion table (10 examples)
- [ ] Fixed-point multiply function

---

## Day 2: Memory Layout & The Stack (3–4 hours)

### Topics to Cover

**1. The Four Sections of Compiled C (45 min)**
```
.text   → Your compiled code (functions). Lives in FLASH. Read-only.
.rodata → Constants (const char[], const int[]). Lives in FLASH. Read-only.
.data   → Initialized globals (int counter = 42). Copied from Flash → RAM at boot.
.bss    → Uninitialized globals (static uint32_t buf[100]). Zeroed at boot. NO Flash cost.
```
- **Why .bss saves Flash:** `static uint8_t buffer[4096];` needs 4 KB of RAM but ZERO bytes of Flash (startup code just memsets it to 0). A `= {0}` initializer would waste 4 KB of Flash too!
- **The startup sequence:** Reset_Handler → copy .data from Flash to RAM → zero .bss → call SystemInit → call main()
- **Exercise:** Create 3 global variables: `int initialized = 42;` (.data), `int uninitialized;` (.bss), `const int constant = 99;` (.rodata). Print their addresses. Observe grouping.

**2. The Stack — How Function Calls Work (60 min)**
- Every function call pushes a **stack frame**: return address, saved registers, local variables
- Stack grows DOWNWARD on ARM (from high address to low)
- Stack size per RTOS task: 256 words (1 KB) to 1024 words (4 KB) — YOU choose at `xTaskCreate()`
- **Stack overflow:** If a task uses more stack than allocated, the stack pointer grows past the boundary into another task's memory or the heap → silent data corruption → crash later (the hardest bug to find)
- The "canary" technique: FreeRTOS fills stack with 0xA5A5A5A5 at creation. `uxTaskGetStackHighWaterMark()` scans from the bottom for the first non-0xA5 byte → tells you how close you got to overflow
- **Exercise:** Write a recursive function `factorial(n)`. Print the address of a local variable inside each recursive call. Watch the stack grow downward. At what depth of recursion does it crash? (On PC with default 1 MB stack, try n=100000)
- **Connection to RTOSTwin:** `task_snapshot_t.stack_used` and `task_snapshot_t.stack_total` report exactly this. The Stack Overflow Predictor watches `stack_used` grow over time.

**3. The Heap — Dynamic Memory (45 min)**
- `malloc(size)` → searches a free list for a block ≥ size → returns pointer (or NULL on failure)
- `free(ptr)` → marks the block as available → may merge with adjacent free blocks (coalescing)
- **Fragmentation:** After many alloc/free cycles, the heap has many small holes. You have 10 KB free total, but no single block ≥ 1 KB → `malloc(1024)` fails even though you have "enough" memory
- **Why malloc is forbidden in embedded hot paths:**
  1. Non-deterministic execution time (worst case: walks entire free list)
  2. Fragmentation (eventually all allocs fail)
  3. FreeRTOS uses a mutex to protect the heap → priority inversion
  4. Can return NULL (most code doesn't check)
  5. Memory leaks (forgot `free` → heap shrinks forever)
- `pvPortMalloc()` / `vPortFree()` — FreeRTOS wrappers, same problems
- **Exercise:** Write a program that mallocs 100-byte blocks in a loop, frees every other one, then tries to malloc 200 bytes. Does it succeed even though there's "enough" total free space? (Demonstrates fragmentation)

**4. RTOSTwin RAM Budget (30 min)**
- STM32F4: 192 KB RAM total
- Calculate: `.data`(~2 KB) + `.bss`(~5 KB) + FreeRTOS heap(~80 KB) + agent(< 10 KB) + ISR stack(~2 KB) = ~99 KB. Margin = 93 KB.
- Agent breakdown: snapshot buffers(~1 KB) + TX queue(~4 KB) + task status buffer(~1 KB) + misc(~4 KB) = ~10 KB
- Teensy 4.1: 1 MB RAM — much more room, same principles
- **Exercise:** Calculate the exact `sizeof(full_snapshot_t)` based on the struct definitions. Multiply by 2 (current + previous for delta). Add TX queue (32 × 128 bytes = 4 KB). Is the total < 10 KB?

### Deliverables for Day 2
- [ ] Address printing program showing .data/.bss/stack/heap grouping
- [ ] Recursive stack growth demo with address logging
- [ ] Fragmentation demonstration program
- [ ] Written RAM budget calculation for STM32F4

---

## Day 3: Pointers, `volatile`, and Static Allocation (3–4 hours)

### Topics to Cover

**1. Pointers as Hardware Addresses (60 min)**
- Desktop: pointer = abstract handle to memory. You never care about the actual address.
- Embedded: pointer = a PHYSICAL WIRE. `*(volatile uint32_t*)0x40020014 |= (1<<5)` physically turns on a pin. The ARM bus routes that write to the GPIO peripheral silicon.
- **Memory-Mapped I/O (MMIO):** On ARM, there are no special I/O instructions. Reading/writing memory addresses 0x40000000+ accesses peripheral hardware, not RAM.
- STM32 defines C structs that overlay the peripheral registers:
  ```c
  // From stm32f4xx.h:
  typedef struct {
      volatile uint32_t MODER;    // offset 0x00
      volatile uint32_t OTYPER;   // offset 0x04
      volatile uint32_t OSPEEDR;  // offset 0x08
      volatile uint32_t PUPDR;    // offset 0x0C
      volatile uint32_t IDR;      // offset 0x10
      volatile uint32_t ODR;      // offset 0x14
      volatile uint32_t BSRR;     // offset 0x18
      // ...
  } GPIO_TypeDef;
  
  #define GPIOA  ((GPIO_TypeDef *)0x40020000)
  // GPIOA->ODR is at 0x40020000 + 0x14 = 0x40020014
  ```
- **Exercise:** Calculate the address of `GPIOA->BSRR` manually. Then verify with `printf("%p", &GPIOA->BSRR)` on real hardware.

**2. Pointer Arithmetic (45 min)**
- `ptr + 1` advances by `sizeof(*ptr)` bytes, NOT 1 byte
- `uint32_t *p; p+1` advances by 4 bytes. `uint8_t *p; p+1` advances by 1 byte.
- Array indexing IS pointer arithmetic: `arr[i]` ≡ `*(arr + i)`
- **Walking a buffer with a pointer:** This is how the delta encoder serializes data:
  ```c
  uint8_t *write_ptr = output_buffer;
  *write_ptr++ = header_byte;          // Write 1 byte, advance
  memcpy(write_ptr, &data, 4);         // Write 4 bytes of data
  write_ptr += 4;                       // Advance by 4
  uint16_t bytes_written = write_ptr - output_buffer;  // Total length
  ```
- **Exercise:** Implement `void hexdump(const void *data, size_t len)` that prints a hex dump of any memory region, 16 bytes per line, with ASCII on the right. This is a tool you'll use forever in embedded debugging.

**3. `volatile` — The Life-or-Death Keyword (60 min)**
- Without `volatile`, the compiler may:
  - Cache a register read and never re-read it (→ infinite loop waiting for hardware flag)
  - Reorder writes to hardware registers (→ peripheral misconfigured)
  - Eliminate a "dead" write that isn't read later (→ hardware never gets poked)
- **Four mandatory uses:**
  1. Hardware peripheral registers (`volatile uint32_t *UART_SR`)
  2. Variables written in ISR, read in main (`volatile uint8_t flag`)
  3. Variables shared between RTOS tasks (`volatile uint32_t counter`)
  4. DMA buffers (DMA writes behind the CPU's back)
- **The concrete bug:** A UART transmit loop:
  ```c
  // BUG: Compiler reads SR once, caches it, loops forever
  while (!(USART1->SR & (1 << 7))) {}  // Wait for TXE flag
  // FIX: SR is declared volatile in the struct → re-reads every iteration
  ```
- **Exercise:** Write a program with a global `volatile uint32_t tick_count = 0;` incremented in a simulated "ISR" (a separate thread on PC using `<threads.h>` or `<pthread.h>`). In main, print `tick_count` every second. Then remove `volatile` and compile with `-O2`. Observe: with optimization, the compiler may cache `tick_count` and never see updates. (This effect may or may not appear depending on your compiler — the point is understanding WHY volatile matters.)

**4. Static Allocation Strategy (30 min)**
- The 3 meanings of `static`:
  1. **File-scope:** `static int x = 0;` at file level → private to this .c file (other files can't `extern` it)
  2. **Persistent local:** `static int count = 0;` inside a function → survives between calls, NOT on stack, in .bss
  3. **Compile-time buffer:** `static uint8_t buf[256];` → allocated in .bss, always same address, zero overhead
- Why `snapshot_capture()` uses `static TaskStatus_t task_buf[MAX_TASKS];`:
  - NOT stack: task stack may be small (1 KB), buffer is 300+ bytes
  - NOT malloc: forbidden in hot path 
  - Static: always available, same address, zero alloc cost, zero fragmentation risk
- **Exercise:** Write `snapshot_capture()` stub with a `static` internal buffer. Add a `static uint32_t call_count` that tracks how many times the function has been called. Print the address of the static buffer — verify it never changes across calls.

### Deliverables for Day 3
- [ ] hexdump() utility function (will actually use this later!)
- [ ] Volatile demonstration program (with/without optimization)
- [ ] snapshot_capture() stub with static buffer + call counter
- [ ] Written explanation: "3 meanings of static" in your own words

---

## Day 4: Review & Teach Prep (2–3 hours)

**1. Self-Test (60 min)**
Without looking at notes, answer:
- [ ] What is `sizeof(uint32_t)` on ARM? On MSP430? Why is `int` dangerous?
- [ ] What is `(uint8_t)300`? Show the math.
- [ ] Draw the RAM layout: .data, .bss, heap (up), stack (down). Label addresses.
- [ ] What does `volatile` prevent the compiler from doing? List 4 cases where it's mandatory.
- [ ] Why is `malloc` forbidden in the telemetry agent? List 5 reasons.
- [ ] What are the 3 meanings of `static`?

**2. Teach Prep (60 min)**

Prepare 3 live demos for VNV:

| Demo | What to Show | Key Takeaway |
|------|-------------|-------------|
| **Overflow** | Store 300 in `uint8_t`, print result | Types have limits, C doesn't warn you |
| **Memory Map** | Print addresses of global, static, stack, heap vars | Memory has structure, stack grows down |
| **volatile** | Show the ISR/main flag pattern, explain compiler bug | Hardware changes memory behind CPU's back |

**3. Complete Homework (remaining time)**
- [ ] Finish `ryn_types_and_memory.c`
- [ ] Fill in `week1_theory_answers.md`
- [ ] Update `week1_notes.md` with personal insights

---

# WEEK 2 — Number Systems, Two's Complement, STM32 Memory Map, Linker Scripts

## Day 1: Binary, Hex & Conversions (3 hours)

### Topics to Cover

**1. Why Embedded Engineers Think in Hex (30 min)**
- Binary is the native language of hardware (each bit = a physical wire/gate)
- Hex is compact binary: each hex digit = exactly 4 bits
- `0xFF` = `1111 1111` (8 bits ON) — instant visual for "all bits set"
- `0xDEADBEEF` = common debug pattern to mark uninitialized memory
- `0xA5A5A5A5` = FreeRTOS stack fill pattern (alternating bits, easy to spot)
- **Exercise:** Convert these by hand (no calculator): 42 → binary → hex. 0xCAFE → binary → decimal. 0b10110100 → hex → decimal.

**2. Powers of 2 (Must Memorize) (15 min)**

| 2^N | Value | Common Name |
|-----|-------|-------------|
| 2^0 | 1 | |
| 2^1 | 2 | |
| 2^4 | 16 | One hex digit |
| 2^8 | 256 | One byte range |
| 2^10 | 1,024 | 1 KB |
| 2^12 | 4,096 | 12-bit ADC max + 1 |
| 2^16 | 65,536 | uint16_t range |
| 2^20 | 1,048,576 | 1 MB |
| 2^32 | 4,294,967,296 | uint32_t range |

**3. Two's Complement Deep Dive (60 min)**
- Positive numbers: same as unsigned
- Negative numbers: invert all bits, add 1
- Example: -5 in 8-bit: 5 = `0000 0101` → invert = `1111 1010` → +1 = `1111 1011` = 0xFB
- The MSB (most significant bit) is the sign bit: 1 = negative
- Range of N-bit signed: -2^(N-1) to +2^(N-1) - 1
- **Why two's complement?** Addition/subtraction works identically for signed and unsigned in hardware. The CPU doesn't even know if a number is signed — it's YOUR code's interpretation.
- **Exercise:** Compute the two's complement representation for: -1, -128, -127, -42 in 8-bit. Verify: `-1 + 1 = 0` in binary (all carries propagate, result = 0x00 with overflow discarded).

**4. Bitfield Extraction & Insertion (60 min)**
- Hardware registers pack multiple fields into a single 32-bit word
- Extraction: `value = (reg >> start_bit) & ((1 << width) - 1);`
- Insertion: `reg = (reg & ~(mask << start)) | ((value & mask) << start);`
- Example: STM32 RCC_CFGR register — bits [3:2] = SWS (System clock Switch Status):
  - `00` = HSI, `01` = HSE, `10` = PLL
  - `uint8_t clock_src = (RCC->CFGR >> 2) & 0x3;`
- **Exercise:** Write `uint32_t extract_field(uint32_t reg, uint8_t start, uint8_t width)` and `uint32_t insert_field(uint32_t reg, uint8_t start, uint8_t width, uint32_t value)`. Test with 10 cases.

## Day 2: STM32F4 Memory Map (3 hours)

### Topics to Cover

**1. The Complete Address Space (60 min)**

```
0x0000_0000 ┌────────────────────────────────────────┐
            │ Aliased to Flash or SRAM (boot config)  │
0x0800_0000 ├────────────────────────────────────────┤
            │ FLASH (1 MB)                            │
            │ Your firmware code lives here            │
            │ Vector table at 0x0800_0000              │
            │ Reset_Handler entry point                │
0x0810_0000 ├────────────────────────────────────────┤
            │ (reserved)                               │
0x1FFF_0000 ├────────────────────────────────────────┤
            │ System Memory (ST bootloader ROM)        │
            │ Used for DFU/UART firmware upload         │
0x1FFF_7A10 │ ← Unique Device ID (96-bit serial #)    │
0x2000_0000 ├────────────────────────────────────────┤
            │ SRAM1 (112 KB)                           │
            │ .data, .bss, heap, task stacks           │
0x2001_C000 ├────────────────────────────────────────┤
            │ SRAM2 (16 KB)                            │
0x2002_0000 ├────────────────────────────────────────┤
            │ SRAM3 (64 KB) — CCM RAM (F407 only)     │
            │ No DMA access! CPU-only. Good for stacks │
0x2003_0000 ├────────────────────────────────────────┤
            │ (reserved)                               │
0x4000_0000 ├────────────────────────────────────────┤
            │ APB1 Peripherals (42 MHz max)            │
            │ TIM2-TIM7, USART2/3, I2C1-3, SPI2/3     │
            │ DAC, PWR, CAN1/2, IWDG, WWDG            │
0x4001_0000 ├────────────────────────────────────────┤
            │ APB2 Peripherals (84 MHz max)            │
            │ TIM1/8-11, USART1/6, SPI1, ADC1-3       │
            │ SDIO, SYSCFG, EXTI                       │
0x4002_0000 ├────────────────────────────────────────┤
            │ AHB1 Peripherals (168 MHz)               │
            │ GPIOA-K, RCC, DMA1/2, Flash Interface    │
0x5000_0000 ├────────────────────────────────────────┤
            │ AHB2 Peripherals                         │
            │ USB OTG FS, RNG, DCMI                    │
0x6000_0000 ├────────────────────────────────────────┤
            │ AHB3 / FSMC (External Memory Controller) │
0xA000_0000 ├────────────────────────────────────────┤
            │ (reserved)                               │
0xE000_0000 ├────────────────────────────────────────┤
            │ ARM Core Peripherals                     │
            │ 0xE000_E010: SysTick Timer               │
            │ 0xE000_E100: NVIC (Interrupt Controller) │
            │ 0xE000_ED00: SCB (System Control Block)  │
            │ 0xE000_EDF0: CoreDebug                   │
            │ 0xE000_1000: DWT (Cycle Counter!)        │
0xFFFF_FFFF └────────────────────────────────────────┘
```

- **Key insight:** When you write `GPIOA->ODR = 0x20`, the CPU puts address `0x40020014` on the AHB bus. The bus routes it to the GPIO peripheral hardware. The peripheral interprets the value and physically changes a pin voltage. No OS involved, no driver — just an address.
- **Exercise:** From the reference manual, find the base addresses of: USART2, TIM2, ADC1, DMA1. Verify them against the `stm32f4xx.h` header file.

**2. The Clock Tree (45 min)**
- External crystal (HSE): 8 MHz (on most Nucleo boards)
- PLL multiplies: HSE (8 MHz) × PLLN / PLLM → typically 336 MHz VCO → ÷ PLLP = 168 MHz SYSCLK
- SYSCLK → AHB prescaler → HCLK (168 MHz) → to CPU, DMA, SRAM
- HCLK → APB1 prescaler (÷4) → 42 MHz → to USART2, I2C, TIM2-7
- HCLK → APB2 prescaler (÷2) → 84 MHz → to USART1, SPI1, ADC, TIM1
- **Why this matters:** To configure UART baud rate, you must know the APB clock. Wrong clock assumption → wrong baud rate → garbage serial output
- **Exercise:** With HCLK=168MHz and APB1 prescaler=4, what is the APB1 clock? If a timer on APB1 counts at APB1 clock speed, how many timer ticks = 1 millisecond?

**3. The Linker Script (45 min)**
- The linker script (`.ld` file) tells the linker WHERE to place each section
- Key sections:
  ```
  FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 1024K
  RAM   (rwx): ORIGIN = 0x20000000, LENGTH = 128K
  
  .text → FLASH
  .rodata → FLASH
  .data → RAM (but initial values stored in FLASH, copied at startup)
  .bss → RAM (zeroed at startup)
  _estack = ORIGIN(RAM) + LENGTH(RAM);  /* Top of RAM = initial stack pointer */
  ```
- **Exercise:** Open the `.ld` file in your STM32CubeIDE project. Find `ORIGIN` and `LENGTH` for FLASH and RAM. Find `_estack`. Verify these match the memory map above.

## Day 3: Practice & Edge Cases (2–3 hours)

**1. Endianness (45 min)**
- **Little-endian (ARM default):** Least significant byte stored at lowest address
  - `uint32_t x = 0xDEADBEEF;` stored as: `EF BE AD DE` in memory
- **Big-endian:** Most significant byte first (network byte order)
- **Why this bites you:** When you send a struct over UART/network, the byte order matters. PC (x86, little-endian) and ARM (little-endian) usually match. But network protocols and some sensors use big-endian.
- `__builtin_bswap32()` swaps byte order
- **Exercise:** Store `0x12345678` in a `uint32_t`. Use your `hexdump()` function to display the raw bytes. Is the first byte `0x12` (big-endian) or `0x78` (little-endian)?

**2. Bit-Banding (ARM Cortex-M4 Specific) (30 min)**
- ARM provides special addresses where writing to a single word atomically sets/clears one bit in a peripheral register
- Formula: `bit_word_addr = bit_band_base + (byte_offset × 32) + (bit_number × 4)`
- Bit-banding on SRAM: base = 0x22000000 (maps to 0x20000000-0x200FFFFF)
- Bit-banding on peripherals: base = 0x42000000 (maps to 0x40000000-0x400FFFFF)
- **Why care:** Atomic bit manipulation without disabling interrupts!
- **Exercise:** Calculate the bit-band address for GPIOA ODR bit 5. Write a value to it. Verify the LED toggles.

## Day 4: Review & Teach VNV

- [ ] Draw the complete STM32F4 memory map from memory (all regions, addresses)
- [ ] Explain the clock tree: crystal → PLL → SYSCLK → AHB → APB1/APB2
- [ ] Convert 5 numbers each way: decimal ↔ binary ↔ hex (without calculator)
- [ ] Explain two's complement for -1, -128, -42

---

# WEEK 3 — ARM Cortex-M4 Architecture

## Day 1: The CPU Core (3–4 hours)

**1. ARM Cortex-M4 Registers (60 min)**

| Register | Name | Purpose |
|----------|------|---------|
| R0–R3 | Arguments | Function parameters, return value (R0) |
| R4–R11 | Callee-saved | Preserved across function calls |
| R12 | IP | Intra-procedure scratch |
| R13 | SP | Stack Pointer (MSP or PSP) |
| R14 | LR | Link Register (return address) |
| R15 | PC | Program Counter (current instruction) |
| xPSR | Status | Flags (Negative, Zero, Carry, oVerflow), exception number |

- **Two stack pointers:** MSP (Main SP, used by ISRs and before RTOS starts) and PSP (Process SP, used by RTOS tasks). FreeRTOS switches to PSP for tasks.
- Exception return: LR gets magic values like `0xFFFFFFF9` (return to thread mode, MSP) or `0xFFFFFFFD` (return to thread mode, PSP)
- **Exercise:** In a debugger, step through code and watch R0-R3 change with function calls. Set a breakpoint in an ISR and observe LR has a magic value.

**2. The Fetch-Decode-Execute Pipeline (45 min)**
- Cortex-M4 has a 3-stage pipeline: Fetch → Decode → Execute
- Most instructions take 1 cycle (after pipeline fills)
- Branch penalty: pipeline flush = 1-3 cycle stall (avoided by branch prediction on M7)
- Thumb-2 instruction set: mix of 16-bit and 32-bit instructions for code density
- **Exercise:** Look at assembly output: `arm-none-eabi-gcc -S -O2 -mcpu=cortex-m4 file.c`. Find examples of 16-bit vs 32-bit instructions.

**3. Floating Point Unit (45 min)**
- Cortex-M4 has single-precision FPU (float, NOT double)
- `float` operations: 1-2 cycles. `double` operations: software emulation (200+ cycles!)
- FPU must be enabled at startup: `SCB->CPACR |= (0xF << 20);` (CubeIDE does this for you)
- FPU has its own register bank: S0–S31 (32-bit) or D0–D15 (64-bit pairs)
- **Context switch cost:** RTOS must save/restore FPU registers if tasks use FPU (~20 extra cycles)
- **Exercise:** Compile a program with `-mfloat-abi=hard -mfpu=fpv4-sp-d16`. Compare code size to `-mfloat-abi=soft`. Measure execution time of 1000 float multiplications vs 1000 fixed-point multiplications.

## Day 2: Bus Architecture & Clock System (3 hours)

**1. AHB & APB Buses (60 min)**
- AHB (Advanced High-performance Bus): 168 MHz, connects CPU to fast peripherals (GPIO, DMA, SRAM)
- APB1: 42 MHz max, connects to "slow" peripherals (UART2-5, I2C, SPI2/3, timers 2-7)
- APB2: 84 MHz max, connects to "fast" peripherals (UART1/6, SPI1, ADC, timers 1/8-11)
- **Bus matrix:** Multi-master. CPU and DMA can access different slaves simultaneously without conflict
- **Wait states:** Flash access is slower than CPU. At 168 MHz, need 5 wait states. ART Accelerator (instruction cache + prefetch) hides this latency.
- **Exercise:** Read RM0090 Section 2.3 (Bus Matrix). Draw the connections between masters (CPU, DMA1, DMA2) and slaves (Flash, SRAM, APB1, APB2).

**2. Clock Configuration Deep Dive (60 min)**
- Follow the clock through: HSE (8 MHz) → PLL (×42 ÷2 = 168 MHz) → SYSCLK → AHB → APB1/APB2
- `SystemCoreClock` variable holds SYSCLK in Hz (168000000)
- Microsecond calculation: `cycles / (SystemCoreClock / 1000000)`
- **Exercise:** Write `uint32_t cycles_to_us(uint32_t cycles)` and `uint32_t us_to_cycles(uint32_t us)`. Use these in all future timing measurements.

## Day 3: Debug Hardware (2–3 hours)

**1. DWT Cycle Counter — Your Profiling Tool (60 min)**
```c
// Enable
CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
DWT->CYCCNT = 0;
DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

// Measure
uint32_t start = DWT->CYCCNT;
my_function();
uint32_t cycles = DWT->CYCCNT - start;
uint32_t us = cycles / (SystemCoreClock / 1000000);
```
- DWT->CYCCNT is a free-running 32-bit counter, wraps every ~25.5 seconds at 168 MHz
- Zero overhead: reading CYCCNT takes 1 cycle
- **This is THE tool for verifying the < 150 µs target for `snapshot_capture()`**
- **Exercise:** Measure `HAL_GPIO_TogglePin()`, `memcpy(dst, src, 100)`, and an empty `for(i=0;i<1000;i++)` loop. Convert to microseconds.

**2. SWO Trace (30 min)**
- Serial Wire Output: printf-like output through the debug probe, no UART needed
- `ITM_SendChar(c)` sends a character to the debugger's trace window
- Useful for debugging without consuming a UART peripheral
- **Exercise:** Configure SWO in CubeIDE. Redirect `printf` to SWO using `_write()` syscall override.

## Day 4: Review & Teach VNV
- [ ] List all ARM registers and their roles from memory
- [ ] Explain pipeline, FPU context switch cost
- [ ] Demo: DWT cycle counter measuring function execution time
- [ ] Draw AHB/APB bus architecture

---

# WEEK 4 — Interrupts, NVIC, Vector Table, Critical Sections

## Day 1: Interrupt Fundamentals (3–4 hours)

**1. What Happens When an Interrupt Fires (60 min)**
- Step by step:
  1. Peripheral asserts its IRQ line (e.g., UART receives a byte)
  2. NVIC checks priority vs current execution priority
  3. If higher priority: CPU saves context (R0-R3, R12, LR, PC, xPSR) to current stack
  4. CPU loads PC from vector table entry for that IRQ
  5. ISR function executes
  6. ISR returns (special LR value triggers unstacking)
  7. CPU restores saved registers, resumes previous code at exact instruction

- Context save is automatic on Cortex-M (hardware stacking) — takes 12 cycles
- **Tail-chaining:** If another interrupt is pending when ISR finishes, CPU skips unstacking/restacking — jumps directly to next ISR (saves 24 cycles)
- **Late arrival:** If a higher-priority interrupt arrives during stacking, CPU switches to that ISR instead

**2. The Vector Table (45 min)**
```c
// At address 0x08000000 (start of Flash):
__attribute__((section(".isr_vector")))
const void (*vectors[])(void) = {
    (void *)&_estack,        // 0: Initial Stack Pointer (not a function!)
    Reset_Handler,            // 1: Reset → runs at power-on / reset button
    NMI_Handler,              // 2: Non-Maskable Interrupt
    HardFault_Handler,        // 3: All faults if no specific handler
    MemManage_Handler,        // 4: Memory protection violation
    BusFault_Handler,         // 5: Bus error (invalid address)
    UsageFault_Handler,       // 6: Undefined instruction, div-by-zero
    0, 0, 0, 0,              // 7-10: Reserved
    SVC_Handler,              // 11: Supervisor Call (used by FreeRTOS!)
    DebugMon_Handler,         // 12: Debug monitor
    0,                        // 13: Reserved
    PendSV_Handler,           // 14: Pendable SV (FreeRTOS context switch!)
    SysTick_Handler,          // 15: System Timer (FreeRTOS tick!)
    // External interrupts start here:
    WWDG_IRQHandler,          // 16: Window Watchdog
    EXTI0_IRQHandler,         // 21: GPIO EXTI line 0
    // ... more entries ...
    USART2_IRQHandler,        // 54: UART2
    DMA1_Stream6_IRQHandler,  // 33: DMA1 channel 6
};
```
- **FreeRTOS uses 3 of these:** SysTick (tick), PendSV (context switch), SVC (first task start)
- **Exercise:** Open `startup_stm32f407xx.s` in your project. Find the vector table. Count how many entries are defined.

**3. NVIC Priority System (60 min)**
- STM32F4 has 4 bits of priority (0–15), where 0 = HIGHEST priority
- Priority grouping: split bits into preemption priority + sub-priority
- Preemption: higher-priority ISR can interrupt a lower-priority ISR (nesting)
- Sub-priority: determines order when two ISRs of same preemption priority are pending
- **FreeRTOS constraint:** Only ISRs with priority ≥ `configMAX_SYSCALL_INTERRUPT_PRIORITY` (typically 5) can call FreeRTOS `FromISR` APIs. ISRs with priority 0–4 are "above" FreeRTOS and CANNOT use any RTOS API.
- **Exercise:** Configure 3 interrupts at priorities 3, 5, and 7. Show that priority 3 preempts priority 5. Show that priority 5 can call `xSemaphoreGiveFromISR()` but priority 3 cannot.

## Day 2: ISR Safety Rules & Critical Sections (3 hours)

**1. ISR Safety Rules (45 min)**

| ❌ NEVER Do in ISR | ✅ Do Instead | Why |
|---|---|---|
| `printf()` | `ITM_SendChar()` or set a flag | printf blocks UART (ms) |
| `malloc()` / `pvPortMalloc()` | Use pre-allocated buffers | Mutex → deadlock |
| `vTaskDelay()` | — | ISR can't block/sleep |
| `xSemaphoreTake()` | `xSemaphoreGiveFromISR()` | Can only GIVE, not TAKE |
| `xQueueSend()` | `xQueueSendFromISR()` | Must use `FromISR` variant |
| Long computation (> 10 µs) | Set flag, defer to task | Delays other interrupts |

**2. Critical Sections (60 min)**
- **`__disable_irq()` / `__enable_irq()`:** Global interrupt disable. Simple but brutal — disables ALL interrupts, even higher-priority ones. Use only for very short critical sections (< 10 µs).
- **`taskENTER_CRITICAL()` / `taskEXIT_CRITICAL()`:** FreeRTOS wrapper. Disables interrupts up to `configMAX_SYSCALL_INTERRUPT_PRIORITY` but allows higher-priority interrupts. Supports nesting (counter-based).
- **`taskENTER_CRITICAL_FROM_ISR()` / `taskEXIT_CRITICAL_FROM_ISR()`:** For use inside ISRs.
- **RTOSTwin usage:** `snapshot_capture()` disables interrupts while copying task states to ensure consistency (no context switch mid-read). Must be < 50 µs.
- **Exercise:** Measure the overhead of `taskENTER_CRITICAL()` + `taskEXIT_CRITICAL()` (should be < 1 µs). Then measure a `memcpy(dst, src, N)` inside a critical section for N = 100, 500, 1000 bytes. At what N does the critical section duration exceed 50 µs?

**3. Shared Variable Patterns (60 min)**
- **Single flag (ISR → task):** `volatile uint8_t flag`. ISR sets to 1. Task polls, clears to 0. Simple but wastes CPU polling.
- **Binary semaphore (ISR → task):** ISR calls `xSemaphoreGiveFromISR()`. Task blocks on `xSemaphoreTake()`. Efficient (task sleeps while waiting).
- **Queue (ISR → task):** ISR pushes data into queue. Task pops. Best for buffered data.
- **Exercise:** Implement all 3 patterns for a button press handler. Measure CPU usage of polling vs semaphore vs queue approach.

## Day 3: Real-World Interrupt Scenarios (2 hours)

**1. UART Receive Interrupt**
- Each received byte triggers an ISR → push byte into circular buffer → task processes buffer
- **Exercise:** Configure UART2 RX interrupt. Receive a string character by character. Reconstruct in buffer.

**2. Timer Interrupt**
- Generate a periodic 1 kHz interrupt (every 1 ms)
- Use for precise timing, LED toggling, sensor sampling schedule
- **Exercise:** Set up TIM2 to generate 1 kHz interrupts. In the ISR, increment a counter. Print the counter value — should increase by 1000 per second.

**3. DMA Completion Interrupt**
- When DMA finishes transferring N bytes (e.g., UART TX complete), it fires an interrupt
- **This is how RTOSTwin knows a packet has been sent** and can start the next one
- **Exercise:** Set up UART TX with DMA. When DMA complete ISR fires, set a flag. In the main task, wait for the flag, then load the next buffer.

## Day 4: Review & Teach VNV
- [ ] Explain the full interrupt flow (hardware stacking, vector table lookup, ISR entry/exit)
- [ ] Recite ISR safety rules from memory (6 rules)
- [ ] Demo: Button ISR toggles LED even while main loop has a 5-second delay
- [ ] Explain: Why does `snapshot_capture()` use a critical section? What's the max allowed duration?

---

# WEEK 5 — UART Protocol

## Day 1: UART Theory (3 hours)

**1. Signal Format (45 min)**
- Idle line = HIGH. Start bit = LOW. 8 data bits (LSB first). Stop bit = HIGH.
- Each bit duration = 1 / baud_rate seconds
- At 115200 baud: 1 bit = 8.68 µs. One byte frame (10 bits including start/stop) = 86.8 µs
- Oversampling: UART samples each bit 16 times, takes majority vote at center → noise immunity
- **Exercise:** At 9600 baud, what is the bit duration? How long to send "Hello\r\n" (7 bytes)?

**2. Baud Rate & Bandwidth Calculation (45 min)**
- Throughput = baud_rate / 10 bytes/sec (10 bits per byte: 1 start + 8 data + 1 stop)
- Common rates: 9600 (legacy), 115200 (default debug), 921600 (fast), 1000000 (1 Mbaud)
- RTOSTwin bandwidth budget:
  - Full snapshot: ~350 bytes. At 10 Hz = 3,500 B/s. At 115200 baud (11,520 B/s) = 30% bandwidth usage → TOO MUCH
  - Delta packet: ~20-50 bytes. At 10 Hz = 200-500 B/s = 1.7-4.3% → PERFECT
- **Exercise:** Calculate maximum snapshot rate at 115200 baud if full snapshot = 350 bytes. Then recalculate with delta encoding (avg 30 bytes).

**3. UART Registers on STM32 (45 min)**
- `USART_SR` (Status Register): TXE (transmit empty), RXNE (receive not empty), TC (transmission complete)
- `USART_DR` (Data Register): write to transmit, read to receive
- `USART_BRR` (Baud Rate Register): clock divider
- **Polling transmit:** Write byte to DR. Wait for TXE. Repeat. (BLOCKING — CPU stuck waiting)
- **Interrupt transmit:** Enable TXE interrupt. ISR writes next byte from buffer. (Better — CPU free between bytes)
- **DMA transmit:** Point DMA at buffer. Start. CPU completely free until DMA done interrupt. (BEST)
- **Exercise:** Implement all 3 transmit methods. Measure CPU cycles consumed for each when sending 100 bytes.

## Day 2: Packet Framing for RTOSTwin (3 hours)

**1. Why Framing? (30 min)**
- Raw UART is just a stream of bytes. Where does one packet end and the next begin?
- If the receiver misses a byte, ALL subsequent data is misinterpreted (frame shift)
- Solution: Frame each packet with a defined structure

**2. RTOSTwin Packet Format (60 min)**
```
┌──────┬──────┬──────┬────────┬─────────────────┬──────────┐
│ SYNC │ SYNC │ TYPE │ SEQ    │ LENGTH │ PAYLOAD │ CRC16    │
│ 0xAA │ 0x55 │ 1B   │ 1B     │ 2B     │ N bytes │ 2B       │
└──────┴──────┴──────┴────────┴────────┴─────────┴──────────┘

SYNC:    0xAA 0x55 — unique pattern to find packet start
TYPE:    0x01=Full Snapshot, 0x02=Delta, 0x03=ACK, 0x04=Heartbeat
SEQ:     Sequence number (0-255, wraps). Detects lost packets.
LENGTH:  Payload length in bytes (little-endian uint16)
PAYLOAD: The actual data (snapshot or delta)
CRC16:   CRC-16-CCITT over TYPE+SEQ+LENGTH+PAYLOAD
```

- **Sync recovery:** If receiver is in an unknown state, it scans for 0xAA 0x55. The probability of this pattern occurring randomly in data is 1/65536 (0.0015%).
- **Sequence number:** If receiver gets SEQ 5 then SEQ 7, it knows packet 6 was lost.
- **Exercise:** Implement `packet_frame(type, payload, payload_len, output_buffer)` that wraps a payload with the header and CRC. Implement `packet_parse(input_buffer, len, payload_out)` that validates sync, CRC, and extracts payload.

**3. Error Handling Strategies (60 min)**
- CRC mismatch → discard packet, increment error counter
- Missing sequence number → request retransmit OR accept gap (configurable)
- Timeout → no data for > 500 ms → mark twin as "stale", alert dashboard
- **Exercise:** Simulate packet loss: send 100 packets, drop every 10th, verify receiver detects gaps via sequence numbers.

## Day 3: UART + DMA (2–3 hours)

**1. DMA UART Transmit Flow (60 min)**
1. Telemetry task fills `tx_buffer[256]` with a framed packet
2. Call `HAL_UART_Transmit_DMA(&huart2, tx_buffer, packet_len)`
3. DMA controller transfers bytes to UART without CPU involvement
4. When DMA complete, `HAL_UART_TxCpltCallback()` fires
5. Telemetry task can now refill the buffer with the next packet

**2. Double-Buffering (60 min)**
- Problem: While DMA is sending buffer A, you can't modify it. But you need to capture the next snapshot NOW.
- Solution: Two buffers. DMA sends from A while you fill B. When A finishes, swap.
- **Exercise:** Implement double-buffered UART DMA transmit. Verify timing: DMA transfer should overlap with next snapshot capture.

## Day 4: Review & Teach VNV
- [ ] Draw the UART signal for the byte 0x55 including start/stop bits
- [ ] Explain the RTOSTwin packet format (each field's purpose)
- [ ] Demo: Send and receive packets over UART with CRC verification
- [ ] Explain: Why DMA is mandatory for < 2% CPU overhead

---

# WEEKS 6–12 — Condensed Roadmap (Will Expand When You Reach Them)

## Week 6: DMA Engine
- DMA controller architecture, streams, channels, priorities
- Circular mode vs normal mode
- Memory-to-memory, memory-to-peripheral, peripheral-to-memory
- UART TX DMA, ADC DMA, double-buffering patterns
- **Deliverable:** UART DMA transmit working with circular buffer integration

## Week 7: RTOS Fundamentals — Tasks
- Super-loop limitations, preemptive vs cooperative scheduling
- `xTaskCreate()`, task priorities, `vTaskDelay()` vs `vTaskDelayUntil()`
- Task states (Ready, Running, Blocked, Suspended), state transitions
- Idle task, timer task, FreeRTOS internals (linked lists, context switch magic)
- **Deliverable:** Multi-task application with 4+ tasks at different priorities

## Week 8: RTOS — Semaphores, Mutexes & Synchronization
- Binary semaphore (signaling), counting semaphore (resource counting)
- Mutex (mutual exclusion), recursive mutex
- Priority inversion explained (Mars Pathfinder bug), priority inheritance
- Deadlock: what it is, how to prevent it (lock ordering, try-lock with timeout)
- **Deliverable:** ISR → semaphore → task pattern, mutex-protected shared resource

## Week 9: RTOS — CPU Measurement & Diagnostics
- Idle hook method for CPU usage calculation
- `vTaskGetRunTimeStats()` with DWT cycle counter as high-resolution clock
- `uxTaskGetStackHighWaterMark()` for stack monitoring
- FreeRTOS trace hooks (`traceTASK_SWITCHED_IN/OUT`) for custom instrumentation
- **Deliverable:** CPU usage and stack watermark reporting over UART

## Week 10: RTOSTwin — Snapshot Engine
- Implement `snapshot.h` / `snapshot.c` with full struct hierarchy
- Implement `snapshot_capture()` with critical section and static buffers
- Measure execution time with DWT (target < 150 µs)
- Implement `snapshot_init()`, integrate with telemetry task
- **Deliverable:** Working snapshot capture at 10 Hz, verified overhead < 2%

## Week 11: RTOSTwin — Transport Layer
- Implement `transport.h` / `transport.c` with packet framing
- Integrate circular buffer TX queue with DMA UART
- Implement sequence numbering and CRC verification
- Implement heartbeat and loss detection
- **Deliverable:** Reliable telemetry stream from MCU to PC

## Week 12: RTOSTwin — Twin State Manager
- C++ `TwinState` class with thread-safe state database
- Ring buffer for history storage (N minutes of snapshots)
- State deserialization from binary packets
- Integration with Python analytics (via shared memory or pipe)
- **Deliverable:** PC-side twin receiving and storing live telemetry

---

## Progress Tracker

| Week | Topic | Status | Started | Completed | Taught VNV? |
|------|-------|--------|---------|-----------|-------------|
| 1 | C Types, Memory, Pointers | ⬜ Not Started | — | — | ⬜ |
| 2 | Number Systems, Memory Map | ⬜ Not Started | — | — | ⬜ |
| 3 | ARM Cortex-M4 Architecture | ⬜ Not Started | — | — | ⬜ |
| 4 | Interrupts & NVIC | ⬜ Not Started | — | — | ⬜ |
| 5 | UART Protocol | ⬜ Not Started | — | — | ⬜ |
| 6 | DMA Engine | ⬜ Not Started | — | — | ⬜ |
| 7 | RTOS Tasks | ⬜ Not Started | — | — | ⬜ |
| 8 | Semaphores & Mutexes | ⬜ Not Started | — | — | ⬜ |
| 9 | CPU Measurement | ⬜ Not Started | — | — | ⬜ |
| 10 | Snapshot Engine | ⬜ Not Started | — | — | ⬜ |
| 11 | Transport Layer | ⬜ Not Started | — | — | ⬜ |
| 12 | Twin State Manager | ⬜ Not Started | — | — | ⬜ |

---

**END OF RYN'S TIMELINE**
