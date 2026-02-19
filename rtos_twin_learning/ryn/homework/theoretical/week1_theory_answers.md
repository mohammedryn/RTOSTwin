# RYN — Week 1 Theoretical Homework
## C Types, Memory Layout, Pointers & volatile

---

### Question 1: Type Width & Overflow

a) If `uint8_t x = 200` and you cast it to `int8_t y = (int8_t)x`, what is `y`?  
   Show the two's complement math.

**Your answer:**


b) A 12-bit ADC (range 0–4095) reads 3000. You store it in `uint8_t`. What value is stored? What type should you use?

**Your answer:**


c) `uint16_t start = 65530; uint16_t end = 5; uint16_t elapsed = end - start;` — what is `elapsed` and why does unsigned subtraction "just work" here?

**Your answer:**


---

### Question 2: Memory Layout

Draw the RAM memory map of an STM32F4 (192 KB, starting at 0x20000000) showing:
- `.data` section
- `.bss` section
- Heap (grows up)
- Stack (grows down)

Label approximate sizes for an RTOS application with 8 tasks at 2 KB stack each.

**Your answer (draw with ASCII art or describe):**


---

### Question 3: volatile

Explain in your own words:

a) What does `volatile` tell the compiler NOT to do?

**Your answer:**


b) Give 4 specific scenarios where `volatile` is mandatory in embedded C.

**Your answer:**


c) What bug occurs if you read a hardware register in a while-loop WITHOUT volatile?

**Your answer:**


---

### Question 4: Static Allocation

The RTOSTwin `snapshot_capture()` function needs a `TaskStatus_t` buffer of 10 elements (~300 bytes).

a) Why NOT stack allocation? (`TaskStatus_t buf[10];`)

**Your answer:**


b) Why NOT heap allocation? (`TaskStatus_t *buf = malloc(...);`)

**Your answer:**


c) Why IS static allocation correct? (`static TaskStatus_t buf[10];`)

**Your answer:**


---

**Submission:** Fill in answers and save. Be ready to explain each to VNV.
