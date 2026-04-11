# MCU Quickstart

Audience: users who want to move from the host-side examples to actual MCU firmware.

This page points to the real-UART read-only sample added for `RP2040` and `ESP32-C3`.

## What The Sample Does

The sample:

- configures `MelsecSerialClient` for the validated `RJ71C24-R2` settings
- opens `Serial1`
- sends the client's `pending_tx_frame()`
- calls `notify_tx_complete()` after `flush()`
- feeds incoming UART bytes into `on_rx_bytes()`
- performs a read-only batch read of `D100-D103`

Sample source:

- [../../examples/platformio_arduino_uart/platformio_arduino_uart.cpp](../../examples/platformio_arduino_uart/platformio_arduino_uart.cpp)

## Build

```bash
pio run -e rpipico-arduino-uart-example
pio run -e esp32-c3-devkitm-1-uart-example
```

## Default Sample Assumptions

- Protocol: `4C ASCII Format4`
- Terminator: `CR/LF`
- Sum check: `off`
- Station: `0`
- Read-only device range: `D100-D103`

The board environments also define default UART pins:

- `rpipico-arduino-uart-example`
  - `TX=0`
  - `RX=1`
- `esp32-c3-devkitm-1-uart-example`
  - `TX=7`
  - `RX=6`

Treat these as starting values. Change them to match your actual board wiring.

## Integration Notes

The sample is intentionally simple.

- It uses `Serial` for debug output.
- It uses `Serial1` for the PLC line.
- It treats `flush()` as the TX completion point.
- It reads only, so it is safe to use during initial wiring bring-up.

In a production firmware port you may replace `flush()` with:

- TX complete interrupt
- DMA completion callback
- scheduler event when the UART peripheral finishes transmission

## Recommended Sequence

1. Confirm the target settings in `HARDWARE_VALIDATION.md`.
2. Wire the MCU through a `TTL <-> RS-232C` level shifter.
3. Build the matching PlatformIO UART example.
4. Confirm a read-only `D` range works from the MCU.
5. Only then add write operations.
