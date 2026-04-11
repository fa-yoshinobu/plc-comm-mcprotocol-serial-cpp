# FX5UC-32MT/D RS-232C Format5 Q/L Report

Date: `2026-04-11`

Audience: maintainers validating the `FX5UC-32MT/D` path under the later `Format5 / Binary`
settings.

Current-status note:

- use [HARDWARE_VALIDATION.md](HARDWARE_VALIDATION.md) for the canonical current summary
- keep this page as the dated evidence log that explains how the current FX5U conclusions were reached

## Target Setup

- PLC CPU: Mitsubishi iQ-F `FX5UC-32MT/D`
- Link: `RS-232C`
- Host device: Linux PC with `/dev/ttyUSB0`
- Communication settings: `38400 bps / 8E2 / station 0`
- MC protocol settings: `Format 5 / Binary / sum-check on`
- CLI family selection: use `--series ql`

## Series Selection Check

Observed result:

- `cpu-model`: `FX5UC-32MT/D`, `0x4A91`
- `--series ql read-words D100 1`: pass
- `--series ql read-bits M100 1`: pass
- `--series iqr read-words D100 1`: PLC error `0x7E40`
- `--series iqr read-bits M100 1`: PLC error `0x7E40`

Interpretation:

- the practical CLI family selection is `--series ql`
- `--series iqr` is not usable here even for contiguous `D100` / `M100` reads

## Summary

| Category | Status |
|---|---|
| link bootstrap | `cpu-model` passed with `FX5UC-32MT/D`, `0x4A91` |
| supported-device screening | validated `21`-target subset passed under `--series ql`; `DX10` and `DY10` returned `0x7E43`, while `ZR10` and `V100` are now treated as not applicable on this FX target |
| supported-device soak | two `180` second runs passed with no protocol errors on the screened `21` target subset |
| native random read/write | `0403`, `1402 words`, and focused `1402 bit` `M` probes now pass natively on this target after the non-iQ-R binary count-width fixes; `1402 bit` additionally needed pair-swapped bit-address correction |
| native monitor / multi-block | `0406/1406` passed after the binary fixes; `0801/0802` stayed `0x7E40` and are now treated as unsupported on FX5U serial `3C/4C` |
| host / module buffer | helper and native buffer probes returned `0x7E40` |
| qualified access | helper `U3E0\\G10` / `U3E0\\HG20` returned `0x7E40`; native `U3E0\\G10` returned `0x7E43`; native `HG` path is not applicable under `--series ql` |

## Supported-device Screening

Observed result:

- validated `21`-target subset:
  `STS10`, `STC10`, `STN10`, `TS10`, `TC10`, `TN10`, `CS10`, `CC10`, `CN10`, `SB10`, `SW10`,
  `X10`, `Y10`, `M100`, `L100`, `F100`, `B10`, `D100`, `W10`, `Z10`, `R100`
- exploratory probes outside that subset:
  `DX10`, `DY10`, `ZR10`, `V100`
- passing non-low-address targets:
  `STS10`, `STC10`, `STN10`, `TS10`, `TC10`, `TN10`, `CS10`, `CC10`, `CN10`, `SB10`, `SW10`,
  `X10`, `Y10`, `M100`, `L100`, `F100`, `B10`, `D100`, `W10`, `Z10`, `R100`
- `DX10` and `DY10` both returned PLC error `0x7E43`
- the historical `ZR10` and `V100` exploratory probes also returned `0x7E43`, but the FX5
  communication manual's `3C/4C frame` accessible-device table marks `V` and `ZR` as inaccessible,
  so they are treated as not applicable on `FX5UC-32MT/D`

Interpretation:

- contiguous access is usable on this target, but the supported device subset is narrower than the
  `RJ71C24-R2`, `LJ71C24`, and `QJ71C24N` sets
- keep `ZR` and `V` out of the FX5U contiguous subset; the FX5 communication manual marks them as
  inaccessible on serial `3C/4C`
- `DX` and `DY` also stay out of the validated subset; the same FX manual marks them inaccessible
  on serial `3C/4C`, which matches the observed `0x7E43`
- the FX5U soak should stay on the screened `21` target subset unless more device families are
  revalidated

## Supported-device Soak

Command:

```bash
./examples/linux_cli/fx5u_supported_device_rw_soak.sh
```

Observed result:

- first run: `pass cycles=18 checks=374 elapsed_sec=180 verify_mismatch=234 restore_mismatch=0 total_commands=1871`
- second run: `pass cycles=18 checks=374 elapsed_sec=180`
- protocol failures observed during either run: none

Interpretation:

- the no-wait, strictly serial contiguous soak is stable on `FX5UC-32MT/D` when limited to the
  screened `21` target subset
- bit-family `verify-mismatch` remained common and is consistent with RUN-mode overwrite
- the screened subset result is reproducible across at least two `180` second runs

## Capture-driven Multi-block Recheck

Observed result:

- captured binary `0406` traffic showed requests with one-byte `word block count` and
  one-byte `bit block count`
- the repository previously encoded those binary count fields as little-endian words
- after switching binary `0406/1406` to one-byte block counts and adding codec tests, FX5U
  `probe-multi-block` changed to:
  - `multi-block-read=ok native`
  - `multi-block-write=skip verify-mismatch`
  - `probe-multi-block: read=native write=skip restore=ok`
- a follow-up diagnostic split `1406` into `word-only`, `bit-only`, and single-block modes:
  - `probe-multi-block[word-only]: read=native write=native restore=ok`
  - `probe-multi-block[bit-b]` isolated the remaining mismatch to the second word of a two-point bit
    block
  - a non-symmetric `bit-b` pattern with expected second word `0x1234` read back as `0x1C84`
    before the fix, proving the binary bit-word payload needed reversed two-bit-pair order
- after correcting that binary `1406` bit-block packing and pinning it in codec tests, FX5U
  `probe-multi-block` changed to:
  - `multi-block-read=ok native`
  - `multi-block-write=ok native`
  - `probe-multi-block[mixed]: read=native write=native restore=ok`

Interpretation:

- the old binary multi-block count-field width was a host-side bug
- the capture and local manual re-read both support the one-byte count layout for binary
- on FX5U, `0406` is no longer part of the unresolved native set after this fix
- on FX5U, `1406` is also no longer part of the unresolved native set after correcting the binary
  bit-block payload from natural bit order to reversed two-bit-pair order within each 16-bit unit

## Native Random and Monitor Recheck

Observed result:

- `0403 random-read`: originally `0x7F23`
- focused `0403` recheck before the count-width fix on the same target:
  - `random-read D100`: `0x7F23`
  - `random-read M100`: `0x7F23`
  - `random-read D100 D105`: `0x7F23`
  - `random-read M100 M105`: `0x7F23`
- dedicated `probe-random-read` recheck before the fix on the same target:
  - contiguous `read-words D100 6`: pass
  - contiguous `read-bits M100 6`: pass
  - native `random-read` single, dense, and sparse word probes: all `0x7F23`
  - native `random-read` single, dense, and sparse bit probes: all `0x7F23`
- `1402 random-write-words`: originally `0x7F23`
- focused `1402` word recheck before the count-width fix on the same target:
  - `random-write-words D100=1`: `0x7F23`
  - `random-write-words D100=1 D101=2`: `0x7F23`
  - `random-write-words D100=1 D105=2`: `0x7F23`
- dedicated `probe-random-write-words` recheck before the fix on the same target:
  - contiguous `write-words D100..D105`: pass
  - native `random-write-words` single, dense, and sparse probes: all `0x7F23`
  - restore back to the original `D100..D105` values: pass
- after switching non-iQ-R binary `0403` and `1402` word/dword counts from two-byte fields to the
  one-byte Q/L-era layout shown by the binary manual examples, the same FX5U probes returned:
  - `probe-random-read: word-single=native word-dense=native word-sparse=native bit-single=native bit-dense=native bit-sparse=native`
  - `probe-random-write-words: contiguous=contiguous single=native dense=native sparse=native restore=ok`
- `1402 random-write-bits`: originally `0x7F23`; after the binary one-byte count fix from page
  `108` plus unrelated captured binary traffic, native writes reached success-end-code but still read back as
  `verify-mismatch`; a second FX5U-focused recheck then showed the remaining issue was pair-swapped
  bit-address parity inside each two-point unit
- post-read after both `1402` probes still showed no write effect at `D300..D305` and `M300..M305`
- baseline `D300..D305` on this target was `0x0000`, `0x0001`, `0x0002`, `0x0003`, `0x0004`, `0x0005`
- focused `probe-random-write-bits` recheck on the same target:
  - contiguous `write-bits` to `M100..M115` with alternating `1010...` pattern: pass
  - before the count-width fix, native `random-write-bits` single item `M100=1`: `0x7F23`
  - before the count-width fix, native `random-write-bits` on the same `M100..M115` pattern:
    `0x7F23`
  - before the count-width fix, native `random-write-bits` sparse subset `M100`, `M105`, `M110`,
    `M115`: `0x7F23`
  - after switching non-iQ-R binary `1402 bit` to a one-byte access-count field, the same probe
    returned success-end-code with `verify-mismatch`
  - direct FX5U parity rechecks then showed the mismatch was not a generic `+1` shift but a strict
    two-point swap:
    - logical `M198=1` wrote to `M199`
    - logical `M199=1` wrote to `M198`
    - logical `M200=1` wrote to `M201`
    - logical `M201=1` wrote to `M200`
    - `M201=0` on an all-ones baseline cleared `M200`, not `M201`
  - after correcting non-iQ-R binary `1402 bit` to flip the device-number low bit within each
    two-point unit, the same probe returned:
    - `batch-write-bits=ok contiguous`
    - `random-write-bits-single=ok native`
    - `random-write-bits-dense=ok native`
    - `random-write-bits-sparse=ok native`
    - `restore=ok`
  - restore back to the original `M100..M115` values: pass
- `0801 probe-monitor`: `probe-monitor: skip register 0x7E40`
- raw `0802` only via `probe-monitor read-only`: `probe-monitor[read-only]: skip 0x7E40`
- `0406 probe-multi-block`: moved to the capture-driven recheck section after the binary count fix
- `1406 probe-multi-block`: moved to the capture-driven recheck section after the binary count fix

Interpretation:

- the old non-iQ-R binary `0403` and `1402` word/dword count widths were host-side compatibility
  bugs; on FX5U, switching those counts from two-byte fields to the one-byte Q/L-era layout made
  both `probe-random-read` and `probe-random-write-words` pass natively
- the old binary `1402 bit` count width was a host-side compatibility bug for Q/L-era binary
  frames; the manual example on page `108` and unrelated captured binary traffic both use a one-byte
  bit-access count
- after correcting that count field, FX5U no longer rejected native `1402 bit` immediately; the
  remaining mismatch was pair-swapped bit-address parity inside each two-point unit
- after correcting that pair swap in the local encoder, native `1402 bit` passed on FX5U for
  single-item, dense, and sparse probes with restore
- FX5U therefore no longer has unresolved native random-read or random-write families
- the FX5 communication manual's serial `3C/4C` command list includes `0403`, `1402`, `0406`, and
  `1406`, but does not list `0801/0802`; treat the repeated `0x7E40` monitor result as
  target-level unsupported behavior rather than an unresolved host-side encoder bug

## Buffer and Qualified Access

Observed result:

- `probe-host-buffer`: `skip 0x7E40`
- `probe-write-host-buffer`: backup failed with `0x7E40`
- `probe-module-buffer`: `skip 0x7E40`
- `probe-write-module-buffer`: backup failed with `0x7E40`
- helper `read-qualified-words 'U3E0\G10' 1`: PLC error `0x7E40`
- helper `read-qualified-words 'U3E0\HG20' 1`: PLC error `0x7E40`
- native `read-native-qualified-words 'U3E0\G10' 1`: PLC error `0x7E43`
- native `write-native-qualified-words 'U3E0\G10' 0x1234`: PLC error `0x7E43`
- native `U3E0\HG20` read/write under `--series ql`: rejected before TX because `HG` requires iQ-R

Interpretation:

- on this target, the helper `U...` path is not a usable substitute for native qualified access
- buffer and qualified probes should stay treated as not applicable or unresolved on FX5U until a
  dedicated implementation path is validated

## FX Manual Re-read

Observed result from the FX5 communication manual:

- FX5 serial ports support MC protocol `1C` and QnA-compatible `3C/4C` frames
- the `3C/4C` command list explicitly includes:
  - `0403`
  - `1402`
  - `0406`
  - `1406`
- the same `3C/4C` command list does not list `0801/0802`
- the `3C/4C` accessible-device table marks `DX`, `DY`, `V`, and `ZR` as inaccessible on this
  target class

Interpretation:

- `0403`, `1402`, `0406`, and `1406` are now aligned with both the manual and the corrected local
  encoder
- `0801/0802` should no longer be described as a generic unresolved native family on FX5U; they
  are unsupported for this serial `3C/4C` target unless contrary hardware evidence appears
- `DX`, `DY`, `V`, and `ZR` should be treated as not applicable on this FX target rather than as
  candidates for additional contiguous soak screening
