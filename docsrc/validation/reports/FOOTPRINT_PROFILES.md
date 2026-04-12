# Footprint Profiles

This page summarizes the memory-footprint measurements captured on the current reduced and ultra-minimal profiles.

## Measured Profiles

| Profile | MelsecSerialClient size | ESP32-C3 RAM | ESP32-C3 Flash | Mega 2560 RAM | Mega 2560 Flash | RP2040 RAM | RP2040 Flash |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Full reference sample | `18,984 B` | `36,740 B` | `289,914 B` | - | - | - | - |
| Reduced PlatformIO profile | `2,168 B` | `15,868 B` | `264,024 B` | `6,695 B` | `20,806 B` | - | - |
| Ultra-minimal PlatformIO profile | `792 B` | `14,508 B` | `261,046 B` | `4,983 B` | `18,716 B` | `41,512 B` | `4,850 B` |

## How The Reduced Profile Shrinks

The reduced profile:

- trims by command-family unit and codec-family unit, not by individual command
- shrinks the fixed request / response / data buffers
- compiles out random, multi-block, monitor, host-buffer, and module-buffer command families
- compiles out binary, `3C`, `2C`, `1C`, and `1E` codec branches
- keeps batch read/write, `cpu-model`, and `loopback`

## How The Ultra-minimal Profile Shrinks Further

The ultra-minimal profile also:

- compiles out `cpu-model`
- compiles out `loopback`
- shrinks the fixed request / response / data buffers to `256 / 256 / 128`

## Trimming Units

Current compile-time trimming works at these units:

- target unit
  CMake can drop host support and CLI as separate build targets
- buffer-capacity unit
  Fixed frame / payload sizes are independent macros
- command-family unit
  random, multi-block, monitor, host-buffer, module-buffer, CPU-model, and loopback each have their
  own switch
- codec-family unit
  ASCII vs binary and `4C` / `3C` / `2C` / `1C` / `1E` each have their own switch

Current compile-time trimming does not work at these units:

- individual commands inside a family
  for example, you cannot keep only `0403` and drop `1402`
- per-device paths inside one family
  for example, you cannot compile out only `Jn\\...` or only `U...`
- per-series support
  `IQ-R`, `Q/L`, `QnA`, and `A` are still runtime protocol choices, not separate build switches

## Where The Settings Live

- `platformio.ini`
- `include/mcprotocol/serial/types.hpp`
- `CMakeLists.txt` through `MCPROTOCOL_FEATURE_PROFILE`, `MCPROTOCOL_BUILD_HOST_SUPPORT`, and `MCPROTOCOL_BUILD_CLI`
