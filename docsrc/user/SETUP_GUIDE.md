# Setup Guide

Audience: users bringing up the Linux CLI against a real Mitsubishi serial module.

This guide is the short path to the known-good setup used for the current real-hardware validation.

## Verified Hardware Setup

- PLC CPU: Mitsubishi iQ-R `R08CPU`
- Serial module: `RJ71C24-R2`
- Link: `RS-232C`
- CLI host: Linux using `/dev/ttyUSB0`
- Communication protocol: `MC protocol (Format 4)`
- Code: ASCII
- Terminator: `CR/LF`
- Sum check: `off`
- Station number: `0`
- Serial line: `19200 bps / 8E1`

## Build

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## First Bring-up

Run the lowest-risk checks first.

```bash
./build/mcprotocol_cli \
  --device /dev/ttyUSB0 \
  --baud 19200 \
  --data-bits 8 \
  --stop-bits 1 \
  --parity E \
  --frame c4-ascii-f4 \
  --sum-check off \
  --station 0 \
  cpu-model
```

Expected result on the validated setup:

```text
model_name=R08CPU
model_code=0x4801
```

Then verify a harmless loopback:

```bash
./build/mcprotocol_cli \
  --device /dev/ttyUSB0 \
  --baud 19200 \
  --data-bits 8 \
  --stop-bits 1 \
  --parity E \
  --frame c4-ascii-f4 \
  --sum-check off \
  --station 0 \
  loopback ABCDE
```

## Recommended Validation Order

1. `cpu-model`
2. `loopback`
3. `read-words`
4. `write-words` against a safe test range
5. `read-bits`
6. `write-bits` against a safe test range
7. `read-host-buffer` / `read-module-buffer`
8. `write-host-buffer` / `write-module-buffer`

After that, use the emulated high-level checks:

1. `random-read`
2. `random-write-words`
3. `random-write-bits`
4. `probe-multi-block`
5. `probe-monitor`

## Where To Look Next

- Current validation matrix: `../validation/reports/HARDWARE_VALIDATION.md`
- Detailed dated evidence: `../validation/reports/RJ71C24_R2_RS232C_FORMAT4_2026-04-10.md`
- Command behavior and fallback notes: `USAGE_GUIDE.md`
- Wiring overview: `WIRING_GUIDE.md`
- MCU integration path: `MCU_QUICKSTART.md`
- Common failures: `TROUBLESHOOTING.md`
