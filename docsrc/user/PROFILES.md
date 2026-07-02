# PLC profiles

Each profile selects the command family and device-layout assumptions used by the serial MC Protocol codecs.

Choose one explicit profile in your application or configuration UI. The library intentionally does not infer it from `ReadTypeName`, CPU model text, or omitted configuration.

## Explicit selection is required

Always select one concrete `PlcProfile` value before sending real PLC requests.

- `PlcProfile::Unspecified` is a configuration error, not a fallback profile.
- No profile is inferred from CPU model text, response data, serial settings, or device strings.
- Short names and case variants are rejected by text parsing.
- Linux CLI wrappers require both `MCPROTOCOL_FRAME` and `MCPROTOCOL_PLC_PROFILE`; neither value is auto-filled.

## Profiles

| Canonical profile | Hardware | API selector | Current internal grouping / status |
| --- | --- | --- | --- |
| `melsec:iq-r` | MELSEC iQ-R serial modules | `PlcProfile::MelsecIqR` | iQ-R command/device-layout branch. Manual-derived request-shape differences include iQ-R subcommands and wider device references. |
| `melsec:iq-l` | MELSEC iQ-L serial modules | `PlcProfile::MelsecIqL` | Separate public profile using Q/L-compatible serial MC request shapes. Long timer/counter devices, `LZ`, `RD`, and `Un\HG` are rejected for serial MC. |
| `melsec:iq-f` | MELSEC iQ-F / FX5 serial paths | `PlcProfile::MelsecIqF` | Separate public profile using the confirmed FX5 C4 binary support surface. Unsupported FX5 devices and routes are rejected locally. |
| `melsec:q` | MELSEC-Q serial modules | `PlcProfile::MelsecQ` | Public Q profile. Currently grouped with the Q/L command-device layout used by the existing Q/L encoder path. |
| `melsec:l` | MELSEC-L serial modules | `PlcProfile::MelsecL` | Public L profile using the Q/L-compatible serial MC request shape. Keep it separate from `melsec:q` for target-family evidence. |
| `melsec:qna` | MELSEC QnA-compatible targets | `PlcProfile::MelsecQnA` | QnA command-family branch. Current implementation groups the shared QnA/AnA/AnU command-family behavior where the codec has no separate rule. |
| `melsec:ana-anu` | MELSEC AnA / AnU-compatible targets | `PlcProfile::MelsecAnAAnU` | Public AnA/AnU profile. Currently shares the QnA-family internal branch until a manual-backed or measured difference is added. |
| `melsec:a` | MELSEC-A-compatible targets | `PlcProfile::MelsecA` | A-series command-family branch. A-only paths such as ER/EW extended file-register commands require this profile. |

## How to select

```cpp
auto protocol = mcprotocol::serial::highlevel::make_c4_ascii_format4_protocol(
    mcprotocol::serial::PlcProfile::MelsecQ);
```

Or assign the field directly when you build a custom `ProtocolConfig`:

```cpp
mcprotocol::serial::ProtocolConfig protocol {};
protocol.plc_profile = mcprotocol::serial::PlcProfile::MelsecQ;
```

`PlcProfile::Unspecified` is a configuration error. It is useful only as the default internal value before your application selects a real profile.

## Profile-specific cautions

| Canonical profile | Caution |
| --- | --- |
| `melsec:iq-r` | Use for iQ-R serial-module routes. The supported device inventory is maintained in [SUPPORTED_REGISTERS.md](SUPPORTED_REGISTERS.md). Some command behavior remains target/module dependent. |
| `melsec:iq-l` | Kept separate from iQ-R because iQ-L serial-module behavior is Q/L-subcommand based even when CPU-side SLMP looks iQ-R-like. Do not use the long timer/counter family, `LZ`, `RD`, or `Un\HG` with this profile. |
| `melsec:iq-f` | Use for FX5/iQ-F serial paths. `V`, `ZR`, `DX`, `DY`, long timer/retentive-timer families, `Un\HG`, `Jn\...`, monitor, host-buffer, and module-buffer helper routes are not part of this profile. |
| `melsec:q` | Use for MELSEC-Q serial paths. The current implementation shares the Q/L request-shape branch but keeps Q as a separate public profile. |
| `melsec:l` | Use for MELSEC-L serial paths. The current implementation shares the Q/L request-shape branch but keeps L as a separate public profile. |
| `melsec:qna` | Select only when the target should use QnA-style command selection. |
| `melsec:ana-anu` | Select only when the target should use AnA/AnU command-family selection. |
| `melsec:a` | Select for A-series-compatible targets; extended file-register ER/EW commands require this profile. |

The profile does not replace serial settings. Baud rate, parity, stop bits, frame type, sum-check behavior, and station number still have to match the PLC serial module.
