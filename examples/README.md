# Examples

These examples show host-side bring-up, MCU UART integration, and the low-level async client. Any example that talks to a real PLC needs a matching serial connection, explicit PLC profile, and matching baud/parity/stop-bit settings.

## What is in this directory

| Group | Files | Use it for |
| --- | --- | --- |
| Host sync | `host_sync_quickstart.cpp` | First read from a Windows, Linux, or POSIX host with the blocking facade. |
| Linux CLI wrapper | `linux_cli/safe_bringup_readonly.sh` | Read-only CLI bring-up with explicit frame and profile environment variables. |
| MCU UART | `platformio_*_arduino_uart/*.cpp` | Real UART reads from `D100-D103` on RP2040, ESP32-C3, and Arduino Mega 2560. |
| Async state machine | `mcu_async_batch_read.cpp`, `platformio_*_arduino_async/*.cpp` | The transport-owned async flow with simulated PLC responses. |

## How to run

### Host sync

```bash
cmake -S . -B build -G Ninja && cmake --build build && ./build/mcprotocol_example_host_sync
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

### Linux CLI

```bash
bash examples/linux_cli/safe_bringup_readonly.sh
```

Set `MCPROTOCOL_FRAME` and `MCPROTOCOL_PLC_PROFILE` before running the CLI wrapper. The wrapper refuses to touch a PLC until both values are explicit.

## Example index

| File or folder | Platform | What it demonstrates |
| --- | --- | --- |
| `host_sync_quickstart.cpp` | Host | `PosixSyncClient`, `make_c4_binary_protocol`, CPU model read, batch word read, and sparse random read. |
| `mcu_async_batch_read.cpp` | Host | Low-level `MelsecSerialClient` flow with a simulated success response. |
| `linux_cli/safe_bringup_readonly.sh` | Linux host | Safe read-only CLI bring-up with explicit serial, frame, and PLC profile settings. |
| `platformio_rpipico_arduino_uart/` | RP2040 Arduino | Real `Serial1` UART read-only polling of `D100-D103`. |
| `platformio_esp32c3_arduino_uart/` | ESP32-C3 Arduino | Real `Serial1` UART read-only polling with explicit RX/TX pins. |
| `platformio_arduino_mega2560_uart/` | Arduino Mega 2560 | Real `Serial1` UART read-only polling of `D100-D103`. |
| `platformio_rpipico_arduino_async/` | RP2040 Arduino | Async client lifecycle with simulated response bytes. |
| `platformio_esp32c3_arduino_async/` | ESP32-C3 Arduino | Async client lifecycle with simulated response bytes. |

## Before live hardware

Read [Gotchas](../docsrc/user/GOTCHAS.md) before changing frame type, profile, serial settings, or write commands. Profile selection is not automatic, and serial framing must match the PLC serial module settings exactly.
