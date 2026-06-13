# TODO

Current active follow-up items only.

## Native Command Holds

- [x] **No command-family holds**: None at the command-family level on the currently validated targets.

## Target-dependent Follow-up

- [ ] **RJ71C24-R2 remote password**: `1630` / `1631` remote password unlock/lock remain unresolved.
  Focused `--series iqr` checks returned `0x7FE7` for a `6`-character `unlock` attempt and
  `0x7F22` for `lock` plus longer `unlock` attempts (`10` and `32` characters), while read-only
  access such as `cpu-model` and `read-words D0 1` remained available. On 2026-06-12 against
  `RJ71C24-R2 + R08CPU`, user-configured password `123456melsec` still returned `0x7F22` for both
  `unlock` and `lock`; after changing the password to `abcdef1`, `unlock` returned `0x7FE7`.
  Read-only sanity still passed afterward. Focused setup, raw frames, and the next recheck plan are
  kept in [TARGET_DEPENDENT_NATIVE_COMMANDS.md](TARGET_DEPENDENT_NATIVE_COMMANDS.md).

## Cross-Library API Alignment

- [ ] **Unify public PLC profile naming**: Review the public selector currently exposed around
  `PlcSeries` and align the user-facing API with the cross-library `PlcProfile` policy where
  practical. The standard route should derive frame format, series, and legacy/iQ-R style access
  from one explicit profile, and saved/displayed names should use canonical lowercase strings such
  as `melsec:iq-r`. Short labels such as `iqr` or `iq-r` may remain input aliases only.

## Notes

- Do not add unsupported access paths here.
- Do not add TODOs for manual families that are explicitly not needed by this library. The current
  omitted-family policy is documented in
  [MANUAL_COMMAND_COVERAGE.md](MANUAL_COMMAND_COVERAGE.md).
- For long timer / long retentive timer contact+coil devices, use the structured `LTN/LSTN`
  `0401` path instead of direct probes.
