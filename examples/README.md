# Examples

Choose an example for your board and purpose. Match the PLC profile, protocol,
serial settings and wiring before connecting to a PLC.
Writing examples change the listed devices; reserve those addresses for testing.

## Example index

| Example | Purpose | PLC access |
| --- | --- | --- |
| [ESP32-S3 UART adapter](platformio_esp32s3_arduino_uart_adapter/README.md) | Minimal STAMPLC / FX5U D100 read | Read only |
| [ESP32 / STAMPLC usage](esp32_uart_usage/README.md) | WORD, bit, DWORD, float, random, multi-block and asynchronous LCD examples | Six examples write on USB w; LCD example is read only |
| [ESP32-C3 UART](platformio_esp32c3_arduino_uart/README.md) | Asynchronous D100-D103 polling through the UART adapter | Read only; stops on error |
| [ESP32-C3 recovery](platformio_esp32c3_arduino_async_polling_reconnect/README.md) | Polling with explicit recovery after uncertain responses | Read only; manual r after required line recovery |
| [Pico UART](platformio_rpipico_arduino_uart/README.md) | Direct core/UART integration on RP2040 | Read only; stops on error |
| [Host quickstart](host_sync_quickstart.cpp) | CPU model, batch and random reads on Windows/POSIX | Read only |
| [Host polling](host_sync_polling_reconnect.cpp) | Polling with retry for open failures and complete PLC error responses | Read only; stops on uncertain read errors |
| [Core lifecycle](mcu_async_batch_read.cpp) | Low-level asynchronous state machine | Simulated response; no physical PLC |
| [Pico simulation](platformio_rpipico_arduino_async/README.md) | Core lifecycle on Arduino | Simulated response |
| [ESP32-C3 simulation](platformio_esp32c3_arduino_async/README.md) | Core lifecycle on Arduino | Simulated response |
| [Read-only CLI bring-up](linux_cli/safe_bringup_readonly.sh) | Explicit serial/profile configuration | CPU model, loopback and word read |
| [Cyclic CLI reads](linux_cli/cyclic_read_words.sh) | Timed repeated word reads | Read only |
| [Device soak test](linux_cli/supported_device_rw_soak.sh) | Read/write/read/restore test | Writes; mismatch or restore failure exits with failure |
| [FX5U soak preset](linux_cli/fx5u_supported_device_rw_soak.sh) | Device list preset for the soak test | Writes |

The soak tests attempt restoration during normal execution, but restoration is not
guaranteed after interruption or communication failure. They are test tools, not
production control programs.

## Selecting a project

The repository root PlatformIO project contains the board-specific examples.
Select its matching environment in PlatformIO.
The seven usage examples have their own project in `esp32_uart_usage`.
Host examples are CMake targets. The simulated examples do not test PLC wiring.

## Real UART defaults

| Target | UART / pins | Serial | Protocol |
| --- | --- | --- | --- |
| STAMPLC / ESP32-S3 | UART1: RX39, TX0, DIR46 | 19200 / 8E1 | MelsecIqF, C4 Binary Format5, sum check on, station0 |
| ESP32-C3 | UART1: RX6, TX7; external direction | 19200 / 8E1 | MelsecQ, C4 ASCII Format4, sum check off, station0 |
| Raspberry Pi Pico | Serial1 / UART0: TX0, RX1 | 19200 / 8E1 | MelsecQ, C4 ASCII Format4, sum check off, station0 |

UART adapters exclusively own their UART: do not initialize Serial1 or Modbus on
the same port. External direction requires an appropriate transceiver.
The Pico hardware completion check is specific to UART0 on GPIO0/1.

## Recovery

A timeout or incomplete reply can leave a delayed PLC response in flight.
Reopening a UART, resetting the MCU or waiting an arbitrary interval does not
prove that the response has been excluded.
Correct the cause and complete the PLC/interface-specific recovery procedure
before restarting a stopped example.

The ESP32-C3 recovery example accepts r only as acknowledgment of that external
procedure. Complete PLC-error responses can be retried as reads after the poll
interval. Do not apply that policy to writes whose execution result is unknown.

Host polling accepts the serial device, PLC profile, head device, point count,
baud, format4/format5 and sum-check selection as arguments; use its help display
for the argument order. It retries open failures and complete PLC error responses
with backoff, but exits on uncertain read errors instead of reopening blindly.

## Verification and scope

Host regression checks cover the example control flow and simulated UART events.
A successful build or simulated test does not establish physical PLC compatibility.
See each example's documentation for hardware verification status.
Arduino Mega 2560 and other AVR/8-bit targets are not supported.

See [Gotchas](../docsrc/user/GOTCHAS.md) for profile, framing and device constraints.
