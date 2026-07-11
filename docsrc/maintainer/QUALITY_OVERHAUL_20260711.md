# MC Protocol Serial C++ quality overhaul

Status: implementation in progress
Branch: `quality/2026-07-overhaul`
Authoritative cross-library decisions: `D:\APP\omittable_configuration_decisions_20260711.md`

This record preserves the repository-specific implementation contract and evidence. A checked item
means evidence exists; Claude and live-hardware items remain open until they are actually performed
or explicitly dispositioned.

## D-087: explicit serial device path

Scope: `PosixSerialConfig`, Windows/POSIX serial backends, `PosixSyncClient`, CLI, examples, tests,
and documentation.

Target contract: a serial configuration cannot be default-constructed. The device path is a required
constructor/CLI value, empty and embedded-NUL values are rejected before OS handle access, and no
COM or `/dev` path is inferred.

Compatibility impact: `PosixSerialConfig config {}; config.device_path = ...;` no longer compiles;
construct with all six explicit connection fields.

Acceptance criteria:

1. `std::is_default_constructible_v<PosixSerialConfig>` is false.
2. Empty and embedded-NUL paths return `InvalidArgument` before `open`/`CreateFile`.
3. CLI execution without `--device` is rejected and no default port constant remains.
4. POSIX passes a checked NUL-terminated copy to `open()`; Windows retains checked path copying.

- [x] Implementation completed in this repository.
- [x] Tests added or updated for the acceptance criteria that do not require hardware.
- [x] Relevant checks passed and evidence recorded.
- [x] Codex self-review completed.
- [ ] Claude source review completed (`pending user authorization`).
- [ ] Claude findings dispositioned and affected checks rerun.
- [ ] Required live serial/PLC checks passed or explicitly dispositioned.
- [x] Documentation, migration notes, changelog, and source examples agree with the implementation.
- [ ] Final acceptance criteria verified and the item marked complete.

## D-088: explicit baud rate

Scope: the same host configuration, backends, CLI, examples, tests, and documentation as D-087.

Target contract: baud rate is required and must be greater than zero. The library does not inject,
infer, round to, or retry with 9600 or any other rate.

Compatibility impact: callers relying on `baud_rate = 9600` must explicitly pass 9600.

Acceptance criteria:

1. Constructor and CLI require an explicit baud rate.
2. Zero and malformed CLI values are rejected before OS handle access.
3. No default/fallback baud remains in public configuration or CLI behavior.

- [x] Implementation completed in this repository.
- [x] Tests added or updated for the acceptance criteria that do not require hardware.
- [x] Relevant checks passed and evidence recorded.
- [x] Codex self-review completed.
- [ ] Claude source review completed (`pending user authorization`).
- [ ] Claude findings dispositioned and affected checks rerun.
- [ ] Required live serial/PLC checks passed or explicitly dispositioned.
- [x] Documentation, migration notes, changelog, and source examples agree with the implementation.
- [ ] Final acceptance criteria verified and the item marked complete.

## D-089: explicit data bits and code-mode validation

Scope: `PosixSerialConfig`, `ProtocolConfig.code_mode`, both host backends, `PosixSyncClient`, CLI,
examples, tests, and documentation.

Target contract: data bits are required. Binary accepts only 8; ASCII accepts 7 or 8. Values 5, 6,
and all other widths are rejected without correction before opening the port through MC client paths.

Compatibility impact: callers must explicitly pass 8 for binary or the module's explicit 7/8 value
for ASCII; generic host serial use with 5/6 bits is no longer supported by this MC-specific backend.

Acceptance criteria:

1. Binary+8 and ASCII+7/8 validate successfully.
2. Binary+7 and generic 5/6/other values fail before OS handle access.
3. Windows and POSIX backends do not retain 5/6-bit configuration branches.

- [x] Implementation completed in this repository.
- [x] Tests added or updated for every acceptance criterion.
- [x] Relevant checks passed and evidence recorded.
- [x] Codex self-review completed.
- [ ] Claude source review completed (`pending user authorization`).
- [ ] Claude findings dispositioned and affected checks rerun.
- [ ] Required live serial/PLC checks passed or explicitly dispositioned.
- [x] Documentation, migration notes, changelog, and source examples agree with the implementation.
- [ ] Final acceptance criteria verified and the item marked complete.

## D-090: explicit stop bits

Scope: host serial configuration/backends, `PosixSyncClient`, CLI, examples, tests, and documentation.

Target contract: stop bits are required and exactly 1 or 2 is accepted. No value is inferred or
rounded and invalid values fail before OS handle access.

Compatibility impact: callers relying on the former value 1 must pass 1 explicitly.

Acceptance criteria:

1. Constructor and CLI require an explicit stop-bit value.
2. Values 1 and 2 are accepted; 0, 3, and out-of-range inputs are rejected.
3. Documentation examples and reference tables use internally consistent explicit values.

- [x] Implementation completed in this repository.
- [x] Tests added or updated for the acceptance criteria that do not require hardware.
- [x] Relevant checks passed and evidence recorded.
- [x] Codex self-review completed.
- [ ] Claude source review completed (`pending user authorization`).
- [ ] Claude findings dispositioned and affected checks rerun.
- [ ] Required live serial/PLC checks passed or explicitly dispositioned.
- [x] Documentation, migration notes, changelog, and source examples agree with the implementation.
- [ ] Final acceptance criteria verified and the item marked complete.

## D-091: typed explicit parity

Scope: public host serial API, both backends, `PosixSyncClient`, CLI, examples, tests, and docs.

Target contract: `SerialParity::{None, Even, Odd}` is a required constructor value. Unknown enums,
missing/empty/unknown CLI input, and backend fallback to another parity are prohibited.

Compatibility impact: the old `char parity` field and its `'N'` default are removed.

Acceptance criteria:

1. All three enum values map consistently in Windows and POSIX backends.
2. Unknown enum values fail validation before OS handle access.
3. CLI strictly normalizes N/E/O (including documented case variants) to the enum and rejects other
   values or omission.

- [x] Implementation completed in this repository.
- [x] Tests added or updated for the public API acceptance criteria.
- [x] Relevant checks passed and evidence recorded.
- [x] Codex self-review completed.
- [ ] Claude source review completed (`pending user authorization`).
- [ ] Claude findings dispositioned and affected checks rerun.
- [ ] Required live serial/PLC checks passed or explicitly dispositioned.
- [x] Documentation, migration notes, changelog, and source examples agree with the implementation.
- [ ] Final acceptance criteria verified and the item marked complete.

## D-092: typed explicit hardware flow control

Scope: public host serial API, both backends, `PosixSyncClient`, CLI, examples, tests, and docs.

Target contract: `HardwareFlowControl::{None, RtsCts}` is a required constructor/CLI selection.
Hardware flow control is separate from the explicit `rts_toggle` RS-485 direction-control setting,
and neither mode is inferred or used as fallback for the other.

Compatibility impact: the old `bool rts_cts` field and its false default are removed.

Acceptance criteria:

1. Both enum values map consistently to Windows DCB and POSIX termios settings.
2. Unknown enum values fail validation before OS handle access.
3. CLI requires `--hardware-flow none|rts-cts`; omission and unknown values are rejected.
4. `--rts-toggle` remains a separate, explicitly selected RS-485 direction-control feature.

- [x] Implementation completed in this repository.
- [x] Tests added or updated for the public API acceptance criteria.
- [x] Relevant checks passed and evidence recorded.
- [x] Codex self-review completed.
- [ ] Claude source review completed (`pending user authorization`).
- [ ] Claude findings dispositioned and affected checks rerun.
- [ ] Required live serial/PLC checks passed or explicitly dispositioned.
- [x] Documentation, migration notes, changelog, and source examples agree with the implementation.
- [ ] Final acceptance criteria verified and the item marked complete.

## D-093: explicit frame family

Scope: `ProtocolConfig`, frame codec, clients, CLI, examples, tests, and docs.

Target contract: frame family is a required session choice; no C4 default or runtime fallback to a
different family exists. Unknown enum values and compiled-out families fail before encoding.

Compatibility impact: generic `ProtocolConfig {}` no longer represents C4. Callers must assign a
defined frame or use a frame-named preset.

Acceptance criteria:

1. Omitted and unknown frame values fail with zero encoded bytes.
2. Every defined frame is validated only against its supported code/format/profile combinations.
3. No timeout, PLC error, or decode error changes the configured frame.

Progress: invalid-by-default frame state, exhaustive enum validation, CLI-required `--frame`, and
zero-size-on-encode-failure are implemented and tested. The final constructor/tagged-type migration
and D-099 route-type integration remain open.

- [ ] Implementation completed in this repository.
- [ ] Tests added or updated for every acceptance criterion.
- [ ] Relevant checks passed and evidence recorded.
- [ ] Codex self-review completed.
- [ ] Claude source review completed (`pending user authorization`).
- [ ] Claude findings dispositioned and affected checks rerun.
- [ ] Required live serial/PLC checks passed or explicitly dispositioned.
- [ ] Documentation and final acceptance agree with the completed implementation.

## D-094: explicit code mode

Scope: `ProtocolConfig`, frame codec, serial-data validation, clients, presets, CLI, tests, and docs.

Target contract: Binary or ASCII is selected explicitly and remains fixed for the session. Binary
configuration cannot accept or silently ignore ASCII-only input.

Compatibility impact: generic config no longer defaults to Binary.

Acceptance criteria:

1. Omitted/unknown mode and unsupported frame/mode combinations fail before encoding/open.
2. Binary requires eight serial data bits.
3. Public binary configuration has no ASCII-only settings and no mode fallback occurs.

Progress: invalid-by-default mode, exhaustive validation, named presets, and serial combination
validation are implemented. Binary/ASCII tagged-type separation remains open with D-095/D-096.

- [ ] Implementation completed in this repository.
- [ ] Tests added or updated for every acceptance criterion.
- [ ] Relevant checks passed and evidence recorded.
- [ ] Codex self-review completed.
- [ ] Claude source review completed (`pending user authorization`).
- [ ] Claude findings dispositioned and affected checks rerun.
- [ ] Required live serial/PLC checks passed or explicitly dispositioned.
- [ ] Documentation and final acceptance agree with the completed implementation.

## D-095: explicit ASCII format

Scope: ASCII protocol configuration, codecs, clients, presets, CLI, tests, and docs.

Target contract: supported ASCII frames require an explicit Format1/2/3/4 selection; Binary and 1E
public configuration do not expose an irrelevant ASCII-format field.

Compatibility impact: generic config no longer defaults to Format3.

Acceptance criteria:

1. Omitted/unknown format fails for ASCII before encoding.
2. Frame-specific formats are enforced and the decoder remains fixed to the selected format.
3. Binary/1E types contain no inactive ASCII-format input.

Progress: invalid-by-default format, exhaustive validation, CLI frame parsing, and fixed decoder
selection are implemented. Tagged-type field removal remains open.

- [ ] Implementation completed in this repository.
- [ ] Tests added or updated for every acceptance criterion.
- [ ] Relevant checks passed and evidence recorded.
- [ ] Codex self-review completed.
- [ ] Claude source review completed (`pending user authorization`).
- [ ] Claude findings dispositioned and affected checks rerun.
- [ ] Required live serial/PLC checks passed or explicitly dispositioned.
- [ ] Documentation and final acceptance agree with the completed implementation.

## D-096: automatic Format2 block number

Scope: Format2 encoder/decoder, runtime client request state, timeout/cancel isolation, raw codec,
tests, and docs.

Target contract: normal clients allocate a new 00..FF block number per wire request and accept only
the matching, strictly parsed response. Normal session configuration does not expose a fixed block
number; raw investigation remains explicitly separate.

Compatibility impact: public `ascii_block_number` and CLI `--block-no` are removed from normal
runtime configuration.

Acceptance criteria:

1. 00/01/FE/FF/wrap allocation is deterministic with one in-flight request.
2. Malformed/mismatched/late responses cannot complete the current or next request.
3. Retry uses a new number and non-Format2 configurations have no block field.

Progress: completed. `ProtocolConfig::ascii_block_number` and CLI `--block-no` are removed. Normal
clients allocate one byte per successfully encoded request, preserve the sequence across
reconfiguration, and can wrap only because the single-in-flight guard and completion path leave RX
clean. Raw codec use is explicit through `FrameCodecContext::format2(number)`.

- [x] Implementation completed in this repository.
- [x] Tests added or updated for every acceptance criterion.
- [x] Relevant checks passed and evidence recorded.
- [x] Codex self-review completed.
- [ ] Claude source review completed (`pending user authorization`).
- [ ] Claude findings dispositioned and affected checks rerun.
- [x] Required live serial/PLC checks passed or explicitly dispositioned (no new live check required;
  deterministic frame identity and state isolation are fully covered by codec/client tests, while
  existing Format2 hardware evidence remains unchanged).
- [ ] Documentation and final acceptance agree with the completed implementation.

## D-097: explicit canonical PLC profile

Scope: profile enum/parser/name helpers, protocol configuration, all codecs/clients/presets, CLI,
tests, and docs.

Target contract: only one of the eight canonical profiles is accepted; Unspecified and unknown enum
values never select a generic layout or reach transport.

Compatibility impact: profile omission remains invalid and the final constructor migration will
remove two-step generic config construction.

Acceptance criteria:

1. Unspecified, unknown underlying values, empty/unknown text, and reserved values fail pre-send.
2. Every canonical profile parses and maps exhaustively.
3. Presets and all command families do not infer or switch profiles.

Progress: `is_plc_profile_specified()` now performs exhaustive membership instead of `!=`, unknown
values fail centrally, and CLI already requires canonical profile text. Constructor-level required
profile and preset error-return design remain open.

- [ ] Implementation completed in this repository.
- [ ] Tests added or updated for every acceptance criterion.
- [ ] Relevant checks passed and evidence recorded.
- [ ] Codex self-review completed.
- [ ] Claude source review completed (`pending user authorization`).
- [ ] Claude findings dispositioned and affected checks rerun.
- [ ] Required live serial/PLC checks passed or explicitly dispositioned.
- [ ] Documentation and final acceptance agree with the completed implementation.

## D-098: explicit typed sum-check mode

Scope: protocol configuration, ASCII/Binary codecs, presets, clients, CLI/scripts, tests, and docs.

Target contract: configurable frames require `SumCheckMode::Enabled` or `Disabled`; no bool/default,
format-derived choice, retry, or automatic policy switch exists.

Compatibility impact: `bool sum_check_enabled`, one-argument named presets, CLI/script defaults, and
environment fallback are removed.

Acceptance criteria:

1. Omitted/unknown mode fails before encoding and both defined enum values are accepted.
2. Every named preset and executable configuration boundary requires a mode.
3. Enabled/Disabled encode and decode strictly without fallback or state-changing retry.

Progress: enum surface, invalid generic default, required preset argument, required CLI/script input,
central validation, and omission/unknown tests are implemented. Final typed ProtocolConfig
constructor and complete checksum negative-vector audit remain open.

- [ ] Implementation completed in this repository.
- [ ] Tests added or updated for every acceptance criterion.
- [ ] Relevant checks passed and evidence recorded.
- [ ] Codex self-review completed.
- [ ] Claude source review completed (`pending user authorization`).
- [ ] Claude findings dispositioned and affected checks rerun.
- [ ] Required live serial/PLC checks passed or explicitly dispositioned.
- [ ] Documentation and final acceptance agree with the completed implementation.

## D-099: explicit typed route selection

Scope: route configuration types, protocol presets, frame/command codecs, clients, CLI, examples,
tests, generated API reference, migration notes, and user documentation.

Target contract: a protocol/session configuration contains either an explicit fieldless
`HostStationRoute` or an explicit frame-specific multidrop route. An omitted route is invalid. Host-station fixed
header values are not public inputs, frame/format selection does not infer a route, and timeout or
decode failure never changes the selected route.

Compatibility impact: the mutable `RouteConfig` aggregate and implicit HostStation default are
removed. Callers and presets must supply one of the typed route values explicitly.

Acceptance criteria:

1. Omitted routes fail before request encoding or transport and unknown route enum construction is
   not part of the public API.
2. HostStation has no mutable station/network/PC/module/self-station inputs and always resolves to
   the canonical connected-station header.
3. Multidrop remains a distinct explicit type; every preset, CLI path, example, and package
   consumer selects a route without frame-derived or station-value-derived fallback.
4. Timeout, NAK, parse failure, and no-response paths do not retry with another route.

Progress: implementation completed. `RouteConfig {}` is invalid; valid construction uses
`HostStationRoute`, `C1MultidropRoute`, `C2MultidropRoute`, `C3MultidropRoute`,
`C4MultidropRoute`, or `E1Route`. Inactive frame fields do not exist on C1/C2/C3 types, wrong-frame
route types are rejected, every C4 preset requires a route, and CLI route selection is explicit.
D-101..D-103 remain separate for typed PC/module/self-station selectors and constraints; D-101 is
now implemented below.

- [x] Implementation completed in this repository.
- [x] Tests added or updated for every acceptance criterion.
- [x] Relevant checks passed and evidence recorded.
- [x] Codex self-review completed.
- [ ] Claude source review completed (`pending user authorization`).
- [ ] Claude findings dispositioned and affected checks rerun.
- [x] Required live serial/PLC checks passed or explicitly dispositioned (no new live check required;
  construction, pre-transport validation, fixed HostStation bytes, and no-fallback behavior are
  deterministic API/codec/client contracts).
- [ ] Documentation and final acceptance agree with the completed implementation.

## D-100: mandatory frame-specific multidrop station and network

Scope: 1C/2C/3C/4C route types, ASCII/binary frame encoders and decoders, client stream isolation,
presets, CLI/scripts, tests, generated API reference, migration notes, and user documentation.

Target contract: every multidrop request explicitly supplies its active station; 3C/4C additionally
supplies network. Explicit zero is valid, omission is not converted to zero, out-of-range/special
generic values are rejected before encoding, and only a complete matching route response can finish
the in-flight request.

Compatibility impact: the generic multidrop aggregate and station/network defaults are removed.
CLI users must provide `--station` for multidrop and `--network` for 3C/4C multidrop.

Acceptance criteria:

1. C1/C2 have a mandatory station and no network input; C3/C4 have mandatory station and network.
2. Explicit zero and ordinary boundaries encode unchanged; station above `0x1F`, generic `0xFF`,
   network above `0xEF`, generic `0xFE`, negative/overflow/nonnumeric external inputs fail before TX.
3. ASCII route fields are strict hexadecimal and binary/ASCII response identities are compared
   with the configured station/network (plus the other present route fields).
4. A complete foreign route response is consumed without completing the request; the matching
   response can still complete it, including after timeout and route reconfiguration.
5. No timeout, NAK, parse error, mismatch, or no-response path changes station/network or retries a
   discovery route.

Progress: implementation completed. Frame-specific route constructors require active values and
store station/network wider than wire width until validation, preventing narrowing before range
checks. The decoder strictly parses ASCII route text, compares ASCII/binary route identity, and the
client discards complete mismatches while waiting. CLI/scripts require the same active inputs.

- [x] Implementation completed in this repository.
- [x] Tests added or updated for every acceptance criterion.
- [x] Relevant checks passed and evidence recorded.
- [x] Codex self-review completed.
- [ ] Claude source review completed (`pending user authorization`).
- [ ] Claude findings dispositioned and affected checks rerun.
- [x] Required live serial/PLC checks passed or explicitly dispositioned (no new live check required;
  frame layout, strict parsing, identity comparison, and client stream isolation are covered by
  deterministic vectors and simulated client state; existing hardware support evidence is not
  changed or promoted).
- [ ] Documentation and final acceptance agree with the completed implementation.

## D-101: mandatory frame-specific PC target

Scope: 3C/4C multidrop and 1E route types, HostStation and 1C/2C inactive fields, ASCII/binary
encoders and decoders, client response isolation, presets, CLI/scripts, tests, generated API
reference, migration notes, and user documentation.

Target contract: HostStation fixes PC `0xFF` internally and exposes no PC input; 1C/2C route types
expose no PC input; 3C/4C and 1E routes require a frame-specific typed PC target. Special 3C/4C
wire values are selected by named constructors instead of being accepted as ordinary numbered
targets. No omission, parse failure, overflow, or timeout changes or supplies a PC target.

Compatibility impact: the former optional raw `pc_no=0xFF` argument is removed. 3C/4C multidrop
callers must supply `C34PcTarget`, and 1E callers must supply `E1PcTarget`, including when the
intended value is the connected-station `0xFF` selector. CLI/scripts must supply `--pc-target` for
3C/4C/1E non-host routes.

Acceptance criteria:

1. 3C/4C require `C34PcTarget`; 1E requires `E1PcTarget`; HostStation and 1C/2C have no caller-settable
   PC target.
2. 3C/4C numbered targets accept `0x01..0x78`; `0x7D`, `0x7E`, `0xFE`, and `0xFF` require their
   named selectors in the typed C++ API. 1E numbers accept `0x01..0x40`, while `0xFF` requires its
   connected selector. A raw numeric CLI special value is converted to that canonical selector at
   the external boundary rather than retained as an ordinary numbered target.
3. Missing, zero, negative, nonnumeric, and values at or above 256 fail before request bytes or OS
   transport; no value is narrowed, masked, wrapped, or replaced by `0xFF` before validation.
4. 3C/4C response PC fields are parsed strictly and compared with the configured target. A complete
   foreign-PC response is consumed without completing the active request, and no error path retries
   with another PC. The 1E response format contains no echoed PC field, so it has no response PC
   identity to compare.
5. Presets preserve the mandatory route type; CLI and validation scripts require the same active
   PC input and reject PC input for host/1C/2C routes.

Progress: implementation and deterministic tests are complete. The public route constructors use
typed targets, wide external values survive until validation, request encoding checks the target
before producing bytes, strict 3C/4C response identity includes PC, and CLI/scripts require the
active target and canonicalize raw special values. `special_fe()` intentionally uses a value-based
neutral name because the reviewed local material establishes the supported wire value but not one
universally valid topology meaning.

- [x] Implementation completed in this repository.
- [x] Tests added or updated for every acceptance criterion.
- [x] Relevant checks passed and evidence recorded.
- [x] Codex self-review completed.
- [ ] Claude source review completed (`pending user authorization`).
- [ ] Claude findings dispositioned and affected checks rerun.
- [x] Required live serial/PLC checks passed or explicitly dispositioned (no new live check required;
  mandatory construction, range validation, request bytes, response identity, and no-fallback
  behavior are deterministic contracts; existing hardware support evidence is unchanged).
- [ ] Documentation and final acceptance agree with the completed implementation.

## Verification evidence

- Build/test command: `run_ci.bat --build-dir build_win`, passed 2026-07-11.
- Automated results: CMake/Ninja full host build passed; `codec_tests`,
  `standard_header_consumer`, and `cli_serial_config_tests` all passed (3/3); Markdown links and the
  generated API-reference drift check passed.
- PlatformIO results: after adding the existing
  `C:\Users\GMKtek\.platformio\penv\Scripts` directory to the process PATH, all nine configured
  native/RP2040/ESP32-C3/Arduino Mega environments and the packed-package consumers passed. The
  initial PATH-only lookup failure was an environment discovery issue, not a build failure.
- Script checks: all four Linux CLI scripts passed `bash -n`; the PowerShell password recheck script
  parsed successfully; `git diff --check` passed.
- Codex self-review covered the actual diff, public constructor/enum surface, integer truncation,
  validation-before-open order, Windows DCB and POSIX termios mappings, path lifetime/termination,
  CLI omission and unknown-value behavior, RS-485 separation, examples, scripts, docs, and generated
  API reference. It found and corrected uint8 truncation, stale CLI wrappers, POSIX string-view path
  termination, and invalid-reconfiguration state loss before this evidence was marked complete.
- The D-093 onward partial review additionally covered invalid default state, exhaustive enum
  membership, sum-check encode/decode call sites, named-preset signatures, CLI/environment omission,
  MCU/package consumers, and zero output size on frame-encode validation failure. D-096 and final
  tagged protocol/route types are explicitly still open and were not marked complete.
- D-096 evidence covers explicit raw-context rejection/acceptance, strict hexadecimal parsing,
  mismatched metadata, automatic request values 00 through FF and wrap, stale response followed by
  the correct response, cancellation, timeout followed by a late response, single-in-flight state,
  and absence of the former public field/CLI option. Host CI and all nine PlatformIO plus packed
  package consumers passed after the final implementation.
- D-099 evidence covers the invalid omitted route, fieldless HostStation type and canonical fixed
  header, explicit Multidrop type, required preset route argument, required CLI `--route`, rejection
  of host plus station input, and removal of station-derived route selection. Host CI, all
  PlatformIO environments, and both packed-package consumers passed after the route API change.
- D-100 evidence covers non-default-constructible frame route types, required station/network,
  explicit zero, normal boundaries, overflow and special-value rejection, wrong-frame types, strict
  ASCII route text, ASCII/binary mismatch metadata, foreign response discard, matching response
  completion, and timeout/reconfiguration late-response isolation. Host CI, all configured
  PlatformIO environments, and both packed-package consumers passed after the final D-100 changes.
- D-101 evidence covers mandatory typed C++ targets; HostStation/1C/2C inactive-field absence;
  numbered and special-selector boundaries for 3C/4C and 1E; raw CLI canonicalization; missing,
  zero, negative, nonnumeric, and overflow rejection; no request bytes or client TX on invalid
  targets; state-changing command frame rejection; strict ASCII and binary foreign-PC identity;
  and absence of PC fallback. Host CI passed 3/3, all configured PlatformIO environments passed,
  both packed-package consumers passed, scripts parsed, Markdown links passed, and the generated
  API reference matched after the D-101 implementation.
- Physical serial/PLC communication: not executed in this implementation batch.
- Claude review: not executed; explicit user authorization is required for a future review batch.
