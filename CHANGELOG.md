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
- Library: Added a long-state read helper for long timer, long retentive timer, and long counter contact/coil devices.
- Library: Added synchronous special-route helpers for link-direct `Jn\...` access and diagnostic native qualified `Un\Gn` / `Un\HGn` probes.
- Library: Added ASCII link-direct extension encoding for `Jn\X`, `Jn\Y`, `Jn\B`, `Jn\W`, `Jn\SB`, and `Jn\SW` routes.
- Tooling: Added the `read-long-state-bits` command to the CLI.
- Tooling: Added `MCPROTOCOL_SERIAL_TRACE=1` support for logging MC TX/RX frame bytes from the synchronous host client.

### Changed
- Library: Routed long counter contact and coil reads through the long-state helper path.
- Docs: Updated maintainer and user rules for long-state routing, special-route helpers, diagnostic native qualified probes, and trace logging.
- Docs: Closed the `remote_reset` no-response timeout policy investigation as manual-derived behavior (SH-080003-AF p.173: the target is reset, so the response message may not be returned) and recorded the rule in the maintainer manual-derived rules.

### Fixed
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
