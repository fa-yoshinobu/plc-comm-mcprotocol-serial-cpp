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
| Clear error information | `1617` via `error-clear` | native pass | validated on `R120PCPU / RJ71C24-R2 / --series iqr` |
| Remote RUN/STOP/PAUSE | `1001/1002/1003` via `remote-run` / `remote-stop` / `remote-pause` | native pass | validated on `R120PCPU / RJ71C24-R2 / --series iqr` |
| Remote RESET | `1006` via `remote-reset` | native ng / parameter-dependent | currently returns PLC error `0x408B`; the manual requires the target to be `STOP` and remote RESET to be enabled in the target parameter |
| Contiguous word read | `0401` via `read-words` | native pass | validated up to `960` words |
| Contiguous word write | `1401` via `write-words` | native pass | validated up to `960` words |
| Contiguous bit read | `0401` via `read-bits` | native pass | validated up to `3584` bits |
| Contiguous bit write | `1401` via `write-bits` | native pass | validated up to `3584` bits |
| Host buffer read | `0613` | native pass | validated up to `480` words |
| Host buffer write | `1613` | native pass | real-hardware verify and restore completed |
| Module buffer read | `0601` | native pass | validated up to `1920` bytes |
| Module buffer write | `1601` | native pass | real-hardware verify and restore completed |
| Qualified helper read/write | `read-qualified-words` / `write-qualified-words` over `0601/1601` | helper pass, narrow scope | recommended public path on this setup; `U3E0\\HG20` single-word read/write/restore passed |
| Qualified native read/write | `read-native-qualified-words` / `write-native-qualified-words` over native extended-device access | unsupported / diagnostic only | helper-only is the supported specification on this target; native qualified access is not treated as a supported workflow |
| Random read | native `0403` | native pass | validated under both `--series ql` and `--series iqr` on the current RJ71C24-R2 targets |
| Random write words | native `1402` | native pass | validated under both `--series ql` and `--series iqr` on the current RJ71C24-R2 targets |
| Random write bits | native `1402` | native pass | validated under both `--series ql` and `--series iqr` on the current RJ71C24-R2 targets |
| Multi-block read | native `0406` | native pass | validated on the current setup |
| Multi-block write | native `1406` | native pass | validated with restore on the current setup |
| Monitor register/read | native `0801/0802` | native pass | validated under both `--series ql` and `--series iqr` on the current RJ71C24-R2 targets |
| iQ-R-only spot devices and `Jn\\...` surface | `SM`, `SD`, `RD`, `LZ`, `J1\\...`, `LTN/LSTN/LCN` | native pass | current per-device read/write matrix lives in [RJ71C24_R2_RS232C.md](RJ71C24_R2_RS232C.md) |
| Special-device post-control sanity | `SM`, `SD`, `RD`, `LZ`, `LCN`, `J1\\...` under `--series iqr` | native pass | current focused read/write/restore checks pass on the validated target |
| User frame / serial-module extras | `0610`, `1610`, `1615`, `1618`, `0631` | native pass | validated on `R120PCPU / RJ71C24-R2 / --series iqr`; flash user-frame `0x03E8` reads back with matching registration data and `frame-bytes=0`, while buffer-memory frame `0x8001` returns `frame-bytes=5` |
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
| Host buffer read | `0613` | unsupported / not applicable | `probe-host-buffer` returned `0x7E40`, and the FX5 serial `3C/4C` command list does not list `0613` |
| Host buffer write | `1613` | unsupported / not applicable | backup read failed with `0x7E40`, and the FX5 serial `3C/4C` command list does not list `1613` |
| Module buffer read/write | `0601/1601` | unsupported / not applicable | `probe-module-buffer` and `probe-write-module-buffer` returned `0x7E40`, and the FX5 serial `3C/4C` command list does not list `0601/1601` |
| Qualified helper read/write | `read-qualified-words` / `write-qualified-words` over `0601/1601` | unsupported / not applicable | helper `U3E0\\G10` and `U3E0\\HG20` returned `0x7E40` because the underlying `0601/1601` family is not listed on the FX5 serial `3C/4C` command list |
| Qualified native read/write | `read-native-qualified-words` / `write-native-qualified-words` over native extended-device access | unsupported / not applicable | native `U3E0\\G10` returned `0x7E43`; native `HG` path is not applicable under `--series ql` |
| Random read | native `0403` | native pass | focused single/dense/sparse probes pass under `--series ql` |
| Random write words | native `1402` | native pass | focused single/dense/sparse probes pass with restore under `--series ql` |
| Random write bits | native `1402` | native pass | focused single/dense/sparse probes pass with restore under `--series ql` |
| Multi-block read | native `0406` | native pass | validated on the current setup |
| Multi-block write | native `1406` | native pass | validated with restore on the current setup |
| Monitor register/read | `0801/0802` | target-dependent | `0801` and raw `0802` returned `0x7E40`, and the FX5 communication manual's serial `3C/4C` command list does not include `0801/0802` |

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
| Host buffer write | `1613` | native pass | writable verification starts at `2`; `start 0/1` stayed unchanged on this target |
| Module buffer read/write | `0601/1601` | native pass | `probe-module-buffer` and `probe-write-module-buffer` passed |
| Qualified helper read/write | `read-qualified-words` / `write-qualified-words` over `0601/1601` | helper pass, narrow scope | helper `U3E0\\G10=0x0000`; helper `U3E0\\HG20` read/write/restore passed |
| Qualified native read/write | `read-native-qualified-words` / `write-native-qualified-words` over native extended-device access | unsupported / not applicable | helper-only is the supported specification on this target; native `U3E0\\G10` returned `0x4030`; native `HG` path is not applicable under `--series ql` |
| Random read | native `0403` | native pass | validated on the current setup |
| Random write words | native `1402` | native pass | validated with restore on the current setup |
| Random write bits | native `1402` | native pass | validated with restore on the current setup |
| Multi-block read | native `0406` | native pass | validated on the current setup |
| Multi-block write | native `1406` | native pass | validated with restore on the current setup |
| Monitor register/read | `0801/0802` | native pass | validated on the current setup |

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
| Host buffer write | `1613` | native pass | writable verification starts at `2`; `start 0/1` stayed unchanged on this target |
| Module buffer read/write | `0601/1601` | native pass | `probe-module-buffer` and `probe-write-module-buffer` passed |
| Qualified helper read/write | `read-qualified-words` / `write-qualified-words` over `0601/1601` | helper pass, narrow scope | helper `U3E0\\G10=0x1000`; helper `U3E0\\HG20` read/write/restore passed |
| Qualified native read/write | `read-native-qualified-words` / `write-native-qualified-words` over native extended-device access | unsupported / not applicable | helper-only is the supported specification on this target; native `U3E0\\G10` returned `0x4030`; native `HG` path is not applicable under `--series ql` |
| Random read | native `0403` | native pass | validated on the current setup |
| Random write words | native `1402` | native pass | validated with restore on the current setup |
| Random write bits | native `1402` | native pass | validated with restore on the current setup |
| Multi-block read | native `0406` | native pass | validated on the current setup |
| Multi-block write | native `1406` | native pass | validated with restore on the current setup |
| Monitor register/read | `0801/0802` | native pass | validated on the current setup |

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

- none at the command-family level on the currently validated targets

Unsupported / diagnostic-only items:

- native qualified access is helper-only by specification and is not tracked as an active hold

## Current Interpretation

- request encoding now matches the documented MC protocol request shapes used by the validated targets
- `RJ71C24-R2`, `LJ71C24`, and `QJ71C24N` pass the practical native random / write / multi-block / monitor families under `--series ql`
- `RJ71C24-R2` supports native `0403/1402/0801` on the validated `--series iqr` spot-device path
- `FX5UC-32MT/D` passes native `0403`, `1402`, `0406`, and `1406`
- `FX5UC-32MT/D` `0801/0802` should be treated as unsupported on serial `3C/4C`
- qualified helper commands and native qualified probes remain separate paths; native qualified stays unsupported by specification
