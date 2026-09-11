#include "test_assert.hpp"
#include <Arduino.h>
#include <algorithm>
#include <vector>
#include <cstring>
#define HEX 16
class HardwareSerial {
 public:
  bool partial = false, ended = false;
  std::vector<char> rx;
  void begin(unsigned, unsigned = 0) {}
  void end() { ended = true; }
  std::size_t write(const std::uint8_t*, std::size_t size) { return partial ? size - 1 : size; }
  int available() { return static_cast<int>(rx.size()); }
  std::size_t readBytes(char* out, std::size_t size) {
    size = std::min(size, rx.size());
    std::memcpy(out, rx.data(), size);
    rx.erase(rx.begin(), rx.begin() + size);
    return size;
  }
  template<class... Args> void print(Args...) {}
  template<class... Args> void println(Args...) {}
} Serial, Serial1;
#include "../examples/platformio_rpipico_arduino_uart/platformio_rpipico_arduino_uart.cpp"

int main() {
  using namespace mcprotocol::serial;
  setup();
  fake_esp32::now = 0xffffff00U;
  on_request_complete(&g_app, ok_status());
  start_read_if_due(millis());
  assert(!g_app.client.busy());
  fake_esp32::now = g_app.next_request_ms;
  start_read_if_due(millis());
  fake_uart_flags = UART_UARTFR_BUSY_BITS;
  pump_uart_tx(millis());
  assert(g_app.tx_started && !g_app.tx_sent);
  // Encode an immediate response while physical TX is still marked busy.
  const std::uint8_t data[] = {'0','0','0','1','0','0','0','2','0','0','0','3','0','0','0','4'};
  std::uint8_t response[128] {};
  std::size_t count = 0;
  assert(FrameCodec::encode_success_response(make_protocol(), data, response, count).ok());
  Serial1.rx.assign(reinterpret_cast<char*>(response), reinterpret_cast<char*>(response) + count);
  pump_uart_rx();
  assert(Serial1.rx.size() == count); // Early reply is retained.
  fake_uart_flags = 0;
  pump_uart_tx(millis());
  pump_uart_rx();
  assert(g_app.completion_status.ok() && g_app.out_words[3] == 4);
  fake_esp32::now = g_app.next_request_ms;
  start_read_if_due(millis());
  Serial1.partial = true;
  pump_uart_tx(millis());
  assert(Serial1.ended && g_app.stopped && !g_app.client.busy());
  assert(g_app.completion_status.code == StatusCode::Transport);
  // Fresh test transport for a physical TX deadline, not a live recovery.
  assert(g_app.client.configure(make_protocol()).ok());
  g_app.stopped = false;
  Serial1.partial = Serial1.ended = false;
  fake_esp32::now = g_app.next_request_ms;
  start_read_if_due(millis());
  fake_uart_flags = UART_UARTFR_BUSY_BITS;
  pump_uart_tx(millis());
  fake_esp32::now = g_app.client.transaction_deadline_ms();
  pump_uart_tx(millis());
  assert(Serial1.ended && g_app.stopped && !g_app.client.busy());
  assert(g_app.completion_status.code == StatusCode::Timeout);
  fake_esp32::now += 10000;
  start_read_if_due(millis());
  assert(!g_app.client.busy());
}
