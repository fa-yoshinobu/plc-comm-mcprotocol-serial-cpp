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

- `platformio_arduino_uart/platformio_arduino_uart.cpp`
  - Arduino-style `Serial1` sample
  - read-only PLC access
  - intended for actual UART bring-up through a level shifter
  - shared by `rpipico-arduino-uart-example` and `esp32-c3-devkitm-1-uart-example`

Build:

```bash
pio run -e rpipico-arduino-uart-example
pio run -e esp32-c3-devkitm-1-uart-example
```

## 3. Advanced Async Examples

- `mcu_async_batch_read.cpp`
  - low-level asynchronous state-machine example
  - host-runnable
  - uses a simulated success response
- `platformio_arduino_async/platformio_arduino_async.cpp`
  - Arduino-style `setup()` / `loop()` sample
  - compile-safe on `RP2040` and `ESP32-C3`
  - still uses a simulated response path

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
