# Root Agent Directory

This root `agent/` directory is intentionally minimal after repository cleanup.

What remains canonical here:

- `core/wire_format.h`

Why it remains here:

- the root protocol contract is still anchored at
  `docs/wire_format_spec.md` and `agent/core/wire_format.h`
- the validated `vnv_final/` lane references that root protocol truth

Where the active implementation lives today:

- `vnv_final/agent/`

Where the archived legacy root implementation was moved:

- `archive/legacy/root-runtime-snapshot-2026-05-10/agent/`
