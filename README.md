[![CI](https://github.com/fa-yoshinobu/plc-comm-mcprotocol-serial-cpp/actions/workflows/ci.yml/badge.svg)](https://github.com/fa-yoshinobu/plc-comm-mcprotocol-serial-cpp/actions/workflows/ci.yml)
[![PlatformIO Registry](https://badges.registry.platformio.org/packages/fa-yoshinobu/library/mcprotocol-serial-cpp.svg)](https://registry.platformio.org/libraries/fa-yoshinobu/mcprotocol-serial-cpp)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

# MELSEC MC Protocol Serial for C++

MELSEC MC Protocol over RS-232C/RS-485 in transport-agnostic C++ for MCU firmware and host-side bring-up tools.

Supported MCU targets are ESP32 and RP2040. Arduino Mega 2560 and other AVR/8-bit targets are not
supported; migrate those projects to a supported ESP32 target or maintain the AVR port downstream.

## PLC Comm Family

This library is part of the plc-comm family. See the [package matrix](https://plc-comm-docs-site.fa-labo.com/package-matrix/) for protocol, language, registry, and install-command mapping.

## Supported PLC profiles

The maintained profile table is in the [MC Protocol Serial PLC profiles](https://plc-comm-docs-site.fa-labo.com/mcprotocol/cpp/PROFILES/) page. Choose one exact canonical PLC profile from that table.

## Supported device types

The maintained device and range table is in the shared [MC Protocol Serial supported registers](https://plc-comm-docs-site.fa-labo.com/plc-setup/mcprotocol/supported-registers/) page. Use that page for supported device families, address syntax, and profile-specific notes.

## Installation

```ini
[env:your-board]
lib_deps =
    fa-yoshinobu/mcprotocol-serial-cpp
build_unflags =
    -std=gnu++11
    -std=gnu++14
build_flags =
    -std=c++17
```

The PlatformIO package contains the transport-agnostic `MelsecSerialClient`, codecs, high-level request builders, and MCU compatibility headers for ESP32 and RP2040. It intentionally does not compile the Windows/POSIX serial backend or `HostSyncClient`. Use a source checkout with CMake for those host-only components.

## Core client start

Arduino-ESP32 users can opt into the new
[`Esp32UartClient` UART adapter](docsrc/user/ARDUINO_ESP32_UART.md).
It manages UART initialization, incremental TX/RX, deadlines, and explicit recovery,
and provides synchronous and asynchronous word/bit operations. Include
`mcprotocol_serial_arduino_esp32.hpp` explicitly; the existing core header and host
APIs remain Arduino-independent. The initial build-checked target is Arduino-ESP32
2.0.17 / ESP32-S3. Physical RS-485 validation is still required.

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
| [Full documentation site](https://plc-comm-docs-site.fa-labo.com/) | Unified docs for all PLC communication libraries. |
| [Getting started](https://plc-comm-docs-site.fa-labo.com/mcprotocol/cpp/GETTING_STARTED/) | Install the library, choose a profile, and perform your first read. |
| [Usage guide](https://plc-comm-docs-site.fa-labo.com/mcprotocol/cpp/USAGE_GUIDE/) | Choose the high-level, host sync, or low-level async entry path. |
| [API reference](https://plc-comm-docs-site.fa-labo.com/mcprotocol/cpp/API_REFERENCE/) | Generated reference for the public C++ headers. |
| [PLC profiles](https://plc-comm-docs-site.fa-labo.com/mcprotocol/cpp/PROFILES/) | Choose the exact canonical profile for your target PLC. |
| [Gotchas](https://plc-comm-docs-site.fa-labo.com/mcprotocol/cpp/GOTCHAS/) | Troubleshoot common profile, frame, serial, and address mistakes. |
| [MC Protocol Serial setup](https://plc-comm-docs-site.fa-labo.com/plc-setup/mcprotocol/serial/) | Check PLC-side serial settings, station number, wiring shape, and bring-up order. |
| [MC Protocol Serial supported registers](https://plc-comm-docs-site.fa-labo.com/plc-setup/mcprotocol/supported-registers/) | Check device families, address examples, and current string syntax. |
| [Troubleshooting & Codes](https://plc-comm-docs-site.fa-labo.com/plc-setup/mcprotocol/troubleshooting-codes/) | Interpret library status codes and observed PLC/module error families. |
| [Performance](https://plc-comm-docs-site.fa-labo.com/performance/) | See measured latency, throughput, and long-run soak results from real PLC hardware. |
| [Choosing a Language](https://plc-comm-docs-site.fa-labo.com/choosing-a-language/) | Compare the .NET, Python, Rust, C++, and Node-RED implementations before you pick one. |
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

## StringView compatibility

Public APIs use `mcprotocol::serial::StringView`, declared in
`mcprotocol/serial/string_view_compat.hpp`. On supported standard-library
implementations (including ESP32 GCC 8), it is an alias of `std::string_view`:
existing calls, types and API signatures remain compatible, with no wrapper or
conversion overhead. When the standard implementation is unavailable, it aliases
the library's `detail::StringView`. Selection is automatic.

Applications that previously relied on the bundled fallback defining
`std::string_view` must use `mcprotocol::serial::StringView` instead. String
literals remain accepted. The fallback no longer defines a class in `namespace std`.
Rebuild the library and consumers with the same toolchain and compatibility
settings; do not mix standard and fallback builds. Other bundled compatibility
types are outside the scope of this change.
