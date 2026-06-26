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
