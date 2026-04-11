# FX5UC-32MT/D RS-232C Format5 Q/L Report

Date: `2026-04-11`

Audience: maintainers validating the `FX5UC-32MT/D` path under the later `Format5 / Binary`
settings.

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
| supported-device screening | `21/25` targets passed under `--series ql`; `DX10`, `DY10`, `ZR10`, and `V100` failed with `0x7E43` twice |
| supported-device soak | two `180` second runs passed with no protocol errors on the screened `21` target subset |
| native random read/write | `0403`, `1402` returned `0x7F23` |
| native monitor / multi-block | `0801` returned `0x7E40`; after the binary count-width and bit-block packing fixes, both `0406` and `1406` passed |
| host / module buffer | helper and native buffer probes returned `0x7E40` |
| qualified access | helper `U3E0\\G10` / `U3E0\\HG20` returned `0x7E40`; native `U3E0\\G10` returned `0x7E43`; native `HG` path is not applicable under `--series ql` |

## Supported-device Screening

Observed result:

- `ok=21 fail=4`
- passing non-low-address targets:
  `STS10`, `STC10`, `STN10`, `TS10`, `TC10`, `TN10`, `CS10`, `CC10`, `CN10`, `SB10`, `SW10`,
  `X10`, `Y10`, `M100`, `L100`, `F100`, `B10`, `D100`, `W10`, `Z10`, `R100`
- failing targets:
  `DX10`, `DY10`, `ZR10`, `V100`
- failing targets all returned PLC error `0x7E43`
- read-only recheck on the same date:
  `read-bits DX10 1`, `read-bits DY10 1`, `read-words ZR10 1`, and `read-bits V100 1` all again
  returned `0x7E43`

Interpretation:

- contiguous access is usable on this target, but the supported device subset is narrower than the
  `RJ71C24-R2`, `LJ71C24`, and `QJ71C24N` sets
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

- TAK capture in `cap/write.txt` showed `0406` binary requests with one-byte `word block count` and
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

## Native Unresolved Command Recheck

Observed result:

- `0403 random-read`: PLC error `0x7F23`
- focused `0403` recheck on the same target:
  - `random-read D100`: `0x7F23`
  - `random-read M100`: `0x7F23`
  - `random-read D100 D105`: `0x7F23`
  - `random-read M100 M105`: `0x7F23`
- `1402 random-write-words`: PLC error `0x7F23`
- focused `1402` word recheck on the same target:
  - `random-write-words D100=1`: `0x7F23`
  - `random-write-words D100=1 D101=2`: `0x7F23`
  - `random-write-words D100=1 D105=2`: `0x7F23`
- `1402 random-write-bits`: PLC error `0x7F23`
- post-read after both `1402` probes still showed no write effect at `D300..D305` and `M300..M305`
- baseline `D300..D305` on this target was `0x0000`, `0x0001`, `0x0002`, `0x0003`, `0x0004`, `0x0005`
- focused `probe-random-write-bits` recheck on the same target:
  - contiguous `write-bits` to `M100..M115` with alternating `1010...` pattern: pass
  - native `random-write-bits` single item `M100=1`: `0x7F23`
  - native `random-write-bits` on the same `M100..M115` pattern: `0x7F23`
  - native `random-write-bits` sparse subset `M100`, `M105`, `M110`, `M115`: `0x7F23`
  - restore back to the original `M100..M115` values: pass
- `0801 probe-monitor`: `probe-monitor: skip register 0x7E40`
- `0406 probe-multi-block`: moved to the capture-driven recheck section after the binary count fix
- `1406 probe-multi-block`: moved to the capture-driven recheck section after the binary count fix

Interpretation:

- the unresolved native random family stays unresolved on this FX target
- `0403` is not just a mixed `D+M` problem on FX5U; both word-only and bit-only random reads still
  return `0x7F23`, and the same remains true even for single-item `D100` / `M100`
- `1402` word-write is not just a sparse random case on FX5U; single-item, dense adjacent, and
  sparse `D100` writes all still return `0x7F23`
- the `1402` bit-write failure is not explained by low addresses, RUN overwrite, or the concrete
  `M100..M115` pattern itself because the same target bits passed under contiguous `1401`; the
  failure also persists for a single-item `M100=1` random write
- monitor failures still do not match the `LJ71C24` and `QJ71C24N` end-code mix exactly, so this
  target should stay documented separately

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
