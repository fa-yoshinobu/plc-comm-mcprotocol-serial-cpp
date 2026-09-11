# Arduino-ESP32 UART adapter

`Esp32UartClient` is an opt-in adapter for Arduino-ESP32. It leaves the existing
`MelsecSerialClient` API and non-Arduino builds intact. It owns one core client and
one selected UART port; it does not depend on M5 libraries. This is a source-tree
addition, not yet a published Registry release.

## Include and initialization

For a consuming PlatformIO project, remove older standard flags and select GNU
C++17 (the ESP32-S3 SDK uses GNU extensions):

```ini
build_unflags =
    -std=gnu++11
    -std=gnu++14
build_flags =
    -std=gnu++17
```

Do not list `-std=gnu++17` in `build_unflags`, which would remove the flag again.

```cpp
#include <mcprotocol_serial_arduino_esp32.hpp>
using namespace mcprotocol::serial;

Esp32UartClient plc(1); // UART1, exclusive ownership

void setup() {
  Serial.begin(115200); // separate USB/debug channel
  Esp32UartConfig uart;
  uart.baud = 19200;
  uart.format = SERIAL_8E1;
  uart.rx_pin = 39;
  uart.tx_pin = 0;
  uart.direction = Esp32Direction::Rs485Rts;
  uart.rts_pin = 46;
  const auto protocol = ProtocolConfig::c4_binary(
      PlcProfile::MelsecIqF, SumCheckMode::Enabled,
      RouteConfig{HostStationRoute{}}, TimeoutConfig{3000, 250});
  const Status status = plc.begin(uart, protocol);
  if (!status.ok()) Serial.println(status.message);
}
```

`begin()` returns `Status`; success means UART/protocol initialization succeeded,
not that the PLC responded. A port already installed by another driver returns
`Busy`. Constructing or destroying an unopened/rejected adapter does not close
another owner's UART. Use one adapter per port and one calling task per adapter.

Unlike the initial discussion's borrowed-HardwareSerial proposal, this API takes
the UART number and creates HardwareSerial only after ownership validation.
This keeps ESP-IDF's physical TX completion checks tied to the correct UART and
avoids requiring a separately supplied, potentially inconsistent port number.

Do not use Serial1, Modbus, a console, raw ESP-IDF calls, or another task on the
same port while this adapter owns it. Disable the official M5StamPLC Modbus slave
when using UART1 for MC communication. RX/TX/RTS pins must also be reserved by the
application; the adapter does not arbitrate pins against SPI/I2C or other devices.

## UART configuration

| Field | Default | Meaning |
| --- | --- | --- |
| `baud` | 19200 | 300..115200 bps |
| `format` | `SERIAL_8E1` | 7/8 data bits, N/E/O parity, 1/2 stop bits |
| `rx_pin`, `tx_pin` | -1 | Must be explicitly assigned valid, distinct pins |
| `direction` | `External` | No direction GPIO controlled by the adapter |
| `rts_pin` | -1 | Required for `Rs485Rts`; must be -1 for `External` |
| `rx_buffer_bytes` | 1024 | UART driver receive buffer, 256..65536 bytes |

`Rs485Rts` selects ESP32's hardware-driver-controlled half duplex, with active-high
DE and active-low /RE tied to RTS, as on STAMPLC. `External` is for a converter
that manages direction itself or a full-duplex UART connection to an appropriate
transceiver. Raw MCU pins are not RS-232/RS-485 electrical interfaces.

There is no manual-GPIO direction mode in this initial version. CTS flow control
is disabled. Binary Format5 requires 8 data bits; conflicting settings are rejected.
Unsupported pin combinations, modes and formats fail explicitly.

UART format and protocol are separate. Use the existing `ProtocolConfig` for PLC
profile, frame, ASCII format/Binary Format5, sum check, route and timeouts.

## Synchronous usage

```cpp
std::uint16_t value = 0;
Status status = plc.read_word({DeviceCode::D, 100}, value);
if (status.ok()) {
  // Display value with the application's LCD library.
}
```

| Method | Data argument |
| --- | --- |
| `read_word(address, value)` | `uint16_t&` |
| `read_words(address, values)` | `Span<uint16_t>` |
| `read_bit(address, value)` | `bool&` |
| `read_bits(address, values)` | `Span<bool>` |
| `write_word(address, value)` | `uint16_t` |
| `write_words(address, values)` | `Span<const uint16_t>` |
| `write_bit(address, value)` | `BitValue` (bool) |
| `write_bits(address, values)` | `Span<const BitValue>` |

These methods return the transaction's existing `Status`, including
`plc_error_code` and `OperationOutcomeUnknown` for unconfirmed writes. They wait
by calling the same state machine as asynchronous operations and `delay(1)`
between iterations. Application LCD/button handling does not run during this wait.
Only consume output data after success.

## Asynchronous usage

```cpp
std::uint16_t value; // storage must outlive the pending request
bool request_finished = false;
Status request_result;

void on_complete(void*, Status status) {
  request_result = status;
  request_finished = true;
}

// From application logic, when idle:
// Status started = plc.async_read_words({DeviceCode::D, 100},
//     Span<std::uint16_t>(&value, 1), on_complete, nullptr);
// Check started.ok() before expecting a completion callback.

void loop() {
  plc.update();
  // Update buttons/display and schedule the next read here.
}
```

Async methods: `async_read_words`, `async_read_bits`, `async_write_words`,
`async_write_bits`. Their arguments are address, data span, completion callback,
and an optional user pointer. A null callback is allowed. Immediate validation
failure returns a status without calling the callback. An admitted request has
one completion callback, on success, error or cancellation.

One request can be pending. Other submissions/configuration changes return
`Busy`. Callbacks run from `update()` or `cancel()` in the calling task, never
from an ISR. Starting another operation from a callback returns `Busy`; schedule
it on the next application iteration. Do not destroy the adapter inside its
callback. Keep callback context and buffers valid through cancellation/completion.

`update()` attempts at most 64 TX bytes and consumes at most 64 RX bytes per call.
Short positive writes are continued; zero progress is bounded by the transaction
deadline. It does not use `flush()` or wait for an entire response. Physical TX
completion uses ESP-IDF's zero-wait `uart_wait_tx_done`. TX ring capacity is checked
before writes, under the exclusive-owner contract. These are cooperative operations,
not a hard real-time scheduling guarantee. Call `update()` frequently enough for
your baud rate, receive-buffer size and timeout; 1 ms is a practical starting point.
Timeouts are only serviced when the application calls `update()`.

## Random and multi-block operations

The adapter also exposes these native core commands. Each synchronous method has
an `async_` counterpart with the same arguments followed by a completion callback
and an optional user pointer. They use the same UART state machine and add no
member variables or buffers.

| Synchronous method | Arguments before callback |
| --- | --- |
| `random_read` | `RandomReadRequest`, word output span, dword output span |
| `random_write_words` | `RandomWriteWordItem` span, `RandomWriteDWordItem` span |
| `random_write_bits` | `RandomWriteBitItem` span |
| `multi_block_read` | `MultiBlockReadRequest`, word output span, bit output span, `MultiBlockReadBlockResult` span |
| `multi_block_write` | `MultiBlockWriteRequest` |

For example, read D100 and D200 together:

```cpp
const RandomReadWordItem items[] = {
    {{DeviceCode::D, 100}}, {{DeviceCode::D, 200}}};
std::uint16_t values[2] {};
const Status status = plc.random_read(RandomReadRequest(items, {}), values, {});
// Use values[0] and values[1] only when status.ok().
```

Read two separate contiguous ranges:

```cpp
const MultiBlockReadBlock blocks[] = {
    {{DeviceCode::D, 100}, 2, false},  // D100..D101
    {{DeviceCode::D, 200}, 3, false}}; // D200..D202
std::uint16_t values[5] {};
MultiBlockReadBlockResult results[2] {};
const Status status = plc.multi_block_read(
    MultiBlockReadRequest(blocks), values, {}, results);
```

Results preserve block order and describe offsets into the word/bit output spans.
A bit block's `points` counts 16-bit groups: one point requires 16 `BitValue`
output entries, not one. Random reads of bit devices likewise use word masks;
there is no separate native random-bit-read method.

Core PLC/frame restrictions, feature switches and buffer limits still apply;
unsupported requests return the core status without being split into individual
transactions. Async buffers and callback context must remain valid through
completion/cancellation. Writes are never automatically retried, and an
unconfirmed write can return `OperationOutcomeUnknown`.

Other advanced core commands are still available through the existing standalone core
API. This initial adapter intentionally does not expose its core object, so callers
cannot bypass its TX ownership and request lifecycle.

## Cancellation, closing and recovery

- `cancel()` cancels an admitted request. During TX it stops driving the UART before
  reporting physical TX abort to the core. No write is automatically retried.
- `end()` closes an idle adapter; it returns `Busy` during an active operation.
- `requires_transport_reset()` reports a core-required reset or a closed UART.
- `configure(protocol)` can change protocol on an idle healthy UART. It does not
  clear a reset requirement.
- `recover(protocol)` explicitly closes/reopens the saved UART configuration and
  reconfigures the core. Call `begin()` successfully once before using it.

**Before recovery, the application must establish that an old PLC response can no
longer arrive.** Reopening the local UART and discarding buffered bytes cannot
guarantee this on the wire. Use the PLC/module's documented maximum response timing
and communication reset procedure. No universal quiet-time value is assumed, and
the adapter does not implement automatic recovery or retries.

On UART transmission timeout/error, the adapter detaches TX/RTS and deletes its
driver before reporting completion. RTS is left in receive mode. A write whose
execution cannot be confirmed retains the core's outcome-unknown status.

The destructor cancels an active request while members are still alive, then closes
the owned port. Prefer explicit cancellation and closing before releasing buffers.
Do not share or externally close/reopen the port underneath the adapter.

## Memory and validation

The adapter contains one existing core client, not a second protocol implementation.
Its additional state and HardwareSerial object are small, but the UART driver allocates
RX/TX buffers and RTOS resources dynamically. The RX buffer defaults to 1024 bytes
and the TX ring to 256 bytes. Static ELF size is not total runtime heap usage.

After adding random/multi-block wrappers, the unchanged STAMPLC LCD D100 demo
build uses 45,344 bytes of static RAM (unchanged) and 589,821 bytes of Flash
(100 bytes above 589,721). This includes a shared admission-helper change; unused
methods do not guarantee an identical final binary. Physical PLC tests remain pending.

With the checked ESP32-S3 full-feature build, the adapter object occupies 22,212
bytes, versus 22,060 bytes for a bare core client (152 bytes of additional static
object storage). The USB-only D100 example uses 41,224 bytes of static RAM and
319,773 bytes of program space. It does not include the M5 LCD libraries and should
not be compared directly with the STAMPLC LCD demo's whole-program size.

Build-check target: Arduino-ESP32 2.0.17, Espressif32 6.12.0, ESP32-S3. Other Arduino
cores (RP2040/AVR) are rejected by the dedicated header; ESP-IDF-only applications
continue using the existing transport-independent core. Arduino-ESP32 3.x and other
ESP32 SoCs require their own validation before being claimed as supported targets.

Host tests exercise fragmented transmission/reception, busy/reentrant calls,
timeouts including millis wrap, cancellation, transport failures, PLC errors,
uncertain writes, recovery admission, and port ownership with a stubbed ESP32 API.
The ESP32-S3 example is build-checked against real headers. Hardware RTS timing,
line noise, parity/framing errors, UART overflow, and disconnect/reconnect behavior
must still be tested on physical equipment. The initial implementation relies on
MC framing/checksum/deadline checks; it does not expose the ESP-IDF UART error event
queue as a diagnostic API.

GCC 8's existing `string_view` is now recognized by the compatibility header; no
application-side `gcc8_string_view_fix.h` is required when building this source.

See the [D100 example](../../examples/platformio_esp32s3_arduino_uart_adapter/README.md).
