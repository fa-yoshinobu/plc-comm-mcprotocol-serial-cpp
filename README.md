# MC Protocol Serial C++ Library

MC protocol serial library for MCU-oriented environments.

This repository is for cases like these:

- You want to talk to a Mitsubishi PLC over `serial / RS-232C / RS-485`
- You want a `C++` library that does not allocate dynamically
- You want to run the same core logic on `Linux`, `RP2040`, or `ESP32-C3`

The current codebase includes:

- A transport-agnostic library centered on `MelsecSerialClient`
- A Linux test CLI: `mcprotocol_cli`
- PlatformIO example projects for `RP2040` and `ESP32-C3`
- A read-only real-UART Arduino sample for `Serial1`
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

### 2. Try a verified serial setting

One verified starting point is:

- Module: `RJ71C24-R2`
- Link: `RS-232C`
- Serial: `19200bps / 8E1`
- Protocol: `MC Protocol Format4`
- Code: `ASCII`
- Terminator: `CR/LF`
- Sum check: `off`
- Station: `0`

First command to try:

```bash
./build/mcprotocol_cli \
  --device /dev/ttyUSB0 \
  --baud 19200 \
  --data-bits 8 \
  --stop-bits 1 \
  --parity E \
  --frame c4-ascii-f4 \
  --sum-check off \
  --station 0 \
  cpu-model
```

### 3. Choose your entry path

- If you want shell commands on Linux, start with [Usage Guide](docsrc/user/USAGE_GUIDE.md).
- If you want the low-level MCU state machine, start with [MCU Quickstart](docsrc/user/MCU_QUICKSTART.md).
- If you want a simpler host-side synchronous wrapper, start with
  [host_sync_quickstart.cpp](examples/host_sync_quickstart.cpp).
- If you need verified target settings and current limits, use
  [HARDWARE_VALIDATION.md](docsrc/validation/reports/HARDWARE_VALIDATION.md).

## Documentation Map

Start with these pages instead of reading the whole repository at once.

- [Setup Guide](docsrc/user/SETUP_GUIDE.md)
- [Wiring Guide](docsrc/user/WIRING_GUIDE.md)
- [MCU Quickstart](docsrc/user/MCU_QUICKSTART.md)
- [Usage Guide](docsrc/user/USAGE_GUIDE.md)
- [Troubleshooting](docsrc/user/TROUBLESHOOTING.md)
- [FAQ](docsrc/user/FAQ.md)
- [Examples Index](examples/README.md)
- [Hardware Validation Matrix](docsrc/validation/reports/HARDWARE_VALIDATION.md)
- [Footprint Profiles](docsrc/validation/reports/FOOTPRINT_PROFILES.md)
- [Developer Notes](docsrc/maintainer/DEVELOPER_NOTES.md)
- [Native Command Backlog](docsrc/maintainer/NATIVE_COMMAND_BACKLOG.md)
- [Release Process](docsrc/maintainer/RELEASE_PROCESS.md)
- [Changelog](CHANGELOG.md)

## What Works Today

- The library supports serial MC protocol request/response handling for `3C` and `4C` frame families.
- The codebase includes contiguous, random, multi-block, monitor, host-buffer, module-buffer,
  CPU-model, and loopback command paths.
- Real-hardware validation now covers `RJ71C24-R2`, `LJ71C24`, `QJ71C24N`, and `FX5UC-32MT/D`.
- Contiguous access is validated on all current targets.
- Random, multi-block, and monitor are validated where the target and `--series` combination supports them.
- Qualified helper access over `0601/1601` is the practical public `U...` path.

For the exact PASS / HOLD matrix and the verified serial settings for each target, see
[HARDWARE_VALIDATION.md](docsrc/validation/reports/HARDWARE_VALIDATION.md).

## Current Limits

- Some command families are target-dependent and require the right `--series` selection.
- Native qualified access remains unresolved or semantically inconsistent on several targets.
- `FX5UC-32MT/D` has additional target-specific limits around host/module buffer and qualified access.
- The CLI does not silently fall back from one native family to a different one when a native
  probe fails.
- Large contiguous `write-words` and `write-bits` are still split automatically to fit fixed request buffers.

For target-specific limits, use
[HARDWARE_VALIDATION.md](docsrc/validation/reports/HARDWARE_VALIDATION.md) and
[NATIVE_COMMAND_BACKLOG.md](docsrc/maintainer/NATIVE_COMMAND_BACKLOG.md).

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
- request builders for common contiguous and sparse operations

If you are on Linux and want a synchronous host-side bring-up path, use
[`mcprotocol/serial/host_sync.hpp`](include/mcprotocol/serial/host_sync.hpp).
It wraps `PosixSerialPort` and `MelsecSerialClient` into a single blocking helper for:

- `cpu-model`
- contiguous `read_words`
- contiguous `read_bits`
- contiguous `write_words`
- contiguous `write_bits`

The normal workflow is:

1. Configure `MelsecSerialClient`
2. Start an async request
3. Send `pending_tx_frame()` with your UART layer
4. Call `notify_tx_complete()`
5. Feed received bytes through `on_rx_bytes()`
6. Call `poll()` for timeout handling

Smallest helper-based setup:

```cpp
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
    .device_path = "/dev/ttyUSB0",
    .baud_rate = 19200,
    .data_bits = 8,
    .stop_bits = 1,
    .parity = 'E',
};

auto protocol = mcprotocol::serial::highlevel::make_c4_ascii_format4_protocol();
mcprotocol::serial::Status status = plc.open(serial, protocol);
```

Examples:

- [Examples Index](examples/README.md)
- [mcu_async_batch_read.cpp](examples/mcu_async_batch_read.cpp)
- [host_sync_quickstart.cpp](examples/host_sync_quickstart.cpp)
- [platformio_arduino_async.cpp](examples/platformio_arduino_async/platformio_arduino_async.cpp)
- [platformio_arduino_uart.cpp](examples/platformio_arduino_uart/platformio_arduino_uart.cpp)
- [Linux CLI examples](examples/linux_cli/README.md)

## PlatformIO

Version `0.1.1` adds PlatformIO packaging metadata and example environments.

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

## CLI Notes

The Linux host CLI is intended for bring-up and validation.

Useful commands:

```bash
./build/mcprotocol_cli --device /dev/ttyUSB0 cpu-model
./build/mcprotocol_cli --device /dev/ttyUSB0 read-words D100 4
./build/mcprotocol_cli --device /dev/ttyUSB0 read-bits M100 8
./build/mcprotocol_cli --device /dev/ttyUSB0 write-words D100=123 D101=456
./build/mcprotocol_cli --device /dev/ttyUSB0 write-bits M100=1 M101=0
./build/mcprotocol_cli --device /dev/ttyUSB0 --frame c4-ascii-f4 recover-c24
./build/mcprotocol_cli --device /dev/ttyUSB0 probe-all
./build/mcprotocol_cli --device /dev/ttyUSB0 probe-write-all
```

If your adapter is `RS-485` and needs manual direction control:

```bash
./build/mcprotocol_cli --device /dev/ttyUSB0 --rts-toggle on cpu-model
```

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
