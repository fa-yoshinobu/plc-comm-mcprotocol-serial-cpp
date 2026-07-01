# `melsec:iq-l` Profile Specification

Status: target-validated maintainer specification for the tested iQ-L serial
setup.

This file records the current implementation contract for the public
`melsec:iq-l` profile. Test addresses are intentionally omitted; they were
validation points only and do not define device-number ranges.

## Target family

MELSEC iQ-L CPU modules accessed through MELSEC-L serial communication modules.

## Decision summary

| Area | Decision |
| --- | --- |
| Public profile string | Use `melsec:iq-l` for iQ-L serial-module targets. |
| Internal series branch | Keep `PlcSeries::IQ_L` separate from iQ-R, but use Q/L-compatible serial MC request shapes. |
| Confirmed frame/code mode | C4 binary / Format5 is the current confirmed support basis. |
| Normal devices | The plain device families listed below are supported through normal device access. |
| Special devices | `Un\G` is supported only through the native-qualified route. `Jn\...` is not confirmed on the observed setup. |
| CPU-side SLMP | Do not use CPU-side SLMP visibility to promote serial MC support. |

## Manual evidence

| Manual | Finding |
| --- | --- |
| `SH-082159CHN-F` MELSEC iQ-L Module Configuration Manual, Appendix 3, printed page 103 / PDF page 105 | For MC Protocol communication, iQ-R-series subcommands `00?2` and `00?3` are not supported. Use MELSEC-Q/L-series subcommands. File-control commands `1810`, `1811`, `1820`, `1822`, and `1824` to `182A` are also unsupported. |
| `BCN-P5999-1245-B` MELSEC iQ-L Serial Communication Module FB Reference | The target serial modules are `LJ71C24` and `LJ71C24-R2`; detailed serial behavior is delegated to the MELSEC-L and MELSEC-Q/L serial communication module manuals. |
| `SH-082170CHN-H` MELSEC iQ-L CPU Module User Manual (Application) | CPU-side SLMP covers CPU devices and intelligent-function-module buffer memory. This is not proof that the serial C24 MC wire shape is iQ-R-style. |

## Request-shape branch

The observed iQ-L serial path follows the Q/L request shape. Keep this separate
from CPU-side or Ethernet SLMP behavior.

| Item | iQ-L serial MC behavior |
| --- | --- |
| Normal word subcommand | `0000` |
| Normal bit subcommand | `0001` |
| Extended word subcommand | `0080` |
| Extended bit subcommand | `0081` |
| Device reference width | Q/L-compatible form |
| Remote password | Q/L-compatible length rule unless later target evidence proves otherwise. |

## Confirmed support devices

These are confirmed for `melsec:iq-l` on the tested serial target.

| Support class | Device families |
| --- | --- |
| Plain bit read/write | `X`, `Y`, `M`, `L`, `SM`, `F`, `V`, `B`, `TS`, `TC`, `STS`, `STC`, `CS`, `CC`, `SB`, `DX`, `DY` |
| Plain bit read-only | `S` |
| Plain word read/write | `D`, `SD`, `W`, `TN`, `STN`, `CN`, `SW`, `Z`, `R`, `ZR` |
| Native-qualified read/write | `Un\G` |

## Dedicated route details

The following families require route-specific handling. Do not silently fall
back to ordinary plain-device access when one of these routes is requested.

| Route | Confirmed behavior |
| --- | --- |
| `Un\G` native-qualified access | Use native device access (`0401/1401`) with Q/L-compatible extended subcommand `0080`. |
| `Un\G` helper access | Reject the `0601/1601` helper route for iQ-L; it can read a different target value from the native-qualified route. |
| Standalone `G` | Not a plain device. Require the qualified `Un\G` form. |
| `Jn\X/Y/B/W/SB/SW` link-direct access | Not confirmed on the observed setup; read attempts returned `0x4031`, so write was not attempted. |

## Command-specific notes

| Command family | Notes |
| --- | --- |
| Batch read/write | Confirmed for all supported plain read/write families. `S` is read-only; write returned `0x4030`. |
| Native random read | Confirmed for `X`, `Y`, `M`, `L`, `SM`, `F`, `V`, `B`, `SB`, `S`, `D`, `SD`, `W`, `TN`, `STN`, `CN`, `SW`, `Z`, `R`, and `ZR`. Returned `0x4032` for `TS`, `TC`, `STS`, `STC`, `CS`, `CC`, `DX`, and `DY`. Treat this as a random-read limitation, not a batch-read exclusion. |
| Native random write | Confirmed for the supported read/write plain device families except `S`. `S` returned `0x4030`. |

## Excluded from iQ-L serial MC support

| Category | Device families |
| --- | --- |
| Long timer/counter bit devices | `LTS`, `LTC`, `LSTS`, `LSTC`, `LCS`, `LCC` |
| Long timer/counter word devices | `LTN`, `LSTN`, `LCN` |
| iQ-R long/index/module families | `LZ`, `RD` |
| iQ-R CPU buffer memory | `Un\HG` |
| File-control commands | `1810`, `1811`, `1820`, `1822`, `1824` to `182A` |

## Maintenance notes

- Keep the public `melsec:iq-l` profile separate even though the serial MC wire
  shape is Q/L-compatible.
- Keep serial MC support separate from CPU-side SLMP capabilities.
- Do not infer support from a single validation address or device number.
