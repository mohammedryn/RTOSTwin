# The Complete Embedded & Firmware Engineering Roadmap
## From Zero to Staff Engineer — While Building RTOSTwin

**For:** Complete beginners with no prior embedded experience  
**Duration:** 6 months (1–2 hours/day)  
**Philosophy:** Learn the concept → Apply it immediately in the RTOSTwin project → Never forget it  

---

> *"You don't learn to swim by reading about water. You get in the pool. But you should know which end is deep."*

This roadmap teaches you **every fundamental concept** an embedded/firmware engineer needs — from what a transistor does to writing production RTOS firmware — and immediately connects each concept to a real deliverable in the RTOSTwin project. By the end, you won't just have a portfolio project. You'll have the mental model of a senior firmware engineer.

---

## How This Roadmap Is Structured

Each **Module** follows this pattern:

```
📚 LEARN  → Core concept explained from scratch
🔬 LAB    → Hands-on exercise (LED blink, register read, etc.)
🏗️ BUILD  → Apply the concept directly to RTOSTwin
📝 VERIFY → How to prove you understood it (interview-ready)
```

Modules are grouped into **Tiers**:

| Tier | Name | Duration | What You Learn | RTOSTwin Milestone |
|------|------|----------|----------------|-------------------|
| 0 | **The Foundations** | Week 1–2 | C programming, binary, memory | — |
| 1 | **The Hardware** | Week 3–4 | MCU architecture, GPIO, registers | LED blink on STM32 |
| 2 | **The Peripherals** | Week 5–7 | UART, SPI, I2C, Timers, ADC, DMA | Serial output working |
| 3 | **The RTOS** | Week 8–11 | FreeRTOS tasks, queues, semaphores, mutexes | Multi-task firmware |
| 4 | **The Agent** | Week 12–16 | Snapshot, Delta Encoding, Transport | Telemetry streaming |
| 5 | **The Twin** | Week 17–20 | Host-side C++/Python, Kalman, Simulation | Live dashboard |
| 6 | **The Brain** | Week 21–23 | ML, Prediction, Time-Travel | Predictive analytics |
| 7 | **The Ship** | Week 24 | Testing, Docs, Demo | v1.0 Release |

---

# TIER 0 — THE FOUNDATIONS (Weeks 1–2)

> Before you touch any hardware, you must be fluent in C and understand how a computer actually stores and moves data. Skipping this tier is like trying to build a house without knowing what concrete is.

---

## Module 0.1: C Programming for Embedded (Not Desktop C)

**Why This Is Different From "Normal" C:**
Desktop C programmers use `printf`, `malloc`, and strings freely. Embedded C programmers treat these as dangerous weapons. On a microcontroller with 64 KB of RAM, a single careless `malloc` can kill the system.

### 📚 What You Must Learn

**Data Types & Sizes (This Matters More Than You Think):**
On your PC, an `int` is 32 bits. On some embedded targets, `int` is 16 bits. This breaks code silently.

```c
// WRONG: Non-portable
int sensor_value;       // 16 bits on MSP430, 32 bits on STM32

// RIGHT: Explicit width
#include <stdint.h>
uint16_t sensor_value;  // Always 16 bits, everywhere
int32_t  temperature;   // Always 32 bits, signed
uint8_t  register_val;  // Always 8 bits (1 byte)
```

**The `stdint.h` types you will use daily:**

| Type | Size | Range | When To Use |
|------|------|-------|-------------|
| `uint8_t` | 1 byte | 0 to 255 | Register values, flags, small counters |
| `int8_t` | 1 byte | -128 to 127 | Signed small values |
| `uint16_t` | 2 bytes | 0 to 65,535 | ADC readings, moderate counters |
| `int16_t` | 2 bytes | -32,768 to 32,767 | Temperature (°C × 100) |
| `uint32_t` | 4 bytes | 0 to 4,294,967,295 | Timestamps, addresses, large counters |
| `int32_t` | 4 bytes | ±2.1 billion | General purpose signed |
| `uint64_t` | 8 bytes | 0 to 18.4 quintillion | Microsecond timestamps |

**Bitwise Operations (The Language of Hardware):**
Every peripheral on a microcontroller is controlled by writing individual bits in special registers. You must be fluent in bit manipulation.

```c
// Setting a bit (turn ON)
register |= (1 << bit_position);     // Set bit N to 1

// Clearing a bit (turn OFF)  
register &= ~(1 << bit_position);    // Set bit N to 0

// Toggling a bit (flip)
register ^= (1 << bit_position);     // Flip bit N

// Checking a bit (read)
if (register & (1 << bit_position))  // Is bit N set?

// Example: Turn on LED on GPIO pin 5
GPIOA->ODR |= (1 << 5);   // Set bit 5 of Output Data Register

// Example: Check if button on pin 3 is pressed
if (GPIOA->IDR & (1 << 3)) {
    // Button is pressed (bit 3 is HIGH)
}
```

**Practical Bit Patterns You'll Use In RTOSTwin:**

```c
// Delta encoder: Track which fields changed using a bitmask
#define FIELD_TASKS      (1 << 0)   // Bit 0: task data changed
#define FIELD_MEMORY     (1 << 1)   // Bit 1: memory data changed
#define FIELD_PERIPHERALS (1 << 2)  // Bit 2: peripheral data changed
#define FIELD_HEALTH     (1 << 3)   // Bit 3: health data changed

uint8_t changed_fields = 0;

if (tasks_changed)
    changed_fields |= FIELD_TASKS;       // Set bit 0

if (changed_fields & FIELD_MEMORY)       // Check if bit 1 is set
    send_memory_delta();
```

**Pointers (Your Best Friend & Worst Enemy):**
In embedded, pointers are not abstract. They point to **physical hardware addresses.**

```c
// On your PC, this is just a variable:
int x = 42;
int *p = &x;     // p holds the address of x

// On an STM32, this is a HARDWARE REGISTER:
volatile uint32_t *GPIOA_ODR = (volatile uint32_t *)0x40020014;
*GPIOA_ODR |= (1 << 5);  // PHYSICALLY turns on an LED

// The 'volatile' keyword tells the compiler:
// "This memory location can change at any time (by hardware).
//  Do NOT optimize away reads/writes to it."
```

**Why `volatile` is life-or-death:**
Without `volatile`, the compiler might see you reading a register in a loop and think: "He already read this value, it hasn't changed in the code, so I'll just reuse the cached value." But the hardware IS changing it (a button press, a timer tick, an ADC conversion). Without `volatile`, your code literally cannot see hardware events.

**Structs (How We Model Hardware):**

```c
// Model an RTOS task's state
typedef struct {
    char     name[16];        // Human-readable name
    uint8_t  state;           // 0=Ready, 1=Running, 2=Blocked
    uint8_t  priority;        // 0-255
    uint32_t stack_used;      // Bytes of stack consumed
    uint32_t stack_total;     // Total stack allocated
    uint32_t cpu_time_us;     // Total CPU time in microseconds
} task_snapshot_t;

// Model the entire system state
typedef struct {
    uint64_t         timestamp_us;
    task_snapshot_t   tasks[10];     // Up to 10 tasks
    uint32_t         heap_free;
    uint32_t         heap_total;
    uint8_t          cpu_percent;
    uint16_t         crc16;         // Error detection
} full_snapshot_t;

// Size matters! Calculate it:
// task_snapshot_t = 16 + 1 + 1 + 4 + 4 + 4 = 30 bytes (per task)
// full_snapshot_t = 8 + (30×10) + 4 + 4 + 1 + 2 = 319 bytes
```

**Static vs. Dynamic Allocation (Critical Rule):**

```c
// DESKTOP C: Dynamic allocation is normal
int *arr = malloc(100 * sizeof(int));  // Ask OS for memory at runtime
free(arr);                               // Give it back

// EMBEDDED C: Static allocation is KING
static int arr[100];  // Allocated at compile time. Always available.
                       // Never fragments. Never fails. Never leaks.

// WHY?
// 1. malloc can FAIL (returns NULL if no memory left)
// 2. malloc is SLOW (searches free list)
// 3. malloc FRAGMENTS memory (holes between allocations)
// 4. malloc needs a MUTEX (priority inversion risk in RTOS)
// 5. On a 64KB RAM MCU, every byte is precious — you need determinism
```

### 🔬 Lab 0.1: C Warm-Up Exercises

Do these on your PC (no hardware needed):

1. **Bit manipulation:** Write a function `void set_bit(uint32_t *reg, uint8_t bit)` and `void clear_bit(uint32_t *reg, uint8_t bit)`. Test with `printf`.
2. **Struct sizing:** Define the `full_snapshot_t` struct above. Print `sizeof(full_snapshot_t)`. Is it what you expected? (Hint: look up "struct padding".)
3. **CRC-16:** Implement a CRC-16-CCITT function. This is the exact function you'll use in RTOSTwin for packet integrity.
4. **Circular buffer:** Implement a `circular_buffer_t` with `push()` and `pop()` using a fixed-size array. This is the transmit queue for your telemetry agent.

### 📝 Interview-Ready Knowledge

After this module, you can answer:
- *"Why do we use `uint32_t` instead of `int` in embedded?"*
- *"What does `volatile` do and when is it necessary?"*
- *"Why is `malloc` dangerous in embedded systems?"*

---

## Module 0.2: Number Systems & Memory

### 📚 What You Must Learn

**Binary, Hex, and Why Engineers Think in Powers of 2:**

```
Decimal:  255
Binary:   1111 1111    (8 bits = 1 byte)
Hex:      0xFF         (each hex digit = 4 bits)

Decimal:  3,735,928,559
Binary:   1101 1110 1010 1101 1011 1110 1110 1111
Hex:      0xDEADBEEF   (classic debug pattern!)
```

**Memory Map (How a Microcontroller Sees the World):**

On your PC, memory is just RAM. On a microcontroller, the address space contains *everything* — RAM, Flash, and hardware peripherals — all mapped to specific addresses.

```
STM32F4 Memory Map:
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
0x0000_0000 ┌─────────────────────┐
            │ Flash (Code)         │  1 MB — Your compiled firmware
            │ (Read-only at        │         lives here permanently
            │  runtime)            │
0x0800_0000 ├─────────────────────┤
            │ ...                  │
0x2000_0000 ├─────────────────────┤
            │ SRAM (Data)          │  192 KB — Variables, stack, heap
            │ (Read/Write)         │           This is your "RAM"
0x2003_0000 ├─────────────────────┤
            │ ...                  │
0x4000_0000 ├─────────────────────┤
            │ Peripherals          │  GPIO, UART, SPI, I2C, Timers
            │ (Memory-Mapped I/O)  │  Writing to 0x40020014 turns
            │                      │  on a PHYSICAL LED!
0x5000_0000 ├─────────────────────┤
            │ ...                  │
0xE000_0000 ├─────────────────────┤
            │ System Peripherals   │  NVIC (interrupts), SysTick,
            │ (ARM Core)           │  Debug (DWT cycle counter)
0xFFFF_FFFF └─────────────────────┘
```

**The Stack & The Heap (Where Bugs Hide):**

```
RAM Layout:
┌──────────────┐ 0x2003_0000 (top of RAM)
│              │
│   Stack      │ ↓ Grows DOWN
│   (Local     │   Function calls push frames here
│    variables,│   Each RTOS task has its OWN stack
│    return    │
│    addresses)│
├──────────────┤ ← Stack Pointer (SP)
│              │
│   (Free)     │ ← If stack meets heap = CRASH
│              │
├──────────────┤ ← Heap end
│   Heap       │ ↑ Grows UP
│   (malloc)   │   Dynamic allocations come from here
├──────────────┤
│   .bss       │   Uninitialized global variables (zeroed)
│   .data      │   Initialized global variables
│   .text      │   (This is in Flash, not RAM)
└──────────────┘ 0x2000_0000 (bottom of RAM)
```

**Stack Overflow — The Silent Killer:**
If a task uses more stack than allocated, the stack pointer grows past its boundary into another task's memory or the heap. This corrupts data silently. The system doesn't crash immediately — it crashes *later*, at a seemingly random point, with symptoms that make no sense. This is why **RTOSTwin monitors stack usage** — it's the single most common cause of embedded field failures.

### 🏗️ Connection to RTOSTwin

- The `stack_used` field in `task_snapshot_t` directly measures how close each task is to stack overflow.
- The `heap_free` field monitors the heap for leaks.
- Your **Stack Overflow Predictor** (Week 13 of the project) exists because of this exact failure mode.

---

## Module 0.3: Build Systems & Toolchains

### 📚 What You Must Learn

**The Compilation Pipeline (Desktop vs. Embedded):**

```
Desktop (GCC on Linux):
  main.c → [Preprocessor] → [Compiler] → [Linker] → a.out (runs on your OS)

Embedded (ARM GCC Cross-Compiler):
  main.c → [arm-none-eabi-gcc] → [Linker + Linker Script] → firmware.elf → firmware.bin
                                       ↑
                                  Linker script tells the linker:
                                  "Put code at 0x0800_0000 (Flash)"
                                  "Put variables at 0x2000_0000 (RAM)"
                                  "Stack starts at 0x2003_0000"
```

**Key Difference:** On your PC, the operating system (Windows/Linux) loads your program into memory. On a microcontroller, there IS no operating system (yet — that's what FreeRTOS is). The linker script IS the memory layout.

**Tools You'll Use:**

| Tool | Purpose | Desktop Equivalent |
|------|---------|-------------------|
| `arm-none-eabi-gcc` | Cross-compiler for ARM | `gcc` |
| `arm-none-eabi-gdb` | Debugger | `gdb` |
| `OpenOCD` / `ST-Link` | Flash firmware to the chip | N/A (no equivalent) |
| `STM32CubeIDE` | IDE with built-in tools | VS Code |
| `CMake` / `Makefile` | Build system | Same |
| `minicom` / `PuTTY` | Serial terminal (read UART) | N/A |

### 🔬 Lab 0.3: Setup Your Environment

1. **Install STM32CubeIDE** (free from ST Microelectronics). This bundles the compiler, debugger, and flasher.
2. **Install PuTTY or Tera Term** (serial terminal for Windows).
3. **Create a blank STM32F4 project** in CubeIDE. Build it. If it compiles, you're ready.
4. **If you have an STM32 board:** Flash the default blinky example. See the LED blink. This proves your toolchain works end-to-end.

---

# TIER 1 — THE HARDWARE (Weeks 3–4)

> Now you learn what a microcontroller actually *is* and how to make it do things in the physical world.

---

## Module 1.1: Microcontroller Architecture

### 📚 What You Must Learn

**What's Inside an STM32F4:**

```
┌─────────────────────────────────────────────────┐
│                   STM32F407                       │
│                                                   │
│  ┌──────────┐  ┌──────────┐  ┌──────────────┐   │
│  │ ARM       │  │ Flash    │  │ SRAM          │   │
│  │ Cortex-M4 │  │ 1 MB     │  │ 192 KB        │   │
│  │ 168 MHz   │  │ (Code)   │  │ (Data)        │   │
│  │           │  │          │  │               │   │
│  │ • 32-bit  │  └──────────┘  └──────────────┘   │
│  │ • FPU     │                                    │
│  │ • DSP     │  ┌──────────┐  ┌──────────────┐   │
│  └──────────┘  │ DMA ×2    │  │ NVIC          │   │
│                │ (Auto     │  │ (Interrupt    │   │
│                │  transfer)│  │  Controller)  │   │
│                └──────────┘  └──────────────┘   │
│                                                   │
│  ┌────────────────────────────────────────────┐   │
│  │ Peripherals                                 │   │
│  │ • GPIO (pins)     • UART ×6 (serial)       │   │
│  │ • SPI ×3          • I2C ×3                 │   │
│  │ • Timers ×14      • ADC ×3 (analog input)  │   │
│  │ • DAC ×2          • USB, CAN, Ethernet     │   │
│  └────────────────────────────────────────────┘   │
│                                                   │
│  ┌─────────────────────────┐                      │
│  │ Clock System (RCC)       │                      │
│  │ HSE (8MHz crystal)       │                      │
│  │  → PLL → 168 MHz        │                      │
│  └─────────────────────────┘                      │
└─────────────────────────────────────────────────┘
```

**Key Concepts:**

| Concept | What It Is | Why It Matters |
|---------|-----------|----------------|
| **CPU Core** | The brain. Executes your C code, one instruction at a time. | Your code runs HERE. 168 MHz = 168 million instructions/sec. |
| **Flash** | Non-volatile storage. Your compiled firmware lives here. | Survives power off. Read-only at runtime. |
| **SRAM** | Volatile memory. Variables, stack, heap. | Lost on power off. This is your "RAM budget" (192 KB). |
| **GPIO** | General Purpose Input/Output. Physical pins. | Connect LEDs, buttons, sensors. The bridge to the real world. |
| **UART** | Serial communication. Send/receive bytes over a wire. | How your agent sends telemetry to the PC. |
| **DMA** | Direct Memory Access. Automatic data transfer without CPU. | Sends UART data while CPU does other work. Critical for < 2% overhead. |
| **NVIC** | Interrupt controller. Routes hardware events to your code. | Button press, timer tick, UART byte received → your function runs. |
| **Clock (RCC)** | Controls speed of CPU and peripherals. | Wrong clock = everything runs at wrong speed. |

### 🏗️ Connection to RTOSTwin
- **SRAM** = Your 192 KB budget. The agent must fit in < 10 KB of this.
- **UART** = How telemetry data reaches the PC.
- **DMA** = How you achieve < 2% overhead (UART sends without CPU involvement).
- **NVIC** = How interrupts work (critical for understanding RTOS context switching).

---

## Module 1.2: GPIO — Your First Hardware Interaction

### 📚 What You Must Learn

**GPIO = General Purpose Input/Output**
Each pin on the microcontroller can be configured as:
- **Output:** You control the voltage (HIGH = 3.3V, LOW = 0V). Used for LEDs, relays.
- **Input:** You read the voltage from outside. Used for buttons, sensors.

**Registers That Control GPIO:**

```c
// Every GPIO port (A through K) has these registers:
// MODER   — Mode (Input, Output, Alternate Function, Analog)
// OTYPER  — Output type (Push-Pull or Open-Drain)
// OSPEEDR — Speed (Low, Medium, Fast, High)
// PUPDR   — Pull-up / Pull-down resistor
// IDR     — Input Data Register (READ pin state)
// ODR     — Output Data Register (WRITE pin state)
// BSRR    — Bit Set/Reset Register (atomic set/clear)

// Example: Configure PA5 as output (LED on Nucleo board)
// Step 1: Enable clock to GPIOA
RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

// Step 2: Set PA5 as output (MODER bits [11:10] = 01)
GPIOA->MODER &= ~(3 << (5 * 2));  // Clear bits
GPIOA->MODER |=  (1 << (5 * 2));  // Set to output mode

// Step 3: Turn LED ON
GPIOA->ODR |= (1 << 5);   // Set bit 5 HIGH

// Step 4: Turn LED OFF
GPIOA->ODR &= ~(1 << 5);  // Set bit 5 LOW

// Step 5: Toggle LED
GPIOA->ODR ^= (1 << 5);   // Flip bit 5
```

**HAL vs. Bare Metal:**

```c
// BARE METAL (direct register access):
GPIOA->ODR |= (1 << 5);           // Fast: ~2 CPU cycles

// HAL (Hardware Abstraction Layer from ST):
HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);  // Slow: ~20 cycles
                                                        // But portable & readable
```

For RTOSTwin, you'll use **HAL** for peripheral setup (convenience) and **bare metal** for performance-critical paths (snapshot capture).

### 🔬 Lab 1.2: LED Blink (The "Hello World" of Embedded)

1. Create a new STM32 project in CubeIDE.
2. Configure PA5 as GPIO Output in the CubeMX pin configurator.
3. In `main.c`, inside the `while(1)` loop:
   ```c
   HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
   HAL_Delay(500);  // 500ms delay
   ```
4. Flash to the board. Watch the LED blink at 1 Hz.
5. **Challenge:** Make it blink at exactly 2 Hz. Then 10 Hz. Then 100 Hz. At what frequency can you no longer see it blinking? (This teaches you about human perception vs. machine speed.)

### 📝 Interview-Ready Knowledge
- *"Explain the difference between ODR and BSRR for GPIO output."*
  - ODR is read-modify-write (non-atomic: if an interrupt fires between read and write, you get a race condition).
  - BSRR is write-only and atomic (single bus write sets or clears a pin with no read-modify-write hazard).

---

## Module 1.3: Interrupts — The Heartbeat of Real-Time Systems

### 📚 What You Must Learn

**What Is An Interrupt?**
An interrupt is a hardware signal that **stops** whatever the CPU is doing, saves its state, runs a special function (ISR — Interrupt Service Routine), and then resumes where it left off.

```
Normal execution:         With interrupt:
                          
main() {                  main() {
  step_1();                 step_1();
  step_2();  ←── GPIO IRQ fires!
  step_3();                   ↓
}                           [Save CPU state to stack]
                              ↓
                            ISR() {
                              handle_button_press();
                            }
                              ↓
                            [Restore CPU state]
                              ↓
                            step_2();  ← Resumes exactly here
                            step_3();
                          }
```

**Why Interrupts Matter for RTOSTwin:**
- FreeRTOS uses interrupts for **context switching**. The SysTick timer fires every 1 ms. The ISR checks if a higher-priority task is ready and switches to it. This is how multitasking works.
- Your telemetry snapshot must be safe even if an interrupt fires mid-capture. This is why we use **critical sections** (`taskENTER_CRITICAL()` disables interrupts).

**ISR Rules (Memorize These):**

| Rule | Why |
|------|-----|
| Keep ISRs SHORT (< 10 µs) | Long ISRs delay other interrupts and break timing |
| No `printf` in ISR | `printf` is slow and may use locks |
| No `malloc` in ISR | `malloc` takes a mutex → deadlock risk |
| No blocking calls (delays, semaphore waits) | ISR cannot "wait" — it must return immediately |
| Use `volatile` for shared variables | Compiler may cache values otherwise |
| Use `FromISR` variants of FreeRTOS APIs | Regular APIs are NOT safe to call from ISRs |

### 🔬 Lab 1.3: Button Interrupt

1. Configure a GPIO pin (e.g., PC13 on Nucleo = blue button) as External Interrupt.
2. Write an ISR that toggles the LED:
   ```c
   void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
       if (GPIO_Pin == GPIO_PIN_13) {
           HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
       }
   }
   ```
3. **Observe:** The LED toggles instantly on button press, even if `main()` is doing something else.
4. **Challenge:** Add a `HAL_Delay(5000)` in `main()`. Does the button still respond? (Yes! That's the power of interrupts.)

---

# TIER 2 — THE PERIPHERALS (Weeks 5–7)

> These are the communication channels between your MCU and the outside world. UART is the most critical for RTOSTwin.

---

## Module 2.1: UART — Serial Communication

### 📚 What You Must Learn

**UART = Universal Asynchronous Receiver/Transmitter**
The simplest way for two devices to talk. Uses 2 wires: TX (transmit) and RX (receive).

```
STM32            PC (USB-UART Adapter)
┌────┐           ┌────┐
│ TX ├──────────→│ RX │
│ RX │←──────────┤ TX │
│ GND├──────────→│ GND│
└────┘           └────┘
```

**Key Parameters:**
- **Baud Rate:** Speed in bits/second. Common: 9600, 115200, 921600.
- **Data bits:** Usually 8.
- **Stop bits:** Usually 1.
- **Parity:** Usually None.
- **Notation:** "115200 8N1" = 115200 baud, 8 data bits, No parity, 1 stop bit.

**Bandwidth Calculation (Critical for RTOSTwin):**

```
At 115200 baud, 8N1:
  Each byte = 1 start + 8 data + 1 stop = 10 bits
  Throughput = 115200 / 10 = 11,520 bytes/sec

  Full snapshot = 350 bytes
  At 10 Hz: 3,500 bytes/sec = 30.4% of bandwidth ← TOO MUCH
  With delta encoding (~20 bytes): 200 bytes/sec = 1.7% ← PERFECT
```

**Three Ways to Send UART Data:**

| Method | CPU Usage | Complexity | Use In RTOSTwin? |
|--------|-----------|------------|-----------------|
| **Polling** | CPU waits for each byte | Simple | ❌ No (wastes CPU) |
| **Interrupt** | ISR fires per byte | Medium | ⚠️ Maybe (ISR overhead) |
| **DMA** | Hardware sends entire buffer | Complex setup | ✅ Yes (zero CPU during transfer) |

### 🔬 Lab 2.1: "Hello World" Over UART

1. Configure UART2 at 115200 baud in CubeMX (UART2 is connected to the ST-Link USB on Nucleo boards).
2. Send a string:
   ```c
   char msg[] = "Hello from STM32!\r\n";
   HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 100);
   ```
3. Open PuTTY/Tera Term on the correct COM port at 115200 baud. See the message.
4. **Challenge:** Send the value of a counter that increments every second. Format: `"Count: 42\r\n"`.

### 🏗️ Connection to RTOSTwin
This is the **exact** communication channel your telemetry agent will use. In Week 12, you'll replace `"Hello"` with binary telemetry packets.

---

## Module 2.2: Timers — Precision Timing

### 📚 What You Must Learn

**Timers are free-running hardware counters.** They count clock cycles and can:
- Generate periodic interrupts (e.g., every 1 ms — this is how `HAL_Delay` works).
- Measure elapsed time (e.g., how long a function takes).
- Generate PWM signals (e.g., control LED brightness or motor speed).

**SysTick Timer:**
ARM Cortex-M has a built-in 24-bit timer called SysTick. FreeRTOS uses this as its heartbeat:
- Fires every 1 ms (configurable).
- The ISR (`xPortSysTickHandler`) increments the tick count and checks if any tasks need to wake up.
- This is the most important timer in any RTOS system.

**DWT Cycle Counter (Performance Measurement):**
The Data Watchpoint and Trace unit has a 32-bit cycle counter:

```c
// Enable the cycle counter
CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
DWT->CYCCNT = 0;
DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

// Measure function execution time
uint32_t start = DWT->CYCCNT;
snapshot_capture(&snapshot);
uint32_t end = DWT->CYCCNT;

uint32_t cycles = end - start;
float microseconds = (float)cycles / 168.0f;  // At 168 MHz

printf("snapshot_capture took %lu cycles (%.1f us)\r\n", cycles, microseconds);
```

This is **exactly** how you'll verify that your agent meets the < 150 µs target.

### 🔬 Lab 2.2: Measure Execution Time

1. Enable DWT cycle counter.
2. Measure how long `HAL_GPIO_TogglePin()` takes. (Expect ~20 cycles.)
3. Measure how long a `for(int i=0; i<1000; i++)` empty loop takes.
4. Calculate: at 168 MHz, how many microseconds is 1000 cycles?

---

## Module 2.3: DMA — Zero-Cost Data Transfer

### 📚 What You Must Learn

**DMA = Direct Memory Access**
A hardware engine that copies data between memory and peripherals **without CPU involvement.**

```
WITHOUT DMA (CPU does everything):
  CPU reads byte from memory → writes to UART register → waits → repeats
  Cost: ~10 cycles per byte × 350 bytes = 3,500 cycles

WITH DMA (CPU sets it up, hardware does the rest):
  CPU configures DMA: source=buffer, dest=UART, length=350 → done
  DMA engine handles all 350 bytes automatically
  Cost: ~100 cycles (setup only) → CPU is FREE for 350 byte times!
```

**Why DMA Is Non-Negotiable for RTOSTwin:**
At 115200 baud, transmitting 350 bytes takes 30 ms. If the CPU is busy sending bytes for 30 ms out of every 100 ms, that's **30% CPU overhead** just for UART! With DMA, it's < 0.01%.

### 🔬 Lab 2.3: UART with DMA

1. In CubeMX, enable DMA for UART2 TX.
2. Replace `HAL_UART_Transmit()` with `HAL_UART_Transmit_DMA()`:
   ```c
   HAL_UART_Transmit_DMA(&huart2, (uint8_t*)buffer, length);
   // CPU is immediately free! DMA handles the transfer.
   ```
3. **Verify:** Toggle a GPIO pin before and after the DMA call. Measure the pulse width on a scope / logic analyzer. It should be < 5 µs (just the setup time).

---

## Module 2.4: SPI & I2C (Sensor Communication)

### 📚 What You Must Learn

**I2C (Inter-Integrated Circuit):**
- 2 wires: SDA (data) + SCL (clock)
- Multi-device bus (multiple sensors on the same 2 wires)
- Slower (100 kHz – 400 kHz typical)
- Used for: temperature sensors, accelerometers, displays

**SPI (Serial Peripheral Interface):**
- 4 wires: MOSI, MISO, SCK, CS
- Faster (up to 50+ MHz)
- Each device needs its own CS (chip select) line
- Used for: Flash memory, high-speed ADCs, SD cards

**Why You Need This:**
RTOSTwin monitors `i2c_transactions` and `spi_transactions` in the `peripheral_snapshot_t`. Understanding these protocols tells you what those numbers mean.

### 🔬 Lab 2.4: Read a Sensor Over I2C

1. Connect an I2C temperature sensor (e.g., LM75, TMP102, or BME280).
2. Read the temperature register and print it over UART.
3. **Connection to RTOSTwin:** This sensor reading is what an application task does. Your telemetry agent monitors the task *that reads the sensor* — not the sensor directly.

---

## Module 2.5: ADC — Reading Analog Signals

### 📚 What You Must Learn

**ADC = Analog-to-Digital Converter**
Converts a voltage (0–3.3V) to a digital number (0–4095 for 12-bit).

```
Voltage → ADC → Digital Value
0.0V    → ADC → 0
1.65V   → ADC → 2048
3.3V    → ADC → 4095

Formula: voltage = (adc_value / 4095) × 3.3
```

**Why It Matters:**
Many embedded systems read sensors via ADC (current sensors, temperature thermistors, potentiometers). The RTOSTwin project's industrial motor controller example uses current sensing via ADC.

---

# TIER 3 — THE RTOS (Weeks 8–11)

> This is where everything changes. You go from bare-metal (one thing at a time) to an operating system that runs multiple tasks "simultaneously." This is the foundation of RTOSTwin.

---

## Module 3.1: Why RTOS?

### 📚 What You Must Learn

**The Problem With Bare-Metal (Super Loop):**

```c
// Bare-metal approach: one big loop
while (1) {
    read_sensor();       // Takes 5ms
    process_data();      // Takes 10ms
    update_display();    // Takes 20ms
    check_button();      // Takes 1ms
    send_telemetry();    // Takes 15ms
    // Total loop: 51ms → Max response to button: 51ms (TOO SLOW)
}
```

Problems:
1. Button response time = worst case 50 ms (all other functions must run first).
2. If `read_sensor()` hangs (sensor disconnected), **entire system freezes.**
3. Can't prioritize (the display update shouldn't delay sensor reading).

**The Solution: An RTOS**

```c
// RTOS approach: independent tasks with priorities
void sensor_task(void *p)    { while(1) { read_sensor();     vTaskDelay(5);  } }
void control_task(void *p)   { while(1) { process_data();    vTaskDelay(10); } }
void display_task(void *p)   { while(1) { update_display();  vTaskDelay(50); } }
void button_task(void *p)    { while(1) { check_button();    vTaskDelay(1);  } }
void telemetry_task(void *p) { while(1) { send_telemetry();  vTaskDelay(100);} }

// Each task runs independently
// Higher priority tasks preempt lower priority ones
// Button task responds in 1ms regardless of what else is happening
```

**What FreeRTOS Does:**
1. **Scheduler:** Decides which task runs right now (highest priority ready task).
2. **Context Switching:** Saves one task's CPU registers, loads another's. Happens in ~5 µs.
3. **Timing:** Tracks delays and timeouts via SysTick interrupts.
4. **Synchronization:** Provides tools for tasks to communicate safely (queues, semaphores, mutexes).

---

## Module 3.2: FreeRTOS Tasks

### 📚 What You Must Learn

**Creating a Task:**

```c
void my_task(void *parameters) {
    // Setup (runs once)
    int counter = 0;

    // Main loop (runs forever)
    while (1) {
        counter++;
        printf("Counter: %d\n", counter);

        vTaskDelay(pdMS_TO_TICKS(1000));  // Sleep for 1 second
        // During this sleep, OTHER tasks run!
    }
}

// In main():
xTaskCreate(
    my_task,        // Function pointer
    "MyTask",       // Name (for debugging)
    256,            // Stack size (in words, not bytes! 256 words = 1024 bytes)
    NULL,           // Parameters
    2,              // Priority (higher number = higher priority)
    NULL            // Task handle (optional)
);

vTaskStartScheduler();  // Start FreeRTOS! Never returns.
```

**Task States:**

```
                    ┌──────────┐
         ┌────────→│  READY   │←────────┐
         │         └────┬─────┘         │
         │              │               │
    vTaskResume    Scheduler picks  Timer/Event
         │         this task            occurs
         │              │               │
         │              ↓               │
    ┌────┴─────┐   ┌──────────┐   ┌────┴─────┐
    │SUSPENDED │   │ RUNNING  │   │ BLOCKED  │
    └──────────┘   └────┬─────┘   └──────────┘
                        │               ↑
                   vTaskDelay()    vTaskDelay()
                   xQueueReceive()     or
                   xSemaphoreTake() Waiting for
                        │          queue/semaphore
                        └───────────────┘
```

**This is exactly what `task_snapshot_t.state` captures in RTOSTwin.** Your twin mirrors these state transitions in real time.

---

## Module 3.3: Queues — Inter-Task Communication

### 📚 What You Must Learn

**Problem:** Task A produces data. Task B needs to consume it. How?

**Wrong Way: Shared Global Variable**
```c
volatile int shared_data;  // DANGEROUS: race conditions!
```

**Right Way: FreeRTOS Queue**
```c
QueueHandle_t data_queue = xQueueCreate(10, sizeof(int));  // 10-element queue

// Producer task
void producer(void *p) {
    int value = 42;
    xQueueSend(data_queue, &value, portMAX_DELAY);  // Blocks if queue full
}

// Consumer task
void consumer(void *p) {
    int received;
    xQueueReceive(data_queue, &received, portMAX_DELAY);  // Blocks if queue empty
    printf("Got: %d\n", received);
}
```

### 🏗️ Connection to RTOSTwin
Your telemetry agent uses a queue (or circular buffer) as the transmit buffer. The snapshot task pushes encoded packets. The transport task pops and sends them over UART.

---

## Module 3.4: Semaphores & Mutexes — Synchronization

### 📚 What You Must Learn

**Mutex (Mutual Exclusion):**
Ensures only one task accesses a shared resource at a time.

```c
SemaphoreHandle_t uart_mutex = xSemaphoreCreateMutex();

void task_a(void *p) {
    xSemaphoreTake(uart_mutex, portMAX_DELAY);  // Lock
    printf("Task A printing\n");                 // Safe!
    xSemaphoreGive(uart_mutex);                  // Unlock
}

void task_b(void *p) {
    xSemaphoreTake(uart_mutex, portMAX_DELAY);  // Waits if A has the lock
    printf("Task B printing\n");
    xSemaphoreGive(uart_mutex);
}
```

**Binary Semaphore:**
Used for signaling between a task and an ISR.

```c
SemaphoreHandle_t button_sem = xSemaphoreCreateBinary();

// ISR: Signal that button was pressed
void HAL_GPIO_EXTI_Callback(uint16_t pin) {
    BaseType_t woken = pdFALSE;
    xSemaphoreGiveFromISR(button_sem, &woken);  // NOTE: FromISR variant!
    portYIELD_FROM_ISR(woken);
}

// Task: Wait for button press
void button_handler_task(void *p) {
    while (1) {
        xSemaphoreTake(button_sem, portMAX_DELAY);  // Sleeps until ISR signals
        process_button_event();
    }
}
```

**Priority Inversion (The Pathfinder Bug):**
In 1997, the Mars Pathfinder rover kept resetting because of priority inversion:
- Low priority task took a mutex.
- High priority task needed the same mutex → blocked.
- Medium priority task preempted the low priority task → LOW PRIORITY NEVER RELEASES THE MUTEX.
- High priority task starved → Watchdog reset.

**Fix:** FreeRTOS `xSemaphoreCreateMutex()` implements **priority inheritance** — when a high-priority task blocks on a mutex held by a low-priority task, the low-priority task temporarily inherits the high priority.

This is why **RTOSTwin avoids mutexes in the telemetry path** — we use lock-free circular buffers instead.

---

## Module 3.5: Memory Management in FreeRTOS

### 📚 What You Must Learn

**FreeRTOS provides 5 heap implementations:**

| Allocator | Features | Use Case |
|-----------|----------|----------|
| `heap_1` | Allocate only (no free) | Simplest, safest; create tasks at startup, never delete |
| `heap_2` | Best-fit, no coalescing | Small allocations of same size |
| `heap_3` | Wraps C library `malloc` | When you want standard `malloc` behavior |
| `heap_4` | First-fit with coalescing | **Most common.** Good for varied allocation sizes. |
| `heap_5` | Like heap_4, across non-contiguous memory regions | Advanced (multiple RAM banks) |

**For RTOSTwin:** Your Memory Allocator Simulator (Week 7) models `heap_4` behavior — first-fit search with free block coalescing.

### 🏗️ Connection to RTOSTwin
- `xPortGetFreeHeapSize()` → `memory_snapshot_t.heap_free`
- `xPortGetMinimumEverFreeHeapSize()` → Worst-case heap usage (ever)
- Your leak detector watches `heap_free` decrease over time.

---

## Module 3.6: CPU Usage Measurement

### 📚 What You Must Learn

**Idle Task Hook Method:**
FreeRTOS runs the idle task when no other task is ready. If we measure how much time the idle task runs, the rest is CPU usage.

```c
// In FreeRTOSConfig.h:
#define configUSE_IDLE_HOOK 1

// Global counter
volatile uint32_t idle_count = 0;

void vApplicationIdleHook(void) {
    idle_count++;  // Increments when CPU has nothing to do
}

// In your measurement task (runs every 1 second):
void cpu_measurement_task(void *p) {
    while (1) {
        uint32_t idle_start = idle_count;
        vTaskDelay(pdMS_TO_TICKS(1000));
        uint32_t idle_end = idle_count;

        uint32_t idle_ticks = idle_end - idle_start;
        // Compare to a calibration value (idle_ticks when CPU is 100% idle)
        uint8_t cpu_percent = 100 - (idle_ticks * 100 / calibration_value);
    }
}
```

This is exactly how your `health_snapshot_t.cpu_utilization` is computed.

### 🔬 Lab 3.6: Build a Multi-Task FreeRTOS Application

Create a project with:
1. **Task 1 (Priority 3):** Reads a simulated sensor value every 100 ms, puts it in a queue.
2. **Task 2 (Priority 2):** Takes values from the queue, processes them (running average).
3. **Task 3 (Priority 1):** Prints results over UART every 1 second.
4. **Idle Hook:** Measures CPU usage.

**This is your "application" that RTOSTwin will monitor.**

---

# TIER 4 — THE AGENT (Weeks 12–16)
*(This maps directly to Weeks 1–4 of the original project roadmap)*

At this point, you have all the foundational knowledge. You now build the RTOSTwin telemetry agent using everything you learned:

| Module | You Learned | You Now Build |
|--------|-------------|---------------|
| C types & structs | → | `full_snapshot_t` structure definitions |
| Bitwise operations | → | Delta encoder change bitmask |
| `volatile`, pointers | → | Register-level snapshot capture |
| UART + DMA | → | Transport layer with zero-CPU transmission |
| Interrupts & critical sections | → | ISR-safe snapshot capture |
| FreeRTOS tasks & queues | → | Telemetry task + transmit queue |
| Heap management | → | Memory monitoring (`xPortGetFreeHeapSize`) |
| DWT cycle counter | → | Overhead measurement (< 150 µs target) |

**Refer to the [Project Roadmap](file:///d:/digital_twin/roadmap/roadmap.md) Weeks 1–4 for detailed implementation guidance.**

---

# TIER 5 — THE TWIN (Weeks 17–20)
*(Maps to Weeks 5–10 of the project roadmap)*

Now you move to the **host side** (PC). New skills:

| New Skill | Why | Where Used |
|-----------|-----|-----------|
| **C++ classes** | Model the RTOS simulator as objects | Scheduler sim, Memory sim |
| **Python data science** | numpy, scipy for regression | Leak detector, stack predictor |
| **WebSockets** | Push real-time data to browser | Dashboard backend |
| **React basics** | Build the dashboard UI | Task timeline, memory graph |
| **Linear Algebra** | Kalman filter math | State synchronizer |

---

# TIER 6 — THE BRAIN (Weeks 21–23)
*(Maps to Weeks 11–18 of the project roadmap)*

| New Skill | Why | Where Used |
|-----------|-----|-----------|
| **Linear regression** | Detect trends in time series | Memory leak detector |
| **Polynomial fitting** | Predict non-linear growth | Stack overflow predictor |
| **scikit-learn** | Train anomaly detection models | Isolation Forest |
| **Statistics** | Z-scores, confidence intervals | Anomaly explainability |

---

# TIER 7 — THE SHIP (Week 24)
*(Maps to Weeks 23–24 of the project roadmap)*

| New Skill | Why | Where Used |
|-----------|-----|-----------|
| **Doxygen** | Auto-generate API docs from comments | Documentation |
| **Technical writing** | Blog posts, README, papers | Publication |
| **Video production** | Record and edit demo video | Final demo |
| **Git + CI/CD** | GitHub Actions for automated testing | Release pipeline |

---

# Recommended Resources

## Books (In Order)

| # | Book | When | Why |
|---|------|------|-----|
| 1 | **"Making Embedded Systems" — Elecia White** | Week 1–2 | Best beginner embedded book. Practical, approachable. |
| 2 | **"Mastering STM32" — Carmine Noviello** | Week 3–7 | Comprehensive STM32 guide. Covers every peripheral. |
| 3 | **"Mastering the FreeRTOS Real Time Kernel" — Richard Barry** | Week 8–11 | Written by the FreeRTOS creator. The definitive guide. Free PDF on freertos.org. |
| 4 | **"The Art of Electronics" — Horowitz & Hill** | Reference | The hardware bible. Read chapters as needed. |
| 5 | **"Computer Architecture" — Patterson & Hennessy** | Reference | Deep understanding of CPU internals. |

## Online Courses

| Resource | Topics | Link |
|----------|--------|------|
| **Fastbit Embedded Brain Academy** (Udemy) | STM32 bare metal, FreeRTOS | Udemy (Kiran Nayak's courses) |
| **Digikey's Introduction to RTOS** (YouTube) | FreeRTOS from scratch | Free on YouTube |
| **Phil's Lab** (YouTube) | PCB design, STM32, embedded projects | Free on YouTube |
| **Ben Eater** (YouTube) | Computer architecture from transistors | Free on YouTube |

## Datasheets To Read

| Document | Pages to Focus On |
|----------|-------------------|
| STM32F4 Reference Manual (RM0090) | GPIO (Ch. 8), UART (Ch. 30), DMA (Ch. 10), RCC (Ch. 7) |
| STM32F4 Datasheet | Pin mapping, electrical characteristics |
| ARM Cortex-M4 Technical Reference Manual | Exception model, SysTick, DWT |
| FreeRTOS API Reference | `xTaskCreate`, `xQueueSend`, `xSemaphoreTake` |

---

# Summary: The Learning Path at a Glance

```
WEEK  1-2:  C programming, binary, memory, toolchain setup
WEEK  3-4:  GPIO, LED blink, interrupts, button handling
WEEK  5-7:  UART, SPI, I2C, Timers, DMA, ADC
WEEK  8-11: FreeRTOS tasks, queues, semaphores, mutexes, heap, CPU measurement
WEEK 12-16: BUILD RTOSTwin Agent (snapshot, delta encoder, transport)
WEEK 17-20: BUILD RTOSTwin Twin (C++ simulator, Kalman filter, dashboard)
WEEK 21-23: BUILD RTOSTwin Brain (ML anomaly detection, predictive analytics)
WEEK   24:  SHIP (documentation, demo video, publication)
```

**By Week 24, you will have:**
1. Mastered every major embedded peripheral (GPIO, UART, SPI, I2C, DMA, ADC, Timers).
2. Built production-quality FreeRTOS firmware from scratch.
3. Designed a binary communication protocol with integrity checking.
4. Implemented a Kalman filter for real-time state estimation.
5. Trained and deployed an ML model for anomaly detection.
6. Built a full-stack real-time dashboard (React + WebSocket + Node.js).
7. Published a technical paper / blog post.
8. Created a GitHub portfolio project that demonstrates staff-level systems thinking.

---

*You don't need to know everything before you start. You need to start — and learn what you need, when you need it.*

---

**END OF LEARNING ROADMAP**
