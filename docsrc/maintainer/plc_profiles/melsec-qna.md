# `melsec:qna` Profile Draft

Status: maintainer draft, not a user-facing support contract.

## Target family

MELSEC QnA-compatible targets using the older 1C/1E-oriented command family.

## Evidence status

| Topic | Status | Notes |
| --- | --- | --- |
| Public profile string | Library policy | `melsec:qna` is the explicit QnA-compatible user-facing name. |
| Command-family branch | Manual-derived | Manuals define QnA-style command names separately from ACPU common commands. |
| Current repository behavior | Existing QnA-family branch | The current code groups QnA and AnA/AnU for shared command-family behavior. |
| Hardware evidence | Not in current support contract | Proceed by manual-derived inference and codec-level tests only. |
| Device support list | Scope boundary | Current codec support is command-shape support, not a final device support contract. |

## Verification policy

Because no QnA test target is available, this profile is maintained as an
inferred/manual-backed profile for now.

- Treat manual command tables as the primary evidence.
- Treat local unit tests as encoder/decoder shape checks only.
- Do not mark any behavior as hardware-observed.
- Do not promote the device list to user-facing read/write support without later target evidence.
- When a QnA target becomes available, record the exact CPU, serial module, frame, code mode, and station route before changing support status.

## Current codec branch to audit

For 1C ASCII command paths, the QnA-family symbols are currently:

| Operation | QnA-family command |
| --- | --- |
| Batch read bits | `JR` |
| Batch read words | `QR` |
| Batch write bits | `JW` |
| Batch write words | `QW` |
| Random write bits | `JT` |
| Random write words | `QT` |
| Register monitor bits | `JM` |
| Register monitor words | `QM` |
| Read monitor bits | `MJ` |
| Read monitor words | `MQ` |
| Direct extended file-register read/write | `NR` / `NW` |

## Device-support inventory to test

Current 1C/1E device acceptance in the codec is limited. Verify against manuals
and targets before documenting it as support.

| Route | Candidate families in current codec surface |
| --- | --- |
| 1C/1E common devices | `X`, `Y`, `M`, `L`, `F`, `B`, `D`, `W`, `R`, `TS`, `TC`, `TN`, `CS`, `CC`, `CN` |
| Not supported | `S` |
| Direct extended file register | QnA-family `NR` / `NW` paths are currently separate from A-series `ER` / `EW`. |

## Scope Boundary

- Do not claim `melsec:qna` is identical to `melsec:ana-anu` for every implemented command.
- Do not publish CPU/module-specific C1/E1 support from this draft alone.
- Do not publish point-count or address-range limits from codec acceptance alone.
