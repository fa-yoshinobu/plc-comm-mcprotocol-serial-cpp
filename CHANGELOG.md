# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

**Entry labels**

- `Release`: Package/version metadata and publishing preparation.
- `Library`: Runtime behavior, public API, protocol handling, or validation in the distributed library.
- `Docs`: README, user guides, generated API docs, or other documentation-only changes.
- `Samples`: Examples, sample flows, sample scripts, or sample applications.
- `Tests`: Test suites, test fixtures, golden vectors, or verification data.
- `Tooling`: Developer/operator command-line tools and helper utilities.
- `CI`: Release checks, workflow scripts, or automation-only changes.

## [Unreleased]

- Release: Aligned artifact roles so the registry package contains consumer runtime, native API metadata, license, README, and ecosystem-native examples where applicable while excluding repository tests and maintainer tooling; the GitHub source archive retains tracked non-hardware validation and maintainer inputs.

### BREAKING

- Samples/Packaging: Arduino Mega 2560 and all other AVR/8-bit targets are no longer supported.
  The Mega PlatformIO environments, sample, and AVR package-consumer gate are removed. Existing
  AVR users must migrate to ESP32 or maintain an unsupported downstream port. The public decode API
  is unchanged by this support decision.
- Library: Bit inputs and outputs now use native `bool` through the `BitValue` type alias. Replace
  the removed `BitValue::Off` and `BitValue::On` enumerators with `false` and `true`; invalid bit
  states are unrepresentable at the public type boundary.
- Library: One absolute transaction deadline now covers first TX write, partial TX progress,
  physical drain, and complete RX. Call `notify_tx_started()` immediately before the first write;
  `notify_tx_complete()` does not restart the deadline. `inter_byte_timeout_ms` and its builder/CLI
  option are removed, and host transports use deadline-bounded write, drain, and read APIs. Any
  deadline expiry requires transport reset.
- Library: A deadline reached while physical TX is pending is now latched until the UART/DMA layer
  reports completion or abort through `notify_tx_complete()`. The client stays busy and keeps the
  RS-485 direction hook and completion callback pending; the later notification publishes the
  original timeout result and cannot replace it with a later transport cause.
- Library: `StatusCode::NotConnected` and `StatusCode::Closed` distinguish lifecycle failures, and
  `Status::cause` identifies the underlying failure wrapped by `OperationOutcomeUnknown`.
- Library: Accepted single requests are preflighted against complete request, worst-case binary DLE
  expansion, response, decoder, and caller-output capacities. An operation that cannot fit one wire
  request returns `InvalidArgument` before request-state mutation; an independently undersized
  caller output returns `BufferTooSmall`. Single-request operations are never split or resized.
- Library: Every `async_*` entry point now rejects overlap with `Busy` before changing the active
  frame, outputs, callback, or request metadata. Separate client instances remain independent.
- Library: Multi-point `PosixSyncClient::read_long_state_bits()` for long timer/retentive-timer
  status devices is now explicitly a non-atomic read aggregate: its complete plan is validated
  before transport and caller output is published only after all ordered internal reads succeed.
  All writes and every other public operation remain single-request and are never auto-split.

- Library: The public contiguous-range type is now `mcprotocol::serial::Span<T>` from
  `mcprotocol/serial/span.hpp`. Replace `std::span<T>` and the removed
  `mcprotocol/serial/span_compat.hpp` include. The library no longer injects a pre-C++20 `span`
  implementation into `namespace std`; the complete CMake build, tests, CLI, and examples now use
  strict ISO C++17 without C++20 designated initializers or concepts. `Span` accepts pointer/count,
  pointer-pair, C-array, and matching `std::array` lvalues; use `.data(), .size()` explicitly for
  other containers. Rvalue arrays are rejected, mutable views convert only to const views, and
  slicing now uses checked `try_first` / `try_subspan` output methods.
- Library: Raw public buffers now use `mcprotocol::serial::Byte` and the explicit
  `byte_to_integer<Integer>()` conversion from `mcprotocol/serial/byte.hpp`. Replace `std::byte` and
  `std::to_integer`; no compatibility alias remains in `namespace std`.
- Library: Host serial open now replaces inherited OS line state completely. POSIX uses raw input,
  output, and local modes with software flow control disabled; Win32 preserves only documented
  driver-reserved/provider DCB fields while overwriting DTR, DSR flow, software flow, error
  replacement, null stripping, and abort-on-error behavior. RTS is
  disabled for `HardwareFlowControl::None` and OS-handshaked only for `RtsCts`. Applications that
  relied on a previous process or driver session leaving extra COM/termios flags enabled must now
  express supported settings through `PosixSerialConfig`.
- Library: `qualified_buffer_word_to_byte_address` and `module_buffer_start_address` now return a
  `Status` and write the checked result through an output argument. Callers must handle
  `InvalidArgument` when word-to-byte conversion or the additional module offset cannot be
  represented in the 32-bit module-buffer address space.

- Library: The synchronous facade now returns the core completion callback status after request
  admission. Normal and extended monitor registration therefore preserve
  `OperationOutcomeUnknown` and its structured cause for unconfirmed post-send results. The async
  core exposes `notify_rx_failure(status)` so a receive-side `Timeout` or `Transport` failure is not
  misreported as caller cancellation.
- Library: Win32 receive spans larger than `MAXDWORD` are rejected with `InvalidArgument` before
  `ReadFile`; the receive length is never truncated to `DWORD`.
- Tooling: CLI request execution now returns a completed core callback status after TX/RX failure,
  preserves the underlying receive failure cause, and prints the machine status classification plus
  `cause` for `OperationOutcomeUnknown`.

- Library: Module-buffer and native qualified-buffer encoders now reject address, module-number,
  and end-of-range values that do not fit the active 1E, 1C, Q/L, or iQ-R wire fields instead of
  truncating high bits. Undefined qualified-buffer and long-state enum values are rejected.
- Library: All contiguous, link-direct, and multi-block bit-write encoders accept only native
  `bool` values; invalid bit states are excluded by the public type system.
- Library: Binary 1E monitor bit responses use packed high/low nibbles, require the exact response
  length, and reject nibbles other than `0` and `1`. ASCII 1E module-buffer reads wait for the full
  two-hex-characters-per-byte payload.
- Library: Complete Format 1/2/4 responses with an invalid CRLF terminator report `Framing` instead
  of remaining indefinitely incomplete. Starting a replacement monitor registration invalidates
  the previous registration, and reconfiguration clears all monitor registration state.
- Library: Synchronous host operations close the serial port after transport/framing reset
  conditions. POSIX writes that make zero-byte progress fail, serial descriptors request
  close-on-exec where available, exclusive access is acquired before termios changes, optional
  baud constants are compile-guarded, and Win32 rejects writes larger than the DWORD limit.
- CI: PlatformIO's native CLI target now includes the POSIX serial backend, and reduced/ultra
  subproject configuration no longer overwrites a parent project's `BUILD_TESTING` cache value.
- CI: Worktree source archives use a synthetic Git tree containing modifications, untracked files, and deletions; the extracted archive must pass host CI and build native plus ESP32-C3 consumers from its own packed PlatformIO tarball.
- Docs: Regenerated the public API reference for the checked address-conversion signatures.
- Tests: Added capacity/deadline boundaries, Busy/no-mutation and independent-instance coverage,
  structured outcome causes, native-bool, packed-nibble, CRLF, monitor-state, and ASCII
  payload-length regression coverage.
- Tests: Added deterministic synchronous-wrapper and CLI transport-failure tests plus Win32
  `MAXDWORD` receive boundary coverage.

## [3.2.2] - 2026-07-31

- Release: Bumped library, CMake, PlatformIO, Arduino, public version-header, and install-documentation metadata to `3.2.2`. This version supersedes `3.2.1` on the PlatformIO registry, where the `3.2.1` upload remained stuck in registry-side processing; the library code is unchanged from `3.2.1`.
- Docs: Installation examples no longer pin a library release, and generated API documentation no longer duplicates the current release number; GitHub Releases and registry metadata remain authoritative.
- Docs: README documentation links now include the shared Performance and Choosing a Language pages, and package registry metadata was expanded for discoverability. No functional change.

## [3.2.1] - 2026-07-29

- Release: Bumped library, CMake, PlatformIO, Arduino, public version-header, and install-documentation metadata to `3.2.1`.
- Release: GitHub Release drafts now prepend this version's changelog section to generated notes and repair a missing section on workflow reruns.

- Library: Normal and extended file-register monitor registration now return `Status::OperationOutcomeUnknown` when transmission may have completed but the response cannot be confirmed, matching the existing contract for other state-changing commands. Applications must resolve PLC registration state before retrying.
- Tests: Added monitor-registration coverage for post-transmit timeout and preserved pre-transmit failure classification.
- Library: Loopback commands snapshot caller data before asynchronous use, checksum generation is bounded without stack-sized scratch storage, and payload-size arithmetic uses checked `size_t` calculations.
- Library: C24/C2 command headers are assembled at the required dynamic width, and Q/L normal device numbers wider than their wire field are rejected instead of silently dropping high bits.
- Library: Profile device-range upper bounds are not used as transport send guards; representation and command limits remain enforced.
- Library: The bundled `<algorithm>` compatibility layer now provides the unary `transform` operation required by loopback handling, restoring PlatformIO AVR package-consumer builds.
- Tests: Added a forced-bundled-algorithm compile and behavior test so host CI covers the constrained-toolchain path.

## [3.2.0] - 2026-07-17

- Release: Bumped library, CMake, PlatformIO, Arduino, public version-header, and install-documentation metadata to `3.2.0`.
- CI: The PlatformIO release gate now discovers the standard per-user installation without a caller-specific PATH edit and treats repository-owned compiler warnings as errors.
- CI: Fixed PlatformIO package-command selection and failure propagation so the release gate cannot report success after an empty or failed package invocation.
- CI: Removed and ignored the repository-local `AGENTS.md`, excluded maintainer-only `TODO.md` from the GitHub Release ZIP, and added archive-content checks to the local and GitHub release gates.

## [3.1.0] - 2026-07-13

- Docs: Removed references to the independently maintained cross-repository verification run from library-owned profile records and terminology.
- Library: A rejected concurrent `async_*` call now returns `Busy` before mutating the active
  request's output spans, copied request items, monitor metadata, or response-size expectations.
- Library: Every transmitted state-changing command now reports `OperationOutcomeUnknown` when its
  result cannot be confirmed. This includes contiguous, extended, block, buffer, remote-control,
  password, user-frame, signal, mode-switch, and initialization operations; host-sync wrappers use
  the same contract and never retry automatically.
- Library: Unsequenced receive overflow and decode errors now require transport reset and client
  reconfiguration before reuse, preventing stale response tails from completing a later request.
- Tooling: `recover-c24` now requires an explicit `eot` or `cl` argument; omission is rejected
  before the serial device is opened.

### BREAKING
- Library: `notify_tx_complete` now requires an explicit transport status. RS-485 TX begin/end
  hooks must be installed as a complete pair, cannot change while a request is active, and always
  retain the same callback user through the matching end notification. Cancellation during TX is
  deferred until the transport explicitly reports physical completion or abort, preventing the
  transceiver from being left in transmit direction.

- Library: Replaced `RandomReadItem` and the public `double_word` switches with explicit
  `RandomReadWordItem`/`RandomReadDWordItem`, typed Word/DWord write items, separate request spans,
  and separate `uint16_t`/`uint32_t` response spans. The host-sync API now uses width-specific
  method names. No width is inferred from a device code.
- Library: Link-direct random read/write/monitor now exposes only its supported Word-width item and
  output types; the former DWord boolean is removed.
- Tooling: `random-read` requires `word:DEVICE` or `dword:DEVICE`. DWord writes use the new
  `random-write-dwords` command; `random-write-words` rejects values outside `0..65535`.
- Library: Random Word, DWord, and Bit write items/specs, including link-direct items, are no longer
  default constructible. Device and value must be supplied together; explicit zero/OFF remains
  valid, and unknown bit enum values reject the entire request before transmission.
- Library: A random write whose result cannot be confirmed after transmission now completes as
  `OperationOutcomeUnknown`, clears the pending frame, and is never retried automatically.
- Tooling: Random-write CLI items require `DEVICE=VALUE` and validate every item before opening the
  serial device. Missing/empty values and bit values other than `0` or `1` are rejected.
- Library: Removed default construction from public input request/item/address/spec types. Required
  devices, addresses, counts/data, values, targets, states, channels, and mode changes must be
  supplied at construction; explicit D0/address zero/value zero/OFF remain valid.
- Library: Global-signal control now requires typed target plus `BitValue`, and serial-module mode
  switching rejects a request that selects no change. Unknown control enums, empty containers, and
  any invalid item reject the complete request before a transmit frame is created. Receive/result
  storage remains default constructible.

- Library: Removed default construction and implicit values from `PosixSerialConfig`. Device path,
  baud rate, data bits, stop bits, parity, and hardware flow control are now required constructor
  arguments.
- Library: Replaced the public `char parity` and `bool rts_cts` fields with the typed
  `SerialParity` and `HardwareFlowControl` enums.
- Tooling: Removed the CLI's implicit serial device, 9600 baud, 8N1, and disabled-flow defaults.
  `--device`, `--baud`, `--data-bits`, `--stop-bits`, `--parity`, and `--hardware-flow` are required.
- Library: Deleted public default construction and mutable selector fields from `ProtocolConfig`.
  Callers now use the immutable tagged `c4_binary(...)`, `ascii(AsciiFrameKind, AsciiFormat, ...)`,
  or `e1(CodeMode, ...)` construction path. C4 Binary accepts no ASCII format, C-family ASCII
  requires one, and 1E accepts neither an ASCII format nor an inactive sum-check option.
- Library: Replaced `bool sum_check_enabled` with `SumCheckMode::{Enabled, Disabled}`. The C4
  protocol presets now require a `SumCheckMode` argument.
- Tooling: `mcprotocol_cli --sum-check on|off` and the corresponding validation-script inputs are
  required instead of defaulting to a checksum policy.
- Library: Removed `ProtocolConfig::ascii_block_number` and the CLI `--block-no` option. Format2
  clients now allocate and wrap block numbers per wire request; raw codec callers use an explicit
  `FrameCodecContext::format2(number)`.
- Library: Removed the mutable route aggregate and implicit HostStation selection. Protocol
  configuration and named presets now require `RouteConfig {HostStationRoute {}}` or an explicit
  frame-specific multidrop route; HostStation fixed header fields are no longer writable.
- Tooling: Added required `--route host|multidrop`. `--station` no longer selects a route from its
  numeric value and is rejected for a host route.
- Library: Split multidrop configuration into `C1MultidropRoute` and topology-specific
  `C2/C3/C4StandardMultidropRoute` and `C2/C3/C4MnMultidropRoute` types. Station is mandatory for
  every multidrop frame; network is additionally mandatory for 3C/4C. Generic station `0xFF`,
  network `0xFE`, overflow, and wrong-frame route types are rejected before encoding.
- Tooling: Added required `--network` for 3C/4C multidrop routes. It is rejected for host and 1C/2C
  routes.
- Library: Replaced the optional raw route PC number with mandatory typed `C34PcTarget` and
  `E1PcTarget` values. 3C/4C and 1E callers must explicitly select a numbered or special target,
  including the connected-station `0xFF` target; host fixes `0xFF` internally and 1C/2C expose no
  PC-target input.
- Tooling: Added required `--pc-target` for 3C/4C/1E non-host routes. Named 3C/4C selectors are
  `control`, `standby`, `special-fe`, and `connected`; ordinary numeric targets are range checked
  before narrowing, and equivalent raw special values are normalized to the canonical selector.
- Library: Replaced the defaulted raw 4C destination-module I/O/station pair with mandatory typed
  `C4DestinationModule`. 4C multidrop callers must explicitly select own station, Multiple CPU,
  redundant CPU, or a configuration-dependent explicit target; HostStation fixes own station
  internally and other frame-specific routes expose no destination-module input.
- Tooling: Added required `--module-target` for 4C multidrop routes. Host, 1C, 2C, 3C, and 1E
  routes reject it.
- Library: Removed the ambiguous `self_station_enabled` plus `self_station_no` pair. Normal/1:n
  route types expose no self-station input; m:n route types require typed `SelfStationNo` 0..31,
  including explicit zero.
- Tooling: Added required `--topology standard|mn` for 2C/3C/4C multidrop. `standard` rejects
  `--self-station`; `mn` requires `--self-station 0..31`.
- Library: Changed the omitted host response timeout from 5000 ms to the cross-library 3000 ms
  value. Explicit values must be 1..2147483647 ms and are applied as one wrap-safe total deadline
  from TX completion without extension after RX starts.
- Library: Separated the 1E ACPU monitoring timer from the host response timeout. The new
  `E1MonitoringTimer` defaults to 4000 ms (`0x0010`) and rejects non-250 ms units or field overflow
  instead of rounding or saturating.
- Library: Remote RESET now completes immediately after successful transmission and reports request
  transmission rather than PLC reset success. Global-signal and transmission-sequence timeouts are
  no longer converted to success.
- Tooling: CLI response timeout now defaults to 3000 ms. Added the optional 1E-only
  `--e1-monitoring-timer-ms`; Linux/PowerShell scripts no longer inject a 5000/8000 ms response
  fallback.
- Library: Kept the omitted inter-byte timeout at 250 ms across generic config, presets, clients,
  CLI, and scripts. Explicit values must be 1..2147483647 ms; zero and larger values fail before
  encoding/open instead of becoming immediate or wrap-unsafe deadlines.
- Library: RX deadline checks now occur before accepting each chunk as well as during polling. Each
  delivered chunk restarts only the inter-byte inactivity deadline; the total response deadline is
  unchanged. A chunk at or after either deadline is rejected.
- Tooling: Removed the FX5U soak wrapper's hidden 1000 ms inter-byte fallback. Linux scripts preserve
  omission or forward an explicit `MCPROTOCOL_INTER_BYTE_TIMEOUT_MS` value.
- Library: `PosixSyncClient::remote_run` no longer supplies implicit non-forced and no-clear
  policies. Callers must pass both `RemoteOperationMode` and `RemoteRunClearMode`; the encoder and
  async API continue to require the same typed values.
- Tooling: `remote-run` now requires exactly two positional policies: `no-force|force` and
  `no-clear|outside-latch|all-clear`. Missing, unknown, or extra values fail before serial open.
- Library: `PosixSyncClient::remote_pause` no longer supplies an implicit non-forced policy.
  Callers must pass `RemoteOperationMode` explicitly, matching the encoder and async API.
- Tooling: `remote-pause` now requires exactly one `no-force|force` argument. Removed the
  `normal`, `safe`, and numeric conflict-policy aliases from both Remote RUN and PAUSE parsing.

### Added
- Tests: Added native and Arduino Mega consumers that build the packed tarball through `lib_deps` and require both core implementation objects to be linked.

### Changed
- Library: Added `StatusCode::OperationOutcomeUnknown`. Remote RUN transport failure, timeout,
  cancellation after TX, or an unconfirmable response reports that the PLC RUN state is unknown
  instead of looking like a pre-send validation failure. Remote RUN is never retried automatically.
- Library: Applied the same unknown-outcome and no-automatic-retry contract to Remote PAUSE.
  Explicit PLC errors remain confirmed PLC responses and never trigger force escalation.

- Library: Rejects 5/6 data bits for MC protocol; binary mode requires 8 bits and ASCII accepts an
  explicit 7 or 8. Serial and protocol combinations are validated before opening the OS handle.
- Library: POSIX device paths are copied to a checked NUL-terminated buffer before `open()`.
- Library: Frame, code-mode, ASCII-format, PLC-profile, and sum-check enums are checked exhaustively
  before request encoding; unknown underlying values are rejected as invalid input. Route
  selection is represented by constructible types rather than a caller-settable enum.
- Library: Format2 response block numbers are parsed as strict hexadecimal identity fields. A
  complete mismatched/late frame is consumed without completing the active request, while malformed
  identity text is reported as a parse error.
- Library: ASCII and binary response route headers are parsed and compared with the active request.
  Complete responses from another station/network/PC/module/self-station identity are consumed
  without completing the request; malformed ASCII route text is a parse error.
- Library: CLI and async-client deadline comparisons share the same wrap-safe comparison. The CLI
  continues enforcing the total response deadline after partial RX instead of switching solely to
  the inter-byte deadline.
- Library: Unsequenced frame timeouts now require transport reset and client reconfiguration before
  another request; the host sync wrapper closes its serial port. Format2 remains reusable because
  its block identity isolates late responses.

- Release: The release workflow now checks the PlatformIO registry before publishing so an existing version cannot be republished.
- Release: Bumped library, CMake, PlatformIO, Arduino, public version-header, and install-documentation metadata to `3.1.0`.
- Library: Defined the PlatformIO package as core-only; host serial backends and `PosixSyncClient` remain source-tree CMake features.
- Docs: Replaced README links to package-excluded `docsrc` files with stable public documentation-site links and separated the PlatformIO and host installation paths.

### Fixed
- Library: Corrected PlatformIO `srcFilter` paths to be relative to the manifest `srcDir`, so packed-package consumers compile and link `client.cpp` and `codec.cpp`.
- Library: Moved bundled standard-library compatibility headers out of the public include root so they no longer shadow headers such as `<array>` on MSVC and other host toolchains.
- Build: Made the library's no-exception/no-RTTI size flags opt-in and private so normal CMake consumers retain their exception and RTTI settings.

### Tests
- Added compile-time and runtime coverage for required serial configuration, invalid and unknown
  values, embedded-NUL paths, and binary/ASCII data-bit combinations.
- Added Format2 coverage for explicit raw context, missing/inactive context rejection, 00..FF wrap,
  stale response isolation, cancellation, timeout followed by a late response, and malformed block
  numbers.
- Added route omission, required preset/CLI argument, fieldless HostStation, and explicit
  Multidrop coverage.
- Added frame-specific route-construction, explicit zero, boundary/overflow/special-value,
  wrong-frame, route-match/mismatch/malformed, foreign-response, and post-timeout route-isolation
  coverage.
- Added mandatory typed PC-target, frame-specific range, special-selector, CLI omission/overflow,
  inactive-field, pre-encode rejection, and foreign-PC response-isolation coverage.
- Added mandatory 4C destination-module construction, selector/range/alias, CLI omission and
  malformed-input, connected-only command, routed read, no-TX, and foreign-module response coverage.
- Added response-timeout default/range/wrap/partial-RX coverage, independent 1E timer default and
  boundary vectors, CLI parse/validation cases, and immediate Remote RESET transmission completion.
- Added inter-byte default/range, one-byte/multi-chunk, exact-boundary, deadline restart, 32-bit
  wrap, complete-response, partial-state discard, post-timeout isolation, and CLI boundary coverage.

## [3.0.0] - 2026-07-10

### Changed
- Release: Bumped library, CMake, PlatformIO, and public version-header metadata to `3.0.0`.

### Added
- Library: Added `mcprotocol::serial::plc_profile_display_name(profile)` as the public UI-label helper while keeping `plc_profile_name(profile)` as the canonical saved profile string.
- Tests: Added PLC profile display-name coverage.
- Docs: Documented the profile display-name helper and canonical-ID storage guidance.

## [2.0.0] - 2026-07-06

### BREAKING
- Library: Added the canonical `mcprotocol::serial::module_io` vocabulary for request-destination module I/O routing. Existing code that used raw literals should switch to named constants.

| Raw value | Named constant |
| --- | --- |
| `0x03D0` | `module_io::ControlSystemCpu` |
| `0x03D1` | `module_io::StandbySystemCpu` |
| `0x03D2` | `module_io::SystemACpu` |
| `0x03D3` | `module_io::SystemBCpu` |
| `0x03E0` to `0x03E3` | `module_io::MultipleCpu1` to `module_io::MultipleCpu4` |
| `0x03FF` | `module_io::OwnStation` |

### Added
- Library: Added `mcprotocol::serial::module_io` named constants for `3C` / `4C` request-destination module I/O routing and made `RouteConfig` default to `module_io::OwnStation`.
- Tests: Added coverage for the 13 canonical module I/O constant names and aliases.
- Docs: Documented the module I/O constants through the generated C++ API reference source comments.

### Changed
- Release: Bumped CMake, PlatformIO, Arduino, and public version-header metadata to `2.0.0`.
- Docs: Added the plc-comm family package matrix link to the README and updated PlatformIO install examples to `@^2.0.0`.

## [1.1.1] - 2026-07-05

### Changed
- Release: Bumped package metadata to `1.1.1`.
- Tooling: Added release metadata synchronization from `library.json` so mirrored Arduino, CMake, and public version-header metadata stay aligned before release checks.
- CI: Added PlatformIO package packing and content checks to prevent development files or build outputs from entering published packages.
- CI: Added an optional PlatformIO registry publish path for release workflow dispatches using `PLATFORMIO_AUTH_TOKEN`.
- CI: Aligned the release workflow with the SLMP C++ repository: re-run build/tests and API reference checks, then attach a source archive to the GitHub release.

## [1.1.0] - 2026-07-05

### Added
- Tests: Added profile-rule and unsupported-device validation coverage for serial profile rules.
- Docs: Added generated Doxygen-based API reference for the public C++ headers, with CI freshness validation.
- Docs: Added shared PLC Setup troubleshooting/code guidance for library status codes, serial MC error-family handling, and observed PLC/module codes.
- Library: Added a long-state read helper for long timer, long retentive timer, and long counter contact/coil devices.
- Library: Added synchronous special-route helpers for link-direct `Jn\...` access and diagnostic native qualified `Un\Gn` / `Un\HGn` probes.
- Library: Added ASCII link-direct extension encoding for `Jn\X`, `Jn\Y`, `Jn\B`, `Jn\W`, `Jn\SB`, and `Jn\SW` routes.
- Tests: Added a decode truncation sweep that feeds every strict prefix of valid success and error responses (1C/2C/3C/4C, ASCII Format1–4 and binary, sum-check on/off) into `FrameCodec::decode_response` and asserts `Incomplete`.
- Tooling: Added the `read-long-state-bits` command to the CLI.
- Tooling: Added `MCPROTOCOL_SERIAL_TRACE=1` support for logging MC TX/RX frame bytes from the synchronous host client.

### Changed
- Release: Bumped package metadata to `1.1.0`.
- Library: Renamed the canonical saved serial MC profile strings for
  Q and L targets from `melsec:q` / `melsec:l` to `melsec:qcpu` /
  `melsec:lcpu`. The C++ enum names remain `PlcProfile::MelsecQ` and
  `PlcProfile::MelsecL`; the old textual profile names are no longer accepted.
- Library: Routed long counter contact and coil reads through the long-state helper path.
- Docs: Updated maintainer and user rules for long-state routing, special-route helpers, diagnostic native qualified probes, and trace logging.
- Docs: Removed the manual page-navigation block from Getting Started and rely on site navigation instead.
- Docs: Consolidated user documentation to Getting Started, Usage Guide, PLC Profiles, and Gotchas; moved supported-register and wiring guidance to the shared PLC Setup Guide.
- Docs: Closed the `remote_reset` no-response timeout policy investigation as manual-derived behavior (SH-080003-AF p.173: the target is reset, so the response message may not be returned) and recorded the rule in the maintainer manual-derived rules.
- Docs: Clarified that MC Protocol Format4/Format5 selection is a serial-module configuration match issue, not remote-password behavior, and updated Q/iQ-R maintainer profiles to include verified C4 ASCII Format4 support.
- Docs: Recorded Q/L serial multi-block (`0406`/`1406`) validation in maintainer profile notes so it stays separate from SLMP built-in-Ethernet Q-series block-command guards.
- Docs: Cleaned up maintainer notes and normalized the root TODO.
- Samples: Aligned user-facing quickstart, PlatformIO UART, and Linux CLI sample defaults with the verified C4 ASCII Format4 `19200 / 8E1` serial setup.
- Tooling: Removed local absolute fallback tool paths from `run_ci.bat`.

### Fixed
- Library: Fixed an out-of-bounds read in the ASCII Format1/2/4 response decoder when the received buffer was shorter than the `STX` + block-number + header prefix; the decoder now reports `Incomplete` instead of crashing with an access violation. Observed on live iQ-R link-direct `Jn\...` reads whose responses arrived split into short serial chunks.
- Library: Reject `S` step-relay device access for serial MC profiles instead of treating it as read-only; `S` is not part of the supported MC serial device surface.
- Library: Reject standalone `G` and `HG` device access in normal, random, and multi-block device routes.
- Library: Appended the trailing device-modification field required by SH-080003-AF p.430-431 to the ASCII device extension specification for `Jn\...` and `Un\G` / `Un\HG` routes; 4C ASCII Format4 native extended access now passes on live iQ-R and Q/L targets.
- Tooling: Corrected the CLI help note that described link-direct device extension specification as binary-only; it is supported in both binary and ASCII code modes.
- Tests: Added coverage for long-state helper routing, standalone `G` / `HG` rejection, and ASCII link-direct extension encoding.
- Tests: Added request-shape coverage for the trailing ASCII device-modification field against the SH-080003-AF worked example.

## [1.0.2] - 2026-06-29

### Changed
- Release: Bumped CMake, PlatformIO, Arduino, and public version-header metadata to `1.0.2`.
- Docs: Added a maintainer TODO for the `remote_reset` no-response timeout success policy so the behavior is treated as a specification-policy investigation before any implementation change.

### Fixed
- Library: Preserved the high byte when encoding 4C/3C/2C ASCII PLC error responses with `FrameCodec::encode_error_response`.
- Tests: Added regression coverage for four-digit ASCII error-code encoding in both ENQ/NAK and STX/ETX response formats.

## [1.0.1] - 2026-06-25

### Changed
- Release: Bumped package metadata to `1.0.1`.
- Docs: Documented that MC Protocol Serial frame/profile selection is explicit: `PlcProfile::Unspecified` is an error, textual profiles require canonical names, and Linux CLI wrappers require both `MCPROTOCOL_FRAME` and `MCPROTOCOL_PLC_PROFILE`.
- Samples: Aligned Linux CLI sample scripts with the explicit frame/profile safe-default workflow.

## [1.0.0] - 2026-06-24

### Changed
- Release: Bumped CMake, PlatformIO, Arduino, and public version-header metadata to `1.0.0` for the first stable release line.
