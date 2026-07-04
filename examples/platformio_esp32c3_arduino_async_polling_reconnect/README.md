# ESP32-C3 Arduino Async Polling Reconnect Example

This sample is dedicated to the `esp32-c3-devkitm-1-polling-reconnect` environment.

Main file:

- [platformio_esp32c3_arduino_async_polling_reconnect.cpp](platformio_esp32c3_arduino_async_polling_reconnect.cpp)

Build:

```bash
pio run -e esp32-c3-devkitm-1-polling-reconnect
```

It uses `Serial1` on the ESP32-C3 with fixed example pins `RX=6` and `TX=7`.
The request path is read-only and logs `connected`, `lost`, `reconnecting`,
and `recovered` state transitions while polling `D100-D103`.
