# Linux CLI Examples

This directory contains low-risk shell examples for the Linux CLI.

Scripts:

- [safe_bringup_readonly.sh](safe_bringup_readonly.sh)
  - runs `cpu-model`
  - runs `loopback`
  - runs a small `read-words`
- [cyclic_read_words.sh](cyclic_read_words.sh)
  - repeats `read-words` for a configurable duration
- [supported_device_rw_soak.sh](supported_device_rw_soak.sh)
  - runs an approximately 180-second read/write/verify/restore soak
  - covers the supported non-low-address device set that completed command screening without protocol errors
  - keeps execution strictly serial and avoids the very lowest device numbers

Default communication settings match the validated setup:

- `19200`
- `8E1`
- `c4-ascii-f4`
- `sum-check off`
- `station 0`

Override them with environment variables if needed.

Example for the later `Format5 / Binary / 28800 / 8E2 / sum-check on` setup on iQ-R `R08CPU`:

```bash
MCPROTOCOL_BAUD=28800 \
MCPROTOCOL_STOP_BITS=2 \
MCPROTOCOL_FRAME=c4-binary \
MCPROTOCOL_SUM_CHECK=on \
MCPROTOCOL_SERIES=iqr \
./examples/linux_cli/supported_device_rw_soak.sh
```

Example for the same serial settings on L-series `L26CPU-BT`:

```bash
MCPROTOCOL_BAUD=28800 \
MCPROTOCOL_STOP_BITS=2 \
MCPROTOCOL_FRAME=c4-binary \
MCPROTOCOL_SUM_CHECK=on \
MCPROTOCOL_SERIES=ql \
./examples/linux_cli/supported_device_rw_soak.sh
```
