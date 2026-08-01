# Usage guide

The library exposes three practical entry paths. Pick one based on how much transport control your application needs.

## Design summary

The public API is designed for host tools and MCU firmware:

- no exceptions
- no RTTI
- no dynamic allocation in the library
- caller-owned buffers via `mcprotocol::serial::Span`
- transport-agnostic client state machine

`Span<T>` is the library's C++17 non-owning contiguous view. Construct it from pointer/count,
pointer-pair, a C-array, or a matching `std::array` lvalue. Other containers use the explicit
`Span<T>(container.data(), container.size())` form; rvalue arrays are rejected so the view cannot
immediately dangle. A mutable `Span<T>` converts to `Span<const T>`, never the reverse. `try_at`,
`try_first`, and `try_subspan` report invalid indexes or ranges without producing an invalid view;
`operator[]` requires `index < size()`.

Raw octets use `mcprotocol::serial::Byte`, not `std::byte`. `Byte` has no implicit integer
conversion or arithmetic; use `mcprotocol::serial::byte_to_integer<Integer>(value)` when a numeric
representation is required.

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

`ProtocolConfig` has no public default constructor. Use exactly one tagged construction path:

- `ProtocolConfig::c4_binary(profile, sum_check_mode, route)` fixes C4 Binary/Format5 and exposes
  no ASCII-format input.
- `ProtocolConfig::ascii(AsciiFrameKind::C4|C3|C2|C1, format, profile,
  sum_check_mode, route)` requires an ASCII format and cannot select 1E.
- `ProtocolConfig::e1(code_mode, profile, route)` fixes the 1E frame and exposes neither an ASCII
  format nor a sum-check option because those fields do not exist in the 1E wire format.

The C-frame paths require `SumCheckMode::Enabled` or `SumCheckMode::Disabled`; the CLI likewise
requires `--sum-check` for C1/C2/C3/C4 and rejects it for 1E. The library never switches frame,
code mode, format, profile, sum-check policy, or route after an error or timeout.

### Route selection

Use `RouteConfig {HostStationRoute {}}` for the connected host-station route. This type has no
station, network, PC, destination-module, or self-station inputs; the protocol-defined connected
station header is fixed internally. For multidrop, select the frame-specific type:
`C1MultidropRoute(station)`, a 2C/3C/4C topology-specific route, or `E1Route(pc_target)`.
For a normal or 1:n connection, use `C2StandardMultidropRoute(station)`,
`C3StandardMultidropRoute(station, network, pc_target)`, or
`C4StandardMultidropRoute(station, network, pc_target, destination_module)`. For an m:n
connection, use the corresponding `C2MnMultidropRoute`, `C3MnMultidropRoute`, or
`C4MnMultidropRoute` and supply `SelfStationNo::number(0U..0x1FU)` as the final mandatory
argument. Station zero, network zero, and m:n self-station zero remain valid only when explicitly
passed. A 3C/4C PC target is constructed with
`C34PcTarget::number(0x01U..0x78U)` or one of `control_system()`, `standby_system()`,
`special_fe()`, and `connected_station()`. A 1E target uses
`E1PcTarget::number(0x01U..0x40U)` or `connected_station()`. The special wire values cannot be
passed through `number()`, which prevents an ordinary number from silently acquiring special
meaning. Raw numeric CLI values `0x7D`, `0x7E`, `0xFE`, and `0xFF` are accepted only at that
external boundary and normalized to the corresponding canonical selector before validation.

The 4C destination module is also mandatory. Use `C4DestinationModule::own_station()`,
`multiple_cpu(1U..4U)`, one of the four `redundant_*_cpu()` selectors, or
`explicit_target(io_number, station_number)` for a configuration-dependent target supported by the
selected hardware. The historical RemoteHead constants share wire values with Multiple CPU
constants; this library does not reinterpret one meaning as the other. Use an explicit target when
the module configuration—not the generic selector name—is the source of truth.

The self-station number identifies the request source on an m:n connection. Standard routes expose
no self-station input and encode zero internally. The library validates the field width but cannot
infer the C24 station assignment or the total station-count constraints of a particular wiring and
parameter configuration. Assign those values from the actual serial-network configuration and do
not reuse a C24-side station number accidentally.

`RouteConfig {}` is invalid. The CLI likewise requires `--route host` or `--route multidrop`;
3C/4C additionally require `--network` and `--pc-target`, while 1E multidrop requires
`--pc-target`. A 4C multidrop route additionally requires `--module-target`; use
`--module-target own` when the explicit own-station selector is correct. Every 2C/3C/4C multidrop
CLI command also requires `--topology standard|mn`. `standard` rejects `--self-station`; `mn`
requires `--self-station 0..31`, including an explicit zero when zero is assigned. A station, PC,
module, topology, or self-station value never selects or changes a route implicitly.

Every response route header field that exists in the selected frame is compared with the
configured route. A complete 2C/3C/4C response from a different self-station—or a 3C/4C response
from a different station, network, or PC target, or a 4C response from a different destination module—is
discarded while the client continues waiting for the matching response. The 1E response message
does not echo its request PC target, so the PC target is enforced on request construction and
encoding rather than inferred from response bytes. Malformed ASCII route hexadecimal is reported
as a parse error. Timeout, NAK, malformed input, or mismatch never causes automatic route discovery
or fallback.

### Absolute transaction timeout and 1E monitoring timer

`TimeoutConfig::response_timeout_ms` is the one absolute transaction deadline. Its omitted value
is 3000 ms for every frame and code mode. Call `notify_tx_started(now_ms)` immediately before the
first UART write. That same deadline covers partial writes, physical TX drain, all receive chunks,
response correlation, and complete decode. No byte, chunk, ignored response, or phase transition
restarts it. An explicit value must be 1..2147483647 ms so 32-bit monotonic-clock comparisons remain
wrap-safe.

Every timeout sets `requires_transport_reset()`, including Format2. Abort, drain, and close/reopen
the exact UART generation, then call `configure()` before another request. `PosixSyncClient` does
this retirement itself and must be opened again. The timed-out request is not retried.

The 1E ACPU monitoring timer is a different PLC-side protocol field. It defaults independently to
4000 ms (`0x0010` in 250 ms wire units). Set it with
`config.e1_monitoring_timer = E1MonitoringTimer::milliseconds(value)`. Explicit values must be
exact 250 ms units from 0 through 16383750 ms; the library never rounds or saturates them and never
derives them from the communication timeout. The CLI equivalent is
`--e1-monitoring-timer-ms`; it is rejected for non-1E frames.

Remote RESET is the dedicated no-normal-response operation. Its completion means that the request
bytes were transmitted; it does not wait three seconds and does not confirm that the PLC reset.
Transport failure is still reported. Other commands, including global-signal control and
transmission-sequence initialization, do not convert a response timeout into success.

### Format2 request identity

Do not configure a fixed Format2 block number. `MelsecSerialClient` assigns a new value for each
wire request, advances through `00` to `FF`, wraps to `00` only after the prior request has finished,
and accepts only the matching response. A late response from a timed-out or cancelled request is
discarded instead of being returned as the next request's result.

`FrameCodecContext::format2(number)` is available only for raw frame construction, negative tests,
and protocol investigation where the caller intentionally owns one wire identity. Passing a
Format2 context to another format, or using a Format2 raw codec without one, is an error. The CLI
does not expose a normal `--block-no` connection option.

Public request and item types require their semantic inputs at construction. A missing device,
address, count/data span, value, target, state, channel, or requested mode change is never replaced
with D0, address zero, zero/OFF, or another valid operation. Explicit D0, address zero, value zero,
and Boolean `false` remain valid when passed by the caller. Individual bit inputs use native C++
`bool` only; packed block words remain `std::uint16_t`. Empty request containers are rejected before
any transmit frame is made. Receive/output storage types remain
default constructible.

```cpp
#include <cstdint>
#include <cstdio>

#include "mcprotocol_serial.hpp"

int main() {
  using mcprotocol::serial::BatchReadWordsRequest;
  using mcprotocol::serial::DeviceAddress;
  using mcprotocol::serial::DeviceCode;
  using mcprotocol::serial::PlcProfile;
  using mcprotocol::serial::ProtocolConfig;
  using mcprotocol::serial::highlevel::make_c4_ascii_format4_protocol;

  ProtocolConfig protocol = make_c4_ascii_format4_protocol(
      PlcProfile::MelsecQ,
      mcprotocol::serial::SumCheckMode::Disabled,
      mcprotocol::serial::RouteConfig {mcprotocol::serial::HostStationRoute {}});

  const BatchReadWordsRequest request(DeviceAddress {DeviceCode::D, 100U}, 2U);

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
  using mcprotocol::serial::highlevel::RandomWriteDWordSpec;
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

  std::uint16_t d100 = 0;
  if (!plc.random_read_word("D100", d100).ok()) {
    return 1;
  }

  std::array<std::uint16_t, 1> original_d101 {};
  if (!plc.read_words("D101", original_d101).ok()) {
    return 1;
  }

  const std::array<RandomWriteWordSpec, 1> writes {{RandomWriteWordSpec("D101", d100)}};
  const auto write_status = plc.random_write_words(writes);
  if (write_status.code == mcprotocol::serial::StatusCode::OperationOutcomeUnknown) {
    // The PLC may already have applied the value. Inspect the target; do not retry automatically.
    return 2;
  }
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
  const auto run_status = plc.remote_run(
      RemoteOperationMode::DoNotExecuteForcibly,
      RemoteRunClearMode::DoNotClear);
  if (run_status.code == mcprotocol::serial::StatusCode::OperationOutcomeUnknown) {
    // The request may have reached the PLC. Inspect the PLC state; do not retry automatically.
    return 2;
  }
  return run_status.ok() ? 0 : 1;
}
```

Sparse access never infers width from the device name. Use `random_read_word`/
`RandomWriteWordSpec` for 16-bit data and `random_read_dword`/`RandomWriteDWordSpec` for 32-bit
data. Mixed low-level requests keep separate `word_items`/`dword_items` and separate typed output
spans. `LZ`, `LTN`, `LSTN`, and `LCN` require the DWord path. Link-direct sparse read, write, and
monitor APIs are Word-only.

Every random-write item/spec is constructed with both the target and value. There is no default
constructor and no omitted-value meaning. Explicit Word/DWord zero and Boolean `false` are valid;
missing, empty, or out-of-range values reject the complete request before transmission.
The CLI follows the same rule: use `random-write-words D100=0`, `random-write-dwords D200=0`, or
`random-write-bits M100=0`; a missing `=VALUE` is rejected before the serial device is opened.
After transmission begins, an unconfirmed random-write result is `OperationOutcomeUnknown`, because
the PLC may already have changed. The library clears the pending frame and never retries the write.

This unknown-outcome rule applies to every state-changing command, including contiguous and block
writes, buffer and file-register writes, remote control, password lock state, user-frame changes,
global signal control, mode switching, and transmission-sequence initialization. A timeout,
transport failure, cancellation, malformed response, or other result that cannot confirm the PLC
state is not reported as a definite pre-send failure. Inspect the target state before deciding what
to do next; the library does not resend automatically.

| Status | Meaning / retry rule |
| --- | --- |
| `Timeout` | The configured absolute deadline expired for a read or before a state change could be ambiguous. Reopen/reset before a later operation. |
| `Cancelled` | The caller cancelled; this is not a timeout. |
| `Closed` | A local lifecycle close interrupted/rejected the operation. |
| `NotConnected` | No configured/open session exists. |
| `Transport` | Non-timeout local I/O failure. |
| `Framing` / `Parse` / `SumCheckMismatch` | A response was malformed or invalid. |
| `PlcError` | The PLC returned a confirmed NG/end code; inspect `plc_error_code`. |
| `OperationOutcomeUnknown` | A state-changing request may have been sent. Do not retry automatically; inspect `status.cause` (`Timeout`, `Cancelled`, `Closed`, `Transport`, or protocol reason) and verify PLC state. |

The CLI prints the machine classification before the message. For an unknown state-changing result,
it also prints the structured cause, for example `OperationOutcomeUnknown: ... (cause=Timeout)`.
The synchronous facade and CLI both use the core completion callback as the final result after a
request is admitted; neither keeps a separate state-changing command list.

Remote RUN always requires two explicit decisions. `RemoteOperationMode` selects whether a RUN
conflict is handled forcibly. `RemoteRunClearMode` selects whether device state is retained,
cleared outside the latch range, or cleared completely. There is no overload that infers either
choice. If transmission starts but no trustworthy result is received, the status is
`OperationOutcomeUnknown`: the PLC may already be RUN, so check its state instead of resending the
command automatically.

Remote PAUSE also requires an explicit `RemoteOperationMode`. This mode controls conflict handling
when another external device owns the remote STOP/PAUSE operation; it is not an output-retention
setting. `DoNotExecuteForcibly` preserves that ownership conflict, while `ExecuteForcibly`
overrides it. The library never changes from non-forced to forced after an error. An unconfirmed
PAUSE returns `OperationOutcomeUnknown`, so inspect the PLC state and do not resend automatically.

## Entry path 3: low-level async client

`MelsecSerialClient` owns the protocol state machine but not the UART. Your code configures the
client, starts an async request, calls `notify_tx_started(now)` immediately before its first UART
write, sends `pending_tx_frame()`, calls `notify_tx_complete(now, status)`, feeds response bytes
with `on_rx_bytes()`, calls `notify_rx_failure(status)` if response reception fails, and calls
`poll()` for deadline handling. TX and receive failures are always explicit: pass `ok_status()` to
`notify_tx_complete()` only after the UART confirms physical transmission completion, and otherwise
pass the actual transport failure. Use `cancel()` only for caller-requested cancellation; do not
replace a receive-side `Timeout` or `Transport` status with cancellation.

One instance admits one wire transaction. A second operation returns `Busy` before request-state
mutation. The class retains caller-owned spans, so it has no internal pending queue. Calls from
different operating-system threads into the same instance are prohibited; schedule them in the
caller. Separate instances have independent state and may progress concurrently.

RS-485 direction hooks are optional. Leave both callbacks unset for RS-232 or hardware/driver-
controlled RS-485. When application-controlled direction is needed, install both `on_tx_begin` and
`on_tx_end` together; a one-sided hook is rejected. Hooks cannot be replaced while a request is
busy. If cancellation is requested during TX, the request remains busy until the UART reports
physical completion or abort with `notify_tx_complete`; only then is `on_tx_end` called exactly
once and the completion callback released. Cancellation before `notify_tx_started()` completes
immediately as `Cancelled`, invokes no TX hook, and cannot become `OperationOutcomeUnknown` because
no transport write has started.

This is the path used in the PlatformIO examples.

```cpp
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "mcprotocol_serial.hpp"
#include "mcprotocol/serial/span.hpp"

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
      BatchReadWordsRequest(
          DeviceAddress {DeviceCode::D, 100U},
          static_cast<std::uint16_t>(words.size())),
      mcprotocol::serial::Span<std::uint16_t>(words.data(), words.size()),
      on_complete,
      &completion);
  if (!status.ok()) {
    return 1;
  }

  const mcprotocol::serial::Span<const mcprotocol::serial::Byte> frame = client.pending_tx_frame();
  status = client.notify_tx_started(0);
  if (!status.ok()) {
    return 1;
  }
  // Send `frame` through your UART here.
  (void)frame;
  status = client.notify_tx_complete(1, mcprotocol::serial::ok_status());
  if (!status.ok()) {
    return 1;
  }

  const std::array<std::uint8_t, 8> response_data {'1', '2', '3', '4', '5', '6', '7', '8'};
  std::array<std::uint8_t, mcprotocol::serial::kMaxResponseFrameBytes> response_frame {};
  std::size_t response_frame_size = 0;
  status = FrameCodec::encode_success_response(
      protocol,
      mcprotocol::serial::Span<const std::uint8_t>(response_data.data(), response_data.size()),
      response_frame,
      response_frame_size);
  if (!status.ok()) {
    return 1;
  }

  std::array<mcprotocol::serial::Byte, mcprotocol::serial::kMaxResponseFrameBytes> rx_frame {};
  std::memcpy(rx_frame.data(), response_frame.data(), response_frame_size);
  client.on_rx_bytes(2, mcprotocol::serial::Span<const mcprotocol::serial::Byte>(rx_frame.data(), response_frame_size));
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

### Single-request capacity and aggregation

Except for the aggregate described below, every public operation represents exactly one PLC
request. It never splits an oversized call, retries a smaller count, or grows beyond the selected
fixed capacities. Admission uses the minimum protocol/profile/wire/request/response/decoder/output
limit; binary calculations assume every escapable byte expands through DLE stuffing. A request that
cannot fit returns `InvalidArgument` before TX, while a caller output span that is independently too
small returns `BufferTooSmall`.

`PosixSyncClient::read_long_state_bits()` is explicitly aggregate when an
`LTS`/`LTC`/`LSTS`/`LSTC` call requests more than one point. It validates the complete address,
profile, request, and response plan before the first send; issues one four-word status-block request
per point in address order; stops at the first failure; and changes caller output only after every
request succeeds. The result is non-atomic because internal requests can observe different PLC scan
times. Use a one-point read or a PLC-side snapshot/handshake when the values must share one coherence
point. `LCS`/`LCC` remain one direct bit request.

Callers that need any other group of independent reads or writes submit explicit operations. Writes
are never automatically split, because partial completion and outcome-unknown handling must remain
explicit.

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
adapter; the library does not infer or retry a different setting. A successful open replaces the
port's inherited line behavior: POSIX input/output/local modes are raw, and Win32 preserves only
documented driver-reserved/provider DCB state. Software flow control and DTR/DSR flow are disabled,
and RTS is either disabled (`None`) or owned by the OS RTS/CTS handshake (`RtsCts`).

| Field | Type | Example | Notes |
| --- | --- | --- | --- |
| `device_path` | `std::string_view` | `/dev/ttyUSB0`, `COM3` | Host serial device path. |
| `baud_rate` | `std::uint32_t` | `19200` | Must match the PLC serial module. |
| `data_bits` | `std::uint32_t` | `8` | Binary requires 8. ASCII accepts an explicit 7 or 8. |
| `stop_bits` | `std::uint32_t` | `1` | Explicitly select 1 or 2 to match the module. |
| `parity` | `SerialParity` | `SerialParity::Even` | Explicitly select `None`, `Even`, or `Odd`. |
| `hardware_flow_control` | `HardwareFlowControl` | `HardwareFlowControl::None` | Explicitly select `None` or `RtsCts`; this is separate from RS-485 direction control. |
