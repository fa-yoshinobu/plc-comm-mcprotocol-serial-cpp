#pragma once

#include "mcprotocol/serial/compat/cstddef.hpp"
#include "mcprotocol/serial/compat/cstdint.hpp"

#include "mcprotocol/serial/status.hpp"
#include "mcprotocol/serial/span_compat.hpp"
#include "mcprotocol/serial/string_view_compat.hpp"
#include "mcprotocol/serial/types.hpp"

namespace mcprotocol::serial {

/// \brief Explicit parity selection for a host serial port.
enum class SerialParity : std::uint8_t {
  None,
  Even,
  Odd,
};

/// \brief Explicit hardware flow-control selection for a host serial port.
enum class HardwareFlowControl : std::uint8_t {
  None,
  RtsCts,
};

/// \brief Host-side serial-port configuration used by `PosixSerialPort`.
///
/// Every constructor argument is required. `device_path` accepts `/dev/...` style paths on POSIX
/// systems and `COM3` or `\\.\COM10` style names on Windows. The referenced device-path text must
/// remain alive while the configuration is used.
struct PosixSerialConfig {
  constexpr PosixSerialConfig(
      std::string_view device_path_value,
      std::uint32_t baud_rate_value,
      std::uint32_t data_bits_value,
      std::uint32_t stop_bits_value,
      SerialParity parity_value,
      HardwareFlowControl hardware_flow_control_value) noexcept
      : device_path(device_path_value),
        baud_rate(baud_rate_value),
        data_bits(data_bits_value),
        stop_bits(stop_bits_value),
        parity(parity_value),
        hardware_flow_control(hardware_flow_control_value) {}

  PosixSerialConfig() = delete;

  std::string_view device_path;
  std::uint32_t baud_rate;
  std::uint32_t data_bits;
  std::uint32_t stop_bits;
  SerialParity parity;
  HardwareFlowControl hardware_flow_control;
};

[[nodiscard]] inline Status validate_serial_config(const PosixSerialConfig& config) noexcept {
  if (config.device_path.empty()) {
    return make_status(StatusCode::InvalidArgument, "Device path must not be empty");
  }
  if (config.device_path.find('\0') != std::string_view::npos) {
    return make_status(StatusCode::InvalidArgument, "Device path must not contain embedded NUL");
  }
  if (config.baud_rate == 0U) {
    return make_status(StatusCode::InvalidArgument, "Baud rate must be greater than zero");
  }
  if (config.data_bits != 7U && config.data_bits != 8U) {
    return make_status(StatusCode::InvalidArgument, "MC protocol data bits must be 7 or 8");
  }
  if (config.stop_bits != 1U && config.stop_bits != 2U) {
    return make_status(StatusCode::InvalidArgument, "Stop bits must be 1 or 2");
  }
  switch (config.parity) {
    case SerialParity::None:
    case SerialParity::Even:
    case SerialParity::Odd:
      break;
    default:
      return make_status(StatusCode::InvalidArgument, "Unknown serial parity");
  }
  switch (config.hardware_flow_control) {
    case HardwareFlowControl::None:
    case HardwareFlowControl::RtsCts:
      break;
    default:
      return make_status(StatusCode::InvalidArgument, "Unknown hardware flow control");
  }
  return ok_status();
}

[[nodiscard]] inline Status validate_mc_serial_config(
    const PosixSerialConfig& serial_config,
    const ProtocolConfig& protocol_config) noexcept {
  const Status serial_status = validate_serial_config(serial_config);
  if (!serial_status.ok()) {
    return serial_status;
  }
  if (protocol_config.code_mode == CodeMode::Binary && serial_config.data_bits != 8U) {
    return make_status(StatusCode::InvalidArgument, "Binary MC protocol requires 8 data bits");
  }
  if (protocol_config.code_mode == CodeMode::Ascii &&
      serial_config.data_bits != 7U &&
      serial_config.data_bits != 8U) {
    return make_status(StatusCode::InvalidArgument, "ASCII MC protocol requires 7 or 8 data bits");
  }
  return ok_status();
}

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
