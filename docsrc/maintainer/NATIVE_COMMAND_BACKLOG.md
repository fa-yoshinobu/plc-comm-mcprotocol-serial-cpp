# Native Command Backlog

Audience: maintainers running real-hardware follow-up on unresolved native command families.

This page replaces the old repository-level TODO list for native command follow-up. Keep normal user
docs focused on known-good workflows and keep this page for unresolved native behavior.

## Public Policy

- Keep unresolved native command behavior visible. Do not add fallback behavior that silently
  swaps in a different command family.
- Keep unresolved native commands in the public API, but describe them as native probes on the
  validated real-hardware setups instead of known-good workflows.
- Treat `read-qualified-words` / `write-qualified-words` over `0601/1601` as the practical public
  path for `U...\\G...` / `U...\\HG...` access on the current setup.
- Treat native qualified commands as unsupported diagnostic probes, not as a supported `U...`
  access path.

## Active Backlog
Target-specific remaining items:

- `RJ71C24-R2 + R08CPU / Format5 Binary / --series iqr` still rejects native `LZ` double-word
  random-read probes with `0x7F23`; local support now treats `LZ` as an iQ-R-only double-word
  device and rejects Q/L-mode requests before transmit
- `RJ71C24-R2 + iQ-R CPU / Format5 Binary / --series iqr` currently rejects native `0403`
  random-read probes for `LTN`, `LSTN`, and `LCN` with `0x7F23`, even though structured/batch
  reads now work (`read-words LTN10 4`, `read-words LSTN10 4`, `read-words LCN10 2`)
- `RJ71C24-R2 + iQ-R CPU / Format5 Binary / --series iqr` currently rejects native `1402`
  random-write-words for `LTN`, `LSTN`, and `LCN` with `0x7F23`; `LCN` does work through `1401`
  batch write (`write-words LCN10=<low> LCN11=<high>` readback/restore passed)
- `FX5UC-32MT/D` still holds on host/module buffer access

### Implementation Gaps

Device/register surfaces still missing from this serial C++ library:

- `Jn\\...` random / multi-block / monitor surfaces

## Follow-up Rules

- Add every new hardware result to `docsrc/validation/reports/HARDWARE_VALIDATION.md`.
- Add or extend the consolidated report under `docsrc/validation/reports/` for that hardware
  target, including raw evidence and command examples.
- Keep the top-level `README.md` summary short. Push detailed failure evidence into validation docs.
- Preserve request-shape conformance tests before treating hardware rejection as an encoder bug.
- Record the exact serial settings, PLC model, and native PLC end code for every new result.
- Run shared real-UART probes strictly serially. Parallel access on `/dev/ttyUSB0` produced mixed RX
  fragments during `2026-04-11` revalidation.
- Match `--series` to the actual CPU family before interpreting PLC end codes.
- Keep FX5U notes aligned with its serial manual: `0801/0802` unsupported, `DX/DY/V/ZR` outside the validated subset.
