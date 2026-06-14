[![Documentation](https://img.shields.io/badge/docs-GitHub_Pages-blue.svg)](https://fa-yoshinobu.github.io/plc-comm-mcprotocol-serial-cpp/)
[![Release](https://img.shields.io/github/v/release/fa-yoshinobu/plc-comm-mcprotocol-serial-cpp?label=release)](https://github.com/fa-yoshinobu/plc-comm-mcprotocol-serial-cpp/releases/latest)
[![CI](https://img.shields.io/github/actions/workflow/status/fa-yoshinobu/plc-comm-mcprotocol-serial-cpp/ci.yml?branch=main&label=CI&logo=github)](https://github.com/fa-yoshinobu/plc-comm-mcprotocol-serial-cpp/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![PlatformIO Registry](https://badges.registry.platformio.org/packages/fa-yoshinobu/library/mcprotocol-serial-cpp.svg)](https://registry.platformio.org/libraries/fa-yoshinobu/mcprotocol-serial-cpp)
[![Lint: PIO Check](https://img.shields.io/badge/Lint-PIO%20Check-blue.svg)](https://docs.platformio.org/en/latest/core/userguide/cmd_check.html)
[![pages](https://github.com/fa-yoshinobu/plc-comm-mcprotocol-serial-cpp/actions/workflows/pages.yml/badge.svg)](https://github.com/fa-yoshinobu/plc-comm-mcprotocol-serial-cpp/actions/workflows/pages.yml)
[![Python](https://img.shields.io/badge/Python-3776AB?logo=python&logoColor=white)](https://www.python.org/)
[![C++](https://img.shields.io/badge/C%2B%2B-00599C?logo=cplusplus&logoColor=white)](https://isocpp.org/)
[![CMake](https://img.shields.io/badge/CMake-064F8C?logo=cmake&logoColor=white)](https://cmake.org/)

# MELSEC MC Protocol Serial for C++

MELSEC MC Protocol over RS-232C/RS-485 in transport-agnostic C++ for MCU firmware and host-side bring-up tools.

## Supported PLC profiles

Choose one exact canonical PLC profile. The library does not infer the profile from the PLC model or serial frame.

| Canonical profile | Hardware | API selector | Notes |
| --- | --- | --- | --- |
| `melsec:iq-r` | MELSEC iQ-R serial modules | `PlcProfile::MelsecIqR` | iQ-R command and device-layout profile. |
| `melsec:iq-l` | MELSEC iQ-L serial modules | `PlcProfile::MelsecIqL` | Kept separate from iQ-R for future iQ-L divergence. |
| `melsec:q-l` | MELSEC-Q / MELSEC-L serial modules | `PlcProfile::MelsecQL` | Practical default for the host quickstart examples. |
| `melsec:qna` | MELSEC QnA-compatible targets | `PlcProfile::MelsecQnA` | QnA command-family profile. |
| `melsec:ana-anu` | MELSEC AnA / AnU-compatible targets | `PlcProfile::MelsecAnAAnU` | AnA/AnU command-family profile. |
| `melsec:a` | MELSEC-A-compatible targets | `PlcProfile::MelsecA` | A-series command-family profile. |

## Supported device types

The high-level parser accepts plain device strings such as `D100`, `M100`, and `X10`. See [Supported registers](docsrc/user/SUPPORTED_REGISTERS.md) for the full table.

| Device family | Kind | Example | Notes |
| --- | --- | --- | --- |
| `D`, `SD` | Word | `D100` | Decimal address. |
| `M`, `L`, `SM`, `F`, `V`, `S` | Bit | `M100` | Decimal address. |
| `X`, `Y`, `B`, `SB`, `DX`, `DY` | Bit | `X10` | Hexadecimal address. |
| `W`, `SW` | Word | `W100` | Hexadecimal address. |
| `TS`, `TC`, `STS`, `STC`, `CS`, `CC` | Bit | `TS0` | Timer, retentive timer, and counter contact/coil devices. |
| `TN`, `STN`, `CN` | Word | `TN0` | Timer, retentive timer, and counter current-value devices. |
| `LTS`, `LTC`, `LSTS`, `LSTC`, `LCS`, `LCC` | Bit | `LTS0` | Long timer/counter contact/coil devices. |
| `LTN`, `LSTN`, `LCN`, `LZ`, `R`, `RD`, `ZR`, `Z` | Word | `LZ0` | Long current-value, index, and file-register devices. |

## Installation

```ini
lib_deps =
    fa-yoshinobu/mcprotocol-serial-cpp
```

## Quick example

This host-side example opens `/dev/ttyUSB0` on Linux or `COM3` on Windows, selects `PlcProfile::MelsecQL`, and reads `D100` with `19200 / 8E2`.

```cpp
#include <array>
#include <cstdint>
#include <cstdio>

#include "mcprotocol_serial.hpp"

int main() {
  using mcprotocol::serial::PlcProfile;
  using mcprotocol::serial::PosixSerialConfig;
  using mcprotocol::serial::PosixSyncClient;
  using mcprotocol::serial::Status;
  using mcprotocol::serial::highlevel::make_c4_binary_protocol;

  PosixSerialConfig serial {};
#if defined(_WIN32)
  serial.device_path = "COM3";
#else
  serial.device_path = "/dev/ttyUSB0";
#endif
  serial.baud_rate = 19200;
  serial.data_bits = 8;
  serial.stop_bits = 2;
  serial.parity = 'E';
  serial.rts_cts = false;

  auto protocol = make_c4_binary_protocol(PlcProfile::MelsecQL);
  protocol.route.station_no = 0;

  PosixSyncClient plc;
  Status status = plc.open(serial, protocol);
  if (!status.ok()) {
    std::fprintf(stderr, "open failed: %s\n", status.message);
    return 1;
  }

  std::array<std::uint16_t, 1> words {};
  status = plc.read_words("D100", words);
  if (!status.ok()) {
    std::fprintf(stderr, "read_words failed: %s\n", status.message);
    return 1;
  }

  std::printf("D100=0x%04X\n", words[0]);
  return 0;
}
```

## Documentation

| Page | Use it for |
| --- | --- |
| [Getting started](docsrc/user/GETTING_STARTED.md) | Install the library, choose a profile, and perform your first read. |
| [Usage guide](docsrc/user/USAGE_GUIDE.md) | Choose the high-level, host sync, or low-level async entry path. |
| [Wiring guide](docsrc/user/WIRING_GUIDE.md) | Check RS-232C/RS-485 wiring and physical-layer cautions. |
| [Supported registers](docsrc/user/SUPPORTED_REGISTERS.md) | Check device families, address examples, and current string syntax. |
| [PLC profiles](docsrc/user/PROFILES.md) | Choose the exact canonical profile for your target PLC. |
| [Examples](examples/README.md) | Run maintained host and MCU examples. |

## Hardware verified

Real-hardware validation records cover RJ71C24-R2, LJ71C24, QJ71C24N, and FX5UC-32MT/D serial paths.

## License and registry

MIT licensed. Published as [`mcprotocol-serial-cpp`](https://registry.platformio.org/libraries/fa-yoshinobu/mcprotocol-serial-cpp) on the PlatformIO Registry.
