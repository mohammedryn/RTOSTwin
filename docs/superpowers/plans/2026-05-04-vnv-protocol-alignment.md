# VNV Protocol Alignment Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Align `vnv_final/` with the frozen root-level Phase 1 protocol so the repository has one canonical wire contract and VNV's C/Python code matches it exactly.

**Architecture:** Keep the root protocol files as the only canonical truth: `docs/wire_format_spec.md` and `agent/core/wire_format.h`. Remove VNV's duplicate protocol copies, repoint VNV docs/code to the root truth, then fix the actual implementation drift in the framer, encoder, decoder, mock device, and state reconstruction code so the bytes on the wire match the frozen v1 math.

**Tech Stack:** C99 agent code, Python 3 bridge code, pytest, git worktrees, graphify

---

## File Map

**Canonical protocol files to keep**
- Keep: `docs/wire_format_spec.md`
- Keep: `agent/core/wire_format.h`

**Redundant protocol files to delete**
- Delete: `vnv_final/docs/wire_format_spec.md`
- Delete: `vnv_final/agent/core/wire_format.h`

**VNV C files that must be aligned to the root protocol**
- Modify: `vnv_final/agent/core/framer.h`
- Modify: `vnv_final/agent/core/framer.c`
- Modify: `vnv_final/agent/core/encoder.c`
- Modify: `vnv_final/agent/main.c`
- Modify: `vnv_final/agent/tests/test_framer.c`
- Modify: `vnv_final/agent/tests/test_encoder.c`

**VNV Python files that must be aligned to the root protocol**
- Modify: `vnv_final/bridge/decoder.py`
- Modify: `vnv_final/bridge/tests/conftest.py`
- Modify: `vnv_final/bridge/tests/test_decoder.py`
- Modify: `vnv_final/bridge/state_manager.py`
- Modify: `vnv_final/bridge/mock_device.py`
- Modify: `vnv_final/bridge/manual_e2e_demo.py`

**VNV docs that must stop restating conflicting protocol truth**
- Modify: `vnv_final/PRD/TECH_SPEC.md`
- Modify: `vnv_final/PRD/FILE_STRUCTURE.md`
- Modify: `vnv_final/PRD/TASK_QUEUE.md`
- Modify: `vnv_final/PRD/WAY_TO_USE.md`
- Modify: `vnv_final/docs/reports/RTOSTwin_Complete_Report.md`
- Modify: `roles/rayan_checklist.md`

**Verification targets**
- Test: `vnv_final/bridge/tests/test_decoder.py`
- Test: `vnv_final/bridge/tests/test_oom_analyzer.py`

---

## Drift Ledger To Resolve

- Root spec says `frame_packet()` takes `sequence_num`; VNV framer still owns an internal counter and exposes `framer_reset_sequence()`.
- Root spec says keyframes are serialized field-by-field with no padding; VNV encoder still does `memcpy(out_buf, current, sizeof(full_snapshot_t))`.
- Root spec says keyframe task records are emitted `task_count` times only; VNV bridge mock/parser still assume a fixed padded `MAX_TASKS * 24` block.
- Root spec says `runtime_ticks` is `uint32`; VNV Python code still packs/unpacks task runtime with signed `i`.
- Root spec uses `timestamp_ticks`; VNV decoder still exposes `timestamp_ms`.
- Root header defines delta field IDs and `WF_DELTA_SYSTEM_TAG_BASE`; VNV duplicate header does not.
- Root spec is the canonical protocol truth; VNV still carries a second protocol spec and second header, plus PRD/report text that repeats outdated sequence/framing rules.

---

### Task 1: Delete VNV's Duplicate Protocol Files And Repoint References

**Files:**
- Delete: `vnv_final/docs/wire_format_spec.md`
- Delete: `vnv_final/agent/core/wire_format.h`
- Modify: `vnv_final/PRD/TECH_SPEC.md`
- Modify: `vnv_final/PRD/FILE_STRUCTURE.md`
- Modify: `vnv_final/PRD/TASK_QUEUE.md`
- Modify: `vnv_final/PRD/WAY_TO_USE.md`
- Modify: `vnv_final/docs/reports/RTOSTwin_Complete_Report.md`

- [ ] **Step 1: Confirm every in-tree reference that still assumes VNV owns its own protocol spec/header**

Run:
```powershell
rg -n "wire_format_spec\.md|agent/core/wire_format\.h|wire_format\.h|framer owns|internal static `uint16_t` sequence" vnv_final
```

Expected: matches in `PRD/TECH_SPEC.md`, `PRD/FILE_STRUCTURE.md`, `PRD/TASK_QUEUE.md`, `PRD/WAY_TO_USE.md`, and protocol-report text.

- [ ] **Step 2: Delete the redundant protocol spec file**

Delete:
```text
vnv_final/docs/wire_format_spec.md
```

- [ ] **Step 3: Delete the redundant VNV header file**

Delete:
```text
vnv_final/agent/core/wire_format.h
```

- [ ] **Step 4: Replace VNV PRD/report protocol duplication with root-canonical references**

Make these exact documentation changes:
- In `vnv_final/PRD/TECH_SPEC.md`, replace the local wire-format section with a short note that the canonical v1 contract lives in root `docs/wire_format_spec.md` and root `agent/core/wire_format.h`.
- In `vnv_final/PRD/FILE_STRUCTURE.md`, change `agent/core/wire_format.h -> bridge/decoder.py -> bridge/state_manager.py` to `root agent/core/wire_format.h -> vnv_final/bridge/decoder.py -> vnv_final/bridge/state_manager.py`.
- In `vnv_final/PRD/TASK_QUEUE.md`, rewrite local `wire_format.h` tasks/prompts to reference the root header/spec instead of creating or editing a VNV-local protocol file.
- In `vnv_final/PRD/WAY_TO_USE.md`, change protocol context examples to point at the root canonical files.
- In `vnv_final/docs/reports/RTOSTwin_Complete_Report.md`, replace any local packet-layout/ownership claim with a short pointer to the root frozen Phase 1 spec.

- [ ] **Step 5: Verify that VNV no longer carries duplicate protocol-truth files**

Run:
```powershell
Test-Path vnv_final\docs\wire_format_spec.md
Test-Path vnv_final\agent\core\wire_format.h
```

Expected:
```text
False
False
```

Run:
```powershell
rg -n "vnv_final/docs/wire_format_spec\.md|vnv_final/agent/core/wire_format\.h" vnv_final
```

Expected: no matches.

---

### Task 2: Rewire VNV C Includes To The Root Canonical Header

**Files:**
- Modify: `vnv_final/agent/main.c`
- Modify: `vnv_final/agent/core/framer.c`
- Modify: `vnv_final/agent/tests/test_framer.c`

- [ ] **Step 1: Change VNV C files that include the deleted local header**

Update include lines to explicitly reference the root canonical header:
- `vnv_final/agent/main.c`
- `vnv_final/agent/core/framer.c`
- `vnv_final/agent/tests/test_framer.c`

Target include intent:
```c
#include "../../../agent/core/wire_format.h"
```

Use the correct relative depth per file.

- [ ] **Step 2: Verify no VNV C file still includes the deleted local header path**

Run:
```powershell
rg -n '#include ".*wire_format\.h"' vnv_final\agent
```

Expected: only includes that resolve to the root canonical header.

---

### Task 3: Align The Framer Contract To Snapshot-Owned Sequence Numbers

**Files:**
- Modify: `vnv_final/agent/core/framer.h`
- Modify: `vnv_final/agent/core/framer.c`
- Modify: `vnv_final/agent/main.c`
- Modify: `vnv_final/agent/tests/test_framer.c`

- [ ] **Step 1: Change `frame_packet()` to the frozen v1 signature**

Make `vnv_final/agent/core/framer.h` match the root spec:
```c
uint16_t frame_packet(const uint8_t *payload,
                      uint16_t payload_len,
                      uint8_t packet_type,
                      uint16_t sequence_num,
                      uint32_t timestamp_ticks,
                      uint8_t *out_buf,
                      uint16_t out_buf_size);
```

- [ ] **Step 2: Remove internal framer sequence ownership**

In `vnv_final/agent/core/framer.c`:
- remove `static uint16_t s_seq_num`
- remove `framer_reset_sequence()`
- write header bytes 4-5 from the `sequence_num` argument instead of internal state
- keep CRC coverage exactly over bytes `VERSION..PAYLOAD`

- [ ] **Step 3: Make `main.c` pass the snapshot-owned values**

In `vnv_final/agent/main.c`:
- remove `framer_reset_sequence();`
- change the `frame_packet(...)` call to pass:
  - `pkt_type`
  - `s_current_snapshot.sequence_num`
  - `s_current_snapshot.timestamp_ticks`
- stop passing a fresh `xTaskGetTickCount()` directly into the framer

- [ ] **Step 4: Rewrite framer tests around explicit sequence input**

In `vnv_final/agent/tests/test_framer.c`:
- remove tests about internal counter increment/reset/wrap
- replace them with tests that pass known `sequence_num` values and assert exact little-endian bytes in the header
- add one direct assertion that the root golden delta packet header/CRC bytes are matched for a known test packet

- [ ] **Step 5: Verify framer drift is gone**

Run:
```powershell
rg -n "framer_reset_sequence|s_seq_num|timestamp_ticks Current RTOS tick count|auto-incremented sequence" vnv_final\agent
```

Expected: no remaining internal-sequence ownership language or symbol usage.

---

### Task 4: Replace Raw/Padded Keyframe Serialization With Frozen v1 Keyframe Math

**Files:**
- Modify: `vnv_final/agent/core/encoder.c`
- Modify: `vnv_final/agent/tests/test_encoder.c`
- Modify: `vnv_final/bridge/mock_device.py`
- Modify: `vnv_final/bridge/manual_e2e_demo.py`
- Modify: `vnv_final/bridge/state_manager.py`

- [ ] **Step 1: Stop raw `memcpy(full_snapshot_t)` keyframe output in the C encoder**

In `vnv_final/agent/core/encoder.c`:
- replace the keyframe branch that currently does:
```c
memcpy(out_buf, current, sizeof(full_snapshot_t));
```
- serialize fields in this exact root-spec order:
  1. `sequence_num` (`uint16`)
  2. `timestamp_ticks` (`uint32`)
  3. `task_count` (`uint8`)
  4. `task_count` task records only
  5. `heap_free_bytes` (`uint32`)
  6. `heap_min_ever_bytes` (`uint32`)
  7. `cpu_utilization_pct` (`uint8`)

- [ ] **Step 2: Emit task runtime as unsigned 32-bit**

For each task record in both C and Python serializers/parsers, use unsigned 4-byte runtime encoding:
```text
<BBHI
```
not:
```text
<BBHi
```

- [ ] **Step 3: Remove fixed `MAX_TASKS` padding assumptions from the Python side**

In `vnv_final/bridge/mock_device.py`, `manual_e2e_demo.py`, and `state_manager.py`:
- stop padding keyframes to `MAX_TASKS * 24`
- parse and emit only `task_count` task records
- leave `MAX_TASKS` as a validation cap, not as a required on-wire padding block

- [ ] **Step 4: Rewrite encoder tests around exact payload shape, not `sizeof(full_snapshot_t)`**

In `vnv_final/agent/tests/test_encoder.c`:
- remove assertions like:
```c
assert(size == sizeof(full_snapshot_t));
```
- replace them with exact expected payload-length checks based on `task_count`
- add at least one byte-for-byte assertion against the root golden keyframe payload layout

- [ ] **Step 5: Verify no raw-struct keyframe path remains**

Run:
```powershell
rg -n "memcpy\\(out_buf, current, sizeof\\(full_snapshot_t\\)\\)|sizeof\\(full_snapshot_t\\)|MAX_TASKS \\* 24|<BBHi" vnv_final
```

Expected: no raw keyframe copy path, no padded-keyframe assumption, and no signed runtime packing in protocol code.

---

### Task 5: Align Python Decoder And State Reconstruction Names To The Frozen Contract

**Files:**
- Modify: `vnv_final/bridge/decoder.py`
- Modify: `vnv_final/bridge/tests/conftest.py`
- Modify: `vnv_final/bridge/tests/test_decoder.py`
- Modify: `vnv_final/bridge/state_manager.py`

- [ ] **Step 1: Rename `timestamp_ms` to `timestamp_ticks` in the decoder contract**

In `vnv_final/bridge/decoder.py`:
- rename the `DecodedPacket` field from `timestamp_ms` to `timestamp_ticks`
- update packet construction accordingly

- [ ] **Step 2: Update downstream Python users to consume `timestamp_ticks`**

In `vnv_final/bridge/state_manager.py` and tests:
- replace all `packet.timestamp_ms` usage with `packet.timestamp_ticks`
- update test expectations in `vnv_final/bridge/tests/test_decoder.py`

- [ ] **Step 3: Keep CRC/header parsing exactly aligned with the root spec**

In `vnv_final/bridge/tests/conftest.py` and `test_decoder.py`:
- keep the packet header unpack format as:
```python
struct.unpack("<BBHIH", header_bytes)
```
- add one fixture or assertion that uses the exact root golden delta packet bytes

- [ ] **Step 4: Verify the Python packet contract no longer uses the wrong timestamp name**

Run:
```powershell
rg -n "timestamp_ms" vnv_final\bridge
```

Expected: no matches.

---

### Task 6: Add Golden-Vector Regression Coverage Against The Root Spec

**Files:**
- Modify: `vnv_final/bridge/tests/conftest.py`
- Modify: `vnv_final/bridge/tests/test_decoder.py`
- Modify: `vnv_final/agent/tests/test_framer.c`
- Modify: `vnv_final/agent/tests/test_encoder.c`

- [ ] **Step 1: Copy the exact root golden vectors into test fixtures**

Use these exact packets from `docs/wire_format_spec.md`:
- keyframe full packet ending in CRC `96 85`
- delta full packet ending in CRC `36 2d`
- corrupted delta packet ending in CRC `36 d2`

- [ ] **Step 2: Make Python decoder tests assert the spec vectors directly**

In `vnv_final/bridge/tests/test_decoder.py`:
- decode the golden delta packet and assert:
  - `packet_type == 0x01`
  - `sequence_num == 0x1235`
  - `timestamp_ticks == 0x01020368`
  - payload bytes match `f5 20 1e 00 00 f7 0a`
- feed the corrupted vector and assert no packet is emitted and `drop_count` increments

- [ ] **Step 3: Make C framer/encoder tests derive bytes that match the frozen contract**

In `vnv_final/agent/tests/test_framer.c` and `test_encoder.c`:
- assert exact header bytes, exact CRC bytes, and exact payload length/field layout for at least one known packet
- remove tests that validate the outdated internal-sequence behavior

---

### Task 7: Final Repository Consistency Check

**Files:**
- Modify: `roles/rayan_checklist.md`

- [ ] **Step 1: Re-run the Python verification suite**

Run:
```powershell
python -m pytest vnv_final\bridge\tests -q
```

Expected:
```text
10 passed
```
or higher if new protocol-alignment tests were added.

- [ ] **Step 2: Re-run syntax and CLI smoke checks**

Run:
```powershell
python -m compileall vnv_final\bridge
python vnv_final\bridge\mock_device.py --help
python vnv_final\bridge\main.py --help
```

Expected: all commands succeed with no traceback.

- [ ] **Step 3: Confirm the repository has only one protocol truth**

Run:
```powershell
Test-Path docs\wire_format_spec.md
Test-Path agent\core\wire_format.h
Test-Path vnv_final\docs\wire_format_spec.md
Test-Path vnv_final\agent\core\wire_format.h
```

Expected:
```text
True
True
False
False
```

- [ ] **Step 4: Update the checklist with what was aligned and what remains**

In `roles/rayan_checklist.md`, record:
- VNV duplicate protocol files removed
- framer ownership aligned to snapshot-owned sequence numbers
- keyframe serialization aligned to the frozen unpadded layout
- Python decoder/state naming aligned to `timestamp_ticks`
- next step after this branch: Phase 2 snapshot/profiler stabilization and typed decoder follow-up

- [ ] **Step 5: Refresh the graph after code changes**

Run:
```powershell
graphify update .
```

---

## Notes For Execution

- Do not recreate a second protocol spec or second protocol header under `vnv_final/`.
- Do not preserve `framer_reset_sequence()` behind a compatibility shim; delete the outdated ownership model fully.
- Do not leave any `sizeof(full_snapshot_t)` assertion in keyframe tests.
- Do not leave any protocol parser/generator using signed runtime packing (`i`) where the root spec says `uint32`.
