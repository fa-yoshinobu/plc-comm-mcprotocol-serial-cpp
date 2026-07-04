# Examples

These examples show host-side bring-up, MCU UART integration, and the low-level async client. Any example that talks to a real PLC needs a matching serial connection, explicit PLC profile, and matching baud/parity/stop-bit settings.

Use only test addresses that are safe for your PLC program before you run any write example.

## What is in this directory

| Group | Files | Use it for |
| --- | --- | --- |
| Host sync | `host_sync_quickstart.cpp`, `host_sync_polling_reconnect.cpp` | First read and reconnect polling from a Windows, Linux, or POSIX host with the blocking facade. |
| Linux CLI wrapper | `linux_cli/safe_bringup_readonly.sh` | Read-only CLI bring-up with explicit frame and profile environment variables. |
| MCU UART | `platformio_*_arduino_uart/*.cpp` | Real UART reads from `D100-D103` on RP2040, ESP32-C3, and Arduino Mega 2560. |
| Async state machine | `mcu_async_batch_read.cpp`, `platformio_*_arduino_async/*.cpp` | The transport-owned async flow with simulated PLC responses. |

## How to run

### Host sync

```bash
cmake -S . -B build -G Ninja && cmake --build build && ./build/mcprotocol_example_host_sync
```

```powershell
cmake -S . -B build_win -G Ninja
cmake --build build_win --target mcprotocol_example_host_polling_reconnect
.\build_win\mcprotocol_example_host_polling_reconnect.exe COM3 melsec:qcpu D100 4 19200 format5 off
```

### PlatformIO

```bash
pio run -e rpipico-arduino-uart-example
```

```bash
pio run -e esp32-c3-devkitm-1-uart-example
```

```bash
pio run -e mega2560-arduino-uart-example
```

```bash
pio run -e native-example
```

```bash
pio run -e rpipico-arduino-example
```

```bash
pio run -e esp32-c3-devkitm-1-example
```

```bash
pio run -e esp32-c3-devkitm-1-polling-reconnect
```

### Linux CLI

```bash
bash examples/linux_cli/safe_bringup_readonly.sh
```

Set `MCPROTOCOL_FRAME` and `MCPROTOCOL_PLC_PROFILE` before running the CLI wrapper. The wrapper refuses to touch a PLC until both values are explicit.

## Example index

| File or folder | Platform | What it demonstrates |
| --- | --- | --- |
| `host_sync_quickstart.cpp` | Host | `PosixSyncClient`, `make_c4_ascii_format4_protocol`, CPU model read, batch word read, and sparse random read. |
| `host_sync_polling_reconnect.cpp` | Host | Read-only serial polling of `D100-D103` with selectable Format4/Format5 reconnect/backoff state logs on Windows/POSIX host serial ports. |
| `mcu_async_batch_read.cpp` | Host | Low-level `MelsecSerialClient` flow with a simulated success response. |
| `linux_cli/safe_bringup_readonly.sh` | Linux host | Safe read-only CLI bring-up with explicit serial, frame, and PLC profile settings. |
| `platformio_rpipico_arduino_uart/` | RP2040 Arduino | Real `Serial1` UART read-only polling of `D100-D103`. |
| `platformio_esp32c3_arduino_uart/` | ESP32-C3 Arduino | Real `Serial1` UART read-only polling with explicit RX/TX pins. |
| `platformio_arduino_mega2560_uart/` | Arduino Mega 2560 | Real `Serial1` UART read-only polling of `D100-D103`. |
| `platformio_rpipico_arduino_async/` | RP2040 Arduino | Async client lifecycle with simulated response bytes. |
| `platformio_esp32c3_arduino_async/` | ESP32-C3 Arduino | Async client lifecycle with simulated response bytes. |
| `platformio_esp32c3_arduino_async_polling_reconnect/` | ESP32-C3 Arduino | Read-only async UART polling of `D100-D103` with reconnect/backoff state logs. |

## Real UART sample defaults

The real-UART PlatformIO samples are read-only bring-up examples for `D100-D103`.
They use explicit PLC profile and protocol settings in source, and the UART
settings must match the PLC serial module.

| Board | UART | Pins | Serial | Protocol |
| --- | --- | --- | --- | --- |
| RP2040 / Raspberry Pi Pico | `Serial1` | TX `0`, RX `1` | `19200 / 8E1` | `4C ASCII Format4`, `CR/LF`, station `0`, sum check off |
| ESP32-C3 DevKitM-1 | `Serial1` | TX `7`, RX `6` | `19200 / 8E1` | `4C ASCII Format4`, `CR/LF`, station `0`, sum check off |
| Arduino Mega 2560 | `Serial1` | TX1 `18`, RX1 `19` | `19200 / 8E1` | `4C ASCII Format4`, `CR/LF`, station `0`, sum check off |

Treat the pins and serial settings as sample defaults. Change them to match
your board wiring, level shifter, and PLC serial module settings before live
hardware use.

## Before live hardware

Read [Gotchas](../docsrc/user/GOTCHAS.md) before changing frame type, profile, serial settings, or write commands. Profile selection is not automatic, and serial framing must match the PLC serial module settings exactly.
