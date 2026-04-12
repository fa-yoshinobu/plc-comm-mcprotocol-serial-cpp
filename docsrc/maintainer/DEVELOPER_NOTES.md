# Developer Notes

Audience: maintainers of this repository.

This repository now follows the same documentation split used in `plc-comm-slmp-cpp-minimal`.

## Documentation Layout

- `docsrc/user/`: user-facing bring-up and usage
- `docsrc/validation/reports/`: validation matrix and one consolidated report per hardware target
- `docsrc/maintainer/`: repository-maintenance notes

## Status Terms

Use these terms consistently in validation reports.

- `native pass`: the intended MC command succeeds directly on hardware
- `native ng`: the direct MC command is rejected by the module or PLC
- `hold`: not resolved yet, or intentionally excluded from the active probe set

## Current Native-only Policy

On the validated `RJ71C24-R2` setup, unsupported native commands should stay failed.
Do not add fallback behavior that silently replaces them with other command families.

## Qualified `G/HG` Policy

On the current validated setup, `read-qualified-words` and `write-qualified-words` are the
practical `U...\\G...` / `U...\\HG...` path because they reuse the validated `0601/1601`
module-buffer path.

Native qualified access is not a supported workflow in this repository. Keep
`read-native-qualified-words` and `write-native-qualified-words` separate as diagnostic probes only,
and do not describe them as a supported `U...` access path.

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

- Match `--series` to the actual CPU family before interpreting PLC end codes.
- Do not project the C24-era helper/buffer workflow onto FX targets without revalidation.
- Keep binary non-iQ-R bit and multi-block request layouts aligned with the current manual-backed
  codec tests. Count-width and bit-packing mistakes have been a recurring source of false hardware
  mismatches.

## Validation Reporting Rule

When adding new hardware results:

1. Update `docsrc/validation/reports/HARDWARE_VALIDATION.md`.
2. Add or extend the consolidated report for that hardware target in `docsrc/validation/reports/`.
3. Keep the top-level `README.md` summary short and point to the detailed report.
4. Record the native result and PLC end code without masking it with a different command path.

## Open Items

Track active unresolved items in [TODO.md](TODO.md).
