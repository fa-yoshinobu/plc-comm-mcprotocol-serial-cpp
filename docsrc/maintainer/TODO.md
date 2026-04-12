# TODO

Active unresolved items only.

## Native Command Holds

- `RJ71C24-R2 + iQ-R CPU / Format5 Binary / --series iqr`: `LZ` still rejects every
  manual-listed native path tried so far.
  - `0403 random-read` -> `0x7F23`
  - `1402 random-write-words` -> `0x7F23`
  - `0801 register monitor` -> `0x7F23`
- `RJ71C24-R2 + iQ-R CPU / Format5 Binary / --series iqr`: `LTN`, `LSTN`, and `LCN`
  still reject native `0403 random-read` with `0x7F23`, even though structured/batch reads pass.
- `RJ71C24-R2 + iQ-R CPU / Format5 Binary / --series iqr`: `LTN`, `LSTN`, and `LCN`
  still reject native `1402 random-write-words` with `0x7F23`.
  - `LCN` does work through `1401 batch write`
- `FX5UC-32MT/D`: host/module buffer access still holds.

## Validation Gaps

- `Jn\\...` native random read/write are implemented but not yet hardware-validated beyond the
  current `RJ71C24-R2` rejection result (`0x7F23`).
- `Jn\\...` native monitor registration is implemented but not yet hardware-validated beyond the
  current `RJ71C24-R2` rejection result (`0x7F22`).

## Notes

- Do not add unsupported access paths here.
- For long timer / long retentive timer contact+coil devices, use the structured `LTN/LSTN`
  `0401` path instead of direct probes.
