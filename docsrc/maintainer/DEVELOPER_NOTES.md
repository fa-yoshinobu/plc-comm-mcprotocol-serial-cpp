# Developer Notes

Audience: maintainers of this repository.

This repository now follows the same documentation split used in `plc-comm-slmp-cpp-minimal`.

## Documentation Layout

- `docsrc/user/`: user-facing bring-up and usage
- `docsrc/validation/reports/`: validation matrix and dated evidence logs
- `docsrc/maintainer/`: repository-maintenance notes

## Status Terms

Use these terms consistently in validation reports.

- `native pass`: the intended MC command succeeds directly on hardware
- `native ng`: the direct MC command is rejected by the module or PLC
- `hold`: not resolved yet, or intentionally excluded from the active probe set

## Current Native-only Policy

On the validated `RJ71C24-R2` setup, unsupported native commands should stay failed.
Do not add CLI fallback behavior that silently replaces them with other command families.

## Qualified `G/HG` Policy

On the current validated setup, `read-qualified-words` and `write-qualified-words` are the
practical `U...\\G...` / `U...\\HG...` path because they reuse the validated `0601/1601`
module-buffer path.

Keep `read-native-qualified-words` and `write-native-qualified-words` separate as validation probes
until native extended-device access is revalidated on hardware.

## C24 Recovery Discipline

The validated `RJ71C24-R2` setup behaves like a strict half-duplex shared UART.

- Do not overlap probes on `/dev/ttyUSB0`.
- Drain stale RX bytes before each hardware probe when prior traffic may have been interrupted.
- If ASCII Format4 communication times out or returns mixed fragments, send ASCII `EOT CRLF` or
  `CL CRLF` before the next probe. This matches the manual's transmission-sequence reset guidance
  for abnormal communication and both variants were revalidated on `2026-04-11` as transport
  recovery steps.
- Do not treat successful recovery after `EOT CRLF` or `CL CRLF` as evidence that an unresolved
  native command family is actually supported.

## Request-Shape Conformance

Host-side tests now pin or check these request families:

- `1402` random write words against the official MC protocol example
- `1402` random write bits against the official MC protocol example
  `Q/L` ASCII `1402 bit` uses `00/01` set-reset bytes, not single-character `0/1`
  `Q/L` binary `1402 bit` also uses a one-byte bit-access count in the page `108` example; the old
  local binary test had incorrectly inserted an extra zero byte before the first device reference
- `0406` multi-block read against the official MC protocol example
- `0801` register monitor against the documented `0403`-equivalent request layout
- `0802` read monitor against the documented command structure

This matters because the current hardware problem is not proven to be an encoder bug anymore. The request shapes are now locked down separately from the real-hardware outcome.

`2026-04-11` follow-up hardware rechecks also confirmed that `0403`, `1402` words, `0406`,
`1406`, and `0801` still fail after switching the CLI to `--series iqr`. Those commands did change
to iQ-R subcommands on the wire, while contiguous `0401/1401` traffic still passed with `0002/0003`.
Do not assume the remaining failures are caused by forgetting the PLC family selection.

`2026-04-11` FX5U follow-up also showed a second boundary: matching `--series ql` was necessary but
not sufficient to unlock buffer or qualified paths. `FX5UC-32MT/D` passed contiguous `D100` /
`M100` reads and a screened `21`-target contiguous soak under `--series ql`, but host-buffer,
module-buffer, helper-qualified, and native-qualified probes returned `0x7E40` or `0x7E43`. Monitor was also ng as a family:
`0801` register and raw `0802` read-only both returned `0x7E40`.
Do not project the C24-era helper/buffer workflow onto FX targets without revalidation.

The same date also produced a concrete binary multi-block fix. Captured binary `0406` traffic
showed request frames with one-byte `word block count` and one-byte `bit block count`.
The local implementation had encoded those two fields as little-endian words. After correcting the
binary `0406/1406` count width and adding codec tests, `FX5UC-32MT/D` `probe-multi-block`
changed from native rejection to `multi-block-read=ok native`, while `1406` advanced to
`verify-mismatch` instead of immediate PLC rejection.

The same FX5U follow-up then isolated the remaining `1406` mismatch to the second word of a
two-point bit block. A non-symmetric diagnostic pattern with expected logical word `0x1234`
read back as `0x1C84`, which is the same word with reversed two-bit-pair order. After changing the
binary `1406` bit-block encoder to pre-apply that pair-order reversal and adding a codec test for
the non-symmetric two-point case, `probe-multi-block[mixed]` reached
`multi-block-read=ok native`, `multi-block-write=ok native`, `restore=ok` on `FX5UC-32MT/D`.

`1402` random-write-bits then turned out to be the same kind of host-side compatibility problem,
but not the same exact field as `1406`. Its binary request shape was first corrected after a page
`108` manual re-read plus unrelated captured binary traffic showed a one-byte count field for non-iQ-R
binary `1402 bit`. After that encoder fix, the focused FX5U probe advanced from `0x7F23` to
success-end-code with readback mismatch for single-item `M100=1`, dense `M100..M115`, and sparse
`M100/M105/M110/M115` writes while the same `M100..M115` alternating pattern passed under
contiguous `1401`. A second FX5U recheck then showed the mismatch was pair-swapped bit-address
parity inside each two-point unit: logical `M198=1` wrote to `M199`, `M199=1` wrote to `M198`,
`M200=1` wrote to `M201`, and `M201=1` wrote to `M200`; `M201=0` on an all-ones baseline also
cleared `M200`, not `M201`. Flipping the encoded device-number low bit for non-iQ-R binary `1402`
bit resolved the focused FX5U probes, so `1402` random-write-bits is now in the resolved bucket on
that target.

Dedicated FX5U `probe-random-read` and `probe-random-write-words` runs then tightened that result:
the contiguous `D100..D105` and `M100..M105` baselines passed, but the native `0403` single/dense/
sparse probes and native `1402` word single/dense/sparse probes still all returned `0x7F23`.

## Validation Reporting Rule

When adding new hardware results:

1. Update `docsrc/validation/reports/HARDWARE_VALIDATION.md`.
2. Add or extend a dated report in `docsrc/validation/reports/`.
3. Keep the top-level `README.md` summary short and point to the detailed report.
4. Record the native result and PLC end code without masking it with a different command path.

## Open Items

Track active unresolved items in [NATIVE_COMMAND_BACKLOG.md](NATIVE_COMMAND_BACKLOG.md).
