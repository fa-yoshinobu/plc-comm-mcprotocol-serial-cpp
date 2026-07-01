# Supported registers

This page is a working inventory of device families used by the high-level API.

The per-profile read/write support contract is not final until the matching manual evidence,
library policy, and hardware observations are separated in
[MANUAL_DERIVED_RULES.md](../maintainer/MANUAL_DERIVED_RULES.md). Treat the profile tables below as
candidate support surfaces, not as final PLC-wide guarantees.

The high-level string parser accepts plain device strings only. The examples below are address syntax examples, not guaranteed range limits for every PLC model.

## `melsec:iq-r` read/write candidate

For `PlcProfile::MelsecIqR` / canonical profile `melsec:iq-r`, the following device families are
currently listed as read/write candidates.

| Route | Read/write device families |
| --- | --- |
| Plain bit devices | `X`, `Y`, `M`, `L`, `SM`, `F`, `V`, `B`, `TS`, `TC`, `STS`, `STC`, `CS`, `CC`, `SB`, `S`, `DX`, `DY`, `LTS`, `LTC`, `LSTS`, `LSTC`, `LCS`, `LCC` |
| Plain word devices | `D`, `SD`, `W`, `TN`, `STN`, `CN`, `SW`, `LTN`, `LSTN`, `LCN`, `LZ`, `Z`, `R`, `RD`, `ZR` |
| Link-direct devices | `Jn\X`, `Jn\Y`, `Jn\B`, `Jn\W`, `Jn\SB`, `Jn\SW` |
| Qualified buffer memory | `Un\G`, `Un\HG` |

This table is a candidate iQ-R support surface for request encoding and command execution. Actual
value changes can still be constrained by the target PLC program, module configuration, or
special-device semantics.

## `melsec:iq-l` read/write candidate

For `PlcProfile::MelsecIqL` / canonical profile `melsec:iq-l`, the observed serial MC surface is
Q/L-shaped. iQ-L is kept as a separate public profile because CPU-side SLMP behavior can still
look iQ-R-like.

The iQ-L serial MC path uses Q/L-compatible request shapes. CPU-side SLMP behavior can still look
iQ-R-like, so keep serial MC support and CPU/SLMP support separate when recording target evidence.

| Route | Observed iQ-L serial MC status |
| --- | --- |
| Plain bit read/write | `X`, `Y`, `M`, `L`, `SM`, `F`, `V`, `B`, `TS`, `TC`, `STS`, `STC`, `CS`, `CC`, `SB`, `DX`, `DY` |
| Plain bit read-only | `S` |
| Plain word read/write | `D`, `SD`, `W`, `TN`, `STN`, `CN`, `SW`, `Z`, `R`, `ZR` |
| Native-qualified read/write | `Un\G` |
| Not supported | `LTS`, `LTC`, `LSTS`, `LSTC`, `LCS`, `LCC`, `LTN`, `LSTN`, `LCN`, `LZ`, `RD`, `Un\HG` |
| Not confirmed on observed setup | `Jn\X`, `Jn\Y`, `Jn\B`, `Jn\W`, `Jn\SB`, `Jn\SW` returned `0x4031` before write backup could be taken. |

Use `Un\G` when qualified buffer access is validated for the target. The current observed iQ-L
route uses native-qualified access with the Q/L-compatible `0080` wire shape; the `0601/1601`
helper route is not valid for this target observation.

Observed iQ-L native random-read coverage is narrower than batch-read coverage. Keep `TS`, `TC`,
`STS`, `STC`, `CS`, `CC`, `DX`, and `DY` out of the native `random-read` route unless later manual
evidence or target retest changes that rule.

## `melsec:q-l` read/write candidate

For `PlcProfile::MelsecQL` / canonical profile `melsec:q-l`, the following device families are
currently listed as read/write candidates.

| Route | Read/write device families |
| --- | --- |
| Plain bit devices | `X`, `Y`, `M`, `L`, `SM`, `F`, `V`, `B`, `TS`, `TC`, `STS`, `STC`, `CS`, `CC`, `SB`, `S`, `DX`, `DY` |
| Plain word devices | `D`, `SD`, `W`, `TN`, `STN`, `CN`, `SW`, `Z`, `R`, `ZR` |
| Link-direct devices | `Jn\X`, `Jn\Y`, `Jn\B`, `Jn\W`, `Jn\SB`, `Jn\SW` |
| Qualified buffer memory | `Un\G` |

This table is a candidate Q/L support surface for request encoding and command execution. Actual
value changes can still be constrained by the target PLC program, module configuration, or
special-device semantics.

## Bit device families

| Family | Kind | Example address | Notes |
| --- | --- | --- | --- |
| `X` | Input relay | `X10` | Hexadecimal address. |
| `Y` | Output relay | `Y10` | Hexadecimal address. |
| `M` | Internal relay | `M100` | Decimal address. |
| `L` | Latch relay | `L100` | Decimal address. |
| `SM` | Special relay | `SM100` | Decimal address. |
| `F` | Annunciator | `F100` | Decimal address. |
| `V` | Edge relay | `V100` | Decimal address. |
| `B` | Link relay | `B100` | Hexadecimal address. |
| `TS`, `TC` | Timer contact / coil | `TS0` | Decimal address. |
| `STS`, `STC` | Retentive timer contact / coil | `STS0` | Decimal address. |
| `CS`, `CC` | Counter contact / coil | `CS0` | Decimal address. |
| `SB` | Link special relay | `SB100` | Hexadecimal address. |
| `S` | Step relay | `S100` | Decimal address. |
| `DX`, `DY` | Direct access input/output | `DX10` | Hexadecimal address. |
| `LTS`, `LTC` | Long timer contact / coil | `LTS0` | Decimal address. |
| `LSTS`, `LSTC` | Long retentive timer contact / coil | `LSTS0` | Decimal address. |
| `LCS`, `LCC` | Long counter contact / coil | `LCS0` | Decimal address. |

## Word device families

| Family | Kind | Example address | Notes |
| --- | --- | --- | --- |
| `D` | Data register | `D100` | Decimal address. |
| `SD` | Special register | `SD100` | Decimal address. |
| `W` | Link register | `W100` | Hexadecimal address. |
| `TN` | Timer current value | `TN0` | Decimal address. |
| `STN` | Retentive timer current value | `STN0` | Decimal address. |
| `CN` | Counter current value | `CN0` | Decimal address. |
| `SW` | Link special register | `SW100` | Hexadecimal address. |
| `LTN` | Long timer current value | `LTN0` | Decimal address; treated as double-word in random helpers by default. |
| `LSTN` | Long retentive timer current value | `LSTN0` | Decimal address; treated as double-word in random helpers by default. |
| `LCN` | Long counter current value | `LCN0` | Decimal address; treated as double-word in random helpers by default. |
| `LZ` | Long index register | `LZ0` | Decimal address; treated as double-word in random helpers by default. |
| `Z` | Index register | `Z0` | Decimal address. |
| `R` | File register | `R0` | Decimal address. |
| `RD` | Module access register | `RD0` | Decimal address. |
| `ZR` | File register | `ZR0` | Decimal address. |

## Addressing notes

| Topic | Current behavior |
| --- | --- |
| Plain device string | Supported. Use forms such as `D100`, `M100`, `X10`, `W100`, `LZ0`. |
| Hexadecimal address families | `X`, `Y`, `B`, `W`, `SB`, `SW`, `DX`, and `DY` parse their numeric part as hexadecimal. |
| `:D` / `:F` suffix | Not supported by the current high-level parser. Use typed C++ fields such as `double_word` where available. |
| `.n` bit-in-word suffix | Not supported by the current high-level parser. |
| Long timer/counter state reads | Use `read_long_state_bits()` for `LTS/LTC/LSTS/LSTC/LCS/LCC`. `LTS/LTC/LSTS/LSTC` use status-block reads internally; `LCS/LCC` use direct bit reads internally. |
| Long timer/counter restrictions | Random and multi-block operations reject some long contact/coil devices. Check `Status` and use the dedicated long-state helper for state reads. |
| Link-direct access | Use the `read_link_direct_*()` / `write_link_direct_*()` helpers for `Jn\X/Y/B/SB` bit devices and `Jn\W/SW` word devices. Binary mode is the confirmed route on the current R120/RJ71C24 setup. |
| Qualified buffer memory | Use `read_qualified_words()` / `write_qualified_words()` for practical `Un\Gn` / `Un\HGn` access. Native qualified helpers are diagnostic probes only. |
| Trace logging | Set `MCPROTOCOL_SERIAL_TRACE=1` with the synchronous host client to log MC TX/RX frame bytes. |

Profile-specific range limits depend on the PLC family and serial module. See [PROFILES.md](PROFILES.md) before choosing the final profile for a live system.
