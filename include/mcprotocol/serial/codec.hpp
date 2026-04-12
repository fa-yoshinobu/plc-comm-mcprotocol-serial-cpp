#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "mcprotocol/serial/span_compat.hpp"

#include "mcprotocol/serial/link_direct.hpp"
#include "mcprotocol/serial/qualified_buffer.hpp"
#include "mcprotocol/serial/types.hpp"

namespace mcprotocol::serial {

/// \brief High-level result of frame decoding.
enum class DecodeStatus : std::uint8_t {
  Incomplete,
  Complete,
  Error
};

/// \brief Raw decoded response frame before command-specific parsing.
struct RawResponseFrame {
  ResponseType type = ResponseType::SuccessNoData;
  std::size_t response_size = 0;
  std::uint16_t error_code = 0;
  std::array<std::uint8_t, kMaxResponseFrameBytes> response_data {};
};

/// \brief Result returned by `FrameCodec::decode_response()`.
struct DecodeResult {
  DecodeStatus status = DecodeStatus::Incomplete;
  RawResponseFrame frame {};
  Status error {};
  std::size_t bytes_consumed = 0;
};

/// \brief Frame-level encode/decode helper for complete serial MC frames.
class FrameCodec {
 public:
  /// \brief Validates a static protocol configuration before encoding requests.
  [[nodiscard]] static Status validate_config(const ProtocolConfig& config) noexcept;

  /// \brief Wraps command data in the configured serial frame format.
  [[nodiscard]] static Status encode_request(
      const ProtocolConfig& config,
      std::span<const std::uint8_t> request_data,
      std::span<std::uint8_t> out_frame,
      std::size_t& out_size) noexcept;

  /// \brief Builds a success response frame for tests and local tools.
  [[nodiscard]] static Status encode_success_response(
      const ProtocolConfig& config,
      std::span<const std::uint8_t> response_data,
      std::span<std::uint8_t> out_frame,
      std::size_t& out_size) noexcept;

  /// \brief Builds a PLC-error response frame for tests and local tools.
  [[nodiscard]] static Status encode_error_response(
      const ProtocolConfig& config,
      std::uint16_t error_code,
      std::span<std::uint8_t> out_frame,
      std::size_t& out_size) noexcept;

  /// \brief Decodes one response frame from the front of `bytes`.
  [[nodiscard]] static DecodeResult decode_response(
      const ProtocolConfig& config,
      std::span<const std::uint8_t> bytes) noexcept;
};

/// \brief Command-payload codec helpers below the frame layer.
///
/// These helpers operate on request/response data only. They do not add or remove the surrounding
/// `C1`/`C2`/`C3`/`C4` frame bytes.
namespace CommandCodec {

[[nodiscard]] Status encode_batch_read_words(
    const ProtocolConfig& config,
    const BatchReadWordsRequest& request,
    std::span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept;

[[nodiscard]] Status encode_extended_batch_read_words(
    const ProtocolConfig& config,
    const QualifiedBufferWordDevice& device,
    std::uint16_t points,
    std::span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept;

[[nodiscard]] Status encode_link_direct_batch_read_words(
    const ProtocolConfig& config,
    const LinkDirectDevice& device,
    std::uint16_t points,
    std::span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept;

[[nodiscard]] Status parse_batch_read_words_response(
    const ProtocolConfig& config,
    const BatchReadWordsRequest& request,
    std::span<const std::uint8_t> response_data,
    std::span<std::uint16_t> out_words) noexcept;

[[nodiscard]] Status parse_extended_batch_read_words_response(
    const ProtocolConfig& config,
    std::uint16_t points,
    std::span<const std::uint8_t> response_data,
    std::span<std::uint16_t> out_words) noexcept;

[[nodiscard]] Status encode_batch_read_bits(
    const ProtocolConfig& config,
    const BatchReadBitsRequest& request,
    std::span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept;

[[nodiscard]] Status encode_link_direct_batch_read_bits(
    const ProtocolConfig& config,
    const LinkDirectDevice& device,
    std::uint16_t points,
    std::span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept;

[[nodiscard]] Status parse_batch_read_bits_response(
    const ProtocolConfig& config,
    const BatchReadBitsRequest& request,
    std::span<const std::uint8_t> response_data,
    std::span<BitValue> out_bits) noexcept;

[[nodiscard]] Status encode_batch_write_words(
    const ProtocolConfig& config,
    const BatchWriteWordsRequest& request,
    std::span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept;

[[nodiscard]] Status encode_link_direct_batch_write_words(
    const ProtocolConfig& config,
    const LinkDirectDevice& device,
    std::span<const std::uint16_t> words,
    std::span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept;

[[nodiscard]] Status encode_extended_batch_write_words(
    const ProtocolConfig& config,
    const QualifiedBufferWordDevice& device,
    std::span<const std::uint16_t> words,
    std::span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept;

[[nodiscard]] Status encode_batch_write_bits(
    const ProtocolConfig& config,
    const BatchWriteBitsRequest& request,
    std::span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept;

[[nodiscard]] Status encode_link_direct_batch_write_bits(
    const ProtocolConfig& config,
    const LinkDirectDevice& device,
    std::span<const BitValue> bits,
    std::span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept;

[[nodiscard]] Status encode_random_read(
    const ProtocolConfig& config,
    const RandomReadRequest& request,
    std::span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept;

[[nodiscard]] Status parse_random_read_response(
    const ProtocolConfig& config,
    std::span<const RandomReadItem> items,
    std::span<const std::uint8_t> response_data,
    std::span<std::uint32_t> out_values) noexcept;

[[nodiscard]] Status encode_random_write_words(
    const ProtocolConfig& config,
    std::span<const RandomWriteWordItem> items,
    std::span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept;

[[nodiscard]] Status encode_random_write_bits(
    const ProtocolConfig& config,
    std::span<const RandomWriteBitItem> items,
    std::span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept;

[[nodiscard]] Status encode_multi_block_read(
    const ProtocolConfig& config,
    const MultiBlockReadRequest& request,
    std::span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept;

[[nodiscard]] Status parse_multi_block_read_response(
    const ProtocolConfig& config,
    std::span<const MultiBlockReadBlock> blocks,
    std::span<const std::uint8_t> response_data,
    std::span<std::uint16_t> out_words,
    std::span<BitValue> out_bits,
    std::span<MultiBlockReadBlockResult> out_results) noexcept;

[[nodiscard]] Status encode_multi_block_write(
    const ProtocolConfig& config,
    const MultiBlockWriteRequest& request,
    std::span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept;

[[nodiscard]] Status encode_register_monitor(
    const ProtocolConfig& config,
    const MonitorRegistration& request,
    std::span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept;

[[nodiscard]] Status encode_read_monitor(
    const ProtocolConfig& config,
    std::span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept;

[[nodiscard]] Status parse_read_monitor_response(
    const ProtocolConfig& config,
    std::span<const RandomReadItem> items,
    std::span<const std::uint8_t> response_data,
    std::span<std::uint32_t> out_values) noexcept;

[[nodiscard]] Status encode_read_host_buffer(
    const ProtocolConfig& config,
    const HostBufferReadRequest& request,
    std::span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept;

[[nodiscard]] Status parse_read_host_buffer_response(
    const ProtocolConfig& config,
    const HostBufferReadRequest& request,
    std::span<const std::uint8_t> response_data,
    std::span<std::uint16_t> out_words) noexcept;

[[nodiscard]] Status encode_write_host_buffer(
    const ProtocolConfig& config,
    const HostBufferWriteRequest& request,
    std::span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept;

[[nodiscard]] Status encode_read_module_buffer(
    const ProtocolConfig& config,
    const ModuleBufferReadRequest& request,
    std::span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept;

[[nodiscard]] Status parse_read_module_buffer_response(
    const ProtocolConfig& config,
    const ModuleBufferReadRequest& request,
    std::span<const std::uint8_t> response_data,
    std::span<std::byte> out_bytes) noexcept;

[[nodiscard]] Status encode_write_module_buffer(
    const ProtocolConfig& config,
    const ModuleBufferWriteRequest& request,
    std::span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept;

[[nodiscard]] Status encode_read_cpu_model(
    const ProtocolConfig& config,
    std::span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept;

[[nodiscard]] Status parse_read_cpu_model_response(
    const ProtocolConfig& config,
    std::span<const std::uint8_t> response_data,
    CpuModelInfo& out_info) noexcept;

[[nodiscard]] Status encode_loopback(
    const ProtocolConfig& config,
    std::span<const char> hex_ascii,
    std::span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept;

[[nodiscard]] Status parse_loopback_response(
    const ProtocolConfig& config,
    std::span<const std::uint8_t> response_data,
    std::span<char> out_echoed) noexcept;

[[nodiscard]] constexpr std::uint32_t module_buffer_start_address(
    std::uint32_t buffer_memory_address,
    std::uint32_t module_additional_value) noexcept {
  return (buffer_memory_address * 2U) + module_additional_value;
}

}  // namespace CommandCodec

}  // namespace mcprotocol::serial
