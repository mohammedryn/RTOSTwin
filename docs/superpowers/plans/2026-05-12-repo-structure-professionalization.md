# Repo Structure Professionalization Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make the repository feel professional and easy to navigate while preserving every validated STM32, bridge, evidence, and documentation path that the project already relies on.

**Architecture:** Keep `vnv_final/` physically intact as the validated implementation subtree, and improve professionalism through documentation authority cleanup, clearer repository mapping, and only low-risk file organization. Preserve existing runtime paths and evidence links; prefer explanation and classification over renaming or relocating active code.

**Tech Stack:** Markdown docs, PowerShell commands, `rg`, existing Python bridge verification commands, graphify.

---

## File Structure

### Files to create

- `docs/repository_layout.md`
- `archive/README.md`

### Files to modify

- `README.md`
- `vnv_final/README.md`
- `docs/quick_start.md`
- `vnv_final/docs/quick_start.md`

### Files to move

- `stm32-profiler-capture.bin` -> `evidence/objective1_stm32/raw/stm32-profiler-capture.bin`

### Files to inspect but leave in place

- `log.md`
- `docs/reports/RTOSTwin_Master_Milestone_Record.md`
- `vnv_final/docs/vnv_repo_completion_status.md`

### Verification targets

- `vnv_final/bridge/tests/test_decoder.py`
- `vnv_final/bridge/tests/test_oom_analyzer.py`
- `vnv_final/bridge/tests/test_prometheus_exporter.py`
- `vnv_final/bridge/tests/test_state_manager.py`
- `vnv_final/bridge/tests/test_bridge_smoke.py`

---

### Task 1: Establish A Clear Repo Map And Root Documentation Authority

**Files:**
- Create: `docs/repository_layout.md`
- Modify: `README.md`
- Test: repository path audit via `rg`

- [ ] **Step 1: Capture the current root-facing references before editing**

Run:

```powershell
rg -n "vnv_final/|docs/quick_start|hardware_validation_2026-05-10|vnv_repo_completion_status|repository layout|validated implementation subtree" README.md docs vnv_final
```

Expected:
- existing `vnv_final/` references are listed
- no shell/parser errors

- [ ] **Step 2: Create `docs/repository_layout.md` with the canonical repository map**

Create `docs/repository_layout.md` with this content:

```md
# Repository Layout

This repository is organized around a professional alias model:

- the repo root is the public-facing documentation and evidence surface
- `vnv_final/` is the validated implementation subtree retained for compatibility
- runtime and validation commands continue to use the validated subtree paths

## Top-Level Layout

- `README.md`
  Public project overview and navigation
- `docs/`
  Reports, specs, plans, validation records, and supporting project documentation
- `evidence/`
  Saved screenshots, terminal captures, and milestone proof artifacts
- `tools/`
  Helper scripts and audit utilities
- `archive/`
  Superseded or historical non-primary material
- `vnv_final/`
  Validated implementation subtree containing the active `agent`, `bridge`,
  `dashboard`, `docs`, and local observability stack assets
- `graphify-out/`
  Generated repository graph artifacts

## Why `vnv_final/` Still Exists

The `vnv_final/` subtree is intentionally retained because it is the current
validated runtime lane used by:

- bridge commands
- hardware validation docs
- Objective 1/2/3 evidence references
- local Docker/Grafana/Prometheus bring-up

It is therefore treated as a compatibility-preserved implementation root until
final soak and signoff are complete.
```

- [ ] **Step 3: Add a repository layout section to `README.md`**

Insert a short section after the current validation/status overview using this
exact block:

```md
## Repository Layout

The repository uses a professional alias model:

- the repo root is the public-facing project surface
- `vnv_final/` is the validated implementation subtree retained for compatibility
- formal reports live under `docs/reports/`
- saved milestone proof lives under `evidence/`

Quick map:

- `docs/` - reports, specs, plans, and validation records
- `evidence/` - screenshots, terminal captures, and proof artifacts
- `tools/` - helper scripts and audits
- `archive/` - superseded material kept for reference
- `vnv_final/` - validated implementation subtree

For a fuller description, see [docs/repository_layout.md](docs/repository_layout.md).
```

- [ ] **Step 4: Clarify the meaning of `vnv_final/` in `README.md`**

Replace the current vague phrasing about `vnv_final/` with this exact wording
where the subtree is first introduced:

```md
`vnv_final/` is the validated implementation subtree retained for compatibility.
It contains the active `agent`, `bridge`, `dashboard`, `semantic-conventions`,
and local observability stack assets used by the current proven workflow.
```

- [ ] **Step 5: Verify the new documentation is discoverable**

Run:

```powershell
rg -n "Repository Layout|validated implementation subtree|docs/repository_layout.md" README.md docs/repository_layout.md
```

Expected:
- one hit for the new `README.md` section
- one hit for the new layout doc

- [ ] **Step 6: Commit**

```bash
git add README.md docs/repository_layout.md
git commit -m "docs: clarify repository layout and implementation subtree"
```

---

### Task 2: Make `vnv_final/README.md` Implementation-Focused Instead Of Competing With The Root README

**Files:**
- Modify: `vnv_final/README.md`
- Modify: `vnv_final/docs/quick_start.md`
- Test: `python vnv_final\bridge\main.py --help`

- [ ] **Step 1: Reframe `vnv_final/README.md` as the validated implementation guide**

Replace the opening section of `vnv_final/README.md` with this exact content:

```md
# Validated Implementation Subtree

This subtree contains the current validated RTOSTwin implementation lane.

It is retained under `vnv_final/` for compatibility with:

- validated bridge commands
- local Docker/Grafana/Prometheus bring-up
- hardware validation documents
- Objective 1/2/3 evidence and runbooks

If you want the project overview, milestone status, or evidence links, start at
the repository root [README.md](../README.md).

If you want the currently supported implementation workflow, continue below.
```

- [ ] **Step 2: Keep `vnv_final/README.md` narrowly focused on implementation use**

Ensure the file keeps or adds these exact subsections, in this order:

```md
## What Lives Here

- `agent/` - MCU-side telemetry implementation
- `bridge/` - Python bridge, exporters, and tests
- `dashboard/` - dashboard assets
- `docs/` - validated implementation quick start and VNV-scoped status
- `semantic-conventions/` - RTOS OpenTelemetry proposal draft
- `docker-compose.yml` - local observability stack bring-up

## Start Here

- [Validated quick start](docs/quick_start.md)
- [Hardware validation record](docs/reports/hardware_validation_2026-05-10.md)
- [VNV verification boundary](docs/vnv_repo_completion_status.md)
```

- [ ] **Step 3: Tighten `vnv_final/docs/quick_start.md` so it clearly assumes subtree execution**

Keep the existing quick start structure, but add this exact note near the top:

```md
This guide assumes you are intentionally working inside the compatibility-preserved
validated implementation subtree `vnv_final/`.

For the public project overview and milestone/evidence summary, use the root
`README.md` first.
```

- [ ] **Step 4: Verify bridge CLI still works from documented paths**

Run:

```powershell
python vnv_final\bridge\main.py --help
python vnv_final\bridge\mock_device.py --help
```

Expected:
- both commands print help text successfully

- [ ] **Step 5: Commit**

```bash
git add vnv_final/README.md vnv_final/docs/quick_start.md
git commit -m "docs: refocus validated subtree readme and quick start"
```

---

### Task 3: Classify Root-Level Legacy And Scratch Artifacts Without Breaking References

**Files:**
- Create: `archive/README.md`
- Modify: `README.md`
- Move: `stm32-profiler-capture.bin`
- Inspect: `log.md`
- Test: path existence checks and reference audit

- [ ] **Step 1: Verify which root-level artifacts are referenced and must stay put**

Run:

```powershell
rg -n "log\.md|stm32-profiler-capture\.bin" README.md docs vnv_final tools
```

Expected:
- `log.md` should show references
- `stm32-profiler-capture.bin` should show zero or near-zero references

- [ ] **Step 2: Create `archive/README.md` to define archival intent**

Create `archive/README.md` with this content:

```md
# Archive

This folder holds superseded, historical, or non-primary material that is kept
for traceability but is not part of the current validated runtime path.

Archive rules:

- do not place active implementation files here
- prefer moving superseded documents here instead of deleting them
- keep validation-critical evidence in `evidence/`, not in `archive/`
```

- [ ] **Step 3: Move the stray raw capture file into evidence**

Move:

```powershell
New-Item -ItemType Directory -Force evidence\objective1_stm32\raw | Out-Null
Move-Item -LiteralPath stm32-profiler-capture.bin -Destination evidence\objective1_stm32\raw\stm32-profiler-capture.bin
```

Then add this note to `README.md` in the evidence/navigation area:

```md
Raw capture byproducts used during STM32 validation are stored under the
relevant `evidence/` subtree rather than left at the repository root.
```

- [ ] **Step 4: Classify `log.md` instead of moving it**

Because `log.md` is explicitly referenced in
`docs/reports/RTOSTwin_Master_Milestone_Record.md`, leave it at the root for
this phase and add this exact sentence to `README.md` or
`docs/repository_layout.md`:

```md
`log.md` is retained at the repository root as a referenced historical audit
artifact and is intentionally not treated as the main project narrative.
```

- [ ] **Step 5: Verify artifact placement and references**

Run:

```powershell
Test-Path stm32-profiler-capture.bin
Test-Path evidence\objective1_stm32\raw\stm32-profiler-capture.bin
rg -n "log\.md|objective1_stm32/raw/stm32-profiler-capture\.bin" README.md docs
```

Expected:
- root `stm32-profiler-capture.bin` -> `False`
- moved file under `evidence\objective1_stm32\raw\...` -> `True`
- `log.md` still referenced where intended

- [ ] **Step 6: Commit**

```bash
git add README.md archive/README.md evidence/objective1_stm32/raw/stm32-profiler-capture.bin
git commit -m "chore: classify root artifacts and move raw capture into evidence"
```

---

### Task 4: Final Consistency And Safety Verification

**Files:**
- Verify only

- [ ] **Step 1: Run bridge test suite**

Run:

```powershell
D:\digital_twin\vnv_final\.venv\Scripts\python.exe -m pytest vnv_final\bridge\tests -q
```

Expected:
- all bridge tests pass

- [ ] **Step 2: Run bridge compile verification**

Run:

```powershell
python -m compileall vnv_final\bridge
```

Expected:
- no syntax errors

- [ ] **Step 3: Audit key validated-path references**

Run:

```powershell
rg -n "vnv_final/|objective1_stm32|objective2_bridge_exports|objective3_oom_validation|RTOSTwinF401RE_clean|COM11" README.md docs vnv_final
```

Expected:
- references remain present and coherent
- no obviously broken relative links are introduced

- [ ] **Step 4: Review the final changed file set**

Run:

```powershell
git diff -- README.md docs/repository_layout.md docs/quick_start.md vnv_final/README.md vnv_final/docs/quick_start.md archive/README.md
```

Expected:
- only documentation/presentation changes plus the raw-capture move
- no active runtime code modified

- [ ] **Step 5: Commit final verification pass**

```bash
git add README.md docs/repository_layout.md docs/quick_start.md vnv_final/README.md vnv_final/docs/quick_start.md archive/README.md
git commit -m "docs: finalize repository professionalization pass"
```

---

## Self-Review

### Spec coverage

This plan covers:

- root README authority cleanup
- explicit repository layout documentation
- `vnv_final/` clarification without renaming it
- low-risk artifact organization
- preservation of validated commands and evidence links
- final safety verification

### Placeholder scan

No placeholder markers such as `TODO`, `TBD`, or “similar to above” remain.

### Type and path consistency

All paths used in this plan match existing repository structure:

- `README.md`
- `docs/quick_start.md`
- `vnv_final/README.md`
- `vnv_final/docs/quick_start.md`
- `evidence/objective1_stm32/`
- `vnv_final/bridge/`

---

Plan complete and saved to `docs/superpowers/plans/2026-05-12-repo-structure-professionalization.md`. Two execution options:

**1. Subagent-Driven (recommended)** - I dispatch a fresh subagent per task, review between tasks, fast iteration

**2. Inline Execution** - Execute tasks in this session using executing-plans, batch execution with checkpoints

Which approach?
