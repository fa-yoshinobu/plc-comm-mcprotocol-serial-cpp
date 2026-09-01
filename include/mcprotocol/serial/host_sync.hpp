#pragma once

#include "mcprotocol/serial/compat/array.hpp"
#include "mcprotocol/serial/compat/cstddef.hpp"
#include "mcprotocol/serial/compat/cstdint.hpp"

#include "mcprotocol/serial/client.hpp"
#include "mcprotocol/serial/high_level.hpp"
#include "mcprotocol/serial/host_serial.hpp"
#include "mcprotocol/serial/string_view_compat.hpp"

/// \file host_sync.hpp
/// \brief Blocking host-side wrapper that drives `MelsecSerialClient` over `HostSerialPort`.
///
/// This header is intended for bring-up tools, validation binaries, and small host utilities. MCU
/// firmware normally uses `MelsecSerialClient` directly instead of this blocking wrapper.

namespace mcprotocol::serial {

/// \brief Host-side synchronous convenience wrapper built on `HostSerialPort` and
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
class HostSyncClient {
 public:
  HostSyncClient() = default;

  HostSyncClient(const HostSyncClient&) = delete;
  HostSyncClient& operator=(const HostSyncClient&) = delete;

  /// \brief Opens the serial port and configures the underlying MC protocol client.
  [[nodiscard]] Status open(
      const HostSerialConfig& serial_config,
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
  [[nodiscard]] Status read_user_frame_registration(
      const UserFrameRegistrationReadRequest& request,
      UserFrameRegistrationData& out_data) noexcept;

  /// \brief Compatibility alias for `read_user_frame_registration`.
  [[nodiscard, deprecated("use read_user_frame_registration")]] Status read_user_frame(
      const UserFrameRegistrationReadRequest& request,
      UserFrameRegistrationData& out_data) noexcept {
    return read_user_frame_registration(request, out_data);
  }

  /// \brief Writes user-frame registration data synchronously (`1610`, subcommand `0000`).
  [[nodiscard]] Status write_user_frame_registration(
      const UserFrameRegistrationWriteRequest& request) noexcept;

  /// \brief Compatibility alias for `write_user_frame_registration`.
  [[nodiscard, deprecated("use write_user_frame_registration")]] Status write_user_frame(
      const UserFrameRegistrationWriteRequest& request) noexcept {
    return write_user_frame_registration(request);
  }

  /// \brief Deletes user-frame registration data synchronously (`1610`, subcommand `0001`).
  [[nodiscard]] Status delete_user_frame_registration(
      const UserFrameRegistrationDeleteRequest& request) noexcept;

  /// \brief Compatibility alias for `delete_user_frame_registration`.
  [[nodiscard, deprecated("use delete_user_frame_registration")]] Status delete_user_frame(
      const UserFrameRegistrationDeleteRequest& request) noexcept {
    return delete_user_frame_registration(request);
  }

  /// \brief Controls C24 global signal ON/OFF synchronously (`1618`).
  [[nodiscard]] Status control_global_signal(
      const GlobalSignalControlRequest& request) noexcept;

  /// \brief Switches C24 operation mode / transmission settings synchronously (`1612`).
  [[nodiscard]] Status switch_serial_module_mode(
      const SerialModuleModeSwitchRequest& request) noexcept;

  /// \brief Initializes C24 format-5 transmission sequence synchronously (`1615`).
  [[nodiscard]] Status initialize_c24_transmission_sequence() noexcept;

  /// \brief Reads contiguous words as exactly one PLC request.
  [[nodiscard]] Status read_words_single_request(
      std::string_view head_device,
      std::uint16_t points,
      mcprotocol::serial::Span<std::uint16_t> out_words) noexcept;

  /// \brief Reads contiguous words as exactly one PLC request using `out_words.size()`.
  [[nodiscard]] Status read_words_single_request(
      std::string_view head_device,
      mcprotocol::serial::Span<std::uint16_t> out_words) noexcept;

  /// \brief Compatibility alias for `read_words_single_request`.
  [[nodiscard, deprecated("use read_words_single_request")]] Status read_words(
      std::string_view head_device,
      std::uint16_t points,
      mcprotocol::serial::Span<std::uint16_t> out_words) noexcept;

  /// \brief Compatibility alias for `read_words_single_request`.
  [[nodiscard, deprecated("use read_words_single_request")]] Status read_words(
      std::string_view head_device,
      mcprotocol::serial::Span<std::uint16_t> out_words) noexcept;

  /// \brief Reads extended file-register words synchronously.
  [[nodiscard]] Status read_extended_file_register_words(
      const ExtendedFileRegisterBatchReadWordsRequest& request,
      mcprotocol::serial::Span<std::uint16_t> out_words) noexcept;

  /// \brief Reads direct extended file-register words synchronously.
  [[nodiscard]] Status read_direct_extended_file_register_words(
      const ExtendedFileRegisterDirectBatchReadWordsRequest& request,
      mcprotocol::serial::Span<std::uint16_t> out_words) noexcept;

  /// \brief Compatibility alias for `read_direct_extended_file_register_words`.
  [[nodiscard, deprecated("use read_direct_extended_file_register_words")]]
  Status direct_read_extended_file_register_words(
      const ExtendedFileRegisterDirectBatchReadWordsRequest& request,
      mcprotocol::serial::Span<std::uint16_t> out_words) noexcept {
    return read_direct_extended_file_register_words(request, out_words);
  }

  /// \brief Reads contiguous bits as exactly one PLC request.
  [[nodiscard]] Status read_bits_single_request(
      std::string_view head_device,
      std::uint16_t points,
      mcprotocol::serial::Span<BitValue> out_bits) noexcept;

  /// \brief Reads contiguous bits as exactly one PLC request using `out_bits.size()`.
  [[nodiscard]] Status read_bits_single_request(
      std::string_view head_device,
      mcprotocol::serial::Span<BitValue> out_bits) noexcept;

  /// \brief Compatibility alias for `read_bits_single_request`.
  [[nodiscard, deprecated("use read_bits_single_request")]] Status read_bits(
      std::string_view head_device,
      std::uint16_t points,
      mcprotocol::serial::Span<BitValue> out_bits) noexcept;

  /// \brief Compatibility alias for `read_bits_single_request`.
  [[nodiscard, deprecated("use read_bits_single_request")]] Status read_bits(
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

  /// \brief Reads qualified-buffer `Un\\Gn` or `Un\\HGn` words.
  ///
  /// Use this for profiles whose qualified access route is native device access (`0401`).
  /// The `0601` helper route is profile/target-specific and may be rejected.
  [[nodiscard]] Status read_qualified_buffer_words(
      std::string_view head_device,
      std::uint16_t points,
      mcprotocol::serial::Span<std::uint16_t> out_words) noexcept;

  /// \brief Compatibility alias for `read_qualified_buffer_words`.
  [[nodiscard, deprecated("use read_qualified_buffer_words")]] Status read_native_qualified_words(
      std::string_view head_device,
      std::uint16_t points,
      mcprotocol::serial::Span<std::uint16_t> out_words) noexcept {
    return read_qualified_buffer_words(head_device, points, out_words);
  }

  /// \brief Reads long timer/counter contact or coil states through the dedicated status-block path.
  ///
  /// `LTS`/`LTC`/`LSTS`/`LSTC` with more than one point are explicitly aggregate reads: one
  /// four-word status-block request is issued per point, in address order. The complete plan is
  /// validated before transmission, the result is non-atomic across PLC scan times, and caller
  /// output is changed only after every internal request succeeds. `LCS`/`LCC` use one direct bit
  /// request and are not split. The host aggregate allocates `ceil(points / 8)` staging bytes
  /// before the first send and returns `StatusCode::OutOfMemory` if that allocation fails.
  [[nodiscard]] Status read_long_timer_counter_state_bits(
      std::string_view head_device,
      std::uint16_t points,
      mcprotocol::serial::Span<BitValue> out_bits) noexcept;

  /// \brief Reads long timer/counter states using `out_bits.size()` as the point count.
  [[nodiscard]] Status read_long_timer_counter_state_bits(
      std::string_view head_device,
      mcprotocol::serial::Span<BitValue> out_bits) noexcept;

  /// \brief Compatibility alias for `read_long_timer_counter_state_bits`.
  [[nodiscard, deprecated("use read_long_timer_counter_state_bits")]] Status read_long_state_bits(
      std::string_view head_device,
      std::uint16_t points,
      mcprotocol::serial::Span<BitValue> out_bits) noexcept {
    return read_long_timer_counter_state_bits(head_device, points, out_bits);
  }

  /// \brief Compatibility alias for `read_long_timer_counter_state_bits`.
  [[nodiscard, deprecated("use read_long_timer_counter_state_bits")]] Status read_long_state_bits(
      std::string_view head_device,
      mcprotocol::serial::Span<BitValue> out_bits) noexcept {
    return read_long_timer_counter_state_bits(head_device, out_bits);
  }

  /// \brief Writes contiguous words as exactly one PLC request.
  [[nodiscard]] Status write_words_single_request(
      std::string_view head_device,
      mcprotocol::serial::Span<const std::uint16_t> words) noexcept;

  /// \brief Compatibility alias for `write_words_single_request`.
  [[nodiscard, deprecated("use write_words_single_request")]] Status write_words(
      std::string_view head_device,
      mcprotocol::serial::Span<const std::uint16_t> words) noexcept;

  /// \brief Writes one bit inside an ordinary 16-bit word by one read-modify-write turn.
  ///
  /// The complete two-request plan is validated before transmission. The read and write share one
  /// absolute deadline, and the write is always issued after a successful read even when the bit is
  /// already in the requested state. The operation is not PLC-atomic: PLC logic or another
  /// connection can modify the word between the read and write.
  [[nodiscard]] Status write_bit_in_word(
      std::string_view word_device,
      int bit_index,
      bool value) noexcept;

  /// \brief Bit-in-word update through the block-addressed extended file-register route.
  [[nodiscard]] Status write_extended_file_register_bit_in_word(
      ExtendedFileRegisterAddress word_device,
      int bit_index,
      bool value) noexcept;

  /// \brief Bit-in-word update through the direct extended file-register route.
  [[nodiscard]] Status write_direct_extended_file_register_bit_in_word(
      std::uint32_t word_device_number,
      int bit_index,
      bool value) noexcept;

  /// \brief Compatibility alias for `write_direct_extended_file_register_bit_in_word`.
  [[nodiscard, deprecated("use write_direct_extended_file_register_bit_in_word")]]
  Status direct_write_extended_file_register_bit_in_word(
      std::uint32_t word_device_number,
      int bit_index,
      bool value) noexcept {
    return write_direct_extended_file_register_bit_in_word(word_device_number, bit_index, value);
  }

  /// \brief Bit-in-word update through one immutable `Jn\\...` link-direct route.
  [[nodiscard]] Status write_link_direct_bit_in_word(
      std::string_view word_device,
      int bit_index,
      bool value) noexcept;

  /// \brief Bit-in-word update through one immutable qualified-buffer route.
  [[nodiscard]] Status write_qualified_buffer_bit_in_word(
      std::string_view word_device,
      int bit_index,
      bool value) noexcept;

  /// \brief Compatibility alias for `write_qualified_buffer_bit_in_word`.
  [[nodiscard, deprecated("use write_qualified_buffer_bit_in_word")]]
  Status write_native_qualified_bit_in_word(
      std::string_view word_device,
      int bit_index,
      bool value) noexcept {
    return write_qualified_buffer_bit_in_word(word_device, bit_index, value);
  }

  /// \brief Writes extended file-register words synchronously.
  [[nodiscard]] Status write_extended_file_register_words(
      const ExtendedFileRegisterBatchWriteWordsRequest& request) noexcept;

  /// \brief Writes direct extended file-register words synchronously.
  [[nodiscard]] Status write_direct_extended_file_register_words(
      const ExtendedFileRegisterDirectBatchWriteWordsRequest& request) noexcept;

  /// \brief Compatibility alias for `write_direct_extended_file_register_words`.
  [[nodiscard, deprecated("use write_direct_extended_file_register_words")]]
  Status direct_write_extended_file_register_words(
      const ExtendedFileRegisterDirectBatchWriteWordsRequest& request) noexcept {
    return write_direct_extended_file_register_words(request);
  }

  /// \brief Writes contiguous bits as exactly one PLC request.
  [[nodiscard]] Status write_bits_single_request(
      std::string_view head_device,
      mcprotocol::serial::Span<const BitValue> bits) noexcept;

  /// \brief Compatibility alias for `write_bits_single_request`.
  [[nodiscard, deprecated("use write_bits_single_request")]] Status write_bits(
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

  /// \brief Writes qualified-buffer `Un\\Gn` or `Un\\HGn` words.
  ///
  /// Use this for profiles whose qualified access route is native device access (`1401`).
  /// The `1601` helper route is profile/target-specific and may be rejected.
  [[nodiscard]] Status write_qualified_buffer_words(
      std::string_view head_device,
      mcprotocol::serial::Span<const std::uint16_t> words) noexcept;

  /// \brief Compatibility alias for `write_qualified_buffer_words`.
  [[nodiscard, deprecated("use write_qualified_buffer_words")]] Status write_native_qualified_words(
      std::string_view head_device,
      mcprotocol::serial::Span<const std::uint16_t> words) noexcept {
    return write_qualified_buffer_words(head_device, words);
  }

  /// \brief Reads sparse Word and DWord items synchronously from explicit-width specs.
  [[nodiscard]] Status read_random(
      mcprotocol::serial::Span<const highlevel::RandomReadWordSpec> word_items,
      mcprotocol::serial::Span<const highlevel::RandomReadDWordSpec> dword_items,
      mcprotocol::serial::Span<std::uint16_t> out_words,
      mcprotocol::serial::Span<std::uint32_t> out_dwords) noexcept;

  /// \brief Compatibility alias for `read_random`.
  [[nodiscard, deprecated("use read_random")]] Status random_read(
      mcprotocol::serial::Span<const highlevel::RandomReadWordSpec> word_items,
      mcprotocol::serial::Span<const highlevel::RandomReadDWordSpec> dword_items,
      mcprotocol::serial::Span<std::uint16_t> out_words,
      mcprotocol::serial::Span<std::uint32_t> out_dwords) noexcept {
    return read_random(word_items, dword_items, out_words, out_dwords);
  }

  /// \brief Reads one sparse Word item synchronously from a string address.
  [[nodiscard]] Status read_random_word(
      std::string_view device,
      std::uint16_t& out_value) noexcept;

  /// \brief Compatibility alias for `read_random_word`.
  [[nodiscard, deprecated("use read_random_word")]] Status random_read_word(
      std::string_view device,
      std::uint16_t& out_value) noexcept {
    return read_random_word(device, out_value);
  }

  /// \brief Reads one sparse DWord item synchronously from a string address.
  [[nodiscard]] Status read_random_dword(
      std::string_view device,
      std::uint32_t& out_value) noexcept;

  /// \brief Compatibility alias for `read_random_dword`.
  [[nodiscard, deprecated("use read_random_dword")]] Status random_read_dword(
      std::string_view device,
      std::uint32_t& out_value) noexcept {
    return read_random_dword(device, out_value);
  }

  /// \brief Writes sparse Word items synchronously from string-address specs.
  ///
  /// Each spec requires an explicit value. A result that cannot be confirmed after transmission is
  /// reported as `StatusCode::OperationOutcomeUnknown` and is never retried automatically.
  [[nodiscard]] Status write_random_words(
      mcprotocol::serial::Span<const highlevel::RandomWriteWordSpec> items) noexcept;

  /// \brief Compatibility alias for `write_random_words`.
  [[nodiscard, deprecated("use write_random_words")]] Status random_write_words(
      mcprotocol::serial::Span<const highlevel::RandomWriteWordSpec> items) noexcept {
    return write_random_words(items);
  }

  /// \brief Writes sparse DWord items synchronously from string-address specs.
  ///
  /// Each spec requires an explicit value. A result that cannot be confirmed after transmission is
  /// reported as `StatusCode::OperationOutcomeUnknown` and is never retried automatically.
  [[nodiscard]] Status write_random_dwords(
      mcprotocol::serial::Span<const highlevel::RandomWriteDWordSpec> items) noexcept;

  /// \brief Compatibility alias for `write_random_dwords`.
  [[nodiscard, deprecated("use write_random_dwords")]] Status random_write_dwords(
      mcprotocol::serial::Span<const highlevel::RandomWriteDWordSpec> items) noexcept {
    return write_random_dwords(items);
  }

  /// \brief Writes extended file-register words randomly.
  [[nodiscard]] Status write_random_extended_file_register_words(
      mcprotocol::serial::Span<const ExtendedFileRegisterRandomWriteWordItem> items) noexcept;

  /// \brief Compatibility alias for `write_random_extended_file_register_words`.
  [[nodiscard, deprecated("use write_random_extended_file_register_words")]]
  Status random_write_extended_file_register_words(
      mcprotocol::serial::Span<const ExtendedFileRegisterRandomWriteWordItem> items) noexcept {
    return write_random_extended_file_register_words(items);
  }

  /// \brief Writes one sparse Word item synchronously from a string address.
  [[nodiscard]] Status write_random_word(
      std::string_view device,
      std::uint16_t value) noexcept;

  /// \brief Compatibility alias for `write_random_word`.
  [[nodiscard, deprecated("use write_random_word")]] Status random_write_word(
      std::string_view device,
      std::uint16_t value) noexcept {
    return write_random_word(device, value);
  }

  /// \brief Writes one sparse DWord item synchronously from a string address.
  [[nodiscard]] Status write_random_dword(
      std::string_view device,
      std::uint32_t value) noexcept;

  /// \brief Compatibility alias for `write_random_dword`.
  [[nodiscard, deprecated("use write_random_dword")]] Status random_write_dword(
      std::string_view device,
      std::uint32_t value) noexcept {
    return write_random_dword(device, value);
  }

  /// \brief Writes sparse bit items synchronously from string-address specs.
  ///
  /// Each spec requires an explicit `Off` or `On`. A result that cannot be confirmed after
  /// transmission is `StatusCode::OperationOutcomeUnknown` and is never retried automatically.
  [[nodiscard]] Status write_random_bits(
      mcprotocol::serial::Span<const highlevel::RandomWriteBitSpec> items) noexcept;

  /// \brief Compatibility alias for `write_random_bits`.
  [[nodiscard, deprecated("use write_random_bits")]] Status random_write_bits(
      mcprotocol::serial::Span<const highlevel::RandomWriteBitSpec> items) noexcept {
    return write_random_bits(items);
  }

  /// \brief Writes one sparse bit item synchronously from a string address.
  [[nodiscard]] Status write_random_bit(
      std::string_view device,
      BitValue value) noexcept;

  /// \brief Compatibility alias for `write_random_bit`.
  [[nodiscard, deprecated("use write_random_bit")]] Status random_write_bit(
      std::string_view device,
      BitValue value) noexcept {
    return write_random_bit(device, value);
  }

  /// \brief Reads sparse link-direct word items in input order.
  ///
  /// Bit devices return one 16-point mask in the corresponding output word. The request is never
  /// split, retried, or routed through the plain-device random API.
  [[nodiscard]] Status read_random_link_direct_words(
      Span<const LinkDirectRandomReadWordItem> word_items,
      Span<std::uint16_t> out_words) noexcept;

  /// \brief Writes sparse link-direct word items as one request.
  [[nodiscard]] Status write_random_link_direct_words(
      Span<const LinkDirectRandomWriteWordItem> items) noexcept;

  /// \brief Writes sparse link-direct bit items as one request.
  [[nodiscard]] Status write_random_link_direct_bits(
      Span<const LinkDirectRandomWriteBitItem> items) noexcept;

  /// \brief Runs the existing MC Serial self-test loopback operation synchronously.
  ///
  /// Both spans remain caller-owned. `out_echoed` is NUL-terminated only when its capacity exceeds
  /// the echoed payload length, exactly as in `MelsecSerialClient::async_loopback`.
  [[nodiscard]] Status self_test_loopback(
      Span<const char> hex_ascii,
      Span<char> out_echoed) noexcept;

  /// \brief Reads native multi-block data into caller-owned flat buffers and block metadata.
  ///
  /// Bit-block point counts are 16-bit units and expand to 16 `BitValue` entries per point.
  /// Outputs can be partially updated if response parsing fails.
  [[nodiscard]] Status read_block(
      const MultiBlockReadRequest& request,
      Span<std::uint16_t> out_words,
      Span<BitValue> out_bits,
      Span<MultiBlockReadBlockResult> out_results) noexcept;

  /// \brief Writes native multi-block data as one request without automatic retry.
  [[nodiscard]] Status write_block(const MultiBlockWriteRequest& request) noexcept;

  /// \brief Reads link-direct multi-block data into caller-owned flat buffers and block metadata.
  ///
  /// Result metadata retains each inner `DeviceAddress`; network numbers remain in `request`.
  /// Outputs can be partially updated if response parsing fails.
  [[nodiscard]] Status read_link_direct_block(
      const LinkDirectMultiBlockReadRequest& request,
      Span<std::uint16_t> out_words,
      Span<BitValue> out_bits,
      Span<MultiBlockReadBlockResult> out_results) noexcept;

  /// \brief Writes link-direct multi-block data as one request without automatic retry.
  [[nodiscard]] Status write_link_direct_block(
      const LinkDirectMultiBlockWriteRequest& request) noexcept;

  /// \brief Registers sparse Word and DWord monitor items from explicit-width specs.
  [[nodiscard]] Status register_monitor_devices(
      mcprotocol::serial::Span<const highlevel::RandomReadWordSpec> word_items,
      mcprotocol::serial::Span<const highlevel::RandomReadDWordSpec> dword_items) noexcept;

  /// \brief Compatibility alias for `register_monitor_devices`.
  [[nodiscard, deprecated("use register_monitor_devices")]] Status register_monitor(
      mcprotocol::serial::Span<const highlevel::RandomReadWordSpec> word_items,
      mcprotocol::serial::Span<const highlevel::RandomReadDWordSpec> dword_items) noexcept {
    return register_monitor_devices(word_items, dword_items);
  }

  /// \brief Registers one sparse Word monitor item synchronously.
  [[nodiscard]] Status register_monitor_word(std::string_view device) noexcept;

  /// \brief Registers one sparse DWord monitor item synchronously.
  [[nodiscard]] Status register_monitor_dword(std::string_view device) noexcept;

  /// \brief Registers extended file-register monitor data synchronously.
  [[nodiscard]] Status register_extended_file_register_monitor(
      const ExtendedFileRegisterMonitorRegistration& request) noexcept;

  /// \brief Registers link-direct monitor word items without running a monitor cycle.
  ///
  /// Use `run_monitor_cycle(out_words, {})` explicitly after successful registration.
  [[nodiscard]] Status register_link_direct_monitor_devices(
      const LinkDirectMonitorRegistration& request) noexcept;

  /// \brief Reads the most recently registered Word and DWord monitor items synchronously.
  [[nodiscard]] Status run_monitor_cycle(
      mcprotocol::serial::Span<std::uint16_t> out_words,
      mcprotocol::serial::Span<std::uint32_t> out_dwords) noexcept;

  /// \brief Compatibility alias for `run_monitor_cycle`.
  [[nodiscard, deprecated("use run_monitor_cycle")]] Status read_monitor(
      mcprotocol::serial::Span<std::uint16_t> out_words,
      mcprotocol::serial::Span<std::uint32_t> out_dwords) noexcept {
    return run_monitor_cycle(out_words, out_dwords);
  }

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

  HostSerialPort port_ {};
  MelsecSerialClient client_ {};
  ProtocolConfig protocol_config_ = ProtocolConfig::unconfigured_for_storage();
  std::array<mcprotocol::serial::Byte, kMaxResponseFrameBytes> rx_buffer_ {};
  CompletionState completion_ {};
};

/// \brief One-release compatibility alias for `HostSyncClient`.
using PosixSyncClient [[deprecated("use HostSyncClient")]] = HostSyncClient;

}  // namespace mcprotocol::serial
