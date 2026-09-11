#include <Arduino.h>
#include "mcprotocol_serial_arduino_esp32.hpp"

#ifndef MCPROTOCOL_EXAMPLE_PLC_BAUD
#define MCPROTOCOL_EXAMPLE_PLC_BAUD 19200
#endif
#ifndef MCPROTOCOL_EXAMPLE_DEBUG_BAUD
#define MCPROTOCOL_EXAMPLE_DEBUG_BAUD 115200
#endif
#ifndef MCPROTOCOL_EXAMPLE_POLL_INTERVAL_MS
#define MCPROTOCOL_EXAMPLE_POLL_INTERVAL_MS 1000U
#endif

namespace {
using namespace mcprotocol::serial;

// Own UART1 exclusively; do not also initialize Serial1.
// The adapter checks partial sends, physical TX completion and deadlines.
// It does not flush away an early PLC response.
Esp32UartClient plc(1);
const auto protocol = ProtocolConfig::ascii(
    AsciiFrameKind::C4, AsciiFormat::Format4, PlcProfile::MelsecQ,
    SumCheckMode::Disabled, RouteConfig{HostStationRoute{}});
std::uint16_t words[4] {};
std::uint32_t next_read = 0;
bool stopped = false;
bool recovery_required = false;
bool begun = false;
bool online = false, connected_once = false;

void completed(void*, Status status) {
  if (!status.ok()) {
    Serial.printf("read failed: %s (PLC=%04X)\n", status.message,
        static_cast<unsigned>(status.plc_error_code));
    online = false;
    if (plc.requires_transport_reset()) {
      stopped = true;
      recovery_required = begun;
      Serial.println("Recovery required: exclude old replies first; then send r. See README.");
    } else if (status.code == StatusCode::PlcError) {
      // A complete PLC error response ended this read: no transport reset needed.
      next_read = millis() + MCPROTOCOL_EXAMPLE_POLL_INTERVAL_MS;
      Serial.println("Read-only retry after poll interval.");
    } else {
      stopped = true; // Configuration/admission errors need correction, not a retry loop.
      Serial.println("Correct configuration, then RESET.");
    }
    return;
  }
  if (!online) {
    Serial.println(connected_once ? "recovered" : "connected");
    online = connected_once = true;
  }
  Serial.printf("D100=%04X D101=%04X D102=%04X D103=%04X\n",
      unsigned(words[0]), unsigned(words[1]), unsigned(words[2]), unsigned(words[3]));
  next_read = millis() + MCPROTOCOL_EXAMPLE_POLL_INTERVAL_MS;
}
} // namespace

void setup() {
  Serial.begin(MCPROTOCOL_EXAMPLE_DEBUG_BAUD);
  Esp32UartConfig uart;
  uart.baud = MCPROTOCOL_EXAMPLE_PLC_BAUD;
  uart.format = SERIAL_8E1;
  uart.rx_pin = 6;
  uart.tx_pin = 7;
  // Default: externally controlled direction (e.g. auto-direction transceiver).
  // For RS-485 RTS direction, set uart.direction and uart.rts_pin for your wiring.
  const Status status = plc.begin(uart, protocol);
  begun = status.ok();
  if (!status.ok()) completed(nullptr, status);
}

void loop() {
  // 'r' acknowledges a site-specific PLC/line reset, not just a quiet UART.
  // Never send it until an old response can no longer arrive.
  const int command = Serial.read();
  if (command == 'r' && recovery_required && !plc.busy()) {
    const Status status = plc.recover(protocol);
    if (status.ok()) {
      stopped = recovery_required = false;
      next_read = millis();
      Serial.println("reconnecting");
    } else completed(nullptr, status);
  }
  plc.update(); // Nonblocking TX/RX; callbacks run here, never in an ISR.
  if (!stopped && !plc.busy() &&
      static_cast<std::int32_t>(millis() - next_read) >= 0) {
    const Status status = plc.async_read_words({DeviceCode::D, 100}, words, completed);
    if (!status.ok()) completed(nullptr, status); // Admission failure has no callback.
  }
  delay(1);
}
