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

void completed(void*, Status status) {
  if (!status.ok()) {
    Serial.printf("read failed: %s (PLC=%04X)\n", status.message,
        static_cast<unsigned>(status.plc_error_code));
    stopped = true;
    Serial.println("Check PLC/line, exclude old replies, then RESET. See README.");
    return;
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
  if (!status.ok()) completed(nullptr, status);
}

void loop() {
  plc.update(); // Nonblocking TX/RX; callbacks run here, never in an ISR.
  if (!stopped && !plc.busy() &&
      static_cast<std::int32_t>(millis() - next_read) >= 0) {
    const Status status = plc.async_read_words({DeviceCode::D, 100}, words, completed);
    if (!status.ok()) completed(nullptr, status); // Admission failure has no callback.
  }
  delay(1);
}
