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
  The public C integration surface is `vnv_final/agent/include/rtostwin.h`.
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

## Root-Level Historical Artifacts

Raw capture byproducts used during STM32 validation are stored under the
relevant `evidence/` subtree rather than left at the repository root.

`log.md` is retained at the repository root as a referenced historical audit
artifact and is intentionally not treated as the main project narrative.
