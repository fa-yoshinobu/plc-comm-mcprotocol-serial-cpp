# ESP32-S3 UART adapter: FX5U D100

Build from the repository root:

```sh
pio run -e esp32-s3-uart-adapter
```

The [example](main.cpp) uses STAMPLC PWR-485 pins (RX39, TX0, RTS46) and UART1.
Set the FX5U built-in RS-485 port to MC protocol, station 0, Format5,
19200 baud, 8 data bits, even parity, 1 stop bit, sum check enabled.
Disable M5StamPLC's Modbus slave if integrating the example with its LCD library.
Do not initialize Serial1 separately.

Building does not communicate with a PLC. Upload explicitly to run the read-only
example. It prints D100 to USB serial and stops polling after an error. It does
not initialize the STAMPLC display. The M5 display belongs in the application.

See the [adapter guide](../../docsrc/user/ARDUINO_ESP32_UART.md) for async usage,
ownership, recovery, and validation limits.
