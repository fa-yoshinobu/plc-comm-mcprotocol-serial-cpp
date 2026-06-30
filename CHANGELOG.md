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
- Library: Added synchronous special-route helpers for link-direct `Jn\...` and native qualified `Un\Gn` / `Un\HGn` access.
- Library: Added ASCII link-direct extension encoding for `Jn\X`, `Jn\Y`, `Jn\B`, `Jn\W`, `Jn\SB`, and `Jn\SW` routes.
- Tooling: Added `read-long-state-bits`, link-direct, and native-qualified commands to the CLI.
- Tooling: Added `MCPROTOCOL_SERIAL_TRACE=1` support for logging MC TX/RX frame bytes from the synchronous host client.

### Changed
- Library: Routed long counter contact and coil reads through the long-state helper path.
- Docs: Updated maintainer rules for long-state routing.

### Fixed
- Library: Reject standalone `G` and `HG` device access in normal, random, and multi-block device routes.
- Tests: Added coverage for long-state helper routing, standalone `G` / `HG` rejection, and ASCII link-direct extension encoding.

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
