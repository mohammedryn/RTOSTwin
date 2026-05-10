# Root Runtime Snapshot - 2026-05-10

This archive preserves the pre-cleanup root runtime stack that previously lived
at the repository root.

Archived from the root during structure cleanup:

- `agent/` implementation files
- `bridge/`
- `dashboard/`
- `grafana/`
- `prometheus/`
- root `docker-compose.yml`
- the previous root `docs/quick_start.md`

Why it was archived:

- the validated, actively maintained implementation lane now lives in
  `vnv_final/`
- keeping duplicate runtime stacks at the root made the repository confusing
- the root repo is now reserved for canonical docs, governance, and the shared
  protocol contract

Important note:

- the canonical root protocol header path `agent/core/wire_format.h` was
  preserved outside this archive because the validated lane still references it
