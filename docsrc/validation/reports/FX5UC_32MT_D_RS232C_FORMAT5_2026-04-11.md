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
| supported-device screening | `21/25` targets passed under `--series ql`; `DX10`, `DY10`, `ZR10`, and `V100` failed with `0x7E43` |
| supported-device soak | one `180` second run passed with no protocol errors on the screened `21` target subset |
| native random read/write | `0403`, `1402` returned `0x7F23` |
| native monitor / multi-block | `0801` returned `0x7E40`; `0406` returned `0x7F23`; `1406` returned `0x7F40`, restore passed |
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

- run: `pass cycles=18 checks=374 elapsed_sec=180 verify_mismatch=234 restore_mismatch=0 total_commands=1871`
- protocol failures observed during the run: none

Interpretation:

- the no-wait, strictly serial contiguous soak is stable on `FX5UC-32MT/D` when limited to the
  screened `21` target subset
- bit-family `verify-mismatch` remained common and is consistent with RUN-mode overwrite

## Native Unresolved Command Recheck

Observed result:

- `0403 random-read`: PLC error `0x7F23`
- `1402 random-write-words`: PLC error `0x7F23`
- `1402 random-write-bits`: PLC error `0x7F23`
- post-read after both `1402` probes still showed no write effect at `D300..D305` and `M300..M305`
- baseline `D300..D305` on this target was `0x0000`, `0x0001`, `0x0002`, `0x0003`, `0x0004`, `0x0005`
- `0801 probe-monitor`: `probe-monitor: skip register 0x7E40`
- `0406 probe-multi-block`: `multi-block-read=skip 0x7F23`
- `1406 probe-multi-block`: `multi-block-write=skip 0x7F40`
- `probe-multi-block` contiguous restore: pass

Interpretation:

- the unresolved native random family stays unresolved on this FX target
- monitor and multi-block failures do not match the `LJ71C24` and `QJ71C24N` end-code mix exactly,
  so this target should stay documented separately

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
