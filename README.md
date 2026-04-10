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
- Real-hardware validation records for `RJ71C24-R2`

## Start Here

If you are new to this repository, do this first.

### 1. Build

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

### 2. Try the verified serial settings

The following settings were verified on real hardware:

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

### 3. Read and write a PLC device

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
  write-words D100=123 D101=456
```

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
  write-bits M100=1 M101=0
```

## What Works Today

### Library scope

- `4C` frame
  - ASCII `Format1`
  - ASCII `Format3`
  - ASCII `Format4`
  - Binary `Format5`
- `3C` frame
  - ASCII `Format1`
  - ASCII `Format3`
  - ASCII `Format4`

### Command support in the codebase

- Batch read/write
- Random read/write
- Multi-block read/write
- Monitor register/read
- Host buffer read/write
- Intelligent module buffer read/write
- Read CPU model
- Loopback

### Verified on real hardware

The following command flows were verified on `RJ71C24-R2` with the settings above:

- `cpu-model`
- `loopback`
- `read-words`
- `read-bits`
- `write-words`
- `write-bits`
- `read-host-buffer`
- `write-host-buffer`
- `read-module-buffer`
- `write-module-buffer`
- `random-read`
- `random-write-words`
- `random-write-bits`
- `probe-multi-block`
- `probe-monitor`

Stress tests completed on real hardware:

- `read-words 960`
- `write-words 960`
- `read-bits 3584`
- `write-bits 3584`
- `100-bit / 1 minute`
- `100-word / 30 minutes`
- `mixed supported-command soak 301 seconds`

For the exact PASS / NG / HOLD matrix, see [HARDWARE_VALIDATION.md](/home/user/myproject/OTHER/plc-comm-mcprotocol-serial-cpp/docsrc/validation/reports/HARDWARE_VALIDATION.md).

## Current Limits

These are important if you are using `RJ71C24-R2`.

- Native `0403 random read` returns `0x7F22` on the verified setup
- Native `1402 random write words` returns `0x7F22`
- Native `1402 random write bits` returns `0x7F23`
- Native `0406 multi-block read` returns `0x7F22`
- Native `1406 multi-block write` returns `0x7F22`
- Native `0801 monitor registration` returns `0x7F22`

The CLI works around these module-specific limits:

- `random-read` falls back to repeated batch reads
- `random-write-words` and `random-write-bits` fall back to repeated batch writes
- `probe-multi-block` falls back to repeated batch read/write
- `probe-monitor` falls back to repeated direct reads
- Large contiguous `write-words` and `write-bits` are split automatically to fit fixed request buffers

## For MCU Users

The library design is intentionally simple:

- No exceptions
- No RTTI
- No dynamic allocation in the library
- Caller-owned buffers via `std::span`
- Transport-agnostic client state machine

The normal workflow is:

1. Configure `MelsecSerialClient`
2. Start an async request
3. Send `pending_tx_frame()` with your UART layer
4. Call `notify_tx_complete()`
5. Feed received bytes through `on_rx_bytes()`
6. Call `poll()` for timeout handling

Examples:

- [examples/README.md](/home/user/myproject/OTHER/plc-comm-mcprotocol-serial-cpp/examples/README.md)
- [mcu_async_batch_read.cpp](/home/user/myproject/OTHER/plc-comm-mcprotocol-serial-cpp/examples/mcu_async_batch_read.cpp)
- [platformio_arduino_async.cpp](/home/user/myproject/OTHER/plc-comm-mcprotocol-serial-cpp/examples/platformio_arduino_async/platformio_arduino_async.cpp)

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
- `native-example-ultra-minimal`
- `rpipico-arduino-example-ultra-minimal`
- `esp32-c3-devkitm-1-example-ultra-minimal`

Run them with:

```bash
pio run -e native-example
pio run -e rpipico-arduino-example
pio run -e esp32-c3-devkitm-1-example
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
./build/mcprotocol_cli --device /dev/ttyUSB0 probe-all
./build/mcprotocol_cli --device /dev/ttyUSB0 probe-write-all
```

If your adapter is `RS-485` and needs manual direction control:

```bash
./build/mcprotocol_cli --device /dev/ttyUSB0 --rts-toggle on cpu-model
```

## Documentation Map

This repository uses the same documentation layout style as the companion SLMP minimal repository.

- [SETUP_GUIDE.md](/home/user/myproject/OTHER/plc-comm-mcprotocol-serial-cpp/docsrc/user/SETUP_GUIDE.md): setup and verified serial settings
- [USAGE_GUIDE.md](/home/user/myproject/OTHER/plc-comm-mcprotocol-serial-cpp/docsrc/user/USAGE_GUIDE.md): CLI usage and fallback behavior
- [examples/README.md](/home/user/myproject/OTHER/plc-comm-mcprotocol-serial-cpp/examples/README.md): example programs
- [HARDWARE_VALIDATION.md](/home/user/myproject/OTHER/plc-comm-mcprotocol-serial-cpp/docsrc/validation/reports/HARDWARE_VALIDATION.md): PASS / NG / HOLD matrix
- [RJ71C24_R2_RS232C_FORMAT4_2026-04-10.md](/home/user/myproject/OTHER/plc-comm-mcprotocol-serial-cpp/docsrc/validation/reports/RJ71C24_R2_RS232C_FORMAT4_2026-04-10.md): dated real-hardware log
- [DEVELOPER_NOTES.md](/home/user/myproject/OTHER/plc-comm-mcprotocol-serial-cpp/docsrc/maintainer/DEVELOPER_NOTES.md): implementation notes and follow-up items

## Documentation Build

```bash
cmake --build build --target docs
```

If `doxygen` is installed system-wide, or if a local bundle is placed under `.tools/doxygen`, HTML is generated under `build/docs/doxygen/html`.
