# Gotchas

## No response from PLC

| Symptom | Root cause | Fix |
| --- | --- | --- |
| The request is sent, but no response arrives. | Baud rate, parity, and stop bits do not match the PLC serial module. All three must be correct at the same time. | Confirm the PLC serial module DIP switch or parameter settings, then set `PosixSerialConfig` or your MCU UART to the same values. |

## `PlcProfile::Unspecified`

| Symptom | Root cause | Fix |
| --- | --- | --- |
| Encoding or `open()` fails with a PLC profile error. | `ProtocolConfig::plc_profile` was left as `PlcProfile::Unspecified`. | Select an explicit profile such as `PlcProfile::MelsecQ` before opening or encoding. |

## Frame and PLC profile must be explicit

| Symptom | Root cause | Fix |
| --- | --- | --- |
| A Linux CLI wrapper exits before sending a request. | `MCPROTOCOL_FRAME` or `MCPROTOCOL_PLC_PROFILE` was not set. The wrapper does not infer either value. | Set both values explicitly, for example `MCPROTOCOL_FRAME=c4-binary` and `MCPROTOCOL_PLC_PROFILE=melsec:qcpu`. |
| A program appears to use the wrong command family. | The serial frame/profile was copied from another target or left at a sample value. | Choose the frame and `PlcProfile` for the actual PLC serial module; CPU model text, serial settings, and device strings are not used for inference. |

## Zeros from MCU sample

| Symptom | Root cause | Fix |
| --- | --- | --- |
| The MCU sample runs, but the values are always zero or do not match the PLC. | The sample UART pins are board defaults, not universal wiring rules. | Change the TX/RX pins to match your board wiring and check [WIRING_GUIDE.md](WIRING_GUIDE.md). |

## Frame type mismatch

| Symptom | Root cause | Fix |
| --- | --- | --- |
| The PLC returns framing errors or ignores the request. | `make_c4_binary_protocol()` selects 4C Binary. A PLC module configured for 4C ASCII, 3C, or another frame will not treat that as the same protocol. | Configure the PLC serial module for the same frame type and code mode, or choose a matching `ProtocolConfig` helper. |
| Format4 works but Format5 does not, or the reverse. | The serial module is configured for one MC protocol format at a time. This is a frame-mode mismatch, not remote-password behavior. | Match the PLC serial-module MC protocol format to the client helper, such as `make_c4_ascii_format4_protocol()` for Format4 or `make_c4_binary_protocol()` for Format5. |

## RS-485 multi-drop: wrong station

| Symptom | Root cause | Fix |
| --- | --- | --- |
| Only one station responds, or no RS-485 station responds. | `route.station_no` does not match the station number configured on the target serial module. | Set `protocol.route.kind = RouteKind::MultidropStation` and set `protocol.route.station_no` to the target station. For RS-232C point-to-point host-station use, keep station `0`. |

## `host_sync.hpp` symbol not found

| Symptom | Root cause | Fix |
| --- | --- | --- |
| `PosixSyncClient` or `PosixSerialConfig` is not found. | The host sync API is host-only and is not exposed on Arduino builds. It may also be unavailable if `MCPROTOCOL_SERIAL_ENABLE_HOST_API` is disabled or you included only narrow headers. | Build on a host with host support enabled and include `#include <mcprotocol/serial/host_sync.hpp>` explicitly. |

## PlatformIO UART pins wrong

| Symptom | Root cause | Fix |
| --- | --- | --- |
| The PlatformIO UART sample builds but does not communicate. | The sample pin defaults are starting values: RP2040 TX `0` / RX `1`, ESP32-C3 TX `7` / RX `6`, and Mega 2560 TX1 `18` / RX1 `19`. | Match those definitions to your actual board, level shifter, and cable wiring. |

## Typed suffix does not parse

| Symptom | Root cause | Fix |
| --- | --- | --- |
| `D100:D`, `D100:F`, or `D100.0` fails in the high-level string parser. | The current parser accepts plain device strings only. | Use `D100`, `M100`, `X10`, and typed C++ fields such as `double_word` where needed. |
