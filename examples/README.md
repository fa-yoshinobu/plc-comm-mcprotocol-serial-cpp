# Examples

Start here if you want to choose the first sample without reading the whole repository.

## Pick The First Sample

- `host_sync_quickstart.cpp`
  - simplest host-side blocking example
  - read-only bring-up
  - best first read if you are on Windows or a POSIX host
- `platformio_*_arduino_uart/*.cpp`
  - real UART firmware examples for `RP2040`, `ESP32-C3`, and `Arduino Mega 2560`
  - read-only `D100-D103`
  - best first step for actual MCU wiring
- `mcu_async_batch_read.cpp` and `platformio_*_arduino_async/*.cpp`
  - lower-level async state-machine examples
  - best if you are integrating your own UART driver or scheduler

For exact target settings, do not rely on the sample defaults alone. Use
[../docsrc/validation/reports/HARDWARE_VALIDATION.md](../docsrc/validation/reports/HARDWARE_VALIDATION.md)
as the authority.

## 1. Simple Host Example

- `host_sync_quickstart.cpp`
  - host-side synchronous wrapper
  - read-only bring-up flow
  - easiest library example to read first
  - use `COM3` on Windows or `/dev/ttyUSB0` on POSIX hosts

Build and run:

```bash
cmake -S . -B build
cmake --build build --target mcprotocol_example_host_sync
./build/mcprotocol_example_host_sync
```

## 2. Real MCU UART Example

- `platformio_rpipico_arduino_uart/platformio_rpipico_arduino_uart.cpp`
  - Raspberry Pi Pico specific `Serial1` sample
- `platformio_esp32c3_arduino_uart/platformio_esp32c3_arduino_uart.cpp`
  - ESP32-C3 specific `Serial1` sample
- `platformio_arduino_mega2560_uart/platformio_arduino_mega2560_uart.cpp`
  - Arduino Mega 2560 specific `Serial1` sample
- all three are read-only PLC access samples intended for actual UART bring-up through a level shifter
- the example code keeps simple sample defaults; confirm the real target settings in the validation
  matrix before wiring live hardware

Build:

```bash
pio run -e rpipico-arduino-uart-example
pio run -e esp32-c3-devkitm-1-uart-example
pio run -e mega2560-arduino-uart-example
```

## 3. Advanced Async Examples

- `mcu_async_batch_read.cpp`
  - low-level asynchronous state-machine example
  - host-runnable
  - uses a simulated success response
- `platformio_rpipico_arduino_async/platformio_rpipico_arduino_async.cpp`
  - Raspberry Pi Pico specific async sample
- `platformio_esp32c3_arduino_async/platformio_esp32c3_arduino_async.cpp`
  - ESP32-C3 specific async sample
- both Arduino async samples still use a simulated response path

Build:

```bash
cmake --build build --target mcprotocol_example_mcu_async
pio run -e native-example
pio run -e rpipico-arduino-example
pio run -e esp32-c3-devkitm-1-example
```

Expected host output:

```text
example read ok: D100=0x1234 D101=0x5678
```
