# Examples

Start with the simplest entry that matches your environment.

## 1. Simple Host Example

- `host_sync_quickstart.cpp`
  - host-side synchronous wrapper
  - read-only bring-up flow
  - easiest library example to read first

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
