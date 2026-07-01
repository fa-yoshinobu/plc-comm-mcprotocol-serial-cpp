# `melsec:iq-f` Profile Draft

Status: maintainer draft, not a user-facing support contract.

## Target family

MELSEC iQ-F / FX5 serial communication paths.

## Evidence status

| Topic | Status | Notes |
| --- | --- | --- |
| Public profile string | Library policy | `melsec:iq-f` is a proposed explicit user-facing name for FX5/iQ-F targets. |
| Manual source | Manual-derived source added | `SH-082624-J - MELSEC iQ-F FX5 User's Manual (Communication)` is available in the workspace. |
| FX5 subcommand structure | Manual-derived, needs page-backed extraction | The FX5 manual describes subcommand bit fields for data size, device-reference width, and device-memory extension. |
| Current repository behavior | Needs audit | Earlier FX5 tests used the older combined Q/L-style profile. Do not treat that as the final iQ-F profile. |
| Device support list | Pending | FX5 device support must be separated from Q/L and iQ-R assumptions. |

## Request-shape items to verify

FX5/iQ-F must be checked from its own manual first.

| Item | Verification note |
| --- | --- |
| Normal word/bit subcommands | Confirm which subcommand bit fields are required for each device-reference form. |
| Extended device-memory bit | Confirm when the extension bit is required and which devices use it. |
| Device code width | Confirm short and long forms separately. |
| Device number width | Confirm short and long forms separately. |
| 3C/4C frame behavior | Verify ASCII and binary separately. |
| 1C/1E behavior | Do not assume supported unless the FX5 manual says so. |

## Device-support inventory to test

Do not inherit Q/L or iQ-R device lists automatically.

| Route | Status |
| --- | --- |
| Plain bit/word devices | Pending. Start with ordinary safe devices, then special devices. |
| `DX`, `DY`, `V`, `ZR` | Previous FX5 serial observations reported unsupported or outside the validated subset; retest under the explicit iQ-F profile before deciding. |
| Random / multi-block / monitor | Pending. Previous observations must be reclassified by frame/profile. |
| Host/module buffer | Pending. Previous FX5 serial observations reported unsupported or not applicable. |
| Link direct `Jn\...` | Pending. Do not assume Q/L or iQ-R behavior. |
| Qualified buffer `Un\G` / `Un\HG` | Pending. Do not assume supported. |

## Open questions

- Which FX5 manual tables define the exact serial MC Protocol device list?
- Does FX5 require a profile-specific encoder rather than Q/L or iQ-R grouping?
- Which previously observed FX5 failures are PLC limitations, module limitations, frame limitations, or local software bugs?
