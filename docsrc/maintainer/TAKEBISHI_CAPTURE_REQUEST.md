# Takebishi Capture Request

## 1. Purpose

Capture `0403` random-read if possible.

- `0403` random-read

`0406`, `1406`, and `1402 0001` are already sufficiently captured.

## 2. Rules

- Run UART access serially.
- Run one scenario at a time.
- Stop unrelated polling/groups if possible.
- Save raw TX/RX, not only workspace/session files.
- Prefer read-only runs.

## 3. `0403` Word Read

Read these two tags at the same time:

- `D100`
- `D105`

Do not mix these with contiguous reads such as `D101` to `D104`.

## 4. `0403` Bit Read

Read these two tags at the same time:

- `M100`
- `M105`

Do not mix these with contiguous reads such as `M101` to `M104`.

## 5. Save

Save any of these if possible:

- raw hex dump
- packet list export
- raw data view export

## 6. Success

Useful capture contains:

- `0403`

If only `0406` appears again, treat that as evidence that the current Takebishi read optimization is
collapsing sparse read requests into multi-block read.
