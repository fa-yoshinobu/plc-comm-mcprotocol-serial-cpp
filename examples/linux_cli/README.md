# Linux CLI Examples

This directory contains low-risk shell examples for the Linux CLI.

Scripts:

- [safe_bringup_readonly.sh](safe_bringup_readonly.sh)
  - runs `cpu-model`
  - runs `loopback`
  - runs a small `read-words`
- [cyclic_read_words.sh](cyclic_read_words.sh)
  - repeats `read-words` for a configurable duration

Default communication settings match the validated setup:

- `19200`
- `8E1`
- `c4-ascii-f4`
- `sum-check off`
- `station 0`

Override them with environment variables if needed.
