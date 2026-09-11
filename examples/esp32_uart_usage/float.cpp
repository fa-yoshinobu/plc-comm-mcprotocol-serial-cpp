#include <cstring>
#include <limits>
#include "common.hpp"

static_assert(sizeof(float) == sizeof(std::uint32_t)
    && std::numeric_limits<float>::is_iec559, "IEEE754 binary32 required");

void setup() { begin_example(); }
void loop() {
  if (!ready) { delay(1); return; }
  // PLC側もD100..D101をREALとして使用します。整数1234とは別のビット列です。
  const RandomReadDWordItem items[] = {{{DeviceCode::D, 100}}};
  std::uint32_t raw = 0;
  if (!check(plc.random_read(RandomReadRequest({}, items), {}, {&raw, 1}))) return;
  float value = 0;
  // 数値キャストではなくビット列をコピーします。型の別名参照も避けます。
  std::memcpy(&value, &raw, sizeof(value));
  Serial.printf("D100..101 FLOAT=%.3f hex=%08lX\n",
      static_cast<double>(value), static_cast<unsigned long>(raw));
  if (Serial.read() == 'w') {
    const float setpoint = 12.5f; // D100=0000、D101=4148になる値。
    std::memcpy(&raw, &setpoint, sizeof(raw));
    const RandomWriteDWordItem data[] = {{{DeviceCode::D, 100}, raw}};
    if (!check(plc.random_write_words({}, data))) return;
  }
  delay(1000);
}
