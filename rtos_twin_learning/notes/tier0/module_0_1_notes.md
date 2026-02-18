# Module 0.1 — Study Notes
## C Programming for Embedded Systems
*Auto-generated. Update as you learn.*

---

## Key Rules (Memorize)

| Rule | Why |
|------|-----|
| **Always use `stdint.h` types** (`uint32_t`, `uint8_t`, etc.) | `int` size varies by platform |
| **Always use `volatile`** for hardware registers, ISR-shared vars, DMA buffers | Prevents compiler from caching stale values |
| **Never `malloc` in the hot path** | Non-deterministic, fragments heap, needs mutex |
| **Use `static` for persistent buffers** | Allocated at compile time, zero risk |
| **`memset` structs to zero before use** | Padding bytes contain garbage; `memcmp` fails |
| **`BUFFER_SIZE` must be power of 2** for circular buffers | Enables fast `& (SIZE-1)` wrapping |
| **CRC goes LAST in the struct** | So you can CRC everything before it |
| **Order struct fields largest → smallest** | Minimizes padding waste |

## Formulas

```
Overflow:           stored_value = actual_value mod (2^bit_width)
Sizeof struct:      Sum of fields + alignment padding
CRC-16 cost:        16 cycles/byte × N bytes ÷ CPU_MHz = microseconds
Circular capacity:  BUFFER_SIZE - 1 usable slots
Bitmask for N bits: (1 << N) - 1
```

## My Mistakes / Insights
*(Add your own notes here as you work through the homework)*

- 
- 
- 

## Questions for Coach
*(Write questions here for our next review session)*

- 
- 
