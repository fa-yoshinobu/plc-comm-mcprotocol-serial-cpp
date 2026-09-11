#include "common.hpp"

void setup() { begin_example(); }
void loop() {
  if (!ready) { delay(1); return; }
  // ワード領域2組とビット領域1組をまとめて読みます。
  // ビットブロックのpoints=1は16ビット分（M100..M115）です。
  const MultiBlockReadBlock blocks[] = {
      {{DeviceCode::D, 100}, 2, false}, {{DeviceCode::D, 200}, 3, false},
      {{DeviceCode::M, 100}, 1, true}};
  std::uint16_t words[5] {};
  BitValue bits[16] {};
  MultiBlockReadBlockResult results[3] {};
  if (!check(plc.multi_block_read(MultiBlockReadRequest(blocks), words, bits, results))) return;
  // resultsのオフセットは、ブロック種別に対応する配列内の位置です。
  for (const auto& result : results) {
    Serial.printf("%s%lu:", result.bit_block ? "M" : "D",
        static_cast<unsigned long>(result.head_device.number));
    for (unsigned i = 0; i < result.data_count; ++i) {
      const auto offset = result.data_offset + i;
      Serial.printf(" %u", result.bit_block ? static_cast<unsigned>(bits[offset])
                                          : static_cast<unsigned>(words[offset]));
    }
    Serial.println();
  }
  if (Serial.read() == 'w') {
    const std::uint16_t first[] = {10, 20}, second[] = {30, 40, 50};
    const BitValue flags[16] = {true, false, true}; // 残り13ビットはfalse
    const MultiBlockWriteBlock data[] = {
        {{DeviceCode::D, 100}, 2, Span<const std::uint16_t>(first)},
        {{DeviceCode::D, 200}, 3, Span<const std::uint16_t>(second)},
        {{DeviceCode::M, 100}, 1, Span<const BitValue>(flags)}};
    if (!check(plc.multi_block_write(MultiBlockWriteRequest(data)))) return;
  }
  delay(1000);
}
