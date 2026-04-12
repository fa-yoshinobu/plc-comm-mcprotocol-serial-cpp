# FAQ

## Which page should I start with?

- If you want the simplest host-side example, start with [../../examples/README.md](../../examples/README.md)
  and `host_sync_quickstart.cpp`.
- If you want to run on a real MCU UART, start with [MCU_QUICKSTART.md](MCU_QUICKSTART.md).
- If you need the exact validated serial settings for your target, use
  [../validation/reports/HARDWARE_VALIDATION.md](../validation/reports/HARDWARE_VALIDATION.md).

## Is this library Linux-only?

No. The core library is transport-agnostic and the same core code is used for host-side validation
tools and MCU examples.

## Do the sample defaults equal the validated target settings?

No.

The example projects keep simple starting values so the code stays easy to read. Use
[../validation/reports/HARDWARE_VALIDATION.md](../validation/reports/HARDWARE_VALIDATION.md) as
the authority for the current validated settings of your actual target.

## Which MCU targets are prepared today?

The repository includes PlatformIO examples for:

- `RP2040`
- `ESP32-C3`
- `Arduino Mega 2560`

## Is the MCU example safe to run on a live PLC?

The new real-UART MCU example is read-only. It repeatedly reads `D100-D103` and prints the values.

## Why do some native commands still fail on some targets?

Because support is target-dependent.

- support depends on the target module and the selected `--series`
- helper and contiguous paths are often the practical public workflow even when a direct native
  probe is target-dependent
- native qualified access is not a supported public workflow

For practical qualified-device access, use `read-qualified-words` / `write-qualified-words` where
the helper path is validated. Treat native qualified commands as diagnostic probes only.

## Can I shrink memory usage for small firmware builds?

Yes. Use the build-time macros in `platformio.ini` or define them in your own build:

- buffer capacity macros
- command-family enable / disable macros

The repository already includes `reduced` and `ultra-minimal` example profiles.

## Where do I look for the exact PASS / status matrix?

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

The output is generated under `build/docs/api/`.

If you want to run Doxygen without CMake, use:

```bash
scripts/run_doxygen.bat
```
