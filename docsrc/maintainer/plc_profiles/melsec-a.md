# `melsec:a` Profile Draft

Status: maintainer draft, not a user-facing support contract.

## Target family

MELSEC-A-compatible targets using ACPU common command paths.

## Evidence status

| Topic | Status | Notes |
| --- | --- | --- |
| Public profile string | Library policy | `melsec:a` is the explicit A-series-compatible user-facing name. |
| Command-family branch | Manual-derived | ACPU common command symbols differ from QnA-family symbols. |
| Current repository behavior | Existing A-series branch | The current codec has A-only checks for several extended file-register paths. |
| Hardware evidence | Not in current support contract | Proceed by manual-derived inference and codec-level tests only. |
| Device support list | Scope boundary | Current codec acceptance is not a final target support statement. |

## Verification policy

Because no A-series test target is available, this profile is maintained as an
inferred/manual-backed profile for now.

- Treat manual command tables as the primary evidence.
- Treat local unit tests as encoder/decoder shape checks only.
- Do not mark any behavior as hardware-observed.
- Do not promote the device list to user-facing read/write support without later target evidence.
- Keep A-series-only paths, such as `ER`, `EW`, and `ET`, separate from QnA-family `NR` / `NW` inference.

## Current codec branch to audit

For 1C ASCII command paths, the ACPU common symbols are currently:

| Operation | A-series command |
| --- | --- |
| Batch read bits | `BR` |
| Batch read words | `WR` |
| Batch write bits | `BW` |
| Batch write words | `WW` |
| Random write bits | `BT` |
| Random write words | `WT` |
| Register monitor bits | `BM` |
| Register monitor words | `WM` |
| Read monitor bits | `MB` |
| Read monitor words | `MN` |
| Extended file-register read/write | `ER` / `EW` |
| Extended file-register random write | `ET` |

## Device-support inventory to test

| Route | Candidate families in current codec surface |
| --- | --- |
| 1C/1E common devices | `X`, `Y`, `M`, `L`, `F`, `B`, `D`, `W`, `R`, `TS`, `TC`, `TN`, `CS`, `CC`, `CN` |
| Not supported | `S` |
| Extended file register | A-series `ER`, `EW`, and `ET` paths are currently A-only. |
| Direct extended file register | Verify separately; current direct `NR` / `NW` paths are QnA-family, not A-series. |

## Scope Boundary

- Do not widen A-series CPU/module scope from this draft alone.
- Do not promote additional 1E binary/ASCII combinations without a deliberate support-contract update.
- Do not publish A-series extended file-register limits as supported ranges from codec acceptance alone.
