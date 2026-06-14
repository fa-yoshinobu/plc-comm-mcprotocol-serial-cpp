[![CI](https://github.com/fa-yoshinobu/plc-comm-mcprotocol-serial-cpp/actions/workflows/ci.yml/badge.svg)](https://github.com/fa-yoshinobu/plc-comm-mcprotocol-serial-cpp/actions/workflows/ci.yml)
[![PlatformIO Registry](https://badges.registry.platformio.org/packages/fa-yoshinobu/library/mcprotocol-serial-cpp.svg)](https://registry.platformio.org/libraries/fa-yoshinobu/mcprotocol-serial-cpp)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

# MELSEC MC Protocol Serial for C++

MELSEC MC Protocol over RS-232C/RS-485 in transport-agnostic C++ for MCU firmware and host-side bring-up tools.

## Supported PLC profiles

The maintained profile table is in [PLC profiles](docsrc/user/PROFILES.md). Choose one exact canonical PLC profile from that table.

## Supported device types

The maintained device and range tables are in [Supported registers](docsrc/user/SUPPORTED_REGISTERS.md). Use that page for supported device families, address syntax, and profile-specific notes.

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
| [Full documentation site](https://fa-yoshinobu.github.io/plc-comm-docs-site/) | Unified docs for all PLC communication libraries. |
| [Getting started](docsrc/user/GETTING_STARTED.md) | Install the library, choose a profile, and perform your first read. |
| [Usage guide](docsrc/user/USAGE_GUIDE.md) | Choose the high-level, host sync, or low-level async entry path. |
| [Wiring guide](docsrc/user/WIRING_GUIDE.md) | Check RS-232C/RS-485 wiring and physical-layer cautions. |
| [Supported registers](docsrc/user/SUPPORTED_REGISTERS.md) | Check device families, address examples, and current string syntax. |
| [PLC profiles](docsrc/user/PROFILES.md) | Choose the exact canonical profile for your target PLC. |
| [Gotchas](docsrc/user/GOTCHAS.md) | Troubleshoot common profile, frame, serial, and address mistakes. |
| [Examples](examples/README.md) | Run maintained host and MCU examples. |

## Hardware verified

Live-device verification is maintained in [Latest communication verification](docsrc/user/LATEST_COMMUNICATION_VERIFICATION.md).
See that page for verified PLC models, transports, dates, limitations, and retained validation notes.

## License and registry

| Item | Value |
| --- | --- |
| License | [MIT](LICENSE) |
| Registry | [PlatformIO Registry](https://registry.platformio.org/libraries/fa-yoshinobu/mcprotocol-serial-cpp) |
| Package | `mcprotocol-serial-cpp` |
