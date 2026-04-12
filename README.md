[![Documentation](https://img.shields.io/badge/docs-GitHub_Pages-blue.svg)](https://fa-yoshinobu.github.io/plc-comm-mcprotocol-serial-cpp/)
[![Release](https://img.shields.io/github/v/release/fa-yoshinobu/plc-comm-mcprotocol-serial-cpp?label=release)](https://github.com/fa-yoshinobu/plc-comm-mcprotocol-serial-cpp/releases/latest)
[![CI](https://img.shields.io/github/actions/workflow/status/fa-yoshinobu/plc-comm-mcprotocol-serial-cpp/ci.yml?branch=main&label=CI&logo=github)](https://github.com/fa-yoshinobu/plc-comm-mcprotocol-serial-cpp/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![PlatformIO](https://img.shields.io/badge/PlatformIO-compatible-orange.svg)](https://platformio.org/)
[![Lint: PIO Check](https://img.shields.io/badge/Lint-PIO%20Check-blue.svg)](https://docs.platformio.org/en/latest/core/userguide/cmd_check.html)

# MC Protocol Serial C++ Library

MC protocol serial library for MCU-oriented environments.

This repository is for cases like these:

- You want to talk to a Mitsubishi PLC over `serial / RS-232C / RS-485`
- You want a `C++` library that does not allocate dynamically
- You want to run the same core logic on `Linux`, `RP2040`, `ESP32-C3`, or `Arduino Mega 2560`

The current codebase includes:

- A transport-agnostic library centered on `MelsecSerialClient`
- A simpler host-side synchronous entrypoint via `PosixSyncClient`
- PlatformIO example projects for `RP2040`, `ESP32-C3`, and `Arduino Mega 2560`
- Board-specific Arduino samples for `RP2040`, `ESP32-C3`, and `Arduino Mega 2560`
- GitHub Actions for host build/test/docs and PlatformIO compile checks
- Real-hardware validation records for `RJ71C24-R2`, `LJ71C24`, `QJ71C24N`, and `FX5UC-32MT/D`

## Start Here

Choose the first path that matches what you are trying to do.

### 1. Simplest host-side bring-up

Start with [host_sync_quickstart.cpp](examples/host_sync_quickstart.cpp) if you want the smallest
blocking example on Windows or POSIX.

- best first read for host-side use
- read-only bring-up
- uses the synchronous `PosixSyncClient` facade

### 2. Real MCU UART bring-up

Start with [MCU Quickstart](docsrc/user/MCU_QUICKSTART.md) and
[Examples Index](examples/README.md) if you want to run on `RP2040`, `ESP32-C3`, or
`Arduino Mega 2560`.

- board-specific UART samples
- read-only first step
- intended for actual `TTL UART -> level shifter -> RS-232C` wiring

### 3. Low-level async integration

Start with [mcu_async_batch_read.cpp](examples/mcu_async_batch_read.cpp) if you want the async
state machine directly and plan to integrate your own UART layer or scheduler.

### 4. Confirm the real target settings

Before building against real hardware, confirm the exact serial settings for your target in
[HARDWARE_VALIDATION.md](docsrc/validation/reports/HARDWARE_VALIDATION.md).

The example projects keep intentionally simple defaults. Those defaults are example defaults, not
the authority for the current validated settings.

## Documentation Map

### For Users

- [Examples Index](examples/README.md)
- [Library Entrypoints](docsrc/user/LIBRARY_ENTRYPOINTS.md)
- [PlatformIO](docsrc/user/PLATFORMIO.md)
- [Wiring Guide](docsrc/user/WIRING_GUIDE.md)
- [MCU Quickstart](docsrc/user/MCU_QUICKSTART.md)
- [FAQ](docsrc/user/FAQ.md)
- [Hardware Validation Matrix](docsrc/validation/reports/HARDWARE_VALIDATION.md)
- [Footprint Profiles](docsrc/validation/reports/FOOTPRINT_PROFILES.md)
- [Generated API Docs](https://fa-yoshinobu.github.io/plc-comm-mcprotocol-serial-cpp/)

### For Maintainers

- [Maintainer Docs Index](docsrc/maintainer/README.md)
- [Developer Notes](docsrc/maintainer/DEVELOPER_NOTES.md)
- [Manual Command Coverage](docsrc/maintainer/MANUAL_COMMAND_COVERAGE.md)
- [Docs And CI](docsrc/maintainer/DOCS_AND_CI.md)
- [TODO / Current Follow-up](docsrc/maintainer/TODO.md)
- [Native Command Backlog](docsrc/maintainer/NATIVE_COMMAND_BACKLOG.md)
- [Release Process](docsrc/maintainer/RELEASE_PROCESS.md)
- [Changelog](CHANGELOG.md)

## What Works Today

- The same transport-agnostic core is used by host-side tools and MCU firmware examples.
- `2C`, `3C`, and `4C` command handling are in place. `2C` is ASCII-only. The codebase also has
  initial `1C` and `1E` support for narrower subsets.
- `2C` / `3C` / `4C` ASCII currently support `Format1`, `Format2`, `Format3`, and `Format4`.
  `Format2` is the block-numbered variant and maps to `ProtocolConfig::ascii_block_number` or
  CLI `--block-no`.
- The implemented command families cover the practical device-memory, random, multi-block,
  monitor, host-buffer, module-buffer, CPU-model, remote-control, password/error, loopback,
  user-frame, and C24 extras paths described elsewhere in this repo.
- Real-hardware validation currently covers `RJ71C24-R2`, `LJ71C24`, `QJ71C24N`, and
  `FX5UC-32MT/D`.
- There are no active command-family holds on the currently validated targets.
- Qualified helper access over `0601/1601` is the supported public `U...` path.
- On the validated `RJ71C24-R2 + R120PCPU` iQ-R spot-device path, `Jn\\...` batch and multi-block
  surfaces are validated.

For the exact PASS / status matrix and the verified serial settings for each target, see
[HARDWARE_VALIDATION.md](docsrc/validation/reports/HARDWARE_VALIDATION.md).

## Current Limits

- This repository does not implement the full MC protocol serial command list. Use
  [MANUAL_COMMAND_COVERAGE.md](docsrc/maintainer/MANUAL_COMMAND_COVERAGE.md) for the exact
  implemented vs. missing command families.
- `1C` and `1E` remain subset implementations. They are useful, but they do not expose the full
  `3C` / `4C` surface.
- Some command families are target-dependent and require the right `--series` selection.
- Native qualified access is not a supported public workflow. Keep `U...` access on the helper
  path only.
- `Jn\\...` random and monitor remain outside the currently validated public surface.
- `FX5UC-32MT/D` treats `0613/1613/0601/1601` and `0801/0802` as unsupported / not applicable on
  serial `3C/4C`.
- Large contiguous `write-words` and `write-bits` are still split automatically to fit fixed
  request buffers.
- The current active follow-up is target-dependent validation for `1005` remote latch clear and
  `1630` / `1631` remote password unlock/lock on `RJ71C24-R2 + R120PCPU`. Parked implementation
  gaps such as `1612`, `0630`, and `2101` remain documented in [TODO.md](docsrc/maintainer/TODO.md).

For target-specific limits and current follow-up items, use
[HARDWARE_VALIDATION.md](docsrc/validation/reports/HARDWARE_VALIDATION.md) and
[TODO.md](docsrc/maintainer/TODO.md).

## Library Use

The library exposes three practical entry paths:

- high-level request builders in [`mcprotocol/serial/high_level.hpp`](include/mcprotocol/serial/high_level.hpp)
- synchronous host-side bring-up in [`mcprotocol/serial/host_sync.hpp`](include/mcprotocol/serial/host_sync.hpp)
- low-level async integration through `MelsecSerialClient`

Use these docs for the details:

- [Library Entrypoints](docsrc/user/LIBRARY_ENTRYPOINTS.md)
- [MCU Quickstart](docsrc/user/MCU_QUICKSTART.md)
- [Examples Index](examples/README.md)

## PlatformIO and Build Profiles

PlatformIO packaging, environment names, reduced/ultra profiles, and build-time tuning macros are
documented here:

- [PlatformIO](docsrc/user/PLATFORMIO.md)
- [Footprint Profiles](docsrc/validation/reports/FOOTPRINT_PROFILES.md)

## Docs and CI

Local documentation tasks and GitHub automation are documented here:

- [Docs And CI](docsrc/maintainer/DOCS_AND_CI.md)
