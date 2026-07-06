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

### Added
- Library: Added `mcprotocol::serial::module_io` named constants for `3C` / `4C` request-destination module I/O routing and made `RouteConfig` default to `module_io::OwnStation`.
- Tests: Added coverage for the 13 canonical module I/O constant names and aliases.
- Docs: Documented the module I/O constants through the generated C++ API reference source comments.

## [1.1.1] - 2026-07-05

### Changed
- Release: Bumped package metadata to `1.1.1`.
- Tooling: Added release metadata synchronization from `library.json` so mirrored Arduino, CMake, and public version-header metadata stay aligned before release checks.
- CI: Added PlatformIO package packing and content checks to prevent development files or build outputs from entering published packages.
- CI: Added an optional PlatformIO registry publish path for release workflow dispatches using `PLATFORMIO_AUTH_TOKEN`.
- CI: Aligned the release workflow with the SLMP C++ repository: re-run build/tests and API reference checks, then attach a source archive to the GitHub release.

## [1.1.0] - 2026-07-05

### Added
- Tests: Added profile-rule and unsupported-device validation coverage for serial cross-verification rules.
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
- Library: Renamed the canonical saved/displayed serial MC profile strings for
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
