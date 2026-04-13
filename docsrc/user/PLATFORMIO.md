# PlatformIO

Version `0.2.3` trims the registry tarball to the library-facing files while keeping the PlatformIO packaging metadata and simpler host-side entrypoints.

## Main Files

- `platformio.ini`
- `library.json`
- `library.properties`
- `include/mcprotocol_serial.hpp`

## Available Environments

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

## Reduced Profile

The normal PlatformIO examples already use a reduced-footprint profile.
That profile keeps batch read/write, `cpu-model`, and `loopback`, compiles out the other command
families, and also compiles the codec down to `4C + ASCII` only.

- `MelsecSerialClient`: about `18,984 bytes -> 2,168 bytes`
- `ESP32-C3 RAM`: `36,740 bytes -> 15,868 bytes`
- `ESP32-C3 Flash`: `289,914 bytes -> 264,024 bytes`
- `Mega 2560 RAM`: `7,717 bytes -> 6,695 bytes`
- `Mega 2560 Flash`: `25,420 bytes -> 20,806 bytes`

## Ultra-minimal Profile

The `ultra-minimal` environments are for cases where you only want small batch read/write.
They also compile out `cpu-model` and `loopback`, and shrink the fixed frame/data buffers to
`256 / 256 / 128` bytes. Like the reduced profile, they keep only `4C + ASCII` in the codec.

- `MelsecSerialClient`: about `18,984 bytes -> 792 bytes`
- `ESP32-C3 RAM`: `36,740 bytes -> 14,508 bytes`
- `ESP32-C3 Flash`: `289,914 bytes -> 261,046 bytes`
- `Mega 2560 RAM`: `5,961 bytes -> 4,983 bytes`
- `Mega 2560 Flash`: `23,632 bytes -> 18,716 bytes`
- `RP2040 RAM`: `41,512 bytes`
- `RP2040 Flash`: `4,850 bytes`

## Build-time Tuning Macros

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

## CMake Feature Profiles

The CMake build exposes the same footprint presets:

```bash
cmake -S . -B build-reduced -DMCPROTOCOL_FEATURE_PROFILE=reduced -DMCPROTOCOL_BUILD_HOST_SUPPORT=OFF -DMCPROTOCOL_BUILD_CLI=OFF -DMCPROTOCOL_BUILD_EXAMPLES=ON -DBUILD_TESTING=OFF
cmake -S . -B build-ultra -DMCPROTOCOL_FEATURE_PROFILE=ultra -DMCPROTOCOL_BUILD_HOST_SUPPORT=OFF -DMCPROTOCOL_BUILD_CLI=OFF -DMCPROTOCOL_BUILD_EXAMPLES=ON -DBUILD_TESTING=OFF
```

Profile behavior:

- `full`: complete host-oriented build, including host sync and CLI
- `reduced`: core-only build, smaller buffers, no random/multi-block/monitor/host-buffer/module-buffer, and codec limited to `4C + ASCII`
- `ultra`: reduced profile plus no `cpu-model`, no `loopback`, and the same `4C + ASCII` codec limit

Non-`full` profiles are treated as core-only by CMake, so host sync and CLI are turned off
automatically. If `BUILD_TESTING` is left `ON`, CMake also disables it automatically for these
profiles.
