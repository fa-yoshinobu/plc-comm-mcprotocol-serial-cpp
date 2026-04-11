#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "mcprotocol/serial/client.hpp"
#include "mcprotocol/serial/high_level.hpp"
#include "mcprotocol/serial/posix_serial.hpp"
#include "mcprotocol/serial/string_view_compat.hpp"

namespace mcprotocol::serial {

/// \brief Host-side synchronous convenience wrapper built on `PosixSerialPort` and
/// `MelsecSerialClient`.
///
/// This class is intentionally small:
///
/// - it keeps the existing low-level client unchanged
/// - it opens a POSIX serial port
/// - it runs one request synchronously from TX to completion
/// - it exposes string-address helpers for the common contiguous operations
///
/// Use it on Linux or other POSIX hosts when you want a simpler bring-up path than manually driving
/// `pending_tx_frame()`, `notify_tx_complete()`, `on_rx_bytes()`, and `poll()`.
class PosixSyncClient {
 public:
  PosixSyncClient() = default;

  PosixSyncClient(const PosixSyncClient&) = delete;
  PosixSyncClient& operator=(const PosixSyncClient&) = delete;

  /// \brief Opens the serial port and configures the underlying MC protocol client.
  [[nodiscard]] Status open(
      const PosixSerialConfig& serial_config,
      const ProtocolConfig& protocol_config) noexcept;

  /// \brief Closes the serial port and clears any in-flight request state.
  void close() noexcept;

  /// \brief Returns whether the underlying serial port is open.
  [[nodiscard]] bool is_open() const noexcept;

  /// \brief Returns the currently configured protocol settings.
  [[nodiscard]] const ProtocolConfig& protocol_config() const noexcept;

  /// \brief Reads the remote CPU model synchronously.
  [[nodiscard]] Status read_cpu_model(CpuModelInfo& out_info) noexcept;

  /// \brief Reads contiguous words synchronously from a string address such as `D100`.
  [[nodiscard]] Status read_words(
      std::string_view head_device,
      std::uint16_t points,
      std::span<std::uint16_t> out_words) noexcept;

  /// \brief Reads contiguous bits synchronously from a string address such as `M100`.
  [[nodiscard]] Status read_bits(
      std::string_view head_device,
      std::uint16_t points,
      std::span<BitValue> out_bits) noexcept;

  /// \brief Writes contiguous words synchronously to a string address such as `D100`.
  [[nodiscard]] Status write_words(
      std::string_view head_device,
      std::span<const std::uint16_t> words) noexcept;

  /// \brief Writes contiguous bits synchronously to a string address such as `M100`.
  [[nodiscard]] Status write_bits(
      std::string_view head_device,
      std::span<const BitValue> bits) noexcept;

 private:
  struct CompletionState {
    bool done = false;
    Status status {};
  };

  static void on_request_complete(void* user, Status status) noexcept;
  [[nodiscard]] Status run_until_complete() noexcept;

  PosixSerialPort port_ {};
  MelsecSerialClient client_ {};
  ProtocolConfig protocol_config_ {};
  std::array<std::byte, kMaxResponseFrameBytes> rx_buffer_ {};
  CompletionState completion_ {};
};

}  // namespace mcprotocol::serial
