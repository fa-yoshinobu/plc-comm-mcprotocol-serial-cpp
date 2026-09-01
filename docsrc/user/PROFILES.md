# PLC profiles

Each profile selects the command family and device-layout assumptions used by the serial MC Protocol codecs.

Choose one explicit profile in your application or configuration UI. The library intentionally does not infer it from `ReadTypeName`, CPU model text, or omitted configuration.

The supported device inventory is maintained in the shared [MC Protocol Serial supported registers](https://fa-yoshinobu.github.io/plc-comm-docs-site/plc-setup/mcprotocol/supported-registers/) page.

## Verified hardware available for validation

The maintainer owns the hardware listed below. Communication has been verified
on this hardware, and it is available for focused reproduction and validation
when a problem is reported.

"Verified" does not mean that every library feature has been tested on every
listed PLC or module. Exhaustively testing every combination would require a prohibitive
amount of work.

| PLC family | Serial hardware owned by the maintainer |
| --- | --- |
| MELSEC iQ-R | `RJ71C24-R2`, `RJ71C24-R4` |
| MELSEC iQ-F | `FX5U-32MR/DS`, `FX5UC-32MT/D` |
| MELSEC-L | `LJ71C24` |
| MELSEC-Q | `QJ71C24N` |

## Explicit selection is required

Always select one concrete `PlcProfile` value before sending real PLC requests.

- `PlcProfile::Unspecified` is a configuration error, not a fallback profile.
- No profile is inferred from CPU model text, response data, serial settings, or device strings.
- Short names and case variants are rejected by text parsing.
- Linux CLI wrappers require both `MCPROTOCOL_FRAME` and `MCPROTOCOL_PLC_PROFILE`; neither value is auto-filled.

Use `plc_profile_display_name(profile)` for UI labels. Store and parse the
canonical value from `plc_profile_name(profile)`, not the display text.

## Profiles

| Canonical profile | Display name | Hardware | API selector | Current internal grouping / status |
| --- | --- | --- | --- | --- |
| `melsec:iq-r` | MELSEC iQ-R | MELSEC iQ-R serial modules | `PlcProfile::MelsecIqR` | iQ-R command/device-layout branch. Manual-derived request-shape differences include iQ-R subcommands and wider device references. |
| `melsec:iq-l` | MELSEC iQ-L | MELSEC iQ-L serial modules | `PlcProfile::MelsecIqL` | Separate public profile using Q/L-compatible serial MC request shapes. Long timer/counter devices, `LZ`, `RD`, and `Un\HG` are rejected for serial MC. |
| `melsec:iq-f` | MELSEC iQ-F | MELSEC iQ-F / FX5 serial paths | `PlcProfile::MelsecIqF` | Separate public profile using the confirmed FX5 C4 binary support surface. Unsupported FX5 devices and routes are rejected locally. |
| `melsec:qcpu` | MELSEC-Q | MELSEC-Q serial modules | `PlcProfile::MelsecQ` | Public Q profile. Currently grouped with the Q/L command-device layout used by the existing Q/L encoder path. |
| `melsec:lcpu` | MELSEC-L | MELSEC-L serial modules | `PlcProfile::MelsecL` | Public L profile using the Q/L-compatible serial MC request shape. Keep it separate from `melsec:qcpu` for target-family evidence. |
| `melsec:qna` | MELSEC QnA | MELSEC QnA-compatible targets | `PlcProfile::MelsecQnA` | QnA command-family branch for operations with a shared wire shape. Direct extended file-register `NR/NW` and 1C module-buffer `TR/TW` are rejected for this physical profile. |
| `melsec:ana-anu` | MELSEC AnA/AnU | MELSEC AnA / AnU-compatible targets | `PlcProfile::MelsecAnAAnU` | AnA/AnU physical profile. It uses direct extended file-register `NR/NW`, accepts 1C module-buffer `TR/TW`, and shares QnA-family command selection only for operations without a profile-specific rule. |
| `melsec:a` | MELSEC-A | MELSEC-A-compatible targets | `PlcProfile::MelsecA` | A-series command-family branch. A-only paths such as ER/EW extended file-register commands require this profile. |

## SLMP name alignment

The common PLC-family names intentionally match the SLMP libraries. Profiles
that exist only in one protocol family are listed as not applicable.

| PLC family | Serial MC profile | SLMP profile |
| --- | --- | --- |
| MELSEC iQ-R | `melsec:iq-r` | `melsec:iq-r` |
| MELSEC iQ-L | `melsec:iq-l` | `melsec:iq-l` |
| MELSEC iQ-F / FX5 | `melsec:iq-f` | `melsec:iq-f` |
| MELSEC-Q CPU | `melsec:qcpu` | `melsec:qcpu` |
| MELSEC-L CPU | `melsec:lcpu` | `melsec:lcpu` |
| MELSEC QnU | Not applicable | `melsec:qnu` |
| MELSEC QnUDV | Not applicable | `melsec:qnudv` |
| MELSEC MX-F | Not applicable | `melsec:mx-f` |
| MELSEC MX-R | Not applicable | `melsec:mx-r` |
| MELSEC QnA | `melsec:qna` | Not applicable |
| MELSEC AnA / AnU | `melsec:ana-anu` | Not applicable |
| MELSEC-A | `melsec:a` | Not applicable |

## How to select

```cpp
auto protocol = mcprotocol::serial::highlevel::make_c4_ascii_format4_protocol(
    mcprotocol::serial::PlcProfile::MelsecQ,
    mcprotocol::serial::SumCheckMode::Disabled,
    mcprotocol::serial::RouteConfig {mcprotocol::serial::HostStationRoute {}});
```

For a custom C4 Binary session, use the tagged factory and pass every active selector:

```cpp
auto protocol = mcprotocol::serial::ProtocolConfig::c4_binary(
    mcprotocol::serial::PlcProfile::MelsecQ,
    mcprotocol::serial::SumCheckMode::Enabled,
    mcprotocol::serial::RouteConfig {mcprotocol::serial::HostStationRoute {}});
```

Use the display-name helper only for UI labels:

```cpp
const auto profile = mcprotocol::serial::PlcProfile::MelsecQ;
const char* saved_name = mcprotocol::serial::plc_profile_name(profile); // "melsec:qcpu"
const char* label = mcprotocol::serial::plc_profile_display_name(profile); // "MELSEC-Q"
```

`PlcProfile::Unspecified` and unknown enum values are configuration errors. Public protocol
factories require a profile argument and every encode/client boundary rejects a noncanonical value
before transport is opened or request bytes are produced.

## Profile-specific cautions

| Canonical profile | Caution |
| --- | --- |
| `melsec:iq-r` | Use for iQ-R serial-module routes. Some command behavior remains target/module dependent. |
| `melsec:iq-l` | Kept separate from iQ-R because iQ-L serial-module behavior is Q/L-subcommand based even when CPU-side SLMP looks iQ-R-like. Do not use the long timer/counter family, `LZ`, `RD`, or `Un\HG` with this profile. |
| `melsec:iq-f` | Use for FX5/iQ-F serial paths. `V`, `ZR`, `DX`, `DY`, long timer/retentive-timer families, `Un\HG`, `Jn\...`, monitor, host-buffer, and module-buffer helper routes are not part of this profile. |
| `melsec:qcpu` | Use for MELSEC-Q serial paths. The current implementation shares the Q/L request-shape branch but keeps Q as a separate public profile. |
| `melsec:lcpu` | Use for MELSEC-L serial paths. The current implementation shares the Q/L request-shape branch but keeps L as a separate public profile. |
| `melsec:qna` | Select only for a QnA physical target. Do not use it for direct `NR/NW` or 1C `TR/TW`; those calls are rejected before serial I/O. |
| `melsec:ana-anu` | Select for an AnA/AnU physical target. This is the required profile for direct `NR/NW` and is also accepted for 1C `TR/TW`. |
| `melsec:a` | Select for A-series-compatible targets; extended file-register ER/EW commands require this profile. |

The profile does not replace serial settings. Baud rate, parity, stop bits, frame type, sum-check
behavior, and station number still have to match the PLC serial module. Select sum-check explicitly
with `SumCheckMode::Enabled` or `SumCheckMode::Disabled`; a named frame preset does not infer it.
