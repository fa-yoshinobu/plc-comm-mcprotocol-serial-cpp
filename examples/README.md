# Examples

This directory contains both simulated integration examples and board-oriented bring-up examples.

## Simulated examples

- `mcu_async_batch_read.cpp`
  - generic microcontroller-style asynchronous integration
  - host-runnable
  - uses a simulated success response
- `platformio_arduino_async/platformio_arduino_async.cpp`
  - Arduino-style `setup()` / `loop()` sample
  - compile-safe on `RP2040` and `ESP32-C3`
  - still uses a simulated response path

## Real UART example

- `platformio_arduino_uart/platformio_arduino_uart.cpp`
  - Arduino-style `Serial1` sample
  - read-only PLC access
  - intended for actual UART bring-up through a level shifter
  - shared by `rpipico-arduino-uart-example` and `esp32-c3-devkitm-1-uart-example`

## Safe Linux CLI examples

- `linux_cli/safe_bringup_readonly.sh`
  - runs `cpu-model`
  - runs `loopback`
  - runs a small `read-words`
- `linux_cli/cyclic_read_words.sh`
  - repeats `read-words` for a configurable duration
- `linux_cli/supported_device_rw_soak.sh`
  - runs an approximately 3-minute read/write/verify/restore soak
  - uses the supported non-low-address device set that completed command screening without protocol errors
  - tolerates RUN-mode readback mismatch and reports it as observation data

## Build

```bash
cmake -S . -B build
cmake --build build --target mcprotocol_example_mcu_async
pio run -e native-example
pio run -e rpipico-arduino-example
pio run -e esp32-c3-devkitm-1-example
pio run -e rpipico-arduino-uart-example
pio run -e esp32-c3-devkitm-1-uart-example
```

## Run The Host Example

```bash
./build/mcprotocol_example_mcu_async
```

Expected output:

```text
example read ok: D100=0x1234 D101=0x5678
```
