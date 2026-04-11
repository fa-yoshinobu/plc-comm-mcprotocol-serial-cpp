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
- `0406` multi-block read against the official MC protocol example
- `0801` register monitor against the documented `0403`-equivalent request layout
- `0802` read monitor against the documented command structure

This matters because the current hardware problem is not proven to be an encoder bug anymore. The request shapes are now locked down separately from the real-hardware outcome.

`2026-04-11` follow-up hardware rechecks also confirmed that `0403`, `1402` words, `0406`,
`1406`, and `0801` still fail after switching the CLI to `--series iqr`. Those commands did change
to iQ-R subcommands on the wire, while contiguous `0401/1401` traffic still passed with `0002/0003`.
Do not assume the remaining failures are caused by forgetting the PLC family selection.

## Validation Reporting Rule

When adding new hardware results:

1. Update `docsrc/validation/reports/HARDWARE_VALIDATION.md`.
2. Add or extend a dated report in `docsrc/validation/reports/`.
3. Keep the top-level `README.md` summary short and point to the detailed report.
4. Record the native result and PLC end code without masking it with a different command path.

## Open Items

Track active unresolved items in [NATIVE_COMMAND_BACKLOG.md](NATIVE_COMMAND_BACKLOG.md).
