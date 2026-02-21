# Week 1, Day 2 — Memory Layout, Pointers & The Stack
## RYN's Engineering Coach Notes

**Topic:** From Source Code to Physical Memory — .text/.data/.bss, Stack Frames, Heap, Pointers as Wires  
**Platform:** STM32F407 (ARM Cortex-M4, 168 MHz, 192 KB SRAM, 1 MB Flash)  
**Project Connection:** RTOSTwin RAM Budget, `snapshot_capture()` static buffers, ISR stack safety  

---

> [!IMPORTANT]
> **The mental model shift that separates embedded engineers from desktop software engineers:** On a PC, "memory" is a convenient abstraction managed by the OS. On the STM32F407, memory is a set of physical silicon cells at fixed, documented addresses. When you write `uint32_t *p = (uint32_t*)0x40020014; *p = 0x20;`, you are physically changing a voltage on a metal wire connected to a transistor gate that controls a GPIO pin. There is no indirection. There is no OS. You ARE the OS.

---

# 1️⃣ Motivation & System Context

## The Stack Overflow That Saved a Life — and the One That Didn't

**NASA Mars Pathfinder (1997):** After landing on Mars, the software started resetting. Root cause: priority inversion caused the RTOS to skip a higher-priority task. That task's stack was overwritten by a lower-priority task's overflow. The telemetry system shut down. Engineers on Earth had to patch the firmware remotely from 60 million kilometers away.

**Toyota Unintended Acceleration (2009-2011):** An independent analysis by safety experts found that the throttle control task's stack was undersized by the engineers. Under certain RTOS scheduling conditions, the stack overflowed into adjacent memory, corrupting a variable that controlled the throttle. People died. Toyota paid $1.2 billion in settlements.

In both cases, the root engineer knew C. They did not know what the stack actually is at the hardware level.

## RTOSTwin Budget Context

The `snapshot_capture()` function is the heart of RTOSTwin. It must:
- Collect states of 10 tasks (`TaskStatus_t task_buf[10]` = 360 bytes)
- Use a persistent delta comparison buffer (`full_snapshot_t previous` = ~360 bytes)
- Keep a TX queue (`uint8_t tx_queue[4096]` = 4 KB)
- Do all of this in < 10 KB total RAM

If you put `task_buf[10]` on the stack (inside the function), you consume 360 bytes of the task's stack on every call. If the stack is 1 KB total (512 words × 2 bytes), that's 36% gone from a single buffer, leaving 640 bytes for the rest of the call chain. This **will overflow** when `uxTaskGetSystemState()` is called internally.

If you declare it `static`, it lives in `.bss` at a fixed address forever — zero stack cost, zero runtime allocation cost, zero failure mode.

**This is the core trade-off you must understand today.**

---

# 2️⃣ Foundational Theory — First Principles

## 2.1 The Compilation Pipeline Creates Memory Sections

When you compile `snapshot.c`, the compiler does not produce a single blob of binary. It produces structured **sections**, each with a specific role:

```
Source Code (.c)
      │
      ▼  arm-none-eabi-gcc −S
Assembly (.s)                  ← Human-readable, still organized into sections
      │
      ▼  arm-none-eabi-as
Object File (.o)               ← Binary, section headers describe content
      │
      ▼  arm-none-eabi-ld  (+ linker script .ld)
ELF Binary (.elf)              ← All .o files merged, sections placed at final addresses
      │
      ▼  arm-none-eabi-objcopy
Raw Binary (.bin)              ← What gets flashed to 0x08000000
```

The linker script controls WHERE each section ends up. For STM32F407:

```
MEMORY {
  FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 1024K
  RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
}
```

## 2.2 The Four Sections — Physical Meaning

### `.text` — Machine Code in Flash

Every compiled function becomes a sequence of ARM Thumb-2 instructions in `.text`. These live in Flash (0x08000000+). Flash is **read-only at runtime** — the CPU can't write to Flash during normal execution (it needs a special erase-program cycle).

```c
// This entire function becomes binary machine code in .text:
void delay_ms(uint32_t ms) {
    HAL_Delay(ms);
}
// Binary in .text:
// 0x08001234: PUSH  {R7, LR}         (save R7 and LR)
// 0x08001236: SUB   SP, SP, #8       (allocate 8 bytes on stack for function frame)
// 0x08001238: BL    HAL_Delay        (call HAL_Delay, R0 already has 'ms' from caller)
// 0x0800123C: ADD   SP, SP, #8       (deallocate stack frame)
// 0x0800123E: POP   {R7, PC}         (restore R7, pop LR into PC → returns to caller)
```

Note: `PUSH {R7, LR}` saves the **link register** (LR = return address). When the function was called, the CPU put the return address into LR automatically. The function pushes it to the stack so it can use LR for calling `HAL_Delay`. `POP {R7, PC}` loads the saved LR value into PC, which causes the CPU to jump back to the caller.

### `.rodata` — Constants in Flash

Read-only data. `const` variables, string literals, lookup tables. Cannot be modified at runtime.

```c
const char product_name[] = "RTOSTwin v1.0";  // In .rodata (Flash)
const uint8_t crc_lut[256] = { ... };          // In .rodata (Flash) — 256 bytes
const uint32_t GRAVITY_X10 = 98;               // In .rodata (Flash)

// This looks like RAM but it's NOT — it's stored in Flash, no copy to RAM
// Reading product_name[0] reads from Flash address (0x08XXXXXX)

// Danger: You CANNOT do:
// product_name[0] = 'r';   // Read-only → HardFault exception!
```

**Lookup table optimization for CRC:** The CRC computation loop hits the `crc_lut` array each iteration. Since this is Flash, the ART Accelerator's instruction cache must also cache data accesses. For time-critical code, copy lookup tables to RAM first.

### `.data` — Initialized Globals in RAM

Variables that have initial values AND must be modifiable at runtime. They need to live in RAM, but their initial values must be stored somewhere — so they're stored in Flash and **copied to RAM at startup**.

```
Flash                         RAM
0x08XXXXX: [42][00][00][00]   0x20000000: [42][00][00][00]
           (stored here)      ←  (copied here by startup code)
           int initialized = 42;
```

**The hidden Flash cost:** If you declare `uint8_t big_buf[4096] = {0}`, the compiler stores FOUR THOUSAND ZERO BYTES in Flash to be copied to RAM at boot. That costs 4 KB of Flash for what is essentially just `memset(buf, 0, 4096)`.

Startup code (in `startup_stm32f407xx.s`) does exactly this copy:
```asm
/* Copy .data from Flash to RAM */
    LDR  R0, =_sdata        ; Destination: start of .data in RAM
    LDR  R1, =_edata        ; End of .data in RAM
    LDR  R2, =_sidata       ; Source: initial values in Flash
.copy_loop:
    CMP  R0, R1             ; While dest < end:
    ITT  LT
    LDRLT R3, [R2], #4      ;   Load 4 bytes from Flash
    STRLT R3, [R0], #4      ;   Store 4 bytes to RAM
    BLT  .copy_loop
```

### `.bss` — Zero-Initialized Globals in RAM (No Flash Cost)

Variables without initial values (or explicitly initialized to zero). The startup code just `memset`s this region to zero — so there's NO Flash copy. This is the critical insight:

```c
// .bss — NO Flash cost, just zeroed at startup:
static uint8_t tx_queue[4096];         // 4 KB in .bss
static TaskStatus_t task_buf[10];      // ~360 bytes in .bss
uint32_t snapshot_count;               // 4 bytes in .bss
static full_snapshot_t prev_snapshot;  // ~360 bytes in .bss

// .data — Flash cost + RAM cost (avoid for large buffers):
uint8_t tx_queue[4096] = {0};          // 4 KB Flash + 4 KB RAM = DOUBLE WASTE!
```

Startup code zeros `.bss`:
```asm
/* Zero .bss */
    LDR R0, =_sbss           ; Start of .bss
    LDR R1, =_ebss           ; End of .bss
    MOV R2, #0
.zero_loop:
    CMP R0, R1
    ITT LT
    STRLT R2, [R0], #4       ; Store 0, advance
    BLT .zero_loop
```

## 2.3 Physical RAM Layout for RTOSTwin (Full Map)

```
0x20000000  ┌──────────────────────────────────────────────────────┐
            │  .data (initialized globals, copied from Flash)       │
            │  ~2 KB of application data                            │
            ├──────────────────────────────────────────────────────┤
            │  .bss (zeroed at startup)                             │
            │  tx_queue[4096]                   4,096 bytes         │
            │  task_buf[10] (TaskStatus_t)        360 bytes         │
            │  prev_snapshot (full_snapshot_t)    360 bytes         │
            │  snapshot_count (uint32_t)            4 bytes         │
            │  FreeRTOS ready list, timer list     ~512 bytes       │
            │  Other globals                       ~1 KB            │
            │                                   ──────────          │
            │  Total .bss:                        ~7 KB             │
            ├──────────────────────────────────────────────────────┤
            │  FreeRTOS HEAP (pvPortMalloc pool)                    │
            │  heap_4 managed by FreeRTOS (configTOTAL_HEAP_SIZE)   │
            │  Contains:                                            │
            │    SensorTask TCB + stack (2 KB)                      │
            │    TelemetryTask TCB + stack (2 KB)                   │
            │    ControlTask TCB + stack (4 KB)                     │
            │    MonitorTask TCB + stack (1 KB)                     │
            │    IdleTask TCB + stack (512 B)                       │
            │    TimerTask TCB + stack (1 KB)                       │
            │    Queue objects, semaphores, mutexes                  │
            │  Total heap: ~80 KB                                   │
            ├──────────────────────────────────────────────────────┤
            │  (free space)                       ~93 KB            │
            ├──────────────────────────────────────────────────────┤
            │  MSP (Main Stack Pointer) region                      │
            │  ISR stacks, boot stack (before RTOS starts)          │
            │  ~2 KB                                                │
0x2001_FFFF └──────────────────────────────────────────────────────┘
                                ← Stack grows down from 0x2001FFFF
```

> [!NOTE]
> Each FreeRTOS task has its own **Task Control Block (TCB)** stored in the heap. The TCB contains the task state (priority, list pointers) and the **saved context** (register values) when the task is not running. When FreeRTOS switches tasks, it saves the CPU registers of the current task INTO that task's TCB, then loads the registers from the next task's TCB. This is the context switch.

## 2.4 The Stack — Detailed Mechanics

### Stack Frame Anatomy (ARM Cortex-M4 ABI)

When a function is called:

1. The **caller** places the first 4 arguments in R0, R1, R2, R3
2. Returns address is stored in **LR** (Link Register, R14)
3. The **callee** MAY save registers it wants to use (**callee-saved**: R4–R11)
4. The callee allocates space for local variables by **subtracting from SP**
5. When done, the callee restores R4–R11, restores SP, and branches to LR

```
Before call to foo():
┌─────────────────────────┐  ← SP (Stack Pointer)
│   (previous frame)      │
└─────────────────────────┘

After foo() pushes {R4, R5, LR} and allocates 16 bytes for locals:
┌─────────────────────────┐  ← SP (was here before call)
│   saved R4 (4 bytes)    │
│   saved R5 (4 bytes)    │
│   saved LR (4 bytes)    │  ← return address to whoever called foo()
│   local_var_a (4 bytes) │
│   local_var_b (4 bytes) │
│   local_arr[2] (8 bytes)│  ← SP is now 16+12=28 bytes below caller's SP
└─────────────────────────┘  ← SP (current)
```

### Full Call Chain Example for RTOSTwin

```
SRAM top (0x2001FFFF)
│
│  [TelemetryTask stack — 2 KB allocation]
│
│  snapshot_capture() frame:
│    → local: uint32_t start_cycles   (4 bytes)
│    → local: uint8_t changed_fields  (1 byte + 3 pad)
│    → saved: R4, R5, R6, R7, LR      (20 bytes)
│    TOTAL PER CALL: ~28 bytes
│
│  uxTaskGetSystemState() frame (called from snapshot_capture):
│    → internal buffer (if using stack-based impl): 320 bytes (!)
│    → saved: R4-R11                               (32 bytes)
│    TOTAL: ~352 bytes consumed during this call
│
│  [FreeRTOS overhead when entering uxTaskGetSystemState]:
│    → Scheduler list iteration: minimal stack (< 64 bytes)
│
│  [Free stack space — must never reach this]:
│  [If it does: StackType_t fill = 0xA5A5A5A5 boundary is crossed]
│
└─ (bottom of TelemetryTask stack — if SP goes past here → overflow)
```

**If you put `TaskStatus_t task_buf[10]` as a local variable in `snapshot_capture()`:**
- Stack consumed = 10 × sizeof(TaskStatus_t) ≈ 10 × 36 = 360 bytes
- PLUS `uxTaskGetSystemState()` uses its own 352+ bytes
- PLUS `snapshot_capture()` itself: 28 bytes
- Total: 740+ bytes of stack consumed at peak for ONE function call
- For a 1 KB (1024-byte) stack: only 284 bytes remain — barely enough for anything

**If you use `static TaskStatus_t task_buf[10]`:**
- Stack consumed = 28 bytes (just the function's own frame)
- `task_buf` is in `.bss` — not on any stack — no risk

### Stack Overflow Detection: FreeRTOS Watermark

FreeRTOS fills each task's stack with `0xA5` (`0xA5A5A5A5`) when the task is created.

When `uxTaskGetStackHighWaterMark(task_handle)` is called, FreeRTOS scans from the **bottom** of the stack upward, counting consecutive `0xA5` bytes until it finds a non-`0xA5` byte. The number of unmodified bytes = the **watermark** = how much stack was never used.

```
Task Stack (growing downward, shown from bottom to top):
[A5][A5][A5][A5][A5]  ← Never touched — watermark zone
[A5][A5][A5][A5][A5]
[A5][A5][A5][A5][A5]  ← First non-0xA5 found here ← minimum SP ever reached
[xx][xx][xx][xx][xx]  ← Used by the deepest function call ever
[xx][xx][xx][xx][xx]
[xx][xx][xx][xx][xx]  ← Current stack data
         ▲
         SP (current)
```

`uxTaskGetStackHighWaterMark()` returns the number of 4-byte words that have NEVER been used. If it returns 0 or a very small number, the stack is dangerously close to overflow.

```c
// In RTOSTwin monitor task:
void health_monitor_task(void *params) {
    while (1) {
        UBaseType_t telem_wm = uxTaskGetStackHighWaterMark(telemetry_task_handle);
        health_snapshot.stack_min_free_words = telem_wm;

        if (telem_wm < 64) {  // Less than 64 words (256 bytes) remaining
            // ALERT: Stack overflow imminent!
            trigger_alert(ALERT_STACK_WARNING, "TelemetryTask");
        }

        vTaskDelay(pdMS_TO_TICKS(1000));  // Check every second
    }
}
```

## 2.5 The Heap — Formal Model

### Heap_4 Free List Structure (Default in FreeRTOS)

The heap is a large block of RAM (`uint8_t heap[configTOTAL_HEAP_SIZE]`). FreeRTOS manages it as a linked list of free blocks.

**Initial State:**
```
Heap array: [BlockHeader | ........................ free ........................ ]
            └── BlockLink {size=total, next=NULL}
```

**After `pvPortMalloc(100)` (request 100 bytes, aligned to 16 bytes → 112):**
```
[BlockHeader | 112 bytes (allocated) | BlockHeader | ... remaining free ... ]
↑                                    ↑
returned to caller (+ sizeof header) new free block starts here
```

**After many alloc/free cycles — fragmentation:**
```
[FREE 50B][USED 200B][FREE 30B][USED 100B][FREE 80B][USED 400B][FREE 60B]
         Total free = 50+30+80+60 = 220 bytes
         But: malloc(150) FAILS — no single free block ≥ 150 bytes!
```

**Heap_4 coalescing:** When you `vPortFree()`, if the adjacent blocks are also free, heap_4 merges them into one larger block. Heap_1 and heap_2 never merge. Heap_3 is just a wrapper around standard `malloc/free`. Heap_5 supports non-contiguous memory regions.

### Why `pvPortMalloc()` Is Forbidden in `snapshot_capture()`

1. **Non-deterministic:** Walking the free list can take O(N) time where N = number of free blocks. With 50 free blocks, that's 50 pointer dereferences = ~200 extra cycles. With 500 blocks (fragmented heap) = 2000 extra cycles. Unpredictable.

2. **Requires a mutex:** FreeRTOS heap is protected by a critical section or semaphore. If your high-priority task calls `pvPortMalloc()`, it may block waiting for a lower-priority task to release the heap lock → **priority inversion**.

3. **Can return NULL:** If heap is exhausted or fragmented, malloc returns `NULL`. If your code doesn't check for `NULL` (most embedded code doesn't), you dereference a null pointer → HardFault.

4. **Flash fragmentation is permanent:** Small allocations that are never freed create "holes" in the heap that grow over time. A system that has been running for 72 hours may fail to allocate 1 KB even though it could on startup.

---

# 3️⃣ Deep Dive — Engineering Depth

## 3.1 Pointers as Hardware Wires — The Exact Mechanism

On ARM Cortex-M, **every read and write goes on the AHB bus** regardless of whether the address points to SRAM, Flash, or a peripheral register. The bus itself figures out which physical silicon gets the transaction.

```
CPU executes: LDR R0, [R1]   (R1 = 0x40020014 = GPIOA→ODR)
                    │
                    ▼
            CPU puts 0x40020014 on AHB address bus
                    │
                    ▼
            AHB Bus Decoder checks: 0x40020014 is in range 0x40020000-0x400203FF
                    │
                    ▼
            Routes transaction to GPIOA peripheral block in silicon
                    │
                    ▼
            GPIOA block reads its internal 32-bit register (ODR) and puts value on data bus
                    │
                    ▼
            CPU captures value from data bus → stores in R0
```

**This exact sequence happens when you write `GPIOA->ODR`** — only it's a store (STR) instead of a load (LDR). That store physically changes a digital output flip-flop inside the peripheral, which drives the GPIO pad to 0V or 3.3V.

There is zero abstraction. Zero indirection. One ARM instruction = one electrical event.

### Memory-Mapped I/O: The Struct-as-Register Map

ST's HAL library defines the entire GPIOA peripheral as a C struct:

```c
typedef struct {
    volatile uint32_t MODER;    /* Mode register                */  // offset 0x00
    volatile uint32_t OTYPER;   /* Output type register         */  // offset 0x04
    volatile uint32_t OSPEEDR;  /* Output speed register        */  // offset 0x08
    volatile uint32_t PUPDR;    /* Pull-up/pull-down register   */  // offset 0x0C
    volatile uint32_t IDR;      /* Input data register          */  // offset 0x10
    volatile uint32_t ODR;      /* Output data register         */  // offset 0x14
    volatile uint32_t BSRR;     /* Bit set/reset register       */  // offset 0x18
    volatile uint32_t LCKR;     /* Configuration lock register  */  // offset 0x1C
    volatile uint32_t AFR[2];   /* Alternate function registers */  // offset 0x20, 0x24
} GPIO_TypeDef;

#define GPIOA_BASE  0x40020000UL
#define GPIOA       ((GPIO_TypeDef *) GPIOA_BASE)
```

Therefore:
- `&GPIOA->MODER`   = `(uint32_t*)(0x40020000 + 0x00)` = `0x40020000`
- `&GPIOA->ODR`     = `(uint32_t*)(0x40020000 + 0x14)` = `0x40020014`
- `&GPIOA->BSRR`    = `(uint32_t*)(0x40020000 + 0x18)` = `0x40020018`
- `&GPIOA->AFR[0]`  = `(uint32_t*)(0x40020000 + 0x20)` = `0x40020020`

**Every `volatile` in the struct is non-optional.** The compiler must re-read `GPIOA->IDR` every time you access it in code because the hardware changes it behind the CPU's back (GPIO input levels change based on pin voltage, not CPU activity).

## 3.2 Pointer Arithmetic — The Exact Rules

**Rule:** `ptr + n` advances the pointer by `n × sizeof(*ptr)` bytes, not by `n` bytes.

```c
uint8_t  *p8  = (uint8_t *)0x20000000;
uint16_t *p16 = (uint16_t*)0x20000000;
uint32_t *p32 = (uint32_t*)0x20000000;

p8  + 1 → 0x20000001 (advanced 1 byte)
p16 + 1 → 0x20000002 (advanced 2 bytes)
p32 + 1 → 0x20000004 (advanced 4 bytes)
```

**Proof from the generated assembly:**
```c
uint32_t *p = array;
uint32_t val = *(p + 3);   // Access 4th element

// ARM assembly (with -O2):
LDR R0, [R1, #12]          // R1 = base address, #12 = 3 × 4 bytes offset
```

The compiler multiplied `3 × sizeof(uint32_t)` = `3 × 4` = 12 automatically.

**Array indexing is exactly pointer arithmetic:**
```c
uint32_t arr[10];
arr[5]        ≡  *(arr + 5)        → offset = 5 × 4 = 20 bytes
arr[5] = 99;  compiled to:  STR R0, [R1, #20]
```

### The Buffer Serialization Pattern (Used by the Transport Layer)

```c
uint8_t output_buffer[256];
uint8_t *wp = output_buffer;          // wp = "write pointer"

// Write packet header manually:
*wp++ = 0xAA;                         // SYNC1 — advances by 1 byte
*wp++ = 0x55;                         // SYNC2
*wp++ = PKT_TYPE_DELTA;              // TYPE (1 byte)
*wp++ = sequence_number++;           // SEQ  (1 byte)

// Write 2-byte length (little-endian):
uint16_t payload_len = 30;
*wp++ = (uint8_t)(payload_len >> 0);  // LSB first (little-endian, ARM native)
*wp++ = (uint8_t)(payload_len >> 8);  // MSB second

// Write payload (e.g., a delta struct):
memcpy(wp, &delta_payload, payload_len);
wp += payload_len;

// Write CRC:
uint16_t crc = crc16_ccitt(output_buffer + 2, wp - output_buffer - 2);
*wp++ = (uint8_t)(crc >> 0);          // CRC LSB
*wp++ = (uint8_t)(crc >> 8);          // CRC MSB

// Total bytes written:
uint16_t total_len = (uint16_t)(wp - output_buffer);
// wp - output_buffer is pointer subtraction → gives element count (bytes since wp is uint8_t*)
```

**`wp - output_buffer` is pointer subtraction:** Result is the NUMBER OF ELEMENTS between the two pointers = the number of bytes written. This is pointer arithmetic in reverse.

## 3.3 The `hexdump()` Function — Building Your #1 Debug Tool

A `hexdump` prints the raw bytes of any memory region — exactly what the CPU sees. This is essential for:
- Verifying packet framing (are the sync bytes in the right place?)
- Debugging struct padding (seeing the 0x00 padding bytes)
- Checking endianness (is `0x12345678` stored as `78 56 34 12` or `12 34 56 78`?)
- Validating CRC over the right bytes

```c
void hexdump(const void *data, size_t len) {
    const uint8_t *ptr = (const uint8_t *)data;   // Cast to byte pointer
    size_t i;

    for (i = 0; i < len; i++) {
        // At the start of each 16-byte row: print the offset
        if (i % 16 == 0) {
            if (i > 0) {
                // Print ASCII column for the previous row
                printf("  |");
                for (size_t j = i - 16; j < i; j++) {
                    printf("%c", (ptr[j] >= 0x20 && ptr[j] < 0x7F) ? ptr[j] : '.');
                }
                printf("|\n");
            }
            printf("%04zx  ", i);   // Print row offset
        }

        printf("%02x ", ptr[i]);    // Print byte as two hex digits

        // Extra space in the middle of a row (visual separation)
        if (i % 16 == 7) printf(" ");
    }

    // Pad the final incomplete row
    size_t remaining = len % 16;
    if (remaining != 0) {
        for (size_t pad = remaining; pad < 16; pad++) {
            printf("   ");
            if (pad == 7) printf(" ");
        }
        printf("  |");
        for (size_t j = len - remaining; j < len; j++) {
            printf("%c", (ptr[j] >= 0x20 && ptr[j] < 0x7F) ? ptr[j] : '.');
        }
        printf("|\n");
    } else if (len > 0) {
        printf("  |");
        for (size_t j = len - 16; j < len; j++) {
            printf("%c", (ptr[j] >= 0x20 && ptr[j] < 0x7F) ? ptr[j] : '.');
        }
        printf("|\n");
    }
}

/*
Example output for a packet buffer:
0000  aa 55 01 03  1a 00 00 01  02 00 00 00  00 00 00 00  |.U..............|
0010  00 00 00 00  00 00 00 00  00 00 a5 5a              |...........Z|
         ↑         ↑ ↑
         SYNC1/2   type seq  ← can read the packet structure directly!
*/
```

**Save this function.** You will use it in Week 5 when debugging the packet framing, in Week 10 when verifying snapshot struct integrity, and in Week 11 when troubleshooting the PC receiver.

## 3.4 Alignment and the Bus Width Constraint

ARM Cortex-M4 requires that:
- `uint16_t *` pointers be **2-byte aligned** (address divisible by 2)
- `uint32_t *` pointers be **4-byte aligned** (address divisible by 4)
- `uint64_t *` pointers be **8-byte aligned** (address divisible by 8)

An **unaligned access** on Cortex-M4 is handled in hardware (unlike Cortex-M0 where it causes a HardFault), but it takes extra bus cycles.

```c
// This is ALWAYS safe (guaranteed aligned by compiler):
uint32_t x = 42;   // x lives at some 4-aligned address in .data or stack

// This may be UNALIGNED (dangerous):
uint8_t buffer[10];
uint32_t *p = (uint32_t*)(&buffer[1]);   // buffer[1] is at offset 1 → NOT divisible by 4
uint32_t val = *p;                        // Unaligned 32-bit read → hardware handles but 2x slower
                                         // On Cortex-M0: HARDFAULT!

// Safe pattern for reading misaligned data (e.g., from a UART receive buffer):
uint32_t safe_read_u32(const uint8_t *p) {
    uint32_t result;
    memcpy(&result, p, 4);    // memcpy handles any alignment
    return result;
}
```

**In RTOSTwin:** The packet received over UART is a byte stream. Reading fields directly from the buffer using `uint16_t *` or `uint32_t *` casts may cause alignment faults on some platforms. Always use `memcpy` to extract multi-byte fields from a byte buffer.

---

# 4️⃣ Practical Firmware Implementation Insights

## 4.1 The Static Buffer Pattern — Full RTOSTwin Implementation

```c
// snapshot.c

// ALL large buffers are STATIC — they live in .bss, NEVER on any task's stack

static TaskStatus_t      task_buf[MAX_TASKS];     // ~360 bytes in .bss
static full_snapshot_t   current_snapshot;        // ~360 bytes in .bss
static full_snapshot_t   previous_snapshot;       // ~360 bytes in .bss (for delta)
static uint8_t           tx_buffer_a[PACKET_MAX]; // 256 bytes in .bss (double buffer)
static uint8_t           tx_buffer_b[PACKET_MAX]; // 256 bytes in .bss
static uint8_t           tx_queue[TX_QUEUE_SIZE]; // 4096 bytes in .bss

// Call count tracker (the "persistent local" face of static):
void snapshot_capture(full_snapshot_t *out) {
    static uint32_t call_count = 0;  // .bss — survives between calls
    call_count++;

    // Measure execution time from the very start:
    uint32_t start = DWT->CYCCNT;

    // Capture task states into the STATIC buffer (not the stack):
    UBaseType_t task_count = uxTaskGetSystemState(
        task_buf,            // Static buffer — safe
        MAX_TASKS,
        NULL
    );

    // Fill output struct from captured data:
    memset(out, 0, sizeof(*out));   // MANDATORY: zero padding bytes for memcmp
    out->timestamp_us = get_us_timestamp();

    for (UBaseType_t i = 0; i < task_count && i < MAX_TASKS; i++) {
        out->tasks[i].state    = (uint8_t)task_buf[i].eCurrentState;
        out->tasks[i].priority = (uint8_t)task_buf[i].uxCurrentPriority;
        out->tasks[i].stack_used = task_buf[i].usStackHighWaterMark;
        strncpy(out->tasks[i].name, task_buf[i].pcTaskName, 15);
    }

    out->memory.heap_free          = xPortGetFreeHeapSize();
    out->memory.heap_min_ever_free = xPortGetMinimumEverFreeHeapSize();

    // Verify we met the timing budget:
    uint32_t elapsed_cycles = DWT->CYCCNT - start;
    uint32_t elapsed_us = elapsed_cycles / (SystemCoreClock / 1000000);

    if (elapsed_us > 150) {
        // BUDGET VIOLATION — this should never happen in production
        health_snapshot.error_count++;
    }
}
```

## 4.2 Verifying Memory Placement with arm-none-eabi-nm

After building, you can inspect where every symbol landed:

```bash
arm-none-eabi-nm --size-sort --print-size --radix=d firmware.elf | grep -E "task_buf|tx_queue|current_snapshot"

# Output:
# 00000360 b task_buf         ← 'b' = .bss, 360 bytes
# 00000360 b current_snapshot ← 'b' = .bss, 360 bytes  
# 00004096 b tx_queue         ← 'b' = .bss, 4096 bytes
```

Symbol type codes:
- `b` / `B` = `.bss` (uninitialized global/static)
- `d` / `D` = `.data` (initialized global/static)
- `t` / `T` = `.text` (code)
- `r` / `R` = `.rodata` (read-only data)

**Verify that all your large buffers are lowercase `b` (`.bss`)** — not `d` (`.data`, which wastes Flash).

## 4.3 Startup Sequence — What Happens Before main()

Understanding this sequence is what separates you from developers who think the MCU "just runs main()":

```
Power-on / Reset button pressed
        │
        ▼
CPU reads initial SP value from vector table offset 0x00 (= _estack = top of RAM)
        │
        ▼
CPU reads Reset_Handler address from vector table offset 0x04
        │
        ▼
Reset_Handler() begins (in startup_stm32f407xx.s):
  1. Copy .data section from Flash to RAM   (for initialized globals)
  2. Zero .bss section                       (for uninitialized globals)
  3. Call SystemInit()                       (configure clocks, FPU)
  4. Call __libc_init_array()               (C++ static constructors, if any)
  5. Call main()
        │
        ▼
main() begins:
  1. HAL_Init()                              (configure SysTick, NVIC priorities)
  2. SystemClock_Config()                    (set PLL — already done in SystemInit, but confirmed)
  3. MX_GPIO_Init(), MX_USART2_Init(), etc. (peripheral init from CubeMX)
  4. Initialize FreeRTOS objects (queues, semaphores, tasks)
  5. vTaskStartScheduler()                   (FreeRTOS takes over, main() never returns)
        │
        ▼
FreeRTOS scheduler starts:
  1. Runs IdleTask first (lowest priority, always ready)
  2. If higher-priority task becomes ready (timer, event), switches to it
  3. SysTick_Handler fires every 1 ms → FreeRTOS tick → task delays expire → reschedule
```

**`main()` never returns** when FreeRTOS is used. `vTaskStartScheduler()` effectively replaces the CPU's execution model. If it does return (usually: heap exhaustion when creating tasks), you'll hit `__WFI()` in the default weak implementation — the CPU sleeps forever.

## 4.4 Common Pitfalls

### Pitfall 1: Large Array on Task Stack
```c
// WRONG: 1024 bytes on a 2 KB task stack — leaves only 1 KB for everything else
void snapshot_task(void *p) {
    uint8_t temp_buffer[1024];   // Stack! 50% of 2 KB stack gone immediately
    // ...
}

// CORRECT: Static — zero stack cost
void snapshot_task(void *p) {
    static uint8_t temp_buffer[1024];  // .bss — always there, never on stack
    // ...
}

// CORRECT: Smaller local, justified size
void snapshot_task(void *p) {
    uint8_t hdr[8];  // 8 bytes on stack — completely fine
    // ...
}
```

### Pitfall 2: `memset` Forgotten — Delta Encoder False Positives
```c
// WRONG: Padding bytes contain garbage → memcmp detects "changes" that aren't real
full_snapshot_t snap;   // Uninitialized — padding bytes = random stack garbage
fill_snapshot(&snap);   // Only sets named fields
// snap.tasks[0].state = 1, snap.tasks[0].priority = 2,
// BUT snap.tasks[0].__padding[0] = 0xA5 (old stack data from previous call!)

// On first call: previous_snapshot is zero. memcmp(curr, prev, sizeof) ≠ 0 → sends full snapshot
// On second call: both should be same — but if snap.__padding[0] is different this call → false delta!

// CORRECT: Always memset before filling
full_snapshot_t snap;
memset(&snap, 0, sizeof(snap));   // ALL bytes = 0, including every padding byte
fill_snapshot(&snap);
// Now: padding bytes are guaranteed 0 in ALL instances → memcmp works correctly
```

### Pitfall 3: Pointer Cast Without Alignment Check
```c
// WRONG on Cortex-M0 (HardFault) and slow on Cortex-M4:
void parse_packet(uint8_t *raw_buf) {
    packet_header_t *hdr = (packet_header_t *)raw_buf;
    // If raw_buf is byte-aligned but hdr->length is uint16_t at offset 4 → OK on M4
    // If raw_buf starts at an odd address from UART → uint32_t fields UNALIGNED on M0 → CRASH
    uint16_t len = hdr->length;
}

// CORRECT: memcpy for field extraction from byte buffers
void parse_packet(uint8_t *raw_buf) {
    packet_header_t hdr;
    memcpy(&hdr, raw_buf, sizeof(hdr));  // memcpy handles any alignment
    uint16_t len = hdr.length;
}
```

### Pitfall 4: Stack Pointer Math — Address Printing Confusion
```c
void foo(void) {
    int local_a;
    int local_b;
    int local_c;

    printf("a: %p\n", (void*)&local_a);  // e.g., 0x2001FEC0
    printf("b: %p\n", (void*)&local_b);  // e.g., 0x2001FEC4 — HIGHER than a? or LOWER?
}
```

**Counterintuitive:** Even though the stack grows downward, local variables within a single frame are laid out by the compiler in an order that MAY put `a` at a higher vs lower address than `b`. The compiler has freedom to reorder locals. What IS guaranteed: the entire frame (all locals combined) occupies a lower address range than the previous frame in the call chain.

---

# 📝 Homework

## A. Theoretical Homework

Submit answers to: `ryn/homework/theoretical/week1/day2_theory_answers.md`

**Question 1: Section Placement Analysis**

Given this C code:
```c
uint8_t  rx_buf[2048];              // = {0} NOT present
uint8_t  tx_buf[2048] = {0};        // Has initializer!
const uint8_t sync[2] = {0xAA, 0x55};
uint32_t packet_count = 0;
uint32_t uptime_ms;

void transmit(void) {
    static uint8_t encode_buf[512];
    // ...
}
```

a) For each variable/buffer, state: Which section (`.text`, `.rodata`, `.data`, `.bss`)? How many bytes of FLASH does it consume? How many bytes of RAM?  
b) Calculate the TOTAL Flash overhead and RAM overhead separately.  
c) Rewrite `tx_buf` so it eliminates its Flash overhead without changing its runtime behavior.

---

**Question 2: Stack Overflow Risk Assessment**

A task is created with `stack_depth=256` (256 words = 1024 bytes). The task call chain is:

```
telemetry_task()    → local: 8 bytes, saves R4-R7 + LR (5 × 4 = 20 bytes)
  └─ snapshot_capture() → local: 12 bytes, saves R4-R8 + LR (6 × 4 = 24 bytes) 
       └─ uxTaskGetSystemState() → internal: ~180 bytes, saves R4-R11 (32 bytes)
            └─ list_get_owner() → local: 0 bytes, saves LR (4 bytes)
```

a) Calculate the peak stack depth (sum of all frames when at maximum call depth).  
b) FreeRTOS adds an overhead of ~24 bytes (TCB context save during context switch). Add this to your calculation. Is there a stack overflow risk?  
c) If `uxTaskGetSystemState()` is fed a `static` buffer (from `.bss`) instead of a local array, which item in the list above changes? Recalculate the peak stack depth.

---

**Question 3: Heap Fragmentation Formal Model**

Model the heap usage over time in an embedded system that has been running for T hours:

Assume:
- The system allocates a 64-byte block every 10 seconds (`pvPortMalloc(64)`)
- The system allocates a 1024-byte block every 300 seconds and frees it 60 seconds later
- No other allocations occur
- Initial free heap = 20,480 bytes

a) After exactly 1 hour, how many 64-byte blocks have been allocated and NOT freed?  
b) How much total memory is consumed by those 64-byte blocks?  
c) Is there enough contiguous free space for one more 1024-byte allocation? Explain why or why not. What would `xPortGetFreeHeapSize()` return vs what `xPortGetMinimumEverFreeHeapSize()` would return at this point?  
d) How does `xPortGetMinimumEverFreeHeapSize()` — which RTOSTwin captures in `memory_snapshot.heap_min_ever_free` — help the digital twin predict heap exhaustion?

---

**Question 4: Pointer Arithmetic Verification**

```c
uint32_t arr[8] = {0x11111111, 0x22222222, 0x33333333, 0x44444444,
                   0x55555555, 0x66666666, 0x77777777, 0x88888888};
uint32_t *p = arr;
```

Without running any code, predict:
a) The value of `*(p + 3)` (in hex)
b) The byte address of `p + 3` if `p = 0x20001000`
c) The value of `(uint8_t*)(p + 3) - (uint8_t*)p`
d) If you cast p to `uint8_t *q = (uint8_t *)p`, what does `*(q + 4) + (*(q + 5) << 8) + (*(q + 6) << 16) + (*(q + 7) << 24)` evaluate to on a little-endian system? (This is how the transport layer reads a multi-byte field from a raw byte buffer.)

---

## B. Code Homework

Submit to: `ryn/homework/code/week1/day2/`

**Assignment 1: Memory Section Explorer (`memory_sections.c`)**

Write a C program that:
1. Creates one variable from each section with clear names: `g_initialized = 42` (.data), `g_uninitialized` (.bss), `const g_constant = 99` (.rodata), local variable (stack), `malloc`'d pointer (heap)
2. Prints the address of each using `%p` format
3. Outputs a sorted list of addresses with labels (use `printf` to sort manually by comparing hex values)
4. Identifies which addresses are close to each other (data/bss should be adjacent; stack should be much higher; heap should be between bss and stack)
5. Prints the size of each using `sizeof`
6. In a comment block, draw the memory map with the actual addresses from your output

**Assignment 2: `hexdump()` Implementation (`hexdump.c`)**

Implement `hexdump(const void *data, size_t len)` exactly as described in Section 3.3. Then:
1. Create a `full_snapshot_t`-like struct with 3 fields: `uint32_t id`, `uint8_t state`, `uint16_t counter` (note the padding!)
2. Fill with known values: `id=0xDEADBEEF`, `state=0x42`, `counter=0x1234`
3. Use `hexdump()` to display the raw bytes
4. In a comment: identify which bytes are field data and which are padding
5. Use `hexdump()` on a packet buffer (manually fill: `0xAA 0x55 0x01 0x05 0x04 0x00` as header)
6. Verify little-endian byte order: `0x1234` should appear as `34 12` in the dump

**Assignment 3: Stack Depth Measurement (`stack_measure.c`)**

Write a recursive `void measure_stack_depth(int n)` function that:
1. Creates a local `uint8_t marker = 0xA5`
2. Prints the address of `marker` at EVERY call depth
3. Calculates the delta between consecutive depths (= one stack frame size)
4. Recurse until `n == 0`
5. Run with `n = 20`. Output should show the stack pointer decreasing by a fixed amount each call.
6. Identify: what is the size of one stack frame for this specific function? Does it match your manual calculation of saved registers + locals?

---

# 📚 Reference Materials

1. **ARM Cortex-M4 Processor Technical Reference Manual** — Section 3 (Memory Model) and Section 4 (Instruction Set)  
   → Defines the AHB bus transaction, alignment requirements, and the stack pointer initialization  
   → Free from: [developer.arm.com](https://developer.arm.com/documentation/100166/latest/)

2. **STM32F407 Reference Manual (RM0090)** — Chapter 2 (Memory and Bus Architecture)  
   → Physical memory map with all addresses  
   → AHB/APB bus matrix description  
   → Download from: [st.com/resource/en/reference_manual/rm0090](https://www.st.com/resource/en/reference_manual/rm0090-stm32f405415-stm32f407417-stm32f427437-and-stm32f429439-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)

3. **FreeRTOS Source Code — heap_4.c** — Read the actual implementation  
   → `pvPortMalloc()` source, free list, coalescing logic  
   → Found in your STM32CubeIDE project under: `Middlewares/Third_Party/FreeRTOS/Source/portable/MemMang/heap_4.c`  
   → Key function to read: `pvPortMalloc()` — count the pointer dereferences in the inner loop

4. **"Better Embedded Software" — Philip Koopman (CMU)** — Chapter 5 (Memory)  
   → Professional treatment of stack sizing, heap risks, static vs dynamic allocation  
   → Free PDF: [betterembsw.blogspot.com](https://betterembsw.blogspot.com/)

---

# 🔑 Key Points Summary

Copy this to your `ryn/notes/week1/day2_key_points.md`:

```
SECTION RULES:
  .text   → Code → Flash (read-execute)
  .rodata → const data → Flash (read-only)
  .data   → initialized globals → RAM + Flash (copied at boot): DOUBLE COST
  .bss    → uninitialized statics/globals → RAM only: FLASH FREE
  Stack   → local vars + function frames → RAM, grows DOWN
  Heap    → malloc/pvPortMalloc → RAM, grows UP, fragmentation risk

CRITICAL RULES:
  1. Large buffers (>64B) → ALWAYS static → .bss, never on stack
  2. memset(&struct, 0, sizeof()) ALWAYS before filling, for correct memcmp
  3. Never pvPortMalloc() in time-critical code or ISR hot path
  4. pointer + n advances by (n × sizeof(*pointer)) bytes — not n bytes
  5. Access between peripheral registers with volatile ALWAYS (no exception)
  6. Use memcpy() to read multi-byte fields from unaligned byte buffers

POINTER MATH:
  ptr + 1               → +sizeof(*ptr) bytes
  (uint8_t*)ptr + 1     → +1 byte (cast to byte pointer first)
  ptr_b - ptr_a         → number of ELEMENTS between (same type)
  (uint8_t*)p_b - (uint8_t*)p_a → number of BYTES between

STACK SIZING RULE OF THUMB (RTOSTwin tasks):
  SensorTask:    768 bytes  (no deep calls)
  TelemetryTask: 2048 bytes (calls uxTaskGetSystemState → deep)
  ControlTask:   2048 bytes (may have nested logic)
  MonitorTask:   1024 bytes (simple polling)
  FreeRTOS adds ~200 bytes per task overhead for TCB context

ADDRESSES (STM32F407):
  FLASH start:    0x08000000
  SRAM1 start:    0x20000000
  SRAM1 end:      0x2001FFFF  (top = initial SP)
  Peripherals:    0x40000000+
  ARM CoreDebug:  0xE0000000+
  DWT->CYCCNT:    0xE0001004

INTERVIEW TALKING POINTS:
  - ".bss saves Flash — declaring a 4 KB buffer as 'static uint8_t buf[4096]'
     uses zero Flash bytes. Adding '= {0}' wastes 4 KB of Flash for no benefit."
  - "Stack overflows in RTOS are the hardest bugs: silent corruption hours before crash.
     FreeRTOS watermark (uxTaskGetStackHighWaterMark) tells you how close you got."
  - "I banned pvPortMalloc in the hot path because it's non-deterministic and can fragment.
     All ISR-safe buffers are static — allocated at compile time, zero runtime cost."
  - "MMIO on ARM is not magic: it's just pointer dereference to the peripheral address space.
     The AHB bus routes the access to the right silicon block."
```

---

**END OF WEEK 1 DAY 2 NOTES**

*Next: Day 3 — Pointers as Hardware Wires (MMIO deep dive), `volatile` ISR patterns, and the 3 faces of `static` in production code*
