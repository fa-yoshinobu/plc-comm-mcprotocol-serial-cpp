# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2026-06-24

### Changed
- Bumped CMake, PlatformIO, Arduino, and public version-header metadata to `1.0.0` for the first stable release line.

## [0.7.0] - 2026-06-15

### Changed

- Breaking change: removed `GlobalSignalControlRequest::station_no`. Users who assigned this
  ignored field will now get a compile error; delete the assignment and use
  `ProtocolConfig::route.station_no` for the actual target station.

## [0.6.0] - 2026-06-14

- Replaced the public PLC selector with `PlcProfile`: `ProtocolConfig::plc_profile`
  now carries canonical profiles such as `melsec:q-l` and `melsec:iq-r`, while
  `PlcSeries` remains an internal layout/command-family derivation.
- Changed `mcprotocol_cli` from `--series` to required `--plc-profile`; short
  labels such as `ql` and `iqr` are rejected.
- Renamed Linux CLI wrapper configuration from `MCPROTOCOL_SERIES` to
  `MCPROTOCOL_PLC_PROFILE`.

## [0.2.8] - 2026-06-12

- Bumped release metadata after the resolved live-validation pass.
- Kept the remaining RJ71C24-R2 remote-password item as a target-dependent
  follow-up, not a release blocker for the supported command surface.

## [0.2.7] - 2026-05-03

- Tightened string-device parsers so oversized decimal or hexadecimal values fail instead of
  overflowing in plain, link-direct, and qualified-buffer address helpers.
- Added regression coverage for all-letter hexadecimal device numbers and invalid known-code
  number fallbacks.

## [0.2.6] - 2026-05-02

- Made host `mcprotocol_cli` protocol selection explicit: live commands now
  require `--frame` and `--plc-profile` instead of falling back to `c4-binary` + `melsec:q-l`.
- Updated host shell examples to require `MCPROTOCOL_FRAME` and
  `MCPROTOCOL_PLC_PROFILE` unless a target-specific wrapper supplies them.
- Removed `0631` CPU-monitoring deregistration from the public API, codec, CLI,
  and tests, and documented `0630` / `0631` CPU monitoring as explicitly not
  needed for this library.
- Documented label access, file control, `2101`, and drive/file memory as
  explicitly not needed for this library instead of active implementation TODOs.

## [0.2.5] - 2026-05-02

- Added C24 mode switching (`1612`) request support in `CommandCodec`, `MelsecSerialClient`, and
  `PosixSyncClient`.
- Added manual-shaped binary and ASCII request tests for C24 mode switching and updated docs/TODO
  status for the implemented surface.

## [0.2.4] - 2026-04-14

- added a direct README link to the PlatformIO registry page so install guidance points at the published package metadata

## [0.2.3] - 2026-04-13

- trimmed the PlatformIO package export so build-system files and host-only shell examples are not shipped in the registry tarball

## [0.2.2] - 2026-04-12

- Split the top-level `README.md` into a shorter overview plus focused user and maintainer docs for
  library entrypoints, PlatformIO usage, and docs/CI workflow.
- Added maintainer guardrails for manual-derived protocol rules and difference-first triage.
- Kept release/build metadata aligned with the current documentation layout.

## [0.2.1] - 2026-04-12

- Added `SM`, `SD`, `RD`, `LZ`, `LTN`, `LSTN`, `LCN`, `LTS`, `LTC`, `LSTS`, `LSTC`, `LCS`, and
  `LCC` device support and validated the practical `iQ-R` spot-device paths on `RJ71C24-R2`.
- Added `Jn\\...` link-direct support for batch and multi-block command surfaces, plus CLI
  front-ends and hardware validation for the validated `J1` path.
- Fixed binary single-point bit read/write behavior and aligned current request-shape tests with
  manual-backed bit-packing rules and validated hardware results.
- Moved generated API docs to GitHub Pages workflow deployment, pinned Actions workflow versions,
  and kept release/feature-profile behavior aligned with the current build layout.
- Added maintainer guardrails for manual-derived protocol rules and difference-first triage.

## [0.2.0] - 2026-04-11

- Added simpler host and high-level library entrypoints with `high_level.hpp`,
  `host_sync.hpp`, and a compile-checked host quickstart example.
- Fixed binary request encoding for non-iQ-R random, random-bit, and multi-block
  command families based on manual review and real-hardware validation.
- Consolidated validation reports into one report per target and refreshed the
  top-level documentation to separate stable guidance from hardware evidence.
- Expanded generated API documentation comments and kept GitHub Actions CI and
  tagged-release automation aligned with the current package version.

## [0.1.1] - 2026-04-10

- Added MCU-oriented PlatformIO packaging and example environments for `RP2040` and `ESP32-C3`.
- Added a read-only real-UART Arduino sample for `Serial1` MCU bring-up.
- Added Doxygen generation, Markdown link checking, and GitHub Actions workflows for CI and tagged releases.
- Added beginner-oriented user docs for setup, wiring, MCU quickstart, troubleshooting, FAQ, and safe CLI examples.
- Added validation summaries for hardware support, command fallback behavior, and PlatformIO memory-footprint profiles.
- Added build-time feature switches and reduced / ultra-minimal memory profiles for embedded targets.
