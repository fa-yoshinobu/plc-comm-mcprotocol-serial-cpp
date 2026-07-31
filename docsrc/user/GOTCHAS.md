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
| The MCU sample runs, but the values are always zero or do not match the PLC. | The sample UART pins are board defaults, not universal wiring rules. | Change the TX/RX pins to match your board wiring and check the shared [MC Protocol Serial setup guide](https://fa-yoshinobu.github.io/plc-comm-docs-site/plc-setup/mcprotocol/serial/). |

## Frame type mismatch

| Symptom | Root cause | Fix |
| --- | --- | --- |
| The PLC returns framing errors or ignores the request. | `make_c4_binary_protocol()` selects 4C Binary. A PLC module configured for 4C ASCII, 3C, or another frame will not treat that as the same protocol. | Configure the PLC serial module for the same frame type and code mode, or choose a matching `ProtocolConfig` helper. |
| Format4 works but Format5 does not, or the reverse. | The serial module is configured for one MC protocol format at a time. This is a frame-mode mismatch, not remote-password behavior. | Match the PLC serial-module MC protocol format to the client helper, such as `make_c4_ascii_format4_protocol()` for Format4 or `make_c4_binary_protocol()` for Format5. |

## RS-485 multi-drop: wrong station

| Symptom | Root cause | Fix |
| --- | --- | --- |
| Only one station responds, or no RS-485 station responds. | The selected frame-specific multidrop station/network/PC/destination-module target or connection topology does not match the serial-network configuration. | Construct the appropriate frame-specific route. For normal/1:n use a `*StandardMultidropRoute`; for m:n use a `*MnMultidropRoute` and the assigned mandatory `SelfStationNo` (0..31). Do not reuse a C24-side station number or infer station-count rules. For 3C/4C supply a `C34PcTarget`; 4C also requires `C4DestinationModule`; 1E requires `E1PcTarget`. For an RS-232C point-to-point connected station, select `RouteConfig {HostStationRoute {}}`. |

## 1E timer changes when communication timeout changes

| Symptom | Root cause | Fix |
| --- | --- | --- |
| A 1E request needs a different PLC processing timer than the host response deadline. | These are independent limits: response timeout controls host waiting, while the 1E monitoring timer is encoded into the PLC request. | Leave the common response timeout at 3000 ms or set it explicitly, and configure `E1MonitoringTimer::milliseconds(...)` separately in exact 250 ms units. No rounding or saturation is performed. |

## Response starts but times out before completion

| Symptom | Root cause | Fix |
| --- | --- | --- |
| Response bytes keep arriving slowly, but the request still times out. | One absolute deadline covers first TX, drain, every RX chunk, correlation, and decode. Partial progress never restarts it. | Increase the one transaction timeout if the complete operation legitimately needs longer. After any timeout, close/reopen the serial generation and reconfigure; never reuse partial bytes. |

## A state-changing result is unknown

| Symptom | Root cause | Fix |
| --- | --- | --- |
| A write, remote-control, password, user-frame, signal, mode-switch, or initialization command returns `StatusCode::OperationOutcomeUnknown`. | Transmission may have begun, but the PLC result could not be confirmed. The requested state change may already have occurred. | Do not resend automatically. Inspect the affected PLC state and reset/reopen the transport when required before deciding the next operation. |
| Remote RUN returns `StatusCode::OperationOutcomeUnknown`. | Transmission started, but transport failure, timeout, cancellation, or an invalid response prevented confirmation. The PLC may already have applied the requested RUN and clear policies. | Do not resend automatically. Inspect the PLC state, reset/reopen the transport when required, and decide the next action explicitly. |
| `remote_run()` does not compile, or CLI `remote-run` exits with usage. | Conflict and clear policies are mandatory. | Pass `RemoteOperationMode::{DoNotExecuteForcibly, ExecuteForcibly}` and one `RemoteRunClearMode`, or CLI `no-force|force` plus `no-clear|outside-latch|all-clear`. |
| Remote PAUSE returns `StatusCode::OperationOutcomeUnknown`. | PAUSE transmission started, but its result was not confirmed. The library does not retry or escalate to forced execution. | Inspect the PLC state before deciding the next operation. Reopen/reset the transport when required. |
| `remote_pause()` does not compile, or CLI `remote-pause` exits with usage. | The conflict policy is mandatory and only the exact CLI names `no-force` and `force` are accepted. | Pass one `RemoteOperationMode` or one exact CLI policy. Do not use numeric or compatibility aliases. |

## `host_sync.hpp` symbol not found

| Symptom | Root cause | Fix |
| --- | --- | --- |
| `PosixSyncClient` or `PosixSerialConfig` is not found in a PlatformIO project. | The PlatformIO package intentionally compiles only the transport-agnostic core. Defining `MCPROTOCOL_SERIAL_ENABLE_HOST_API=1` would expose declarations whose implementations are not in that package. | Vendor the source repository in a CMake host project, link the host-enabled `mcprotocol_serial` target, and include `#include <mcprotocol/serial/host_sync.hpp>` explicitly. |

## Constrained standard library

| Symptom | Root cause | Fix |
| --- | --- | --- |
| A constrained MCU toolchain has no `<array>`, `<algorithm>`, or related C++ standard header. | The library auto-detects missing headers and uses fallbacks from `mcprotocol/serial/compat/`. The compatibility files no longer use standard header names in the public include root. | If the toolchain's header probe is inaccurate, add `-DMCPROTOCOL_SERIAL_USE_BUNDLED_STDLIB_COMPAT=1` to that environment. Do not copy compatibility files into the include root under names such as `array` or `algorithm`. |

## PlatformIO UART pins wrong

| Symptom | Root cause | Fix |
| --- | --- | --- |
| The PlatformIO UART sample builds but does not communicate. | The sample pin defaults are starting values: RP2040 TX `0` / RX `1`, ESP32-C3 TX `7` / RX `6`, and Mega 2560 TX1 `18` / RX1 `19`. | Match those definitions to your actual board, level shifter, and cable wiring. |

## Target-dependent native commands

| Symptom | Root cause | Fix |
| --- | --- | --- |
| A qualified `Un\G` / `Un\HG` command, link-direct command, or native helper fails on one target but not another. | Support depends on the selected PLC profile, serial module, mounted route, and command route. The `0601/1601` helper route is not a fallback for profiles that require native-qualified access. | Use the route required by the selected profile and check the shared [MC Protocol Serial supported registers](https://fa-yoshinobu.github.io/plc-comm-docs-site/plc-setup/mcprotocol/supported-registers/) page before changing code. |

## Typed suffix does not parse

| Symptom | Root cause | Fix |
| --- | --- | --- |
| `D100:D`, `D100:F`, or `D100.0` fails in the high-level string parser. | The parser accepts plain device strings; width is selected by the API type rather than a suffix. | Use `D100`, `M100`, or `X10` with the explicit Word or DWord API. For CLI sparse reads, use `word:D100` or `dword:D100`. |
| A random-write item does not compile without a value, or CLI rejects `D100`/`M100`. | Random-write values are mandatory so an omitted value cannot silently become zero or OFF. | Construct the item/spec with both device and value, or use CLI `DEVICE=VALUE`. Explicit `=0` is valid. |
| A random write returns `OperationOutcomeUnknown`. | Transmission started, but the PLC response was not confirmed. The value may already have changed. | Inspect the PLC/device state and reconfigure/reset the transport as required. Do not retry automatically. |
