# Usage Guide

Audience: users running the Linux CLI or integrating the library into host-side bring-up tools.

This guide explains which command paths are native on the validated `RJ71C24-R2` setup and which ones are currently rejected as native commands.

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
- `read-qualified-words`
- `write-qualified-words`

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

If you need the exact on-wire request and response bytes, add:

```bash
--dump-frames on
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

### Qualified `U...\G...` / `U...\HG...` helper access

These helper commands are convenience wrappers over the validated `0601/1601` module-buffer path.
They do not mean that native `0082/0083` extended-device access is validated on this serial setup.

```bash
./build/mcprotocol_cli ... read-qualified-words 'U3E0\G10' 2
./build/mcprotocol_cli ... write-qualified-words 'U3E0\HG20' 0x1234 0x5678
```

Observed status on the validated setup:

- `U3E0\HG20` single-word helper read/write/restore: pass
- `U3E0\G10` helper read: `0x7F22`

### Native non-consecutive access attempt

```bash
./build/mcprotocol_cli ... random-read D100 D105 M100 M105
./build/mcprotocol_cli ... random-write-words D300=123 D305=456
./build/mcprotocol_cli ... random-write-bits M300=1 M305=0
```

Expected behavior on the validated setup:

- `random-read` fails with the native PLC end code
- `random-write-words` fails with the native PLC end code
- `random-write-bits` fails with the native PLC end code

Observed follow-up notes on the validated setup:

- `dword-only` native random/monitor requests tend to fail with `0x7F23`
- `sum-check on` is not part of the validated setup; even `read-words` returns `0x7F23` there

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
