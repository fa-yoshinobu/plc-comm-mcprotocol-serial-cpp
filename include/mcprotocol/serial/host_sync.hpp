#pragma once

#include "mcprotocol/serial/compat/array.hpp"
#include "mcprotocol/serial/compat/cstddef.hpp"
#include "mcprotocol/serial/compat/cstdint.hpp"

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
/// State-changing methods return `OperationOutcomeUnknown` whenever transmission may have begun
/// but the PLC result cannot be confirmed. They are not retried automatically.
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
  ///
  /// Both the conflict policy and clear scope are mandatory. If transmission starts but the
  /// response cannot be confirmed, this returns `StatusCode::OperationOutcomeUnknown`.
  [[nodiscard]] Status remote_run(
      RemoteOperationMode mode,
      RemoteRunClearMode clear_mode) noexcept;

  /// \brief Issues remote STOP (`1002`) synchronously.
  [[nodiscard]] Status remote_stop() noexcept;

  /// \brief Issues remote PAUSE (`1003`) synchronously.
  ///
  /// The conflict policy is mandatory. If transmission starts but the response cannot be
  /// confirmed, this returns `StatusCode::OperationOutcomeUnknown`.
  [[nodiscard]] Status remote_pause(RemoteOperationMode mode) noexcept;

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
  /// Completion means the request bytes were transmitted successfully. It does not confirm the
  /// resulting PLC reset state.
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

  /// \brief Switches C24 operation mode / transmission settings synchronously (`1612`).
  [[nodiscard]] Status switch_serial_module_mode(
      const SerialModuleModeSwitchRequest& request) noexcept;

  /// \brief Initializes C24 format-5 transmission sequence synchronously (`1615`).
  [[nodiscard]] Status initialize_c24_transmission_sequence() noexcept;

  /// \brief Reads contiguous words synchronously from a string address such as `D100`.
  [[nodiscard]] Status read_words(
      std::string_view head_device,
      std::uint16_t points,
      mcprotocol::serial::Span<std::uint16_t> out_words) noexcept;

  /// \brief Reads contiguous words synchronously using `out_words.size()` as the point count.
  [[nodiscard]] Status read_words(
      std::string_view head_device,
      mcprotocol::serial::Span<std::uint16_t> out_words) noexcept;

  /// \brief Reads extended file-register words synchronously.
  [[nodiscard]] Status read_extended_file_register_words(
      const ExtendedFileRegisterBatchReadWordsRequest& request,
      mcprotocol::serial::Span<std::uint16_t> out_words) noexcept;

  /// \brief Reads direct extended file-register words synchronously.
  [[nodiscard]] Status direct_read_extended_file_register_words(
      const ExtendedFileRegisterDirectBatchReadWordsRequest& request,
      mcprotocol::serial::Span<std::uint16_t> out_words) noexcept;

  /// \brief Reads contiguous bits synchronously from a string address such as `M100`.
  [[nodiscard]] Status read_bits(
      std::string_view head_device,
      std::uint16_t points,
      mcprotocol::serial::Span<BitValue> out_bits) noexcept;

  /// \brief Reads contiguous bits synchronously using `out_bits.size()` as the point count.
  [[nodiscard]] Status read_bits(
      std::string_view head_device,
      mcprotocol::serial::Span<BitValue> out_bits) noexcept;

  /// \brief Reads contiguous `Jn\\...` link-direct words synchronously.
  [[nodiscard]] Status read_link_direct_words(
      std::string_view head_device,
      std::uint16_t points,
      mcprotocol::serial::Span<std::uint16_t> out_words) noexcept;

  /// \brief Reads contiguous `Jn\\...` link-direct bits synchronously.
  [[nodiscard]] Status read_link_direct_bits(
      std::string_view head_device,
      std::uint16_t points,
      mcprotocol::serial::Span<BitValue> out_bits) noexcept;

  /// \brief Reads native-qualified `Un\\Gn` or `Un\\HGn` words.
  ///
  /// Use this for profiles whose qualified access route is native device access (`0401`).
  /// The `0601` helper route is profile/target-specific and may be rejected.
  [[nodiscard]] Status read_native_qualified_words(
      std::string_view head_device,
      std::uint16_t points,
      mcprotocol::serial::Span<std::uint16_t> out_words) noexcept;

  /// \brief Reads long timer/counter contact or coil states through the dedicated status-block path.
  ///
  /// `LTS`/`LTC`/`LSTS`/`LSTC` with more than one point are explicitly aggregate reads: one
  /// four-word status-block request is issued per point, in address order. The complete plan is
  /// validated before transmission, the result is non-atomic across PLC scan times, and caller
  /// output is changed only after every internal request succeeds. `LCS`/`LCC` use one direct bit
  /// request and are not split.
  [[nodiscard]] Status read_long_state_bits(
      std::string_view head_device,
      std::uint16_t points,
      mcprotocol::serial::Span<BitValue> out_bits) noexcept;

  /// \brief Reads long timer/counter states using `out_bits.size()` as the point count.
  [[nodiscard]] Status read_long_state_bits(
      std::string_view head_device,
      mcprotocol::serial::Span<BitValue> out_bits) noexcept;

  /// \brief Writes contiguous words synchronously to a string address such as `D100`.
  [[nodiscard]] Status write_words(
      std::string_view head_device,
      mcprotocol::serial::Span<const std::uint16_t> words) noexcept;

  /// \brief Writes extended file-register words synchronously.
  [[nodiscard]] Status write_extended_file_register_words(
      const ExtendedFileRegisterBatchWriteWordsRequest& request) noexcept;

  /// \brief Writes direct extended file-register words synchronously.
  [[nodiscard]] Status direct_write_extended_file_register_words(
      const ExtendedFileRegisterDirectBatchWriteWordsRequest& request) noexcept;

  /// \brief Writes contiguous bits synchronously to a string address such as `M100`.
  [[nodiscard]] Status write_bits(
      std::string_view head_device,
      mcprotocol::serial::Span<const BitValue> bits) noexcept;

  /// \brief Writes contiguous `Jn\\...` link-direct words synchronously.
  [[nodiscard]] Status write_link_direct_words(
      std::string_view head_device,
      mcprotocol::serial::Span<const std::uint16_t> words) noexcept;

  /// \brief Writes contiguous `Jn\\...` link-direct bits synchronously.
  [[nodiscard]] Status write_link_direct_bits(
      std::string_view head_device,
      mcprotocol::serial::Span<const BitValue> bits) noexcept;

  /// \brief Writes native-qualified `Un\\Gn` or `Un\\HGn` words.
  ///
  /// Use this for profiles whose qualified access route is native device access (`1401`).
  /// The `1601` helper route is profile/target-specific and may be rejected.
  [[nodiscard]] Status write_native_qualified_words(
      std::string_view head_device,
      mcprotocol::serial::Span<const std::uint16_t> words) noexcept;

  /// \brief Reads sparse Word and DWord items synchronously from explicit-width specs.
  [[nodiscard]] Status random_read(
      mcprotocol::serial::Span<const highlevel::RandomReadWordSpec> word_items,
      mcprotocol::serial::Span<const highlevel::RandomReadDWordSpec> dword_items,
      mcprotocol::serial::Span<std::uint16_t> out_words,
      mcprotocol::serial::Span<std::uint32_t> out_dwords) noexcept;

  /// \brief Reads one sparse Word item synchronously from a string address.
  [[nodiscard]] Status random_read_word(
      std::string_view device,
      std::uint16_t& out_value) noexcept;

  /// \brief Reads one sparse DWord item synchronously from a string address.
  [[nodiscard]] Status random_read_dword(
      std::string_view device,
      std::uint32_t& out_value) noexcept;

  /// \brief Writes sparse Word items synchronously from string-address specs.
  ///
  /// Each spec requires an explicit value. A result that cannot be confirmed after transmission is
  /// reported as `StatusCode::OperationOutcomeUnknown` and is never retried automatically.
  [[nodiscard]] Status random_write_words(
      mcprotocol::serial::Span<const highlevel::RandomWriteWordSpec> items) noexcept;

  /// \brief Writes sparse DWord items synchronously from string-address specs.
  ///
  /// Each spec requires an explicit value. A result that cannot be confirmed after transmission is
  /// reported as `StatusCode::OperationOutcomeUnknown` and is never retried automatically.
  [[nodiscard]] Status random_write_dwords(
      mcprotocol::serial::Span<const highlevel::RandomWriteDWordSpec> items) noexcept;

  /// \brief Writes extended file-register words randomly.
  [[nodiscard]] Status random_write_extended_file_register_words(
      mcprotocol::serial::Span<const ExtendedFileRegisterRandomWriteWordItem> items) noexcept;

  /// \brief Writes one sparse Word item synchronously from a string address.
  [[nodiscard]] Status random_write_word(
      std::string_view device,
      std::uint16_t value) noexcept;

  /// \brief Writes one sparse DWord item synchronously from a string address.
  [[nodiscard]] Status random_write_dword(
      std::string_view device,
      std::uint32_t value) noexcept;

  /// \brief Writes sparse bit items synchronously from string-address specs.
  ///
  /// Each spec requires an explicit `Off` or `On`. A result that cannot be confirmed after
  /// transmission is `StatusCode::OperationOutcomeUnknown` and is never retried automatically.
  [[nodiscard]] Status random_write_bits(
      mcprotocol::serial::Span<const highlevel::RandomWriteBitSpec> items) noexcept;

  /// \brief Writes one sparse bit item synchronously from a string address.
  [[nodiscard]] Status random_write_bit(
      std::string_view device,
      BitValue value) noexcept;

  /// \brief Registers sparse Word and DWord monitor items from explicit-width specs.
  [[nodiscard]] Status register_monitor(
      mcprotocol::serial::Span<const highlevel::RandomReadWordSpec> word_items,
      mcprotocol::serial::Span<const highlevel::RandomReadDWordSpec> dword_items) noexcept;

  /// \brief Registers one sparse Word monitor item synchronously.
  [[nodiscard]] Status register_monitor_word(std::string_view device) noexcept;

  /// \brief Registers one sparse DWord monitor item synchronously.
  [[nodiscard]] Status register_monitor_dword(std::string_view device) noexcept;

  /// \brief Registers extended file-register monitor data synchronously.
  [[nodiscard]] Status register_extended_file_register_monitor(
      const ExtendedFileRegisterMonitorRegistration& request) noexcept;

  /// \brief Reads the most recently registered Word and DWord monitor items synchronously.
  [[nodiscard]] Status read_monitor(
      mcprotocol::serial::Span<std::uint16_t> out_words,
      mcprotocol::serial::Span<std::uint32_t> out_dwords) noexcept;

  /// \brief Reads the most recently registered extended file-register monitor items synchronously.
  [[nodiscard]] Status read_extended_file_register_monitor(
      mcprotocol::serial::Span<std::uint16_t> out_words) noexcept;

 private:
  struct CompletionState {
    bool done = false;
    Status status {};
  };

  static void on_request_complete(void* user, Status status) noexcept;
  [[nodiscard]] Status run_until_complete() noexcept;

  PosixSerialPort port_ {};
  MelsecSerialClient client_ {};
  ProtocolConfig protocol_config_ = ProtocolConfig::unconfigured_for_storage();
  std::array<mcprotocol::serial::Byte, kMaxResponseFrameBytes> rx_buffer_ {};
  CompletionState completion_ {};
};

}  // namespace mcprotocol::serial
