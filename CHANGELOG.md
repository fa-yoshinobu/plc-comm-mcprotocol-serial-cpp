# Changelog

## 0.2.3 - 2026-04-13

- trimmed the PlatformIO package export so build-system files and host-only shell examples are not shipped in the registry tarball

## 0.2.2 - 2026-04-12

- Split the top-level `README.md` into a shorter overview plus focused user and maintainer docs for
  library entrypoints, PlatformIO usage, and docs/CI workflow.
- Added maintainer guardrails for manual-derived protocol rules and difference-first triage.
- Kept release/build metadata aligned with the current documentation layout.

## 0.2.1 - 2026-04-12

- Added `SM`, `SD`, `RD`, `LZ`, `LTN`, `LSTN`, `LCN`, `LTS`, `LTC`, `LSTS`, `LSTC`, `LCS`, and
  `LCC` device support and validated the practical `iQ-R` spot-device paths on `RJ71C24-R2`.
- Added `Jn\\...` link-direct support for batch and multi-block command surfaces, plus CLI
  front-ends and hardware validation for the validated `J1` path.
- Fixed binary single-point bit read/write behavior and aligned current request-shape tests with
  manual-backed bit-packing rules and validated hardware results.
- Moved generated API docs to GitHub Pages workflow deployment, pinned Actions workflow versions,
  and kept release/feature-profile behavior aligned with the current build layout.
- Added maintainer guardrails for manual-derived protocol rules and difference-first triage.

## 0.2.0 - 2026-04-11

- Added simpler host and high-level library entrypoints with `high_level.hpp`,
  `host_sync.hpp`, and a compile-checked host quickstart example.
- Fixed binary request encoding for non-iQ-R random, random-bit, and multi-block
  command families based on manual review and real-hardware validation.
- Consolidated validation reports into one report per target and refreshed the
  top-level documentation to separate stable guidance from hardware evidence.
- Expanded generated API documentation comments and kept GitHub Actions CI and
  tagged-release automation aligned with the current package version.

## 0.1.1 - 2026-04-10

- Added MCU-oriented PlatformIO packaging and example environments for `RP2040` and `ESP32-C3`.
- Added a read-only real-UART Arduino sample for `Serial1` MCU bring-up.
- Added Doxygen generation, Markdown link checking, and GitHub Actions workflows for CI and tagged releases.
- Added beginner-oriented user docs for setup, wiring, MCU quickstart, troubleshooting, FAQ, and safe CLI examples.
- Added validation summaries for hardware support, command fallback behavior, and PlatformIO memory-footprint profiles.
- Added build-time feature switches and reduced / ultra-minimal memory profiles for embedded targets.
