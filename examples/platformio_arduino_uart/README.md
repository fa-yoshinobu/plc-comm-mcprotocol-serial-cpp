# PlatformIO Arduino UART Example

This directory contains the read-only real-UART sample for `Serial1`.

Main file:

- [platformio_arduino_uart.cpp](platformio_arduino_uart.cpp)

Build:

```bash
pio run -e rpipico-arduino-uart-example
pio run -e esp32-c3-devkitm-1-uart-example
```

What it does:

- opens `Serial1`
- sends the client's pending frame over a real UART
- calls `notify_tx_complete()` after `flush()`
- feeds received bytes from the UART into `on_rx_bytes()`
- repeatedly performs a read-only batch read of `D100-D103`

Important:

- this sample expects a `TTL <-> RS-232C` level shifter between the MCU and the PLC serial port
- it is read-only on purpose
- the default `TX/RX` pins in `platformio.ini` are starting values, not universal board truth
