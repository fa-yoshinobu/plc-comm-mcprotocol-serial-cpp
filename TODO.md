# TODO

Current active TODOs only.

## Current Status

### Live-measure MC serial response codes

Pending scope:

- Optional additional `1C` NAK cases only if a new diagnostic need appears.
  The current iQ-F bench has already produced representative `c1-ascii-f1`
  NAK codes for invalid command, invalid device, and invalid count requests,
  plus Format4 station/PC mismatch and sum-check mismatch behavior.
- Additional unclassified `7Fxx` serial-module responses only when a new
  code appears. The initial iQ-F Format1 probe is recorded in
  [MC_SERIAL_ERROR_CODE_EVIDENCE.md](docsrc/maintainer/MC_SERIAL_ERROR_CODE_EVIDENCE.md).

Collected evidence:

- 2026-07-04, `melsec:iq-f`, `COM4`, `19200bps`, `8E1`, sum-check off,
  CLI `--frame c4-ascii-f1`: invalid command/subcommand and raw `0802`
  returned `0x7E40`; raw `DX0` read/write returned `0x7F21`; `D100`
  write/read sanity passed. A raw `S0` read returned success, but this is not a
  public support change.
- 2026-07-04, same iQ-F bench, CLI `--frame c1-ascii-f1`,
  `--plc-profile melsec:a`: `D100` read returned `0x5A3C`, matching the prior
  `D100` write/read sanity value. This confirms the bench can accept the
  library `C1` Format1 read path; it is not yet 1C error-code evidence.
- 2026-07-04, same `c1-ascii-f1` bench: invalid command `ZZ0` returned NAK
  code `0x03`, invalid device `@` returned NAK code `0x07`, and zero point
  count returned NAK code `0x06`.
- 2026-07-04, same iQ-F bench switched to protocol format 4: `c1-ascii-f4`
  station mismatch timed out, `c1-ascii-f4` PC mismatch returned NAK `0x10`,
  and tested `c4-ascii-f4` header mismatch probes timed out.
- 2026-07-04, same iQ-F bench with protocol format 4 and sum-check enabled:
  correct sum-check reads passed on both `c1-ascii-f4` and `c4-ascii-f4`;
  deliberately bad sum-check returned `c1` NAK `0x02` and `c4` `0x7F24`.
- 2026-07-04, same iQ-F bench with protocol format 1 and sum-check enabled:
  correct sum-check reads passed on both `c1-ascii-f1` and `c4-ascii-f1`;
  deliberately bad sum-check again returned `c1` NAK `0x02` and `c4`
  `0x7F24`; sum-check-enabled writes also passed on both `c1-ascii-f1`
  and `c4-ascii-f1` with cross-readbacks matching.

Record live-device evidence for MC Protocol Serial response codes that should be
documented but are not yet reliable enough to publish from manuals alone.

Required bench:

- Mitsubishi PLC or serial communication module that can accept MC Protocol Serial.
- PC serial interface matching the target port: USB-RS232C, USB-RS422, or USB-RS485.
- Correct cable and wiring for the selected physical layer, including 2-wire/4-wire and
  termination settings when RS-422/485 is used.
- Known-good normal communication settings: baud rate, data bits, parity, stop bits,
  station/PC number, and checksum/sum-check setting.
- Ability to switch or configure the target for both A-compatible `1C` frames
  (`c1-ascii-f1` / `c1-ascii-f4` in this library) and QnA extended `3C/4C`
  frames, or two equivalent benches if one target cannot cover both.

Measurements to collect:

- Additional `1C` NAK codes returned by deliberately malformed but transmitted
  requests only if the project later needs broader coverage beyond the
  2026-07-04 probes.
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
