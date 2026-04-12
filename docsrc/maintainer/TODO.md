# TODO

Current follow-up items only.

## Native Command Holds

- none at the command-family level on the currently validated targets.

## Target-dependent Follow-up

- `RJ71C24-R2 + R120PCPU`: `1630` / `1631` remote password unlock/lock remain unresolved.
  Focused `--series iqr` checks returned `0x7F22` for both `unlock` and `lock`, while read-only
  access such as `cpu-model` and `read-words D0 1` remained available.

## Hardware Validation Follow-up

- `1005`: remote latch clear is implemented but still not hardware-validated in this repo.

## Implementation Gaps

- `1612`: serial-module mode switching is still not implemented.
- `0630`: programmable-controller CPU monitoring register/start is still not implemented.
- `2101`: on-demand data is still not implemented as a receive-side API.

## Notes

- Do not add unsupported access paths here.
- For long timer / long retentive timer contact+coil devices, use the structured `LTN/LSTN`
  `0401` path instead of direct probes.
