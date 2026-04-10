# Usage Guide

Audience: users running the Linux CLI or integrating the library into host-side bring-up tools.

This guide explains which command paths are native on the validated `RJ71C24-R2` setup and which ones are emulated by the CLI.

## Command Classes

### Native PASS

These commands are validated as direct protocol operations on the current setup.

- `cpu-model`
- `loopback`
- `read-words`
- `read-bits`
- `write-words`
- `write-bits`
- `read-host-buffer`
- `write-host-buffer`
- `read-module-buffer`
- `write-module-buffer`

### Emulated PASS

These command names work on the validated setup, but the CLI reaches them through repeated native batch operations after the module rejects the native command.

- `random-read`
- `random-write-words`
- `random-write-bits`
- `probe-multi-block`
- `probe-monitor`

The CLI prints `mode=emulated` for emulated paths where the command output is command-specific.

### Native NG / HOLD

These native request types are still rejected by the validated `RJ71C24-R2` setup.

- `0403` random read: `0x7F22`
- `1402` random write in word units: `0x7F22`
- `1402` random write in bit units: `0x7F23`
- `0406` multi-block read: `0x7F22`
- `1406` multi-block write: `0x7F22`
- `0801` monitor registration: `0x7F22`

`0802` monitor read depends on successful `0801` registration, so the monitor path is treated as native hold on this setup.

## Typical CLI Use

Use the same verified communication options for every command:

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

## Read / Write Examples

### Native contiguous word access

```bash
./build/mcprotocol_cli ... read-words D100 4
./build/mcprotocol_cli ... write-words D100=123 D101=456
```

### Native contiguous bit access

```bash
./build/mcprotocol_cli ... read-bits M100 8
./build/mcprotocol_cli ... write-bits M100=1 M101=0
```

### Emulated non-consecutive access

```bash
./build/mcprotocol_cli ... random-read D100 D105 M100 M105
./build/mcprotocol_cli ... random-write-words D300=123 D305=456
./build/mcprotocol_cli ... random-write-bits M300=1 M305=0
```

Expected behavior on the validated setup:

- `random-read` returns the values directly
- `random-write-words` prints `random-write-words=ok mode=emulated`
- `random-write-bits` prints `random-write-bits=ok mode=emulated`

## Large Contiguous Ranges

The CLI already splits large contiguous write ranges automatically to stay within the fixed request buffer.

- `write-words`: validated up to `960` words
- `write-bits`: validated up to `3584` bits
- `read-words`: validated up to `960` words
- `read-bits`: validated up to `3584` bits

## Validation Reference

For the exact PASS / NG / HOLD matrix and the dated evidence log, use:

- `../validation/reports/HARDWARE_VALIDATION.md`
- `../validation/reports/RJ71C24_R2_RS232C_FORMAT4_2026-04-10.md`

## Safe Read-only Examples

If you want low-risk shell examples instead of typing commands manually, use:

- `../../examples/linux_cli/safe_bringup_readonly.sh`
- `../../examples/linux_cli/cyclic_read_words.sh`
