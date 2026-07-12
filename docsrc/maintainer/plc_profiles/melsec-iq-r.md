# `melsec:iq-r` Profile Specification

Status: target-validated maintainer specification for the tested iQ-R setup.

This file records the current implementation contract for the public
`melsec:iq-r` profile. It separates manual-derived wire-format rules, local
implementation branches, and target observations.

Test addresses in logs are validation points only. This profile does not define
fixed device-number ranges.

## Target family

MELSEC iQ-R serial communication paths, for example RCPU plus RJ71C24-class
serial modules.

## Decision summary

| Area | Decision |
| --- | --- |
| Public profile string | Use `melsec:iq-r` for iQ-R serial-module targets. |
| Internal series branch | Map to `PlcSeries::IQ_R`; `is_iq_r_series()` must be true. |
| Confirmed frame/code modes | C4 binary / Format5 and C4 ASCII / Format4 are confirmed on the tested iQ-R target. The serial module must be configured for the same format selected by the client. |
| Normal devices | The plain device families listed below are supported through normal device access. |
| Special devices | `Jn\...`, `Un\G`, and `Un\HG` are supported only through their dedicated routes. |
| ASCII Format1-3 | Not covered by the current iQ-R profile confirmation. |

## Evidence status

| Topic | Status | Notes |
| --- | --- | --- |
| Public profile string | Library policy | `melsec:iq-r` is the explicit iQ-R user-facing name. |
| iQ-R request-shape branch | Manual-derived | MC Protocol tables distinguish iQ-R request forms from Q/L forms. |
| Current repository behavior | Existing iQ-R branch | `is_iq_r_series()` controls the branches listed below. |
| Hardware observations | Confirmed for tested target | RCPU/RJ71C24-R2-class iQ-R serial setup, tested on 2026-07-01. |
| Device support list | Confirmed for tested target | The C4 binary / Format5 cross-check completed with `total: 125`, `ok: 125`, `ng: 0`. |

## Confirmed test setup

| Item | Value |
| --- | --- |
| PLC family | MELSEC iQ-R |
| Serial module family | RJ71C24-R2-class serial path |
| MC Serial port | `COM3` |
| MC Serial settings | `19200`, `8E1`, no RTS/CTS, station `0`, sum-check off |
| SLMP peer | `192.168.250.100:1025` |
| Validation evidence | Target observations summarized in this profile record. |
| Result summary | `total: 125`, `ok: 125`, `ng: 0` |

## Request-shape branch

These are the iQ-R branches that must stay separate from Q/L behavior.

| Item | iQ-R behavior |
| --- | --- |
| Normal word subcommand | `0002` |
| Normal bit subcommand | `0003` |
| Extended word subcommand | `0082` |
| Extended bit subcommand | `0083` |
| Binary device reference | 2-byte device code plus 4-byte device number |
| ASCII device reference | iQ-R-width fields are confirmed for C4 ASCII Format4, including the trailing device-modification field in native extended references. |
| Random/multi-block limits | Use the existing iQ-R branch limits; do not reuse Q/L limits. |
| Link-direct native wire body | Keep the current binary compatibility exception isolated to link-direct native traffic. |

## Confirmed support devices

These are confirmed for `melsec:iq-r` on the tested iQ-R target.

| Support class | Device families |
| --- | --- |
| Plain bit read/write | `X`, `Y`, `M`, `L`, `SM`, `F`, `V`, `B`, `TS`, `TC`, `STS`, `STC`, `CS`, `CC`, `SB`, `DX`, `DY` |
| Plain word read/write | `D`, `SD`, `W`, `TN`, `STN`, `CN`, `SW`, `Z`, `R`, `RD`, `ZR` |
| Long-state read/write helper | `LTS`, `LTC`, `LSTS`, `LSTC`, `LCS`, `LCC` |
| Long/current-value read/write | `LTN`, `LSTN`, `LCN`, `LZ` |
| Link-direct read/write | `Jn\X`, `Jn\Y`, `Jn\B`, `Jn\W`, `Jn\SB`, `Jn\SW` |
| Native-qualified read/write | `Un\G`, `Un\HG` |
| Not supported | `S` |

## Dedicated route details

The following families require route-specific handling. Do not silently fall
back to ordinary plain-device access when one of these routes is requested.

| Route | Confirmed behavior |
| --- | --- |
| Long-state bit devices | Use the long-state helper route for `LTS/LTC/LSTS/LSTC/LCS/LCC`. Ordinary direct `read-bits` is intentionally rejected for this class. |
| Link-direct devices | Use the link-direct route for `Jn\X/Y/B/W/SB/SW`. Do not treat these as plain device strings. |
| Native-qualified devices | Use the native-qualified route for `Un\G` and `Un\HG`. Do not treat standalone `G` or `HG` as plain devices. |

For `Un\G` and `Un\HG`, the confirmed route is native device access
(`0401/1401` with iQ-R extended subcommands). Keep this separate from any
qualified-buffer helper path so the implementation cannot choose the wrong
route silently.

## Helper-route observations (2026-07-03, Format1/Format2 full suite)

| Route | Observation |
| --- | --- |
| `S` device | Not supported for serial MC. ASCII formats are rejected by the C24 with `7F22H` before CPU forwarding; a historical binary Format5 read observation is not promoted to support because `S` is absent from the SH-080003-AF MC device-code list (p.68-69). |
| Raw monitor read (`0802` without `0801`) | `7155H` monitor-not-registered per SH-081249-L p.518. Passes immediately after a registration in the same module state. |
| `0601/1601` module-buffer helper | CPU error `0x4043` on this rack. SH-080003-AF p.155 scopes `0601/1601` to QnA-series special function modules; use the native-qualified `Un\G` route for iQ-R module access. |

## Excluded from this decision

| Item | Reason |
| --- | --- |
| C4 ASCII Format1-3 | Not covered by the current iQ-R profile confirmation. |
| C1/C2/C3/E1 frames | Not covered by the current iQ-R profile confirmation. |
| Standalone `G` / `HG` | Profile-common rule: these are not plain devices for any MELSEC profile. Use qualified `Un\G` / `Un\HG` forms only when that profile supports them. |
