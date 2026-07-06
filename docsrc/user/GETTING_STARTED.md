# Getting started

This library speaks MELSEC serial MC Protocol from host tools and MCU firmware. The core client is transport-agnostic, so you can drive it from a blocking host serial port or from your board UART layer.

## Prerequisites

| Requirement | Notes |
| --- | --- |
| C++ standard | C++17 for PlatformIO package use. The repository CMake build uses C++20. |
| Build system | PlatformIO for MCU projects, or CMake for host examples and local integration. |
| Serial interface | RS-232C or RS-485 hardware that matches your PLC serial module. |
| PLC settings | Baud rate, parity, stop bits, frame type, and station number must match the PLC module settings. |

## Install

For PlatformIO, add the library package:

```ini
lib_deps =
    fa-yoshinobu/mcprotocol-serial-cpp@^2.0.0
```

For a CMake project that vendors this repository, add the library directory and link the target:

```cmake
add_subdirectory(external/plc-comm-mcprotocol-serial-cpp)
target_link_libraries(your_app PRIVATE mcprotocol_serial)
```

## PlatformIO examples

Run the maintained PlatformIO environments with `pio run -e <env>`.

| Purpose | Environments |
| --- | --- |
| Host / simulated examples | `native-example`, `native-example-ultra-minimal` |
| RP2040 / Raspberry Pi Pico | `rpipico-arduino-example`, `rpipico-arduino-uart-example`, `rpipico-arduino-example-ultra-minimal` |
| ESP32-C3 DevKitM-1 | `esp32-c3-devkitm-1-example`, `esp32-c3-devkitm-1-uart-example`, `esp32-c3-devkitm-1-example-ultra-minimal` |
| Arduino Mega 2560 real UART | `mega2560-arduino-uart-example`, `mega2560-arduino-uart-example-ultra-minimal` |

The normal MCU examples use a reduced footprint profile. The ultra-minimal examples keep only the smallest batch read/write path and reduce fixed buffers for small firmware builds.

## Choose your PLC profile

`PlcProfile` is required. There is no default profile for live communication.

| Canonical profile | Hardware | API selector |
| --- | --- | --- |
| `melsec:iq-r` | MELSEC iQ-R serial modules | `PlcProfile::MelsecIqR` |
| `melsec:iq-l` | MELSEC iQ-L serial modules | `PlcProfile::MelsecIqL` |
| `melsec:iq-f` | MELSEC iQ-F / FX5 serial paths | `PlcProfile::MelsecIqF` |
| `melsec:qcpu` | MELSEC-Q serial modules | `PlcProfile::MelsecQ` |
| `melsec:lcpu` | MELSEC-L serial modules | `PlcProfile::MelsecL` |
| `melsec:qna` | MELSEC QnA-compatible targets | `PlcProfile::MelsecQnA` |
| `melsec:ana-anu` | MELSEC AnA / AnU-compatible targets | `PlcProfile::MelsecAnAAnU` |
| `melsec:a` | MELSEC-A-compatible targets | `PlcProfile::MelsecA` |

```cpp
auto protocol = mcprotocol::serial::highlevel::make_c4_ascii_format4_protocol(
    mcprotocol::serial::PlcProfile::MelsecQ);
```

## First read on a host

This example uses `PosixSyncClient`, `PosixSerialConfig`, `make_c4_ascii_format4_protocol(PlcProfile::MelsecQ)`, and `read_words("D100", words)`.

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
  using mcprotocol::serial::highlevel::make_c4_ascii_format4_protocol;

  PosixSerialConfig serial {};
#if defined(_WIN32)
  serial.device_path = "COM3";
#else
  serial.device_path = "/dev/ttyUSB0";
#endif
  serial.baud_rate = 19200;
  serial.data_bits = 8;
  serial.stop_bits = 1;
  serial.parity = 'E';
  serial.rts_cts = false;

  auto protocol = make_c4_ascii_format4_protocol(PlcProfile::MelsecQ);
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

Expected output shape:

```text
D100=0x0000
```

The value depends on your PLC memory.

## First read on an MCU

Start from the real-UART PlatformIO examples:

| Board | Example | Default PLC UART |
| --- | --- | --- |
| RP2040 / Raspberry Pi Pico | [platformio_rpipico_arduino_uart](../../examples/platformio_rpipico_arduino_uart/platformio_rpipico_arduino_uart.cpp) | `Serial1`, TX `0`, RX `1`, `19200 / 8E1` |
| ESP32-C3 DevKitM-1 | [platformio_esp32c3_arduino_uart](../../examples/platformio_esp32c3_arduino_uart/platformio_esp32c3_arduino_uart.cpp) | `Serial1`, TX `7`, RX `6`, `19200 / 8E1` |
| Arduino Mega 2560 | [platformio_arduino_mega2560_uart](../../examples/platformio_arduino_mega2560_uart/platformio_arduino_mega2560_uart.cpp) | `Serial1`, TX1 `18`, RX1 `19`, `19200 / 8E1` |

The pin numbers are sample defaults. Change them to match your actual board wiring and level shifter.
The serial values in examples are also sample defaults. Match the actual PLC serial-module frame, baud rate, parity, stop bits, and station number before using them as validation settings.

## Confirm success

1. Your PLC serial module settings match the host or MCU serial settings exactly.
2. The profile selected in code matches the target PLC family.
3. The frame type configured in the PLC matches the helper you selected.
4. The first read returns a `Status` where `status.ok()` is true.
5. The returned word values match a memory range you can safely inspect.

## If it does not work

| Symptom | Check |
| --- | --- |
| No response from the PLC | Baud rate, parity, and stop bits must all match the PLC serial module DIP switch or parameter settings. |
| PLC error or framing error | Check that the PLC module is configured for the same frame type and code mode. |
| RS-485 multi-drop does not answer | `protocol.route.station_no` must match the station number of the target serial module. |
| MCU sample prints zeros or no values | Verify the UART TX/RX pins and the TTL-to-RS-232C or RS-485 interface. |
| Wiring uncertainty | See the shared [MC Protocol Serial setup guide](https://fa-yoshinobu.github.io/plc-comm-docs-site/plc-setup/mcprotocol/serial/) before changing software settings. |

## Next steps

- Open the runnable samples: [examples README](https://github.com/fa-yoshinobu/plc-comm-mcprotocol-serial-cpp/tree/main/examples).
- Continue with the [Usage guide](USAGE_GUIDE.md) and [Gotchas](GOTCHAS.md).
