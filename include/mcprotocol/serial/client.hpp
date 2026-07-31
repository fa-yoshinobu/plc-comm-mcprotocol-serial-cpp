#pragma once

#include "mcprotocol/serial/compat/array.hpp"
#include "mcprotocol/serial/compat/cstddef.hpp"
#include "mcprotocol/serial/compat/cstdint.hpp"

#include "mcprotocol/serial/codec.hpp"
#include "mcprotocol/serial/detail/fixed_item_array.hpp"
#include "mcprotocol/serial/link_direct.hpp"
#include "mcprotocol/serial/qualified_buffer.hpp"
#include "mcprotocol/serial/span.hpp"

/// \file client.hpp
/// \brief Asynchronous request-execution state machine for serial MC protocol traffic.
///
/// `MelsecSerialClient` sits above `CommandCodec` and `FrameCodec` and below any concrete transport.
/// It owns no UART implementation itself. The embedding application is responsible for:
///
/// - moving bytes from `pending_tx_frame()` to the actual serial port
/// - calling `notify_tx_complete(now_ms, transport_status)` when TX finishes or aborts
/// - feeding received bytes back through `on_rx_bytes()`
/// - calling `poll()` for timeout handling

namespace mcprotocol::serial {

/// \brief Asynchronous MC protocol client for UART / serial integrations.
///
/// The intended MCU-side workflow is:
/// 1. call `configure()`
/// 2. start an `async_*` request
/// 3. call `notify_tx_started(now_ms)` immediately before the first UART write
/// 4. transmit `pending_tx_frame()` with the board UART layer
/// 5. call `notify_tx_complete(now_ms, transport_status)` when TX finishes or aborts
/// 6. feed received bytes with `on_rx_bytes()`
/// 7. call `poll()` from the main loop or scheduler for deadline handling
///
/// Output spans passed to `async_*` requests must remain valid until the completion callback fires.
/// Only one request may be active. A second enabled `async_*` request returns `Busy` before
/// changing the active request's output storage, request metadata, or monitor state.
/// Same-instance calls from different operating-system threads are prohibited; the caller owns
/// scheduling. Separate instances are independent and may progress concurrently.
/// Cancelling before `notify_tx_started()` completes immediately as `Cancelled`. Cancelling during
/// TX records the cancellation but does not complete the request until the UART reports physical
/// TX completion or abort through `notify_tx_complete()`.
/// After transmission may have begun, any unconfirmed state-changing command completes as
/// `OperationOutcomeUnknown`; the client never retries it automatically.
class MelsecSerialClient {
 public:
  MelsecSerialClient() = default;

  /// \brief Stores protocol settings and validates the static configuration.
  /// \brief Configures a session, or acknowledges that the caller reset the underlying transport.
  ///
  /// When `requires_transport_reset()` is true, drain/close/reopen the UART transport before
  /// calling this again. A successful call clears the flag and allows new requests.
  [[nodiscard]] Status configure(const ProtocolConfig& config) noexcept;
  /// \brief Installs optional RS-485 TX begin/end hooks used by the async workflow.
  ///
  /// Both callbacks must be supplied together or both omitted. Hooks cannot be changed while a
  /// request is in flight, which guarantees that each TX begin callback is paired with the matching
  /// TX end callback and user pointer.
  [[nodiscard]] Status set_rs485_hooks(const Rs485Hooks& hooks) noexcept;

  /// \brief Returns whether a request is currently in flight.
  [[nodiscard]] bool busy() const noexcept;
  /// \brief Returns true after an unsequenced ambiguous receive/transport failure until reset plus
  /// reconfiguration.
  ///
  /// Format2 has a per-request block identity and can discard its own late response. Other frame
  /// families cannot safely distinguish a same-route late response from the next request.
  [[nodiscard]] bool requires_transport_reset() const noexcept;
  /// \brief Returns the encoded frame that should be sent to the UART layer.
  [[nodiscard]] mcprotocol::serial::Span<const mcprotocol::serial::Byte> pending_tx_frame() const noexcept;

  /// \brief Starts the one absolute TX/drain/RX transaction deadline.
  ///
  /// Call this immediately before the first transport write attempt. The same deadline remains in
  /// force through physical drain, receive, correlation, and decode. It is never restarted by
  /// partial progress. Calling it more than once for a request is rejected.
  [[nodiscard]] Status notify_tx_started(std::uint32_t now_ms) noexcept;
  /// \brief Returns the active absolute deadline, or zero before TX start / with no active request.
  [[nodiscard]] std::uint32_t transaction_deadline_ms() const noexcept;

  /// \brief Advances the state machine after the transport finished or aborted the pending TX.
  ///
  /// `transport_status` is mandatory. Pass `ok_status()` only after confirmed physical TX
  /// completion; otherwise pass the actual transport failure or cancellation status.
  [[nodiscard]] Status notify_tx_complete(
      std::uint32_t now_ms,
      Status transport_status) noexcept;

  /// \brief Feeds received bytes into the response decoder.
  void on_rx_bytes(std::uint32_t now_ms, mcprotocol::serial::Span<const mcprotocol::serial::Byte> bytes) noexcept;
  /// \brief Checks timeouts for the current in-flight request.
  void poll(std::uint32_t now_ms) noexcept;
  /// \brief Requests cancellation of the in-flight request.
  ///
  /// Before TX starts, cancellation completes immediately. During TX, completion is deferred until
  /// `notify_tx_complete()` confirms that physical TX has completed or stopped, so an active
  /// RS-485 direction hook can always be released exactly once.
  void cancel() noexcept;

  /// \brief Starts contiguous word read (`0401`).
  [[nodiscard]] Status async_batch_read_words(
      std::uint32_t now_ms,
      const BatchReadWordsRequest& request,
      mcprotocol::serial::Span<std::uint16_t> out_words,
      CompletionHandler callback,
      void* user) noexcept;

  /// \brief Starts extended file-register word read.
  [[nodiscard]] Status async_read_extended_file_register_words(
      std::uint32_t now_ms,
      const ExtendedFileRegisterBatchReadWordsRequest& request,
      mcprotocol::serial::Span<std::uint16_t> out_words,
      CompletionHandler callback,
      void* user) noexcept;

  /// \brief Starts direct extended file-register word read.
  [[nodiscard]] Status async_direct_read_extended_file_register_words(
      std::uint32_t now_ms,
      const ExtendedFileRegisterDirectBatchReadWordsRequest& request,
      mcprotocol::serial::Span<std::uint16_t> out_words,
      CompletionHandler callback,
      void* user) noexcept;

  /// \brief Starts `Jn\\...` link-direct word read over device extension specification.
  [[nodiscard]] Status async_link_direct_batch_read_words(
      std::uint32_t now_ms,
      const LinkDirectDevice& device,
      std::uint16_t points,
      mcprotocol::serial::Span<std::uint16_t> out_words,
      CompletionHandler callback,
      void* user) noexcept;

  /// \brief Starts contiguous bit read (`0401` bit path).
  [[nodiscard]] Status async_batch_read_bits(
      std::uint32_t now_ms,
      const BatchReadBitsRequest& request,
      mcprotocol::serial::Span<BitValue> out_bits,
      CompletionHandler callback,
      void* user) noexcept;

  /// \brief Starts `Jn\\...` link-direct bit read over device extension specification.
  [[nodiscard]] Status async_link_direct_batch_read_bits(
      std::uint32_t now_ms,
      const LinkDirectDevice& device,
      std::uint16_t points,
      mcprotocol::serial::Span<BitValue> out_bits,
      CompletionHandler callback,
      void* user) noexcept;

  /// \brief Starts contiguous word write (`1401`).
  [[nodiscard]] Status async_batch_write_words(
      std::uint32_t now_ms,
      const BatchWriteWordsRequest& request,
      CompletionHandler callback,
      void* user) noexcept;

  /// \brief Starts extended file-register word write.
  [[nodiscard]] Status async_write_extended_file_register_words(
      std::uint32_t now_ms,
      const ExtendedFileRegisterBatchWriteWordsRequest& request,
      CompletionHandler callback,
      void* user) noexcept;

  /// \brief Starts direct extended file-register word write.
  [[nodiscard]] Status async_direct_write_extended_file_register_words(
      std::uint32_t now_ms,
      const ExtendedFileRegisterDirectBatchWriteWordsRequest& request,
      CompletionHandler callback,
      void* user) noexcept;

  /// \brief Starts `Jn\\...` link-direct contiguous word write over device extension specification.
  [[nodiscard]] Status async_link_direct_batch_write_words(
      std::uint32_t now_ms,
      const LinkDirectDevice& device,
      mcprotocol::serial::Span<const std::uint16_t> words,
      CompletionHandler callback,
      void* user) noexcept;

  /// \brief Starts contiguous bit write (`1401` bit path).
  [[nodiscard]] Status async_batch_write_bits(
      std::uint32_t now_ms,
      const BatchWriteBitsRequest& request,
      CompletionHandler callback,
      void* user) noexcept;

  /// \brief Starts `Jn\\...` link-direct contiguous bit write over device extension specification.
  [[nodiscard]] Status async_link_direct_batch_write_bits(
      std::uint32_t now_ms,
      const LinkDirectDevice& device,
      mcprotocol::serial::Span<const BitValue> bits,
      CompletionHandler callback,
      void* user) noexcept;

  /// \brief Starts helper qualified word read over module-buffer access.
  [[nodiscard]] Status async_extended_batch_read_words(
      std::uint32_t now_ms,
      const QualifiedBufferWordDevice& device,
      std::uint16_t points,
      mcprotocol::serial::Span<std::uint16_t> out_words,
      CompletionHandler callback,
      void* user) noexcept;

  /// \brief Starts helper qualified word write over module-buffer access.
  [[nodiscard]] Status async_extended_batch_write_words(
      std::uint32_t now_ms,
      const QualifiedBufferWordDevice& device,
      mcprotocol::serial::Span<const std::uint16_t> words,
      CompletionHandler callback,
      void* user) noexcept;

  /// \brief Starts native random read (`0403`).
  [[nodiscard]] Status async_random_read(
      std::uint32_t now_ms,
      const RandomReadRequest& request,
      mcprotocol::serial::Span<std::uint16_t> out_words,
      mcprotocol::serial::Span<std::uint32_t> out_dwords,
      CompletionHandler callback,
      void* user) noexcept;

  /// \brief Starts native `Jn\\...` random read (`0403` + device extension specification).
  [[nodiscard]] Status async_link_direct_random_read(
      std::uint32_t now_ms,
      mcprotocol::serial::Span<const LinkDirectRandomReadWordItem> word_items,
      mcprotocol::serial::Span<std::uint16_t> out_words,
      CompletionHandler callback,
      void* user) noexcept;

  /// \brief Starts native random word/dword write (`1402` word path).
  ///
  /// Every item requires an explicit value. Once transmission has started, timeout, cancellation,
  /// or an unconfirmed transport failure completes as `StatusCode::OperationOutcomeUnknown`; the
  /// library never retries the write.
  [[nodiscard]] Status async_random_write_words(
      std::uint32_t now_ms,
      mcprotocol::serial::Span<const RandomWriteWordItem> word_items,
      mcprotocol::serial::Span<const RandomWriteDWordItem> dword_items,
      CompletionHandler callback,
      void* user) noexcept;

  /// \brief Starts extended file-register random word write.
  [[nodiscard]] Status async_random_write_extended_file_register_words(
      std::uint32_t now_ms,
      mcprotocol::serial::Span<const ExtendedFileRegisterRandomWriteWordItem> items,
      CompletionHandler callback,
      void* user) noexcept;

  /// \brief Starts native `Jn\\...` random word write (`1402` + device extension specification).
  ///
  /// Every item requires an explicit value. An unconfirmed result after transmission is
  /// `StatusCode::OperationOutcomeUnknown` and is never retried automatically.
  [[nodiscard]] Status async_link_direct_random_write_words(
      std::uint32_t now_ms,
      mcprotocol::serial::Span<const LinkDirectRandomWriteWordItem> items,
      CompletionHandler callback,
      void* user) noexcept;

  /// \brief Starts native random bit write (`1402` bit path).
  ///
  /// Every item requires an explicit `Off` or `On`. An unconfirmed result after transmission is
  /// `StatusCode::OperationOutcomeUnknown` and is never retried automatically.
  [[nodiscard]] Status async_random_write_bits(
      std::uint32_t now_ms,
      mcprotocol::serial::Span<const RandomWriteBitItem> items,
      CompletionHandler callback,
      void* user) noexcept;

  /// \brief Starts native `Jn\\...` random bit write (`1402` + device extension specification).
  ///
  /// Every item requires an explicit `Off` or `On`. An unconfirmed result after transmission is
  /// `StatusCode::OperationOutcomeUnknown` and is never retried automatically.
  [[nodiscard]] Status async_link_direct_random_write_bits(
      std::uint32_t now_ms,
      mcprotocol::serial::Span<const LinkDirectRandomWriteBitItem> items,
      CompletionHandler callback,
      void* user) noexcept;

  /// \brief Starts native multi-block read (`0406`).
  [[nodiscard]] Status async_multi_block_read(
      std::uint32_t now_ms,
      const MultiBlockReadRequest& request,
      mcprotocol::serial::Span<std::uint16_t> out_words,
      mcprotocol::serial::Span<BitValue> out_bits,
      mcprotocol::serial::Span<MultiBlockReadBlockResult> out_results,
      CompletionHandler callback,
      void* user) noexcept;

  /// \brief Starts native `Jn\\...` multi-block read (`0406` + device extension specification).
  ///
  /// The returned `out_results` preserve block order, point counts, and offsets. Their
  /// `head_device` field contains the inner device code/address, while the network number stays in
  /// the original request blocks.
  [[nodiscard]] Status async_link_direct_multi_block_read(
      std::uint32_t now_ms,
      const LinkDirectMultiBlockReadRequest& request,
      mcprotocol::serial::Span<std::uint16_t> out_words,
      mcprotocol::serial::Span<BitValue> out_bits,
      mcprotocol::serial::Span<MultiBlockReadBlockResult> out_results,
      CompletionHandler callback,
      void* user) noexcept;

  /// \brief Starts native multi-block write (`1406`).
  [[nodiscard]] Status async_multi_block_write(
      std::uint32_t now_ms,
      const MultiBlockWriteRequest& request,
      CompletionHandler callback,
      void* user) noexcept;

  /// \brief Starts native `Jn\\...` multi-block write (`1406` + device extension specification).
  [[nodiscard]] Status async_link_direct_multi_block_write(
      std::uint32_t now_ms,
      const LinkDirectMultiBlockWriteRequest& request,
      CompletionHandler callback,
      void* user) noexcept;

  /// \brief Starts monitor registration (`0801`).
  [[nodiscard]] Status async_register_monitor(
      std::uint32_t now_ms,
      const MonitorRegistration& request,
      CompletionHandler callback,
      void* user) noexcept;

  /// \brief Starts extended file-register monitor registration.
  [[nodiscard]] Status async_register_extended_file_register_monitor(
      std::uint32_t now_ms,
      const ExtendedFileRegisterMonitorRegistration& request,
      CompletionHandler callback,
      void* user) noexcept;

  /// \brief Starts native `Jn\\...` monitor registration (`0801` + device extension specification).
  [[nodiscard]] Status async_link_direct_register_monitor(
      std::uint32_t now_ms,
      const LinkDirectMonitorRegistration& request,
      CompletionHandler callback,
      void* user) noexcept;

  /// \brief Starts monitor read (`0802`) using the most recent registration.
  [[nodiscard]] Status async_read_monitor(
      std::uint32_t now_ms,
      mcprotocol::serial::Span<std::uint16_t> out_words,
      mcprotocol::serial::Span<std::uint32_t> out_dwords,
      CompletionHandler callback,
      void* user) noexcept;

  /// \brief Starts extended file-register monitor read.
  [[nodiscard]] Status async_read_extended_file_register_monitor(
      std::uint32_t now_ms,
      mcprotocol::serial::Span<std::uint16_t> out_words,
      CompletionHandler callback,
      void* user) noexcept;

  /// \brief Starts host-buffer read (`0613`).
  [[nodiscard]] Status async_read_host_buffer(
      std::uint32_t now_ms,
      const HostBufferReadRequest& request,
      mcprotocol::serial::Span<std::uint16_t> out_words,
      CompletionHandler callback,
      void* user) noexcept;

  /// \brief Starts host-buffer write (`1613`).
  [[nodiscard]] Status async_write_host_buffer(
      std::uint32_t now_ms,
      const HostBufferWriteRequest& request,
      CompletionHandler callback,
      void* user) noexcept;

  /// \brief Starts module-buffer byte read (`0601`).
  [[nodiscard]] Status async_read_module_buffer(
      std::uint32_t now_ms,
      const ModuleBufferReadRequest& request,
      mcprotocol::serial::Span<mcprotocol::serial::Byte> out_bytes,
      CompletionHandler callback,
      void* user) noexcept;

  /// \brief Starts module-buffer byte write (`1601`).
  [[nodiscard]] Status async_write_module_buffer(
      std::uint32_t now_ms,
      const ModuleBufferWriteRequest& request,
      CompletionHandler callback,
      void* user) noexcept;

  /// \brief Starts CPU-model read.
  [[nodiscard]] Status async_read_cpu_model(
      std::uint32_t now_ms,
      CpuModelInfo& out_info,
      CompletionHandler callback,
      void* user) noexcept;

  /// \brief Starts remote RUN (`1001`) with mandatory conflict and clear policies.
  ///
  /// After transmission starts, an unconfirmed transport/timeout result is reported as
  /// `StatusCode::OperationOutcomeUnknown`; the library never retries this command.
  [[nodiscard]] Status async_remote_run(
      std::uint32_t now_ms,
      RemoteOperationMode mode,
      RemoteRunClearMode clear_mode,
      CompletionHandler callback,
      void* user) noexcept;

  /// \brief Starts remote STOP (`1002`).
  [[nodiscard]] Status async_remote_stop(
      std::uint32_t now_ms,
      CompletionHandler callback,
      void* user) noexcept;

  /// \brief Starts remote PAUSE (`1003`) with a mandatory conflict policy.
  ///
  /// After transmission starts, an unconfirmed transport/timeout result is reported as
  /// `StatusCode::OperationOutcomeUnknown`; the library never retries with another policy.
  [[nodiscard]] Status async_remote_pause(
      std::uint32_t now_ms,
      RemoteOperationMode mode,
      CompletionHandler callback,
      void* user) noexcept;

  /// \brief Starts remote latch clear (`1005`).
  [[nodiscard]] Status async_remote_latch_clear(
      std::uint32_t now_ms,
      CompletionHandler callback,
      void* user) noexcept;

  /// \brief Unlocks remote-password-protected access (`1630`).
  [[nodiscard]] Status async_unlock_remote_password(
      std::uint32_t now_ms,
      std::string_view remote_password,
      CompletionHandler callback,
      void* user) noexcept;

  /// \brief Locks remote-password-protected access (`1631`).
  [[nodiscard]] Status async_lock_remote_password(
      std::uint32_t now_ms,
      std::string_view remote_password,
      CompletionHandler callback,
      void* user) noexcept;

  /// \brief Starts clear error information (`1617`) for serial/C24 targets.
  [[nodiscard]] Status async_clear_error_information(
      std::uint32_t now_ms,
      CompletionHandler callback,
      void* user) noexcept;

  /// \brief Starts remote RESET (`1006`).
  ///
  /// Completion means the request bytes were transmitted successfully. The command does not wait
  /// for a normal response and does not claim that the PLC completed its reset.
  [[nodiscard]] Status async_remote_reset(
      std::uint32_t now_ms,
      CompletionHandler callback,
      void* user) noexcept;

  /// \brief Starts user-frame registration-data read (`0610`).
  [[nodiscard]] Status async_read_user_frame(
      std::uint32_t now_ms,
      const UserFrameReadRequest& request,
      UserFrameRegistrationData& out_data,
      CompletionHandler callback,
      void* user) noexcept;

  /// \brief Starts user-frame registration-data write (`1610`, subcommand `0000`).
  [[nodiscard]] Status async_write_user_frame(
      std::uint32_t now_ms,
      const UserFrameWriteRequest& request,
      CompletionHandler callback,
      void* user) noexcept;

  /// \brief Starts user-frame registration-data delete (`1610`, subcommand `0001`).
  [[nodiscard]] Status async_delete_user_frame(
      std::uint32_t now_ms,
      const UserFrameDeleteRequest& request,
      CompletionHandler callback,
      void* user) noexcept;

  /// \brief Starts C24 global-signal ON/OFF control (`1618`).
  [[nodiscard]] Status async_control_global_signal(
      std::uint32_t now_ms,
      const GlobalSignalControlRequest& request,
      CompletionHandler callback,
      void* user) noexcept;

  /// \brief Starts C24 mode switching (`1612`).
  [[nodiscard]] Status async_switch_serial_module_mode(
      std::uint32_t now_ms,
      const SerialModuleModeSwitchRequest& request,
      CompletionHandler callback,
      void* user) noexcept;

  /// \brief Starts C24 transmission-sequence initialization (`1615`).
  [[nodiscard]] Status async_initialize_c24_transmission_sequence(
      std::uint32_t now_ms,
      CompletionHandler callback,
      void* user) noexcept;

  /// \brief Starts loopback using hexadecimal ASCII payload bytes.
  [[nodiscard]] Status async_loopback(
      std::uint32_t now_ms,
      mcprotocol::serial::Span<const char> hex_ascii,
      mcprotocol::serial::Span<char> out_echoed,
      CompletionHandler callback,
      void* user) noexcept;

 private:
  enum class OperationKind : std::uint8_t {
    None,
    BatchReadWords,
    ReadExtendedFileRegisterWords,
    DirectReadExtendedFileRegisterWords,
    BatchReadBits,
    BatchWriteWords,
    WriteExtendedFileRegisterWords,
    DirectWriteExtendedFileRegisterWords,
    BatchWriteBits,
    ExtendedBatchReadWords,
    ExtendedBatchWriteWords,
#if MCPROTOCOL_SERIAL_ENABLE_RANDOM_COMMANDS
    RandomRead,
    RandomWriteWords,
    RandomWriteExtendedFileRegisterWords,
    RandomWriteBits,
#endif
#if MCPROTOCOL_SERIAL_ENABLE_MULTI_BLOCK_COMMANDS
    MultiBlockRead,
    MultiBlockWrite,
#endif
#if MCPROTOCOL_SERIAL_ENABLE_MONITOR_COMMANDS
    RegisterMonitor,
    RegisterExtendedFileRegisterMonitor,
    ReadMonitor,
    ReadExtendedFileRegisterMonitor,
#endif
#if MCPROTOCOL_SERIAL_ENABLE_HOST_BUFFER_COMMANDS
    ReadHostBuffer,
    WriteHostBuffer,
#endif
#if MCPROTOCOL_SERIAL_ENABLE_MODULE_BUFFER_COMMANDS
    ReadModuleBuffer,
    WriteModuleBuffer,
#endif
#if MCPROTOCOL_SERIAL_ENABLE_CPU_MODEL_COMMANDS
    ReadCpuModel,
#endif
    RemoteRun,
    RemoteStop,
    RemotePause,
    RemoteLatchClear,
    UnlockRemotePassword,
    LockRemotePassword,
    ClearErrorInformation,
    RemoteReset,
    ReadUserFrame,
    WriteUserFrame,
    DeleteUserFrame,
    ControlGlobalSignal,
    SwitchSerialModuleMode,
    InitializeTransmissionSequence,
#if MCPROTOCOL_SERIAL_ENABLE_LOOPBACK_COMMANDS
    Loopback
#endif
  };

  [[nodiscard]] Status start_request(
      std::uint32_t now_ms,
      OperationKind operation,
      std::size_t request_data_size,
      CompletionHandler callback,
      void* user) noexcept;
  [[nodiscard]] Status validate_request_admission() const noexcept;

  [[nodiscard]] std::uint8_t expected_e1_response_subheader() const noexcept;
  [[nodiscard]] std::size_t expected_success_response_data_size(
      OperationKind operation) const noexcept;
  [[nodiscard]] Status handle_response(mcprotocol::serial::Span<const std::uint8_t> response_data) noexcept;
  [[nodiscard]] Status active_timeout_status(const char* timeout_message) const noexcept;
  [[nodiscard]] Status active_transport_failure_status(Status transport_status) const noexcept;
  [[nodiscard]] Status active_unconfirmed_failure_status(Status failure_status) const noexcept;
  [[nodiscard]] bool active_operation_outcome_can_be_unknown() const noexcept;
  void complete(Status status) noexcept;
  void clear_pending_outputs() noexcept;
  void clear_pending_copies() noexcept;

  ProtocolConfig config_ = ProtocolConfig::unconfigured_for_storage();
  Rs485Hooks rs485_hooks_ {};
  bool configured_ = false;
  bool busy_ = false;
  bool transport_reset_required_ = false;
  bool awaiting_write_complete_ = false;
  bool tx_started_ = false;
  bool cancel_requested_during_tx_ = false;
  OperationKind operation_ = OperationKind::None;
  CompletionHandler callback_ = nullptr;
  void* callback_user_ = nullptr;
  std::uint32_t response_deadline_ms_ = 0;
  std::uint8_t next_format2_block_number_ = 0;
  std::uint8_t active_format2_block_number_ = 0;
  bool active_format2_block_number_valid_ = false;

  std::array<std::uint8_t, kMaxRequestFrameBytes> tx_frame_ {};
  std::size_t tx_frame_size_ = 0;
  std::array<std::uint8_t, kMaxResponseFrameBytes> rx_frame_ {};
  std::size_t rx_frame_size_ = 0;
  std::array<std::uint8_t, kMaxRequestDataBytes> request_data_ {};

  BatchReadWordsRequest batch_read_words_request_ {DeviceAddress {DeviceCode::D, 0U}, 0U};
  ExtendedFileRegisterBatchReadWordsRequest extended_file_register_read_request_ {
      ExtendedFileRegisterAddress {1U, 0U}, 0U};
  ExtendedFileRegisterDirectBatchReadWordsRequest direct_extended_file_register_read_request_ {0U, 0U};
  BatchReadBitsRequest batch_read_bits_request_ {DeviceAddress {DeviceCode::M, 0U}, 0U};
  UserFrameReadRequest user_frame_read_request_ {0U};
  QualifiedBufferWordDevice extended_batch_words_device_ {QualifiedBufferDeviceKind::G, 0U, 0U};
  std::uint16_t extended_batch_words_points_ = 0;
#if MCPROTOCOL_SERIAL_ENABLE_HOST_BUFFER_COMMANDS
  HostBufferReadRequest host_buffer_read_request_ {0U, 0U};
#endif
#if MCPROTOCOL_SERIAL_ENABLE_MODULE_BUFFER_COMMANDS
  ModuleBufferReadRequest module_buffer_read_request_ {0U, 0U, 0U};
#endif

  mcprotocol::serial::Span<std::uint16_t> out_words_ {};
  mcprotocol::serial::Span<BitValue> out_bits_ {};
#if MCPROTOCOL_SERIAL_ENABLE_RANDOM_COMMANDS || MCPROTOCOL_SERIAL_ENABLE_MONITOR_COMMANDS
  mcprotocol::serial::Span<std::uint16_t> out_random_words_ {};
  mcprotocol::serial::Span<std::uint32_t> out_random_dwords_ {};
#endif
#if MCPROTOCOL_SERIAL_ENABLE_MODULE_BUFFER_COMMANDS
  mcprotocol::serial::Span<mcprotocol::serial::Byte> out_bytes_ {};
#endif
#if MCPROTOCOL_SERIAL_ENABLE_LOOPBACK_COMMANDS
  mcprotocol::serial::Span<char> out_chars_ {};
#endif
#if MCPROTOCOL_SERIAL_ENABLE_MULTI_BLOCK_COMMANDS
  mcprotocol::serial::Span<MultiBlockReadBlockResult> out_block_results_ {};
#endif
#if MCPROTOCOL_SERIAL_ENABLE_CPU_MODEL_COMMANDS
  CpuModelInfo* out_cpu_model_ = nullptr;
#endif
  UserFrameRegistrationData* out_user_frame_data_ = nullptr;

#if MCPROTOCOL_SERIAL_ENABLE_RANDOM_COMMANDS || MCPROTOCOL_SERIAL_ENABLE_MONITOR_COMMANDS
  std::array<RandomReadWordItem, kMaxRandomAccessItems> pending_random_word_items_ =
      detail::make_filled_array<RandomReadWordItem, kMaxRandomAccessItems>(
          RandomReadWordItem {
              DeviceAddress {static_cast<DeviceCode>(0xFFU), 0U}});
  std::array<RandomReadDWordItem, kMaxRandomAccessItems> pending_random_dword_items_ =
      detail::make_filled_array<RandomReadDWordItem, kMaxRandomAccessItems>(
          RandomReadDWordItem {
              DeviceAddress {static_cast<DeviceCode>(0xFFU), 0U}});
  std::size_t pending_random_word_item_count_ = 0;
  std::size_t pending_random_dword_item_count_ = 0;
#endif
#if MCPROTOCOL_SERIAL_ENABLE_MONITOR_COMMANDS
  std::array<RandomReadWordItem, kMaxMonitorItems> monitor_word_items_ =
      detail::make_filled_array<RandomReadWordItem, kMaxMonitorItems>(
          RandomReadWordItem {
              DeviceAddress {static_cast<DeviceCode>(0xFFU), 0U}});
  std::array<RandomReadDWordItem, kMaxMonitorItems> monitor_dword_items_ =
      detail::make_filled_array<RandomReadDWordItem, kMaxMonitorItems>(
          RandomReadDWordItem {
              DeviceAddress {static_cast<DeviceCode>(0xFFU), 0U}});
  std::size_t monitor_word_item_count_ = 0;
  std::size_t monitor_dword_item_count_ = 0;
  bool monitor_registered_ = false;
  std::array<ExtendedFileRegisterAddress, kMaxMonitorItems> pending_extended_file_register_items_ =
      detail::make_filled_array<ExtendedFileRegisterAddress, kMaxMonitorItems>(
          ExtendedFileRegisterAddress(0U, 0U));
  std::size_t pending_extended_file_register_item_count_ = 0;
  std::array<ExtendedFileRegisterAddress, kMaxMonitorItems> extended_file_register_monitor_items_ =
      detail::make_filled_array<ExtendedFileRegisterAddress, kMaxMonitorItems>(
          ExtendedFileRegisterAddress(0U, 0U));
  std::size_t extended_file_register_monitor_item_count_ = 0;
  bool extended_file_register_monitor_registered_ = false;
#endif
#if MCPROTOCOL_SERIAL_ENABLE_MULTI_BLOCK_COMMANDS
  std::array<MultiBlockReadBlock, kMaxMultiBlockCount> pending_multi_blocks_ =
      detail::make_filled_array<MultiBlockReadBlock, kMaxMultiBlockCount>(
          MultiBlockReadBlock(
              DeviceAddress {static_cast<DeviceCode>(0xFFU), 0U}, 0U, false));
  std::size_t pending_multi_block_count_ = 0;
#endif
#if MCPROTOCOL_SERIAL_ENABLE_LOOPBACK_COMMANDS
  std::array<char, kMaxLoopbackBytes> pending_loopback_ {};
  std::size_t pending_loopback_size_ = 0;
#endif
};

}  // namespace mcprotocol::serial
