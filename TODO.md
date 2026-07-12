# TODO

Current active TODOs only.

## Current Status

Static implementation and mock/CLI validation for D-087 through D-092 are complete. The following
physical checks remain intentionally deferred until the matching PLC/module is connected and the
user explicitly approves the stated batch.

## D-087 through D-092 live serial verification

### MELSEC iQ-R / `melsec:iq-r`

- [ ] Target: RCPU with RJ71C24-R2-class module; endpoint `COM3`; explicit settings `19200`, `8E1`,
  `HardwareFlowControl::None`; read-only `D100`, 1 word. Purpose: confirm the new required typed
  configuration opens the known endpoint and preserves the confirmed C4 frame behavior. Expected
  evidence: successful model/read result plus recorded effective host serial settings. Restoration:
  none (read-only).
- [ ] Same controlled setup, but only if an RTS/CTS-wired adapter and matching module configuration
  are available: explicit `HardwareFlowControl::RtsCts`; read-only `D100`, 1 word. Purpose: confirm
  Windows/POSIX RTS/CTS mapping on real wiring. Expected evidence: successful read and captured
  adapter/module configuration. Restoration: restore module and adapter to no-flow 8E1.

### MELSEC-Q / `melsec:qcpu`

- [ ] Target: Q06UDVCPU with QJ71C24N; endpoint `COM3`; explicit settings `19200`, `8E1`,
  `HardwareFlowControl::None`; read-only `D100`, 1 word. Purpose: confirm required settings and
  validation order against the known Q-series setup. Expected evidence: successful model/read result
  plus recorded effective host serial settings. Restoration: none (read-only).

### Alternate serial formats

- [ ] Target/profile/endpoint: to be selected only from available test hardware configured for ASCII
  with 7 data bits and/or two stop bits. Device: `D100`, 1-word read only. Purpose: confirm the
  explicitly supported ASCII+7 and stop-bit 2 backend paths. Expected evidence: configuration
  snapshot and successful response using the same explicit library values. Restoration: return the
  serial module to its recorded original data/parity/stop settings.

Each unavailable row remains `unverified`; release disposition must be decided row by row. Do not
change module settings or communicate with `COM3` until the user identifies the connected PLC and
explicitly says `OK` for the exact batch.

## Notes

- Do not add unsupported access paths here.
- Do not add TODOs for manual families that are explicitly not needed by this library. The current
  omitted-family policy is documented in [MANUAL_DERIVED_RULES.md](docsrc/maintainer/MANUAL_DERIVED_RULES.md).
- Do not mark a target-dependent PLC rejection as an implementation bug unless request-shape tests
  or new hardware evidence point to the encoder/client code.
- For long timer / long retentive timer contact+coil devices, use the structured `LTN/LSTN`
  `0401` path instead of direct probes.
