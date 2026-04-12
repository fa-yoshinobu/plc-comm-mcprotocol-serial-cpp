# TODO

Current follow-up items only.

## Native Command Holds

- none at the command-family level on the currently validated targets.

## Implementation Gaps

- `1612`: serial-module mode switching is still not implemented.
- `0630`: programmable-controller CPU monitoring register/start is still not implemented.
- `2101`: on-demand data is still not implemented as a receive-side API.

## Target-dependent Follow-up

- `RJ71C24-R2 + R120PCPU`: `1006 remote-reset` still returns `0x408B` in current validation.
  Manual conditions require `STOP` state and target-side remote RESET enable, so this remains a
  parameter-dependent hardware follow-up rather than a codec bug.

## Notes

- Do not add unsupported access paths here.
- For long timer / long retentive timer contact+coil devices, use the structured `LTN/LSTN`
  `0401` path instead of direct probes.
