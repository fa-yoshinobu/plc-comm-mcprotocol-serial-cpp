# Wiring Guide

Audience: users wiring a Linux host or MCU board to a Mitsubishi serial module.

This repository validates protocol behavior, not every cable pinout. Use this page as the high-level wiring plan, then confirm the connector pin numbers in the Mitsubishi module manual and your adapter datasheet.

## Verified Host-side Path

Known-good real-hardware validation used this path:

```text
Linux host
  -> USB to RS-232C adapter
  -> RS-232C cable
  -> RJ71C24-R2 serial port
```

Important points:

- Use an actual `RS-232C` adapter, not a bare `3.3V TTL UART`.
- `RS-232C` and MCU UART logic levels are different.
- The validated setup used `RJ71C24-R2`, `RS-232C`, `19200`, `8E1`, `MC Protocol Format4`, `ASCII`, `CR/LF`, `sum-check off`, `station 0`.

## MCU-side Path

For `ESP32-C3`, `RP2040`, or similar boards, do not wire `TX/RX` directly to the PLC `RS-232C` connector.

Use this shape instead:

```text
MCU UART (3.3V TTL)
  -> TTL / RS-232C level shifter such as MAX3232
  -> RS-232C cable
  -> RJ71C24-R2 serial port
```

Important points:

- The MCU side is `TTL UART`.
- The PLC side is `RS-232C`.
- A level shifter is required between them.
- `RS-485 DE/RE` control is not needed for the validated `RS-232C` setup.

## Before Power-up

Confirm these before sending frames:

- `TX`, `RX`, and `GND` are mapped correctly through the level shifter.
- The MCU board and the PLC side share a valid signal ground path through the interface.
- USB debug serial and PLC serial are not accidentally using the same UART pins.
- The PLC port is configured for the exact serial settings you plan to use.

## Recommended Bring-up Order

1. Run `cpu-model`
2. Run `loopback`
3. Run read-only `read-words`
4. Run write commands only against a safe test area

## Related Pages

- [SETUP_GUIDE.md](SETUP_GUIDE.md)
- [MCU_QUICKSTART.md](MCU_QUICKSTART.md)
- [TROUBLESHOOTING.md](TROUBLESHOOTING.md)
