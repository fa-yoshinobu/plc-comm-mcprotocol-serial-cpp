#include "common.hpp"

void setup() { begin_example(); }
void loop() {
  if (!ready) { delay(1); return; }
  // DWORDは32bit符号なし整数。D100が下位16bit、D101が上位16bitです。
  // DWORD専用アイテムを使うので、2ワードの結合はコアが行います。
  const RandomReadDWordItem items[] = {{{DeviceCode::D, 100}}};
  std::uint32_t value = 0;
  if (!check(plc.random_read(RandomReadRequest({}, items), {}, {&value, 1}))) return;
  // 符号付き32bitとして表示する場合は、符号を拡張して解釈します。
  const std::int64_t signed_value = value < 0x80000000U
      ? static_cast<std::int64_t>(value) : static_cast<std::int64_t>(value) - 0x100000000LL;
  Serial.printf("D100..101 DWORD=%lu signed=%lld hex=%08lX\n",
      static_cast<unsigned long>(value), static_cast<long long>(signed_value),
      static_cast<unsigned long>(value));
  if (Serial.read() == 'w') {
    const RandomWriteDWordItem data[] = {{{DeviceCode::D, 100}, 123456789U}};
    if (!check(plc.random_write_words({}, data))) return;
  }
  delay(1000);
}
