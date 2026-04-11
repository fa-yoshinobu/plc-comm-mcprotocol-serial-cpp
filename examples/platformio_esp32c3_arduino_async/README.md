# ESP32-C3 Arduino Async Example

This sample is dedicated to the `esp32-c3-devkitm-1-example` environment.

Main file:

- [platformio_esp32c3_arduino_async.cpp](platformio_esp32c3_arduino_async.cpp)

Build:

```bash
pio run -e esp32-c3-devkitm-1-example
pio run -e esp32-c3-devkitm-1-example-ultra-minimal
```

It keeps the simulated-response async flow in an ESP32-C3-specific file instead of sharing code
with other boards.
