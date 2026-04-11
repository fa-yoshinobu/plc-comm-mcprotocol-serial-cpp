# Takebishi Capture Scenarios

## Goal

Capture these MC protocol families if possible:

- `0403` random-read
- `1402` random-write-words
- `1402` random-write-bits

`0406` is already sufficiently captured.  
`0801/0802` is low priority because ordinary OPC polling often does not use monitor commands.

## Basic Rule

Use sparse tags and operate on multiple tags at once.

If you use contiguous ranges only, the server will tend to emit:

- `0401` for read
- `1401` for write

So the capture should force discontinuous access.

## Recommended Tags

Prepare plain tags like these:

- `D100`
- `D101`
- `D105`
- `M100`
- `M101`
- `M105`
- `M110`
- `M115`

If available, also:

- `B100`
- `B105`

## Capture Scenarios

### 1. Capture `0403` random-read word

Read these two tags at the same time:

- `D100`
- `D105`

Expected tendency:

- sparse word random-read
- likely `0403`

Do not include `D101`, `D102`, `D103`, `D104` in the same read.

### 2. Capture `0403` random-read bit

Read these two tags at the same time:

- `M100`
- `M105`

Expected tendency:

- sparse bit random-read
- likely `0403`

Do not mix with `M101-M104`.

### 3. Capture `1402` random-write-words sparse

Write these tags in one multi-item write operation:

- `D100=1`
- `D105=2`

Expected tendency:

- sparse word random-write
- likely `1402`

Important:

- do not write them one by one
- send one write request containing both items

### 4. Capture `1402` random-write-words dense

Write these tags in one multi-item write operation:

- `D100=1`
- `D101=2`

This is useful for comparing whether the OPC server prefers:

- `1402`
- or falls back to `1401`

### 5. Capture `1402` random-write-bits sparse

Write these tags in one multi-item write operation:

- `M100=1`
- `M105=0`
- `M110=1`
- `M115=0`

Expected tendency:

- sparse bit random-write
- likely `1402`

Again, one request is required.  
If the OPC client writes each tag separately, this will not help.

## Operation Rules

- Enable capture before the operation.
- Run only one scenario at a time.
- Stop unrelated polling/groups if possible.
- Do not mix contiguous and sparse tag sets in the same test.
- For writes, use a client operation that writes multiple items in one call.

## Good Capture Order

1. `D100` + `D105` simultaneous read
2. `M100` + `M105` simultaneous read
3. `D100=1`, `D105=2` simultaneous write
4. `M100=1`, `M105=0`, `M110=1`, `M115=0` simultaneous write

## What To Save

Save raw TX/RX data if possible.

Useful files:

- raw hex dump
- packet list export
- text export from raw data view

Workspace/session files alone are not enough.

## Success Criteria

Useful capture contains at least one of:

- `0403`
- `1402 0000`
- `1402 0001`

If only `0401` / `1401` appear, the test was too contiguous or too fragmented.
