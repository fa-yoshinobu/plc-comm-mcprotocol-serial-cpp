#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

#include "mcprotocol/serial/status.hpp"

namespace mcprotocol::serial {

struct PosixSerialConfig {
  std::string_view device_path {};
  std::uint32_t baud_rate = 9600;
  std::uint8_t data_bits = 8;
  std::uint8_t stop_bits = 1;
  char parity = 'N';
  bool rts_cts = false;
};

class PosixSerialPort {
 public:
  PosixSerialPort() = default;
  ~PosixSerialPort();

  PosixSerialPort(const PosixSerialPort&) = delete;
  PosixSerialPort& operator=(const PosixSerialPort&) = delete;

  [[nodiscard]] Status open(const PosixSerialConfig& config) noexcept;
  void close() noexcept;

  [[nodiscard]] bool is_open() const noexcept;
  [[nodiscard]] int native_handle() const noexcept;

  [[nodiscard]] Status write_all(std::span<const std::byte> bytes) noexcept;
  [[nodiscard]] Status read_some(
      std::span<std::byte> buffer,
      int timeout_ms,
      std::size_t& out_size) noexcept;
  [[nodiscard]] Status flush_rx() noexcept;
  [[nodiscard]] Status drain_tx() noexcept;
  [[nodiscard]] Status set_rts(bool enabled) noexcept;

 private:
  int fd_ = -1;
};

}  // namespace mcprotocol::serial
