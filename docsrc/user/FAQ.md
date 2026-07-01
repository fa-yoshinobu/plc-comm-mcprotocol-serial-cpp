# FAQ

## Which page should I start with?

- If you want the simplest host-side example, start with [../../examples/README.md](../../examples/README.md)
  and `host_sync_quickstart.cpp`.
- If you want to run on a real MCU UART, start with [Getting started](GETTING_STARTED.md) and the [examples README](../../examples/README.md).
- If you need the exact validated serial settings for your target, use
  the maintainer validation archive.

## Is this library Linux-only?

No. The core library is transport-agnostic and the same core code is used for host-side validation
tools and MCU examples.

## Do the sample defaults equal the validated target settings?

No.

The example projects keep simple starting values so the code stays easy to read. Use
the maintainer validation archive as
the authority for the current validated settings of your actual target.

The host CLI does not use those sample defaults for protocol selection. Live CLI commands require
explicit `--frame` and `--plc-profile` values.

## Which MCU targets are prepared today?

The repository includes PlatformIO examples for:

- `RP2040`
- `ESP32-C3`
- `Arduino Mega 2560`

## Is the MCU example safe to run on a live PLC?

The new real-UART MCU example is read-only. It repeatedly reads `D100-D103` and prints the values.

## Why do some native commands still fail on some targets?

Because support is target-dependent.

- support depends on the target module and the selected `--plc-profile`
- support depends on the command route as well as the target module
- qualified `Un\G` / `Un\HG` access is profile-specific

For qualified-device access, use the route required by the selected profile.
Most validated Q/L, iQ-L, and iQ-F serial paths use the native-qualified
`0401/1401` route; the `0601/1601` helper route is not a fallback.

## Can I shrink memory usage for small firmware builds?

Yes. Use the build-time macros in `platformio.ini` or define them in your own build:

- buffer capacity macros
- command-family enable / disable macros

The repository already includes `reduced` and `ultra-minimal` example profiles.

## Where do I look for the exact PASS / status matrix?

Use:

- the maintainer validation archive
- RJ71C24-R2 RS-232C report
- the maintainer archive
- QJ71C24N RS-232C report
- FX5UC-32MT/D RS-232C report
