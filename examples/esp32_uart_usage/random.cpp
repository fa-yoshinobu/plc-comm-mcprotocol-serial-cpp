#include "common.hpp"

void setup() { begin_example(); }
void loop() {
  if (!ready) { delay(1); return; }
  // 離れた番地を1要求で読みます。WORD出力とDWORD出力は別の配列です。
  const RandomReadWordItem word_items[] = {{{DeviceCode::D, 100}}, {{DeviceCode::D, 200}}};
  const RandomReadDWordItem dword_items[] = {{{DeviceCode::D, 500}}};
  std::uint16_t words[2] {};
  std::uint32_t dwords[1] {};
  if (!check(plc.random_read(RandomReadRequest(word_items, dword_items), words, dwords))) return;
  Serial.printf("D100=%u D200=%u D500..501=%lu\n",
      words[0], words[1], static_cast<unsigned long>(dwords[0]));
  if (Serial.read() == 'w') {
    const RandomWriteWordItem w[] = {{{DeviceCode::D, 100}, 10}, {{DeviceCode::D, 200}, 20}};
    const RandomWriteDWordItem dw[] = {{{DeviceCode::D, 500}, 123456U}};
    if (!check(plc.random_write_words(w, dw))) return;
    // ビットのランダム書き込みは別要求です。上の書き込みと一括確定ではありません。
    const RandomWriteBitItem bits[] = {{{DeviceCode::M, 100}, true}, {{DeviceCode::M, 200}, false}};
    if (!check(plc.random_write_bits(bits))) return;
  }
  delay(1000);
}
