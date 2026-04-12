# TODO

Current active follow-up items only.

## Native Command Holds

- none at the command-family level on the currently validated targets.

## Target-dependent Follow-up

- `RJ71C24-R2 + R120PCPU`: `1630` / `1631` remote password unlock/lock remain unresolved.
  Focused `--series iqr` checks returned `0x7FE7` for a `6`-character `unlock` attempt and
  `0x7F22` for `lock` plus longer `unlock` attempts (`10` and `32` characters), while read-only
  access such as `cpu-model` and `read-words D0 1` remained available.

- `RJ71C24-R2 + R120PCPU`: `1005` remote latch clear remains target-dependent.
  A focused `--series iqr` check returned `0x4013`, while `cpu-model` and `read-words D0 1`
  still passed immediately afterward.

- `RJ71C24-R2 + R120PCPU`: `LZ1` native `1402` remains target-dependent.
  Focused `--series iqr` checks returned `random-write-words=ok mode=native`, but an immediate
  `random-read LZ1` stayed at `1234`; `LZ0` full 32-bit write/readback/restore passed on the same
  target.

## Parked Implementation Gaps

These are known missing surfaces, but they are not active work right now.

- `1612`: serial-module mode switching is still not implemented.
- `0630`: programmable-controller CPU monitoring register/start is still not implemented.
- `2101`: on-demand data is still not implemented as a receive-side API.

## Notes

- Do not add unsupported access paths here.
- For long timer / long retentive timer contact+coil devices, use the structured `LTN/LSTN`
  `0401` path instead of direct probes.
