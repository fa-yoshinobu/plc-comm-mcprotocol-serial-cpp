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
- `emulated pass`: the CLI succeeds by falling back to repeated native-safe operations
- `native ng`: the direct MC command is rejected by the module or PLC
- `hold`: not resolved yet, or intentionally excluded from the active probe set

## Current Fallback Policy

On the validated `RJ71C24-R2` setup:

- `random-read` falls back to repeated batch reads
- `random-write-words` falls back to repeated batch word writes
- `random-write-bits` falls back to repeated batch bit writes
- `probe-multi-block` falls back to repeated block-wise batch read/write
- `probe-monitor` falls back to repeated direct reads

The CLI must keep printing whether the observed path is `native` or `emulated`.

## Request-Shape Conformance

Host-side tests now pin or check these request families:

- `1402` random write words against the official MC protocol example
- `1402` random write bits against the official MC protocol example
- `0406` multi-block read against the official MC protocol example
- `0801` register monitor against the documented `0403`-equivalent request layout
- `0802` read monitor against the documented command structure

This matters because the current hardware problem is not proven to be an encoder bug anymore. The request shapes are now locked down separately from the real-hardware outcome.

## Validation Reporting Rule

When adding new hardware results:

1. Update `docsrc/validation/reports/HARDWARE_VALIDATION.md`.
2. Add or extend a dated report in `docsrc/validation/reports/`.
3. Keep the top-level `README.md` summary short and point to the detailed report.
4. If a command only works through fallback, record both:
   - native result and PLC end code
   - emulated result and verification path
