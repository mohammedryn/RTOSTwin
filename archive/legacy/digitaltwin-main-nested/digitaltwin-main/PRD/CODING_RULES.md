# RTOSTwin — Coding Rules

> These rules apply to ALL code generated for RT OSTwin. Attach this file to every AI prompt.

---

## 1. Language Standards

| Component | Language | Standard | Compiler Flags |
|---|---|---|---|
| Agent firmware | C | C99 (`-std=c99`) | `-Wall -Wextra -Werror -O2` |
| Bridge | Python | 3.9+ | `mypy --strict` (type hints required) |
| Tests (agent) | C | C99 | `-std=c99 -Wall -Wextra` |
| Tests (bridge) | Python | 3.9+ | `pytest` |

---

## 2. Forbidden Patterns (NEVER Generate These)

### C Agent — Absolute Prohibitions

| Pattern | Why Forbidden | What To Use Instead |
|---|---|---|
| `malloc()` / `free()` / `pvPortMalloc()` / `vPortFree()` inside any agent function | Priority inversion, heap fragmentation, non-deterministic timing | `static` buffers at file scope |
| `printf()` inside `snapshot_capture()` | Blocking I/O, unpredictable timing, uses heap internally | DMA UART only via `transport_send()` |
| Global variables without `static` keyword | Namespace pollution, unintended cross-module coupling | `static` for all file-scope variables |
| `float` or `double` anywhere in agent | Cortex-M4 has FPU but floating point is slow and non-deterministic for WCET | Fixed-point integer math (multiply then divide) |
| Recursive functions | Stack overflow risk on 2KB task stacks | Iterative loops only |
| `strlen()` on task names inside hot path | O(n) scan, non-deterministic | Use `TASK_NAME_MAX_LEN` constant, pad with null |
| Mutex inside ISR context | FreeRTOS mutexes cannot be used from ISR | `taskENTER_CRITICAL()` / `taskEXIT_CRITICAL()` |
| Blocking calls inside telemetry task | Telemetry must never delay application tasks | Non-blocking DMA only |
| `memcpy` on volatile buffers | Compiler may optimize away volatile semantics | Byte-by-byte copy loop |
| C++ features (classes, templates, exceptions) | Agent must be C99 only | Pure C structs and functions |

### Python Bridge — Absolute Prohibitions

| Pattern | Why Forbidden | What To Use Instead |
|---|---|---|
| `time.sleep()` in main loop | Blocks entire bridge, misses packets | `asyncio.sleep()` or event-driven |
| Bare `except:` or `except Exception:` | Swallows ALL errors silently | Specific exception types only |
| Global mutable state | Multi-device support breaks | Class instances with explicit state |
| `eval()` or `exec()` | Security vulnerability | Never needed in this project |
| Hardcoded serial port names | Breaks cross-platform | `config.py` with CLI args/env vars |
| `from module import *` | Namespace pollution | Explicit named imports only |

---

## 3. Naming Conventions

### C Agent

| Element | Convention | Example |
|---|---|---|
| Functions | `module_action()` | `snapshot_capture()`, `encoder_encode()`, `transport_send()` |
| Types (structs) | `snake_case_t` | `full_snapshot_t`, `task_snapshot_t`, `profiler_stats_t` |
| Constants/defines | `UPPER_CASE` | `MAX_TASKS`, `WF_SYNC_0`, `WF_CRC_POLY` |
| Static file-scope vars | `s_descriptive_name` | `s_task_status_buf`, `s_last_snapshot`, `s_idle_cycle_count` |
| Local variables | `snake_case` | `packet_len`, `crc_value`, `elapsed_cycles` |
| Header guards | `RTOSTWIN_MODULE_H` | `RTOSTWIN_SNAPSHOT_H`, `RTOSTWIN_WIRE_FORMAT_H` |
| Module prefix on all public functions | Module name | `snapshot_*`, `encoder_*`, `framer_*`, `transport_*`, `profiler_*` |

### Python Bridge

| Element | Convention | Example |
|---|---|---|
| Classes | `PascalCase` | `PacketDecoder`, `OOMAnalyzer`, `DeviceRegistry` |
| Functions/methods | `snake_case` | `feed_byte()`, `add_sample()`, `get_projection_seconds()` |
| Constants | `UPPER_CASE` | `SYNC_0`, `CRC_POLY`, `DEFAULT_WINDOW_SIZE` |
| Private methods | `_prefixed` | `_compute_crc()`, `_parse_payload()` |
| Module files | `snake_case.py` | `decoder.py`, `oom_analyzer.py`, `state_manager.py` |
| Type hints | Required on ALL function signatures | `def feed_byte(self, byte: int) -> Optional[DecodedPacket]:` |
| Dataclasses for all data objects | `@dataclass` | `DecodedPacket`, `TaskSnapshot`, `DeviceState` |

---

## 4. Error Handling

### C Agent

| Situation | Action |
|---|---|
| DMA busy when trying to send | Return `-1`, increment `s_tx_drop_count`. Do NOT wait or retry. |
| `uxTaskGetSystemState` returns > MAX_TASKS | Clamp to `MAX_TASKS`. Log warning via debug UART if enabled. |
| CRC mismatch on self-test | Assert in debug builds (`configASSERT`). In release: increment error counter. |
| Any function receives NULL pointer | `configASSERT(ptr != NULL)` in debug. In release: return immediately with error code. |

### Python Bridge

| Situation | Action |
|---|---|
| CRC validation fails | Discard packet, increment `drop_count`, log warning. Do NOT raise exception. |
| Sequence gap detected | Compute gap size, increment `sequence_gap_count`, continue processing. |
| Serial port disconnects | Log error, attempt reconnect every 5 seconds. Do not crash. |
| OTLP export fails | Log error, continue running. Retry on next export interval. |
| OOM analyzer has < 30 samples | Return `-1.0` (insufficient data). Do not attempt regression. |

---

## 5. Testing Requirements

### Every Function Must Have

| Test Type | C (Unity) | Python (pytest) |
|---|---|---|
| Happy path | At least 1 test with known-good inputs and verified outputs | Same |
| Error path | At least 1 test with invalid/edge-case inputs | Same |
| Boundary | Test at MAX_TASKS, buffer full, sequence wrap (65535→0) | Same |

### Test File Naming

| Source File | Test File |
|---|---|
| `agent/core/snapshot.c` | `agent/tests/test_snapshot.c` |
| `agent/core/framer.c` | `agent/tests/test_framer.c` |
| `bridge/decoder.py` | `bridge/tests/test_decoder.py` |
| `bridge/oom_analyzer.py` | `bridge/tests/test_oom_analyzer.py` |

### Test Data

- All known-good packet byte sequences stored in `bridge/tests/conftest.py` as `pytest.fixture`.
- CRC test vector: `crc16_ccitt(b"123456789") == 0x29B1` — tested in BOTH C and Python.
- Mock device output must be used as integration test input.

---

## 6. Documentation Requirements

| Element | Rule |
|---|---|
| Every `.h` file | Doxygen-style comment block on every public function and struct |
| Every `.py` module | Module-level docstring explaining purpose |
| Every `.py` class | Class-level docstring with usage example |
| Every `.py` function | Google-style docstring with Args, Returns, Raises |
| Every configurable parameter | Default value + explanation in `config.py` AND in `docs/configuration.md` |
| Every performance claim in README | Paired with exact measurement command to reproduce |

---

## 7. `volatile` Rules (C Agent)

| Variable | Needs `volatile`? | Reason |
|---|---|---|
| `s_idle_cycle_count` | **YES** | Written by idle hook (ISR-level), read by `snapshot_capture()` |
| `s_total_cycle_count` | **YES** | Same as above |
| `s_tx_drop_count` | **YES** | Written by DMA callback (ISR), read by telemetry task |
| `s_task_status_buf` | NO | Only accessed within critical section, single writer |
| `s_last_snapshot` | NO | Only accessed by encoder, single task |
| `DWT->CYCCNT` | Already volatile | Declared volatile in ARM CMSIS headers |
| All peripheral registers | Already volatile | Declared volatile in STM32 HAL/CMSIS headers |

---

## 8. Git Commit Message Format

```
[component] brief description

component = agent | bridge | dashboard | docs | ci | examples
```

Examples:
```
[agent] implement snapshot_capture with static allocation
[bridge] add CRC-16-CCITT validation to decoder
[ci] add arm-none-eabi-size check to GitHub Actions
[dashboard] add stack watermark bar chart panel
```

---

## 9. Pull Request Checklist

Every PR must satisfy before merge:
- [ ] All existing tests pass
- [ ] New code has tests (happy + error path)
- [ ] No new compiler warnings with `-Wall -Wextra -Werror`
- [ ] No `malloc/free` added to agent (grep verified)
- [ ] Type hints on all new Python functions
- [ ] Updated relevant docs if API changed
- [ ] CI pipeline green
