# Repo Structure Professionalization Design

**Date:** 2026-05-12
**Status:** Proposed
**Scope:** Safe repository-structure cleanup and presentation improvement without breaking the validated STM32 and bridge workflows

---

## 1. Goal

Make the repository look deliberate, professional, and easy to navigate while
preserving every currently validated runtime path, command, and evidence link.

This design explicitly avoids risky renames or relocations that could break:

- the validated `NUCLEO-F401RE` workflow
- the proven `vnv_final/bridge` command paths
- current Docker/Grafana/Prometheus paths
- evidence links embedded in reports
- current developer instructions and validation commands

---

## 2. Problem Statement

The repository is already functionally strong, but it presents a few
professionalism issues:

- the top-level structure mixes product files, reports, evidence, generated
  artifacts, local scratch artifacts, and historical material
- the validated implementation lives under `vnv_final/`, which is a confusing
  name for a long-term product subtree
- there are multiple documentation entry points with overlapping authority
- some root-level files look temporary or machine-local rather than intentional
- the validated path is real, but the repo does not yet explain that structure
  clearly enough for a new reviewer

The repository therefore feels more like an active lab workspace than a
finished, production-minded project.

---

## 3. Design Principles

This cleanup follows five principles:

### 3.1 Preserve validated paths

Anything already validated in the STM32, Objective 1, Objective 2, or Objective
3 lanes must remain callable at the same path unless there is a compatibility
layer that guarantees no breakage.

### 3.2 Prefer clarification over risky relocation

If a folder name is awkward but already embedded in commands, docs, and
evidence, the first step is to reframe and document it rather than rename it.

### 3.3 Single source of authority

The repo root should become the clear public entrypoint, with the validated
implementation subtree explicitly described as an internal delivery lane rather
than an unexplained parallel project.

### 3.4 Archive, do not silently delete

Potentially obsolete or superseded material should be moved into clearly named
archival locations when safe, not casually removed.

### 3.5 Stage risky cleanup after signoff

Large renames such as replacing `vnv_final/` should be deferred until after the
final soak and signoff, when the system is frozen and can be migrated with a
dedicated compatibility pass.

---

## 4. Chosen Approach

### Recommended approach: Professional alias model

The repository will adopt a **professional alias** model:

- the root of the repo becomes the polished public-facing project surface
- `vnv_final/` remains physically intact as the validated implementation subtree
- documentation explains why `vnv_final/` still exists and what role it plays
- low-risk root cleanup improves presentation without changing runtime behavior

This is intentionally more conservative than a full rename. It delivers a
professional structure now, while protecting the already-proven system.

---

## 5. Target Structure

The repository should read conceptually like this:

- `README.md`
  public project overview and navigation
- `docs/`
  reports, specs, plans, architecture, and validation records
- `evidence/`
  captured validation artifacts and screenshots
- `tools/`
  helper scripts and automation utilities
- `archive/`
  superseded or non-primary historical material
- `vnv_final/`
  validated implementation subtree retained for compatibility
- `graphify-out/`
  generated repository graph artifacts

This structure is already close to reality. The cleanup work is therefore
primarily about **clarity, labeling, and organization**, not large-scale file
movement.

---

## 6. Specific Design Decisions

### 6.1 `vnv_final/` remains in place

`vnv_final/` will not be renamed in this phase.

Instead, it will be explicitly described as:

- the validated implementation subtree
- the compatibility-preserved runtime root for current commands
- the delivery lane containing the active `agent`, `bridge`, `dashboard`,
  `semantic-conventions`, and local observability stack assets

This removes ambiguity without risking import-path or documentation breakage.

### 6.2 Root README becomes the public authority

The root [README.md](/D:/digital_twin/README.md) will be treated as the single
public project landing page.

Its responsibilities:

- explain what the project is
- show validation status
- explain the repository layout
- explain what `vnv_final/` is
- link to reports, evidence, and validated workflows

### 6.3 `vnv_final/README.md` becomes implementation-focused

The subtree README should serve as the developer/operator guide for the
validated implementation lane rather than competing with the root README for
project-level authority.

### 6.4 Root clutter is classified

Root-level items will be classified into four categories:

- intentional source-of-truth files
- generated artifacts
- local scratch outputs
- historical/superseded content

Examples of likely cleanup candidates:

- `stm32-profiler-capture.bin`
- `log.md`
- ad hoc root-level learning or scratch materials that are not active runtime
  dependencies

Each candidate will be either:

- documented as intentional
- moved to `archive/`
- relocated into a more appropriate folder
- or left in place if moving it risks breaking workflows

### 6.5 Duplicate documentation authority is reduced

Documentation will be made more intentional:

- root `README.md` = project overview and navigation
- `vnv_final/README.md` = validated implementation usage
- `docs/reports/...` = formal record
- `docs/superpowers/...` = planning/spec history

This structure makes it easier for external readers and future maintainers to
know where to start.

---

## 7. What Will Not Change In This Phase

The following are explicitly deferred:

- renaming `vnv_final/`
- moving `vnv_final/bridge/`, `vnv_final/agent/`, or `vnv_final/dashboard/`
- changing Python import roots
- changing Docker Compose paths
- changing evidence folder names already cited by reports
- changing validated commands used in hardware proof

These are post-signoff tasks, not pre-signoff cleanup tasks.

---

## 8. Execution Plan Shape

This design should be implemented in two low-risk passes.

### Pass 1: Documentation and presentation cleanup

- clarify root README structure
- add a repository map
- explain the role of `vnv_final/`
- reduce duplicate or conflicting doc authority
- document generated/local-only files more clearly

### Pass 2: Low-risk organization cleanup

- move clearly stray root-level artifacts into better locations or `archive/`
- keep runtime paths unchanged
- verify that documented validated commands still work

This sequence gives the repo a more professional face immediately while keeping
technical risk low.

---

## 9. Risks and Mitigations

### Risk: Breaking validated commands

Mitigation:
- do not move the active runtime subtree in this phase

### Risk: Broken evidence links in reports

Mitigation:
- keep `evidence/` paths unchanged

### Risk: Confusion between “current product root” and “implementation subtree”

Mitigation:
- explicitly define both roles in the root README and related docs

### Risk: Cleanup accidentally hides useful historical context

Mitigation:
- prefer `archive/` moves over deletion

---

## 10. Success Criteria

This professionalization pass is successful if:

- a new reader can understand the repository layout quickly from the root README
- the role of `vnv_final/` is clearly explained
- root-level clutter is reduced or intentionally classified
- no validated STM32 or bridge workflow breaks
- no evidence/report links break
- the repo feels like a coherent product rather than an accumulation of
  partially related folders

---

## 11. Recommendation

Proceed with the professional alias model now.

Do **not** rename `vnv_final/` until after final soak and signoff.

Once the project is frozen and all major objectives plus soak are complete, a
separate post-signoff migration can decide whether a full rename is worth the
additional risk and churn.
