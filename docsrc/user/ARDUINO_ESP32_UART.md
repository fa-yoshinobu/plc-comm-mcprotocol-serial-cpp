# Using the Arduino-ESP32 UART Adapter

`Esp32UartClient` provides MC protocol communication over an ESP32 UART.
It supports single-point, contiguous, random, and multi-block reads and writes,
with synchronous and asynchronous APIs.
It does not depend on M5 libraries. The application handles the LCD and buttons.

## Requirements and Configuration

Use Arduino-ESP32 and C++17.
Builds have been verified for ESP32-S3 and ESP32-C3 with Arduino-ESP32 2.0.17.
Arduino-ESP32 3.x is unverified. This adapter cannot be used on RP2040, AVR, or standalone ESP-IDF.

Make sure the library includes `mcprotocol_serial_arduino_esp32.hpp`.
Add the following settings to your existing PlatformIO configuration, keeping any other required flags.

```ini
build_unflags =
    -std=gnu++11
    -std=gnu++14

build_flags =
    -std=gnu++17
    -DMCPROTOCOL_SERIAL_MAX_REQUEST_FRAME_BYTES=768
    -DMCPROTOCOL_SERIAL_MAX_RESPONSE_FRAME_BYTES=768
    -DMCPROTOCOL_SERIAL_MAX_REQUEST_DATA_BYTES=384
```

Adding `-std=gnu++17` to `build_unflags` removes the required compiler flag.
Apply the same buffer capacity definitions to both the application and the library.

These settings are intended for reads and writes involving a small number of points.
They do not disable features, but frame capacity limits the number of points per request.
Before increasing the point count, check the frame capacity and the stack size of the task in use.
The UART receive buffer is separate from these protocol frame buffers.

## Configuring the Connection

Use `Esp32UartConfig` for UART settings and `ProtocolConfig` for the PLC communication format.

| UART setting | Default | Description |
| --- | --- | --- |
| `baud` | 19200 | 300 to 115200 bps |
| `format` | `SERIAL_8E1` | 7/8 data bits, no/even/odd parity, 1/2 stop bits |
| `rx_pin` / `tx_pin` | -1 | RX/TX pins; must be specified explicitly |
| `direction` | `External` | External for automatic direction control or similar; Rs485Rts for RTS-controlled RS-485 |
| `rts_pin` | -1 | Direction control pin for Rs485Rts; -1 for External |
| `rx_buffer_bytes` | 1024 | UART receive buffer capacity, from 256 to 65536 bytes |

`Rs485Rts` is intended for half-duplex RS-485 circuits with DE and /RE connected to RTS.
With `External`, the adapter does not operate a direction control pin.
Use an RS-485/RS-232 transceiver suitable for your board.
CTS flow control is not supported.

The following example configures a connection between STAMPLC PWR-485 and the FX5U built-in RS-485 port.

| Item | Setting |
| --- | --- |
| UART / pins | UART1, RX=39, TX=0, DIR=46 |
| Baud rate / data format | 19200 bps, 8 data bits, even parity, 1 stop bit |
| PLC profile | `PlcProfile::MelsecIqF` |
| MC frame | `ProtocolConfig::c4_binary()`: 4C binary, format 5 |
| Sum check / station number | Enabled / 0 |

Change the pins for other boards.
Match the PLC settings for frame format, sum check, station number, baud rate, and parity.
Binary format 5 requires 8 data bits.

## Reading D100 Periodically with the Asynchronous API

The following program reads D100 as a signed 16-bit integer and sends the next request
one second after the read completes.
Values and errors are printed to the debug Serial interface.
The Serial connection depends on the board and USB settings. Keep it separate from UART1 used by the PLC.

```cpp
#include <Arduino.h>
#include <mcprotocol_serial_arduino_esp32.hpp>

using namespace mcprotocol::serial;

Esp32UartClient plc(1); // This class owns UART1 exclusively.
std::int16_t d100 = 0; // Keep the asynchronous output alive until completion.
std::uint32_t nextRead = 0;
bool ready = false;

void onRead(void*, Status status) {
  if (!status.ok()) {
    ready = false;
    Serial.printf("COMM Error: %s (PLC=%04X)\n",
        status.message, static_cast<unsigned>(status.plc_error_code));
    return; // Stop communication on error.
  }
  Serial.printf("D100:%d\n", static_cast<int>(d100));
  nextRead = millis() + 1000;
}

void setup() {
  Serial.begin(115200);

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
  ready = status.ok();
  if (!ready) onRead(nullptr, status);
}

void loop() {
  plc.update(); // Advance transmission, reception, and timeout handling.

  if (ready && !plc.busy() &&
      static_cast<std::int32_t>(millis() - nextRead) >= 0) {
    const Status status =
        plc.async_read_word({DeviceCode::D, 100}, d100, onRead);
    if (!status.ok()) onRead(nullptr, status); // Request admission failed.
  }

  // Update the LCD, buttons, and other application components here.
  delay(1);
}
```

A successful `begin()` means initialization succeeded; it does not confirm communication with the PLC.
Check the read completion result to confirm successful communication.
`TimeoutConfig{3000, 250}` specifies the overall response timeout and the inter-byte receive timeout in milliseconds.
The overall deadline includes physical transmission time.

## Synchronous and Asynchronous APIs

| Mode | When the function returns | How to check the result |
| --- | --- | --- |
| Synchronous | After communication completes or an error occurs | Returned Status |
| Asynchronous | Immediately after request admission | Returned Status for admission; callback Status for completion |

While a synchronous function waits for a response, LCD and button processing in the same task cannot proceed.
Use asynchronous functions when the display or I/O must keep updating.

Follow these rules for asynchronous operations:

- Call `update()` regularly from `loop()` or equivalent. Long gaps also delay receive processing and deadline checks.
- Only one request can run at a time. Additional requests return Busy while `busy()` is true.
- If request admission fails, the completion callback is not called for that request.
- After successful admission, a supplied callback is called exactly once on success, error, or cancellation.
- Keep output buffers and data referenced by the callback alive until completion or completed cancellation.
- Callbacks run in the task that calls `update()` or `cancel()`, not in an interrupt handler.
- Do not start a new request inside a callback. Start it in the next loop iteration.
- Do not access the same adapter concurrently from multiple tasks.

## Read and Write Functions

Specify `address` as, for example, `{DeviceCode::D, 100}`.
`Span<T>` passes an array and its element count. Fixed-size arrays can be passed directly.
Available devices and point counts depend on the PLC and frame format, even when the function name is the same.

| Synchronous function | Data |
| --- | --- |
| `read_word(address, value)` | uint16_t& or int16_t& |
| `read_words(address, values)` | Span<uint16_t> |
| `read_bit(address, value)` | bool& |
| `read_bits(address, values)` | Span<bool> |
| `write_word(address, value)` | uint16_t |
| `write_words(address, values)` | Span<const uint16_t> |
| `write_bit(address, value)` | BitValue (bool) |
| `write_bits(address, values)` | Span<const BitValue> |

For asynchronous operations, use `async_read_words`, `async_read_bits`, `async_write_words`,
and `async_write_bits`, passing a callback and an optional user pointer after the data arguments.
For a single signed word, you can also use `async_read_word(address, int16_t&, callback, user)`.
For other single-point operations, pass one element to the corresponding array-based function.

Signed reads interpret the sign before invoking the callback.
The signed output is unchanged on error. For all reads, use the returned values only on success.

## Random Access, Multiple Blocks, and 32-Bit Values

| Synchronous function | Arguments |
| --- | --- |
| `random_read` | RandomReadRequest, WORD output, DWORD output |
| `random_write_words` | RandomWriteWordItem array, RandomWriteDWordItem array |
| `random_write_bits` | RandomWriteBitItem array |
| `multi_block_read` | MultiBlockReadRequest, WORD output, bit output, MultiBlockReadBlockResult output |
| `multi_block_write` | MultiBlockWriteRequest |

Each function has an asynchronous version with the `async_` prefix.
Append a callback and an optional user pointer to the arguments.

Random access groups noncontiguous addresses in one request.
WORD and DWORD results are stored in separate arrays, in the order specified.
Random reads of bit devices treat the data as word-sized groups of bits;
there is no dedicated random bit-read function.

Multi-block requests group ranges such as two points starting at D100 and three points starting at D200.
The result fields `data_offset` and `data_count` specify the position and count within the corresponding WORD or bit output array.
**For a bit block, points=1 means 16 bits. The output requires 16 elements.**

A DWORD in ordinary D registers uses two consecutive words, with the low word first.
Convert between float and the corresponding 32-bit DWORD representation using `memcpy`, not a numeric cast.

Unsupported requests and requests that exceed capacity return errors.
The adapter does not automatically split them into multiple requests.
See the [usage examples](../../examples/esp32_uart_usage/README.md) for code demonstrating each operation.

## UART Ownership and Shutdown

`Esp32UartClient plc(1)` initializes and uses UART1.
Do not use the same port through Serial1, Modbus, or another adapter.
If another driver owns the port, `begin()` returns Busy.
Also assign pins so they do not conflict with SPI, I2C, or other peripherals.

When using UART1 on M5StamPLC, disable the official library's Modbus slave feature
before initializing M5StamPLC. For a configuration example that includes the LCD,
see [async_lcd.cpp](../../examples/esp32_uart_usage/async_lcd.cpp).

- `cancel()`: Cancels the active request. If transmission is in progress, it also stops physical transmission.
- `end()`: Closes an idle UART. Returns Busy while a request is active, so call cancel first.
- `configure(protocol)`: Changes the protocol configuration on an idle, healthy UART. It does not clear a recovery requirement.

Complete or cancel the request before destroying output buffers or callback data.
Do not destroy the adapter inside a callback.

## Errors and Resuming Communication

Check success with `Status::ok()`.
`code` identifies the error category, `message` provides a description, and `plc_error_code` holds the detailed PLC error code.
A write whose outcome cannot be confirmed may return `OperationOutcomeUnknown`.

If `requires_transport_reset()` is true, you cannot send the next request without recovery.
It is also true when the UART is closed.
If initialization fails, correct the cause and call `begin()`.

To recover a UART after a successful begin, follow this sequence:

1. Stop new requests and check the wiring, configuration, and PLC state.
2. Follow the PLC or communication module procedure to ensure that no delayed response from the previous transaction can still arrive.
3. Call `recover(protocol)` to reinitialize the UART and protocol configuration.
4. Check that it succeeds before starting a new request.

Reopening the UART, discarding its receive buffer, or resetting the MCU alone cannot
exclude delayed responses still on the line or pending in the PLC. A fixed waiting period does not guarantee this either.
The adapter does not recover or retry automatically.

The PLC may have completed a write even if its response never arrives.
Do not blindly resend a request whose write outcome is unknown.
See the [ESP32-C3 recovery example](../../examples/platformio_esp32c3_arduino_async_polling_reconnect/README.md) for an implementation.
