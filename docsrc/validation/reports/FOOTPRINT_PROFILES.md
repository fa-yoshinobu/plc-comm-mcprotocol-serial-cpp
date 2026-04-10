# Footprint Profiles

This page summarizes the memory-footprint measurements captured during the `0.1.1` work.

## Measured Profiles

| Profile | MelsecSerialClient size | ESP32-C3 RAM | ESP32-C3 Flash | RP2040 RAM | RP2040 Flash |
| --- | ---: | ---: | ---: | ---: | ---: |
| Full reference sample | `18,984 B` | `36,740 B` | `289,914 B` | - | - |
| Reduced PlatformIO profile | `2,168 B` | `15,868 B` | `264,024 B` | - | - |
| Ultra-minimal PlatformIO profile | `792 B` | `14,508 B` | `261,046 B` | `41,512 B` | `4,850 B` |

## How The Reduced Profile Shrinks

The reduced profile:

- shrinks the fixed request / response / data buffers
- compiles out random, multi-block, monitor, host-buffer, and module-buffer command families
- keeps batch read/write, `cpu-model`, and `loopback`

## How The Ultra-minimal Profile Shrinks Further

The ultra-minimal profile also:

- compiles out `cpu-model`
- compiles out `loopback`
- shrinks the fixed request / response / data buffers to `256 / 256 / 128`

## Where The Settings Live

- `platformio.ini`
- `include/mcprotocol/serial/types.hpp`
