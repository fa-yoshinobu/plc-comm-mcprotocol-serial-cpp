# Hardware Validation

Audience: maintainers and future validation follow-up work.

This file is the validation matrix and backlog for the current real-hardware setup.

Use it together with:

- [../../../README.md](../../../README.md) for the repository overview
- [../../user/SETUP_GUIDE.md](../../user/SETUP_GUIDE.md) for the verified serial settings
- [../../user/USAGE_GUIDE.md](../../user/USAGE_GUIDE.md) for command behavior and fallback notes
- [RJ71C24_R2_RS232C_FORMAT4_2026-04-10.md](RJ71C24_R2_RS232C_FORMAT4_2026-04-10.md) for the dated evidence log

## Validation Flow

1. Confirm `cpu-model` and `loopback` first.
2. Run read-only paths before write-oriented checks.
3. Save the exact serial settings, PLC model, and module model.
4. Distinguish `native pass` from `emulated pass`.
5. Record PLC error codes for every native rejection.

## Current Matrix

### Host / Build Validation

| Area | Target | Current status | Notes |
|---|---|---|---|
| CMake build | Linux host | pass | `cmake --build build`, `ctest`, `mcprotocol_example_mcu_async`, and `docs` target passed |
| PlatformIO example build | `native-example` | pass | compile-check plus local execution of the built example |
| PlatformIO example build | `rpipico-arduino-example` | pass | compile-check on `Raspberry Pi Pico` environment |
| PlatformIO example build | `esp32-c3-devkitm-1-example` | pass | compile-check on `Espressif ESP32-C3-DevKitM-1` environment |

Validated target:

- PLC CPU: Mitsubishi iQ-R `R08CPU`
- Serial module: `RJ71C24-R2`
- Link: `RS-232C`
- Settings: `19200 / 8E1 / MC Protocol Format4 ASCII / CRLF / sum-check off / station 0`

| Area | Command path | Current status | Notes |
|---|---|---|---|
| CPU identification | `cpu-model` | native pass | returns `R08CPU`, `0x4801` |
| Loopback | `loopback` | native pass | validated on real hardware |
| Contiguous word read | `0401` via `read-words` | native pass | validated up to `960` words |
| Contiguous word write | `1401` via `write-words` | native pass | validated up to `960` words |
| Contiguous bit read | `0401` via `read-bits` | native pass | validated up to `3584` bits |
| Contiguous bit write | `1401` via `write-bits` | native pass | validated up to `3584` bits |
| Host buffer read | `0613` | native pass | validated up to `480` words |
| Host buffer write | `1613` | native pass | real-hardware verify and restore completed |
| Module buffer read | `0601` | native pass | validated up to `1920` bytes |
| Module buffer write | `1601` | native pass | real-hardware verify and restore completed |
| Random read | native `0403` | native ng | module returns `0x7F22` |
| Random read | CLI `random-read` | emulated pass | repeated batch reads |
| Random write words | native `1402` | native ng | module returns `0x7F22` |
| Random write words | CLI `random-write-words` | emulated pass | repeated batch word writes |
| Random write bits | native `1402` | native ng | module returns `0x7F23` |
| Random write bits | CLI `random-write-bits` | emulated pass | repeated batch bit writes |
| Multi-block read | native `0406` | native ng | module returns `0x7F22` |
| Multi-block read | CLI `probe-multi-block` | emulated pass | repeated block-wise batch reads |
| Multi-block write | native `1406` | native ng | module returns `0x7F22` |
| Multi-block write | CLI `probe-multi-block` | emulated pass | repeated block-wise batch writes |
| Monitor register/read | native `0801/0802` | native ng | `0801` register path returns `0x7F22` |
| Monitor register/read | CLI `probe-monitor` | emulated pass | repeated direct reads |
| Device-family read probe | `probe-all` | partial pass | `26/27` passed, `RD0` returned `0x7F22` |
| Device-family write probe | `probe-write-all` | pass with exclusions | `25/25` passed after excluding `RD0` and `S`, and using `F100` instead of `F0` |

## Stress / Endurance Snapshot

| Test | Result |
|---|---|
| `100-bit / 1 minute` | pass, `167 cycles`, `fail=0` |
| `100-word / 30 minutes` | pass, `2554 cycles`, `fail=0` |
| `read-words 960` | pass |
| `write-words 960` | pass |
| `read-bits 3584` | pass |
| `write-bits 3584` | pass |
| `host-buffer 480 + module-buffer 1920 / 1 minute` | pass, `17 cycles`, `fail=0` |
| `host/module buffer write-soak / 60 seconds` | pass, `64 cycles`, `fail=0` |
| `mixed supported-command soak / 61 seconds` | pass, `28 cycles`, `fail=0` |
| `extended mixed supported-command soak / 301 seconds` | pass, `140 cycles`, `fail=0` |

## Current HOLD Items

- `RD0` direct access on this setup
- native `0403`
- native `1402`
- native `0406`
- native `1406`
- native `0801/0802`

## Maintainer Note

The library now pins native request-data shapes for `1402`, `0406`, `0801`, and `0802` with host-side tests. At this point, the best current interpretation is:

- request encoding matches the official MC protocol reference examples or documented request structure
- the validated `RJ71C24-R2` setup still rejects those native commands
- the CLI therefore treats them as emulated features on this setup rather than as native pass
