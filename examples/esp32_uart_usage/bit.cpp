#include "common.hpp"

void setup() { begin_example(); }
void loop() {
  if (!ready) { delay(1); return; }
  // 個別ビットはbool。連続ビットもbool配列で受け取ります。
  bool single = false, values[3] {};
  if (!check(plc.read_bit({DeviceCode::M, 100}, single))) return;
  if (!check(plc.read_bits({DeviceCode::M, 110}, values))) return;
  Serial.printf("M100=%d M110..112=%d,%d,%d\n",
      single, values[0], values[1], values[2]);
  if (Serial.read() == 'w') {
    const BitValue data[] = {true, false, true};
    if (!check(plc.write_bit({DeviceCode::M, 100}, true))) return;
    if (!check(plc.write_bits({DeviceCode::M, 110}, data))) return;
  }
  delay(1000);
}
