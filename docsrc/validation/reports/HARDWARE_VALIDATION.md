# Hardware Validation

Audience: maintainers and future validation follow-up work.

This file is the validation matrix and backlog for the current real-hardware setups.

Use it together with:

- [../../../README.md](../../../README.md) for the repository overview
- [../../user/SETUP_GUIDE.md](../../user/SETUP_GUIDE.md) for the verified serial settings
- [../../user/USAGE_GUIDE.md](../../user/USAGE_GUIDE.md) for command behavior notes
- [FX5UC_32MT_D_RS232C_FORMAT5_2026-04-11.md](FX5UC_32MT_D_RS232C_FORMAT5_2026-04-11.md) for the dated FX/iQ-F evidence log
- [LJ71C24_RS232C_FORMAT5_2026-04-11.md](LJ71C24_RS232C_FORMAT5_2026-04-11.md) for the dated L-series / `LJ71C24` evidence log
- [QJ71C24N_RS232C_FORMAT5_2026-04-11.md](QJ71C24N_RS232C_FORMAT5_2026-04-11.md) for the dated Q-series / `QJ71C24N` evidence log
- [RJ71C24_R2_RS232C_FORMAT5_2026-04-11.md](RJ71C24_R2_RS232C_FORMAT5_2026-04-11.md) for the dated iQ-R / `RJ71C24-R2` Format5 evidence log
- [RJ71C24_R2_RS232C_FORMAT4_2026-04-10.md](RJ71C24_R2_RS232C_FORMAT4_2026-04-10.md) for the dated evidence log
- [RJ71C24_R2_RS232C_FORMAT4_2026-04-11.md](RJ71C24_R2_RS232C_FORMAT4_2026-04-11.md) for recovery and follow-up native-command rechecks

## Validation Flow

1. Confirm `cpu-model` and `loopback` first.
2. Run read-only paths before write-oriented checks.
3. Save the exact serial settings, PLC model, and module model.
4. Distinguish `native pass` from `native ng`.
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
| Qualified helper read/write | `read-qualified-words` / `write-qualified-words` over `0601/1601` | helper pass, narrow scope | recommended public path on this setup; `U3E0\\HG20` single-word read/write/restore passed; `2026-04-11` spot recheck `U3E0\\G10=0x83BD` |
| Qualified native read/write | `read-native-qualified-words` / `write-native-qualified-words` over native extended-device access | hold | mixed results across retries; `U3E0\\G10` produced both `0x7F22` and timeout, `U3E0\\HG20` native read returned `0x7F22`, and native `U3E0\\HG20` write timed out; no validated native write effect |
| Random read | native `0403` | native ng | module returns `0x7F22`; `2026-04-11` recheck with `--series iqr` switched to `0403 0002` and still failed |
| Random write words | native `1402` | native ng | module returns `0x7F22`; `2026-04-11` recheck with `--series iqr` switched to `1402 0002` and still failed |
| Random write bits | native `1402` | native ng | module returns `0x7F23` |
| Multi-block read | native `0406` | native ng | module returns `0x7F22`; `2026-04-11` recheck with `--series iqr` switched to `0406 0002` and still failed |
| Multi-block write | native `1406` | native ng | module returns `0x7F22`; `2026-04-11` recheck with `--series iqr` switched to `1406 0002` and still failed |
| Monitor register/read | native `0801/0802` | native ng | `0801` register path returns `0x7F22`; `2026-04-11` recheck with `--series iqr` switched to `0801 0002` and still failed |
| Device-family read probe | `probe-all` | pass | `26/26` passed after dropping `RD` from the supported device set |
| Device-family write probe | `probe-write-all` | pass with exclusions | `25/25` passed after excluding `S` and using `F100` instead of `F0` |

Additional validated target:

- PLC CPU: Mitsubishi iQ-F `FX5UC-32MT/D`
- Link: `RS-232C`
- Settings: `38400 / 8E2 / MC Protocol Format5 Binary / sum-check on / station 0`
- CLI family selection: use `--series ql`; `--series iqr` caused contiguous `D100` / `M100` reads to fail with `0x7E40`

| Area | Command path | Current status | Notes |
|---|---|---|---|
| CPU identification | `cpu-model` | native pass | returns `FX5UC-32MT/D`, `0x4A91` |
| Contiguous read/write | `0401/1401` via `read-*` / `write-*` | native pass, narrow subset | non-low-address screening passed on `21/25` targets under `--series ql`; `DX10`, `DY10`, `ZR10`, and `V100` failed with `0x7E43` and repeated the same code on read-only recheck |
| Supported-device soak | `fx5u_supported_device_rw_soak.sh` | pass | two `180` second runs passed with no protocol errors on the screened `21` target subset |
| Host buffer read | `0613` | native ng / not applicable | `probe-host-buffer` returned `0x7E40` |
| Host buffer write | `1613` | native ng / not applicable | backup read failed with `0x7E40` |
| Module buffer read/write | `0601/1601` | native ng / not applicable | `probe-module-buffer` and `probe-write-module-buffer` returned `0x7E40` |
| Qualified helper read/write | `read-qualified-words` / `write-qualified-words` over `0601/1601` | helper ng / not applicable | helper `U3E0\\G10` and `U3E0\\HG20` returned `0x7E40` |
| Qualified native read/write | `read-native-qualified-words` / `write-native-qualified-words` over native extended-device access | native ng / not applicable | native `U3E0\\G10` returned `0x7E43`; native `HG` path is not applicable under `--series ql` |
| Random read | native `0403` | native ng | `0x7F23` |
| Random write words | native `1402` | native ng | `0x7F23`; no write effect confirmed |
| Random write bits | native `1402` | native ng | `0x7F23`; no write effect confirmed |
| Multi-block read | native `0406` | native pass | after the binary block-count fix based on capture and manual re-read, `probe-multi-block` read passed natively |
| Multi-block write | native `1406` | hold | after the same fix, the write path reached `verify-mismatch`; contiguous restore still passed |
| Monitor register/read | native `0801/0802` | native ng / hold | `0801` register path returned `0x7E40` |

Additional validated target:

- PLC CPU: Mitsubishi L-series `L26CPU-BT`
- Serial module: `LJ71C24`
- Link: `RS-232C`
- Settings: `28800 / 8E2 / MC Protocol Format5 Binary / sum-check on / station 0`
- CLI family selection: use `--series ql`; `--series iqr` caused even contiguous `D100` / `M100` reads to fail with `0x7F22`

| Area | Command path | Current status | Notes |
|---|---|---|---|
| CPU identification | `cpu-model` | native pass | returns `L26CPU-BT`, `0x0542` |
| Contiguous read/write | `0401/1401` via `read-*` / `write-*` | native pass | supported-device screening `25/25` passed with non-low addresses under `--series ql` |
| Supported-device soak | `supported_device_rw_soak.sh` | pass | two `180` second runs passed with no protocol errors; bit-family readback often showed RUN overwrite |
| Host buffer read | `0613` | native pass | `probe-host-buffer` passed |
| Host buffer write | `1613` | hold | `probe-write-host-buffer` wrote but verify mismatched at start `0` |
| Module buffer read/write | `0601/1601` | native pass | `probe-module-buffer` and `probe-write-module-buffer` passed |
| Qualified helper read/write | `read-qualified-words` / `write-qualified-words` over `0601/1601` | helper pass, narrow scope | helper `U3E0\\G10=0x0000`; helper `U3E0\\HG20` read/write/restore passed |
| Qualified native read/write | `read-native-qualified-words` / `write-native-qualified-words` over native extended-device access | hold / not applicable | native `U3E0\\G10` returned `0x4030`; native `HG` path is not applicable under `--series ql` |
| Random read | native `0403` | native ng | `0x7F23` |
| Random write words | native `1402` | native ng | `0x7F23`; no write effect confirmed |
| Random write bits | native `1402` | native ng | `0x7F23`; no write effect confirmed |
| Multi-block read | native `0406` | native ng | `0x7F23` |
| Multi-block write | native `1406` | native ng | `0x7F22`; contiguous restore passed |
| Monitor register/read | native `0801/0802` | native ng / hold | `0801` register path returned `0x7F23` |

Additional validated target:

- PLC CPU: Mitsubishi Q-series `Q06UDVCPU`
- Serial module: `QJ71C24N`
- Link: `RS-232C`
- Settings: `28800 / 8E2 / MC Protocol Format5 Binary / sum-check on / station 0`
- CLI family selection: use `--series ql`

| Area | Command path | Current status | Notes |
|---|---|---|---|
| CPU identification | `cpu-model` | native pass | returns `Q06UDVCPU`, `0x0368` |
| Contiguous read/write | `0401/1401` via `read-*` / `write-*` | native pass | `read-words D100 1` and `read-bits M100 1` passed; supported-device soak passed |
| Supported-device soak | `supported_device_rw_soak.sh` | pass | one `180` second run passed with no protocol errors; bit-family readback often showed RUN overwrite |
| Host buffer read | `0613` | native pass | `probe-host-buffer` passed |
| Host buffer write | `1613` | hold | `probe-write-host-buffer` wrote but verify mismatched at start `0` |
| Module buffer read/write | `0601/1601` | native pass | `probe-module-buffer` and `probe-write-module-buffer` passed |
| Qualified helper read/write | `read-qualified-words` / `write-qualified-words` over `0601/1601` | helper pass, narrow scope | helper `U3E0\\G10=0x1000`; helper `U3E0\\HG20` read/write/restore passed |
| Qualified native read/write | `read-native-qualified-words` / `write-native-qualified-words` over native extended-device access | hold / not applicable | native `U3E0\\G10` returned `0x4030`; native `HG` path is not applicable under `--series ql` |
| Random read | native `0403` | native ng | `0x7F23` |
| Random write words | native `1402` | native ng | `0x7F23`; no write effect confirmed |
| Random write bits | native `1402` | native ng | `0x7F23`; no write effect confirmed |
| Multi-block read | native `0406` | native ng | `0x7F23` |
| Multi-block write | native `1406` | native ng | `0x7F22`; contiguous restore passed |
| Monitor register/read | native `0801/0802` | native ng / hold | `0801` register path returned `0x7F23` |

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

- native `0403`
- native `1402`
- native `0406`
- native `1406`
- native `0801/0802`
- native qualified extended-device access

## Maintainer Note

The library now pins native request-data shapes for `1402`, `0406`, `0801`, and `0802` with host-side tests. At this point, the best current interpretation is:

- request encoding matches the official MC protocol reference examples or documented request structure
- `2026-04-11` follow-up rechecks showed the same native failures even after switching those families to iQ-R subcommands with `--series iqr`
- the validated `RJ71C24-R2`, `LJ71C24`, `QJ71C24N`, and `FX5UC-32MT/D` setups still reject those native commands once the CLI family selection is matched to the actual CPU family
- the CLI should expose those native failures directly on this setup
- qualified helper commands and native qualified probes should stay documented as separate paths
- one exception is now known: binary `0406/1406` used one-byte block counts, not little-endian
  word counts; after correcting that host-side bug, `FX5UC-32MT/D` `0406` passed natively and
  `1406` advanced to write-verify follow-up
