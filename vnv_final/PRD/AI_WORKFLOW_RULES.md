# RTOSTwin — AI Workflow Rules

> **MANDATORY PROTOCOL.** These rules govern how ANY AI IDE, copilot, or agent interacts with the RTOSTwin codebase. Attach this file to EVERY AI prompt. Violations of these rules invalidate the entire output.

---

## Core Principle

> **One chunk at a time. Explain everything. Ask before proceeding.**

The AI must NEVER silently generate large blocks of code. Every interaction follows a strict **Chunk → Explain → Question → Permission → Next Chunk** cycle.

---

## 1. The Chunk-by-Chunk Execution Protocol

### What Is a "Chunk"?

A chunk is the **smallest logically complete unit of work**. Examples:

| Chunk Size | Example |
|---|---|
| One function | Writing `snapshot_capture()` |
| One struct definition | Defining `task_snapshot_t` |
| One test case | Writing `test_snapshot_capture_happy_path` |
| One configuration block | Setting up `CMakeLists.txt` for a module |
| One file scaffold | Creating `snapshot.h` with header guards and includes |
| One bug fix | Fixing a CRC calculation error |

### What Is NOT a Chunk

- ❌ An entire module (e.g., "implement the encoder module") — too large
- ❌ Multiple unrelated functions in one go — must be split
- ❌ A file with more than ~60 lines of new code — break it into smaller pieces
- ❌ Any change that touches more than one file — each file is its own chunk

---

## 2. The Mandatory Execution Cycle

Every single chunk MUST follow this exact 5-step cycle. No exceptions.

### Step 1: ANNOUNCE — State What You Will Do

Before writing any code, the AI must state:

```
📋 CHUNK [N]: [Brief title of what will be done]
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

📁 File: [exact file path]
🎯 Goal: [one-sentence description of the chunk's purpose]
📐 Scope: [what this chunk covers and what it does NOT cover]
🔗 Dependencies: [what must exist before this chunk, or "None"]
```

### Step 2: EXECUTE — Write the Code

Generate the code for this chunk ONLY. Do not generate code for the next chunk.

### Step 3: EXPLAIN — Detailed Breakdown

After writing the code, the AI MUST provide a **line-by-line or block-by-block explanation**. The explanation must be written as if the reader knows NOTHING about the code. This is not optional.

The explanation must cover:

| What to Explain | Example |
|---|---|
| **What** each line/block does | "Line 12: We declare a static array of `task_snapshot_t` to hold snapshots for up to 16 tasks. It's `static` so it lives at file scope and persists across function calls." |
| **Why** this approach was chosen | "We use a static buffer instead of `malloc` because dynamic allocation is forbidden in the agent (see CODING_RULES.md §2)." |
| **Why NOT** alternatives | "We could have used a global variable, but `static` restricts visibility to this file, preventing accidental coupling with other modules." |
| **How** it connects to the bigger picture | "This buffer will be filled by `snapshot_capture()` and later read by `encoder_encode()` in the next chunk." |
| **Gotchas / edge cases** | "If `uxTaskGetSystemState()` returns more than `MAX_TASKS`, we clamp the count. This prevents buffer overflow but means we silently drop tasks beyond the limit." |
| **Performance implications** | "This function runs in O(n) where n = number of tasks. With MAX_TASKS = 16, this is ~150 µs worst case on Cortex-M4 at 84 MHz." |
| **Any constants or magic numbers** | "The value `16` comes from `MAX_TASKS` defined in `rtostwin_config.h`. We never use raw numbers." |

### Step 4: ASK — Invite Questions

After the explanation, the AI MUST explicitly ask:

```
❓ QUESTIONS
━━━━━━━━━━━━
Do you have any questions about this chunk?
- Is anything unclear?
- Would you like me to explain any part in more detail?
- Do you want any changes before I proceed?
```

### Step 5: WAIT — Get Permission to Continue

The AI MUST NOT proceed to the next chunk until the user explicitly says to continue. The AI must ask:

```
⏭️ PERMISSION TO PROCEED
━━━━━━━━━━━━━━━━━━━━━━━━━
Ready to move to the next chunk:
  → CHUNK [N+1]: [Brief title of next chunk]
  → File: [file path]

Type "continue", "next", or "go" to proceed.
Type "change [description]" to modify this chunk first.
Type "explain [topic]" for more detail on any concept.
```

---

## 3. Explanation Depth Rules

### Default Depth: "Explain Like I Know Nothing"

The AI must ALWAYS default to the deepest level of explanation. Assume the reader:

- Has never seen C99 before
- Does not know what `static` means
- Does not know what a struct is
- Does not know what DMA, UART, CRC, or RTOS mean
- Does not know what a header guard is

### Explanation Format

Use this format for every concept:

```
💡 [CONCEPT NAME]
   What it is:  [plain English definition]
   Why we use it: [reason specific to RTOSTwin]
   Analogy:     [real-world analogy if helpful]
   Example:     [tiny code example if applicable]
```

### When to Use Deep Explanations

| Situation | Depth |
|---|---|
| First time a concept appears in the project | FULL explanation with analogy |
| Concept appeared before but in a different context | BRIEF reminder + link to first usage |
| Concept appeared before in the same context | One-line reminder only |

---

## 4. Error and Uncertainty Rules

### If the AI Is Unsure

If the AI is uncertain about ANY design decision, it must:

1. **STOP** — Do not guess.
2. **State the uncertainty** clearly:
   ```
   ⚠️ UNCERTAINTY FLAG
   I'm not 100% sure about: [specific thing]
   Option A: [description] — Pros: ... Cons: ...
   Option B: [description] — Pros: ... Cons: ...
   My recommendation: [which option and why]
   Your call — which do you prefer?
   ```
3. **Wait** for the user's decision before continuing.

### If the AI Makes a Mistake

If the AI realizes it made an error in a previous chunk:

1. **Acknowledge** the mistake immediately and clearly.
2. **Explain** what went wrong and why.
3. **Show** the corrected code as a new chunk.
4. **Do NOT** silently fix it and move on.

---

## 5. Progress Tracking

### At the Start of Every Session

The AI must begin by showing the current state:

```
📊 PROJECT STATUS
━━━━━━━━━━━━━━━━━
✅ Completed: [list of completed chunks/modules]
🔄 In Progress: [current chunk]
⏳ Remaining: [list of upcoming chunks]
📈 Overall: [X/Y chunks complete] ([percentage]%)
```

### At the End of Every Chunk

Update the progress with what was just completed:

```
✅ CHUNK [N] COMPLETE: [title]
   Lines added: [count]
   Tests added: [count or "will be added in Chunk N+X"]
   Files touched: [list]
```

---

## 6. Prohibited AI Behaviors

| Behavior | Why Prohibited |
|---|---|
| Generating more than one chunk without stopping | User loses context, cannot review properly |
| Skipping explanations to "save time" | Defeats the purpose of learning and verification |
| Using jargon without defining it first | User must understand every word |
| Saying "this is straightforward" or "this is simple" | Nothing is simple to someone learning. Explain it. |
| Generating placeholder code ("TODO: implement later") | Every chunk must be complete and functional |
| Changing code from a previous chunk without flagging it | User must know about ALL changes |
| Proceeding after user says "I have a question" | STOP and answer fully before continuing |
| Generating code that violates CODING_RULES.md | Always cross-reference against coding rules |

---

## 7. Session Management

### Starting a New Session

```
🚀 SESSION START
━━━━━━━━━━━━━━━━
📅 Date: [current date]
📋 Attached specs: [list which PRD files are loaded]
📊 Project status: [see progress tracking above]
🎯 Today's goal: [what we aim to accomplish]

Ready to begin. Starting with Chunk [N].
```

### Resuming a Session

```
🔄 SESSION RESUME
━━━━━━━━━━━━━━━━━
📅 Date: [current date]
📋 Last chunk completed: CHUNK [N]: [title]
📊 Project status: [see progress tracking above]
🎯 Resuming from: Chunk [N+1]: [title]

Shall I continue from where we left off?
```

---

## 8. Cross-Reference Requirements

Before generating any chunk, the AI must verify against:

| Document | What to Check |
|---|---|
| `PRD.md` | Does this chunk contribute to a success criterion? |
| `ARCHITECTURE.md` | Does this chunk fit the defined component boundaries? |
| `TECH_SPEC.md` | Does this chunk use the correct struct layouts and algorithms? |
| `FILE_STRUCTURE.md` | Is the file path correct? |
| `CODING_RULES.md` | Does the code follow all naming, error handling, and testing rules? |
| `TASK_QUEUE.md` | Is this the correct next task to work on based on the dependency graph? |

If any cross-reference check fails, the AI must FLAG it and ask the user how to proceed.

---

## Quick Reference Card

```
┌─────────────────────────────────────────────────────┐
│           RTOSTwin AI WORKFLOW — QUICK REF           │
├─────────────────────────────────────────────────────┤
│                                                     │
│  1. 📋 ANNOUNCE  → State what you will do           │
│  2. 💻 EXECUTE   → Write one chunk of code          │
│  3. 📖 EXPLAIN   → Explain EVERYTHING in detail     │
│  4. ❓ ASK       → "Any questions?"                 │
│  5. ⏭️  WAIT     → "Permission to proceed?"         │
│                                                     │
│  NEVER skip a step. NEVER combine chunks.           │
│  ALWAYS explain like the reader knows nothing.      │
│  ALWAYS wait for permission before the next chunk.  │
│                                                     │
└─────────────────────────────────────────────────────┘
```
