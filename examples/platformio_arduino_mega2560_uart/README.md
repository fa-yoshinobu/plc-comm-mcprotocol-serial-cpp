# Arduino Mega 2560 UART Example

This sample is dedicated to the `mega2560-arduino-uart-example` environment.

Main file:

- [platformio_arduino_mega2560_uart.cpp](platformio_arduino_mega2560_uart.cpp)

Build:

```bash
pio run -e mega2560-arduino-uart-example
```

Notes:

- It uses `Serial` for debug output.
- It uses `Serial1` for the PLC line on pins `18/19`.
- It reads `D100-D103` only.
- Wire the PLC side through a real level shifter.
