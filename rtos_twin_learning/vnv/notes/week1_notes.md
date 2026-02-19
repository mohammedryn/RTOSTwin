# VNV — Week 1 Study Notes
## Bitwise Operations, Structs, Padding & CRC
*Update this as you study!*

---

## Key Rules (Memorize)

| Rule | Why |
|------|-----|
| SET: `reg ❘= (1 << bit)` | Turn a bit ON |
| CLEAR: `reg &= ~(1 << bit)` | Turn a bit OFF |
| TOGGLE: `reg ^= (1 << bit)` | Flip a bit |
| CHECK: `if (reg & (1 << bit))` | Test if bit is set |
| Order struct fields largest → smallest | Minimizes padding |
| `memset` structs to 0 before use | Padding = garbage → `memcmp` fails |
| CRC goes LAST in the struct | So you CRC everything before it |
| Use `packed` only for wire protocol | Unaligned access is slow/dangerous |

## Formulas

```
Struct padding:    align each field to its own size
                   32-bit field → address must be ÷ 4
                   16-bit field → address must be ÷ 2
Bitmask for bit N: (1 << N)
Multi-bit mask:    (((1 << width) - 1) << start_bit)
CRC overhead:      16 cycles/byte × num_bytes ÷ CPU_MHz
Compression ratio: full_size ÷ delta_size
```

## What I Learned
*(Fill in after studying)*

- 
- 

## Questions for RYN
*(Things RYN should know from their topics)*

- 
- 

## Teaching Prep
*(Notes for when you explain to RYN)*

**Demo 1:** `print_binary()` — show bit changes live with set/clear/toggle  
**Demo 2:** Struct padding — two structs, same fields, different sizeof  
**Demo 3:** Delta bitmask — show how 1 byte tells you what changed  
