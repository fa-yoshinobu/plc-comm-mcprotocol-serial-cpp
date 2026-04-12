# MC Protocol Serial C++ Library

[![ci](https://github.com/fa-yoshinobu/plc-comm-mcprotocol-serial-cpp/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/fa-yoshinobu/plc-comm-mcprotocol-serial-cpp/actions/workflows/ci.yml)
[![release](https://github.com/fa-yoshinobu/plc-comm-mcprotocol-serial-cpp/actions/workflows/release.yml/badge.svg)](https://github.com/fa-yoshinobu/plc-comm-mcprotocol-serial-cpp/actions/workflows/release.yml)

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

If you are new to this repository, do this first.

### 1. Build

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

### 2. Try the smallest library entrypoint

```bash
cmake --build build --target mcprotocol_example_host_sync
./build/mcprotocol_example_host_sync
```

That example uses the synchronous host-side facade in
[host_sync_quickstart.cpp](examples/host_sync_quickstart.cpp).

### 3. Choose your entry path

- If you want the simplest library example on a host machine, start with
  [host_sync_quickstart.cpp](examples/host_sync_quickstart.cpp).
- If you want the low-level MCU state machine, start with [MCU Quickstart](docsrc/user/MCU_QUICKSTART.md).
- If you need verified target settings and current limits, use
  [HARDWARE_VALIDATION.md](docsrc/validation/reports/HARDWARE_VALIDATION.md).

## Documentation Map

Start with these pages instead of reading the whole repository at once.

- [Wiring Guide](docsrc/user/WIRING_GUIDE.md)
- [MCU Quickstart](docsrc/user/MCU_QUICKSTART.md)
- [FAQ](docsrc/user/FAQ.md)
- [Examples Index](examples/README.md)
- [Hardware Validation Matrix](docsrc/validation/reports/HARDWARE_VALIDATION.md)
- [Footprint Profiles](docsrc/validation/reports/FOOTPRINT_PROFILES.md)
- [Developer Notes](docsrc/maintainer/DEVELOPER_NOTES.md)
- [Manual Command Coverage](docsrc/maintainer/MANUAL_COMMAND_COVERAGE.md)
- [TODO / Current Follow-up](docsrc/maintainer/TODO.md)
- [Native Command Backlog](docsrc/maintainer/NATIVE_COMMAND_BACKLOG.md)
- [Release Process](docsrc/maintainer/RELEASE_PROCESS.md)
- [Changelog](CHANGELOG.md)

## What Works Today

- The library supports MC protocol request/response handling for `2C`, `3C`, `4C`, and initial
  `1E` frame families. `2C` is ASCII-only. `1E` currently supports the chapter-18
  device-memory / block-addressed extended-file-register / special-function-module subset in both
  ASCII and binary on `--series a` and `--series qna`, plus direct extended-file-register
  read/write on `--series a`.
- The codebase also has initial `1C` ASCII support on `--series a` and `--series qna` for
  contiguous device-memory read/write, random write, monitor, module-buffer access,
  extended-file register access, and loopback.
- The codebase includes contiguous, random, multi-block, monitor, host-buffer, module-buffer,
  CPU-model, remote RUN/STOP/PAUSE/latch-clear, password lock/unlock, error-clear, remote-reset,
  loopback, user-frame registration/read/delete, C24 global-signal control, transmission-sequence
  initialization, and CPU-monitoring deregistration command paths.
- Real-hardware validation now covers `RJ71C24-R2`, `LJ71C24`, `QJ71C24N`, and `FX5UC-32MT/D`.
- There are currently no active command-family holds on the validated targets.
- Contiguous access is validated on all current targets.
- Random, multi-block, and monitor are validated where the target and `--series` combination supports them.
- Qualified helper access over `0601/1601` is the supported public `U...` path.

For the exact PASS / status matrix and the verified serial settings for each target, see
[HARDWARE_VALIDATION.md](docsrc/validation/reports/HARDWARE_VALIDATION.md).

## Current Limits

- This repository does not implement the full `sh080008ab` command list. Use
  [MANUAL_COMMAND_COVERAGE.md](docsrc/maintainer/MANUAL_COMMAND_COVERAGE.md) for the exact
  implemented vs. missing command families.
- `1C` support is currently limited to contiguous device-memory read/write, random write, monitor,
  module-buffer access, extended-file register access, and loopback. ACPU-common `ER/EW/ET/EM/ME`
  is available on `PlcSeries::A`, and QnA-common direct `NR/NW` is available on
  `PlcSeries::QnA`.
- `1E` support is currently limited to chapter `18`: contiguous device-memory read/write, random
  write, monitor register/read, block-addressed extended-file-register
  read/write/random-write/monitor on `--series a` and `--series qna`, direct extended-file-register
  read/write on `--series a`, and special-function-module buffer read/write. It does not expose
  CPU-model, host-buffer, remote control, password/error control, loopback, multi-block,
  qualified helper, or link-direct paths.
- Some command families are target-dependent and require the right `--series` selection.
- Native qualified access is not part of the supported library workflow. Keep `U...` access on the
  helper path only.
- `FX5UC-32MT/D` treats `0613/1613/0601/1601` and `0801/0802` as unsupported / not applicable on
  serial `3C/4C`; qualified access is also not available there.
- Large contiguous `write-words` and `write-bits` are still split automatically to fit fixed request buffers.
- The remaining repository follow-up is implementation work for `1612`, `0630`, and `2101`, plus
  parameter-dependent `1006 remote-reset` behavior on `RJ71C24-R2 + R120PCPU`.

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
    .stop_bits = 1,
    .parity = 'E',
};

auto protocol = mcprotocol::serial::highlevel::make_c4_ascii_format4_protocol();
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
pio run -e native-example-ultra-minimal
pio run -e rpipico-arduino-example-ultra-minimal
pio run -e esp32-c3-devkitm-1-example-ultra-minimal
```

### Reduced profile

The normal PlatformIO examples already use a reduced-footprint profile.
That profile keeps batch read/write, `cpu-model`, and `loopback`, and compiles out the other command families.

- `MelsecSerialClient`: about `18,984 bytes -> 2,168 bytes`
- `ESP32-C3 RAM`: `36,740 bytes -> 15,868 bytes`
- `ESP32-C3 Flash`: `289,914 bytes -> 264,024 bytes`

### Ultra-minimal profile

The `ultra-minimal` environments are for cases where you only want small batch read/write.
They also compile out `cpu-model` and `loopback`, and shrink the fixed frame/data buffers to `256 / 256 / 128` bytes.

- `MelsecSerialClient`: about `18,984 bytes -> 792 bytes`
- `ESP32-C3 RAM`: `36,740 bytes -> 14,508 bytes`
- `ESP32-C3 Flash`: `289,914 bytes -> 261,046 bytes`
- `RP2040 RAM`: `41,512 bytes`
- `RP2040 Flash`: `4,850 bytes`

### Build-time tuning macros

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
