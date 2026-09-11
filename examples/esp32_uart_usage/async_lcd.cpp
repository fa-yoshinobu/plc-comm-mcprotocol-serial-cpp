#include <M5StamPLC.h>
#include "common.hpp"

// 非同期の受信先・結果は、要求完了まで寿命が続くグローバル領域に置きます。
std::uint16_t value = 0;
Status result {};
bool completed = false;
std::uint32_t next_read = 0, button_count = 0;

void on_complete(void*, Status status) {
  result = status;
  completed = true; // コールバック内では次の要求を開始しません。
}

void setup() {
  auto config = M5StamPLC.config();
  config.enableModbusSlave = false; // UART1はMC通信用
  M5StamPLC.config(config);
  M5StamPLC.begin();
  M5StamPLC.Display.setRotation(1);
  M5StamPLC.Display.setTextSize(2);
  M5StamPLC.Display.fillScreen(TFT_BLACK);
  begin_example();
  if (!ready) M5StamPLC.Display.println("INIT ERROR");
}

void loop() {
  M5StamPLC.update(); // 通信待ち中・通信エラー後もボタン処理を継続
  if (M5StamPLC.BtnA.wasPressed()) {
    ++button_count;
    M5StamPLC.Display.fillRect(0, 90, 240, 30, TFT_BLACK);
    M5StamPLC.Display.setCursor(0, 90);
    M5StamPLC.Display.printf("Button A: %lu", static_cast<unsigned long>(button_count));
  }
  plc.update(); // 送受信・期限の評価を少しずつ進める
  if (completed) {
    completed = false;
    M5StamPLC.Display.fillRect(0, 0, 240, 80, TFT_BLACK);
    M5StamPLC.Display.setCursor(0, 0);
    if (check(result)) {
      M5StamPLC.Display.printf("D100: %u", value);
      Serial.printf("D100=%u\n", value);
    } else {
      M5StamPLC.Display.println("COMM ERROR"); // 古い値を消す
    }
    next_read = millis() + 1000;
  }
  // millis()の周回にも対応。delay(1000)でアプリ全体を止めません。
  if (ready && !plc.busy() && static_cast<std::int32_t>(millis() - next_read) >= 0) {
    check(plc.async_read_words({DeviceCode::D, 100}, {&value, 1}, on_complete));
    if (!ready) {
      M5StamPLC.Display.fillRect(0, 0, 240, 80, TFT_BLACK);
      M5StamPLC.Display.setCursor(0, 0);
      M5StamPLC.Display.println("START ERROR");
    }
  }
  delay(1); // RTOSへ実行機会を渡す
}
