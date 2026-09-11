#include "common.hpp"

void setup() { begin_example(); }
void loop() {
  if (!ready) { delay(1); return; }
  // WORDは16bit。D100を1点、D110..D112を連続3点として読みます。
  std::uint16_t single = 0, values[3] {};
  if (!check(plc.read_word({DeviceCode::D, 100}, single))) return;
  if (!check(plc.read_words({DeviceCode::D, 110}, values))) return;
  Serial.printf("D100=%u D110..112=%u,%u,%u\n",
      single, values[0], values[1], values[2]);
  // USBからwを受けた時だけ書き込み。結果は次の読み取りで確認できます。
  if (Serial.read() == 'w') {
    const std::uint16_t data[] = {10, 20, 30};
    if (!check(plc.write_word({DeviceCode::D, 100}, 1234))) return;
    if (!check(plc.write_words({DeviceCode::D, 110}, data))) return;
  }
  delay(1000); // 同期入門例。通信中も他の処理をする場合はasync_lcd.cppへ。
}
