# Raspberry Pi Pico Arduino Async Example

This sample is dedicated to the `rpipico-arduino-example` environment.

Main file:

- [platformio_rpipico_arduino_async.cpp](platformio_rpipico_arduino_async.cpp)

Build:

```bash
pio run -e rpipico-arduino-example
pio run -e rpipico-arduino-example-ultra-minimal
```

It keeps the simulated-response async flow in a Pico-specific file instead of sharing code with
other boards.
