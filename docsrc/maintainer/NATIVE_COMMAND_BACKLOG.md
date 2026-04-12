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

Validated setup:

- `RJ71C24-R2 / RS-232C / 19200 / 8E1 / MC Protocol Format4 ASCII / CRLF / sum-check off / station 0`
- `RJ71C24-R2 / RS-232C / 28800 / 8E2 / MC Protocol Format5 Binary / sum-check on / station 0 / R08CPU / --series ql`
- `LJ71C24 / RS-232C / 28800 / 8E2 / MC Protocol Format5 Binary / sum-check on / station 0 / L26CPU-BT / --series ql`
- `QJ71C24N / RS-232C / 19200 / 8E2 / MC Protocol Format5 Binary / sum-check on / station 0 / Q06UDVCPU / --series ql`
- `FX5UC-32MT/D / RS-232C / 38400 / 8E2 / MC Protocol Format5 Binary / sum-check on / station 0 / --series ql`

### Remaining Snapshot

Shared native-family holds across the currently validated targets:

- none at the family level

Target-specific remaining items:

- `RJ71C24-R2 + R08CPU / Format5 Binary / --series iqr` still rejects native `LZ` double-word
  random-read probes with `0x7F23`; local support now treats `LZ` as an iQ-R-only double-word
  device and rejects Q/L-mode requests before transmit
- `FX5UC-32MT/D` still holds on host/module buffer access

### Implementation Gaps

Device/register surfaces still missing from this serial C++ library:

- `LTN` long timer current value
- `LSTN` long retentive timer current value
- `LCN` long counter current value

Manual re-read notes:

- `RD` is now implemented as a direct word device (`RD**`, binary `002CH`).
- `2026-04-12` spot rechecks on `RJ71C24-R2 + R120PCPU / Format5 Binary / --series iqr` showed
  `read-bits SM0 4`, `read-words SD0 2`, and `read-words RD0 2` all passing.
- `LTN`, `LSTN`, and `LCN` are actual current-value devices, not just helper aliases.
- `LTN` / `LSTN` / `LCN` are not ordinary one-word devices; the manual calls out special access
  rules for long timer / long retentive timer / long counter current values.
- For `LTN` / `LSTN` / `LCN`, implementation may require special point accounting and result
  interpretation on top of normal `0401` / `0403` / `1401` / `1402`, not just a bare `DeviceCode`
  addition.

Resolved enough to remove from the active hold list:

- `FX5UC-32MT/D` native `0403` moved to pass after switching non-iQ-R binary word/dword counts
  from two-byte fields to the one-byte Q/L-era layout
- `FX5UC-32MT/D` native `1402` random-write-words moved to pass after the same non-iQ-R binary
  one-byte word/dword count fix
- `RJ71C24-R2 + R08CPU` native `0403`, `1402` words, and `1402` bits moved to pass on `2026-04-11`
  when the same corrected encoder was rechecked under `--series ql`
- `LJ71C24 + L26CPU-BT` native `0403`, `1402` words, `1402` bits, `0406/1406`, and `0801/0802`
  moved to pass on `2026-04-11` when the same corrected encoder was rechecked under `--series ql`
- `QJ71C24N + Q06UDVCPU` native `0403`, `1402` words, `1402` bits, `0406/1406`, and `0801/0802`
  moved to pass on `2026-04-11` when the same corrected encoder was rechecked under `--series ql`
- `FX5UC-32MT/D` native `0801/0802` should be treated as unsupported on serial `3C/4C`, not as an
  active unresolved encoder hold; the FX5 communication manual's command list includes `0403`,
  `1402`, `0406`, and `1406`, but does not list `0801/0802`
- `FX5UC-32MT/D` native `0406` moved to pass after the binary one-byte block-count fix
- `FX5UC-32MT/D` native `1406` moved to pass after the binary one-byte block-count fix plus bit-block two-bit-pair reversal
- `RJ71C24-R2 + R08CPU + --series iqr` native `0406/1406` moved to pass on `2026-04-11` after the same corrected binary encoder was rechecked on hardware
- `RJ71C24-R2 + R08CPU` native `0801/0802` moved to pass on `2026-04-11` under `--series ql`
- `FX5UC-32MT/D` focused native `1402` random-write-bits `M` probes moved to pass after the binary
  one-byte count fix plus the non-iQ-R pair-swapped bit-address correction

### Next Actions

If only `FX5UC-32MT/D` is available:

- treat `0406/1406` as resolved on that target unless a regression appears
- treat `0403` and `1402` as resolved on that target unless a regression appears
- do not spend more probe time on FX5U host/module buffer access until a concrete shape change is identified

If `RJ71C24-R2`, `LJ71C24`, or `QJ71C24N` become available again:

- re-run native `0406/1406` with the corrected binary encoder
- `2026-04-11` captured binary `0406` traffic matched the MC manual's binary `0406` layout with
  one-byte `word block count` and one-byte `bit block count`; the repository had been encoding
  those fields as two-byte little-endian words

## Follow-up Rules

- Add every new hardware result to `docsrc/validation/reports/HARDWARE_VALIDATION.md`.
- Add or extend the consolidated report under `docsrc/validation/reports/` for that hardware
  target, including raw evidence and command examples.
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
  `0x7F22`, while the same setup passed under `--series ql`. On the same date,
  `FX5UC-32MT/D` with `--series iqr` caused contiguous `D100` / `M100` reads to fail with
  `0x7E40`, while the same setup passed under `--series ql`.
- Do not assume the Q/L-era supported contiguous device subset applies unchanged to FX targets. On
  `2026-04-11`, `DX10` and `DY10` returned `0x7E43` on `FX5UC-32MT/D` even though the screened
  `21`-target subset completed a `180` second contiguous soak without protocol errors. `ZR` and
  `V` should now be treated as not applicable on this FX target rather than supported-subset
  failures.
- Treat binary `0406/1406` count-field width as a proven host-side compatibility point. On
  `2026-04-11`, the local implementation was corrected from two-byte block counts to one-byte block
  counts based on captured binary traffic plus manual re-read; `FX5UC-32MT/D` `0406` then passed natively.
- Treat binary `1406` bit-block packing as a second proven host-side compatibility point on FX5U.
  On `2026-04-11`, diagnostic `bit-b` rechecks showed expected logical word `0x1234` reading back
  as `0x1C84`, which is the same word with reversed two-bit-pair order. After correcting the local
  binary encoder to pre-apply that pair-order reversal, `FX5UC-32MT/D` `probe-multi-block[mixed]`
  passed natively.
- Treat non-iQ-R binary `1402` random-write-bits as two separate host-side compatibility points.
  On `2026-04-11`, a page `108` manual re-read plus unrelated captured binary traffic showed a one-byte
  count field for Q/L-era binary `1402 bit`, moving `FX5UC-32MT/D` from `0x7F23` to
  success-end-code with readback mismatch. The remaining mismatch then narrowed to pair-swapped
  bit-address parity inside each two-point unit (`M198↔M199`, `M200↔M201`), and flipping the
  encoded device-number low bit made focused FX5U single/dense/sparse probes pass natively.
- Treat non-iQ-R binary `0403` and `1402` random-write-words as a third host-side compatibility
  point. On `2026-04-11`, the Q/L-era binary manual examples showed one-byte word-count and
  double-word-count fields instead of the repository's old two-byte layout. Switching those counts
  to one-byte fields made `FX5UC-32MT/D` `probe-random-read` and `probe-random-write-words` pass
  natively.
- Treat native `0801/0802` monitor as unsupported on FX5U serial `3C/4C` unless contrary evidence
  appears. The FX5 communication manual's `3C/4C` command list includes `0403`, `1402`, `0406`,
  and `1406`, but does not list `0801/0802`; `2026-04-11` hardware rechecks returned `0x7E40` on
  both `0801` and raw `0802`.
- When C24 ASCII communication times out or returns mixed fragments, send ASCII `EOT CRLF` or
  `CL CRLF` before the next probe to reinitialize the transmission sequence. Treat this as
  transport recovery only, not as evidence that the native command family works.
