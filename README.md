# MC Protocol Serial C++ Library

MC protocol serial library for MCU-oriented environments.

## Scope

- `4C` frame
  - ASCII `Format1`
  - ASCII `Format3`
  - ASCII `Format4`
  - Binary `Format5`
- `3C` frame
  - ASCII `Format1`
  - ASCII `Format3`
  - ASCII `Format4`
- Commands
  - Batch read/write
  - Random read/write
  - Multi-block read/write
  - Monitor register/read
  - Host buffer read/write
  - Intelligent module buffer read/write
  - Read CPU model
  - Loopback

## Design

- No exceptions
- No RTTI
- No dynamic allocation in the library
- Caller-owned buffers via `std::span`
- Transport-agnostic client state machine

The client exposes a transport-neutral workflow:

1. Configure `MelsecSerialClient`
2. Start an async operation
3. Send `pending_tx_frame()` with your UART / RS-485 layer
4. Call `notify_tx_complete()`
5. Feed received bytes back through `on_rx_bytes()`
6. Call `poll()` for response and inter-byte timeout handling

## Build

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
cmake --build build --target docs
```

If `doxygen` is installed system-wide, or if a local bundle is placed under `.tools/doxygen`, the `docs` target generates HTML documentation under `build/docs/doxygen/html`.

## PlatformIO

Version `0.1.1` adds PlatformIO packaging metadata and example environments.

Files:

- `platformio.ini`: local example environments
- `library.json`: PlatformIO library manifest
- `library.properties`: Arduino library metadata
- `include/mcprotocol_serial.hpp`: single-entry umbrella header

Capacity override macros are available when you need a smaller MCU footprint:

- `MCPROTOCOL_SERIAL_MAX_REQUEST_FRAME_BYTES`
- `MCPROTOCOL_SERIAL_MAX_RESPONSE_FRAME_BYTES`
- `MCPROTOCOL_SERIAL_MAX_REQUEST_DATA_BYTES`
- `MCPROTOCOL_SERIAL_MAX_RANDOM_ACCESS_ITEMS`
- `MCPROTOCOL_SERIAL_MAX_MULTI_BLOCK_COUNT`
- `MCPROTOCOL_SERIAL_MAX_MONITOR_ITEMS`
- `MCPROTOCOL_SERIAL_MAX_LOOPBACK_BYTES`

Command-family feature switches are also available when you want to compile out unused code paths:

- `MCPROTOCOL_SERIAL_ENABLE_RANDOM_COMMANDS`
- `MCPROTOCOL_SERIAL_ENABLE_MULTI_BLOCK_COMMANDS`
- `MCPROTOCOL_SERIAL_ENABLE_MONITOR_COMMANDS`
- `MCPROTOCOL_SERIAL_ENABLE_HOST_BUFFER_COMMANDS`
- `MCPROTOCOL_SERIAL_ENABLE_MODULE_BUFFER_COMMANDS`
- `MCPROTOCOL_SERIAL_ENABLE_CPU_MODEL_COMMANDS`
- `MCPROTOCOL_SERIAL_ENABLE_LOOPBACK_COMMANDS`

Available example environments:

- `native-example`
- `rpipico-arduino-example`
- `esp32-c3-devkitm-1-example`

The PlatformIO example environments intentionally apply a reduced-capacity plus reduced-feature profile for MCU footprint checks.
The profile keeps batch read/write, CPU-model, and loopback support, and compiles out random, multi-block, monitor, host-buffer, and module-buffer command families:

- `MelsecSerialClient`: about `18,984 bytes -> 2,168 bytes`
- `ESP32-C3 sample RAM`: `36,740 bytes -> 15,868 bytes`
- `ESP32-C3 sample Flash`: `289,914 bytes -> 264,024 bytes`

Compile-checked on `2026-04-10`:

- `native-example`: pass
- `rpipico-arduino-example`: pass
- `esp32-c3-devkitm-1-example`: pass

Examples:

```bash
pio run -e native-example
pio run -e rpipico-arduino-example
pio run -e esp32-c3-devkitm-1-example
```

## CLI

Linux host testing CLI is available as `mcprotocol_cli`.

Examples:

```bash
./build/mcprotocol_cli --device /dev/ttyUSB0 cpu-model
./build/mcprotocol_cli --device /dev/ttyUSB0 loopback ABCDE
./build/mcprotocol_cli --device /dev/ttyUSB0 read-words D100 4
./build/mcprotocol_cli --device /dev/ttyUSB0 read-bits M100 8
./build/mcprotocol_cli --device /dev/ttyUSB0 read-host-buffer 0 16
./build/mcprotocol_cli --device /dev/ttyUSB0 read-module-buffer 0 64 0
./build/mcprotocol_cli --device /dev/ttyUSB0 write-host-buffer 0 0x1234
./build/mcprotocol_cli --device /dev/ttyUSB0 write-module-buffer 0 0 0x12 0x34
./build/mcprotocol_cli --device /dev/ttyUSB0 write-words D100=123 D101=456
./build/mcprotocol_cli --device /dev/ttyUSB0 write-bits M100=1 Y2F=0
./build/mcprotocol_cli --device /dev/ttyUSB0 random-write-words D100=123 D105=456
./build/mcprotocol_cli --device /dev/ttyUSB0 random-write-bits M100=1 M105=0
./build/mcprotocol_cli --device /dev/ttyUSB0 probe-all
./build/mcprotocol_cli --device /dev/ttyUSB0 probe-write-all
./build/mcprotocol_cli --device /dev/ttyUSB0 probe-multi-block
./build/mcprotocol_cli --device /dev/ttyUSB0 probe-monitor
./build/mcprotocol_cli --device /dev/ttyUSB0 probe-host-buffer
./build/mcprotocol_cli --device /dev/ttyUSB0 probe-module-buffer
./build/mcprotocol_cli --device /dev/ttyUSB0 probe-write-host-buffer
./build/mcprotocol_cli --device /dev/ttyUSB0 probe-write-module-buffer
```

## Documentation

This repository now follows the same documentation layout used in the companion SLMP minimal repository.

- `docsrc/user/SETUP_GUIDE.md`: bring-up steps and the verified RJ71C24-R2 serial settings
- `docsrc/user/USAGE_GUIDE.md`: command usage, native vs emulated behavior, and recommended CLI flow
- `examples/README.md`: compile-checked MCU-oriented sample programs
- `platformio.ini`: PlatformIO example entry points for native, RP2040, and ESP32-C3
- `docsrc/validation/reports/HARDWARE_VALIDATION.md`: current PASS / NG / HOLD matrix and validation backlog
- `docsrc/validation/reports/RJ71C24_R2_RS232C_FORMAT4_2026-04-10.md`: dated real-hardware evidence log for the current setup
- `docsrc/maintainer/DEVELOPER_NOTES.md`: maintainer notes for request-shape conformance, fallback policy, and future follow-up

Verified on real hardware:

- Module: `RJ71C24-R2`
- Link: `RS-232C`
- Serial: `19200bps / 8E1`
- Protocol: `MC Protocol Format4 (ASCII, CR/LF, sum-check off)`
- Verified commands: `cpu-model`, `loopback`, `read-words`, `read-bits`, `read-host-buffer`, `read-module-buffer`, `write-host-buffer`, `write-module-buffer`, `write-words`, `write-bits`, `random-read`, `random-write-words`, `random-write-bits`, `probe-multi-block`, `probe-monitor`

Notes:

- On this `RJ71C24-R2` setup, native `0403 random read` returns `0x7F22`.
- On this `RJ71C24-R2` setup, native `1402 random write words` returns `0x7F22`, and native `1402 random write bits` returns `0x7F23`.
- On this `RJ71C24-R2` setup, native `0406 multi-block read`, `1406 multi-block write`, and `0801 monitor registration` return `0x7F22`.
- `mcprotocol_cli random-read` automatically falls back to repeated batch reads so mixed/non-consecutive device reads still work.
- `mcprotocol_cli random-write-words` and `random-write-bits` automatically fall back to repeated batch writes when native `1402` is rejected by the module.
- `mcprotocol_cli write-words` and `write-bits` automatically split large contiguous ranges across multiple batch-write frames to stay within the fixed request buffer.
- `mcprotocol_cli probe-all` performs a read-only probe of all 27 supported device-code families with address `0`.
- `mcprotocol_cli probe-write-all` performs `read -> write test value -> verify -> restore` against address `0` of each supported device family.
- `mcprotocol_cli probe-multi-block` falls back to repeated batch read/write per block when native `0406/1406` is rejected, then restores the original values.
- `mcprotocol_cli probe-monitor` falls back to repeated direct reads when native `0801/0802` is rejected.
- `mcprotocol_cli probe-host-buffer` reads host buffer address `0` for `1` word.
- `mcprotocol_cli probe-module-buffer` reads module buffer address `0` for `2` bytes with `module=0`.
- `mcprotocol_cli probe-write-host-buffer` performs `read -> write test value -> verify -> restore` at host buffer address `0`.
- `mcprotocol_cli probe-write-module-buffer` performs `read -> write test value -> verify -> restore` at module buffer start `0`, `module=0`, `bytes=2`.
- Unit tests pin native `1402`, `0406`, `0801`, and `0802` request-data layouts against the official MC protocol reference examples or their documented request structure.
- Real-hardware emulation checks completed: `random-write-words D300/D305 -> verify -> restore`, `random-write-bits M300/M305 -> verify -> restore`, `probe-multi-block` with `0406/1406` fallback, and `probe-monitor` with `0801/0802` fallback.
- Real-hardware stress checks completed: `read-words 960`, `read-bits 3584`, `write-words 960`, `write-bits 3584`, `100-bit / 1 minute`, `100-word / 30 minutes`, `read-host-buffer 1/16/64/480`, `read-module-buffer 2/64/512/1920`, `host-buffer 480 + module-buffer 1920 / 1 minute`, `host/module buffer write-soak 64 cycles / 60 seconds`, `mixed supported-command soak 28 cycles / 61 seconds`, `extended mixed supported-command soak 140 cycles / 301 seconds`, `unsupported 1402 -> read-words recovery x20`, and `unsupported multi-block -> cpu-model recovery x10`.
- For the exact PASS / NG / HOLD matrix, see `docsrc/validation/reports/HARDWARE_VALIDATION.md`.

Example with the verified settings:

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
  read-words D100 2
```

For RS-485 adapters that need manual RTS direction control:

```bash
./build/mcprotocol_cli --device /dev/ttyUSB0 --rts-toggle on cpu-model
```
