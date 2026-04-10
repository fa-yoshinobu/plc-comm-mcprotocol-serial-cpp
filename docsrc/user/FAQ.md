# FAQ

## Is this library Linux-only?

No. The core library is transport-agnostic and the same core code is used for Linux CLI tests and MCU examples.

## Which MCU targets are prepared today?

The repository includes PlatformIO examples for:

- `RP2040`
- `ESP32-C3`

## Is the MCU example safe to run on a live PLC?

The new real-UART MCU example is read-only. It repeatedly reads `D100-D103` and prints the values.

## Why do `random-read` and `probe-monitor` work if native `0403` and `0801` are on hold?

Because the CLI emulates those higher-level command names with repeated native batch operations after the `RJ71C24-R2` setup rejects the native commands.

## Can I shrink memory usage for small firmware builds?

Yes. Use the build-time macros in `platformio.ini` or define them in your own build:

- buffer capacity macros
- command-family enable / disable macros

The repository already includes `reduced` and `ultra-minimal` example profiles.

## Where do I look for the exact PASS / NG / HOLD matrix?

Use:

- [../validation/reports/HARDWARE_VALIDATION.md](../validation/reports/HARDWARE_VALIDATION.md)
- [../validation/reports/RJ71C24_R2_RS232C_FORMAT4_2026-04-10.md](../validation/reports/RJ71C24_R2_RS232C_FORMAT4_2026-04-10.md)

## How do I generate API docs?

Run:

```bash
cmake --build build --target docs
```

The output is generated under `build/docs/doxygen/html`.

If you want to run Doxygen without CMake, use:

```bash
doxygen Doxyfile
```
