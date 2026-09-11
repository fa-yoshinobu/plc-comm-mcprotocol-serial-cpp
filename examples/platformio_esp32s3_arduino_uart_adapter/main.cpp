#include <mcprotocol_serial_arduino_esp32.hpp>

using namespace mcprotocol::serial;
Esp32UartClient plc(1); // Own UART1; do not also initialize Serial1 or Modbus.
bool initialized = false;

void setup() {
  Serial.begin(115200);
  delay(1500);
  Esp32UartConfig uart;
  uart.baud = 19200;
  uart.format = SERIAL_8E1;
  uart.rx_pin = 39; // STAMPLC PWR-485
  uart.tx_pin = 0;
  uart.direction = Esp32Direction::Rs485Rts;
  uart.rts_pin = 46;
  const auto protocol = ProtocolConfig::c4_binary(
      PlcProfile::MelsecIqF, SumCheckMode::Enabled,
      RouteConfig{HostStationRoute{}}, TimeoutConfig{3000, 250});
  const auto status = plc.begin(uart, protocol);
  initialized = status.ok();
  Serial.println(status.message);
}
void loop() {
  if (!initialized) { delay(1000); return; }
  std::uint16_t value = 0;
  const auto status = plc.read_word({DeviceCode::D, 100}, value);
  if (status.ok()) Serial.printf("D100=%u\n", static_cast<unsigned>(value));
  else {
    Serial.printf("Error: %s / PLC=%04X. Check link, then RESET.\n",
        status.message, static_cast<unsigned>(status.plc_error_code));
    initialized = false; // no automatic retry of an ambiguous transaction
  }
  delay(1000);
}
