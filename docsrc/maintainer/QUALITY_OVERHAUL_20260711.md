# MC Protocol Serial C++ quality overhaul

Status: complete; final acceptance recorded 2026-07-18
Original implementation branch: `quality/2026-07-overhaul` (merged)
Authoritative cross-library decisions: archived workspace record `omittable_configuration_decisions_20260711.md`

This record preserves the repository-specific implementation contract and evidence. A checked item
means evidence exists. All decisions documented here have completed final acceptance; the approved
live-hardware evidence for D-087 through D-092 is recorded in the repository-root
[TODO.md](../../TODO.md).

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
- [x] Claude source review completed (`CLAUDE-MCS-20260712-01`; reviewed through `ec7f6b9`).
- [x] Claude findings dispositioned and affected checks rerun; all four findings were accepted and corrected.
- [x] Required live serial/PLC checks passed or explicitly dispositioned (no live check required; path lifetime, NUL validation, and pre-open ordering are deterministic API/backend properties and make no claim about a physical serial device).
- [x] Documentation, migration notes, changelog, and source examples agree with the implementation.
- [x] Final acceptance criteria verified and the item marked complete (2026-07-18).

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
- [x] Claude source review completed (`CLAUDE-MCS-20260712-01`; reviewed through `ec7f6b9`).
- [x] Claude findings dispositioned and affected checks rerun; all four findings were accepted and corrected.
- [x] Required live serial/PLC checks passed or explicitly dispositioned (no live check required; required-value parsing, zero rejection, and absence of baud fallback are deterministic pre-open properties).
- [x] Documentation, migration notes, changelog, and source examples agree with the implementation.
- [x] Final acceptance criteria verified and the item marked complete (2026-07-18).

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
- [x] Claude source review completed (`CLAUDE-MCS-20260712-01`; reviewed through `ec7f6b9`).
- [x] Claude findings dispositioned and affected checks rerun; all four findings were accepted and corrected.
- [x] Required live serial/PLC checks passed or explicitly dispositioned (no live check required; public type shape, mode/width validation, and Windows/POSIX branch removal are covered before OS handle access and make no signal-quality claim).
- [x] Documentation, migration notes, changelog, and source examples agree with the implementation.
- [x] Final acceptance criteria verified and the item marked complete (2026-07-18).

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
- [x] Claude source review completed (`CLAUDE-MCS-20260712-01`; reviewed through `ec7f6b9`).
- [x] Claude findings dispositioned and affected checks rerun; all four findings were accepted and corrected.
- [x] Required live serial/PLC checks passed or explicitly dispositioned (no live check required; accepted widths, invalid-width rejection, and backend mapping are deterministic configuration properties).
- [x] Documentation, migration notes, changelog, and source examples agree with the implementation.
- [x] Final acceptance criteria verified and the item marked complete (2026-07-18).

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
- [x] Claude source review completed (`CLAUDE-MCS-20260712-01`; reviewed through `ec7f6b9`).
- [x] Claude findings dispositioned and affected checks rerun; all four findings were accepted and corrected.
- [x] Required live serial/PLC checks passed or explicitly dispositioned (no live check required; enum membership, Windows DCB/POSIX termios mapping, CLI normalization, and no-fallback behavior are deterministic; actual PLC parity compatibility is outside this API decision).
- [x] Documentation, migration notes, changelog, and source examples agree with the implementation.
- [x] Final acceptance criteria verified and the item marked complete (2026-07-18).

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
- [x] Claude source review completed (`CLAUDE-MCS-20260712-01`; reviewed through `ec7f6b9`).
- [x] Claude findings dispositioned and affected checks rerun; all four findings were accepted and corrected.
- [x] Required live serial/PLC checks passed or explicitly dispositioned (no live check required; enum membership, DCB/termios mapping, and separation from RS-485 direction control are deterministic; this disposition does not claim that a particular adapter is wired for RTS/CTS).
- [x] Documentation, migration notes, changelog, and source examples agree with the implementation.
- [x] Final acceptance criteria verified and the item marked complete (2026-07-18).

## D-093: explicit frame family

Scope: `ProtocolConfig`, frame codec, clients, CLI, examples, tests, and docs.

Target contract: frame family is a required session choice; no C4 default or runtime fallback to a
different family exists. Unknown enum values and compiled-out families fail before encoding.

Compatibility impact: public `ProtocolConfig {}` construction and mutable frame assignment are
removed. Callers use the frame-tagged `c4_binary`, `ascii(AsciiFrameKind, ...)`, or `e1` factory.

Acceptance criteria:

1. Omitted and unknown frame values fail with zero encoded bytes.
2. Every defined frame is validated only against its supported code/format/profile combinations.
3. No timeout, PLC error, or decode error changes the configured frame.

Progress: completed. The immutable tagged factories make frame omission impossible, C4 Binary is
fixed by `c4_binary`, C1/C2/C3/C4 ASCII uses `AsciiFrameKind`, and 1E has its own factory. Unknown
ASCII frame values, compiled-out selections, and every invalid combination fail before encoding;
timeouts and decode failures do not mutate the configuration.

- [x] Implementation completed in this repository.
- [x] Tests added or updated for every acceptance criterion.
- [x] Relevant checks passed and evidence recorded.
- [x] Codex self-review completed.
- [x] Claude source review completed (`CLAUDE-MCS-20260712-01`; reviewed through `ec7f6b9`).
- [x] Claude findings dispositioned and affected checks rerun; all four findings were accepted and corrected.
- [x] Required live serial/PLC checks passed or explicitly dispositioned (no new live check required;
  constructor shape, validation ordering, encoded-byte absence, and immutable session state are
  deterministic and covered without claiming a physical PLC result).
- [x] Documentation and final acceptance agree with the completed implementation.

## D-094: explicit code mode

Scope: `ProtocolConfig`, frame codec, serial-data validation, clients, presets, CLI, tests, and docs.

Target contract: Binary or ASCII is selected explicitly and remains fixed for the session. Binary
configuration cannot accept or silently ignore ASCII-only input.

Compatibility impact: generic config no longer defaults to Binary.

Acceptance criteria:

1. Omitted/unknown mode and unsupported frame/mode combinations fail before encoding/open.
2. Binary requires eight serial data bits.
3. Public binary configuration has no ASCII-only settings and no mode fallback occurs.

Progress: completed. C4 Binary and C-family ASCII are separate construction paths, 1E requires an
explicit `CodeMode`, Binary has no ASCII-format input, serial-data validation still requires eight
bits for Binary, and no error path changes mode or retries in another mode.

- [x] Implementation completed in this repository.
- [x] Tests added or updated for every acceptance criterion.
- [x] Relevant checks passed and evidence recorded.
- [x] Codex self-review completed.
- [x] Claude source review completed (`CLAUDE-MCS-20260712-01`; reviewed through `ec7f6b9`).
- [x] Claude findings dispositioned and affected checks rerun; all four findings were accepted and corrected.
- [x] Required live serial/PLC checks passed or explicitly dispositioned (no new live check required;
  tagged API shape, serial validation, and no-fallback state are deterministic testable behavior).
- [x] Documentation and final acceptance agree with the completed implementation.

## D-095: explicit ASCII format

Scope: ASCII protocol configuration, codecs, clients, presets, CLI, tests, and docs.

Target contract: supported ASCII frames require an explicit Format1/2/3/4 selection; Binary and 1E
public configuration do not expose an irrelevant ASCII-format field.

Compatibility impact: generic config no longer defaults to Format3.

Acceptance criteria:

1. Omitted/unknown format fails for ASCII before encoding.
2. Frame-specific formats are enforced and the decoder remains fixed to the selected format.
3. Binary/1E types contain no inactive ASCII-format input.

Progress: completed. `ProtocolConfig::ascii` requires `AsciiFrameKind` plus `AsciiFormat`;
`c4_binary` accepts no ASCII input and `e1` accepts neither an ASCII frame/format nor a hidden
Format1 value. C1 Format2 and unknown formats reject before encoding, and the decoder remains fixed
to the selected format.

- [x] Implementation completed in this repository.
- [x] Tests added or updated for every acceptance criterion.
- [x] Relevant checks passed and evidence recorded.
- [x] Codex self-review completed.
- [x] Claude source review completed (`CLAUDE-MCS-20260712-01`; reviewed through `ec7f6b9`).
- [x] Claude findings dispositioned and affected checks rerun; all four findings were accepted and corrected.
- [x] Required live serial/PLC checks passed or explicitly dispositioned (no new live check required;
  public input absence and frame/format decoder selection are compile-time/codec properties).
- [x] Documentation and final acceptance agree with the completed implementation.

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
- [x] Claude source review completed (`CLAUDE-MCS-20260712-01`; reviewed through `ec7f6b9`).
- [x] Claude findings dispositioned and affected checks rerun; all four findings were accepted and corrected.
- [x] Required live serial/PLC checks passed or explicitly dispositioned (no new live check required;
  deterministic frame identity and state isolation are fully covered by codec/client tests, while
  existing Format2 hardware evidence remains unchanged).
- [x] Documentation and final acceptance agree with the completed implementation (2026-07-18).

## D-097: explicit canonical PLC profile

Scope: profile enum/parser/name helpers, protocol configuration, all codecs/clients/presets, CLI,
tests, and docs.

Target contract: only one of the eight canonical profiles is accepted; Unspecified and unknown enum
values never select a generic layout or reach transport.

Compatibility impact: profile omission is no longer representable through public default/two-step
configuration; every tagged factory and preset requires a profile argument.

Acceptance criteria:

1. Unspecified, unknown underlying values, empty/unknown text, and reserved values fail pre-send.
2. Every canonical profile parses and maps exhaustively.
3. Presets and all command families do not infer or switch profiles.

Progress: completed. Every public factory/preset requires the profile argument, canonical text is
required at the CLI boundary, exhaustive membership rejects `Unspecified` and unknown underlying
values, and all codec/client entry points validate before producing bytes or opening transport.

- [x] Implementation completed in this repository.
- [x] Tests added or updated for every acceptance criterion.
- [x] Relevant checks passed and evidence recorded.
- [x] Codex self-review completed.
- [x] Claude source review completed (`CLAUDE-MCS-20260712-01`; reviewed through `ec7f6b9`).
- [x] Claude findings dispositioned and affected checks rerun; all four findings were accepted and corrected.
- [x] Required live serial/PLC checks passed or explicitly dispositioned (no new live check required;
  canonical membership and pre-transport rejection are deterministic validation behavior).
- [x] Documentation and final acceptance agree with the completed implementation.

## D-098: explicit typed sum-check mode

Scope: protocol configuration, ASCII/Binary codecs, presets, clients, CLI/scripts, tests, and docs.

Target contract: configurable C frames require `SumCheckMode::Enabled` or `Disabled`; no
bool/default, format-derived choice, retry, or automatic policy switch exists. 1E has no sum-check
field and therefore exposes no sum-check option.

Compatibility impact: `bool sum_check_enabled`, one-argument named presets, CLI/script defaults, and
environment fallback are removed.

Acceptance criteria:

1. Omitted/unknown mode fails before encoding for configurable frames and both defined enum values
   are accepted; 1E rejects an inactive sum-check input at the CLI and has none in its C++ factory.
2. Every C-frame named preset and executable configuration boundary requires a mode.
3. Enabled/Disabled encode and decode strictly without fallback or state-changing retry.

Progress: completed. C-frame factories/presets require the enum; the 1E factory has no sum-check
parameter and the CLI rejects one. Positive, missing, corrupt, and extra-checksum vectors cover
Format1/2/3/4, C1, and C4 Binary. Enabled responses require and verify the checksum; Disabled
responses do not generate it or consume trailing checksum bytes as part of the current frame.

- [x] Implementation completed in this repository.
- [x] Tests added or updated for every acceptance criterion.
- [x] Relevant checks passed and evidence recorded.
- [x] Codex self-review completed.
- [x] Claude source review completed (`CLAUDE-MCS-20260712-01`; reviewed through `ec7f6b9`).
- [x] Claude findings dispositioned and affected checks rerun; all four findings were accepted and corrected.
- [x] Required live serial/PLC checks passed or explicitly dispositioned (no new live check required;
  checksum generation/parsing and no-retry state are deterministic codec/client behavior; existing
  hardware checksum evidence remains unchanged).
- [x] Documentation and final acceptance agree with the completed implementation.

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
`HostStationRoute`, `C1MultidropRoute`, the topology-specific 2C/3C/4C route types, or `E1Route`.
Inactive frame fields do not exist on frame-specific types, wrong-frame route types are rejected,
every preset requires a route, and CLI route selection is explicit. D-101..D-103 refine this route
contract with mandatory typed PC, destination-module, topology, and self-station inputs.

- [x] Implementation completed in this repository.
- [x] Tests added or updated for every acceptance criterion.
- [x] Relevant checks passed and evidence recorded.
- [x] Codex self-review completed.
- [x] Claude source review completed (`CLAUDE-MCS-20260712-01`; reviewed through `ec7f6b9`).
- [x] Claude findings dispositioned and affected checks rerun; all four findings were accepted and corrected.
- [x] Required live serial/PLC checks passed or explicitly dispositioned (no new live check required;
  construction, pre-transport validation, fixed HostStation bytes, and no-fallback behavior are
  deterministic API/codec/client contracts).
- [x] Documentation and final acceptance agree with the completed implementation (2026-07-18).

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
- [x] Claude source review completed (`CLAUDE-MCS-20260712-01`; reviewed through `ec7f6b9`).
- [x] Claude findings dispositioned and affected checks rerun; all four findings were accepted and corrected.
- [x] Required live serial/PLC checks passed or explicitly dispositioned (no new live check required;
  frame layout, strict parsing, identity comparison, and client stream isolation are covered by
  deterministic vectors and simulated client state; existing hardware support evidence is not
  changed or promoted).
- [x] Documentation and final acceptance agree with the completed implementation (2026-07-18).

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
- [x] Claude source review completed (`CLAUDE-MCS-20260712-01`; reviewed through `ec7f6b9`).
- [x] Claude findings dispositioned and affected checks rerun; all four findings were accepted and corrected.
- [x] Required live serial/PLC checks passed or explicitly dispositioned (no new live check required;
  mandatory construction, range validation, request bytes, response identity, and no-fallback
  behavior are deterministic contracts; existing hardware support evidence is unchanged).
- [x] Documentation and final acceptance agree with the completed implementation (2026-07-18).

## D-102: mandatory typed 4C destination module

Scope: 4C multidrop and HostStation route types, 1C/2C/3C/1E inactive fields, ASCII/binary
encoders and decoders, connected-only command validation, client response isolation, presets,
CLI/scripts, tests, generated API reference, migration notes, and user documentation.

Target contract: every 4C multidrop route contains an explicit `C4DestinationModule`. HostStation
fixes OwnStation `0x03FF/0x00` internally, while other frame-specific routes expose no
destination-module input. Known CPU meanings use typed selectors; configuration-dependent module
routes use an explicit I/O/station pair without profile-derived fallback.

Compatibility impact: the defaulted raw module I/O/station constructor arguments are removed.
4C multidrop callers must supply a typed target even when OwnStation is correct. CLI/scripts must
supply `--module-target`; the option is rejected for every other route/frame.

Acceptance criteria:

1. Neither `C4StandardMultidropRoute` nor `C4MnMultidropRoute` can be constructed without
   `C4DestinationModule`; HostStation and 1C/2C/3C/1E route types have no caller-settable
   destination-module field.
2. OwnStation, Multiple CPU 1..4, and redundant control/standby/system A/system B selectors encode
   their defined values. Explicit targets preserve all 16-bit I/O and 8-bit station values.
3. Missing, malformed, negative, nonnumeric, I/O above `0xFFFF`, station above `0xFF`, and invalid
   known-selector indices fail before request bytes or OS transport; no value is narrowed, wrapped,
   masked, or replaced with OwnStation.
4. RemoteHead numeric aliases are not presented as Multiple CPU semantic proof. A
   configuration-dependent route uses the explicit selector and remains subject to hardware/profile
   evidence outside the wire-width validator.
5. ASCII and binary response destination-module fields are compared with the configured route. A
   complete foreign-module response is consumed without completing the request, and no error path
   retries with another module.
6. Connected-station-only helpers accept HostStation or the explicit OwnStation selector and reject
   other module selectors before request encoding. Routed read/write commands remain valid for
   non-own targets when the selected command/profile supports them.

Progress: implementation and deterministic tests are complete. Public construction, wide-value
validation, CLI/scripts, pre-encode/no-TX behavior, selector values, connected-only command routing,
routed reads, and ASCII/binary module identity are covered. RemoteHead aliases remain raw vocabulary
constants and do not acquire a convenient typed selector without configuration evidence.

- [x] Implementation completed in this repository.
- [x] Tests added or updated for every acceptance criterion.
- [x] Relevant checks passed and evidence recorded.
- [x] Codex self-review completed.
- [x] Claude source review completed (`CLAUDE-MCS-20260712-01`; reviewed through `ec7f6b9`).
- [x] Claude findings dispositioned and affected checks rerun; all four findings were accepted and corrected.
- [x] Required live serial/PLC checks passed or explicitly dispositioned (no new live check required;
  selector construction, wire-width validation, request bytes, response identity, command routing,
  and no-fallback behavior are deterministic contracts; configuration-dependent module support
  remains explicit and is not promoted to verified hardware support).
- [x] Documentation and final acceptance agree with the completed implementation (2026-07-18).

## D-103: mandatory typed m:n self-station topology

Scope: 2C/3C/4C route types, HostStation/1C/1E inactive fields, ASCII/binary encoders and decoders,
client response isolation, presets, CLI/scripts, tests, generated API reference, migration notes,
and user documentation.

Target contract: normal/1:n and m:n are different route types. A standard route contains no
self-station input and encodes zero internally. An m:n route requires a typed `SelfStationNo`
between 0 and 31, including an explicit zero when zero is assigned. No bool/number state pair,
omission fallback, topology inference, narrowing, or retry with another self-station exists.

Compatibility impact: `C2MultidropRoute`, `C3MultidropRoute`, `C4MultidropRoute`,
`self_station_enabled`, and raw optional `self_station_no` construction are removed. Callers select
`C2/C3/C4StandardMultidropRoute` or `C2/C3/C4MnMultidropRoute`; the latter requires
`SelfStationNo`. CLI/scripts require `--topology standard|mn` for 2C/3C/4C multidrop, and only
`mn` accepts and requires `--self-station`.

Acceptance criteria:

1. Standard 2C/3C/4C route types expose no self-station input; m:n counterparts cannot be
   constructed without `SelfStationNo`; HostStation, 1C, and 1E expose neither topology nor
   self-station input.
2. `SelfStationNo` accepts explicit 0 through 31. Missing, negative, nonnumeric, 32, and wider
   values fail before request bytes or OS transport without narrowing, masking, wrapping, or
   conversion to standard topology.
3. Standard and m:n-zero requests encode the same self-station wire value while remaining distinct
   caller intent. 2C, 3C, and ASCII/binary 4C encode all valid values unchanged.
4. CLI/scripts require topology only for 2C/3C/4C multidrop. Standard rejects self-station; m:n
   requires it; all other route/frame combinations reject both options.
5. ASCII self-station text is parsed strictly and ASCII/binary response identity includes the
   configured self-station. A complete foreign-source response is consumed without completing the
   request, and the matching response can still complete it.
6. Timeout, NAK, mismatch, malformed input, or no response never changes topology/self-station or
   retries another value. Hardware assignment and station-count rules remain explicit user
   configuration rather than library inference.

Progress: implementation and deterministic tests are complete. Public topology types, mandatory
typed m:n value, explicit zero, upper and invalid boundaries, C1/1E inactive-field absence,
2C/3C/4C wire bytes, CLI omission/combination validation, strict foreign self-station identity, and
client discard-then-match behavior are covered. User documentation states that the library cannot
infer C24 station assignment or configuration-wide station-count constraints.

- [x] Implementation completed in this repository.
- [x] Tests added or updated for every acceptance criterion.
- [x] Relevant checks passed and evidence recorded.
- [x] Codex self-review completed.
- [x] Claude source review completed (`CLAUDE-MCS-20260712-01`; reviewed through `ec7f6b9`).
- [x] Claude findings dispositioned and affected checks rerun; all four findings were accepted and corrected.
- [x] Required live serial/PLC checks passed or explicitly dispositioned (no new live check required;
  type construction, width validation, frame bytes, response identity, stream isolation, and
  no-fallback behavior are deterministic contracts; PLC topology assignment is documented without
  being promoted to verified hardware evidence).
- [x] Documentation and final acceptance agree with the completed implementation (2026-07-18).

## D-104: three-second response deadline and independent 1E monitoring timer

Scope: `TimeoutConfig`, `E1MonitoringTimer`, frame validation/encoding, async/sync clients, CLI raw
and normal request loops, high-level presets, Linux/PowerShell scripts, Remote RESET completion,
tests, generated API reference, migration notes, and user documentation.

Target contract: the omitted host communication response timeout is 3000 ms for every frame and
runtime path. It is one wrap-safe deadline from successful TX completion to a complete response and
is not restarted by received data. The PLC-side 1E monitoring timer is independent, defaults to
4000 ms (`0x0010`), and preserves only exact representable 250 ms units without rounding or
saturation. A command defined not to return a normal response completes on successful TX as
request-transmitted, not PLC-operation-success.

Compatibility impact: code relying on the former 5000 ms host default now receives 3000 ms unless
it supplies a valid explicit value. 1E callers that relied on response-timeout-derived rounding or
`0xFFFF` saturation must set `E1MonitoringTimer` independently. Remote RESET no longer waits for a
timeout or accepts a fabricated timeout-success result; its completion message describes request
transmission. Global-signal and transmission-sequence timeout paths now remain timeout failures.

Acceptance criteria:

1. `TimeoutConfig`, named presets, CLI, sync/async paths, and scripts use 3000 ms when the response
   timeout is omitted. Explicit 1..2147483647 ms values are preserved; zero, negative/nonnumeric
   external values, and larger unsigned values fail before frame bytes or serial open.
2. The response deadline begins only after successful TX completion, compares correctly across a
   32-bit monotonic-clock wrap, remains fixed after partial RX, and is separate from queue/open,
   retry, inter-byte, PLC processing, and application deadlines. An unsequenced timeout prevents
   another request until transport reset plus reconfiguration; the host sync wrapper closes its
   owned serial port. Format2 instead isolates its late response by block identity.
3. `E1MonitoringTimer` defaults to 4000 ms/`0x0010` independently of a 3000 ms host timeout.
   Explicit 0, 250, and maximum 16383750 ms encode unchanged; non-250 ms values and overflow fail
   before encoding without rounding, truncation, or saturation.
4. CLI exposes `--e1-monitoring-timer-ms` only for 1E. Environment/PowerShell wrappers preserve
   omission rather than injecting 5000/8000 ms response defaults and reject the E1-only option on
   other frames.
5. Remote RESET completes immediately after successful TX with an explicit request-transmitted,
   PLC-state-unconfirmed result. Transport failure remains failure. It does not wait for or use the
   response timeout as a success condition.
6. Commands not proven to be no-normal-response operations do not convert timeout to success.
   Timeout, partial RX, malformed response, PLC error, or no response does not retry or alter either
   timeout/timer value.

Progress: implementation and deterministic tests are complete. Defaults, explicit boundaries,
pre-encode/pre-open rejection, 1E ASCII/binary request fields, response/monitoring independence,
clock wrap, fixed total deadline after partial RX, Remote RESET transmission completion, transport
failure, non-RESET timeout behavior, unsequenced post-timeout reuse blocking, host-port closure
policy, and Format2 identity-based reuse are covered.

- [x] Implementation completed in this repository.
- [x] Tests added or updated for every acceptance criterion.
- [x] Relevant checks passed and evidence recorded.
- [x] Codex self-review completed.
- [x] Claude source review completed (`CLAUDE-MCS-20260712-01`; reviewed through `ec7f6b9`).
- [x] Claude findings dispositioned and affected checks rerun; all four findings were accepted and corrected.
- [x] Required live serial/PLC checks passed or explicitly dispositioned (no new live check required;
  defaults, validation, deadline math, request fields, and TX-completion state are deterministic;
  no existing hardware compatibility evidence is changed or promoted).
- [x] Documentation and final acceptance agree with the completed implementation (2026-07-18).

## D-105: 250 ms inter-byte RX-inactivity timeout

Scope: `TimeoutConfig::inter_byte_timeout_ms`, frame validation, async/sync client receive paths,
CLI raw receive loop, high-level presets, Linux/PowerShell scripts, partial-frame state, tests,
generated API reference, migration notes, and user documentation.

Target contract: the omitted inter-byte timeout is 250 ms everywhere. It begins only after the
library receives response data and measures inactivity since the last delivered byte/chunk. Each
new chunk restarts only this deadline; it never restarts or extends the fixed total response
deadline. Explicit values are positive and wrap-safe, and expired partial data cannot be reused by
another request.

Compatibility impact: zero and values above 2147483647 ms are now configuration errors rather than
immediate or wrap-unsafe deadlines. A chunk delivered at the exact deadline is timed out before
decode. The FX5U soak wrapper no longer silently replaces omission with 1000 ms; users needing that
value must specify it. Unsequenced partial timeout requires transport reset and reconfiguration.

Acceptance criteria:

1. `TimeoutConfig`, named presets, async/sync clients, CLI, and scripts use 250 ms on omission.
   Explicit 1..2147483647 ms values are preserved; zero, negative/nonnumeric external values, and
   larger unsigned values fail before frame bytes or serial open.
2. No response data means only the total response deadline applies. After the first received
   byte/chunk, each additional chunk arriving before expiry restarts only the inter-byte deadline.
3. A chunk arriving at or after the inactivity deadline is rejected before append/decode. One-byte,
   multi-chunk, complete-frame, exact-boundary, and 32-bit clock-wrap behavior use the same
   comparison in async/MCU, sync, and CLI paths.
4. The 3000 ms total response deadline remains fixed after every chunk and wins when it expires
   first. Inter-byte timeout does not cover queueing, port open, TX, retry, PLC processing, or
   application deadlines.
5. Timeout discards incomplete decoder bytes. Unsequenced frames block subsequent requests until
   transport reset plus reconfiguration; the host sync wrapper closes its port. Format2 may remain
   usable because its block identity rejects the old response.
6. Documentation defines this as library-observed RX inactivity. When an OS read/UART callback
   contains multiple physical bytes, their internal physical spacing is not observable and is not
   presented as independently measured.

Progress: implementation and deterministic tests are complete. Default/preset/CLI/script values,
valid and invalid bounds, no-RX behavior, one-byte and multiple-chunk restarts, complete response,
exact expiry, 32-bit wrap, fixed total deadline, partial-state clearing, transport-reset gating,
and Format2 late-response identity are covered.

- [x] Implementation completed in this repository.
- [x] Tests added or updated for every acceptance criterion.
- [x] Relevant checks passed and evidence recorded.
- [x] Codex self-review completed.
- [x] Claude source review completed (`CLAUDE-MCS-20260712-01`; reviewed through `ec7f6b9`).
- [x] Claude findings dispositioned and affected checks rerun; all four findings were accepted and corrected.
- [x] Required live serial/PLC checks passed or explicitly dispositioned (no new live check required;
  defaults, value validation, clock math, chunk state, and timeout state transitions are
  deterministic; physical adapter gap characteristics remain user configuration).
- [x] Documentation and final acceptance agree with the completed implementation (2026-07-18).

## D-106: mandatory Remote RUN conflict and clear policies

Scope: `RemoteOperationMode`, `RemoteRunClearMode`, command encoder, async and host-sync clients,
CLI positional arguments, result status, transport/reset state, tests, generated API reference,
migration notes, and user documentation.

Target contract: Remote RUN requires an explicit conflict policy and clear scope at every public
entry point. The library never infers non-forced execution or no-clear. After transmission starts,
failure to obtain a trustworthy result is distinguishable as an unknown PLC outcome and never
causes an automatic retry.

Compatibility impact: `PosixSyncClient::remote_run()` and `remote_run(mode)` no longer compile.
CLI `remote-run` with zero or one policy is rejected before serial open. Code that treated a
post-transmission timeout as proof that RUN did not occur must handle
`StatusCode::OperationOutcomeUnknown` and inspect PLC state before deciding whether to retry.

Acceptance criteria:

1. Encoder, async client, host-sync client, and CLI require both typed policies. The two conflict
   modes and three clear modes remain meaning-bearing enums rather than bools or magic numbers.
2. Missing either or both CLI policies, empty/unknown text, extra arguments, and unknown enum
   values fail before serial open, frame bytes, or client TX. No default or fallback is applied.
3. All six policy combinations produce identical command data through binary/ASCII encoding and
   the shared sync/async/CLI path. Do-not-clear is accepted only when explicitly selected.
4. Unsupported frame/profile combinations and invalid policy values return a pre-send error with
   zero encoded size and no active request.
5. Once Remote RUN transmission starts, transport failure, timeout, post-TX cancellation, or an
   unconfirmable response returns `OperationOutcomeUnknown`; an explicit PLC error remains a
   confirmed PLC response. Host sync closes the port when the outcome is unknown.
6. Remote RUN is issued at most once. An unknown outcome produces no internal retry, clears the
   pending frame, and requires transport reset/reconfiguration for unsequenced frames; Format2
   continues to isolate late responses by block identity.

Progress: implementation and deterministic tests are complete. Sync omission is a compile-time
failure; CLI omission/unknown/extra input fails before open; all six policies, binary/ASCII wire
data, unknown enums, unsupported 1E, zero output, async transport failure, timeout, post-TX cancel,
unknown outcome, no retry, reset gating, and existing success roundtrips are covered.

- [x] Implementation completed in this repository.
- [x] Tests added or updated for every acceptance criterion.
- [x] Relevant checks passed and evidence recorded.
- [x] Codex self-review completed.
- [x] Claude source review completed (`CLAUDE-MCS-20260712-01`; reviewed through `ec7f6b9`).
- [x] Claude findings dispositioned and affected checks rerun; all four findings were accepted and corrected.
- [x] Required live serial/PLC checks passed or explicitly dispositioned (no new live check required;
  argument presence, enum validation, command bytes, retry absence, result classification, and
  state transitions are deterministic; no claim is made about a particular PLC's resulting state).
- [x] Documentation and final acceptance agree with the completed implementation (2026-07-18).

## D-107: mandatory Remote PAUSE conflict policy

Scope: `RemoteOperationMode`, Remote PAUSE command encoder, async and host-sync clients, CLI
positional arguments and parser aliases, result status, transport/reset state, tests, generated API
reference, migration notes, and user documentation.

Target contract: Remote PAUSE requires one explicit conflict policy at every public entry point.
The library never infers non-forced execution, accepts a magic-number substitute, escalates to
forced execution after failure, or retries an unconfirmed PAUSE.

Compatibility impact: `PosixSyncClient::remote_pause()` no longer compiles. CLI `remote-pause`
without exactly one `no-force|force` argument is rejected before serial open. The former `normal`,
`safe`, `1`, `0001`, `3`, and `0003` CLI aliases are removed. Post-transmission failure may now
return `StatusCode::OperationOutcomeUnknown` instead of a generic timeout/transport status.

Acceptance criteria:

1. Encoder, async client, host-sync client, and CLI require one typed conflict policy. Both enum
   values remain explicit and there is no no-argument overload or bool/magic-number API.
2. Missing, empty/unknown, alias, numeric, or extra CLI input and unknown enum values fail before
   serial open, frame bytes, or client TX. No non-forced fallback is applied.
3. Both policies produce identical command data through binary/ASCII encoding and the shared
   sync/async/CLI path. The documentation defines force as remote-operation-source conflict
   handling, not output retention.
4. Unsupported frame/profile combinations and invalid policies return a pre-send error with zero
   encoded size and no active request.
5. Once PAUSE transmission starts, transport failure, timeout, post-TX cancellation, or an
   unconfirmable response returns `OperationOutcomeUnknown`; a PLC error remains a confirmed error
   and does not trigger a forced retry.
6. PAUSE is issued at most once. Unknown outcome clears the pending frame and requires transport
   reset/reconfiguration for unsequenced frames; Format2 retains block-identity isolation.

Progress: implementation and deterministic tests are complete. Sync omission is a compile-time
failure; exact CLI input is checked before open; both policies, binary/ASCII data, unknown enums,
unsupported 1E, zero output, success, confirmed PLC error, transport failure, timeout, post-TX
cancel, unknown outcome, no retry/escalation, and reset gating are covered.

- [x] Implementation completed in this repository.
- [x] Tests added or updated for every acceptance criterion.
- [x] Relevant checks passed and evidence recorded.
- [x] Codex self-review completed.
- [x] Claude source review completed (`CLAUDE-MCS-20260712-01`; reviewed through `ec7f6b9`).
- [x] Claude findings dispositioned and affected checks rerun; all four findings were accepted and corrected.
- [x] Required live serial/PLC checks passed or explicitly dispositioned (no new live check required;
  argument presence, enum validation, command bytes, retry/escalation absence, result
  classification, and state transitions are deterministic; no PLC resulting state is claimed).
- [x] Documentation and final acceptance agree with the completed implementation (2026-07-18).

## D-108: explicit Word/DWord sparse-access types

Scope: generic and link-direct random read/write and monitor item types, codec and response parser,
async and host-sync clients, CLI syntax, examples, tests, generated API reference, migration notes,
and user documentation.

Target contract: sparse access width is selected by the public type and output span, never inferred
from the device code or a defaulted boolean. Generic native commands keep Word and DWord items and
results separate. Link-direct sparse access exposes only its supported Word width.

Compatibility impact: `RandomReadItem`, `RandomReadSpec`, public `double_word` members, ambiguous
`random_read`/monitor scalar overloads, and link-direct width switches are removed. Word write
values are `uint16_t`; DWord values are `uint32_t`. CLI sparse reads require `word:` or `dword:`,
and DWord writes use `random-write-dwords`.

Acceptance criteria:

1. Generic random read/write and monitor APIs use distinct Word and DWord item/spec types, separate
   request spans, and separate `uint16_t`/`uint32_t` result spans. No public bool, device-code
   inference, narrowing, masking, alias, or fallback selects width.
2. Word writes preserve `0` and `0xFFFF`; DWord writes preserve `0x00010000` and `0xFFFFFFFF`.
   Mixed requests encode Word items followed by DWord items and decode each group into the matching
   typed output without losing request/result mapping.
3. Devices that require the DWord route, including `LZ`, are rejected through Word item types and
   accepted only through an applicable DWord path. Long-device and profile restrictions remain
   explicit and are not bypassed by a width conversion.
4. 1C and 1E restrictions reject unsupported DWord random-write/monitor requests before client TX;
   1E random read remains unsupported. Invalid requests leave no pending frame or active request.
5. Link-direct random read, random write, and monitor expose only Word item/value/result types. No
   DWord option or `double_word` member remains in their public surface.
6. CLI sparse reads require exact `word:DEVICE` or `dword:DEVICE` input. Word and DWord writes use
   separate commands, with Word overflow rejected before serial open.
7. Source examples, user guidance, generated API reference, migration notes, and changelog use only
   the explicit-width contract.

Progress: implementation and deterministic tests are complete. Boundary values, mixed random and
monitor decoding, D/LZ width selection, 1C/1E pre-send rejection, link-direct Word-only types, CLI
width syntax, output-buffer preflight, count-overflow rejection, and removal of former public names
are covered. All repository verification below passed.

- [x] Implementation completed in this repository.
- [x] Tests added or updated for every acceptance criterion.
- [x] Relevant checks passed and evidence recorded.
- [x] Codex self-review completed.
- [x] Claude source review completed (`CLAUDE-MCS-20260712-01`; reviewed through `ec7f6b9`).
- [x] Claude findings dispositioned and affected checks rerun; all four findings were accepted and corrected.
- [x] Required live serial/PLC checks passed or explicitly dispositioned (no new live check required;
  type separation, exact command bytes, response decoding, validation order, and no-TX behavior are
  deterministic, and no target PLC value or capability is claimed).
- [x] Documentation and final acceptance agree with the completed implementation.

## D-109: mandatory random-write values and unknown outcomes

Scope: generic and link-direct random Word/DWord/Bit item types, high-level specs and helpers,
codec validation, asynchronous and host-sync clients, CLI parsing, examples, tests, generated API
reference, migration notes, and user documentation.

Target contract: every sparse write target and value are supplied at the same construction
boundary. Explicit Word/DWord zero and `BitValue::Off` are valid values; omission, empty input,
unknown bit values, and partially valid item lists never become zero/OFF writes. Once transmission
has started, an unconfirmed result is outcome-unknown and is never retried automatically.

Compatibility impact: `RandomWriteWordItem`, `RandomWriteDWordItem`, `RandomWriteBitItem`, their
high-level specs, and link-direct random-write items are no longer aggregates with public default
construction. Code that default-constructs an item and later assigns only some fields must migrate
to the device-and-value constructor. CLI entries without `=VALUE` are rejected before serial open.

Acceptance criteria:

1. Public generic item/spec types for Word, DWord, and Bit random writes require device and value in
   one constructor and are not default constructible. Link-direct random-write items follow the
   same rule. Explicit zero/OFF construction remains valid.
2. Word values remain exactly 16-bit, DWord values exactly 32-bit, and Bit values must be exactly
   `BitValue::Off` or `BitValue::On`; unknown bit enum values are rejected before frame generation.
3. A request containing one invalid item is rejected atomically before transmission. The library
   does not filter the invalid item and send the remaining writes.
4. Single-item and many-item high-level helpers require explicit values. No overload, builder,
   aggregate initialization, or internal fallback supplies zero/OFF for a caller.
5. CLI generic and link-direct random writes require a nonempty `DEVICE=VALUE` for every item,
   reject Word overflow and Bit values other than `0`/`1`, and complete all parsing before opening
   the serial device. Explicit `=0` reaches normal connection handling.
6. Word `0`/`0xFFFF`, DWord `0`/`0x00010000`/`0xFFFFFFFF`, and Bit OFF/ON retain their exact wire
   values without narrowing, masking, or missing-value substitution.
7. Cancellation during TX follows D-122 and waits for explicit physical completion/abort. If a
   state-changing request may have been transmitted, cancellation, transport failure, timeout, or
   another unconfirmed failure is `OperationOutcomeUnknown`, clears the pending frame, and never
   triggers an automatic retry. A confirmed PLC error remains `PlcError`.
8. Source examples, user guidance, generated API reference, migration notes, and changelog use only
   the mandatory-value contract and explain explicit zero/OFF and outcome-unknown handling.

Progress: implementation, deterministic tests, full repository/package verification, and final
diff self-review are complete. Constructor omission, explicit boundary values, atomic invalid-item
rejection, CLI pre-open parsing, unknown enums, success/confirmed PLC error, transport failure,
timeout, pre/post-TX cancellation, outcome-unknown classification, no retry, reset gating, feature-
disabled builds, generated documentation, and packaged consumers are covered.

- [x] Implementation completed in this repository.
- [x] Tests added or updated for every acceptance criterion.
- [x] Relevant checks passed and evidence recorded.
- [x] Codex self-review completed.
- [x] Claude source review completed (`CLAUDE-MCS-20260712-01`; reviewed through `ec7f6b9`).
- [x] Claude findings dispositioned and affected checks rerun; all four findings were accepted and corrected.
- [x] Required live serial/PLC checks passed or explicitly dispositioned (no new live check required;
  constructor availability, parser validation, exact wire values, no-TX behavior, outcome
  classification, reset gating, and retry absence are deterministic, and no PLC resulting state is
  claimed).
- [x] Documentation and final acceptance agree with the completed implementation.

## D-110: mandatory construction of public input types

Scope: public device and extended-file-register addresses; all read, write, control, user-frame,
buffer, random, monitor, and multi-block request/item types; link-direct and qualified-buffer
types; high-level specs/builders; codec and client validation; CLI, examples, tests, generated API
reference, migration notes, and user documentation.

Target contract: every public input object requires its semantic device, address, count/data,
value, target, state, channel, and requested change at its construction boundary. Omission never
becomes D0, address zero, zero/OFF, ReceivedSide, Ch1, or another valid command. Caller-supplied
D0, address zero, numeric zero, and `BitValue::Off` remain valid. Receive/result storage remains
default constructible.

Compatibility impact: default construction followed by member assignment and designated
initializers for non-aggregate request types are removed. Callers must use the required-field
constructors or validated high-level builders. `GlobalSignalControlRequest::turn_on` is replaced by
typed `BitValue value`. A serial-module mode-switch request must select at least one change.

Acceptance criteria:

1. `DeviceAddress`, extended-file-register addresses, every in-scope input request/item/block,
   link-direct/qualified input, and high-level request spec are not default constructible. Required
   values are visible in constructors or a validated builder boundary.
2. `CpuModelInfo`, `UserFrameRegistrationData`, `MultiBlockReadBlockResult`, and other receive or
   parse-result storage retain default construction and cannot accidentally become sendable input.
3. Explicit D0, address/block zero, numeric value zero, and `BitValue::Off` retain their exact
   meaning and wire representation. No constructor or parser interprets omission as those values.
4. Empty item/block/data containers, zero/invalid counts, unknown enums, and any invalid member of
   a multi-item request reject the complete request before transmission; no valid subset is sent.
5. Global-signal control requires an explicit valid target and `BitValue::Off`/`On`. Unknown target
   or state leaves the asynchronous client idle with no pending transmit frame.
6. Serial-module mode switching requires an explicit valid channel and at least one selected mode,
   transmission-setting, or speed change. All flags false leaves no pending transmit frame.
7. Internal pending storage uses explicit inert seeds without reintroducing a public default
   constructor or treating unused capacity as a request item.
8. The required-construction API compiles under the supported C++17 host, RP2040, ESP32-C3, and AVR
   profiles, including reduced and ultra-minimal feature configurations.
9. CLI, examples, user guidance, generated API reference, migration notes, and changelog contain no
   former default-construction, partial aggregate, `turn_on`, or implicit-D0 usage.

Progress: implementation, deterministic tests, full repository/package verification, generated
documentation, and final diff self-review are complete. Compile-time traits cover every in-scope
input and output exception. Empty/invalid containers, mixed valid/invalid items, explicit zero/OFF,
unknown control enums, and a no-change mode switch are covered with no-TX assertions.

- [x] Implementation completed in this repository.
- [x] Tests added or updated for every acceptance criterion.
- [x] Relevant checks passed and evidence recorded.
- [x] Codex self-review completed.
- [x] Claude source review completed (`CLAUDE-MCS-20260712-01`; reviewed through `ec7f6b9`).
- [x] Claude findings dispositioned and affected checks rerun; all four findings were accepted and corrected.
- [x] Required live serial/PLC checks passed or explicitly dispositioned (no new live check required;
  constructor availability, compile-time traits, validation order, exact request bytes, and no-TX
  behavior are deterministic, and no PLC capability or resulting device state is claimed).
- [x] Documentation and final acceptance agree with the completed implementation.

## D-122: paired RS-485 hooks and explicit TX completion status

Scope: `Rs485Hooks`, `set_rs485_hooks`, asynchronous request start/cancel/completion, host-sync
transport integration, CLI RTS direction control, MCU examples, tests, generated API reference,
migration notes, and user documentation.

Target contract: omitting the complete hook set is valid for RS-232 and hardware/driver-controlled
RS-485. Application-controlled RS-485 requires begin/end as one pair, retained with the same user
pointer for the request lifetime. TX completion always carries an explicit transport status.
Cancellation during TX is a request to stop, not proof that the UART is physically idle; completion
and direction release wait for explicit physical completion or abort notification.

Compatibility impact: one-sided hook sets and hook replacement while busy now fail. The one-argument
`notify_tx_complete(now_ms)` call no longer compiles. Callers must pass `ok_status()` after confirmed
physical TX completion or the actual failure/cancellation status after TX abort completes. Code that
expects `cancel()` during active TX to invoke completion immediately must wait for the explicit TX
notification.

Acceptance criteria:

1. Both callbacks null is accepted and performs no direction callback. Begin-only and end-only sets
   return `InvalidArgument`; a complete pair accepts a null or non-null user pointer.
2. Hook replacement while a request is busy returns `Busy`. Every invoked begin callback has exactly
   one matching end callback using the same installed pair and user; duplicate TX notification does
   not invoke end twice.
3. `notify_tx_complete` has no default status and one-argument use fails at compile time. Success,
   transport failure, and cancellation are always explicit at every library, CLI, host, example,
   test, and packaged-consumer call site.
4. Cancellation during TX leaves the request busy, output/callback storage alive, and end uninvoiced
   until the transport reports physical completion or abort. The notification invokes end once and
   then completes as cancelled or outcome-unknown according to whether a transmitted state-changing
   operation can be confirmed.
5. Cancellation after successful TX does not invoke end again. Unsequenced cancellation/failure
   requires transport reset; Format2 retains its block-identity isolation.
6. Host-sync close and TX failure paths close/abort the serial transport and deliver an explicit TX
   result so the async client and RS-485 direction lifecycle cannot remain pending.
7. User guidance states when hooks are omitted, why a complete pair is required, when `ok_status()`
   is valid, and why `cancel()` alone does not prove that the transceiver left transmit direction.

Progress: implementation and deterministic verification are complete. Pair validation, null user,
busy replacement, begin/end counts, explicit-status compile surface, TX-time cancellation, duplicate
notification, hookless transport failure, host-sync cleanup, existing post-TX outcome handling, all
host tests, and all 12 PlatformIO environments are covered.

- [x] Implementation completed in this repository.
- [x] Tests added or updated for every acceptance criterion.
- [x] Relevant checks passed and evidence recorded.
- [x] Codex self-review completed.
- [x] Claude source review completed (`CLAUDE-MCS-20260712-01`; reviewed through `ec7f6b9`).
- [x] Claude findings dispositioned and affected checks rerun; all four findings were accepted and corrected.
- [x] Required live serial/PLC checks passed or explicitly dispositioned (no new live check required;
  callback pairing, busy-state transitions, compile-time API shape, cancellation deferral, explicit
  status propagation, and transport-reset state are deterministic and claim no physical bus result).
- [x] Documentation and final acceptance agree with the completed implementation.

## MCS-CR-001: independent-review request-state and error-contract hardening

Scope: async request admission, all state-changing async and host-sync operations, receive failure
isolation, C24 recovery CLI input, tests, generated API reference, migration notes, and changelog.

Target contract: a rejected concurrent request cannot alter the active request. Once transmission
of any state-changing operation may have begun, an unconfirmed result is
`OperationOutcomeUnknown`, never a definite pre-send failure and never an automatic retry.
Unsequenced receive overflow or decode failure requires transport reset before reuse. C24 recovery
always requires the operator to select EOT or CL explicitly.

Compatibility impact: applications that previously treated timeouts, transport errors, or
cancellation from write/control operations as definite non-execution must handle
`OperationOutcomeUnknown`. `recover-c24` without `eot` or `cl` is rejected before serial open.

Acceptance criteria:

1. Every async wrapper that stores output spans, copied request items, request metadata, monitor
   metadata, or loopback state checks configured/busy/reset admission before mutating that state.
   A rejected second call returns `Busy`; the first request retains its exact frame, destination
   storage, point count, and callback result.
2. Every operation that can change PLC, module, password, frame-registration, signal, mode, or
   transmission state returns `OperationOutcomeUnknown` after an unconfirmed post-transmission
   timeout, transport failure, cancellation, decode failure, or response parse failure. Confirmed
   PLC errors remain `PlcError`; Remote RESET with confirmed physical TX completion retains its
   documented request-sent status even when cancellation was requested during TX.
3. Host-sync state-changing wrappers apply the same unknown-outcome contract, close ambiguous
   transport sessions as required, and never retry automatically.
4. Receive overflow and decoder error set the unsequenced transport-reset gate for reads as well as
   writes. Format2 retains block-identity isolation.
5. CLI `recover-c24` accepts exactly one explicit case-insensitive `eot` or `cl` argument. Omission,
   unknown values, and extra arguments fail before the serial device is opened.
6. Full/reduced/ultra host builds, CTest, generated API drift, Markdown links, package consumers,
   and the configured PlatformIO release matrix pass after the corrections.

- [x] Implementation completed in this repository.
- [x] Tests added or updated for every acceptance criterion.
- [x] Relevant checks passed and evidence recorded.
- [x] Codex self-review completed.
- [x] Claude source review completed (`CLAUDE-MCS-20260712-01`; reviewed through `ec7f6b9`).
- [x] Claude findings dispositioned and affected checks rerun; all four findings were accepted and corrected.
- [x] Required live serial/PLC checks passed or explicitly dispositioned (no live check required;
  request admission, error classification, reset gating, and CLI pre-open validation are
  deterministic host/codec contracts and make no new hardware-support claim).
- [x] Documentation and final acceptance agree with the completed implementation.

## Final acceptance summary (2026-07-18)

- All implementation, test, review, documentation, and final-acceptance checklist items in this
  record are complete.
- D-087 through D-092 additionally passed the approved Linux/POSIX read-only live checks recorded in
  the repository-root [TODO.md](../../TODO.md): iQ-R at `19200 / 8E1` with and without RTS/CTS, and
  Q-series at `19200 / 7E2` with C4 ASCII Format 4.
- D-096 and D-099 through D-107 received final sign-off against their already recorded automated,
  package, documentation, and review evidence; they did not require additional live PLC checks.
- No active quality-overhaul TODO remains.

## Verification evidence

- Build/test commands: `run_ci.bat --build-dir build_review --with-platformio` and
  `release_check.bat`, last passed 2026-07-13 after MCS-CR-001.
- Automated results: CMake/Ninja full host build passed; `codec_tests`,
  `standard_header_consumer`, and `cli_serial_config_tests` all passed (3/3); Markdown links and the
  generated API-reference drift check passed.
- PlatformIO results: after adding the existing
  `%USERPROFILE%\.platformio\penv\Scripts` directory to the process PATH, the nine CI-selected
  native/RP2040/ESP32-C3/Arduino Mega environments and packed-package consumers passed. A direct
  full matrix run also passed all 12 configured environments, including native CLI, polling-
  reconnect, and Mega ultra-minimal. The D-110 review additionally caught and fixed C++17 aggregate
  parenthesized initialization and AVR's missing `<utility>` header before the final pass.
- Script checks: all four Linux CLI scripts passed `bash -n`; the PowerShell password recheck script
  parsed successfully; `git diff --check` passed.
- Codex self-review covered the actual diff, public constructor/enum surface, integer truncation,
  validation-before-open order, Windows DCB and POSIX termios mappings, path lifetime/termination,
  CLI omission and unknown-value behavior, RS-485 separation, examples, scripts, docs, and generated
  API reference. It found and corrected uint8 truncation, stale CLI wrappers, POSIX string-view path
  termination, and invalid-reconfiguration state loss before this evidence was marked complete.
- MCS-CR-001 evidence covers Busy-first admission before all shared in-flight state mutation;
  preservation of the first request frame, output span, and callback; exhaustive classification of
  every state-changing `OperationKind`; host-sync propagation; Remote RESET cancellation after
  confirmed physical TX; read-side checksum/decode failure and RX overflow reset gating; exact CLI
  argument count and pre-open rejection for `recover-c24`; updated public and generated docs; host
  CTest 3/3; nine configured PlatformIO environments; packed PlatformIO consumers; release archive
  verification; Markdown links; generated API drift; and `git diff --check`.
- D-093/D-094/D-095/D-097 evidence covers deleted default/two-step construction; immutable tagged
  `c4_binary`, C-family `ascii(AsciiFrameKind, AsciiFormat, ...)`, and `e1(CodeMode, ...)` paths;
  required profile/route inputs; unknown enum and unsupported combination rejection; Binary
  eight-data-bit validation; zero output on encode failure; fixed decoder selection; no fallback;
  CLI omission/unknown tests; source examples; generated API; all MCU builds; and packed consumers.
- D-098 evidence covers required typed modes for every configurable C-frame factory/preset/CLI
  boundary; removal and CLI rejection of the inactive 1E sum-check option; Enabled/Disabled output
  length; correct/missing/corrupt/extra checksum vectors across Format1/2/3/4, C1, and C4 Binary;
  `SumCheckMismatch` classification; no mode mutation; and no retry/fallback. Self-review found and
  corrected the previously ignored 1E sum-check argument before completion was recorded.
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
- D-102 evidence covers mandatory typed 4C destination modules; HostStation and inactive-frame
  field absence; OwnStation, Multiple CPU, redundant CPU, explicit-target, width, omission,
  malformed-input, and alias semantics; zero request bytes/client TX on invalid targets;
  OwnStation-only read/write command gating; routed batch read/write; strict ASCII module parsing;
  ASCII/binary foreign-module identity; and absence of module fallback. Host CI passed 3/3, all
  configured PlatformIO environments passed, both packed-package consumers passed, scripts parsed,
  Markdown links passed, and the generated API reference matched after the D-102 implementation.
- D-103 evidence covers separate standard and m:n route types; mandatory `SelfStationNo`; explicit
  zero and 1..31; missing, negative, nonnumeric, overflow, and invalid-combination rejection before
  serial open; absence from HostStation/1C/1E; 2C/3C/4C ASCII/binary wire values; strict foreign
  self-station decode identity; client discard followed by matching-response completion; and no
  topology/self-station fallback. Host CI passed 3/3, all nine PlatformIO environments passed, both
  packed-package consumers passed, all four Bash scripts and the PowerShell script parsed,
  Markdown links passed, the generated API reference matched, and `git diff --check` passed.
- D-104 evidence covers the common 3000 ms default; 1..`INT32_MAX`, zero, overflow, negative, and
  nonnumeric boundaries; wrap-safe async/CLI comparison; total-deadline preservation after partial
  RX; independent 1E 4000 ms/`0x0010` default, zero/max vectors, non-unit/overflow rejection;
  Remote RESET TX-completion semantics and transport failure; ordinary command timeout failure;
  unsequenced timeout reuse blocking and Format2 identity-based reuse. Host CI passed 3/3, all nine
  PlatformIO environments passed, both packed-package consumers passed, all four Bash scripts and
  the PowerShell script parsed, Markdown links passed, the generated API reference matched, and
  `git diff --check` passed.
- D-105 evidence covers the common 250 ms default; 1..`INT32_MAX`, zero, overflow, negative, and
  nonnumeric boundaries; no-RX behavior; one-byte and multiple-chunk deadline restarts; a complete
  response assembled from chunks; exact-boundary rejection before append/decode; 32-bit clock
  wrap; preservation of the fixed total response deadline; incomplete-state discard; unsequenced
  transport-reset gating; Format2 identity isolation; and CLI post-read deadline checks. Host CI
  passed 3/3, all nine PlatformIO environments passed, both packed-package consumers passed, all
  four Bash scripts and the PowerShell script parsed, Markdown links passed, the generated API
  reference matched, and `git diff --check` passed.
- D-106 evidence covers compile-time rejection of zero/one-policy sync calls; exact CLI two-policy
  validation before serial open; all six conflict/clear combinations; binary and ASCII command
  data; unknown enum and unsupported 1E rejection with zero output; async success; transport
  failure, timeout, and post-TX cancellation as `OperationOutcomeUnknown`; pending-frame clearing;
  no automatic retry; and unsequenced reset gating. Existing `StatusCode` numeric values were
  preserved by appending the new value. Host CI passed 3/3, all nine PlatformIO environments
  passed, both packed-package consumers passed, all four Bash scripts and the PowerShell script
  parsed, Markdown links passed, the generated API reference matched, and `git diff --check`
  passed.
- D-107 evidence covers compile-time rejection of no-policy sync calls; exact CLI one-policy
  validation before serial open; removal and rejection of normal/safe/numeric aliases for both RUN
  and PAUSE; both PAUSE policies; binary and ASCII command data; unknown enum and unsupported 1E
  rejection with zero output; success and confirmed PLC-error responses; transport failure,
  timeout, and post-TX cancellation as `OperationOutcomeUnknown`; no force escalation or retry;
  pending-frame clearing; and unsequenced reset gating. Host CI passed 3/3, all nine PlatformIO
  environments passed, both packed-package consumers passed, all four Bash scripts and the
  PowerShell script parsed, Markdown links passed, the generated API reference matched, and
  `git diff --check` passed.
- D-108 evidence covers the absence of public width booleans and former ambiguous types; separate
  Word/DWord request and result spans; Word `0`/`0xFFFF` and DWord `0x00010000`/`0xFFFFFFFF` wire
  values; mixed random and monitor decoding; D and LZ route selection; 1C/1E pre-send rejection;
  link-direct Word-only types; client output-buffer preflight; non-wrapping oversized-count
  rejection; strict CLI width syntax and Word overflow rejection before serial open. Host CI passed
  3/3, all nine PlatformIO environments passed, both packed-package consumers passed, Markdown
  links passed, the generated API reference matched, and `git diff --check` passed.
- D-109 evidence covers compile-time rejection of default-constructed generic/high-level/link-direct
  random-write items; device-and-value construction; explicit Word/DWord zero and maxima; explicit
  Bit OFF/ON; high-level and codec unknown-enum rejection; atomic no-TX behavior for a mixed valid
  and invalid item list; generic and link-direct CLI missing/empty/unknown-value rejection before
  serial open; D-122 TX-cancellation deferral; timeout, transport failure, and post-TX
  cancellation as `OperationOutcomeUnknown`; confirmed PLC error preservation; pending-frame
  clearing; reset gating; and no automatic retry. The first full run detected and corrected a
  feature-disabled `OperationKind` conditional-compilation defect. The final run passed Host CI
  3/3, all nine PlatformIO environments, both packed-package consumers, Markdown links, generated
  API-reference drift, Bash/PowerShell syntax, and `git diff --check`.
- D-122 evidence covers hookless operation, complete-pair enforcement, null user, busy replacement,
  stable begin/end user and exact call counts, compile-time rejection of omitted TX status,
  TX-time cancellation deferral until explicit completion/abort, duplicate-notify rejection,
  hookless transport failure, host-sync cleanup, transport-reset gating, and existing post-TX
  outcome classification. Host CI passed 3/3 and all 12 PlatformIO environments passed. Generated
  API-reference, packaged consumers, Markdown links, scripts, and final diff checks are recorded
  after their final runs below.
- Physical serial/PLC communication: not executed in this correction batch; no new hardware claim
  was introduced and MCS-CR-001 records the deterministic-test disposition.
- Subsequent approved read-only live evidence was recorded on 2026-07-18 for the Linux POSIX serial
  backend, both hardware-flow modes, explicit `7E2`, and C4 ASCII Format 4. See the repository-root
  [TODO.md](../../TODO.md) for the exact targets, endpoints, settings, responses, and no-write safety
  record.
- Claude review: `CLAUDE-MCS-20260712-01` was executed against `ec7f6b9`. All four findings were
  accepted, corrected, self-reviewed, and reverified. The later `a403a4f` commit changed only
  archived review-document references and did not alter runtime, tests, or the public API.

## BH-LIVE-SERIAL-20260729 — Supplemental bug-hunt serial verification

Scope: commit `060e4e7fbc94d4775df9eedbcd2174f996f8b37f`; R120PCPU with RJ71C24-R2
CH2; `COM3`; `19200 / 8N1`; RTS/CTS; C4 binary Format 5; station `0`; sum check
enabled; profile `melsec:iq-r`.

Target contract: the current host implementation communicates with the explicitly selected serial
configuration, sends profile-catalog range exceedances that fit the wire format, and executes the
normal monitor registration/read path without changing device values.

Acceptance evidence:

- [x] A one-word `D100` read succeeded with value `0x0000`; sum-check-bearing TX and RX frames were
  exchanged successfully.
- [x] A one-word `R32768` read was transmitted and the PLC returned end code `0x4031`; no pre-send
  profile-range rejection occurred.
- [x] Native monitor registration for `D100`, `D105`, `M100`, and `M105` succeeded. One monitor read
  returned zero for all four items, and direct verification reads matched. No device value was
  written. The final monitor registration remains those four devices until replaced or reset.
- [x] With a `1 ms` response timeout, the same monitor registration emitted exactly one 46-byte TX
  frame and returned the state-changing-request-transmitted/outcome-unknown classification. A
  complete 20-byte response arrived too late to confirm the operation within the deadline. No
  automatic registration retry or subsequent command was transmitted in that session.
- [x] After closing the timed-out session and waiting one second, a normal-timeout read-only monitor
  request emitted exactly one 20-byte TX frame and returned eight data bytes. All four values were
  zero, proving that the PLC had applied the registration while the host had correctly declined to
  claim success. This follow-up did not register again or write any device value.
- [x] The repository working tree was clean after the live probes.

Disposition: the normal serial route, enabled sum check, profile-range non-guard, and monitor
success path passed on the stated hardware. The live timeout probe additionally demonstrated
post-transmit unknown-outcome classification, no automatic retry, transport-session closure, and
successful read-only inspection after reopening. Deterministic mock fault injection remains the
boundary coverage for failure timings that cannot be forced precisely on the live link.

Final release disposition (2026-07-29): accepted. The user explicitly approved using the live
timeout evidence together with deterministic mock fault injection as REL-012 release evidence. The
2026-07-18 Linux/POSIX live evidence remains applicable: changes after that evidence affect monitor
outcome classification, codec validation/encoding, and the bundled algorithm compatibility layer,
but do not change the POSIX serial configuration or termios backend. The current Windows live probe
covers the changed monitor/codec path, while mock tests cover exact pre/post-transmit failure
boundaries. A current Release build plus all four CTest targets (`codec_tests`,
`standard_header_consumer`, `bundled_algorithm_compat_tests`, and `cli_serial_config_tests`) passed
again. No additional serial live check is required for REL-012, and final evidence review is
complete.
