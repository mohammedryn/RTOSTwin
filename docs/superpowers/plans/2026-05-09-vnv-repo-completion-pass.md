# VNV Repo Completion Pass Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Finish the repo-completable VNV ownership scope in `vnv_final/` by proving the mock-device bridge path, metrics exposition path, and documentation path work cleanly without pretending hardware evidence exists.

**Architecture:** Keep the existing `vnv_final/bridge` design, but tighten the runtime boundary so `main.py` reliably updates Prometheus and optional OTLP exporters from decoded packets. Add targeted pytest coverage around state reconstruction, exporter exposition, and a bridge smoke path driven by mock packets, then upgrade the docs to show a reproducible local demo flow and explicitly separate local proof from external STM32 hardware proof.

**Tech Stack:** Python 3.9+, pytest, prometheus-client, OpenTelemetry SDK, existing `vnv_final/bridge/*` modules, Markdown docs.

---

### Task 1: Add failing tests for state reconstruction and metrics exposition

**Files:**
- Create: `vnv_final/bridge/tests/test_state_manager.py`
- Create: `vnv_final/bridge/tests/test_prometheus_exporter.py`
- Modify: `vnv_final/bridge/tests/conftest.py`

- [ ] **Step 1: Write failing state-manager tests for keyframe and delta reconstruction**
- [ ] **Step 2: Run those tests to verify they fail for the current behavior gaps or missing helpers**
- [ ] **Step 3: Write failing Prometheus exposition tests for required metric names and labels**
- [ ] **Step 4: Run those tests to verify they fail for the current exporter behavior**

### Task 2: Implement the minimal bridge/runtime changes to satisfy the tests

**Files:**
- Modify: `vnv_final/bridge/state_manager.py`
- Modify: `vnv_final/bridge/prometheus_exporter.py`
- Modify: `vnv_final/bridge/otlp_exporter.py`
- Modify: `vnv_final/bridge/main.py`
- Modify: `vnv_final/bridge/config.py`

- [ ] **Step 1: Implement only the state-manager changes needed by the failing tests**
- [ ] **Step 2: Implement only the Prometheus exporter changes needed by the failing tests**
- [ ] **Step 3: Make OTLP explicit and safe for local development while still supporting export when enabled**
- [ ] **Step 4: Wire `main.py` so packet handling updates both exporters through a testable path**
- [ ] **Step 5: Re-run the focused tests until they pass**

### Task 3: Add an end-to-end local proof path and tighten docs

**Files:**
- Create: `vnv_final/bridge/tests/test_bridge_smoke.py`
- Modify: `vnv_final/bridge/manual_integration_demo.py`
- Modify: `vnv_final/docs/quick_start.md`
- Create: `vnv_final/docs/vnv_repo_completion_status.md`

- [ ] **Step 1: Write a failing smoke test for mock packets flowing through the bridge path**
- [ ] **Step 2: Run the smoke test to verify it fails before implementation**
- [ ] **Step 3: Make the minimal runtime/doc updates required for the smoke test and local demo flow**
- [ ] **Step 4: Re-run the smoke test and the full bridge test suite**
- [ ] **Step 5: Record the verified local-only evidence and the remaining external hardware proof gap in docs**
