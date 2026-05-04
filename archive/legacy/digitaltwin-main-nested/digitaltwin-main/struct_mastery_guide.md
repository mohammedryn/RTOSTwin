# Struct Mastery Guide: The "Any Question" Reference

Designed for the **RTOSTwin** digital twin project. Master these to handle any embedded interview or firmware challenge.

---

## 1. Natural Alignment Rules
- `uint32_t` $\rightarrow$ Must be at address divisible by **4**.
- `uint16_t` $\rightarrow$ Must be at address divisible by **2**.
- **The Struct Rule:** A struct's total size is a multiple of its **largest member's requirement**. If it contains a `uint32_t`, the total size will be a multiple of 4 (e.g., 8, 12, 16).

---

## 2. Bit-Fields (Saving Space)
When you only need a few bits for a flag:
```c
struct {
    uint8_t id    : 4; // 0-15
    uint8_t ready : 1; // 0-1
    uint8_t prio  : 2; // 0-3
} flags;
```
- **Benefit:** Packs all of these into **1 single byte**.
- **Use case:** Hardware registers or tight radio packets.

---

## 3. Unions (Shared Memory)
One memory slot, multiple ways to look at it:
```c
union {
    float f;
    uint8_t b[4];
} data;
```
- **Fact:** `sizeof(union)` is the size of its **largest** member.
- **RTOSTwin Use:** Use unions to split `float` data into bytes to send over Serial/SPI.

---

## 4. The `__packed` Attribute
Tells the compiler: **"No padding! I want the exact size."**
- **Risks:** Can cause **Bus Faults** or crashes on ARM CPUs if you try to read a 4-byte number at an odd address.
- **Rule:** Use for communication packets ONLY, not for local variables.

---

## 5. Interview Challenges
- **Q:** Why `memset` zero? **A:** To clear hidden padding "garbage" so `memcmp` works.
- **Q:** Performance cost of padding? **A:** Padding *improves* performance by aligning data for the CPU bus.
- **Q:** `offsetof` usage? **A:** Returns the number of bytes from the struct start to a specific member.
- **Q:** Union vs Struct? **A:** Struct = House (everyone has a room); Union = Phone Booth (one at a time).
