#include "test_assert.hpp"
#include <Arduino.h>

struct Console {
  int command = -1;
  void begin(unsigned) {}
  template<class... Args> void printf(const char*, Args...) {}
  void println(const char*) {}
  int read() { const int result = command; command = -1; return result; }
} Serial;

#if defined(TEST_RECONNECT)
#include "../examples/platformio_esp32c3_arduino_async_polling_reconnect/platformio_esp32c3_arduino_async_polling_reconnect.cpp"
#else
#include "../examples/platformio_esp32c3_arduino_uart/platformio_esp32c3_arduino_uart.cpp"
#endif

int main() {
  setup();
  assert(!stopped);
  fake_esp32::now = 0xffffff00U;
  completed(nullptr, ok_status());
  loop();
  assert(!plc.busy()); // Deadline wraps, but must not fire early.
  fake_esp32::now = next_read;
  loop();
  assert(plc.busy());
  loop(); // Start TX; the stub never supplies an RX response.
  fake_esp32::now += 60000;
  loop();
  assert(stopped && plc.requires_transport_reset());
  for (unsigned i = 0; i < 10; ++i) loop();
  assert(!plc.busy()); // No blind retry after an uncertain transaction.
#if defined(TEST_RECONNECT)
  assert(recovery_required);
  Serial.command = 'x';
  loop();
  assert(stopped);
  // Simulate the operator's explicit acknowledgment; no physical PLC is involved.
  Serial.command = 'r';
  loop();
  assert(!stopped && !recovery_required && plc.busy());
  plc.cancel();
  assert(plc.recover(protocol).ok());
  stopped = recovery_required = false;
  completed(nullptr, make_status(StatusCode::PlcError, "PLC error"));
  assert(!stopped && !recovery_required);
  loop();
  assert(!plc.busy()); // PLC-error retry still respects the interval.
  fake_esp32::now = next_read;
  loop();
  assert(plc.busy());
  plc.cancel();
#endif
}
