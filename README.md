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
- [Wiring Guide](docsrc/user/WIRING_GUIDE.md)
- [MCU Quickstart](docsrc/user/MCU_QUICKSTART.md)
- [FAQ](docsrc/user/FAQ.md)
- [Hardware Validation Matrix](docsrc/validation/reports/HARDWARE_VALIDATION.md)
- [Footprint Profiles](docsrc/validation/reports/FOOTPRINT_PROFILES.md)
- [Generated API Docs](docs/api/index.html)

### For Maintainers

- [Maintainer Docs Index](docsrc/maintainer/README.md)
- [Developer Notes](docsrc/maintainer/DEVELOPER_NOTES.md)
- [Manual Command Coverage](docsrc/maintainer/MANUAL_COMMAND_COVERAGE.md)
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
- `FX5UC-32MT/D` treats `0613/1613/0601/1601` and `0801/0802` as unsupported / not applicable on
  serial `3C/4C`.
- Large contiguous `write-words` and `write-bits` are still split automatically to fit fixed
  request buffers.
- The remaining repository follow-up is implementation work for `1612`, `0630`, and `2101`, plus
  hardware validation for `1005` remote latch clear, plus target-dependent validation for `1630` /
  `1631` remote password unlock/lock on `RJ71C24-R2 + R120PCPU`.

For target-specific limits and current follow-up items, use
[HARDWARE_VALIDATION.md](docsrc/validation/reports/HARDWARE_VALIDATION.md) and
[TODO.md](docsrc/maintainer/TODO.md).

## For MCU Users

The library design is intentionally simple:

- No exceptions
- No RTTI
- No dynamic allocation in the library
- Caller-owned buffers via `std::span`
- Transport-agnostic client state machine

If you want a lighter entry path, use the optional high-level helpers in
[`mcprotocol/serial/high_level.hpp`](include/mcprotocol/serial/high_level.hpp).
They provide:

- baseline `ProtocolConfig` presets
- string-address parsing such as `D100`, `M100`, `X10`
- request builders for common contiguous, sparse random, and monitor operations

If you want a synchronous host-side bring-up path on Windows or POSIX, use
[`mcprotocol/serial/host_sync.hpp`](include/mcprotocol/serial/host_sync.hpp).
It wraps `PosixSerialPort` and `MelsecSerialClient` into a single blocking helper for:

- `cpu-model`
- `remote_run`
- `remote_stop`
- `remote_pause`
- `remote_latch_clear`
- `unlock_remote_password`
- `lock_remote_password`
- `clear_error_information`
- `remote_reset`
- contiguous `read_words`
- contiguous `read_bits`
- contiguous `write_words`
- contiguous `write_bits`
- sparse `random_read`
- sparse `random_write_words`
- sparse `random_write_bits`
- `register_monitor` and `read_monitor`

The normal workflow is:

1. Configure `MelsecSerialClient`
2. Start an async request
3. Send `pending_tx_frame()` with your UART layer
4. Call `notify_tx_complete()`
5. Feed received bytes through `on_rx_bytes()`
6. Call `poll()` for timeout handling

Smallest helper-based setup:

```cpp
#include <array>
#include <mcprotocol_serial.hpp>

mcprotocol::serial::ProtocolConfig config =
    mcprotocol::serial::highlevel::make_c4_binary_protocol();
config.route.station_no = 0;

mcprotocol::serial::BatchReadWordsRequest request {};
mcprotocol::serial::Status status =
    mcprotocol::serial::highlevel::make_batch_read_words_request("D100", 2, request);
```

Smallest synchronous host-side setup:

This snippet matches [host_sync_quickstart.cpp](examples/host_sync_quickstart.cpp). Treat it as an
example default, not as the authority for your target's validated serial settings.

```cpp
#include <mcprotocol_serial.hpp>

mcprotocol::serial::PosixSyncClient plc;
mcprotocol::serial::PosixSerialConfig serial {
#if defined(_WIN32)
    .device_path = "COM3",
#else
    .device_path = "/dev/ttyUSB0",
#endif
    .baud_rate = 19200,
    .data_bits = 8,
    .stop_bits = 2,
    .parity = 'E',
};

auto protocol = mcprotocol::serial::highlevel::make_c4_binary_protocol();
mcprotocol::serial::Status status = plc.open(serial, protocol);

std::array<std::uint16_t, 2> words {};
status = plc.read_words("D100", words);

std::uint32_t d100 = 0;
status = plc.random_read("D100", d100);
```

On Windows, pass `COM3`-style names. On POSIX hosts, pass `/dev/ttyUSB0`-style device paths.

Examples:

- [Examples Index](examples/README.md)
- [host_sync_quickstart.cpp](examples/host_sync_quickstart.cpp)
- [platformio_rpipico_arduino_uart.cpp](examples/platformio_rpipico_arduino_uart/platformio_rpipico_arduino_uart.cpp)
- [platformio_esp32c3_arduino_uart.cpp](examples/platformio_esp32c3_arduino_uart/platformio_esp32c3_arduino_uart.cpp)
- [platformio_arduino_mega2560_uart.cpp](examples/platformio_arduino_mega2560_uart/platformio_arduino_mega2560_uart.cpp)
- [mcu_async_batch_read.cpp](examples/mcu_async_batch_read.cpp)
- [platformio_rpipico_arduino_async.cpp](examples/platformio_rpipico_arduino_async/platformio_rpipico_arduino_async.cpp)
- [platformio_esp32c3_arduino_async.cpp](examples/platformio_esp32c3_arduino_async/platformio_esp32c3_arduino_async.cpp)

## PlatformIO

Version `0.2.0` includes PlatformIO packaging metadata and simpler host-side entrypoints.

Main files:

- `platformio.ini`
- `library.json`
- `library.properties`
- `include/mcprotocol_serial.hpp`

Available environments:

- `native-example`
- `rpipico-arduino-example`
- `esp32-c3-devkitm-1-example`
- `rpipico-arduino-uart-example`
- `esp32-c3-devkitm-1-uart-example`
- `mega2560-arduino-uart-example`
- `mega2560-arduino-uart-example-ultra-minimal`
- `native-example-ultra-minimal`
- `rpipico-arduino-example-ultra-minimal`
- `esp32-c3-devkitm-1-example-ultra-minimal`

Run them with:

```bash
pio run -e native-example
pio run -e rpipico-arduino-example
pio run -e esp32-c3-devkitm-1-example
pio run -e rpipico-arduino-uart-example
pio run -e esp32-c3-devkitm-1-uart-example
pio run -e mega2560-arduino-uart-example
pio run -e mega2560-arduino-uart-example-ultra-minimal
pio run -e native-example-ultra-minimal
pio run -e rpipico-arduino-example-ultra-minimal
pio run -e esp32-c3-devkitm-1-example-ultra-minimal
```

### Reduced profile

The normal PlatformIO examples already use a reduced-footprint profile.
That profile keeps batch read/write, `cpu-model`, and `loopback`, compiles out the other command families,
and also compiles the codec down to `4C + ASCII` only.

- `MelsecSerialClient`: about `18,984 bytes -> 2,168 bytes`
- `ESP32-C3 RAM`: `36,740 bytes -> 15,868 bytes`
- `ESP32-C3 Flash`: `289,914 bytes -> 264,024 bytes`
- `Mega 2560 RAM`: `7,717 bytes -> 6,695 bytes`
- `Mega 2560 Flash`: `25,420 bytes -> 20,806 bytes`

### Ultra-minimal profile

The `ultra-minimal` environments are for cases where you only want small batch read/write.
They also compile out `cpu-model` and `loopback`, and shrink the fixed frame/data buffers to `256 / 256 / 128` bytes.
Like the reduced profile, they keep only `4C + ASCII` in the codec.

- `MelsecSerialClient`: about `18,984 bytes -> 792 bytes`
- `ESP32-C3 RAM`: `36,740 bytes -> 14,508 bytes`
- `ESP32-C3 Flash`: `289,914 bytes -> 261,046 bytes`
- `Mega 2560 RAM`: `5,961 bytes -> 4,983 bytes`
- `Mega 2560 Flash`: `23,632 bytes -> 18,716 bytes`
- `RP2040 RAM`: `41,512 bytes`
- `RP2040 Flash`: `4,850 bytes`

### Build-time tuning macros

What can be compiled out:

- build-target unit
  CMake can drop host-side pieces as separate targets: `MCPROTOCOL_BUILD_HOST_SUPPORT=OFF` removes
  `host_sync` plus the host serial backend, and `MCPROTOCOL_BUILD_CLI=OFF` removes `mcprotocol_cli`
- buffer-capacity unit
  The fixed request / response / payload capacities are independent macros
- command-family unit
  Each feature switch below removes one whole command family from the codec and client surface
- codec-family unit
  You can remove whole code modes and whole frame families from the codec dispatch
- not yet a trim unit
  This repository does not yet cut individual commands inside one family, per-device paths, or
  per-PLC-series support as separate compile-time switches

Capacity tuning:

- `MCPROTOCOL_SERIAL_MAX_REQUEST_FRAME_BYTES`
- `MCPROTOCOL_SERIAL_MAX_RESPONSE_FRAME_BYTES`
- `MCPROTOCOL_SERIAL_MAX_REQUEST_DATA_BYTES`
- `MCPROTOCOL_SERIAL_MAX_RANDOM_ACCESS_ITEMS`
- `MCPROTOCOL_SERIAL_MAX_MULTI_BLOCK_COUNT`
- `MCPROTOCOL_SERIAL_MAX_MONITOR_ITEMS`
- `MCPROTOCOL_SERIAL_MAX_LOOPBACK_BYTES`

Feature switches:

- `MCPROTOCOL_SERIAL_ENABLE_RANDOM_COMMANDS`
- `MCPROTOCOL_SERIAL_ENABLE_MULTI_BLOCK_COMMANDS`
- `MCPROTOCOL_SERIAL_ENABLE_MONITOR_COMMANDS`
- `MCPROTOCOL_SERIAL_ENABLE_HOST_BUFFER_COMMANDS`
- `MCPROTOCOL_SERIAL_ENABLE_MODULE_BUFFER_COMMANDS`
- `MCPROTOCOL_SERIAL_ENABLE_CPU_MODEL_COMMANDS`
- `MCPROTOCOL_SERIAL_ENABLE_LOOPBACK_COMMANDS`

Codec switches:

- `MCPROTOCOL_SERIAL_ENABLE_ASCII_MODE`
- `MCPROTOCOL_SERIAL_ENABLE_BINARY_MODE`
- `MCPROTOCOL_SERIAL_ENABLE_FRAME_C4`
- `MCPROTOCOL_SERIAL_ENABLE_FRAME_C3`
- `MCPROTOCOL_SERIAL_ENABLE_FRAME_C2`
- `MCPROTOCOL_SERIAL_ENABLE_FRAME_C1`
- `MCPROTOCOL_SERIAL_ENABLE_FRAME_E1`

Practical examples:

- `MCPROTOCOL_SERIAL_ENABLE_RANDOM_COMMANDS=0`
  removes the whole random-read / random-write family
- `MCPROTOCOL_SERIAL_ENABLE_MONITOR_COMMANDS=0`
  removes `0801/0802` monitor as one family
- `MCPROTOCOL_SERIAL_ENABLE_BINARY_MODE=0`
  removes all binary codec branches at once
- `MCPROTOCOL_SERIAL_ENABLE_FRAME_C1=0`
  removes the whole `1C` frame family at once
- `MCPROTOCOL_SERIAL_ENABLE_FRAME_C4=1` with the other frame macros `=0`
  keeps only `4C`

### CMake feature profiles

The CMake build exposes the same footprint presets:

```bash
cmake -S . -B build-reduced -DMCPROTOCOL_FEATURE_PROFILE=reduced -DMCPROTOCOL_BUILD_HOST_SUPPORT=OFF -DMCPROTOCOL_BUILD_CLI=OFF -DMCPROTOCOL_BUILD_EXAMPLES=ON -DBUILD_TESTING=OFF
cmake -S . -B build-ultra -DMCPROTOCOL_FEATURE_PROFILE=ultra -DMCPROTOCOL_BUILD_HOST_SUPPORT=OFF -DMCPROTOCOL_BUILD_CLI=OFF -DMCPROTOCOL_BUILD_EXAMPLES=ON -DBUILD_TESTING=OFF
```

Profile behavior:

- `full`: complete host-oriented build, including host sync and CLI
- `reduced`: core-only build, smaller buffers, no random/multi-block/monitor/host-buffer/module-buffer, and codec limited to `4C + ASCII`
- `ultra`: reduced profile plus no `cpu-model`, no `loopback`, and the same `4C + ASCII` codec limit

Non-`full` profiles are treated as core-only by CMake, so host sync and CLI are turned off automatically.

## Docs and CI

Generate API docs:

```bash
cmake --build build --target docs
```

Or run Doxygen directly from the repository root:

```bash
doxygen Doxyfile
```

Check Markdown links:

```bash
cmake --build build --target check-markdown-links
```

Run both:

```bash
cmake --build build --target docs-all
```

GitHub Actions now verifies:

- host build
- `ctest`
- Doxygen generation
- Markdown link checks
- PlatformIO compile checks for the supported example environments
