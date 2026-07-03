# Target-Dependent Native Command Evidence

Audience: maintainers preparing focused real-hardware checks for
target-dependent MC Protocol serial native-command results.

## Current Validation Hold

No active native-command hold is currently actionable.

Remote password control is kept here as historical target-dependent evidence,
but it is not treated as an implementation gap. On the available PLC setup, the
remote-password state could not be made active/known from the PLC side, so the
command result cannot currently be separated from target configuration.

| Area | Command | Evidence | Current status |
| --- | ---: | --- | --- |
| Remote password unlock | `1630` | `123456melsec` returned `0x7F22`; `abcdef1` returned `0x7FE7`; link sanity still passed. | waiting for a known active PLC remote-password state |
| Remote password lock | `1631` | `123456melsec` returned `0x7F22`; link sanity still passed. | waiting for a known active PLC remote-password state |

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

Reopen this as an active TODO only after the target-side remote password
setting and CPU/module state are known. At that point, capture the resulting PLC
end code and explain it against the known target settings.

## Recheck Rules

- Use one serial client at a time. Do not overlap probes on the same COM port.
- Record PLC model, serial module, frame format, serial settings, station, sum-check, profile, command, raw TX/RX, and PLC end code.
- After each failed native command, immediately run a read-only sanity check such as `cpu-model` or `read-words D0 1`.
- Do not add silent fallback behavior for rejected native commands.
