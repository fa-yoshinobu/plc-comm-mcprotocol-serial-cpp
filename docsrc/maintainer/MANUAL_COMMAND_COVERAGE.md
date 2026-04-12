# Manual Command Coverage

This page tracks which MC protocol serial command families are implemented in this repository.

Scope:

- Primary reference: the Mitsubishi MC protocol serial manual
- Primary wire scope: serial `2C/3C/4C` frame families plus initial `1C/1E` support
- Current ASCII wire scope for `2C/3C/4C`: `Format1`, `Format2`, `Format3`, and `Format4`
- Public library scope: device memory access, buffer memory access, CPU-model, remote control,
  remote password control, loopback, and helper-qualified access over `0601/1601`
- Out of scope by design unless stated otherwise: label access, file control,
  serial-module dedicated commands beyond user-frame handling, and the later drive/file memory chapters

## Summary

- Device-memory chapter `8` is covered as command families: `0401`, `1401`, `0403`, `1402`,
  `0406`, `1406`, `0801`, `0802`
- Buffer-memory chapter `10` is covered as command families: `0613`, `1613`, `0601`, `1601`
- Implemented chapter `11` families now include `0101`, `1001`, `1002`, `1003`, `1005`, `1006`,
  `1617`, `1630`, `1631`, and `0619`
- Implemented chapter `13` user-frame families now include `0610`, `1610`, `1615`, `1618`, and `0631`
- `1E` now has initial chapter `18` coverage for device-memory read/write, random write, monitor,
  extended-file-register access, and special-function-module buffer read/write
- The library does not implement the full MC protocol serial command list

## Compile-time Trim Scope

Compile-time trimming in this repository works at these units:

- build-target unit
  host support and CLI can be dropped as separate targets
- command-family unit
  random, multi-block, monitor, host-buffer, module-buffer, CPU-model, and loopback each have their
  own switch
- codec-family unit
  ASCII vs binary and `4C` / `3C` / `2C` / `1C` / `1E` each have their own switch

Compile-time trimming does not yet work at these units:

- individual commands inside one family
  you cannot compile out only `0403` while keeping `1402`
- per-device variants inside one family
  you cannot compile out only `Jn\\...` or only helper-qualified `U...`
- per-series behavior
  `IQ-R`, `Q/L`, `QnA`, and `A` remain runtime protocol choices

For target-specific PASS/HOLD results, use
[HARDWARE_VALIDATION.md](../validation/reports/HARDWARE_VALIDATION.md).

## 2C/3C/4C Coverage Matrix

| Manual section | Command family | Status | Public surface | Notes |
| --- | --- | --- | --- | --- |
| Chapter 8 device memory | `0401` batch read words/bits | implemented | `async_batch_read_words`, `async_batch_read_bits`, `PosixSyncClient::read_words/read_bits`, CLI | `Jn\\...` extension-spec variants also implemented |
| Chapter 8 device memory | `1401` batch write words/bits | implemented | `async_batch_write_words`, `async_batch_write_bits`, `PosixSyncClient::write_words/write_bits`, CLI | `Jn\\...` extension-spec variants also implemented |
| Chapter 8 device memory | `0403` random read | implemented | `async_random_read`, `async_link_direct_random_read`, `PosixSyncClient::random_read`, CLI | Long-device restrictions follow the manual and target validation |
| Chapter 8 device memory | `1402` random write words/bits | implemented | `async_random_write_words`, `async_random_write_bits`, link-direct variants, `PosixSyncClient::random_write_word(s)/random_write_bit(s)`, CLI | `LTN/LSTN` native writes stay blocked per the serial manual's device-path restrictions |
| Chapter 8 device memory | `0406` multi-block read | implemented | `async_multi_block_read`, `async_link_direct_multi_block_read`, CLI | Long-device head restrictions follow the manual |
| Chapter 8 device memory | `1406` multi-block write | implemented | `async_multi_block_write`, `async_link_direct_multi_block_write`, CLI | Long-device head restrictions follow the manual |
| Chapter 8 device memory | `0801` monitor register | implemented | `async_register_monitor`, `async_link_direct_register_monitor`, `PosixSyncClient::register_monitor`, CLI | Some targets such as `FX5UC-32MT/D` still reject monitor on serial `3C/4C` |
| Chapter 8 device memory | `0802` monitor read | implemented | `async_read_monitor`, `PosixSyncClient::read_monitor`, CLI | Requires prior successful `0801` registration |
| Chapter 9 label access | `041A`, `141A`, `041C`, `141B` | not implemented | none | No label codec/client/public API exists |
| Chapter 10 buffer memory | `0613`, `1613` host-buffer read/write | implemented | `async_read_host_buffer`, `async_write_host_buffer`, CLI | Target-dependent support remains; see validation matrix |
| Chapter 10 buffer memory | `0601`, `1601` module-buffer read/write | implemented | `async_read_module_buffer`, `async_write_module_buffer`, CLI | Public qualified-word helpers intentionally reuse this path |
| Chapter 11 control/diagnostic | `0101` read CPU model | implemented | `async_read_cpu_model`, `PosixSyncClient::read_cpu_model`, CLI | Implemented and validated |
| Chapter 11 remote operation | `1001` remote RUN | implemented | `async_remote_run`, `PosixSyncClient::remote_run`, CLI | Public API exposes mode and clear-mode; CLI defaults to `no-force` + `no-clear`; hardware-validated on `RJ71C24-R2 + R120PCPU` |
| Chapter 11 remote operation | `1002` remote STOP | implemented | `async_remote_stop`, `PosixSyncClient::remote_stop`, CLI | Encodes the documented fixed `0001H` value; hardware-validated on `RJ71C24-R2 + R120PCPU` |
| Chapter 11 remote operation | `1003` remote PAUSE | implemented | `async_remote_pause`, `PosixSyncClient::remote_pause`, CLI | Public API exposes mode; CLI defaults to `no-force`; hardware-validated on `RJ71C24-R2 + R120PCPU` |
| Chapter 11 remote operation | `1005` remote latch clear | implemented | `async_remote_latch_clear`, `PosixSyncClient::remote_latch_clear`, CLI | Encodes the documented fixed `0001H` value; not hardware-validated in this repo yet |
| Chapter 11 password/error control | `1617` clear error information | implemented | `async_clear_error_information`, `PosixSyncClient::clear_error_information`, CLI | Implements the serial/C24 clear-error-information variant; request includes both communication-error-information words using the documented default values |
| Chapter 11 password/error control | `1630`, `1631` remote password unlock/lock | implemented | `async_unlock_remote_password`, `async_lock_remote_password`, sync wrappers, CLI | Enforces documented password-length rules: Q/L fixed `4`, iQ-R `6..32`; focused `RJ71C24-R2 + R120PCPU` `--series iqr` checks with a 10-character password currently return `0x7F22` for both `unlock` and `lock`, so this remains a target-dependent follow-up |
| Chapter 11 remote operation | `1006` remote RESET | implemented | `async_remote_reset`, `PosixSyncClient::remote_reset`, CLI | Manual notes some targets may reset before returning a response; this library treats a pure no-response timeout as success for this command. Hardware-validated on `RJ71C24-R2 + R120PCPU` after enabling the target-side remote RESET parameter |
| Chapter 11 control/diagnostic | `0619` loopback | implemented | `async_loopback`, CLI | Implemented and validated |
| Chapter 12 file control | `1810`..`182A` | not implemented | none | No file-control codec/client/public API exists |
| Chapter 13 serial-module user frame | `0610`, `1610` | implemented | `async_read_user_frame`, `async_write_user_frame`, `async_delete_user_frame`, sync wrappers, CLI | This is C24 user-frame registration/read/delete, not chapter `8` device-memory monitor |
| Chapter 13 serial-module control extras | `1618`, `1615`, `0631` | implemented | `async_control_global_signal`, `async_initialize_c24_transmission_sequence`, `async_deregister_cpu_monitoring`, sync wrappers, CLI | `1615` is binary `4C` format-5 only; `1618` and `0631` are `2C/3C/4C` only |
| Chapter 13 serial-module advanced extras | `1612`, `0630`, `2101` | not implemented | none | `1612` changes link settings, `0630` is the large CPU-monitor registration family, and `2101` is receive-side on-demand data rather than a normal outbound request |
| Chapter 15 drive/file memory | `0201`..`0206`, `0808`, `1202`..`1207` | not implemented | none | No drive/file memory codec/client/public API exists |

## Important Scope Notes

- `Jn\\...` support in this repository is not a separate command family. It is the same chapter `8`
  command set encoded with device extension specification where the target requires it.
- Qualified `U...\\G...` and `U...\\HG...` access is intentionally exposed as helper-qualified access
  over `0601/1601`, not as a separate public native command family.
- `2C` support reuses the same command payload codecs as `3C/4C`, but only on the ASCII frame
  layer. Binary remains `4C Format5` only, and this repository has not hardware-validated `2C`
  yet.
- `1C` now has initial ASCII support through the existing APIs for contiguous device-memory
  read/write, random write, monitor, module-buffer access, extended-file register access, and
  loopback on `PlcSeries::A` and `PlcSeries::QnA`. Extended-file support uses ACPU-common
  `ER/EW/ET/EM/ME` on `PlcSeries::A` and direct `NR/NW` on `PlcSeries::QnA`. Hardware validation
  remains open.
- `1E` now has initial ASCII/binary support on `PlcSeries::A` and `PlcSeries::QnA` through the
  existing APIs for contiguous device-memory read/write, random write, monitor register/read,
  block-addressed extended-file-register read/write/random-write/monitor, and special-function-
  module buffer read/write. The direct extended-file-register read/write path is currently
  `PlcSeries::A` only. `1E` still excludes CPU-model, host-buffer, remote control,
  password/error control, loopback, multi-block, qualified helper, and link-direct paths.
- "Implemented" in this table means the library has codec/client/public entry points. It does not
  mean every PLC/serial target accepts that command family.
- Device-level restrictions still apply. Examples:
  `LTS/LTC/LSTS/LSTC` are treated via the documented structured paths,
  `LTN/LSTN` native writes remain blocked, and `FX5UC-32MT/D` treats `0801/0802` as unsupported on
  serial `3C/4C`.

## Current Conclusion

- If the question is "are chapter `8` device-memory command families all present?", the answer is
  yes for the intended `2C/3C/4C` library scope.
- If the question is "does this repository implement every MC protocol serial `2C/3C/4C` command?", the
  answer is no. The main missing families are label access, file control, registered-data
  commands, network/module extras, and drive/file memory commands.
