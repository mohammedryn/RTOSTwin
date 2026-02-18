# Module 0.1 — Theoretical Homework
## C Programming for Embedded Systems

**Instructions:** Answer each question in detail. Show your work (calculations, diagrams). Write your answers directly below each question.

---

### Question 1: Type Width and Overflow

A temperature sensor returns a 12-bit ADC value (0–4095). You store it in a `uint8_t`.

a) What is the maximum value `uint8_t` can represent?

**Your answer:**


b) If the ADC reads 3000, what value is stored in the `uint8_t`? Show the math.

**Your answer:**


c) What type should you use instead? Why?

**Your answer:**


---

### Question 2: Memory Layout

Given this struct:
```c
typedef struct {
    uint8_t   flags;
    uint32_t  timestamp;
    uint16_t  value;
    uint8_t   type;
    uint32_t  sequence;
} packet_t;
```

a) Calculate `sizeof(packet_t)` assuming default ARM alignment (4-byte boundary). Draw the memory layout byte-by-byte, showing where padding is inserted.

**Your answer:**


b) Reorder the fields to minimize padding. What is the new `sizeof`?

**Your answer:**


c) If you transmit this struct as raw bytes over UART to a different processor (e.g., x86 PC), what problems might the receiver have?

**Your answer:**


---

### Question 3: Bitwise Operations

You have an 8-bit register `STATUS = 0b10110100`.

a) Which bits are set? (list their bit numbers, 0-indexed from LSB)

**Your answer:**


b) Write a C expression to check if bit 5 is set.

**Your answer:**


c) Write a C expression to clear bit 4 without changing other bits.

**Your answer:**


d) Write a C expression to toggle bits 7 and 2 simultaneously.

**Your answer:**


e) Starting from the original value `0b10110100`, apply operations (c) then (d) in sequence. What is the final binary value?

**Your answer:**


---

### Question 4: CRC Overhead Estimation

The CRC-16 function processes one byte in approximately 16 CPU cycles on a Cortex-M4 at 168 MHz.

a) How many CPU cycles to compute CRC over a 350-byte snapshot?

**Your answer:**


b) How many microseconds does that take? (Show: cycles ÷ clock_frequency)

**Your answer:**


c) If `snapshot_capture()` takes 85 µs and CRC takes X µs (from part b), what is the total? Does it meet the < 150 µs target?

**Your answer:**


d) The CRC computation does NOT need to happen inside a critical section (interrupts disabled). Why not? What property of the data makes this safe?

**Your answer:**


---

### Question 5: Circular Buffer Capacity

Your transmit circular buffer has `BUFFER_SIZE = 32` slots, each slot holds a `packet_t` of 128 bytes.

a) What is the total RAM consumed by the buffer? (Note: a power-of-2 ring buffer can hold SIZE-1 items. Why?)

**Your answer:**


b) If the producer pushes 10 packets/sec and the consumer pops 8 packets/sec, after how many seconds does the buffer fill up?

**Your answer:**


c) What should your code do when the buffer is full? List at least 3 strategies and their tradeoffs (for an RTOS telemetry system).

**Your answer:**


---

**Submission:** Save this file with your answers filled in. We will review together before moving to Module 0.2.
