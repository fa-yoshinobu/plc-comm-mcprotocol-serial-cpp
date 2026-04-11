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

## Additional Verified Setup

- PLC CPU: Mitsubishi L-series `L26CPU-BT`
- Serial module: `LJ71C24`
- Link: `RS-232C`
- Communication protocol: `MC protocol (Format 5)`
- Code: Binary
- Sum check: `on`
- Station number: `0`
- Serial line: `28800 bps / 8E2`
- CLI family selection: `--series ql`

Do not reuse `--series iqr` on this L-series target. On `2026-04-11`, even contiguous
`read-words D100 1` and `read-bits M100 1` failed with `0x7F22` until the CLI family selection was
switched to `ql`.

## Additional Verified Setup

- PLC CPU: Mitsubishi iQ-F `FX5UC-32MT/D`
- Link: `RS-232C`
- Communication protocol: `MC protocol (Format 5)`
- Code: Binary
- Sum check: `on`
- Station number: `0`
- Serial line: `38400 bps / 8E2`
- CLI family selection: `--series ql`

Do not reuse `--series iqr` on this FX target. On `2026-04-11`, even contiguous
`read-words D100 1` and `read-bits M100 1` failed with `0x7E40` under `iqr`.

Also do not assume the full `supported_device_rw_soak.sh` target set applies unchanged. The FX5
communication manual's serial `3C/4C` accessible-device table marks `DX`, `DY`, `V`, and `ZR`
inaccessible on this target class, and `DX10` / `DY10` also returned `0x7E43` on `2026-04-11`, so use
`../../examples/linux_cli/fx5u_supported_device_rw_soak.sh` for the validated FX5U contiguous soak.

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
9. `read-qualified-words` / `write-qualified-words`

After that, use the unsupported native checks only if you explicitly want to confirm what the module rejects:

1. `random-read`
2. `random-write-words`
3. `random-write-bits`
4. `read-native-qualified-words`
5. `probe-multi-block`
6. `probe-monitor`

If you need `U...\\G...` or `U...\\HG...` access during normal bring-up, stay on
`read-qualified-words` / `write-qualified-words`. The native qualified commands are follow-up probes
on this setup.

## Where To Look Next

- Current validation matrix: `../validation/reports/HARDWARE_VALIDATION.md`
- Detailed dated evidence: `../validation/reports/RJ71C24_R2_RS232C_FORMAT4_2026-04-10.md`
- Command behavior notes: `USAGE_GUIDE.md`
- Wiring overview: `WIRING_GUIDE.md`
- MCU integration path: `MCU_QUICKSTART.md`
- Common failures: `TROUBLESHOOTING.md`
