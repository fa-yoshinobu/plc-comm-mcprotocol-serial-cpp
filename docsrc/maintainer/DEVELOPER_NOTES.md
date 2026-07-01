# Developer Notes

Audience: maintainers of this repository.

## Documentation Layout

- `docsrc/user/`: user-facing bring-up and usage
- the maintainer archive: validation matrix and one consolidated report per hardware target
- `docsrc/maintainer/`: repository-maintenance notes

## Documentation Boundary

- Keep `docsrc/user/` and the top-level `README.md` focused on current user workflows.
- Keep unresolved hardware evidence, command coverage detail, and backlog policy in
  the maintainer archive or `docsrc/maintainer/`.
- Keep manual-derived operational rules in [MANUAL_DERIVED_RULES.md](MANUAL_DERIVED_RULES.md).
- Label sample defaults as sample defaults. Do not describe example-code defaults as the current
  validated target settings.
- When the practical settings or follow-up state changes, re-check `README.md`, `docs/index.md`,
  and `docsrc/user/` for stale current-looking text.

## Status Terms

Use these terms consistently in validation reports.

- `native pass`: the intended MC command succeeds directly on hardware
- `native ng`: the direct MC command is rejected by the module or PLC
- `hold`: not resolved yet, or intentionally excluded from the active probe set

## Current Native-only Policy

On the validated `RJ71C24-R2` setup, unsupported native commands should stay failed.
Do not add fallback behavior that silently replaces them with other command families.

## Qualified `Un\G` / `Un\HG` Policy

Do not probe standalone `G...` or `HG...` as plain devices. Across MELSEC
profiles, `G` and `HG` are only valid as qualified forms when the selected
profile supports that route.

Keep the `0601/1601` qualified-buffer helper route separate from the
`0401/1401` native-qualified route. The selected profile decides which route is
valid; do not silently replace one route with the other after a target rejects a
request.

For the current Q, L, iQ-L, and iQ-F serial profile decisions, supported
`Un\G` access uses the native-qualified route. iQ-R supports `Un\G` and
`Un\HG` through dedicated native-qualified access.

## C24 Recovery Discipline

The validated `RJ71C24-R2` setup behaves like a strict half-duplex shared UART.

- Do not overlap probes on the same shared serial port.
- Drain stale RX bytes before each hardware probe when prior traffic may have been interrupted.
- If ASCII Format4 communication times out or returns mixed fragments, send ASCII `EOT CRLF` or
  `CL CRLF` before the next probe. This matches the manual's transmission-sequence reset guidance
  for abnormal communication.
- Do not treat successful recovery after `EOT CRLF` or `CL CRLF` as evidence that an unresolved
  native command family is actually supported.

## Request-Shape Conformance

Keep host-side tests aligned with the documented request shapes before blaming hardware:

- Pin representative `1402`, `0406`, `0801`, and `0802` request layouts against manual-backed
  fixtures in `tests/codec_tests.cpp`.
- Match `--plc-profile` to the actual CPU/profile family before interpreting PLC end codes.
- Keep binary non-iQ-R bit and multi-block layouts covered by tests. Count-width and bit-packing
  mistakes are a common source of false hardware mismatches.
- Revalidate helper/buffer workflows per target. Do not project the C24-era behavior onto FX
  targets without fresh hardware results.

## Validation Reporting Rule

When adding new hardware results:

1. Update the maintainer archive.
2. Add or extend the consolidated report for that hardware target in the maintainer archive.
3. Keep the top-level `README.md` summary short and point to the detailed report.
4. Record the native result and PLC end code without masking it with a different command path.

## Difference-First Triage

When a test, build, or probe regresses:

1. Check what changed first.
2. Localize the failing file, request shape, or target-specific area.
3. Reconcile the manual, current implementation, and existing tests.
4. Only then decide whether to change code, tests, or documentation.

Do not patch first and justify later.

## Open Items

Track active unresolved items in [TODO.md](TODO.md).
