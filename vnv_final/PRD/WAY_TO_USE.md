# RTOSTwin — How to Use the PRD Files With AI

> **PURPOSE OF THIS FILE.**
> This is your master operating manual. Every time you open a coding session, a new task, or want to explain the project to an AI, you come here first. This document tells you which files to use, when to use them, what to actually type to the AI, and what to expect back.

---

## Table of Contents

1. [The 7 Files — What Each One Is For](#1-the-7-files--what-each-one-is-for)
2. [The Golden Rule — How These Files Work Together](#2-the-golden-rule--how-these-files-work-together)
3. [Scenario 1 — Starting a Fresh Coding Session](#3-scenario-1--starting-a-fresh-coding-session)
4. [Scenario 2 — Picking and Executing the Next Task](#4-scenario-2--picking-and-executing-the-next-task)
5. [Scenario 3 — Writing C Firmware Code](#5-scenario-3--writing-c-firmware-code)
6. [Scenario 4 — Writing Python Bridge Code](#6-scenario-4--writing-python-bridge-code)
7. [Scenario 5 — Writing Unit Tests](#7-scenario-5--writing-unit-tests)
8. [Scenario 6 — The AI Hallucinated or Made a Mistake](#8-scenario-6--the-ai-hallucinated-or-made-a-mistake)
9. [Scenario 7 — A Team Member (VNV or Partner) Is Asking How to Start](#9-scenario-7--a-team-member-vnv-or-partner-is-asking-how-to-start)
10. [Scenario 8 — The AI Goes Off-Track or Gets Too Big](#10-scenario-8--the-ai-goes-off-track-or-gets-too-big)
11. [Scenario 9 — Resuming a Session After a Break](#11-scenario-9--resuming-a-session-after-a-break)
12. [Scenario 10 — Debugging a Failing Test or Bug](#12-scenario-10--debugging-a-failing-test-or-bug)
13. [Cheat Sheet — Quick Reference](#13-cheat-sheet--quick-reference)

---

## 1. The 7 Files — What Each One Is For

Read this section once and never forget it. Every file has exactly one job.

---

### `PRD.md` — The "Why" Document

**What it contains:**
- The problem this project solves and who it's for
- What the product is (3 components: Agent, Bridge, Dashboard)
- Performance targets that MUST be met (e.g., CPU overhead < 2%, RAM < 10KB)
- What the product deliberately does NOT do (Non-Goals)
- What hardware you need (`NUCLEO-F401RE`, `ESP32-P4`, `Teensy 4.1`)

**When to give it to the AI:**
- When you want the AI to understand the big picture before starting any task
- When the AI starts writing code that goes beyond scope (e.g., starts adding ML features)
- When writing the README, documentation, or blog post
- As a sanity check to make sure a new feature idea is actually in scope

**Example prompt using this file:**

> "Read `PRD.md`. Based on the success criteria in Section 'Performance Gates', tell me whether a telemetry task that uses `vTaskDelay(pdMS_TO_TICKS(50))` meets our 10 Hz requirement."

**What NOT to use it for:**
- Do not use PRD.md alone when writing code. It doesn't contain structs, function signatures, or data formats — use TECH_SPEC.md for that.

---

### `ARCHITECTURE.md` — The "How It All Fits Together" Document

**What it contains:**
- The full system diagram showing how data flows from the MCU all the way to Grafana
- Which module is responsible for what (snapshot → encoder → framer → transport on the C side)
- The exact data flow on both the C side and Python side as a step-by-step pipeline
- Transport options (UART DMA vs USB CDC vs UDP)
- Bandwidth budget (why delta encoding is mandatory)
- How failure cases are handled (DMA busy, CRC fail, serial disconnect)
- Security limitations

**When to give it to the AI:**
- When writing code for any module that talks to another module (e.g., `transport.c` calls `frame_packet()`)
- When the AI doesn't know what inputs a module receives or what it should output
- When writing `bridge/main.py` (the Python entry point that wires everything together)
- When writing Docker Compose or GitHub Actions CI (which need to understand the full picture)
- When someone asks you to explain the project

**Example prompt using this file:**

> "Read `ARCHITECTURE.md`. I need to write `bridge/main.py`. Based on the 'Data flow' section under 'Component 2: Python Bridge', wire together `decoder.py`, `state_manager.py`, `prometheus_exporter.py`, `otlp_exporter.py`, and `oom_analyzer.py` in a single asyncio event loop."

**What NOT to use it for:**
- Don't use ARCHITECTURE.md for specific struct field names, function signatures, or byte-level wire format details — that's in TECH_SPEC.md.

---

### `TECH_SPEC.md` — The "Exact Blueprints" Document

**What it contains:**
- Exact C struct definitions (`task_snapshot_t`, `full_snapshot_t`, `memory_snapshot_t`)
- Every field, its type, its size in bytes, and its purpose
- The complete binary wire format packet layout (exactly which byte offset holds which field)
- Every `#define` protocol constant from the root canonical `agent/core/wire_format.h` (`WF_SYNC_0 = 0xAA`, `WF_CRC_POLY = 0x1021`, etc.)
- The exact CRC-16-CCITT algorithm in both C and Python
- Every function signature for both the C agent and the Python bridge
- The delta encoding format (tag byte encoding, field IDs)
- All algorithms (CPU utilization calculation, OOM detection)
- Known hard problems and their solutions

**When to give it to the AI:**
- **Almost always** — this is the single most important file for code generation
- Any time you write a struct, the AI must use TECH_SPEC.md Section 1 exactly
- Any time you write or parse a packet, the AI must use TECH_SPEC.md Section 2 exactly
- Any time you implement a function, the AI must match the exact signature in Section 3 or 4
- Any time you implement CRC, the AI must match Section 2.3 exactly
- Any time you implement OOM detection, the AI must use Section 5.2

**Example prompt using this file:**

> "Read `TECH_SPEC.md` Section 1.1 only. Create `agent/core/snapshot.h` with the exact struct definitions. Do not change field names, field types, or field sizes. Add Doxygen comments explaining every field."

**What NOT to use it for:**
- Don't use TECH_SPEC.md for file paths — use FILE_STRUCTURE.md for that.
- Don't use TECH_SPEC.md for naming conventions or error handling patterns — use CODING_RULES.md.

---

### `FILE_STRUCTURE.md` — The "Where Does This Go?" Document

**What it contains:**
- The complete folder and file tree for the entire project
- The exact file path for every file that will be created (e.g., `agent/core/snapshot.h`)
- What each file's responsibility is
- Which tests correspond to which source files

**When to give it to the AI:**
- Every time you start a new file — to confirm the exact path and filename
- When the AI puts a file in the wrong folder (e.g., puts `uart_dma.c` in `agent/core/` instead of `agent/hal/stm32/`)
- When writing CMakeLists.txt or any build system file (it needs to know the full file tree)
- When setting up GitHub Actions CI (to know which test files to run)

**Example prompt using this file:**

> "Read `FILE_STRUCTURE.md`. I'm about to implement the STM32 DMA UART HAL files. Confirm the exact file paths I should create, and list all other files in the `agent/hal/` directory."

**What NOT to use it for:**
- Don't use FILE_STRUCTURE.md for what to write inside the files — that's TECH_SPEC.md + CODING_RULES.md.

---

### `CODING_RULES.md` — The "How to Write the Code" Document

**What it contains:**
- Language standards for C (C99, with `-Wall -Wextra -Werror`) and Python (3.9+, mypy strict)
- A complete table of forbidden patterns and exactly why they are forbidden, with alternatives
  - e.g., NO `malloc()` — use `static` buffers instead
  - e.g., NO `float` or `double` in C agent
  - e.g., NO bare `except:` in Python
- Naming conventions for functions, structs, constants, variables (with examples)
- Error handling rules for every failure scenario
- Testing requirements (every function needs a happy-path test AND an error-path test)
- Documentation requirements (Doxygen on C headers, Google-style docstrings on Python)
- `volatile` rules (which variables need it and why)
- Git commit message format

**When to give it to the AI:**
- **Every single time you write any C or Python code.** No exceptions.
- When you see the AI generate `malloc()` or `pvPortMalloc()` — stop it immediately
- When you see a Python function missing type hints
- When the AI uses a naming convention that doesn't match the rules (e.g., `TaskSnapshot` instead of `task_snapshot_t`)
- When reviewing AI-generated code for correctness before accepting it

**Example prompt using this file:**

> "Read `CODING_RULES.md`. Before I accept any code you generate, I need you to self-check: does it use `static` for all file-scope variables? Does it avoid `malloc`? Do all Python functions have type hints? After generating each chunk, explicitly state which rules from CODING_RULES.md you applied."

**What NOT to use it for:**
- Don't use CODING_RULES.md for struct definitions — that's TECH_SPEC.md.
- Don't use CODING_RULES.md for which file to put the code in — that's FILE_STRUCTURE.md.

---

### `TASK_QUEUE.md` — The "What To Do Next" Document

**What it contains:**
- 25 numbered tasks in the exact order they must be done
- For each task: the exact prompt to give the AI, which context files to attach, the expected output files, and the verification gate (how to know the task is done correctly)
- The dependency tree at the bottom (Task 3 depends on Task 1 and Task 2, etc.)
- Grouped into 4 phases: Foundation (no hardware), Agent Core (hardware required), Python Bridge, Dashboard + Docs

**When to use it:**
- This is your map. At the start of every session, open this file and find the first unchecked task.
- Copy the exact prompt from the task description and give it to the AI.
- Do NOT skip tasks. Dependencies are strict (you cannot write `encoder.c` before `snapshot.h` exists).
- After the AI finishes a task, run the verification gate. Only then mark the task done and move to the next.

**Example of using it correctly:**

> *You open TASK_QUEUE.md. Task 1, 2, and 3 are done. Task 4 is next.*
>
> "We are on Task 4. The prompt is: 'Create `agent/tests/test_crc.c` and `agent/tests/test_framer.c` using the Unity test framework.' Attach these context files: `TECH_SPEC.md`, `CODING_RULES.md`, `framer.h`, `framer.c`, and the root canonical `../../agent/core/wire_format.h`. Follow the AI_WORKFLOW_RULES.md 5-step cycle."

**What NOT to use it for:**
- Don't use TASK_QUEUE.md as a replacement for TECH_SPEC.md. The task prompts are short summaries — the AI still needs TECH_SPEC.md to get the exact struct/function details right.

---

### `AI_WORKFLOW_RULES.md` — The "How the AI Must Behave" Document

**What it contains:**
- The mandatory 5-step execution cycle every chunk of work must follow:
  1. ANNOUNCE (state what will be done)
  2. EXECUTE (write the code)
  3. EXPLAIN (explain every line assuming you know nothing)
  4. ASK (invite questions)
  5. WAIT (get permission before the next chunk)
- What counts as a "chunk" and what does NOT
- Explanation depth rules ("explain like I know nothing")
- How the AI should handle uncertainty (STOP and present options)
- How the AI should handle mistakes (acknowledge, explain, corrected chunk)
- Progress tracking format (session start, resume, per-chunk updates)
- A list of prohibited AI behaviors (no jargon without defining it, no placeholder TODOs, etc.)

**When to use it:**
- Attach this file at the start of EVERY session. It is the foundation of how all work gets done.
- When the AI dumps hundreds of lines of code without stopping to explain — invoke this file.
- When the AI uses a word you don't understand — invoke Section 3.
- When the AI seems unsure or makes a choice you didn't expect — invoke Section 4.

**Example prompt:**

> "Before we begin, read `AI_WORKFLOW_RULES.md` in full. Confirm you understand the 5-step cycle. Do NOT generate any code yet. Just acknowledge the rules, state the current project progress, and wait for me to tell you which task to start."

---

## 2. The Golden Rule — How These Files Work Together

Every coding session uses this formula. Memorize it.

```
AI_WORKFLOW_RULES.md     ← HOW the AI behaves (always attached)
   +
CODING_RULES.md          ← HOW the code is written (always attached)
   +
TASK_QUEUE.md            ← WHAT to build next (pick one task)
   +
TECH_SPEC.md             ← EXACT blueprints (almost always attached)
   +
FILE_STRUCTURE.md        ← WHERE the file goes (attach when creating files)
   +
ARCHITECTURE.md          ← WHY components connect this way (attach when needed)
   +
PRD.md                   ← WHY the project exists (attach when needed)
```

**The minimum set for any coding task is:**
`AI_WORKFLOW_RULES.md` + `CODING_RULES.md` + `TECH_SPEC.md`

**The maximum set (when creating a brand new file) is all 7 files.**

---

## 3. Scenario 1 — Starting a Fresh Coding Session

**Situation:** You just opened your IDE. You want to start coding.

**Step 1: Open `TASK_QUEUE.md` yourself. Find the first unchecked task.**

Look at the task. Note the "Context Files" listed. Those are the files you need to attach.

**Step 2: Paste this exact opening prompt into the AI:**

```
I am working on the RTOSTwin project. Before we write any code, read these files
and acknowledge that you understand them:

1. @AI_WORKFLOW_RULES.md — This defines HOW you must work with me.
   Confirm you understand the 5-step cycle.

2. @CODING_RULES.md — This defines HOW you write the code.
   List the top 5 forbidden patterns from Section 2.

3. @TASK_QUEUE.md — This is our task list.
   Tell me what the next unchecked task is and what its verification gate is.

Do NOT write any code yet. Wait for my instructions.
```

**Step 3: The AI will respond with a summary of the rules and the next task. Then you say:**

```
Good. Let's start on Task [N]: [title from TASK_QUEUE.md].
Also attach: @TECH_SPEC.md @FILE_STRUCTURE.md
Begin Chunk 1. Follow the 5-step cycle.
```

**What to expect:** The AI will ANNOUNCE what it will do, write a small chunk of code, then EXPLAIN it line by line in plain English, then ask if you have questions, then ask permission to continue.

---

## 4. Scenario 2 — Picking and Executing the Next Task

**Situation:** You know what you want to build. You open TASK_QUEUE.md, find the task, and follow the task's own instructions.

Each task in TASK_QUEUE.md has this structure:

```
### Task N: [Title]
Prompt: "[exact text to give the AI]"
Context Files: [list of files to attach]
Output: [file(s) that will be created]
Gate: [how to verify the task is done correctly]
```

**How to execute it — word for word:**

> **Step 1:** Copy the "Prompt" field from the task exactly.
> **Step 2:** Prepend these lines to it:
>
> ```
> Follow AI_WORKFLOW_RULES.md (5-step cycle, one chunk at a time).
> Follow CODING_RULES.md for all code.
> Task description:
> [PASTE THE TASK PROMPT HERE]
> ```
>
> **Step 3:** Attach the "Context Files" listed under that task.
>
> **Step 4:** After the AI finishes, run the "Gate" verification check yourself before marking it done.

**Real example for Task 3 (Framer + CRC):**

```
Follow @AI_WORKFLOW_RULES.md (5-step cycle, one chunk at a time).
Follow @CODING_RULES.md for all code.

Task: Create `agent/core/framer.h` first, then `agent/core/framer.c` in the next chunk.
Implement `crc16_ccitt()` exactly as specified in @TECH_SPEC.md Section 2.3.
Also implement `frame_packet()` as specified in Section 3.3, including `packet_type` and `timestamp_ticks`.
All buffers are static. No malloc.

After each chunk, wait for my permission before the next one.
```

**Verification gate for Task 3:**
Run this in C after the code is written:
```c
// Expected: 0x29B1
assert(crc16_ccitt((uint8_t*)"123456789", 9) == 0x29B1);
```

---

## 5. Scenario 3 — Writing C Firmware Code

**Situation:** You are writing any `.c` or `.h` file for the agent (the MCU side).

**Files to always attach:**
- `AI_WORKFLOW_RULES.md` (the workflow)
- `CODING_RULES.md` (the rules)
- `TECH_SPEC.md` (exact struct definitions and function signatures)
- `FILE_STRUCTURE.md` (to confirm the correct file path)

**The prompt pattern:**

```
Read @TECH_SPEC.md Section [X] for the exact struct/function definition.
Read @FILE_STRUCTURE.md to confirm the file path.
Follow @CODING_RULES.md — specifically: no malloc, static buffers, C99 only, volatile where required.

Now implement [specific thing] as one chunk per function.
Start with Chunk 1: [first function name].
Follow the 5-step cycle from @AI_WORKFLOW_RULES.md.
```

**Real example — writing `snapshot.c`:**

```
Read @TECH_SPEC.md Section 3.1 for the function signatures.
Read @TECH_SPEC.md Section 1.1 for the `full_snapshot_t` struct.
Read @FILE_STRUCTURE.md to confirm this goes in `agent/core/snapshot.c`.
Follow @CODING_RULES.md — no malloc, use static s_ prefix for file-scope variables,
use taskENTER_CRITICAL() around FreeRTOS API calls, volatile on idle counters.

Chunk 1: Implement `snapshot_init()`. Nothing else yet.
Explain what every line does, including what 'static' means and why we use it.
Wait for my permission before Chunk 2.
```

**C-specific things to watch for (from CODING_RULES.md):**

| If the AI does this | Say this |
|---|---|
| Uses `malloc()` or `pvPortMalloc()` | "You violated CODING_RULES.md §2. Replace with a static buffer at file scope. Re-explain why malloc is forbidden in embedded systems." |
| Uses `float` or `double` | "Forbidden by CODING_RULES.md §2. Use integer math only. Explain what fixed-point arithmetic is." |
| Uses `printf()` inside a sensor function | "Forbidden. Printf can block and uses heap internally. Explain why DMA UART is required." |
| Names a variable without the `s_` prefix | "Naming violation. All file-scope statics must use the `s_` prefix per CODING_RULES.md §3." |
| Writes a recursive function | "Forbidden on embedded. Stack is only 2KB. Use iteration." |

---

## 6. Scenario 4 — Writing Python Bridge Code

**Situation:** You are writing any `.py` file for the bridge (the PC/host side).

**Files to always attach:**
- `AI_WORKFLOW_RULES.md`
- `CODING_RULES.md`
- `TECH_SPEC.md` (for Python function signatures and metric names)
- `ARCHITECTURE.md` (for how this module connects to others)

**The prompt pattern:**

```
Read @TECH_SPEC.md Section [4.X] for the Python class/function signatures.
Read @ARCHITECTURE.md 'Component 2: Python Bridge' for the data flow.
Follow @CODING_RULES.md — full type hints required, dataclasses for all data objects,
no time.sleep() in main loop, no bare except clauses.

Implement [class name] one method at a time.
Chunk 1: The class definition and __init__ method only.
Follow the 5-step cycle from @AI_WORKFLOW_RULES.md.
```

**Real example — writing the OOM Analyzer:**

```
Read @TECH_SPEC.md Section 4.2 for the OOMAnalyzer class signature.
Read @TECH_SPEC.md Section 5.2 for the two detection algorithms (linear regression + rolling minimum).
Follow @CODING_RULES.md — explicit named imports, type hints on everything, dataclasses.

Chunk 1: Implement the `OOMAnalyzer.__init__()` method.
Explain what each parameter does (window_size, min_r_squared, rolling_min_threshold, total_heap_bytes).
Explain what a sliding window means in plain English.
Wait for my permission.
```

**Python-specific things to watch for:**

| If the AI does this | Say this |
|---|---|
| Uses `time.sleep()` in a loop | "Forbidden by CODING_RULES.md §2. This blocks the entire bridge. Use asyncio.sleep() instead. Explain what blocking means." |
| Writes `except:` or `except Exception:` | "Forbidden. Catches all errors silently. Use specific exception types. Explain the difference." |
| Uses a global variable | "Forbidden. Breaks multi-device support. Use class instances with explicit state." |
| Missing type hints on a function | "CODING_RULES.md requires type hints on ALL function signatures. Add them and explain what each type annotation means." |
| Uses `from module import *` | "Forbidden by CODING_RULES.md. Use explicit named imports only. Explain why." |

**Important for the Python bridge:** You can develop and test the ENTIRE Python bridge without any physical hardware. Use `mock_device.py` (Task 7) to generate fake packets and pipe them into the decoder. This is by design.

---

## 7. Scenario 5 — Writing Unit Tests

**Situation:** The task requires writing tests (C with Unity or Python with pytest).

**Files to attach:**
- `AI_WORKFLOW_RULES.md`
- `CODING_RULES.md`
- `TECH_SPEC.md` (for test vectors — e.g., CRC must equal `0x29B1`)
- The source file being tested (e.g., `framer.c` or `decoder.py`)

**The rule from CODING_RULES.md §5:** Every function must have at minimum:
1. A happy-path test (known-good inputs, verified outputs)
2. An error-path test (invalid/edge inputs)
3. A boundary test (MAX_TASKS, buffer full, sequence wrap 65535→0)

**Prompt for C tests:**

```
Read @TECH_SPEC.md for the exact test vectors (e.g., crc16_ccitt("123456789") == 0x29B1).
Read @CODING_RULES.md §5 for the testing requirements.

I need tests for `crc16_ccitt()` and `frame_packet()` using the Unity framework.
Chunk 1: Write the test file header, includes, and test runner structure only.
Explain what Unity is and how the TEST(), RUN_TEST(), and UNITY_BEGIN()/UNITY_END() macros work.
Wait for permission.
```

**Prompt for Python tests:**

```
Read @CODING_RULES.md §5 for testing requirements.
The source file being tested is [attached: decoder.py].

Write pytest tests for PacketDecoder.
Chunk 1: Create conftest.py with a fixture for a known-good packet byte sequence.
Explain what a pytest fixture is, why we use conftest.py, and how fixtures get injected.
Wait for permission.
```

---

## 8. Scenario 6 — The AI Hallucinated or Made a Mistake

**Situation:** The AI generated code that is wrong — wrong struct fields, wrong function name, missing `static`, etc.

**Do NOT just say "fix it." Be specific.** Point to the exact document that defines the correct version.

**Templates:**

**For a wrong struct field:**
```
Stop. You invented the field name `task_runtime_ms`. That does not exist.
Look at @TECH_SPEC.md Section 1.1. The correct field name is `runtime_ticks` of type uint32_t.
Fix only that field. Explain where the field comes from and what it measures.
```

**For a wrong function signature:**
```
Stop. Your function signature is wrong.
Check @TECH_SPEC.md Section 3.3. The correct signature is:
`uint16_t frame_packet(const uint8_t *payload, uint16_t payload_len, uint8_t packet_type, uint32_t timestamp_ticks, uint8_t *out_buf, uint16_t out_buf_size);`
You changed the parameter names and types. Rewrite only the signature.
Explain what each parameter is and what the return value means.
```

**For a forbidden pattern:**
```
Stop. You used malloc() on line 14.
Read @CODING_RULES.md §2 (Forbidden Patterns — C Agent).
Malloc is absolutely forbidden. Replace it with a static buffer declared at file scope.
Explain why dynamic allocation is dangerous in embedded real-time systems.
Then rewrite the function without malloc.
```

**For a completely hallucinated module:**
```
Stop. You created a module called `metrics_aggregator.c` that does not exist in the project.
Check @FILE_STRUCTURE.md for the complete file list.
Check @ARCHITECTURE.md for the component map.
Delete your output. Explain what the actual correct next module to write is.
```

---

## 9. Scenario 7 — A Team Member (VNV or Partner) Is Asking How to Start

**Situation:** A teammate is onboarding and needs to know what to do.

**Direct them like this:**

> "Here is how to get started on your assigned deliverables:"
>
> 1. **Read your role document** (e.g., `PRD/roles/vnv_role_assignment.md`) to understand exactly what you own.
> 2. **Read `TASK_QUEUE.md`**. Find the tasks that match your deliverables. Your tasks are the relevant numbered tasks in the queue.
> 3. **Open your AI IDE.** At the start of every session, attach: `AI_WORKFLOW_RULES.md`, `CODING_RULES.md`.
> 4. **For any coding task, also attach:** `TECH_SPEC.md` (for exact struct/function specs) and `FILE_STRUCTURE.md` (for correct file paths).
> 5. **Copy the exact prompt from TASK_QUEUE.md** for your current task. Follow the 5-step cycle.
> 6. **Before accepting any output,** check: Does it follow CODING_RULES.md? Does it match TECH_SPEC.md?

**For VNV specifically:** Your deliverables are Tasks 3, 4, 5, 6, 7, 11, 12, 14, 15, 16, 17, 18, 20, 21 in TASK_QUEUE.md. You can start Tasks 5–7 (Python decoder + mock device) immediately without any hardware.

---

## 10. Scenario 8 — The AI Goes Off-Track or Gets Too Big

**Situation:** The AI starts implementing two modules at once, or writes 200 lines in one go, or starts asking what you want instead of following the task queue.

**Immediately invoke the workflow rules:**

```
Stop. You are violating AI_WORKFLOW_RULES.md.
You wrote [X lines / multiple functions / multiple files] in one go.
A chunk is ONE FUNCTION or ONE STRUCT or ONE FILE SCAFFOLD.

Start over from [specific thing the AI should write next].
Write ONLY that one chunk.
Then STOP and explain it. Then ask for permission.
```

**If the AI is generating code for a future task without being asked:**

```
Stop. I did not ask you to implement [module name].
We are currently on Task [N]: [title].
Complete this task and only this task before we move on.
Check TASK_QUEUE.md to confirm the scope of Task N.
```

---

## 11. Scenario 9 — Resuming a Session After a Break

**Situation:** You closed your IDE. You're coming back the next day. You need to pick up where you left off.

**Step 1:** Open `TASK_QUEUE.md`. Find the first task that is NOT marked done. That's where you resume.

**Step 2:** Use this resume prompt:

```
We are resuming work on RTOSTwin.

Read @AI_WORKFLOW_RULES.md (the workflow rules).
Read @CODING_RULES.md (the coding standards).
Read @TASK_QUEUE.md and find the first incomplete task.

Tell me:
1. Which task we are on
2. What has already been done for this task (if anything)
3. What the next chunk is
4. What files I should also attach for this task

Do not write any code yet. Just orient us.
```

**The AI will read TASK_QUEUE.md and give you a project status report.** You then say "go" and the session proceeds from the correct point.

---

## 12. Scenario 10 — Debugging a Failing Test or Bug

**Situation:** A test is failing or the code is producing wrong output.

**Files to attach:**
- `TECH_SPEC.md` (to verify what the correct output should be)
- The failing test file
- The source file being tested
- `CODING_RULES.md` (to check if a forbidden pattern caused the bug)

**Prompt pattern:**

```
A test is failing. Here is the failing test and the error output:
[paste test file]
[paste error output]

Before guessing the cause, check:
1. @TECH_SPEC.md Section [X] — what is the expected correct output?
2. @CODING_RULES.md §4 — is there an error handling rule that was violated?

Then diagnose the root cause in plain English.
Then propose ONE fix as a single chunk.
Explain every change you make and why.
Do NOT rewrite the whole file. Only change what is broken.
```

**Real example — CRC test fails:**

```
The test `test_crc16_ccitt_standard_vector` is failing.
Expected: 0x29B1
Got: 0x0000

Check @TECH_SPEC.md Section 2.3.
The algorithm must have: Polynomial 0x1021, Initial value 0xFFFF, No reflection, No final XOR.
Identify which of those four parameters is wrong in my implementation.
Fix only the CRC function. Show me the before/after diff. Explain each line.
```

---

## 13. Cheat Sheet — Quick Reference

Keep this section open while you work.

### Which Files to Attach — by Task Type

| Task Type | Minimum Files to Attach |
|---|---|
| Starting a session | `AI_WORKFLOW_RULES.md`, `CODING_RULES.md`, `TASK_QUEUE.md` |
| Writing any C code | + `TECH_SPEC.md`, `FILE_STRUCTURE.md` |
| Writing any Python code | + `TECH_SPEC.md`, `ARCHITECTURE.md` |
| Writing unit tests | + `TECH_SPEC.md`, the source file being tested |
| Writing main/entry point | + `ARCHITECTURE.md`, all module files |
| Writing build/CI files | + `FILE_STRUCTURE.md`, `CODING_RULES.md` |
| Writing documentation/README | + `PRD.md`, `ARCHITECTURE.md` |
| Debugging a bug/test | + `TECH_SPEC.md`, failing file, source file |
| Fixing a coding rules violation | + `CODING_RULES.md` |
| Checking if a feature is in scope | + `PRD.md` |

---

### Emergency Correction Phrases

| Problem | Exact phrase to use |
|---|---|
| AI wrote too much at once | "Stop. One chunk at a time. Read AI_WORKFLOW_RULES.md §2." |
| AI skipped an explanation | "Explain every line. Assume I know nothing. Read AI_WORKFLOW_RULES.md §3." |
| AI used malloc | "Forbidden. Read CODING_RULES.md §2. Replace with static buffer." |
| AI used wrong struct fields | "Stop. Read TECH_SPEC.md Section 1.1 for the exact struct definition." |
| AI put file in wrong folder | "Stop. Read FILE_STRUCTURE.md for the correct path." |
| AI did more than one task | "Stop. We are only on Task N. Read TASK_QUEUE.md." |
| AI used undefined jargon | "Explain what [term] means. Assume I've never heard it before." |
| AI is unsure about a decision | "State your options. Read AI_WORKFLOW_RULES.md §4 (Uncertainty Flag)." |
| AI made a mistake silently | "Acknowledge the mistake explicitly. Explain what went wrong. Then fix it." |

---

### The 5-Step Cycle (From AI_WORKFLOW_RULES.md)

```
1. 📋 ANNOUNCE  → AI states: file, goal, scope, dependencies
2. 💻 EXECUTE   → AI writes ONE chunk of code only
3. 📖 EXPLAIN   → AI explains every line assuming you know nothing
4. ❓ ASK       → AI asks: "Any questions?"
5. ⏭️  WAIT     → AI asks: "Permission to proceed to Chunk N+1?"

YOU TYPE: "continue" or "next" or "go" to proceed.
YOU TYPE: "explain [topic]" for more detail.
YOU TYPE: "change [description]" to fix before proceeding.
```

---

*RTOSTwin v1.0 — AI Usage Guide | Last updated: March 2026*

