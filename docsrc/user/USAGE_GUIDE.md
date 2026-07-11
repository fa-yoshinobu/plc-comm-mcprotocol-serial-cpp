# Usage guide

The library exposes three practical entry paths. Pick one based on how much transport control your application needs.

## Design summary

The public API is designed for host tools and MCU firmware:

- no exceptions
- no RTTI
- no dynamic allocation in the library
- caller-owned buffers via `std::span`
- transport-agnostic client state machine

## Entry paths

| Entry path | Header | Use it when |
| --- | --- | --- |
| High-level helpers | `mcprotocol/serial/high_level.hpp` | You want protocol presets and string-address request builders. |
| Host sync facade | `mcprotocol/serial/host_sync.hpp` | You are writing a blocking Linux or Windows bring-up tool. |
| Low-level async client | `mcprotocol/serial/client.hpp` | You are integrating your own UART, DMA, interrupt, or scheduler layer. |

## Entry path 1: high-level helpers

`make_c4_ascii_format4_protocol(PlcProfile::..., SumCheckMode::..., RouteConfig {...})` creates a
`ProtocolConfig` preset with explicit checksum and route policies. Request builders such as
`make_batch_read_words_request("D100", count, request)` convert plain device strings into typed
request structs.

`ProtocolConfig {}` is intentionally not a usable connection preset: frame family, code mode,
ASCII format (for ASCII), PLC profile, sum-check mode, and route must be selected explicitly. Named presets
make the frame and code-mode selection visible in the function name, while still requiring the PLC
profile, `SumCheckMode::Enabled` or `SumCheckMode::Disabled`, and a typed route argument. The
library never switches frame, code mode, format, profile, sum-check policy, or route after an error
or timeout.

### Route selection

Use `RouteConfig {HostStationRoute {}}` for the connected host-station route. This type has no
station, network, PC, destination-module, or self-station inputs; the protocol-defined connected
station header is fixed internally. For multidrop, select the frame-specific type:
`C1MultidropRoute(station)`, `C2MultidropRoute(station)`,
`C3MultidropRoute(station, network, pc_target)`,
`C4MultidropRoute(station, network, pc_target)`, or `E1Route(pc_target)`. Station zero and network
zero remain valid only when explicitly passed. A 3C/4C PC target is constructed with
`C34PcTarget::number(0x01U..0x78U)` or one of `control_system()`, `standby_system()`,
`special_fe()`, and `connected_station()`. A 1E target uses
`E1PcTarget::number(0x01U..0x40U)` or `connected_station()`. The special wire values cannot be
passed through `number()`, which prevents an ordinary number from silently acquiring special
meaning. Raw numeric CLI values `0x7D`, `0x7E`, `0xFE`, and `0xFF` are accepted only at that
external boundary and normalized to the corresponding canonical selector before validation.

`RouteConfig {}` is invalid. The CLI likewise requires `--route host` or `--route multidrop`;
3C/4C additionally require `--network` and `--pc-target`, while 1E multidrop requires
`--pc-target`. Use `--pc-target connected` when explicit `0xFF` is correct. A station or PC value
never selects or changes a route implicitly.

Every response route header field that exists in the selected frame is compared with the
configured route. A complete 3C/4C response from a different station, network, or PC target is
discarded while the client continues waiting for the matching response. The 1E response message
does not echo its request PC target, so the PC target is enforced on request construction and
encoding rather than inferred from response bytes. Malformed ASCII route hexadecimal is reported
as a parse error. Timeout, NAK, malformed input, or mismatch never causes automatic route discovery
or fallback.

### Format2 request identity

Do not configure a fixed Format2 block number. `MelsecSerialClient` assigns a new value for each
wire request, advances through `00` to `FF`, wraps to `00` only after the prior request has finished,
and accepts only the matching response. A late response from a timed-out or cancelled request is
discarded instead of being returned as the next request's result.

`FrameCodecContext::format2(number)` is available only for raw frame construction, negative tests,
and protocol investigation where the caller intentionally owns one wire identity. Passing a
Format2 context to another format, or using a Format2 raw codec without one, is an error. The CLI
does not expose a normal `--block-no` connection option.

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
  using mcprotocol::serial::highlevel::make_c4_ascii_format4_protocol;

  ProtocolConfig protocol = make_c4_ascii_format4_protocol(
      PlcProfile::MelsecQ,
      mcprotocol::serial::SumCheckMode::Disabled,
      mcprotocol::serial::RouteConfig {mcprotocol::serial::HostStationRoute {}});

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
  using mcprotocol::serial::HardwareFlowControl;
  using mcprotocol::serial::PlcProfile;
  using mcprotocol::serial::PosixSerialConfig;
  using mcprotocol::serial::PosixSyncClient;
  using mcprotocol::serial::SerialParity;
  using mcprotocol::serial::Status;
  using mcprotocol::serial::highlevel::make_c4_ascii_format4_protocol;

  const PosixSerialConfig serial(
      "/dev/ttyUSB0",
      19200,
      8,
      1,
      SerialParity::Even,
      HardwareFlowControl::None);

  auto protocol = make_c4_ascii_format4_protocol(
      PlcProfile::MelsecQ,
      mcprotocol::serial::SumCheckMode::Disabled,
      mcprotocol::serial::RouteConfig {mcprotocol::serial::HostStationRoute {}});
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
  using mcprotocol::serial::HardwareFlowControl;
  using mcprotocol::serial::PlcProfile;
  using mcprotocol::serial::PosixSerialConfig;
  using mcprotocol::serial::PosixSyncClient;
  using mcprotocol::serial::SerialParity;
  using mcprotocol::serial::highlevel::make_c4_ascii_format4_protocol;

  const PosixSerialConfig serial(
      "/dev/ttyUSB0",
      19200,
      8,
      1,
      SerialParity::Even,
      HardwareFlowControl::None);
  PosixSyncClient plc;
  auto protocol = make_c4_ascii_format4_protocol(
      PlcProfile::MelsecQ,
      mcprotocol::serial::SumCheckMode::Disabled,
      mcprotocol::serial::RouteConfig {mcprotocol::serial::HostStationRoute {}});
  if (!plc.open(serial, protocol).ok()) {
    return 1;
  }

  std::array<std::uint16_t, 2> original {};
  if (!plc.read_words("D100", original).ok()) {
    return 1;
  }

  const std::array<std::uint16_t, 2> words {0x1234, 0x5678};
  const auto write_status = plc.write_words("D100", words);
  const auto restore_status = plc.write_words("D100", original);
  return write_status.ok() && restore_status.ok() ? 0 : 1;
}
```

### Random read and random write

```cpp
#include <array>
#include <cstdint>

#include "mcprotocol_serial.hpp"

int main() {
  using mcprotocol::serial::HardwareFlowControl;
  using mcprotocol::serial::PlcProfile;
  using mcprotocol::serial::PosixSerialConfig;
  using mcprotocol::serial::PosixSyncClient;
  using mcprotocol::serial::SerialParity;
  using mcprotocol::serial::highlevel::RandomWriteWordSpec;
  using mcprotocol::serial::highlevel::make_c4_ascii_format4_protocol;

  const PosixSerialConfig serial(
      "/dev/ttyUSB0",
      19200,
      8,
      1,
      SerialParity::Even,
      HardwareFlowControl::None);
  PosixSyncClient plc;
  auto protocol = make_c4_ascii_format4_protocol(
      PlcProfile::MelsecQ,
      mcprotocol::serial::SumCheckMode::Disabled,
      mcprotocol::serial::RouteConfig {mcprotocol::serial::HostStationRoute {}});
  if (!plc.open(serial, protocol).ok()) {
    return 1;
  }

  std::uint32_t d100 = 0;
  if (!plc.random_read("D100", d100).ok()) {
    return 1;
  }

  std::array<std::uint16_t, 1> original_d101 {};
  if (!plc.read_words("D101", original_d101).ok()) {
    return 1;
  }

  const std::array<RandomWriteWordSpec, 1> writes {{{.device = "D101", .value = d100, .double_word = false}}};
  const auto write_status = plc.random_write_words(writes);
  const auto restore_status = plc.write_words("D101", original_d101);
  return write_status.ok() && restore_status.ok() ? 0 : 1;
}
```

### Remote control and CPU model

```cpp
#include <cstdio>

#include "mcprotocol_serial.hpp"

int main() {
  using mcprotocol::serial::CpuModelInfo;
  using mcprotocol::serial::HardwareFlowControl;
  using mcprotocol::serial::PlcProfile;
  using mcprotocol::serial::PosixSerialConfig;
  using mcprotocol::serial::PosixSyncClient;
  using mcprotocol::serial::RemoteOperationMode;
  using mcprotocol::serial::RemoteRunClearMode;
  using mcprotocol::serial::SerialParity;
  using mcprotocol::serial::highlevel::make_c4_ascii_format4_protocol;

  const PosixSerialConfig serial(
      "/dev/ttyUSB0",
      19200,
      8,
      1,
      SerialParity::Even,
      HardwareFlowControl::None);
  PosixSyncClient plc;
  auto protocol = make_c4_ascii_format4_protocol(
      PlcProfile::MelsecQ,
      mcprotocol::serial::SumCheckMode::Disabled,
      mcprotocol::serial::RouteConfig {mcprotocol::serial::HostStationRoute {}});
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
  const auto protocol = make_c4_ascii_format4_protocol(
      PlcProfile::MelsecQ,
      mcprotocol::serial::SumCheckMode::Disabled,
      mcprotocol::serial::RouteConfig {mcprotocol::serial::HostStationRoute {}});
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
| Link-direct string | `J1\W100` | Parsed by link-direct helpers, not by `parse_device_address()`. |

## Special helper notes

- Use `read_long_state_bits()` for `LTS/LTC/LSTS/LSTC/LCS/LCC` state reads. Timer and retentive timer state devices use the long-current status block internally; `LCS/LCC` use direct bit reads internally.
- Use `read_link_direct_*()` / `write_link_direct_*()` for `Jn\X/Y/B/SB` bit devices and `Jn\W/SW` word devices. C4 Binary / Format5 and C4 ASCII / Format4 are both confirmed for the validated Q and iQ-R targets when the serial module is configured for the matching format.
- Use `read_native_qualified_words()` / `write_native_qualified_words()` for profiles whose supported `Un\G` / `Un\HG` route is native device access.
- The `0601/1601` qualified helper route is profile/target-specific and is rejected by profiles that require native-qualified access.
- Set `MCPROTOCOL_SERIAL_TRACE=1` when using the synchronous host client to log MC TX/RX frame bytes to stderr.

## Build-time tuning

For small firmware builds, use the PlatformIO environments or define the same macros in your own build.

| Tuning area | Macros |
| --- | --- |
| Buffer capacity | `MCPROTOCOL_SERIAL_MAX_REQUEST_FRAME_BYTES`, `MCPROTOCOL_SERIAL_MAX_RESPONSE_FRAME_BYTES`, `MCPROTOCOL_SERIAL_MAX_REQUEST_DATA_BYTES`, `MCPROTOCOL_SERIAL_MAX_RANDOM_ACCESS_ITEMS`, `MCPROTOCOL_SERIAL_MAX_MULTI_BLOCK_COUNT`, `MCPROTOCOL_SERIAL_MAX_MONITOR_ITEMS`, `MCPROTOCOL_SERIAL_MAX_LOOPBACK_BYTES` |
| Command families | `MCPROTOCOL_SERIAL_ENABLE_RANDOM_COMMANDS`, `MCPROTOCOL_SERIAL_ENABLE_MULTI_BLOCK_COMMANDS`, `MCPROTOCOL_SERIAL_ENABLE_MONITOR_COMMANDS`, `MCPROTOCOL_SERIAL_ENABLE_HOST_BUFFER_COMMANDS`, `MCPROTOCOL_SERIAL_ENABLE_MODULE_BUFFER_COMMANDS`, `MCPROTOCOL_SERIAL_ENABLE_CPU_MODEL_COMMANDS`, `MCPROTOCOL_SERIAL_ENABLE_LOOPBACK_COMMANDS` |
| Codec families | `MCPROTOCOL_SERIAL_ENABLE_ASCII_MODE`, `MCPROTOCOL_SERIAL_ENABLE_BINARY_MODE`, `MCPROTOCOL_SERIAL_ENABLE_FRAME_C4`, `MCPROTOCOL_SERIAL_ENABLE_FRAME_C3`, `MCPROTOCOL_SERIAL_ENABLE_FRAME_C2`, `MCPROTOCOL_SERIAL_ENABLE_FRAME_C1`, `MCPROTOCOL_SERIAL_ENABLE_FRAME_E1` |

CMake exposes the same footprint presets through `MCPROTOCOL_FEATURE_PROFILE`:

| Profile | Behavior |
| --- | --- |
| `full` | Complete host-oriented build, including host sync and CLI. |
| `reduced` | Core-only build with smaller buffers, random/multi-block/monitor/host-buffer/module-buffer disabled, and codec limited to `4C + ASCII`. |
| `ultra` | Reduced profile plus no CPU-model or loopback helpers. |

Non-`full` CMake profiles are core-only. Host sync, CLI, and tests are disabled automatically unless you override the build.

CMake preserves exception and RTTI support by default. A size-constrained build can set
`MCPROTOCOL_DISABLE_EXCEPTIONS_RTTI=ON`; those flags apply only while compiling the library target
and are not propagated to applications that link it.

## Serial config reference

`PosixSerialConfig` has no default constructor. All six connection fields are required and are
validated before the OS serial handle is opened. Values must match the PLC serial module and host
adapter; the library does not infer or retry a different setting.

| Field | Type | Example | Notes |
| --- | --- | --- | --- |
| `device_path` | `std::string_view` | `/dev/ttyUSB0`, `COM3` | Host serial device path. |
| `baud_rate` | `std::uint32_t` | `19200` | Must match the PLC serial module. |
| `data_bits` | `std::uint32_t` | `8` | Binary requires 8. ASCII accepts an explicit 7 or 8. |
| `stop_bits` | `std::uint32_t` | `1` | Explicitly select 1 or 2 to match the module. |
| `parity` | `SerialParity` | `SerialParity::Even` | Explicitly select `None`, `Even`, or `Odd`. |
| `hardware_flow_control` | `HardwareFlowControl` | `HardwareFlowControl::None` | Explicitly select `None` or `RtsCts`; this is separate from RS-485 direction control. |
