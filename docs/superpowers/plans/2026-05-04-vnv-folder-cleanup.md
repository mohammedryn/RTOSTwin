# VNV Folder Purge and Clean Plan

Date: 2026-05-04
Branch: `chore/vnv-folder-cleanup`
Worktree: `D:\digital_twin\.worktrees\chore\vnv-folder-cleanup`

## Goal

Clean `vnv_final/` into a source-only, reviewable drop by removing generated garbage, deleting stale or misleading artifacts, and tightening any obviously messy files without changing the intended product scope.

## Baseline Facts

- Current Python bridge test baseline is healthy: `python -m pytest vnv_final\bridge\tests -q` passes (`10 passed`).
- `vnv_final/` contains committed generated Python cache artifacts.
- `vnv_final/` also contains at least one stale audit document that no longer matches the code it sits beside.
- A few files behave like one-off demos or manual test harnesses rather than clean repository assets.

## Strict Task Breakdown

### Phase A - Freeze the exact purge set

1. Record the exact generated garbage already confirmed in `vnv_final/`.
   - Delete `vnv_final/bridge/__pycache__/`
   - Delete `vnv_final/bridge/tests/__pycache__/`
   - Delete these compiled Python artifacts:
     - `vnv_final/bridge/__pycache__/config.cpython-313.pyc`
     - `vnv_final/bridge/__pycache__/decoder.cpython-311.pyc`
     - `vnv_final/bridge/__pycache__/decoder.cpython-313.pyc`
     - `vnv_final/bridge/__pycache__/device_registry.cpython-313.pyc`
     - `vnv_final/bridge/__pycache__/mock_device.cpython-311.pyc`
     - `vnv_final/bridge/__pycache__/oom_analyzer.cpython-311.pyc`
     - `vnv_final/bridge/__pycache__/oom_analyzer.cpython-313.pyc`
     - `vnv_final/bridge/__pycache__/otlp_exporter.cpython-313.pyc`
     - `vnv_final/bridge/__pycache__/prometheus_exporter.cpython-313.pyc`
     - `vnv_final/bridge/__pycache__/state_manager.cpython-313.pyc`
     - `vnv_final/bridge/tests/__pycache__/conftest.cpython-311-pytest-9.0.3.pyc`
     - `vnv_final/bridge/tests/__pycache__/conftest.cpython-313-pytest-9.0.2.pyc`
     - `vnv_final/bridge/tests/__pycache__/integration_test.cpython-311-pytest-9.0.3.pyc`
     - `vnv_final/bridge/tests/__pycache__/test_decoder.cpython-311-pytest-9.0.3.pyc`
     - `vnv_final/bridge/tests/__pycache__/test_decoder.cpython-313-pytest-9.0.2.pyc`
     - `vnv_final/bridge/tests/__pycache__/test_oom_analyzer.cpython-311-pytest-9.0.3.pyc`
     - `vnv_final/bridge/tests/__pycache__/test_oom_analyzer.cpython-313-pytest-9.0.2.pyc`

2. Record the stale or misleading non-code artifacts that should be deleted outright.
   - Delete `vnv_final/log.md`
     - Reason: it is an old audit log that claims multiple Python files are unparseable because they start with `/**`, but the current `vnv_final/bridge/*.py` files are valid Python and the bridge tests already pass.
   - Delete `vnv_final/RTOSTwin_Complete_Report (1).md`
     - Reason: the `(1)` suffix marks it as a duplicate-style export, and `vnv_final/docs/reports/RTOSTwin_Complete_Report.md` already serves as the canonical report location.

### Phase B - Clean repo hygiene files

3. Replace the nested ignore file with something intentional.
   - Refactor `vnv_final/.gitignore`
   - Current problem: it appears to be a copy of the root repo ignore list and does not reflect the nested folder's needs.
   - Target outcome: keep only rules that make sense for `vnv_final/`, such as `__pycache__/`, `*.pyc`, and other local build/runtime artifacts generated from inside that subtree.

### Phase C - Triage suspicious or messy code/support files

4. Review script-style test assets and decide whether to delete, rename, move, or refactor them.
   - `vnv_final/bridge/test_e2e.py`
     - Suspicion: named like an automated test but is actually a console-printing demo script with no assertions.
     - Cleanup target: either move/rename it into an explicit demo or tools location, or convert it into a real automated integration test.
   - `vnv_final/bridge/tests/integration_test.py`
     - Suspicion: manual long-running harness, uses subprocesses and console messaging, not structured as a deterministic automated test.
     - Cleanup target: either move it out of the test suite into a manual-run script location, or rewrite it into an assert-based integration test with a bounded runtime.
   - `vnv_final/bridge/tests/test_decoder.py`
     - Suspicion: the `if __name__ == "__main__":` manual runner block is extra noise inside a pytest file.
     - Cleanup target: remove the manual runner or split it into a separate helper script if it still provides value.

5. Clean small code-quality issues that are clearly accidental, not architectural.
   - `vnv_final/bridge/state_manager.py`
     - The `print(state.heap_free_bytes)` line is only in the usage docstring, so it is not executable garbage.
     - Still review the file for consistency while touching nearby cleanup, but do not treat that specific line as a runtime bug.
   - `vnv_final/bridge/mock_device.py`
     - Keep the stderr prints; they are intentional CLI behavior, not debug garbage.

6. Inspect documentation/code contract drift that should be corrected if found during cleanup.
   - `vnv_final/docs/wire_format_spec.md`
   - `vnv_final/agent/core/wire_format.h`
   - Reason: the embedded protocol docs look older and less strict than the current root-level Phase 1 freeze work. If cleanup reveals obvious contradiction inside `vnv_final/` itself, fix or flag it during this branch rather than leaving misleading protocol docs behind.

### Phase D - Purge execution

7. Delete all confirmed garbage files and folders in one scoped cleanup change.

8. Apply the decided hygiene refactors to the remaining files from Phases B and C.

### Phase E - Verification

9. Re-run the Python bridge tests after the purge.
   - Command: `python -m pytest vnv_final\bridge\tests -q`

10. Run a syntax-only compile check across the bridge sources.
   - Command: `python -m compileall vnv_final\bridge`
   - Purpose: catch broken imports or syntax issues after file moves/removals.

11. Run a focused smoke check on the main bridge entry points.
   - Suggested commands:
     - `python vnv_final\bridge\mock_device.py --help`
     - `python vnv_final\bridge\main.py --help`
   - Purpose: verify the CLI entry points still import and start cleanly after the purge.

12. Review the git diff to confirm the branch removed junk without collateral damage.
   - Confirm only intended `vnv_final/` files changed.
   - Confirm no new generated artifacts were re-added during verification.

## Expected Deliverable

After execution, `vnv_final/` should be:

- free of committed `__pycache__` folders and `.pyc` files
- free of the stale `log.md` audit dump
- free of the duplicate-style `RTOSTwin_Complete_Report (1).md`
- cleaner about which files are real tests versus manual demos
- still passing the bridge test suite

## Reminder After Execution

- If we modify any code files during cleanup, run `graphify update .`
- After the branch work is complete, update `roles/rayan_checklist.md` with:
  - what was cleaned in `vnv_final/`
  - what remains as the next recommended step
