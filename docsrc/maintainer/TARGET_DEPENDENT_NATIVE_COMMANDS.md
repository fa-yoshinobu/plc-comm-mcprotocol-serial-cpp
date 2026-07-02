# Target-Dependent Native Command Follow-up

Audience: maintainers preparing focused real-hardware rechecks for
target-dependent MC Protocol serial native-command results.

## Current Open Hold

Only remote password control remains open.

| Area | Command | Evidence | Current status |
| --- | ---: | --- | --- |
| Remote password unlock | `1630` | `123456melsec` returned `0x7F22`; `abcdef1` returned `0x7FE7`; link sanity still passed. | unresolved target/configuration question |
| Remote password lock | `1631` | `123456melsec` returned `0x7F22`; link sanity still passed. | unresolved target/configuration question |

Focused setup for the recorded checks:

- PLC: `R08CPU`
- Serial module: `RJ71C24-R2`
- Port/settings: `COM3`, `28800 / 8E2`
- Frame/profile: `c4-binary`, `melsec:iq-r`
- Station: `0`
- Sum-check: on

Use [scripts/recheck_remote_password.ps1](../../scripts/recheck_remote_password.ps1)
for the next run. It performs read-only sanity checks by default and requires
`-AllowRemotePasswordCommands` before sending `1630` / `1631`.

Close this hold only after the target-side remote password setting and CPU/module
state are known and the resulting PLC end code is explained.

## Resolved Target-Dependent Items

These are closed. Keep detailed evidence out of this page unless the issue is
reopened with fresh hardware data.

| Area | Result |
| --- | --- |
| 4C ASCII Format4 native extended access | Resolved as a client encoder bug. Post-fix `Jn\...`, `Un\G`, and `Un\HG` checks passed on iQ-R and Q/L targets. See [FORMAT4_ASCII_NATIVE_EXTENSION_ANALYSIS.md](FORMAT4_ASCII_NATIVE_EXTENSION_ANALYSIS.md). |
| Remote latch clear `1005` | Resolved as CPU-state dependent. Run after putting the CPU into STOP from the same channel. |
| Long index register `LZ1` native write | Resolved on the checked `R08CPU`; write/readback/restore passed. |

## Recheck Rules

- Use one serial client at a time. Do not overlap probes on the same COM port.
- Record PLC model, serial module, frame format, serial settings, station, sum-check, profile, command, raw TX/RX, and PLC end code.
- After each failed native command, immediately run a read-only sanity check such as `cpu-model` or `read-words D0 1`.
- Do not add silent fallback behavior for rejected native commands.
