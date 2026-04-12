#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "mcprotocol/serial/client.hpp"
#include "mcprotocol/serial/high_level.hpp"
#include "mcprotocol/serial/posix_serial.hpp"
#include "mcprotocol/serial/string_view_compat.hpp"

/// \file host_sync.hpp
/// \brief Blocking host-side wrapper that drives `MelsecSerialClient` over `PosixSerialPort`.
///
/// This header is intended for bring-up tools, validation binaries, and small host utilities. MCU
/// firmware normally uses `MelsecSerialClient` directly instead of this blocking wrapper.

namespace mcprotocol::serial {

/// \brief Host-side synchronous convenience wrapper built on `PosixSerialPort` and
/// `MelsecSerialClient`.
///
/// This class is intentionally small:
///
/// - it keeps the existing low-level client unchanged
/// - it opens a host-side serial port
/// - it runs one request synchronously from TX to completion
/// - it exposes string-address helpers for common contiguous, sparse random, and monitor operations
///
/// Use it on Windows or POSIX hosts when you want a simpler bring-up path than manually driving
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

  /// \brief Issues remote RUN (`1001`) synchronously.
  [[nodiscard]] Status remote_run(
      RemoteOperationMode mode = RemoteOperationMode::DoNotExecuteForcibly,
      RemoteRunClearMode clear_mode = RemoteRunClearMode::DoNotClear) noexcept;

  /// \brief Issues remote STOP (`1002`) synchronously.
  [[nodiscard]] Status remote_stop() noexcept;

  /// \brief Issues remote PAUSE (`1003`) synchronously.
  [[nodiscard]] Status remote_pause(
      RemoteOperationMode mode = RemoteOperationMode::DoNotExecuteForcibly) noexcept;

  /// \brief Issues remote latch clear (`1005`) synchronously.
  [[nodiscard]] Status remote_latch_clear() noexcept;

  /// \brief Unlocks remote-password-protected access (`1630`) synchronously.
  [[nodiscard]] Status unlock_remote_password(std::string_view remote_password) noexcept;

  /// \brief Locks remote-password-protected access (`1631`) synchronously.
  [[nodiscard]] Status lock_remote_password(std::string_view remote_password) noexcept;

  /// \brief Clears serial/C24 error information (`1617`) synchronously.
  [[nodiscard]] Status clear_error_information() noexcept;

  /// \brief Issues remote RESET (`1006`) synchronously.
  ///
  /// Some targets reset before returning a response. In that case the underlying client treats a
  /// pure response-timeout with no received bytes as success for this operation.
  [[nodiscard]] Status remote_reset() noexcept;

  /// \brief Reads user-frame registration data synchronously (`0610`).
  [[nodiscard]] Status read_user_frame(
      const UserFrameReadRequest& request,
      UserFrameRegistrationData& out_data) noexcept;

  /// \brief Writes user-frame registration data synchronously (`1610`, subcommand `0000`).
  [[nodiscard]] Status write_user_frame(
      const UserFrameWriteRequest& request) noexcept;

  /// \brief Deletes user-frame registration data synchronously (`1610`, subcommand `0001`).
  [[nodiscard]] Status delete_user_frame(
      const UserFrameDeleteRequest& request) noexcept;

  /// \brief Controls C24 global signal ON/OFF synchronously (`1618`).
  [[nodiscard]] Status control_global_signal(
      const GlobalSignalControlRequest& request) noexcept;

  /// \brief Initializes C24 format-5 transmission sequence synchronously (`1615`).
  [[nodiscard]] Status initialize_c24_transmission_sequence() noexcept;

  /// \brief Deregisters programmable-controller CPU monitoring synchronously (`0631`).
  [[nodiscard]] Status deregister_cpu_monitoring() noexcept;

  /// \brief Reads contiguous words synchronously from a string address such as `D100`.
  [[nodiscard]] Status read_words(
      std::string_view head_device,
      std::uint16_t points,
      std::span<std::uint16_t> out_words) noexcept;

  /// \brief Reads contiguous words synchronously using `out_words.size()` as the point count.
  [[nodiscard]] Status read_words(
      std::string_view head_device,
      std::span<std::uint16_t> out_words) noexcept;

  /// \brief Reads extended file-register words synchronously.
  [[nodiscard]] Status read_extended_file_register_words(
      const ExtendedFileRegisterBatchReadWordsRequest& request,
      std::span<std::uint16_t> out_words) noexcept;

  /// \brief Reads direct extended file-register words synchronously.
  [[nodiscard]] Status direct_read_extended_file_register_words(
      const ExtendedFileRegisterDirectBatchReadWordsRequest& request,
      std::span<std::uint16_t> out_words) noexcept;

  /// \brief Reads contiguous bits synchronously from a string address such as `M100`.
  [[nodiscard]] Status read_bits(
      std::string_view head_device,
      std::uint16_t points,
      std::span<BitValue> out_bits) noexcept;

  /// \brief Reads contiguous bits synchronously using `out_bits.size()` as the point count.
  [[nodiscard]] Status read_bits(
      std::string_view head_device,
      std::span<BitValue> out_bits) noexcept;

  /// \brief Writes contiguous words synchronously to a string address such as `D100`.
  [[nodiscard]] Status write_words(
      std::string_view head_device,
      std::span<const std::uint16_t> words) noexcept;

  /// \brief Writes extended file-register words synchronously.
  [[nodiscard]] Status write_extended_file_register_words(
      const ExtendedFileRegisterBatchWriteWordsRequest& request) noexcept;

  /// \brief Writes direct extended file-register words synchronously.
  [[nodiscard]] Status direct_write_extended_file_register_words(
      const ExtendedFileRegisterDirectBatchWriteWordsRequest& request) noexcept;

  /// \brief Writes contiguous bits synchronously to a string address such as `M100`.
  [[nodiscard]] Status write_bits(
      std::string_view head_device,
      std::span<const BitValue> bits) noexcept;

  /// \brief Reads sparse word/dword items synchronously from string-address specs.
  [[nodiscard]] Status random_read(
      std::span<const highlevel::RandomReadSpec> items,
      std::span<std::uint32_t> out_values) noexcept;

  /// \brief Reads one sparse word/dword item synchronously from a string address.
  [[nodiscard]] Status random_read(
      std::string_view device,
      std::uint32_t& out_value,
      bool double_word = false) noexcept;

  /// \brief Writes sparse word/dword items synchronously from string-address specs.
  [[nodiscard]] Status random_write_words(
      std::span<const highlevel::RandomWriteWordSpec> items) noexcept;

  /// \brief Writes extended file-register words randomly.
  [[nodiscard]] Status random_write_extended_file_register_words(
      std::span<const ExtendedFileRegisterRandomWriteWordItem> items) noexcept;

  /// \brief Writes one sparse word/dword item synchronously from a string address.
  [[nodiscard]] Status random_write_word(
      std::string_view device,
      std::uint32_t value,
      bool double_word = false) noexcept;

  /// \brief Writes sparse bit items synchronously from string-address specs.
  [[nodiscard]] Status random_write_bits(
      std::span<const highlevel::RandomWriteBitSpec> items) noexcept;

  /// \brief Writes one sparse bit item synchronously from a string address.
  [[nodiscard]] Status random_write_bit(
      std::string_view device,
      BitValue value) noexcept;

  /// \brief Registers a sparse monitor synchronously from string-address specs.
  [[nodiscard]] Status register_monitor(
      std::span<const highlevel::RandomReadSpec> items) noexcept;

  /// \brief Registers one sparse monitor item synchronously from a string address.
  [[nodiscard]] Status register_monitor(
      std::string_view device,
      bool double_word = false) noexcept;

  /// \brief Registers extended file-register monitor data synchronously.
  [[nodiscard]] Status register_extended_file_register_monitor(
      const ExtendedFileRegisterMonitorRegistration& request) noexcept;

  /// \brief Reads the most recently registered monitor items synchronously.
  [[nodiscard]] Status read_monitor(std::span<std::uint32_t> out_values) noexcept;

  /// \brief Reads one previously registered monitor item synchronously.
  [[nodiscard]] Status read_monitor(std::uint32_t& out_value) noexcept;

  /// \brief Reads the most recently registered extended file-register monitor items synchronously.
  [[nodiscard]] Status read_extended_file_register_monitor(
      std::span<std::uint16_t> out_words) noexcept;

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
