# PLC profiles

Each profile selects the command family and device-layout assumptions used by the serial MC Protocol codecs.

Choose one explicit profile in your application or configuration UI. The library intentionally does not infer it from `ReadTypeName`, CPU model text, or omitted configuration.

## Profiles

| Profile string | Hardware | API selector | Notes |
| --- | --- | --- | --- |
| `melsec:iq-r` | MELSEC iQ-R serial modules | `PlcProfile::MelsecIqR` | iQ-R command and device-layout profile. |
| `melsec:iq-l` | MELSEC iQ-L serial modules | `PlcProfile::MelsecIqL` | Separate public profile for iQ-L targets. |
| `melsec:q-l` | MELSEC-Q / MELSEC-L serial modules | `PlcProfile::MelsecQL` | Used by the host quickstart examples. |
| `melsec:qna` | MELSEC QnA-compatible targets | `PlcProfile::MelsecQnA` | Enables QnA command-family selection. |
| `melsec:ana-anu` | MELSEC AnA / AnU-compatible targets | `PlcProfile::MelsecAnAAnU` | Enables AnA/AnU command-family selection. |
| `melsec:a` | MELSEC-A-compatible targets | `PlcProfile::MelsecA` | Enables A-series command-family selection. |

## How to select

```cpp
auto protocol = mcprotocol::serial::highlevel::make_c4_binary_protocol(
    mcprotocol::serial::PlcProfile::MelsecQL);
```

Or assign the field directly when you build a custom `ProtocolConfig`:

```cpp
mcprotocol::serial::ProtocolConfig protocol {};
protocol.plc_profile = mcprotocol::serial::PlcProfile::MelsecQL;
```

`PlcProfile::Unspecified` is a configuration error. It is useful only as the default internal value before your application selects a real profile.

## Profile-specific cautions

| Profile string | Caution |
| --- | --- |
| `melsec:iq-r` | Use for iQ-R serial-module routes. Some native command behavior remains target-dependent. |
| `melsec:iq-l` | Kept separate from iQ-R so iQ-L behavior can diverge later without changing saved configuration names. |
| `melsec:q-l` | Use for MELSEC-Q and MELSEC-L serial paths that use the Q/L command-family assumptions. |
| `melsec:qna` | Select only when the target should use QnA-style command selection. |
| `melsec:ana-anu` | Select only when the target should use AnA/AnU command-family selection. |
| `melsec:a` | Select for A-series-compatible targets; extended file-register ER/EW commands require this profile. |

The profile does not replace serial settings. Baud rate, parity, stop bits, frame type, sum-check behavior, and station number still have to match the PLC serial module.
