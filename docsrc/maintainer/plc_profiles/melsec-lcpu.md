# `melsec:lcpu` Profile Specification

Status: target-validated maintainer specification for the tested MELSEC-L
serial setup.

This file records the current `melsec:lcpu` basis. It uses manual-derived Q/L
request shapes and treats MELSEC-L as equivalent to `melsec:qcpu` unless fresh
target evidence proves a split. Keep exact validation addresses out of this
file; they are test points only.

## Target family

MELSEC-L serial communication paths, for example LCPU plus LJ71C24-class serial
modules.

## Decision summary

| Area | Decision |
| --- | --- |
| Public profile string | Use `melsec:lcpu` for MELSEC-L serial-module targets. |
| Internal series branch | Use the Q/L-compatible `PlcSeries::Q_L` request-shape branch. Keep the public profile separate from `melsec:qcpu`. |
| Confirmed frame/code basis | C4 binary / Format5 is the current support basis. |
| Normal devices | The plain device families listed below passed read/write/restore on the tested L target. |
| Special devices | Treat native-qualified `Un\G` as the Q-equivalent dedicated route. `Jn\...` is expected to match Q, but this setup cannot confirm it. Do not treat standalone `G` or `HG` as plain devices. |

## Evidence status

| Topic | Status | Notes |
| --- | --- | --- |
| Public profile string | Library policy | The manual names the MELSEC-L family; `melsec:lcpu` is the library's explicit user-facing selector. |
| Manual family | Manual-derived | Q/L serial manuals group MELSEC-Q and MELSEC-L request-shape behavior. |
| Current implementation | Implemented grouping | `PlcProfile::MelsecL` maps to `PlcSeries::Q_L`. |
| Current hardware observation | L target observed | `L26CPU-BT` with `LJ71C24`; C4 binary / Format5 / `19200`, `8E1`, station `0`, sum-check off. |
| Normal-device read/write | Confirmed for tested target | Batch read/write/restore passed for the read/write families below. `S` is not part of the supported serial MC surface. |
| Native random access | Partially command-limited | Random write passed for the support surface below. Random read rejected `TS`, `TC`, `STS`, `STC`, `CS`, and `CC` with `0x4032`; batch read/write still passed for those families. |
| Q-equivalent policy | Partially rechecked | Native-qualified `Un\G` matched the prepared value on the L target; helper access read a different value, so it must not be silently used as a fallback. |

## Manual evidence

| Manual | Finding |
| --- | --- |
| `SH-080949CHN-C` MELSEC-L Serial Communication Module User Manual (Basic) | Covers `LJ71C24` and `LJ71C24-R2` serial communication modules. |
| `SH-080284CHN-E` MELSEC-Q/L Serial Communication Module User Manual (Application) | Groups Q/L serial communication behavior and lists the ordinary Q/L device families used by the Q/L command branch. |

## Request-shape branch

| Item | MELSEC-L serial MC behavior |
| --- | --- |
| Normal word subcommand | `0000` |
| Normal bit subcommand | `0001` |
| Extended word subcommand | `0080` |
| Extended bit subcommand | `0081` |
| Device reference width | Q/L-compatible form |
| Remote password | Q/L-compatible length rule unless later target evidence proves otherwise. |

## Current support surface

Keep this aligned with `melsec:qcpu` until a fresh L target run proves a split from
Q/L behavior.

| Support class | Device families |
| --- | --- |
| Plain bit read/write | `X`, `Y`, `M`, `L`, `SM`, `F`, `V`, `B`, `TS`, `TC`, `STS`, `STC`, `CS`, `CC`, `SB`, `DX`, `DY` |
| Plain word read/write | `D`, `SD`, `W`, `TN`, `STN`, `CN`, `SW`, `Z`, `R`, `ZR` |
| Native-qualified read/write | `Un\G` |

## Dedicated route details

These forms require route-specific handling. Do not silently fall back to
ordinary plain-device access when one of these routes is requested.

| Route | Current L policy |
| --- | --- |
| `Jn\X/Y/B/SB` link-direct bits | Expected to match `melsec:qcpu`, but not confirmed on the available L setup because the required link-direct equipment is not present. |
| `Jn\W/SW` link-direct words | Expected to match `melsec:qcpu`, but not confirmed on the available L setup because the required link-direct equipment is not present. |
| `Un\G` native-qualified access | Use native device access (`0401/1401`) with Q/L-compatible extended subcommand `0080`, same as `melsec:qcpu`. A representative read matched the prepared value. |
| `Un\G` helper access | Reject the `0601/1601` helper route for `melsec:lcpu` `Un\G` access. On the L target it read a different value from the native-qualified route. |
| Standalone `G` / `HG` | Not plain devices. Require qualified forms, and only expose the forms supported by the selected profile. |

## Command-specific notes

| Command family | Notes |
| --- | --- |
| Batch read/write | Confirmed for all supported plain read/write families. `S` is locally rejected because it is not part of the supported serial MC surface. |
| Native random read | Confirmed for `X`, `Y`, `M`, `L`, `SM`, `F`, `V`, `B`, `SB`, `DX`, `DY`, `D`, `SD`, `W`, `TN`, `STN`, `CN`, `SW`, `Z`, `R`, and `ZR`. Returned `0x4032` for `TS`, `TC`, `STS`, `STC`, `CS`, and `CC`. Treat this as a random-read limitation, not a batch-read exclusion. |
| Native random write | Confirmed for all supported plain read/write families. `S` is locally rejected because it is not part of the supported write surface. |
| Native multi-block read/write (`0406`/`1406`) | Confirmed on the tested L serial path for supported plain devices. Treat this as serial-module evidence, separate from SLMP built-in-Ethernet Q-series block-command behavior. |
| Native-qualified read/write | Confirmed for `Un\G` through the native-qualified route. |

## Excluded from current L support

| Category | Device families |
| --- | --- |
| Long timer/counter bit devices | `LTS`, `LTC`, `LSTS`, `LSTC`, `LCS`, `LCC` |
| Long timer/counter word devices | `LTN`, `LSTN`, `LCN` |
| iQ-R long/index/module families | `LZ`, `RD` |
| iQ-R CPU buffer memory | `Un\HG` |
| Step relay | `S` |

## Future validation notes

- Recheck `Jn\X/Y/B/W/SB/SW` on C4 binary if matching link-direct equipment
  becomes available.
- Compare the L result with fresh Q runs only for native random-read behavior;
  `S` is intentionally outside the supported serial MC surface.

## Maintenance notes

- Do not collapse the public `melsec:lcpu` selector into `melsec:qcpu`; they may
  share implementation today, but validation results must remain target-family
  specific.
- Do not infer device-number restrictions from validation addresses.
- Keep helper `0601/1601` and native-qualified `0401/1401` evidence separate.
