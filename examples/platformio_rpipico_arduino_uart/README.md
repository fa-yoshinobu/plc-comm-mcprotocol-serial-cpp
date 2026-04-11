# Raspberry Pi Pico Arduino UART Example

This sample is dedicated to the `rpipico-arduino-uart-example` environment.

Main file:

- [platformio_rpipico_arduino_uart.cpp](platformio_rpipico_arduino_uart.cpp)

Build:

```bash
pio run -e rpipico-arduino-uart-example
```

It uses `Serial1` for the PLC line and keeps the Pico-specific real-UART flow in its own file.
