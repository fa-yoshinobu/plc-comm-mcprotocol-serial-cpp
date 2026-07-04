# TODO

Current active TODOs only.

## Current Status

### Live-measure MC serial response codes

Pending scope:

- `1C` NAK code meanings.
- Unclassified `7Fxx` serial-module error responses.

Record live-device evidence for MC Protocol Serial response codes that should be
documented but are not yet reliable enough to publish from manuals alone.

Required bench:

- Mitsubishi PLC or serial communication module that can accept MC Protocol Serial.
- PC serial interface matching the target port: USB-RS232C, USB-RS422, or USB-RS485.
- Correct cable and wiring for the selected physical layer, including 2-wire/4-wire and
  termination settings when RS-422/485 is used.
- Known-good normal communication settings: baud rate, data bits, parity, stop bits,
  station/PC number, and checksum/sum-check setting.
- Ability to switch or configure the target for both A-compatible `1C` frames and QnA
  extended `3C/4C` frames, or two equivalent benches if one target cannot cover both.

Measurements to collect:

- `1C` NAK codes returned by deliberately malformed but transmitted requests:
  checksum/sum-check mismatch, protocol/command error, station or PC number mismatch,
  and invalid address/count where applicable.
- QnA extended `3C/4C` serial-link `7Fxx` codes returned by deliberately malformed
  serial requests: checksum/sum-check mismatch, frame format mismatch, station mismatch,
  data length mismatch, and response-unavailable cases where the PLC/module returns a
  code instead of timing out.
- CPU-side 4000-series errors only when they appear on the serial path; these should be
  linked to the shared SLMP-style CPU error explanation rather than duplicated as a new
  MC-serial-only table.

How to use the results:

- Add only observed codes to the shared PLC Setup MC Protocol Serial error-code page,
  using project wording rather than copied manual text.
- Add practical recovery guidance to `GOTCHAS.md` only when the recovery is
  library-specific. Put common protocol/setup guidance in the shared PLC Setup pages.
- Add maintainer notes under `docsrc/maintainer/` with the bench model, serial settings,
  request shape, observed code, and whether the result is PLC/module-specific.
- Add parser/client regression tests only for codes that the library can classify from a
  received response. Do not add tests for pure timeout/no-response cases unless the code
  behavior changes.
- Do not expand public documentation with unobserved `7Fxx` or `1C` code guesses.

Remote password checks are kept as target-dependent historical evidence in
[TARGET_DEPENDENT_NATIVE_COMMANDS.md](docsrc/maintainer/TARGET_DEPENDENT_NATIVE_COMMANDS.md), but
they are not an active TODO until a PLC/module setup with a known active remote
password state is available.

## Notes

- Do not add unsupported access paths here.
- Do not add TODOs for manual families that are explicitly not needed by this library. The current
  omitted-family policy is documented in [MANUAL_DERIVED_RULES.md](docsrc/maintainer/MANUAL_DERIVED_RULES.md).
- Do not mark a target-dependent PLC rejection as an implementation bug unless request-shape tests
  or new hardware evidence point to the encoder/client code.
- For long timer / long retentive timer contact+coil devices, use the structured `LTN/LSTN`
  `0401` path instead of direct probes.
