#pragma once

#include <cstddef>
#include <cstdint>

#include "mcprotocol/serial/status.hpp"
#include "mcprotocol/serial/span_compat.hpp"
#include "mcprotocol/serial/string_view_compat.hpp"

namespace mcprotocol::serial {

/// \brief Host-side serial-port configuration used by `PosixSerialPort`.
///
/// `device_path` accepts `/dev/...` style paths on POSIX systems and `COM3` or `\\.\COM10`
/// style names on Windows.
struct PosixSerialConfig {
  std::string_view device_path {};
  std::uint32_t baud_rate = 9600;
  std::uint8_t data_bits = 8;
  std::uint8_t stop_bits = 1;
  char parity = 'N';
  bool rts_cts = false;
};

/// \brief Minimal blocking host-side serial-port wrapper used by the CLI tools.
///
/// This class is not required on MCU targets. It exists so the same request/response codec and
/// client logic can be exercised from host-side validation tools.
class PosixSerialPort {
 public:
  PosixSerialPort() = default;
  ~PosixSerialPort();

  PosixSerialPort(const PosixSerialPort&) = delete;
  PosixSerialPort& operator=(const PosixSerialPort&) = delete;

  /// \brief Opens and configures the serial port.
  [[nodiscard]] Status open(const PosixSerialConfig& config) noexcept;
  /// \brief Closes the serial port if it is open.
  void close() noexcept;

  /// \brief Returns whether the serial port is currently open.
  [[nodiscard]] bool is_open() const noexcept;
  /// \brief Returns the native handle value, or `-1` when closed.
  [[nodiscard]] std::intptr_t native_handle() const noexcept;

  /// \brief Writes the entire byte range before returning.
  [[nodiscard]] Status write_all(std::span<const std::byte> bytes) noexcept;
  /// \brief Reads up to `buffer.size()` bytes with a timeout.
  [[nodiscard]] Status read_some(
      std::span<std::byte> buffer,
      int timeout_ms,
      std::size_t& out_size) noexcept;
  /// \brief Drops unread RX data that is already buffered by the driver.
  [[nodiscard]] Status flush_rx() noexcept;
  /// \brief Waits until queued TX data has physically drained.
  [[nodiscard]] Status drain_tx() noexcept;
  /// \brief Sets the RTS line when the underlying driver supports it.
  [[nodiscard]] Status set_rts(bool enabled) noexcept;

 private:
  std::intptr_t fd_ = -1;
};

}  // namespace mcprotocol::serial
