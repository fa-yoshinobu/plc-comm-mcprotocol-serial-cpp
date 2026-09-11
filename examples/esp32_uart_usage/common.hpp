#pragma once
#include <mcprotocol_serial_arduino_esp32.hpp>

using namespace mcprotocol::serial;

// 全例でUART1を専有します。Serial1やModbusと併用しないでください。
Esp32UartClient plc(1);
bool ready = false;

// エラー時は通信を止めます。他の画面・入出力処理は続けられます。
// 遅延応答との混同を避けるため、自動再送・自動復旧は行いません。
bool check(Status status) {
  if (status.ok()) return true;
  Serial.printf("ERROR: %s (status=%u, PLC=%04X)\n", status.message,
      static_cast<unsigned>(status.code), static_cast<unsigned>(status.plc_error_code));
  Serial.println("Check connection/settings, then RESET.");
  ready = false;
  return false;
}

void begin_example() {
  Serial.begin(115200);
  delay(1500);
  Esp32UartConfig uart;
  uart.baud = 19200;
  uart.format = SERIAL_8E1;
  uart.rx_pin = 39; // STAMPLC PWR-485。別基板では配線に合わせて変更。
  uart.tx_pin = 0;
  uart.direction = Esp32Direction::Rs485Rts;
  uart.rts_pin = 46;
  const auto protocol = ProtocolConfig::c4_binary(
      PlcProfile::MelsecIqF, SumCheckMode::Enabled,
      RouteConfig{HostStationRoute{}}, TimeoutConfig{3000, 250});
  ready = check(plc.begin(uart, protocol));
  Serial.println("Read example. Send lowercase w to execute the documented writes once.");
}
