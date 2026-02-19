# VNV — Week 1 Theoretical Homework
## Bitwise Operations, Structs, Padding & CRC

---

### Question 1: Bitwise Operations

Given `STATUS = 0b10110100`:

a) Which bits are set? (list positions, 0-indexed from LSB)

**Your answer:**


b) Write C to check if bit 5 is set. What is the result (1 or 0)?

**Your answer:**


c) Write C to clear bit 4 without changing other bits. What is the new binary value?

**Your answer:**


d) Write C to toggle bits 7 and 2 simultaneously. What is the new binary value?

**Your answer:**


e) Starting from the original `0b10110100`, apply (c) then (d) in sequence. What is the final value in binary and hex?

**Your answer:**


---

### Question 2: Struct Padding

```c
typedef struct {
    uint8_t   flags;
    uint32_t  timestamp;
    uint16_t  value;
    uint8_t   type;
    uint32_t  sequence;
} packet_t;
```

a) Draw the byte-by-byte memory layout. Label each byte as field name or PAD.

**Your answer (ASCII diagram):**


b) What is `sizeof(packet_t)` with default ARM alignment?

**Your answer:**


c) Reorder the fields to minimize sizeof. What is the new sizeof?

**Your answer:**


d) If you transmit this struct as raw bytes over UART to a PC, what two problems might occur?

**Your answer:**


---

### Question 3: Delta Encoder Bitmask

RTOSTwin defines:
```c
#define FIELD_TASKS       (1 << 0)
#define FIELD_MEMORY      (1 << 1)
#define FIELD_PERIPHERALS (1 << 2)
#define FIELD_HEALTH      (1 << 3)
```

a) If `changed_fields = 0b00001011`, which sections changed?

**Your answer:**


b) If a full snapshot is 360 bytes and only MEMORY changed (16 bytes), what is the compression ratio of delta encoding? (Show: full_size / delta_size)

**Your answer:**


c) Why do we send a full "keyframe" snapshot periodically instead of always sending deltas?

**Your answer:**


---

### Question 4: CRC Overhead

CRC-16 costs ~16 cycles per byte on Cortex-M4 at 168 MHz.

a) Cycles to CRC a 350-byte snapshot?

**Your answer:**


b) Microseconds? (cycles ÷ MHz)

**Your answer:**


c) Can we compute CRC outside the critical section (interrupts enabled)? Why?

**Your answer:**


---

**Submission:** Fill in answers and save. Be ready to explain each to RYN.
