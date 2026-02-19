# RYN — Week 1 Study Notes
## C Types, Memory, Pointers & Static Allocation
*Update this as you study!*

---

## Key Rules (Memorize)

| Rule | Why |
|------|-----|
| Always use `stdint.h` (`uint32_t`, not `int`) | `int` is 16-bit on some platforms |
| Always use `volatile` for hardware registers | Compiler caches stale values otherwise |
| Never `malloc` in the hot path | Non-deterministic, fragments, needs mutex |
| Use `static` for persistent buffers | Compile-time, zero overhead, never fails |
| Verify types with `sizeof()` | Assumptions kill |

## Formulas

```
Overflow:          stored = actual mod (2^bits)
Two's complement:  signed = unsigned - 2^bits   (if MSB is set)
Timestamp diff:    elapsed = end - start         (works even if wrapped!)
RAM budget:        .data + .bss + heap + stacks + agent < total RAM
```

## What I Learned
*(Fill in after studying)*

- 
- 

## Questions for VNV
*(Things VNV should know from their topics)*

- 
- 

## Teaching Prep
*(Notes for when you explain to VNV)*

**Demo 1:** Overflow — show 300 in uint8_t → 44  
**Demo 2:** Memory addresses — print addresses of global, stack, heap variables  
**Demo 3:** volatile — explain the infinite loop bug  
