#pragma once

#include "mcprotocol/serial/compat/array.hpp"
#include "mcprotocol/serial/compat/cstddef.hpp"
#include "mcprotocol/serial/compat/cstdint.hpp"

#include "mcprotocol/serial/span.hpp"
#include "mcprotocol/serial/string_view_compat.hpp"

#include "mcprotocol/serial/link_direct.hpp"
#include "mcprotocol/serial/qualified_buffer.hpp"
#include "mcprotocol/serial/types.hpp"

/// \file codec.hpp
/// \brief Frame-level and command-payload-level encode/decode helpers.
///
/// This header is split into two layers:
///
/// - `FrameCodec`: wraps raw command data in the selected serial frame family and decodes full
///   response frames
/// - `CommandCodec`: builds and parses command payloads without the surrounding serial frame bytes

namespace mcprotocol::serial {

/// \brief High-level result of frame decoding.
enum class DecodeStatus : std::uint8_t {
  /// More bytes are required before a full frame can be decoded.
  Incomplete,
  /// One full frame was decoded successfully.
  Complete,
  /// The byte stream is invalid for the selected frame configuration.
  Error
};

/// \brief Raw decoded response frame before command-specific parsing.
struct RawResponseFrame {
  /// Success-with-data, success-without-data, or PLC-error classification.
  ResponseType type = ResponseType::SuccessNoData;
  /// Number of valid bytes in `response_data`.
  std::size_t response_size = 0;
  /// PLC error code when `type == ResponseType::PlcError`.
  std::uint16_t error_code = 0;
  /// Raw response payload bytes with the serial frame already removed.
  std::array<std::uint8_t, kMaxResponseFrameBytes> response_data {};
};

/// \brief Result returned by `FrameCodec::decode_response()`.
struct DecodeResult {
  /// Stream-level decode status.
  DecodeStatus status = DecodeStatus::Incomplete;
  /// Raw response frame when `status == DecodeStatus::Complete`.
  RawResponseFrame frame {};
  /// Decoder-side error when `status == DecodeStatus::Error`.
  Status error {};
  /// Number of bytes consumed from the input span.
  std::size_t bytes_consumed = 0;
  /// True when a complete response belongs to a different Format2 or route identity.
  bool response_identity_mismatch = false;
};

/// \brief Per-wire-frame identity context kept outside static protocol configuration.
///
/// Normal clients allocate Format2 block numbers automatically. `format2()` exists for raw codec,
/// test, and investigation callers that intentionally construct or decode one explicit wire frame.
class FrameCodecContext {
 public:
  [[nodiscard]] static constexpr FrameCodecContext none() noexcept {
    return FrameCodecContext(false, 0U);
  }

  [[nodiscard]] static constexpr FrameCodecContext format2(std::uint8_t block_number) noexcept {
    return FrameCodecContext(true, block_number);
  }

  [[nodiscard]] constexpr bool has_format2_block_number() const noexcept {
    return has_format2_block_number_;
  }

  [[nodiscard]] constexpr std::uint8_t format2_block_number() const noexcept {
    return format2_block_number_;
  }

 private:
  constexpr FrameCodecContext(bool has_format2_block_number, std::uint8_t format2_block_number) noexcept
      : has_format2_block_number_(has_format2_block_number),
        format2_block_number_(format2_block_number) {}

  bool has_format2_block_number_;
  std::uint8_t format2_block_number_;
};

/// \brief Returns the requested-point value from a sparse native bit result word.
///
/// On `2C`/`3C`/`4C`, native sparse bit reads (`0403`) and monitor reads (`0802`) return the
/// addressed point inside a 16-point mask word. The requested head device is represented by bit `0`
/// of that returned word.
[[nodiscard]] constexpr BitValue sparse_native_requested_bit_value(std::uint32_t raw_value) noexcept {
  return (raw_value & 0x0001U) != 0U ? true : false;
}

/// \brief Returns the raw 16-point mask word from a sparse native bit result.
///
/// Keep this raw word visible for diagnostics when the target-specific offset pattern matters.
[[nodiscard]] constexpr std::uint16_t sparse_native_mask_word(std::uint32_t raw_value) noexcept {
  return static_cast<std::uint16_t>(raw_value & 0xFFFFU);
}

/// \brief Frame-level encode/decode helper for complete serial MC frames.
///
/// Use this class when you already have a command payload and only need the outer serial frame
/// layer, or when you need to decode a stream of returned bytes before passing the payload to a
/// command-specific parser.
class FrameCodec {
 public:
  /// \brief Validates a static protocol configuration before encoding requests.
  ///
  /// This checks frame-family / code-mode compatibility, route constraints, and combinations that
  /// are compiled out by feature macros.
  [[nodiscard]] static Status validate_config(const ProtocolConfig& config) noexcept;

  /// \brief Verifies that a request payload fits the configured fixed frame capacity even when
  /// every byte eligible for binary DLE escaping expands on the wire.
  ///
  /// This is a single-request admission check. `InvalidArgument` means the requested operation is
  /// not representable by this build; it is distinct from a caller-supplied output span being too
  /// small (`BufferTooSmall`).
  [[nodiscard]] static Status validate_request_capacity(
      const ProtocolConfig& config,
      std::size_t request_data_size) noexcept;

  /// \brief Verifies that a successful response payload fits the complete receive path.
  ///
  /// Binary Format5 uses the worst case in which every unescaped payload byte is DLE and therefore
  /// occupies two wire bytes.
  [[nodiscard]] static Status validate_response_capacity(
      const ProtocolConfig& config,
      std::size_t response_data_size) noexcept;

  /// \brief Wraps command data in the configured serial frame format.
  ///
  /// `request_data` must already contain the command payload generated by `CommandCodec`.
  [[nodiscard]] static Status encode_request(
      const ProtocolConfig& config,
      mcprotocol::serial::Span<const std::uint8_t> request_data,
      mcprotocol::serial::Span<std::uint8_t> out_frame,
      std::size_t& out_size) noexcept;

  /// \brief Wraps command data using an explicit per-wire-frame identity context.
  [[nodiscard]] static Status encode_request(
      const ProtocolConfig& config,
      FrameCodecContext context,
      mcprotocol::serial::Span<const std::uint8_t> request_data,
      mcprotocol::serial::Span<std::uint8_t> out_frame,
      std::size_t& out_size) noexcept;

  /// \brief Builds a success response frame for tests and local tools.
  ///
  /// This is mainly used by tests and local validation helpers. Library users typically decode real
  /// target responses instead of constructing synthetic ones.
  [[nodiscard]] static Status encode_success_response(
      const ProtocolConfig& config,
      mcprotocol::serial::Span<const std::uint8_t> response_data,
      mcprotocol::serial::Span<std::uint8_t> out_frame,
      std::size_t& out_size) noexcept;

  [[nodiscard]] static Status encode_success_response(
      const ProtocolConfig& config,
      FrameCodecContext context,
      mcprotocol::serial::Span<const std::uint8_t> response_data,
      mcprotocol::serial::Span<std::uint8_t> out_frame,
      std::size_t& out_size) noexcept;

  /// \brief Builds a PLC-error response frame for tests and local tools.
  [[nodiscard]] static Status encode_error_response(
      const ProtocolConfig& config,
      std::uint16_t error_code,
      mcprotocol::serial::Span<std::uint8_t> out_frame,
      std::size_t& out_size) noexcept;

  [[nodiscard]] static Status encode_error_response(
      const ProtocolConfig& config,
      FrameCodecContext context,
      std::uint16_t error_code,
      mcprotocol::serial::Span<std::uint8_t> out_frame,
      std::size_t& out_size) noexcept;

  /// \brief Decodes one response frame from the front of `bytes`.
  ///
  /// The caller can use `bytes_consumed` to drop the decoded prefix and continue stream processing.
  [[nodiscard]] static DecodeResult decode_response(
      const ProtocolConfig& config,
      mcprotocol::serial::Span<const std::uint8_t> bytes) noexcept;

  /// \brief Decodes one response using an explicit per-wire-frame identity context.
  [[nodiscard]] static DecodeResult decode_response(
      const ProtocolConfig& config,
      FrameCodecContext context,
      mcprotocol::serial::Span<const std::uint8_t> bytes) noexcept;
};

/// \brief Command-payload codec helpers below the frame layer.
///
/// These helpers operate on request/response data only. They do not add or remove the surrounding
/// `C1`/`C2`/`C3`/`C4` frame bytes.
namespace CommandCodec {

/// \name Contiguous Device-Memory Read Helpers
/// @{
[[nodiscard]] Status encode_batch_read_words(
    const ProtocolConfig& config,
    const BatchReadWordsRequest& request,
    mcprotocol::serial::Span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept;

[[nodiscard]] Status encode_read_extended_file_register_words(
    const ProtocolConfig& config,
    const ExtendedFileRegisterBatchReadWordsRequest& request,
    mcprotocol::serial::Span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept;

[[nodiscard]] Status encode_direct_read_extended_file_register_words(
    const ProtocolConfig& config,
    const ExtendedFileRegisterDirectBatchReadWordsRequest& request,
    mcprotocol::serial::Span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept;

[[nodiscard]] Status encode_extended_batch_read_words(
    const ProtocolConfig& config,
    const QualifiedBufferWordDevice& device,
    std::uint16_t points,
    mcprotocol::serial::Span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept;

[[nodiscard]] Status encode_link_direct_batch_read_words(
    const ProtocolConfig& config,
    const LinkDirectDevice& device,
    std::uint16_t points,
    mcprotocol::serial::Span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept;

[[nodiscard]] Status parse_batch_read_words_response(
    const ProtocolConfig& config,
    const BatchReadWordsRequest& request,
    mcprotocol::serial::Span<const std::uint8_t> response_data,
    mcprotocol::serial::Span<std::uint16_t> out_words) noexcept;

[[nodiscard]] Status parse_read_extended_file_register_words_response(
    const ProtocolConfig& config,
    std::uint16_t points,
    mcprotocol::serial::Span<const std::uint8_t> response_data,
    mcprotocol::serial::Span<std::uint16_t> out_words) noexcept;

[[nodiscard]] Status parse_extended_batch_read_words_response(
    const ProtocolConfig& config,
    std::uint16_t points,
    mcprotocol::serial::Span<const std::uint8_t> response_data,
    mcprotocol::serial::Span<std::uint16_t> out_words) noexcept;

[[nodiscard]] Status encode_batch_read_bits(
    const ProtocolConfig& config,
    const BatchReadBitsRequest& request,
    mcprotocol::serial::Span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept;

[[nodiscard]] Status encode_link_direct_batch_read_bits(
    const ProtocolConfig& config,
    const LinkDirectDevice& device,
    std::uint16_t points,
    mcprotocol::serial::Span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept;

[[nodiscard]] Status parse_batch_read_bits_response(
    const ProtocolConfig& config,
    const BatchReadBitsRequest& request,
    mcprotocol::serial::Span<const std::uint8_t> response_data,
    mcprotocol::serial::Span<BitValue> out_bits) noexcept;
/// @}

/// \name Contiguous Device-Memory Write Helpers
/// @{
[[nodiscard]] Status encode_batch_write_words(
    const ProtocolConfig& config,
    const BatchWriteWordsRequest& request,
    mcprotocol::serial::Span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept;

[[nodiscard]] Status encode_write_extended_file_register_words(
    const ProtocolConfig& config,
    const ExtendedFileRegisterBatchWriteWordsRequest& request,
    mcprotocol::serial::Span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept;

[[nodiscard]] Status encode_direct_write_extended_file_register_words(
    const ProtocolConfig& config,
    const ExtendedFileRegisterDirectBatchWriteWordsRequest& request,
    mcprotocol::serial::Span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept;

[[nodiscard]] Status encode_link_direct_batch_write_words(
    const ProtocolConfig& config,
    const LinkDirectDevice& device,
    mcprotocol::serial::Span<const std::uint16_t> words,
    mcprotocol::serial::Span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept;

[[nodiscard]] Status encode_extended_batch_write_words(
    const ProtocolConfig& config,
    const QualifiedBufferWordDevice& device,
    mcprotocol::serial::Span<const std::uint16_t> words,
    mcprotocol::serial::Span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept;

[[nodiscard]] Status encode_batch_write_bits(
    const ProtocolConfig& config,
    const BatchWriteBitsRequest& request,
    mcprotocol::serial::Span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept;

[[nodiscard]] Status encode_link_direct_batch_write_bits(
    const ProtocolConfig& config,
    const LinkDirectDevice& device,
    mcprotocol::serial::Span<const BitValue> bits,
    mcprotocol::serial::Span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept;
/// @}

/// \name Random-Access Helpers
/// @{
[[nodiscard]] Status encode_link_direct_random_read(
    const ProtocolConfig& config,
    mcprotocol::serial::Span<const LinkDirectRandomReadWordItem> word_items,
    mcprotocol::serial::Span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept;

[[nodiscard]] Status encode_random_read(
    const ProtocolConfig& config,
    const RandomReadRequest& request,
    mcprotocol::serial::Span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept;

[[nodiscard]] Status parse_random_read_response(
    const ProtocolConfig& config,
    const RandomReadRequest& request,
    mcprotocol::serial::Span<const std::uint8_t> response_data,
    mcprotocol::serial::Span<std::uint16_t> out_words,
    mcprotocol::serial::Span<std::uint32_t> out_dwords) noexcept;

[[nodiscard]] Status encode_random_write_words(
    const ProtocolConfig& config,
    mcprotocol::serial::Span<const RandomWriteWordItem> word_items,
    mcprotocol::serial::Span<const RandomWriteDWordItem> dword_items,
    mcprotocol::serial::Span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept;

[[nodiscard]] Status encode_random_write_extended_file_register_words(
    const ProtocolConfig& config,
    mcprotocol::serial::Span<const ExtendedFileRegisterRandomWriteWordItem> items,
    mcprotocol::serial::Span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept;

[[nodiscard]] Status encode_link_direct_random_write_words(
    const ProtocolConfig& config,
    mcprotocol::serial::Span<const LinkDirectRandomWriteWordItem> items,
    mcprotocol::serial::Span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept;

[[nodiscard]] Status encode_random_write_bits(
    const ProtocolConfig& config,
    mcprotocol::serial::Span<const RandomWriteBitItem> items,
    mcprotocol::serial::Span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept;

[[nodiscard]] Status encode_link_direct_random_write_bits(
    const ProtocolConfig& config,
    mcprotocol::serial::Span<const LinkDirectRandomWriteBitItem> items,
    mcprotocol::serial::Span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept;
/// @}

/// \name Multi-Block Helpers
/// @{
[[nodiscard]] Status encode_link_direct_multi_block_read(
    const ProtocolConfig& config,
    const LinkDirectMultiBlockReadRequest& request,
    mcprotocol::serial::Span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept;

[[nodiscard]] Status encode_multi_block_read(
    const ProtocolConfig& config,
    const MultiBlockReadRequest& request,
    mcprotocol::serial::Span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept;

[[nodiscard]] Status parse_multi_block_read_response(
    const ProtocolConfig& config,
    mcprotocol::serial::Span<const MultiBlockReadBlock> blocks,
    mcprotocol::serial::Span<const std::uint8_t> response_data,
    mcprotocol::serial::Span<std::uint16_t> out_words,
    mcprotocol::serial::Span<BitValue> out_bits,
    mcprotocol::serial::Span<MultiBlockReadBlockResult> out_results) noexcept;

[[nodiscard]] Status encode_multi_block_write(
    const ProtocolConfig& config,
    const MultiBlockWriteRequest& request,
    mcprotocol::serial::Span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept;

[[nodiscard]] Status encode_link_direct_multi_block_write(
    const ProtocolConfig& config,
    const LinkDirectMultiBlockWriteRequest& request,
    mcprotocol::serial::Span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept;
/// @}

/// \name Monitor Helpers
/// @{
[[nodiscard]] Status encode_register_monitor(
    const ProtocolConfig& config,
    const MonitorRegistration& request,
    mcprotocol::serial::Span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept;

[[nodiscard]] Status encode_register_extended_file_register_monitor(
    const ProtocolConfig& config,
    const ExtendedFileRegisterMonitorRegistration& request,
    mcprotocol::serial::Span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept;

[[nodiscard]] Status encode_link_direct_register_monitor(
    const ProtocolConfig& config,
    const LinkDirectMonitorRegistration& request,
    mcprotocol::serial::Span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept;

[[nodiscard]] Status encode_read_monitor(
    const ProtocolConfig& config,
    mcprotocol::serial::Span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept;

[[nodiscard]] Status encode_read_monitor(
    const ProtocolConfig& config,
    const MonitorRegistration& registration,
    mcprotocol::serial::Span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept;

[[nodiscard]] Status encode_read_extended_file_register_monitor(
    const ProtocolConfig& config,
    mcprotocol::serial::Span<const ExtendedFileRegisterAddress> items,
    mcprotocol::serial::Span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept;

[[nodiscard]] Status parse_read_monitor_response(
    const ProtocolConfig& config,
    const MonitorRegistration& registration,
    mcprotocol::serial::Span<const std::uint8_t> response_data,
    mcprotocol::serial::Span<std::uint16_t> out_words,
    mcprotocol::serial::Span<std::uint32_t> out_dwords) noexcept;

[[nodiscard]] Status parse_read_extended_file_register_monitor_response(
    const ProtocolConfig& config,
    mcprotocol::serial::Span<const ExtendedFileRegisterAddress> items,
    mcprotocol::serial::Span<const std::uint8_t> response_data,
    mcprotocol::serial::Span<std::uint16_t> out_words) noexcept;
/// @}

/// \name Serial-Module Dedicated Helpers
/// @{
[[nodiscard]] Status encode_read_user_frame(
    const ProtocolConfig& config,
    const UserFrameReadRequest& request,
    mcprotocol::serial::Span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept;

[[nodiscard]] Status parse_read_user_frame_response(
    const ProtocolConfig& config,
    mcprotocol::serial::Span<const std::uint8_t> response_data,
    UserFrameRegistrationData& out_data) noexcept;

[[nodiscard]] Status encode_write_user_frame(
    const ProtocolConfig& config,
    const UserFrameWriteRequest& request,
    mcprotocol::serial::Span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept;

[[nodiscard]] Status encode_delete_user_frame(
    const ProtocolConfig& config,
    const UserFrameDeleteRequest& request,
    mcprotocol::serial::Span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept;

[[nodiscard]] Status encode_control_global_signal(
    const ProtocolConfig& config,
    const GlobalSignalControlRequest& request,
    mcprotocol::serial::Span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept;

[[nodiscard]] Status encode_switch_serial_module_mode(
    const ProtocolConfig& config,
    const SerialModuleModeSwitchRequest& request,
    mcprotocol::serial::Span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept;

[[nodiscard]] Status encode_initialize_transmission_sequence(
    const ProtocolConfig& config,
    mcprotocol::serial::Span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept;

/// @}

/// \name Buffer-Memory Helpers
/// @{
[[nodiscard]] Status encode_read_host_buffer(
    const ProtocolConfig& config,
    const HostBufferReadRequest& request,
    mcprotocol::serial::Span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept;

[[nodiscard]] Status parse_read_host_buffer_response(
    const ProtocolConfig& config,
    const HostBufferReadRequest& request,
    mcprotocol::serial::Span<const std::uint8_t> response_data,
    mcprotocol::serial::Span<std::uint16_t> out_words) noexcept;

[[nodiscard]] Status encode_write_host_buffer(
    const ProtocolConfig& config,
    const HostBufferWriteRequest& request,
    mcprotocol::serial::Span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept;

[[nodiscard]] Status encode_read_module_buffer(
    const ProtocolConfig& config,
    const ModuleBufferReadRequest& request,
    mcprotocol::serial::Span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept;

[[nodiscard]] Status parse_read_module_buffer_response(
    const ProtocolConfig& config,
    const ModuleBufferReadRequest& request,
    mcprotocol::serial::Span<const std::uint8_t> response_data,
    mcprotocol::serial::Span<mcprotocol::serial::Byte> out_bytes) noexcept;

[[nodiscard]] Status encode_write_module_buffer(
    const ProtocolConfig& config,
    const ModuleBufferWriteRequest& request,
    mcprotocol::serial::Span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept;
/// @}

/// \name Diagnostic And Remote-Control Helpers
/// @{
[[nodiscard]] Status encode_read_cpu_model(
    const ProtocolConfig& config,
    mcprotocol::serial::Span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept;

[[nodiscard]] Status parse_read_cpu_model_response(
    const ProtocolConfig& config,
    mcprotocol::serial::Span<const std::uint8_t> response_data,
    CpuModelInfo& out_info) noexcept;

[[nodiscard]] Status encode_remote_run(
    const ProtocolConfig& config,
    RemoteOperationMode mode,
    RemoteRunClearMode clear_mode,
    mcprotocol::serial::Span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept;

[[nodiscard]] Status encode_remote_stop(
    const ProtocolConfig& config,
    mcprotocol::serial::Span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept;

[[nodiscard]] Status encode_remote_pause(
    const ProtocolConfig& config,
    RemoteOperationMode mode,
    mcprotocol::serial::Span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept;

[[nodiscard]] Status encode_remote_latch_clear(
    const ProtocolConfig& config,
    mcprotocol::serial::Span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept;

[[nodiscard]] Status encode_remote_reset(
    const ProtocolConfig& config,
    mcprotocol::serial::Span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept;

[[nodiscard]] Status encode_unlock_remote_password(
    const ProtocolConfig& config,
    std::string_view remote_password,
    mcprotocol::serial::Span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept;

[[nodiscard]] Status encode_lock_remote_password(
    const ProtocolConfig& config,
    std::string_view remote_password,
    mcprotocol::serial::Span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept;

[[nodiscard]] Status encode_clear_error_information(
    const ProtocolConfig& config,
    mcprotocol::serial::Span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept;

[[nodiscard]] Status encode_loopback(
    const ProtocolConfig& config,
    mcprotocol::serial::Span<const char> hex_ascii,
    mcprotocol::serial::Span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept;

[[nodiscard]] Status parse_loopback_response(
    const ProtocolConfig& config,
    mcprotocol::serial::Span<const std::uint8_t> response_data,
    mcprotocol::serial::Span<char> out_echoed) noexcept;

/// \brief Converts a logical buffer-memory word address plus module offset into a byte start address.
[[nodiscard]] inline Status module_buffer_start_address(
    std::uint32_t buffer_memory_address,
    std::uint32_t module_additional_value,
    std::uint32_t& out_start_address) noexcept {
  const std::uint64_t start_address =
      (static_cast<std::uint64_t>(buffer_memory_address) * 2U) + module_additional_value;
  if (start_address > 0xFFFFFFFFULL) {
    return make_status(
        StatusCode::InvalidArgument,
        "Module-buffer start address cannot be represented in 32 bits");
  }
  out_start_address = static_cast<std::uint32_t>(start_address);
  return ok_status();
}
/// @}

}  // namespace CommandCodec

}  // namespace mcprotocol::serial
