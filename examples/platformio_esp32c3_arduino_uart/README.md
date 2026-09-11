# ESP32-C3 Arduino UART Example

This example reads D100-D103 using `Esp32UartClient`.
UART1 is owned exclusively by the adapter; do not also initialize `Serial1`.
Pins: RX=6, TX=7. Default: 19200 baud, 8E1, C4 ASCII Format4,
MelsecQ profile, sum check disabled, host station route.
Match the PLC settings and transceiver wiring before use.
Direction control defaults to external/automatic; RTS direction needs explicit pins.

The existing PlatformIO environment is `esp32-c3-devkitm-1-uart-example`.
It retains C++17 and uses the reduced MCU buffer configuration.

The adapter handles partial writes, physical transmission completion and the
completion timestamp. Receive bytes are retained while waiting for TX completion.
`update()` advances communication without blocking the application loop.

On any error this minimal example stops requesting data. It never retries writes.
Correct the cause and exclude delayed replies **before** resetting the MCU.
Resetting the MCU alone does not remove a response still pending at the PLC.

See the [recovery example](../platformio_esp32c3_arduino_async_polling_reconnect/README.md)
for error categories and the explicit restart procedure.
