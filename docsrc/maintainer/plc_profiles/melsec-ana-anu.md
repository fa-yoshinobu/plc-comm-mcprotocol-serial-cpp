# `melsec:ana-anu` Profile Draft

Status: maintainer draft, not a user-facing support contract.

## Target family

MELSEC AnA / AnU-compatible targets.

## Evidence status

| Topic | Status | Notes |
| --- | --- | --- |
| Public profile string | Library policy | Keep a separate public name for AnA/AnU even if some command shapes match QnA. |
| Command-family branch | Manual-derived grouping | Manuals distinguish CPU families; current code may share behavior where command shapes match. |
| Current repository behavior | Grouped with QnA-family branch | `AnA_AnU` is value-compatible with the QnA internal selector in the current code. |
| Hardware evidence | Not in current support contract | Proceed by manual-derived inference and codec-level tests only. |
| Device support list | Scope boundary | Do not copy QnA support without manual or deliberate support-contract evidence. |

## Verification policy

Because no AnA/AnU test target is available, this profile is maintained as an
inferred/manual-backed profile for now.

- Treat manual command tables as the primary evidence.
- Treat the current QnA-family grouping as an implementation hypothesis, not a hardware result.
- Treat local unit tests as encoder/decoder shape checks only.
- Do not mark any behavior as hardware-observed.
- Do not promote the device list to user-facing read/write support without later target evidence.

## Current codec branch to audit

The current implementation uses the QnA-family C1 command symbols for AnA/AnU.
This is an implementation grouping, not proof that all AnA/AnU behavior equals
QnA behavior.

| Operation | Currently grouped command |
| --- | --- |
| Batch read bits | `JR` |
| Batch read words | `QR` |
| Batch write bits | `JW` |
| Batch write words | `QW` |
| Random write bits | `JT` |
| Random write words | `QT` |
| Register monitor | `JM` / `QM` |
| Read monitor | `MJ` / `MQ` |
| Direct extended file-register | `NR` / `NW` |

## Device-support inventory to test

Use the QnA-family current codec surface only as a starting point.

| Route | Candidate families in current codec surface |
| --- | --- |
| 1C/1E common devices | `X`, `Y`, `M`, `L`, `F`, `B`, `D`, `W`, `R`, `TS`, `TC`, `TN`, `CS`, `CC`, `CN` |
| Not supported | `S` |
| Direct extended file register | Treat AnA/AnU manual applicability separately from QnA. |

## Scope Boundary

- Do not claim AnA/AnU equals QnA for every operation from the current grouping alone.
- Do not publish AnA/AnU address ranges or device families without profile-specific support evidence.
- Keep the internal selector grouping as an implementation detail unless a deliberate support-contract update changes it.
