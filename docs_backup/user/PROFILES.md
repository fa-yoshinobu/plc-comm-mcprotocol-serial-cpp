# PLC Profiles

Choose one canonical PLC profile explicitly. The profile controls the command
family and device-layout assumptions used by the serial MC Protocol helpers.

Do not rely on omitted profile values or short aliases for live commands.

| Canonical profile | Human label | API selector | Notes |
| --- | --- | --- | --- |
| `melsec:iq-r` | MELSEC iQ-R serial modules | `PlcProfile::MelsecIqR` | Validated for RJ71C24-R2 class iQ-R routes. |
| `melsec:iq-l` | MELSEC iQ-L serial modules | `PlcProfile::MelsecIqL` | iQ-L profile selector for serial MC Protocol routing. |
| `melsec:q-l` | MELSEC-Q / MELSEC-L serial modules | `PlcProfile::MelsecQL` | Q/L profile for 3C/4C serial paths. |
| `melsec:qna` | MELSEC QnA-compatible targets | `PlcProfile::MelsecQnA` | Enables QnA-style command-family selection. |
| `melsec:ana-anu` | MELSEC AnA / AnU-compatible targets | `PlcProfile::MelsecAnAAnU` | Enables AnA/AnU command-family selection. |
| `melsec:a` | MELSEC-A-compatible targets | `PlcProfile::MelsecA` | Enables A-series command-family selection. |

## How to select

For live CLI commands, pass `--plc-profile` every time. The CLI intentionally
does not infer the profile from the serial module, CPU model response, or frame
format.

```powershell
.\build\manual\mcprotocol_cli.exe `
  --device COM3 `
  --baud 28800 `
  --data-bits 8 `
  --parity E `
  --stop-bits 2 `
  --frame c4-binary `
  --plc-profile melsec:iq-r `
  --station 0 `
  --sum-check on `
  cpu-model
```

In C++ code, set `ProtocolConfig::plc_profile` explicitly before encoding or
executing a request. `PlcProfile::Unspecified` is a configuration error for
live communication.

## Profile-specific cautions

| Canonical profile | Caution |
| --- | --- |
| `melsec:iq-r` | Use for iQ-R serial-module routes such as the validated RJ71C24-R2 setups. Remote password length and some native command behavior remain target-dependent. |
| `melsec:iq-l` | Kept separate from iQ-R so iQ-L serial behavior can diverge later without changing public configuration names. |
| `melsec:q-l` | Q/L serial paths use the Q/L command-family assumptions for 3C/4C frames. |
| `melsec:qna` | Select only for QnA-compatible targets where QnA-style command selection is intended. |
| `melsec:ana-anu` | Select only for AnA / AnU-compatible targets. |
| `melsec:a` | Select only for A-series-compatible targets. |

## Related pages

- [Library entrypoints](LIBRARY_ENTRYPOINTS.md)
- [MCU quickstart](MCU_QUICKSTART.md)
- [Hardware validation matrix](../validation/reports/HARDWARE_VALIDATION.md)
