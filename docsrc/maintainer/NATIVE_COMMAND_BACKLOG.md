# Native Command Backlog

Audience: maintainers running real-hardware follow-up on unresolved native command families.

This page replaces the old repository-level TODO list for native command follow-up. Keep normal user
docs focused on known-good workflows and keep this page for native-command follow-up policy and
interpretation.

## Public Policy

- Keep unresolved native command behavior visible. Do not add fallback behavior that silently
  swaps in a different command family.
- Keep unresolved native commands in the public API, but describe them as native probes on the
  validated real-hardware setups instead of known-good workflows.
- Treat `read-qualified-words` / `write-qualified-words` over `0601/1601` as the practical public
  path for `U...\\G...` / `U...\\HG...` access on the current setup.
- Treat native qualified commands as unsupported diagnostic probes, not as a supported `U...`
  access path.

## Active Items

Track current follow-up items in [TODO.md](TODO.md).

## Implementation Gaps

Track current implementation gaps in [TODO.md](TODO.md).

## Follow-up Rules

- Add every new hardware result to `docsrc/validation/reports/HARDWARE_VALIDATION.md`.
- Add or extend the consolidated report under `docsrc/validation/reports/` for that hardware
  target, including raw evidence and command examples.
- Keep the top-level `README.md` summary short. Push detailed failure evidence into validation docs.
- Preserve request-shape conformance tests before treating hardware rejection as an encoder bug.
- Record the exact serial settings, PLC model, and native PLC end code for every new result.
- Run shared real-UART probes strictly serially. Parallel access on the same serial port can
  produce mixed RX fragments and invalidate the result.
- Keep FX5U notes aligned with its serial manual: `0801/0802` unsupported, `DX/DY/V/ZR` outside
  the validated subset.
- Do not turn unsupported access paths into backlog items. For long timer / long retentive timer
  contact+coil devices, keep `LTS/LTC/LSTS/LSTC` on the structured `LTN/LSTN` `0401` path.
- Re-read the device-specific considerations before assuming that a rejected command family is a
  bug. `LTN/LSTN/LCN` are not treated as monitor targets here because the manual-listed paths are
  `0401`, `0403`, and `1402` (`LCN` also `1401`), while `LZ` explicitly lists `0801`.
