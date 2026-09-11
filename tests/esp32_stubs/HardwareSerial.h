#pragma once
#include <Arduino.h>
class HardwareSerial {
 public:
  explicit HardwareSerial(std::uint8_t) {}
  ~HardwareSerial() { ++fake_esp32::destroyed; fake_esp32::installed = false; }
  void setRxBufferSize(std::size_t) {}
  void setTxBufferSize(std::size_t) {}
  void begin(std::uint32_t, std::uint32_t, int, int) {
    fake_esp32::installed = !fake_esp32::init_fails;
  }
  bool setPins(int, int, int, int) { return !fake_esp32::setup_fails; }
  bool setHwFlowCtrlMode(int) { return !fake_esp32::setup_fails; }
  bool setMode(int) { return !fake_esp32::setup_fails; }
};
