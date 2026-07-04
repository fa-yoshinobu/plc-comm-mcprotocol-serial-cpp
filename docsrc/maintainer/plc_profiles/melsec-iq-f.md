# `melsec:iq-f` Profile Specification

Status: target-observed maintainer specification for the tested FX5 serial
setups. This file is the current implementation basis, not a user-facing
support contract.

This file records the current `melsec:iq-f` profile decision for MELSEC iQ-F /
FX5 serial MC Protocol access. Validation addresses are test points only; do
not infer device-number ranges from them.

## Target family

MELSEC iQ-F / FX5 CPU modules accessed through FX5 serial communication paths.

## Decision summary

| Area | Decision |
| --- | --- |
| Public profile string | Use `melsec:iq-f` for FX5/iQ-F serial targets. |
| Internal series branch | Keep `PlcSeries::IQ_F` separate from Q/L, iQ-L, and iQ-R. |
| Confirmed frame/code mode | C4 binary / Format5 is the current confirmed support basis. |
| Normal devices | The plain device families listed below are supported through normal device access. |
| Special devices | `Un\G` is supported only through the native-qualified route. `Un\HG` and `Jn\...` are not part of this profile. |

## Evidence

| Source | Finding |
| --- | --- |
| `SH-082624-J - MELSEC iQ-F FX5 User's Manual (Communication)`, physical PDF pages 695-696 / manual pages 693-694 | Lists 3C/4C device codes and FX5 device presence. |
| `FX5UC-32MT/D`, CPU model code `0x4A91` | Initial read-only inventory through C4 binary Format5. |
| `FX5U-32MR/DS`, CPU model code `0x4A41` | Representative reads, focused write/restore checks, native random checks, multi-block checks, and native-qualified `Un\G` read/write. |
| `FX5` target, CLI `c4-ascii-f1`, `COM4`, `19200bps`, `8E1`, sum-check off | Error-code investigation only: `D100` write/read sanity passed, raw invalid command/subcommand and `0802` returned `0x7E40`, and raw `DX0` read/write returned `0x7F21`. See [MC serial error-code evidence](../MC_SERIAL_ERROR_CODE_EVIDENCE.md). |
| Same FX5 target, protocol format 1, sum-check enabled | Correct sum-check reads passed on both `c1-ascii-f1` and `c4-ascii-f1`; deliberately bad sum-check returned `c1` NAK `0x02` and `c4` `0x7F24`; sum-check-enabled writes also passed on both `c1` and `c4` with cross-readbacks matching. See [MC serial error-code evidence](../MC_SERIAL_ERROR_CODE_EVIDENCE.md). |
| `FX5` target, CLI `c1-ascii-f1` with `melsec:a` compatibility profile, `COM4`, `19200bps`, `8E1`, sum-check off | Error-code investigation only: `D100` read succeeded and representative 1C NAK codes `0x03`, `0x06`, and `0x07` were observed. See [MC serial error-code evidence](../MC_SERIAL_ERROR_CODE_EVIDENCE.md). |
| `FX5` target switched to protocol format 4, `COM4`, `19200bps`, `8E1`, sum-check off | `c1-ascii-f4` / `melsec:a` and `c4-ascii-f4` / `melsec:iq-f` both read `D100` successfully; `c1-ascii-f4` wrote `D100=0x3C5A` and `c4-ascii-f4` wrote `D100=0xA53C`, with cross-readbacks matching. Representative Format4 abnormal probes matched the Format1 observations: 1C NAK `0x03`/`0x06`/`0x07`, C4 `0x7E40`/`0x7F21`. Station/PC mismatch probes produced `c1` NAK `0x10` for PC mismatch and no-response cases for the other tested mismatches. See [MC serial error-code evidence](../MC_SERIAL_ERROR_CODE_EVIDENCE.md). |
| Same FX5 target, protocol format 4, sum-check enabled | Correct sum-check reads passed on both `c1-ascii-f4` and `c4-ascii-f4`; deliberately bad sum-check returned `c1` NAK `0x02` and `c4` `0x7F24`. See [MC serial error-code evidence](../MC_SERIAL_ERROR_CODE_EVIDENCE.md). |

The primary support-validation settings were `COM4`, `19200`, `8E1`, station
`0`, sum-check off, C4 binary Format5. These settings describe the validation
setup only; the `c4-ascii-f1` row above is error-code investigation evidence,
not a support-basis change.

## Confirmed request shape

| Item | iQ-F serial MC behavior |
| --- | --- |
| Normal word subcommand | `0000` |
| Normal bit subcommand | `0001` |
| Extended word subcommand for `Un\G` | `0080` |
| Device reference width | Q/L-compatible normal device form for C4 binary Format5. |
| Native random double-word route | Required for `LCN` and `LZ`. |

ASCII frames, 1C, and 1E are outside the current iQ-F support decision.

## Confirmed support devices

These are confirmed for `melsec:iq-f` on the tested FX5 serial targets.

| Support class | Device families |
| --- | --- |
| Plain bit read/write | `X`, `Y`, `M`, `L`, `SM`, `F`, `B`, `TS`, `TC`, `STS`, `STC`, `CS`, `CC`, `SB` |
| Plain word read/write | `D`, `SD`, `W`, `TN`, `STN`, `CN`, `SW`, `Z`, `R` |
| Long counter state read/write | `LCS`, `LCC`; reads use the long-state read route. |
| Native random double-word read/write | `LCN`, `LZ` |
| Native-qualified read/write | `Un\G` |

## Dedicated route details

The following families require route-specific handling. Do not silently fall
back to ordinary plain-device access when one of these routes is requested.

| Route | Confirmed behavior |
| --- | --- |
| `Un\G` native-qualified access | Use native device access (`0401/1401`) with extended subcommand `0080`. |
| `Un\G` helper access | Reject the `0601/1601` helper route for iQ-F. The tested target returned `0x7E40` for module-buffer probes, and public access should remain native-qualified. |
| `Un\HG` | Not part of FX5/iQ-F serial support. |
| `Jn\X/Y/B/W/SB/SW` link-direct access | Not part of this profile. |

## Command-specific notes

| Command family | Notes |
| --- | --- |
| Batch read/write | Confirmed for supported plain read/write families. `S` is locally rejected because it is not part of the supported serial MC surface. |
| Native random read | Confirmed for supported random-capable families, including `LCN` and `LZ` as double-word items. |
| Native random write | Confirmed for supported read/write families. `LCN` and `LZ` require double-word items. |
| Multi-block read/write | Confirmed for supported normal plain devices. Long-state and native-random-only families are not multi-block heads. |
| Monitor `0801/0802` | Not supported for this profile; tested probes returned `0x7E40`. |
| Host/module buffer | Not supported for this profile; tested `0613` host-buffer and `0601` module-buffer probes returned `0x7E40`. |
| `DX`/`DY` | Not supported for this profile. A CLI `c4-ascii-f1` error-code probe returned `0x7F21` for raw `DX0` read/write on the tested iQ-F setup. |

## Excluded from iQ-F serial MC support

| Category | Device families or routes |
| --- | --- |
| FX5 device not present in the checked manual table | `V`, `ZR`, `DX`, `DY` |
| Unsupported serial MC step relay | `S` |
| Long timer / long retentive timer families absent on FX5 | `LTS`, `LTC`, `LTN`, `LSTS`, `LSTC`, `LSTN` |
| iQ-R CPU buffer memory | `Un\HG` |
| Link-direct routes | `Jn\X`, `Jn\Y`, `Jn\B`, `Jn\W`, `Jn\SB`, `Jn\SW` |
| Unsupported command routes | Monitor, host-buffer, and module-buffer helper paths |

## Implementation guards

| Guard | Current behavior |
| --- | --- |
| iQ-F unsupported plain devices | `S`, `V`, `ZR`, `DX`, `DY`, `LTS`, `LTC`, `LTN`, `LSTS`, `LSTC`, and `LSTN` are locally rejected under `melsec:iq-f`. |
| iQ-F unsupported special routes | `Jn\...`, `Un\HG`, host/module buffer, and monitor paths are locally rejected under `melsec:iq-f`. |

## Maintenance notes

- Keep `melsec:iq-f` separate from Q/L and iQ-R even where the C4 binary request
  shape is Q/L-compatible.
- Keep serial MC support separate from any CPU-side Ethernet or SLMP capability.
- Revisit ASCII, 1C, and 1E only if a dedicated FX5 validation pass is planned.
