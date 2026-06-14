# Supported registers

This page lists device families supported by the high-level API.

The high-level string parser accepts plain device strings only. The examples below are address syntax examples, not guaranteed range limits for every PLC model.

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
| Long timer/counter restrictions | Some random and multi-block operations reject long contact/coil devices. Check `Status` and use simpler contiguous operations first. |

Profile-specific range limits depend on the PLC family and serial module. See [PROFILES.md](PROFILES.md) before choosing the final profile for a live system.
