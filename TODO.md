# TODO

Current active TODOs only.

## Current Status

No active TODOs are currently tracked.

## Notes

- Do not add unsupported access paths here.
- Do not add TODOs for manual families that are explicitly not needed by this library. The current
  omitted-family policy is documented in [MANUAL_DERIVED_RULES.md](docsrc/maintainer/MANUAL_DERIVED_RULES.md).
- Do not mark a target-dependent PLC rejection as an implementation bug unless request-shape tests
  or new hardware evidence point to the encoder/client code.
- For long timer / long retentive timer contact+coil devices, use the structured `LTN/LSTN`
  `0401` path instead of direct probes.
