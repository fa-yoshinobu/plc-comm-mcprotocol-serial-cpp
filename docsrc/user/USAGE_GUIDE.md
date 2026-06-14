# Usage guide

The library exposes three practical entry paths. Pick one based on how much transport control your application needs.

## Entry paths

| Entry path | Header | Use it when |
| --- | --- | --- |
| High-level helpers | `mcprotocol/serial/high_level.hpp` | You want protocol presets and string-address request builders. |
| Host sync facade | `mcprotocol/serial/host_sync.hpp` | You are writing a blocking Linux or Windows bring-up tool. |
| Low-level async client | `mcprotocol/serial/client.hpp` | You are integrating your own UART, DMA, interrupt, or scheduler layer. |

## Entry path 1: high-level helpers

`make_c4_binary_protocol(PlcProfile::...)` creates a `ProtocolConfig` preset. Request builders such as `make_batch_read_words_request("D100", count, request)` convert plain device strings into typed request structs.

```cpp
#include <cstdint>
#include <cstdio>

#include "mcprotocol_serial.hpp"

int main() {
  using mcprotocol::serial::BatchReadWordsRequest;
  using mcprotocol::serial::PlcProfile;
  using mcprotocol::serial::ProtocolConfig;
  using mcprotocol::serial::Status;
  using mcprotocol::serial::highlevel::make_batch_read_words_request;
  using mcprotocol::serial::highlevel::make_c4_binary_protocol;

  ProtocolConfig protocol = make_c4_binary_protocol(PlcProfile::MelsecQL);
  protocol.route.station_no = 0;

  BatchReadWordsRequest request {};
  Status status = make_batch_read_words_request("D100", 2, request);
  if (!status.ok()) {
    std::fprintf(stderr, "request build failed: %s\n", status.message);
    return 1;
  }

  std::printf("head=%u points=%u\n", request.head_device.number, request.points);
  return 0;
}
```

## Entry path 2: synchronous host facade

`PosixSyncClient` opens a host serial port, configures the protocol client, transmits one request, waits for completion, and returns a `Status`.

### Read words

```cpp
#include <array>
#include <cstdint>
#include <cstdio>

#include "mcprotocol_serial.hpp"

int main() {
  using mcprotocol::serial::PlcProfile;
  using mcprotocol::serial::PosixSerialConfig;
  using mcprotocol::serial::PosixSyncClient;
  using mcprotocol::serial::Status;
  using mcprotocol::serial::highlevel::make_c4_binary_protocol;

  PosixSerialConfig serial {};
  serial.device_path = "/dev/ttyUSB0";
  serial.baud_rate = 19200;
  serial.data_bits = 8;
  serial.stop_bits = 2;
  serial.parity = 'E';
  serial.rts_cts = false;

  auto protocol = make_c4_binary_protocol(PlcProfile::MelsecQL);
  PosixSyncClient plc;
  Status status = plc.open(serial, protocol);
  if (!status.ok()) {
    return 1;
  }

  std::array<std::uint16_t, 2> words {};
  status = plc.read_words("D100", words);
  if (!status.ok()) {
    return 1;
  }

  std::printf("D100=0x%04X D101=0x%04X\n", words[0], words[1]);
  return 0;
}
```

### Write words

```cpp
#include <array>
#include <cstdint>

#include "mcprotocol_serial.hpp"

int main() {
  using mcprotocol::serial::PlcProfile;
  using mcprotocol::serial::PosixSerialConfig;
  using mcprotocol::serial::PosixSyncClient;
  using mcprotocol::serial::highlevel::make_c4_binary_protocol;

  PosixSerialConfig serial {};
  serial.device_path = "/dev/ttyUSB0";
  serial.baud_rate = 19200;
  serial.data_bits = 8;
  serial.stop_bits = 2;
  serial.parity = 'E';
  PosixSyncClient plc;
  auto protocol = make_c4_binary_protocol(PlcProfile::MelsecQL);
  if (!plc.open(serial, protocol).ok()) {
    return 1;
  }

  const std::array<std::uint16_t, 2> words {0x1234, 0x5678};
  return plc.write_words("D100", words).ok() ? 0 : 1;
}
```

### Random read and random write

```cpp
#include <array>
#include <cstdint>

#include "mcprotocol_serial.hpp"

int main() {
  using mcprotocol::serial::PlcProfile;
  using mcprotocol::serial::PosixSerialConfig;
  using mcprotocol::serial::PosixSyncClient;
  using mcprotocol::serial::highlevel::RandomWriteWordSpec;
  using mcprotocol::serial::highlevel::make_c4_binary_protocol;

  PosixSerialConfig serial {};
  serial.device_path = "/dev/ttyUSB0";
  serial.baud_rate = 19200;
  serial.data_bits = 8;
  serial.stop_bits = 2;
  serial.parity = 'E';
  PosixSyncClient plc;
  auto protocol = make_c4_binary_protocol(PlcProfile::MelsecQL);
  if (!plc.open(serial, protocol).ok()) {
    return 1;
  }

  std::uint32_t d100 = 0;
  if (!plc.random_read("D100", d100).ok()) {
    return 1;
  }

  const std::array<RandomWriteWordSpec, 1> writes {{{.device = "D101", .value = d100, .double_word = false}}};
  return plc.random_write_words(writes).ok() ? 0 : 1;
}
```

### Remote control and CPU model

```cpp
#include <cstdio>

#include "mcprotocol_serial.hpp"

int main() {
  using mcprotocol::serial::CpuModelInfo;
  using mcprotocol::serial::PlcProfile;
  using mcprotocol::serial::PosixSerialConfig;
  using mcprotocol::serial::PosixSyncClient;
  using mcprotocol::serial::RemoteOperationMode;
  using mcprotocol::serial::RemoteRunClearMode;
  using mcprotocol::serial::highlevel::make_c4_binary_protocol;

  PosixSerialConfig serial {};
  serial.device_path = "/dev/ttyUSB0";
  serial.baud_rate = 19200;
  serial.data_bits = 8;
  serial.stop_bits = 2;
  serial.parity = 'E';
  PosixSyncClient plc;
  auto protocol = make_c4_binary_protocol(PlcProfile::MelsecQL);
  if (!plc.open(serial, protocol).ok()) {
    return 1;
  }

  CpuModelInfo info {};
  if (!plc.read_cpu_model(info).ok()) {
    return 1;
  }

  std::printf("model=%s code=0x%04X\n", info.model_name.data(), info.model_code);
  if (!plc.remote_stop().ok()) {
    return 1;
  }
  return plc.remote_run(RemoteOperationMode::DoNotExecuteForcibly, RemoteRunClearMode::DoNotClear).ok() ? 0 : 1;
}
```

## Entry path 3: low-level async client

`MelsecSerialClient` owns the protocol state machine but not the UART. Your code configures the client, starts an async request, sends `pending_tx_frame()`, calls `notify_tx_complete()`, feeds response bytes with `on_rx_bytes()`, and calls `poll()` for timeout handling.

This is the path used in the PlatformIO examples.

```cpp
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "mcprotocol_serial.hpp"
#include "mcprotocol/serial/span_compat.hpp"

namespace {

struct Completion {
  bool done = false;
  mcprotocol::serial::Status status {};
};

void on_complete(void* user, mcprotocol::serial::Status status) {
  auto* completion = static_cast<Completion*>(user);
  completion->done = true;
  completion->status = status;
}

}  // namespace

int main() {
  using mcprotocol::serial::BatchReadWordsRequest;
  using mcprotocol::serial::DeviceAddress;
  using mcprotocol::serial::DeviceCode;
  using mcprotocol::serial::FrameCodec;
  using mcprotocol::serial::MelsecSerialClient;
  using mcprotocol::serial::PlcProfile;
  using mcprotocol::serial::Status;
  using mcprotocol::serial::highlevel::make_c4_ascii_format4_protocol;

  MelsecSerialClient client;
  const auto protocol = make_c4_ascii_format4_protocol(PlcProfile::MelsecQL);
  Status status = client.configure(protocol);
  if (!status.ok()) {
    return 1;
  }

  std::array<std::uint16_t, 2> words {};
  Completion completion {};
  status = client.async_batch_read_words(
      0,
      BatchReadWordsRequest {
          .head_device = DeviceAddress {.code = DeviceCode::D, .number = 100},
          .points = static_cast<std::uint16_t>(words.size()),
      },
      std::span<std::uint16_t>(words.data(), words.size()),
      on_complete,
      &completion);
  if (!status.ok()) {
    return 1;
  }

  const std::span<const std::byte> frame = client.pending_tx_frame();
  // Send `frame` through your UART here.
  (void)frame;
  status = client.notify_tx_complete(1);
  if (!status.ok()) {
    return 1;
  }

  const std::array<std::uint8_t, 8> response_data {'1', '2', '3', '4', '5', '6', '7', '8'};
  std::array<std::uint8_t, mcprotocol::serial::kMaxResponseFrameBytes> response_frame {};
  std::size_t response_frame_size = 0;
  status = FrameCodec::encode_success_response(
      protocol,
      std::span<const std::uint8_t>(response_data.data(), response_data.size()),
      response_frame,
      response_frame_size);
  if (!status.ok()) {
    return 1;
  }

  std::array<std::byte, mcprotocol::serial::kMaxResponseFrameBytes> rx_frame {};
  std::memcpy(rx_frame.data(), response_frame.data(), response_frame_size);
  client.on_rx_bytes(2, std::span<const std::byte>(rx_frame.data(), response_frame_size));
  client.poll(2);

  if (!completion.done || !completion.status.ok()) {
    return 1;
  }

  std::printf("D100=0x%04X D101=0x%04X\n", words[0], words[1]);
  return 0;
}
```

See [examples/mcu_async_batch_read.cpp](../../examples/mcu_async_batch_read.cpp) for a complete simulated async example.

## Address format

| Address form | Example | Status |
| --- | --- | --- |
| Plain decimal word device | `D100` | Supported. |
| Plain decimal bit device | `M100` | Supported. |
| Plain hexadecimal bit device | `X10` | Supported. |
| Plain hexadecimal word device | `W100` | Supported. |
| Typed suffix | `D100:D`, `D100:F` | Not supported by the current parser. |
| Bit-in-word suffix | `D100.0`, `D100.F` | Not supported by the current parser. |
| Link-direct string | `J1\D100` | Parsed by link-direct helpers, not by `parse_device_address()`. |

## Serial config reference

| Field | Type | Example | Notes |
| --- | --- | --- | --- |
| `device_path` | `std::string_view` | `/dev/ttyUSB0`, `COM3` | Host serial device path. |
| `baud_rate` | `std::uint32_t` | `19200` | Must match the PLC serial module. |
| `data_bits` | `std::uint8_t` | `8` | Host quickstart uses 8 data bits. |
| `stop_bits` | `std::uint8_t` | `2` | Host quickstart uses 2 stop bits. |
| `parity` | `char` | `'E'` | Use `'N'`, `'E'`, or `'O'` as supported by the host backend. |
| `rts_cts` | `bool` | `false` | Hardware flow-control flag for host serial ports. |
