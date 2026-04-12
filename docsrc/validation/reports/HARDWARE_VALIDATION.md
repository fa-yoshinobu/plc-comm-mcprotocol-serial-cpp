# Hardware Validation

Audience: maintainers and future validation follow-up work.

This file is the validation matrix and backlog for the current real-hardware setups.

Use it together with:

- [../../../README.md](../../../README.md) for the repository overview
- [../../user/MCU_QUICKSTART.md](../../user/MCU_QUICKSTART.md) for the firmware-side entry path
- [FX5UC_32MT_D_RS232C.md](FX5UC_32MT_D_RS232C.md) for the consolidated FX/iQ-F target report
- [LJ71C24_RS232C.md](LJ71C24_RS232C.md) for the consolidated L-series target report
- [QJ71C24N_RS232C.md](QJ71C24N_RS232C.md) for the consolidated Q-series target report
- [RJ71C24_R2_RS232C.md](RJ71C24_R2_RS232C.md) for the consolidated iQ-R target report

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
| PlatformIO example build | `mega2560-arduino-uart-example` | pass | compile-check on `Arduino Mega 2560` environment |

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
| Qualified native read/write | `read-native-qualified-words` / `write-native-qualified-words` over native extended-device access | unsupported / diagnostic only | helper-only is the supported specification on this target; historical native probes produced `0x7F22`, timeout, `0x4031`, and semantically mismatched success |
| Random read | native `0403` | native pass | `2026-04-11` focused recheck passed natively under `--series ql`; `--series iqr` still failed with `0x7F23` |
| Random write words | native `1402` | native pass | `2026-04-11` focused recheck passed natively under `--series ql`; `--series iqr` still failed with `0x7F23` |
| Random write bits | native `1402` | native pass | `2026-04-11` focused recheck passed natively under `--series ql`; `--series iqr` still failed with `0x7F23` |
| Multi-block read | native `0406` | native pass | `2026-04-11` recheck on the corrected binary encoder passed with `0406 0002` |
| Multi-block write | native `1406` | native pass | `2026-04-11` recheck on the corrected binary encoder passed with `1406 0002` and restore |
| Monitor register/read | native `0801/0802` | native pass | `2026-04-11` focused recheck passed under `--series ql`; `--series iqr` still produced `0801=0x7F23` and raw `0802=0x7155` |
| iQ-R-only device spot reads | `SM` / `SD` / `RD` / `LZ` | target-dependent | `2026-04-12` `RJ71C24-R2 + R120PCPU / Format5 Binary / --series iqr`: `SM0`, `SD0`, and `RD0` passed; `LZ0`, `LZ1`, and `LZ0 LZ1` random-read probes returned `0x7F23` |
| Long current-value device spot reads | `LTN` / `LSTN` / `LCN` | target-dependent | `2026-04-12` `RJ71C24-R2 + R120PCPU / Format5 Binary / --series iqr`: `read-words LTN0 4`, `read-words LSTN0 4`, and `read-words LCN0 2` passed; `random-read LTN0`, `LSTN0`, and `LCN0` returned `0x7F23` |
| Device-family read probe | `probe-all` | pass | `26/26` passed after dropping `RD` from the supported device set |
| Device-family write probe | `probe-write-all` | pass with exclusions | `25/25` passed after excluding `S` and using `F100` instead of `F0` |

Additional validated target:

- PLC CPU: Mitsubishi iQ-F `FX5UC-32MT/D`
- Link: `RS-232C`
- Settings: `38400 / 8E2 / MC Protocol Format5 Binary / sum-check on / station 0`
- Validated family selection: use `ql`; `iqr` caused contiguous `D100` / `M100` reads to fail with `0x7E40`

| Area | Command path | Current status | Notes |
|---|---|---|---|
| CPU identification | `cpu-model` | native pass | returns `FX5UC-32MT/D`, `0x4A91` |
| Contiguous read/write | `0401/1401` via `read-*` / `write-*` | native pass, narrow subset | validated `21`-target subset passed under `--series ql`; the FX5 communication manual's serial `3C/4C` accessible-device table marks `DX`, `DY`, `V`, and `ZR` inaccessible, matching observed `DX10` / `DY10` `0x7E43` probes and the decision to exclude all four from the FX subset |
| Supported-device soak | `fx5u_supported_device_rw_soak.sh` | pass | two `180` second runs passed with no protocol errors on the screened `21` target subset |
| Host buffer read | `0613` | native ng / not applicable | `probe-host-buffer` returned `0x7E40` |
| Host buffer write | `1613` | native ng / not applicable | backup read failed with `0x7E40` |
| Module buffer read/write | `0601/1601` | native ng / not applicable | `probe-module-buffer` and `probe-write-module-buffer` returned `0x7E40` |
| Qualified helper read/write | `read-qualified-words` / `write-qualified-words` over `0601/1601` | helper ng / not applicable | helper `U3E0\\G10` and `U3E0\\HG20` returned `0x7E40` |
| Qualified native read/write | `read-native-qualified-words` / `write-native-qualified-words` over native extended-device access | unsupported / not applicable | native `U3E0\\G10` returned `0x7E43`; native `HG` path is not applicable under `--series ql` |
| Random read | native `0403` | native pass | `RJ71C24-R2`, `LJ71C24`, and `QJ71C24N` focused rechecks passed under `--series ql`; `RJ71C24-R2 --series iqr` still failed and should be treated as a false-negative probe mode. `FX5UC-32MT/D`: after switching non-iQ-R binary word/dword counts from two-byte fields to the one-byte Q/L-era layout used in the binary manual examples, focused single/dense/sparse word/bit probes passed natively |
| Random write words | native `1402` | native pass | `RJ71C24-R2`, `LJ71C24`, and `QJ71C24N` focused rechecks passed under `--series ql`; `RJ71C24-R2 --series iqr` still failed and should be treated as a false-negative probe mode. `FX5UC-32MT/D`: the same non-iQ-R binary one-byte word/dword count fix made focused single/dense/sparse `D100` probes pass natively with restore |
| Random write bits | native `1402` | native pass | `RJ71C24-R2`, `LJ71C24`, and `QJ71C24N` focused rechecks passed under `--series ql`; `RJ71C24-R2 --series iqr` still failed and should be treated as a false-negative probe mode. `FX5UC-32MT/D`: after the binary one-byte count fix from the page `108` example plus unrelated captured binary traffic, a second FX5U recheck isolated pair-swapped bit-address parity inside each two-point unit; correcting that made focused single/dense/sparse `M100..M115` probes pass natively |
| Multi-block read | native `0406` | native pass | after the binary block-count fix based on capture and manual re-read, `probe-multi-block` read passed natively |
| Multi-block write | native `1406` | native pass | after the follow-up binary bit-block pair-order fix, `probe-multi-block[mixed]` passed natively with restore |
| Monitor register/read | `0801/0802` | target-dependent | `RJ71C24-R2`, `LJ71C24`, and `QJ71C24N` focused rechecks passed under `--series ql`; `RJ71C24-R2 --series iqr` still produced `0801=0x7F23`, raw `0802=0x7155`. `FX5UC-32MT/D`: `0801` and raw `0802` both returned `0x7E40`, and the FX5 communication manual's serial `3C/4C` command list does not include `0801/0802` |

Additional validated target:

- PLC CPU: Mitsubishi L-series `L26CPU-BT`
- Serial module: `LJ71C24`
- Link: `RS-232C`
- Settings: `28800 / 8E2 / MC Protocol Format5 Binary / sum-check on / station 0`
- Validated family selection: use `ql`; `iqr` caused even contiguous `D100` / `M100` reads to fail with `0x7F22`

| Area | Command path | Current status | Notes |
|---|---|---|---|
| CPU identification | `cpu-model` | native pass | returns `L26CPU-BT`, `0x0542` |
| Contiguous read/write | `0401/1401` via `read-*` / `write-*` | native pass | supported-device screening `25/25` passed with non-low addresses under `--series ql` |
| Supported-device soak | `supported_device_rw_soak.sh` | pass | two `180` second runs passed with no protocol errors; bit-family readback often showed RUN overwrite |
| Host buffer read | `0613` | native pass | `probe-host-buffer` passed |
| Host buffer write | `1613` | hold | `probe-write-host-buffer` wrote but verify mismatched at start `0` |
| Module buffer read/write | `0601/1601` | native pass | `probe-module-buffer` and `probe-write-module-buffer` passed |
| Qualified helper read/write | `read-qualified-words` / `write-qualified-words` over `0601/1601` | helper pass, narrow scope | helper `U3E0\\G10=0x0000`; helper `U3E0\\HG20` read/write/restore passed |
| Qualified native read/write | `read-native-qualified-words` / `write-native-qualified-words` over native extended-device access | unsupported / not applicable | helper-only is the supported specification on this target; native `U3E0\\G10` returned `0x4030`; native `HG` path is not applicable under `--series ql` |
| Random read | native `0403` | native pass | focused `single/dense/sparse` rechecks passed under `--series ql` |
| Random write words | native `1402` | native pass | focused `single/dense/sparse` rechecks passed under `--series ql` with restore |
| Random write bits | native `1402` | native pass | focused `single/dense/sparse` rechecks passed under `--series ql` with restore |
| Multi-block read | native `0406` | native pass | focused `probe-multi-block[mixed]` recheck passed natively |
| Multi-block write | native `1406` | native pass | focused `probe-multi-block[mixed]` recheck passed natively with restore |
| Monitor register/read | `0801/0802` | native pass | focused recheck passed under `--series ql` |

Additional validated target:

- PLC CPU: Mitsubishi Q-series `Q06UDVCPU`
- Serial module: `QJ71C24N`
- Link: `RS-232C`
- Settings: `19200 / 8E2 / MC Protocol Format5 Binary / sum-check on / station 0`
- Validated family selection: use `ql`

| Area | Command path | Current status | Notes |
|---|---|---|---|
| CPU identification | `cpu-model` | native pass | returns `Q06UDVCPU`, `0x0368` |
| Contiguous read/write | `0401/1401` via `read-*` / `write-*` | native pass | `read-words D100 1` and `read-bits M100 1` passed; supported-device soak passed |
| Supported-device soak | `supported_device_rw_soak.sh` | pass | one `180` second run passed with no protocol errors; bit-family readback often showed RUN overwrite |
| Host buffer read | `0613` | native pass | `probe-host-buffer` passed |
| Host buffer write | `1613` | hold | `probe-write-host-buffer` wrote but verify mismatched at start `0` |
| Module buffer read/write | `0601/1601` | native pass | `probe-module-buffer` and `probe-write-module-buffer` passed |
| Qualified helper read/write | `read-qualified-words` / `write-qualified-words` over `0601/1601` | helper pass, narrow scope | helper `U3E0\\G10=0x1000`; helper `U3E0\\HG20` read/write/restore passed |
| Qualified native read/write | `read-native-qualified-words` / `write-native-qualified-words` over native extended-device access | unsupported / not applicable | helper-only is the supported specification on this target; native `U3E0\\G10` returned `0x4030`; native `HG` path is not applicable under `--series ql` |
| Random read | native `0403` | native pass | focused `single/dense/sparse` rechecks passed under `--series ql` |
| Random write words | native `1402` | native pass | focused `single/dense/sparse` rechecks passed under `--series ql` with restore |
| Random write bits | native `1402` | native pass | focused `single/dense/sparse` rechecks passed under `--series ql` with restore |
| Multi-block read | native `0406` | native pass | focused `probe-multi-block[mixed]` recheck passed natively |
| Multi-block write | native `1406` | native pass | focused `probe-multi-block[mixed]` recheck passed natively with restore |
| Monitor register/read | `0801/0802` | native pass | focused recheck passed under `--series ql` |

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

Target-specific holds:

- native `LZ` double-word random-read on `RJ71C24-R2 + iQ-R CPU / Format5 Binary / --series iqr`
  (`random-read LZ0`, `LZ1`, and `LZ0 LZ1` returned `0x7F23` on `2026-04-11` and `2026-04-12`;
  local support now treats `LZ` as an iQ-R-only double-word device and rejects Q/L-mode requests
  before transmit)
- native `0403` random-read on `LTN`, `LSTN`, and `LCN` on `RJ71C24-R2 + iQ-R CPU / Format5 Binary / --series iqr`
  (`2026-04-12` spot probes `random-read LTN0`, `LSTN0`, and `LCN0` returned `0x7F23`, while
  spot batch reads `read-words LTN0 4`, `read-words LSTN0 4`, and `read-words LCN0 2` passed)
- host/module buffer access on `FX5UC-32MT/D`

Unsupported / diagnostic-only items:

- native qualified access is helper-only by specification and is not tracked as an active hold

## Maintainer Note

The library now pins native request-data shapes for `0403`, `1402`, `0406`, `1406`, `0801`, and
`0802` with host-side tests. At this point, the best current interpretation is:

- request encoding matches the official MC protocol reference examples or documented request structure
- `RJ71C24-R2`, `LJ71C24`, and `QJ71C24N` all pass the practical native random / write / multi-block / monitor families under `--series ql`
- `RJ71C24-R2 --series iqr` remains useful as a probe mode, but it produces false negatives for random and monitor traffic on that target
- `FX5UC-32MT/D` now passes native `0403`, `1402`, `0406`, and `1406` after the documented host-side compatibility fixes
- the FX5 communication manual's serial `3C/4C` command list does not include `0801/0802`, so that target's repeated `0x7E40` monitor result should be treated as unsupported behavior rather than an unresolved encoder bug
- qualified helper commands and native qualified probes should stay documented as separate paths, with
  native qualified marked unsupported by specification
