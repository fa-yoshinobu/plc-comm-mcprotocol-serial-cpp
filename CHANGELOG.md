# Changelog

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
