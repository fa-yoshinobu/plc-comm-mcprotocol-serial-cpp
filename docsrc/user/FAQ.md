# FAQ

## Is this library Linux-only?

No. The core library is transport-agnostic and the same core code is used for host-side validation
tools and MCU examples.

## Which MCU targets are prepared today?

The repository includes PlatformIO examples for:

- `RP2040`
- `ESP32-C3`
- `Arduino Mega 2560`

## Is the MCU example safe to run on a live PLC?

The new real-UART MCU example is read-only. It repeatedly reads `D100-D103` and prints the values.

## Why do some native commands still fail on some targets?

Because support is target-dependent.

- `RJ71C24-R2`, `LJ71C24`, and `QJ71C24N` pass native random / multi-block / monitor traffic under `--series ql`
- `FX5UC-32MT/D` passes native random / multi-block traffic under `--series ql`
- `FX5UC-32MT/D` monitor `0801/0802` is treated as unsupported on serial `3C/4C`
- native qualified access is not a supported workflow

For practical qualified-device access, use `read-qualified-words` / `write-qualified-words` where
the helper path is validated. Treat native qualified commands as diagnostic probes only.

## Can I shrink memory usage for small firmware builds?

Yes. Use the build-time macros in `platformio.ini` or define them in your own build:

- buffer capacity macros
- command-family enable / disable macros

The repository already includes `reduced` and `ultra-minimal` example profiles.

## Where do I look for the exact PASS / NG / HOLD matrix?

Use:

- [../validation/reports/HARDWARE_VALIDATION.md](../validation/reports/HARDWARE_VALIDATION.md)
- [../validation/reports/RJ71C24_R2_RS232C.md](../validation/reports/RJ71C24_R2_RS232C.md)
- [../validation/reports/LJ71C24_RS232C.md](../validation/reports/LJ71C24_RS232C.md)
- [../validation/reports/QJ71C24N_RS232C.md](../validation/reports/QJ71C24N_RS232C.md)
- [../validation/reports/FX5UC_32MT_D_RS232C.md](../validation/reports/FX5UC_32MT_D_RS232C.md)

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
