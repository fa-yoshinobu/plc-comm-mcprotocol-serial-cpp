# Native Command Backlog

Audience: maintainers running real-hardware follow-up on unresolved native command families.

This page replaces the old repository-level TODO list for native command follow-up. Keep normal user
docs focused on known-good workflows and keep this page for unresolved native behavior.

## Public Policy

- Keep unresolved native command behavior visible. Do not add CLI fallback behavior that silently
  swaps in a different command family.
- Keep unresolved native commands in the public API and CLI, but describe them as native probes on
  the validated real-hardware setups instead of known-good workflows.
- Treat `read-qualified-words` / `write-qualified-words` over `0601/1601` as the practical public
  path for `U...\\G...` / `U...\\HG...` access on the current setup.
- Keep native qualified commands separate from helper qualified commands in docs, validation logs,
  and future probe notes.

## Active Backlog

Validated setup:

- `RJ71C24-R2 / RS-232C / 19200 / 8E1 / MC Protocol Format4 ASCII / CRLF / sum-check off / station 0`
- `LJ71C24 / RS-232C / 28800 / 8E2 / MC Protocol Format5 Binary / sum-check on / station 0 / L26CPU-BT / --series ql`

Unresolved native command families:

- `0403` random read: native ng, PLC end code `0x7F22`, unchanged after `2026-04-11` `--series iqr` recheck (`0403 0002`)
- `1402` random write words: native ng, PLC end code `0x7F22`, unchanged after `2026-04-11` `--series iqr` recheck (`1402 0002`)
- `1402` random write bits: native ng, PLC end code `0x7F23`
- `0406` multi-block read: native ng, PLC end code `0x7F22`, unchanged after `2026-04-11` `--series iqr` recheck (`0406 0002`)
- `1406` multi-block write: native ng, PLC end code `0x7F22`, unchanged after `2026-04-11` `--series iqr` recheck (`1406 0002`)
- `0801/0802` monitor register/read: native ng / hold, `0801` currently `0x7F22`, unchanged after `2026-04-11` `--series iqr` recheck (`0801 0002`)

Qualified-device follow-up:

- Helper path over `0601/1601`: usable, but current validated evidence is narrow
- `U3E0\\HG20` helper single-word read/write/restore: pass
- `U3E0\\G10` helper spot recheck on `2026-04-11`: `0x83BD`
- Native extended-device path over `0401/1401 + 0080/0082`: hold
- `read-native-qualified-words 'U3E0\\G10' 1 --series iqr`: mixed `0x7F22` / timeout across retries
- `read-native-qualified-words 'U3E0\\HG20' 1 --series iqr`: `0x7F22`
- `write-native-qualified-words 'U3E0\\HG20' 0x1234 --series iqr`: timeout
- `2026-04-11` Format5/Binary spot recheck: helper `U3E0\\G10=0x83BD`, helper `U3E0\\HG20=0x0000`, native `U3E0\\G10=0x0000`, native `U3E0\\HG20=0x4031`
- `2026-04-11` `LJ71C24 + L26CPU-BT + --series ql`: helper `U3E0\\G10=0x0000`, helper `U3E0\\HG20` read/write/restore passed, native `U3E0\\G10` read/write returned `0x4030`, native `HG` path was not applicable outside iQ-R
- `2026-04-11` Format5/Binary status-word recheck: helper-visible `U3E0\\G599`, `U3E0\\G600`, and `U3E0\\G31998..32003` stayed `0x0000` even after native `0x7F23` and `0x4031` probes
- No validated native qualified write effect yet

## Follow-up Rules

- Add every new hardware result to `docsrc/validation/reports/HARDWARE_VALIDATION.md`.
- Add or extend a dated report under `docsrc/validation/reports/` for raw evidence and command
  examples.
- Keep the top-level `README.md` summary short. Push detailed failure evidence into validation docs.
- Preserve request-shape conformance tests before treating hardware rejection as an encoder bug.
- Record the exact serial settings, PLC model, and native PLC end code for every new result.
- Run shared real-UART probes strictly serially. Parallel access on `/dev/ttyUSB0` produced mixed RX
  fragments during `2026-04-11` revalidation.
- Do not assume unresolved native failures are caused by default Q/L subcommands. `0403`, `1402`
  words, `0406`, `1406`, and `0801` were rechecked with `--series iqr` on `2026-04-11` and still
  failed with the same PLC end codes.
- Match `--series` to the actual CPU family before interpreting PLC end codes. On `2026-04-11`,
  `L26CPU-BT` with `--series iqr` caused even contiguous `D100` / `M100` reads to fail with
  `0x7F22`, while the same setup passed under `--series ql`.
- Do not treat a native qualified success code as proof of semantic correctness. On `2026-04-11`
  under `Format5 / Binary / 28800 / 8E2 / sum-check on`, native `U3E0\\G10` returned `0x0000`
  while the helper path still returned `0x83BD`.
- Do not assume the helper-visible C24 status words will capture these native failures. On
  `2026-04-11`, helper reads of `U3E0\\G599`, `U3E0\\G600`, and `U3E0\\G31998..32003` stayed
  zero after native `0x7F23` and `0x4031` probes.
- When C24 ASCII communication times out or returns mixed fragments, send ASCII `EOT CRLF` or
  `CL CRLF` before the next probe to reinitialize the transmission sequence. `EOT CRLF` is the
  default recovery path exposed by the CLI. Treat this as transport recovery only, not as evidence
  that the native command family works.
