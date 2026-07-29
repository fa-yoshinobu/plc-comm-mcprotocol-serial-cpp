[![CI](https://github.com/fa-yoshinobu/plc-comm-mcprotocol-serial-cpp/actions/workflows/ci.yml/badge.svg)](https://github.com/fa-yoshinobu/plc-comm-mcprotocol-serial-cpp/actions/workflows/ci.yml)
[![PlatformIO Registry](https://badges.registry.platformio.org/packages/fa-yoshinobu/library/mcprotocol-serial-cpp.svg)](https://registry.platformio.org/libraries/fa-yoshinobu/mcprotocol-serial-cpp)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

# MELSEC MC Protocol Serial for C++

MELSEC MC Protocol over RS-232C/RS-485 in transport-agnostic C++ for MCU firmware and host-side bring-up tools.

## PLC Comm Family

This library is part of the plc-comm family. See the [package matrix](https://fa-yoshinobu.github.io/plc-comm-docs-site/package-matrix/) for protocol, language, registry, and install-command mapping.

## Supported PLC profiles

The maintained profile table is in the [MC Protocol Serial PLC profiles](https://fa-yoshinobu.github.io/plc-comm-docs-site/mcprotocol/cpp/PROFILES/) page. Choose one exact canonical PLC profile from that table.

## Supported device types

The maintained device and range table is in the shared [MC Protocol Serial supported registers](https://fa-yoshinobu.github.io/plc-comm-docs-site/plc-setup/mcprotocol/supported-registers/) page. Use that page for supported device families, address syntax, and profile-specific notes.

## Installation

```ini
[env:your-board]
lib_deps =
    fa-yoshinobu/mcprotocol-serial-cpp
build_unflags =
    -std=gnu++11
    -std=gnu++14
build_flags =
    -std=gnu++17
```

The PlatformIO package contains the transport-agnostic `MelsecSerialClient`, codecs, high-level request builders, and MCU compatibility headers. It intentionally does not compile the Windows/POSIX serial backend or `PosixSyncClient`. Use a source checkout with CMake for those host-only components.

## Core client start

Select the PLC profile explicitly, configure the core client, then connect its async TX/RX lifecycle to your UART or simulated transport:

```cpp
#include "mcprotocol_serial.hpp"

auto protocol = mcprotocol::serial::highlevel::make_c4_ascii_format4_protocol(
    mcprotocol::serial::PlcProfile::MelsecQ,
    mcprotocol::serial::SumCheckMode::Disabled,
    mcprotocol::serial::RouteConfig {mcprotocol::serial::HostStationRoute {}});

mcprotocol::serial::MelsecSerialClient plc;
mcprotocol::serial::Status status = plc.configure(protocol);
```

See the maintained [PlatformIO and CMake examples](https://github.com/fa-yoshinobu/plc-comm-mcprotocol-serial-cpp/tree/main/examples) for complete UART, simulated async, and host workflows. No example communicates with a PLC merely by being built.

## Documentation

| Page | Use it for |
| --- | --- |
| [Full documentation site](https://fa-yoshinobu.github.io/plc-comm-docs-site/) | Unified docs for all PLC communication libraries. |
| [Getting started](https://fa-yoshinobu.github.io/plc-comm-docs-site/mcprotocol/cpp/GETTING_STARTED/) | Install the library, choose a profile, and perform your first read. |
| [Usage guide](https://fa-yoshinobu.github.io/plc-comm-docs-site/mcprotocol/cpp/USAGE_GUIDE/) | Choose the high-level, host sync, or low-level async entry path. |
| [API reference](https://fa-yoshinobu.github.io/plc-comm-docs-site/mcprotocol/cpp/API_REFERENCE/) | Generated reference for the public C++ headers. |
| [PLC profiles](https://fa-yoshinobu.github.io/plc-comm-docs-site/mcprotocol/cpp/PROFILES/) | Choose the exact canonical profile for your target PLC. |
| [Gotchas](https://fa-yoshinobu.github.io/plc-comm-docs-site/mcprotocol/cpp/GOTCHAS/) | Troubleshoot common profile, frame, serial, and address mistakes. |
| [MC Protocol Serial setup](https://fa-yoshinobu.github.io/plc-comm-docs-site/plc-setup/mcprotocol/serial/) | Check PLC-side serial settings, station number, wiring shape, and bring-up order. |
| [MC Protocol Serial supported registers](https://fa-yoshinobu.github.io/plc-comm-docs-site/plc-setup/mcprotocol/supported-registers/) | Check device families, address examples, and current string syntax. |
| [Troubleshooting & Codes](https://fa-yoshinobu.github.io/plc-comm-docs-site/plc-setup/mcprotocol/troubleshooting-codes/) | Interpret library status codes and observed PLC/module error families. |
| [Examples](https://github.com/fa-yoshinobu/plc-comm-mcprotocol-serial-cpp/tree/main/examples) | Run maintained host and MCU examples. |

## License and registry

| Item | Value |
| --- | --- |
| License | [MIT](LICENSE) |
| Registry | [PlatformIO Registry](https://registry.platformio.org/libraries/fa-yoshinobu/mcprotocol-serial-cpp) |
| Package | `mcprotocol-serial-cpp` |

## Commercial support

If you plan to embed this library in a paid or commercial product, please consider a separate support agreement or supporting the project as a sponsor.

Contact: <https://fa-labo.com/contact.html>
