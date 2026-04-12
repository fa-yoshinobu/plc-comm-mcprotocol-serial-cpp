#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "mcprotocol/serial/codec.hpp"
#include "mcprotocol/serial/link_direct.hpp"
#include "mcprotocol/serial/qualified_buffer.hpp"
#include "mcprotocol/serial/span_compat.hpp"

namespace mcprotocol::serial {

/// \brief Asynchronous MC protocol client for UART / serial integrations.
///
/// The intended MCU-side workflow is:
/// 1. call `configure()`
/// 2. start an `async_*` request
/// 3. transmit `pending_tx_frame()` with the board UART layer
/// 4. call `notify_tx_complete()` when TX finishes
/// 5. feed received bytes with `on_rx_bytes()`
/// 6. call `poll()` from the main loop or scheduler for timeout handling
class MelsecSerialClient {
 public:
  MelsecSerialClient() = default;

  /// \brief Stores protocol settings and validates the static configuration.
  [[nodiscard]] Status configure(const ProtocolConfig& config) noexcept;
  /// \brief Installs optional RS-485 TX begin/end hooks used by the async workflow.
  void set_rs485_hooks(const Rs485Hooks& hooks) noexcept;

  /// \brief Returns whether a request is currently in flight.
  [[nodiscard]] bool busy() const noexcept;
  /// \brief Returns the encoded frame that should be sent to the UART layer.
  [[nodiscard]] std::span<const std::byte> pending_tx_frame() const noexcept;

  /// \brief Advances the state machine after the transport finished sending the pending frame.
  [[nodiscard]] Status notify_tx_complete(
      std::uint32_t now_ms,
      Status transport_status = ok_status()) noexcept;

  /// \brief Feeds received bytes into the response decoder.
  void on_rx_bytes(std::uint32_t now_ms, std::span<const std::byte> bytes) noexcept;
  /// \brief Checks timeouts for the current in-flight request.
  void poll(std::uint32_t now_ms) noexcept;
  /// \brief Cancels the in-flight request and clears transient state.
  void cancel() noexcept;

  /// \brief Starts contiguous word read (`0401`).
  [[nodiscard]] Status async_batch_read_words(
      std::uint32_t now_ms,
      const BatchReadWordsRequest& request,
      std::span<std::uint16_t> out_words,
      CompletionHandler callback,
      void* user) noexcept;

  /// \brief Starts extended file-register word read.
  [[nodiscard]] Status async_read_extended_file_register_words(
      std::uint32_t now_ms,
      const ExtendedFileRegisterBatchReadWordsRequest& request,
      std::span<std::uint16_t> out_words,
      CompletionHandler callback,
      void* user) noexcept;

  /// \brief Starts direct extended file-register word read.
  [[nodiscard]] Status async_direct_read_extended_file_register_words(
      std::uint32_t now_ms,
      const ExtendedFileRegisterDirectBatchReadWordsRequest& request,
      std::span<std::uint16_t> out_words,
      CompletionHandler callback,
      void* user) noexcept;

  /// \brief Starts `Jn\\...` link-direct word read over device extension specification.
  [[nodiscard]] Status async_link_direct_batch_read_words(
      std::uint32_t now_ms,
      const LinkDirectDevice& device,
      std::uint16_t points,
      std::span<std::uint16_t> out_words,
      CompletionHandler callback,
      void* user) noexcept;

  /// \brief Starts contiguous bit read (`0401` bit path).
  [[nodiscard]] Status async_batch_read_bits(
      std::uint32_t now_ms,
      const BatchReadBitsRequest& request,
      std::span<BitValue> out_bits,
      CompletionHandler callback,
      void* user) noexcept;

  /// \brief Starts `Jn\\...` link-direct bit read over device extension specification.
  [[nodiscard]] Status async_link_direct_batch_read_bits(
      std::uint32_t now_ms,
      const LinkDirectDevice& device,
      std::uint16_t points,
      std::span<BitValue> out_bits,
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
      std::span<const std::uint16_t> words,
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
      std::span<const BitValue> bits,
      CompletionHandler callback,
      void* user) noexcept;

  /// \brief Starts helper qualified word read over module-buffer access.
  [[nodiscard]] Status async_extended_batch_read_words(
      std::uint32_t now_ms,
      const QualifiedBufferWordDevice& device,
      std::uint16_t points,
      std::span<std::uint16_t> out_words,
      CompletionHandler callback,
      void* user) noexcept;

  /// \brief Starts helper qualified word write over module-buffer access.
  [[nodiscard]] Status async_extended_batch_write_words(
      std::uint32_t now_ms,
      const QualifiedBufferWordDevice& device,
      std::span<const std::uint16_t> words,
      CompletionHandler callback,
      void* user) noexcept;

  /// \brief Starts native random read (`0403`).
  [[nodiscard]] Status async_random_read(
      std::uint32_t now_ms,
      const RandomReadRequest& request,
      std::span<std::uint32_t> out_values,
      CompletionHandler callback,
      void* user) noexcept;

  /// \brief Starts native `Jn\\...` random read (`0403` + device extension specification).
  [[nodiscard]] Status async_link_direct_random_read(
      std::uint32_t now_ms,
      std::span<const LinkDirectRandomReadItem> items,
      std::span<std::uint32_t> out_values,
      CompletionHandler callback,
      void* user) noexcept;

  /// \brief Starts native random word/dword write (`1402` word path).
  [[nodiscard]] Status async_random_write_words(
      std::uint32_t now_ms,
      std::span<const RandomWriteWordItem> items,
      CompletionHandler callback,
      void* user) noexcept;

  /// \brief Starts extended file-register random word write.
  [[nodiscard]] Status async_random_write_extended_file_register_words(
      std::uint32_t now_ms,
      std::span<const ExtendedFileRegisterRandomWriteWordItem> items,
      CompletionHandler callback,
      void* user) noexcept;

  /// \brief Starts native `Jn\\...` random word write (`1402` + device extension specification).
  [[nodiscard]] Status async_link_direct_random_write_words(
      std::uint32_t now_ms,
      std::span<const LinkDirectRandomWriteWordItem> items,
      CompletionHandler callback,
      void* user) noexcept;

  /// \brief Starts native random bit write (`1402` bit path).
  [[nodiscard]] Status async_random_write_bits(
      std::uint32_t now_ms,
      std::span<const RandomWriteBitItem> items,
      CompletionHandler callback,
      void* user) noexcept;

  /// \brief Starts native `Jn\\...` random bit write (`1402` + device extension specification).
  [[nodiscard]] Status async_link_direct_random_write_bits(
      std::uint32_t now_ms,
      std::span<const LinkDirectRandomWriteBitItem> items,
      CompletionHandler callback,
      void* user) noexcept;

  /// \brief Starts native multi-block read (`0406`).
  [[nodiscard]] Status async_multi_block_read(
      std::uint32_t now_ms,
      const MultiBlockReadRequest& request,
      std::span<std::uint16_t> out_words,
      std::span<BitValue> out_bits,
      std::span<MultiBlockReadBlockResult> out_results,
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
      std::span<std::uint16_t> out_words,
      std::span<BitValue> out_bits,
      std::span<MultiBlockReadBlockResult> out_results,
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
      std::span<std::uint32_t> out_values,
      CompletionHandler callback,
      void* user) noexcept;

  /// \brief Starts extended file-register monitor read.
  [[nodiscard]] Status async_read_extended_file_register_monitor(
      std::uint32_t now_ms,
      std::span<std::uint16_t> out_words,
      CompletionHandler callback,
      void* user) noexcept;

  /// \brief Starts host-buffer read (`0613`).
  [[nodiscard]] Status async_read_host_buffer(
      std::uint32_t now_ms,
      const HostBufferReadRequest& request,
      std::span<std::uint16_t> out_words,
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
      std::span<std::byte> out_bytes,
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

  /// \brief Starts remote RUN (`1001`).
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

  /// \brief Starts remote PAUSE (`1003`).
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
  /// The manual notes that some targets may reset before returning a response. In that case this
  /// client treats a pure response-timeout with no received bytes as success for this operation.
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

  /// \brief Starts C24 transmission-sequence initialization (`1615`).
  [[nodiscard]] Status async_initialize_c24_transmission_sequence(
      std::uint32_t now_ms,
      CompletionHandler callback,
      void* user) noexcept;

  /// \brief Starts programmable-controller CPU monitoring deregistration (`0631`).
  [[nodiscard]] Status async_deregister_cpu_monitoring(
      std::uint32_t now_ms,
      CompletionHandler callback,
      void* user) noexcept;

  /// \brief Starts loopback using hexadecimal ASCII payload bytes.
  [[nodiscard]] Status async_loopback(
      std::uint32_t now_ms,
      std::span<const char> hex_ascii,
      std::span<char> out_echoed,
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
    InitializeTransmissionSequence,
    DeregisterCpuMonitoring,
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

  [[nodiscard]] std::uint8_t expected_e1_response_subheader() const noexcept;
  [[nodiscard]] std::size_t expected_e1_success_response_data_size() const noexcept;
  [[nodiscard]] Status handle_response(std::span<const std::uint8_t> response_data) noexcept;
  void complete(Status status) noexcept;
  void clear_pending_outputs() noexcept;
  void clear_pending_copies() noexcept;

  ProtocolConfig config_ {};
  Rs485Hooks rs485_hooks_ {};
  bool configured_ = false;
  bool busy_ = false;
  bool awaiting_write_complete_ = false;
  OperationKind operation_ = OperationKind::None;
  CompletionHandler callback_ = nullptr;
  void* callback_user_ = nullptr;
  std::uint32_t response_deadline_ms_ = 0;
  std::uint32_t inter_byte_deadline_ms_ = 0;

  std::array<std::uint8_t, kMaxRequestFrameBytes> tx_frame_ {};
  std::size_t tx_frame_size_ = 0;
  std::array<std::uint8_t, kMaxResponseFrameBytes> rx_frame_ {};
  std::size_t rx_frame_size_ = 0;
  std::array<std::uint8_t, kMaxRequestDataBytes> request_data_ {};

  BatchReadWordsRequest batch_read_words_request_ {};
  ExtendedFileRegisterBatchReadWordsRequest extended_file_register_read_request_ {};
  ExtendedFileRegisterDirectBatchReadWordsRequest direct_extended_file_register_read_request_ {};
  BatchReadBitsRequest batch_read_bits_request_ {};
  UserFrameReadRequest user_frame_read_request_ {};
  QualifiedBufferWordDevice extended_batch_words_device_ {};
  std::uint16_t extended_batch_words_points_ = 0;
#if MCPROTOCOL_SERIAL_ENABLE_HOST_BUFFER_COMMANDS
  HostBufferReadRequest host_buffer_read_request_ {};
#endif
#if MCPROTOCOL_SERIAL_ENABLE_MODULE_BUFFER_COMMANDS
  ModuleBufferReadRequest module_buffer_read_request_ {};
#endif

  std::span<std::uint16_t> out_words_ {};
  std::span<BitValue> out_bits_ {};
#if MCPROTOCOL_SERIAL_ENABLE_RANDOM_COMMANDS || MCPROTOCOL_SERIAL_ENABLE_MONITOR_COMMANDS
  std::span<std::uint32_t> out_values_ {};
#endif
#if MCPROTOCOL_SERIAL_ENABLE_MODULE_BUFFER_COMMANDS
  std::span<std::byte> out_bytes_ {};
#endif
#if MCPROTOCOL_SERIAL_ENABLE_LOOPBACK_COMMANDS
  std::span<char> out_chars_ {};
#endif
#if MCPROTOCOL_SERIAL_ENABLE_MULTI_BLOCK_COMMANDS
  std::span<MultiBlockReadBlockResult> out_block_results_ {};
#endif
#if MCPROTOCOL_SERIAL_ENABLE_CPU_MODEL_COMMANDS
  CpuModelInfo* out_cpu_model_ = nullptr;
#endif
  UserFrameRegistrationData* out_user_frame_data_ = nullptr;

#if MCPROTOCOL_SERIAL_ENABLE_RANDOM_COMMANDS || MCPROTOCOL_SERIAL_ENABLE_MONITOR_COMMANDS
  std::array<RandomReadItem, kMaxRandomAccessItems> pending_random_items_ {};
  std::size_t pending_random_item_count_ = 0;
#endif
#if MCPROTOCOL_SERIAL_ENABLE_MONITOR_COMMANDS
  std::array<RandomReadItem, kMaxMonitorItems> monitor_items_ {};
  std::size_t monitor_item_count_ = 0;
  bool monitor_registered_ = false;
  std::array<ExtendedFileRegisterAddress, kMaxMonitorItems> pending_extended_file_register_items_ {};
  std::size_t pending_extended_file_register_item_count_ = 0;
  std::array<ExtendedFileRegisterAddress, kMaxMonitorItems> extended_file_register_monitor_items_ {};
  std::size_t extended_file_register_monitor_item_count_ = 0;
  bool extended_file_register_monitor_registered_ = false;
#endif
#if MCPROTOCOL_SERIAL_ENABLE_MULTI_BLOCK_COMMANDS
  std::array<MultiBlockReadBlock, kMaxMultiBlockCount> pending_multi_blocks_ {};
  std::size_t pending_multi_block_count_ = 0;
#endif
#if MCPROTOCOL_SERIAL_ENABLE_LOOPBACK_COMMANDS
  std::array<char, kMaxLoopbackBytes> pending_loopback_ {};
  std::size_t pending_loopback_size_ = 0;
#endif
};

}  // namespace mcprotocol::serial
