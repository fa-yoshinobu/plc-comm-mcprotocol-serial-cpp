# Library Entrypoints

This page is for choosing and using the public library entrypoints.

## Design Summary

The library design is intentionally simple:

- no exceptions
- no RTTI
- no dynamic allocation in the library
- caller-owned buffers via `std::span`
- transport-agnostic client state machine

## Entry Paths

### 1. High-level helpers

Use [`mcprotocol/serial/high_level.hpp`](../../include/mcprotocol/serial/high_level.hpp) if you want:

- baseline `ProtocolConfig` presets
- string-address parsing such as `D100`, `M100`, `X10`
- request builders for common contiguous, sparse random, and monitor operations

Smallest helper-based setup:

```cpp
#include <array>
#include <mcprotocol_serial.hpp>

mcprotocol::serial::ProtocolConfig config =
    mcprotocol::serial::highlevel::make_c4_binary_protocol();
config.route.station_no = 0;

mcprotocol::serial::BatchReadWordsRequest request {};
mcprotocol::serial::Status status =
    mcprotocol::serial::highlevel::make_batch_read_words_request("D100", 2, request);
```

### 2. Synchronous host-side facade

Use [`mcprotocol/serial/host_sync.hpp`](../../include/mcprotocol/serial/host_sync.hpp) if you want a
small blocking host-side bring-up path on Windows or POSIX.

It wraps `PosixSerialPort` and `MelsecSerialClient` into a single helper for:

- `cpu-model`
- `remote_run`
- `remote_stop`
- `remote_pause`
- `remote_latch_clear`
- `unlock_remote_password`
- `lock_remote_password`
- `clear_error_information`
- `remote_reset`
- contiguous `read_words`
- contiguous `read_bits`
- contiguous `write_words`
- contiguous `write_bits`
- sparse `random_read`
- sparse `random_write_words`
- sparse `random_write_bits`
- `register_monitor` and `read_monitor`

Smallest synchronous host-side setup:

```cpp
#include <mcprotocol_serial.hpp>

mcprotocol::serial::PosixSyncClient plc;
mcprotocol::serial::PosixSerialConfig serial {
#if defined(_WIN32)
    .device_path = "COM3",
#else
    .device_path = "/dev/ttyUSB0",
#endif
    .baud_rate = 19200,
    .data_bits = 8,
    .stop_bits = 2,
    .parity = 'E',
};

auto protocol = mcprotocol::serial::highlevel::make_c4_binary_protocol();
mcprotocol::serial::Status status = plc.open(serial, protocol);

std::array<std::uint16_t, 2> words {};
status = plc.read_words("D100", words);

std::uint32_t d100 = 0;
status = plc.random_read("D100", d100);
```

On Windows, pass `COM3`-style names. On POSIX hosts, pass `/dev/ttyUSB0`-style device paths.

### 3. Low-level async client

Use [`MelsecSerialClient`](../../include/mcprotocol/serial/client.hpp) directly if you want the
full async state machine and plan to integrate your own UART layer or scheduler.

Normal workflow:

1. Configure `MelsecSerialClient`
2. Start an async request
3. Send `pending_tx_frame()` with your UART layer
4. Call `notify_tx_complete()`
5. Feed received bytes through `on_rx_bytes()`
6. Call `poll()` for timeout handling

## Example Index

- [Examples Index](../../examples/README.md)
- [host_sync_quickstart.cpp](../../examples/host_sync_quickstart.cpp)
- [platformio_rpipico_arduino_uart.cpp](../../examples/platformio_rpipico_arduino_uart/platformio_rpipico_arduino_uart.cpp)
- [platformio_esp32c3_arduino_uart.cpp](../../examples/platformio_esp32c3_arduino_uart/platformio_esp32c3_arduino_uart.cpp)
- [platformio_arduino_mega2560_uart.cpp](../../examples/platformio_arduino_mega2560_uart/platformio_arduino_mega2560_uart.cpp)
- [mcu_async_batch_read.cpp](../../examples/mcu_async_batch_read.cpp)
- [platformio_rpipico_arduino_async.cpp](../../examples/platformio_rpipico_arduino_async/platformio_rpipico_arduino_async.cpp)
- [platformio_esp32c3_arduino_async.cpp](../../examples/platformio_esp32c3_arduino_async/platformio_esp32c3_arduino_async.cpp)
