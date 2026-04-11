#include "mcprotocol/serial/client.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace mcprotocol::serial {
namespace {

[[nodiscard]] constexpr Status timeout_status() noexcept {
  return make_status(StatusCode::Timeout, "Timed out while waiting for a response");
}

[[nodiscard]] constexpr Status cancelled_status() noexcept {
  return make_status(StatusCode::Cancelled, "The active request was cancelled");
}

[[nodiscard]] constexpr Status feature_disabled(const char* message) noexcept {
  return make_status(StatusCode::UnsupportedConfiguration, message);
}

[[nodiscard]] std::span<const std::uint8_t> as_u8_span(std::span<const std::byte> bytes) noexcept {
  return {
      reinterpret_cast<const std::uint8_t*>(bytes.data()),
      bytes.size(),
  };
}

[[nodiscard]] std::span<const std::byte> as_const_byte_span(
    const std::array<std::uint8_t, kMaxRequestFrameBytes>& bytes,
    std::size_t size) noexcept {
  return {
      reinterpret_cast<const std::byte*>(bytes.data()),
      size,
  };
}

[[nodiscard]] bool is_response_start_byte(const ProtocolConfig& config, std::uint8_t byte) noexcept {
  if (config.code_mode == CodeMode::Ascii) {
    if (config.ascii_format == AsciiFormat::Format3) {
      return byte == 0x02U;
    }
    return byte == 0x02U || byte == 0x06U || byte == 0x15U;
  }
  return byte == 0x10U;
}

void discard_rx_prefix(
    std::array<std::uint8_t, kMaxResponseFrameBytes>& buffer,
    std::size_t& size,
    std::size_t prefix_size) noexcept {
  if (prefix_size == 0U || prefix_size > size) {
    return;
  }
  const std::size_t remaining = size - prefix_size;
  std::memmove(buffer.data(), buffer.data() + prefix_size, remaining);
  size = remaining;
}

struct StreamDecodeResult {
  DecodeStatus status = DecodeStatus::Incomplete;
  DecodeResult decode {};
  std::size_t discard_prefix = 0;
};

[[nodiscard]] StreamDecodeResult decode_stream_buffer(
    const ProtocolConfig& config,
    std::span<const std::uint8_t> bytes) noexcept {
  StreamDecodeResult result {};
  if (bytes.empty()) {
    return result;
  }

  bool saw_candidate = false;
  bool first_candidate_incomplete = false;
  bool have_error = false;
  DecodeResult first_error {};

  for (std::size_t offset = 0; offset < bytes.size(); ++offset) {
    if (!is_response_start_byte(config, bytes[offset])) {
      continue;
    }

    saw_candidate = true;
    if (offset != 0U && result.discard_prefix == 0U) {
      result.discard_prefix = offset;
    }

    DecodeResult candidate = FrameCodec::decode_response(config, bytes.subspan(offset));
    if (candidate.status == DecodeStatus::Complete) {
      candidate.bytes_consumed += offset;
      result.status = DecodeStatus::Complete;
      result.decode = candidate;
      result.discard_prefix = offset;
      return result;
    }

    if (candidate.status == DecodeStatus::Incomplete) {
      if (offset == 0U) {
        first_candidate_incomplete = true;
        break;
      }
      continue;
    }

    if (!have_error) {
      candidate.bytes_consumed += offset;
      first_error = candidate;
      have_error = true;
    }
  }

  if (!saw_candidate) {
    result.discard_prefix = bytes.size();
    return result;
  }

  if (result.discard_prefix != 0U) {
    return result;
  }

  if (!first_candidate_incomplete && have_error) {
    result.status = DecodeStatus::Error;
    result.decode = first_error;
  }
  return result;
}

}  // namespace

Status MelsecSerialClient::configure(const ProtocolConfig& config) noexcept {
  if (busy_) {
    return make_status(StatusCode::Busy, "Cannot reconfigure while a request is in flight");
  }
  const Status status = FrameCodec::validate_config(config);
  if (!status.ok()) {
    return status;
  }
  config_ = config;
  configured_ = true;
  return ok_status();
}

void MelsecSerialClient::set_rs485_hooks(const Rs485Hooks& hooks) noexcept {
  rs485_hooks_ = hooks;
}

bool MelsecSerialClient::busy() const noexcept {
  return busy_;
}

std::span<const std::byte> MelsecSerialClient::pending_tx_frame() const noexcept {
  return as_const_byte_span(tx_frame_, tx_frame_size_);
}

Status MelsecSerialClient::notify_tx_complete(
    std::uint32_t now_ms,
    Status transport_status) noexcept {
  if (!busy_ || !awaiting_write_complete_) {
    return make_status(StatusCode::InvalidArgument, "No pending transmit completion is expected");
  }

  awaiting_write_complete_ = false;
  if (rs485_hooks_.on_tx_end != nullptr) {
    rs485_hooks_.on_tx_end(rs485_hooks_.user);
  }

  if (!transport_status.ok()) {
    complete(transport_status);
    return ok_status();
  }

  response_deadline_ms_ = now_ms + config_.timeout.response_timeout_ms;
  inter_byte_deadline_ms_ = now_ms + config_.timeout.inter_byte_timeout_ms;
  return ok_status();
}

void MelsecSerialClient::on_rx_bytes(
    std::uint32_t now_ms,
    std::span<const std::byte> bytes) noexcept {
  if (!busy_ || awaiting_write_complete_ || bytes.empty()) {
    return;
  }

  const auto incoming = as_u8_span(bytes);
  if ((rx_frame_size_ + incoming.size()) > rx_frame_.size()) {
    complete(make_status(StatusCode::BufferTooSmall, "Receive frame buffer overflow"));
    return;
  }

  std::memcpy(rx_frame_.data() + rx_frame_size_, incoming.data(), incoming.size());
  rx_frame_size_ += incoming.size();
  inter_byte_deadline_ms_ = now_ms + config_.timeout.inter_byte_timeout_ms;

  for (;;) {
    const StreamDecodeResult stream_decode = decode_stream_buffer(
        config_,
        std::span<const std::uint8_t>(rx_frame_.data(), rx_frame_size_));
    if (stream_decode.discard_prefix != 0U) {
      discard_rx_prefix(rx_frame_, rx_frame_size_, stream_decode.discard_prefix);
      if (rx_frame_size_ == 0U) {
        return;
      }
      continue;
    }

    if (stream_decode.status == DecodeStatus::Incomplete) {
      return;
    }

    if (stream_decode.status == DecodeStatus::Error) {
      complete(stream_decode.decode.error);
      return;
    }

    if (stream_decode.decode.frame.type == ResponseType::PlcError) {
      complete(make_status(
          StatusCode::PlcError,
          "PLC returned an error",
          stream_decode.decode.frame.error_code));
      return;
    }

    const Status parse_status = handle_response(
        std::span<const std::uint8_t>(
            stream_decode.decode.frame.response_data.data(),
            stream_decode.decode.frame.response_size));
    complete(parse_status);
    return;
  }
}

void MelsecSerialClient::poll(std::uint32_t now_ms) noexcept {
  if (!busy_ || awaiting_write_complete_) {
    return;
  }

  if (rx_frame_size_ == 0U) {
    if (now_ms >= response_deadline_ms_) {
      complete(timeout_status());
    }
    return;
  }

  if (now_ms >= inter_byte_deadline_ms_) {
    complete(make_status(StatusCode::Timeout, "Timed out while waiting for the rest of the response"));
    return;
  }

  if (now_ms >= response_deadline_ms_) {
    complete(timeout_status());
  }
}

void MelsecSerialClient::cancel() noexcept {
  if (!busy_) {
    return;
  }
  complete(cancelled_status());
}

Status MelsecSerialClient::start_request(
    std::uint32_t now_ms,
    OperationKind operation,
    std::size_t request_data_size,
    CompletionHandler callback,
    void* user) noexcept {
  (void)now_ms;

  if (!configured_) {
    return make_status(StatusCode::InvalidArgument, "Client must be configured before use");
  }
  if (busy_) {
    return make_status(StatusCode::Busy, "Only one request can be in flight at a time");
  }
  if (callback == nullptr) {
    return make_status(StatusCode::InvalidArgument, "Completion callback must not be null");
  }

  std::size_t encoded_size = 0;
  const Status encode_status = FrameCodec::encode_request(
      config_,
      std::span<const std::uint8_t>(request_data_.data(), request_data_size),
      tx_frame_,
      encoded_size);
  if (!encode_status.ok()) {
    return encode_status;
  }

  tx_frame_size_ = encoded_size;
  rx_frame_size_ = 0;
  response_deadline_ms_ = 0;
  inter_byte_deadline_ms_ = 0;
  callback_ = callback;
  callback_user_ = user;
  busy_ = true;
  awaiting_write_complete_ = true;
  operation_ = operation;

  if (rs485_hooks_.on_tx_begin != nullptr) {
    rs485_hooks_.on_tx_begin(rs485_hooks_.user);
  }
  return ok_status();
}

Status MelsecSerialClient::handle_response(std::span<const std::uint8_t> response_data) noexcept {
  switch (operation_) {
    case OperationKind::BatchReadWords:
      return CommandCodec::parse_batch_read_words_response(
          config_,
          batch_read_words_request_,
          response_data,
          out_words_);
    case OperationKind::ExtendedBatchReadWords:
      return CommandCodec::parse_extended_batch_read_words_response(
          config_,
          extended_batch_words_points_,
          response_data,
          out_words_);
    case OperationKind::BatchReadBits:
      return CommandCodec::parse_batch_read_bits_response(
          config_,
          batch_read_bits_request_,
          response_data,
          out_bits_);
    case OperationKind::BatchWriteWords:
    case OperationKind::BatchWriteBits:
    case OperationKind::ExtendedBatchWriteWords:
#if MCPROTOCOL_SERIAL_ENABLE_RANDOM_COMMANDS
    case OperationKind::RandomWriteWords:
    case OperationKind::RandomWriteBits:
#endif
#if MCPROTOCOL_SERIAL_ENABLE_MULTI_BLOCK_COMMANDS
    case OperationKind::MultiBlockWrite:
#endif
#if MCPROTOCOL_SERIAL_ENABLE_HOST_BUFFER_COMMANDS
    case OperationKind::WriteHostBuffer:
#endif
#if MCPROTOCOL_SERIAL_ENABLE_MODULE_BUFFER_COMMANDS
    case OperationKind::WriteModuleBuffer:
#endif
      if (!response_data.empty()) {
        return make_status(StatusCode::Parse, "Write response must not contain response data");
      }
      return ok_status();
#if MCPROTOCOL_SERIAL_ENABLE_RANDOM_COMMANDS
    case OperationKind::RandomRead:
      return CommandCodec::parse_random_read_response(
          config_,
          std::span<const RandomReadItem>(pending_random_items_.data(), pending_random_item_count_),
          response_data,
          out_values_);
#endif
#if MCPROTOCOL_SERIAL_ENABLE_MULTI_BLOCK_COMMANDS
    case OperationKind::MultiBlockRead:
      return CommandCodec::parse_multi_block_read_response(
          config_,
          std::span<const MultiBlockReadBlock>(pending_multi_blocks_.data(), pending_multi_block_count_),
          response_data,
          out_words_,
          out_bits_,
          out_block_results_);
#endif
#if MCPROTOCOL_SERIAL_ENABLE_MONITOR_COMMANDS
    case OperationKind::RegisterMonitor:
      if (!response_data.empty()) {
        return make_status(StatusCode::Parse, "Monitor registration response must not contain response data");
      }
      std::copy_n(pending_random_items_.begin(), pending_random_item_count_, monitor_items_.begin());
      monitor_item_count_ = pending_random_item_count_;
      monitor_registered_ = true;
      return ok_status();
    case OperationKind::ReadMonitor:
      return CommandCodec::parse_read_monitor_response(
          config_,
          std::span<const RandomReadItem>(monitor_items_.data(), monitor_item_count_),
          response_data,
          out_values_);
#endif
#if MCPROTOCOL_SERIAL_ENABLE_HOST_BUFFER_COMMANDS
    case OperationKind::ReadHostBuffer:
      return CommandCodec::parse_read_host_buffer_response(
          config_,
          host_buffer_read_request_,
          response_data,
          out_words_);
#endif
#if MCPROTOCOL_SERIAL_ENABLE_MODULE_BUFFER_COMMANDS
    case OperationKind::ReadModuleBuffer:
      return CommandCodec::parse_read_module_buffer_response(
          config_,
          module_buffer_read_request_,
          response_data,
          out_bytes_);
#endif
#if MCPROTOCOL_SERIAL_ENABLE_CPU_MODEL_COMMANDS
    case OperationKind::ReadCpuModel:
      return CommandCodec::parse_read_cpu_model_response(
          config_,
          response_data,
          *out_cpu_model_);
#endif
#if MCPROTOCOL_SERIAL_ENABLE_LOOPBACK_COMMANDS
    case OperationKind::Loopback: {
      const Status parse_status = CommandCodec::parse_loopback_response(
          config_,
          response_data,
          out_chars_);
      if (!parse_status.ok()) {
        return parse_status;
      }
      if (out_chars_.size() < pending_loopback_size_) {
        return make_status(StatusCode::BufferTooSmall, "Loopback output buffer is too small");
      }
      for (std::size_t index = 0; index < pending_loopback_size_; ++index) {
        if (out_chars_[index] != pending_loopback_[index]) {
          return make_status(StatusCode::Parse, "Loopback response does not match the request");
        }
      }
      return ok_status();
    }
#endif
    case OperationKind::None:
      break;
  }

  return make_status(StatusCode::Parse, "Unknown operation kind");
}

void MelsecSerialClient::complete(Status status) noexcept {
  CompletionHandler callback = callback_;
  void* user = callback_user_;

  callback_ = nullptr;
  callback_user_ = nullptr;
  busy_ = false;
  awaiting_write_complete_ = false;
  operation_ = OperationKind::None;
  tx_frame_size_ = 0;
  rx_frame_size_ = 0;
  response_deadline_ms_ = 0;
  inter_byte_deadline_ms_ = 0;
  clear_pending_outputs();
  clear_pending_copies();

  if (callback != nullptr) {
    callback(user, status);
  }
}

void MelsecSerialClient::clear_pending_outputs() noexcept {
  out_words_ = {};
  out_bits_ = {};
#if MCPROTOCOL_SERIAL_ENABLE_RANDOM_COMMANDS || MCPROTOCOL_SERIAL_ENABLE_MONITOR_COMMANDS
  out_values_ = {};
#endif
#if MCPROTOCOL_SERIAL_ENABLE_MODULE_BUFFER_COMMANDS
  out_bytes_ = {};
#endif
#if MCPROTOCOL_SERIAL_ENABLE_LOOPBACK_COMMANDS
  out_chars_ = {};
#endif
#if MCPROTOCOL_SERIAL_ENABLE_MULTI_BLOCK_COMMANDS
  out_block_results_ = {};
#endif
#if MCPROTOCOL_SERIAL_ENABLE_CPU_MODEL_COMMANDS
  out_cpu_model_ = nullptr;
#endif
  batch_read_words_request_ = {};
  batch_read_bits_request_ = {};
  extended_batch_words_device_ = {};
  extended_batch_words_points_ = 0U;
#if MCPROTOCOL_SERIAL_ENABLE_HOST_BUFFER_COMMANDS
  host_buffer_read_request_ = {};
#endif
#if MCPROTOCOL_SERIAL_ENABLE_MODULE_BUFFER_COMMANDS
  module_buffer_read_request_ = {};
#endif
}

void MelsecSerialClient::clear_pending_copies() noexcept {
#if MCPROTOCOL_SERIAL_ENABLE_RANDOM_COMMANDS || MCPROTOCOL_SERIAL_ENABLE_MONITOR_COMMANDS
  pending_random_item_count_ = 0;
#endif
#if MCPROTOCOL_SERIAL_ENABLE_MULTI_BLOCK_COMMANDS
  pending_multi_block_count_ = 0;
#endif
#if MCPROTOCOL_SERIAL_ENABLE_LOOPBACK_COMMANDS
  pending_loopback_size_ = 0;
#endif
}

Status MelsecSerialClient::async_batch_read_words(
    std::uint32_t now_ms,
    const BatchReadWordsRequest& request,
    std::span<std::uint16_t> out_words,
    CompletionHandler callback,
    void* user) noexcept {
  batch_read_words_request_ = request;
  out_words_ = out_words;
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_batch_read_words(config_, request, request_data_, request_size);
  if (!status.ok()) {
    clear_pending_outputs();
    return status;
  }
  return start_request(now_ms, OperationKind::BatchReadWords, request_size, callback, user);
}

Status MelsecSerialClient::async_batch_read_bits(
    std::uint32_t now_ms,
    const BatchReadBitsRequest& request,
    std::span<BitValue> out_bits,
    CompletionHandler callback,
    void* user) noexcept {
  batch_read_bits_request_ = request;
  out_bits_ = out_bits;
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_batch_read_bits(config_, request, request_data_, request_size);
  if (!status.ok()) {
    clear_pending_outputs();
    return status;
  }
  return start_request(now_ms, OperationKind::BatchReadBits, request_size, callback, user);
}

Status MelsecSerialClient::async_batch_write_words(
    std::uint32_t now_ms,
    const BatchWriteWordsRequest& request,
    CompletionHandler callback,
    void* user) noexcept {
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_batch_write_words(config_, request, request_data_, request_size);
  if (!status.ok()) {
    return status;
  }
  return start_request(now_ms, OperationKind::BatchWriteWords, request_size, callback, user);
}

Status MelsecSerialClient::async_batch_write_bits(
    std::uint32_t now_ms,
    const BatchWriteBitsRequest& request,
    CompletionHandler callback,
    void* user) noexcept {
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_batch_write_bits(config_, request, request_data_, request_size);
  if (!status.ok()) {
    return status;
  }
  return start_request(now_ms, OperationKind::BatchWriteBits, request_size, callback, user);
}

Status MelsecSerialClient::async_extended_batch_read_words(
    std::uint32_t now_ms,
    const QualifiedBufferWordDevice& device,
    std::uint16_t points,
    std::span<std::uint16_t> out_words,
    CompletionHandler callback,
    void* user) noexcept {
  extended_batch_words_device_ = device;
  extended_batch_words_points_ = points;
  out_words_ = out_words;
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_extended_batch_read_words(
      config_,
      device,
      points,
      request_data_,
      request_size);
  if (!status.ok()) {
    clear_pending_outputs();
    return status;
  }
  return start_request(now_ms, OperationKind::ExtendedBatchReadWords, request_size, callback, user);
}

Status MelsecSerialClient::async_extended_batch_write_words(
    std::uint32_t now_ms,
    const QualifiedBufferWordDevice& device,
    std::span<const std::uint16_t> words,
    CompletionHandler callback,
    void* user) noexcept {
  extended_batch_words_device_ = device;
  extended_batch_words_points_ = static_cast<std::uint16_t>(words.size());
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_extended_batch_write_words(
      config_,
      device,
      words,
      request_data_,
      request_size);
  if (!status.ok()) {
    clear_pending_outputs();
    return status;
  }
  return start_request(now_ms, OperationKind::ExtendedBatchWriteWords, request_size, callback, user);
}

Status MelsecSerialClient::async_random_read(
    std::uint32_t now_ms,
    const RandomReadRequest& request,
    std::span<std::uint32_t> out_values,
    CompletionHandler callback,
    void* user) noexcept {
#if MCPROTOCOL_SERIAL_ENABLE_RANDOM_COMMANDS
  if (request.items.size() > pending_random_items_.size()) {
    return make_status(StatusCode::InvalidArgument, "Random read item count exceeds the client limit");
  }
  pending_random_item_count_ = request.items.size();
  std::copy(request.items.begin(), request.items.end(), pending_random_items_.begin());
  out_values_ = out_values;

  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_random_read(config_, request, request_data_, request_size);
  if (!status.ok()) {
    clear_pending_outputs();
    clear_pending_copies();
    return status;
  }
  return start_request(now_ms, OperationKind::RandomRead, request_size, callback, user);
#else
  (void)now_ms;
  (void)request;
  (void)out_values;
  (void)callback;
  (void)user;
  return feature_disabled("Random commands are disabled at build time");
#endif
}

Status MelsecSerialClient::async_random_write_words(
    std::uint32_t now_ms,
    std::span<const RandomWriteWordItem> items,
    CompletionHandler callback,
    void* user) noexcept {
#if MCPROTOCOL_SERIAL_ENABLE_RANDOM_COMMANDS
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_random_write_words(config_, items, request_data_, request_size);
  if (!status.ok()) {
    return status;
  }
  return start_request(now_ms, OperationKind::RandomWriteWords, request_size, callback, user);
#else
  (void)now_ms;
  (void)items;
  (void)callback;
  (void)user;
  return feature_disabled("Random commands are disabled at build time");
#endif
}

Status MelsecSerialClient::async_random_write_bits(
    std::uint32_t now_ms,
    std::span<const RandomWriteBitItem> items,
    CompletionHandler callback,
    void* user) noexcept {
#if MCPROTOCOL_SERIAL_ENABLE_RANDOM_COMMANDS
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_random_write_bits(config_, items, request_data_, request_size);
  if (!status.ok()) {
    return status;
  }
  return start_request(now_ms, OperationKind::RandomWriteBits, request_size, callback, user);
#else
  (void)now_ms;
  (void)items;
  (void)callback;
  (void)user;
  return feature_disabled("Random commands are disabled at build time");
#endif
}

Status MelsecSerialClient::async_multi_block_read(
    std::uint32_t now_ms,
    const MultiBlockReadRequest& request,
    std::span<std::uint16_t> out_words,
    std::span<BitValue> out_bits,
    std::span<MultiBlockReadBlockResult> out_results,
    CompletionHandler callback,
    void* user) noexcept {
#if MCPROTOCOL_SERIAL_ENABLE_MULTI_BLOCK_COMMANDS
  if (request.blocks.size() > pending_multi_blocks_.size()) {
    return make_status(StatusCode::InvalidArgument, "Multi-block read block count exceeds the client limit");
  }
  pending_multi_block_count_ = request.blocks.size();
  std::copy(request.blocks.begin(), request.blocks.end(), pending_multi_blocks_.begin());
  out_words_ = out_words;
  out_bits_ = out_bits;
  out_block_results_ = out_results;

  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_multi_block_read(config_, request, request_data_, request_size);
  if (!status.ok()) {
    clear_pending_outputs();
    clear_pending_copies();
    return status;
  }
  return start_request(now_ms, OperationKind::MultiBlockRead, request_size, callback, user);
#else
  (void)now_ms;
  (void)request;
  (void)out_words;
  (void)out_bits;
  (void)out_results;
  (void)callback;
  (void)user;
  return feature_disabled("Multi-block commands are disabled at build time");
#endif
}

Status MelsecSerialClient::async_multi_block_write(
    std::uint32_t now_ms,
    const MultiBlockWriteRequest& request,
    CompletionHandler callback,
    void* user) noexcept {
#if MCPROTOCOL_SERIAL_ENABLE_MULTI_BLOCK_COMMANDS
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_multi_block_write(config_, request, request_data_, request_size);
  if (!status.ok()) {
    return status;
  }
  return start_request(now_ms, OperationKind::MultiBlockWrite, request_size, callback, user);
#else
  (void)now_ms;
  (void)request;
  (void)callback;
  (void)user;
  return feature_disabled("Multi-block commands are disabled at build time");
#endif
}

Status MelsecSerialClient::async_register_monitor(
    std::uint32_t now_ms,
    const MonitorRegistration& request,
    CompletionHandler callback,
    void* user) noexcept {
#if MCPROTOCOL_SERIAL_ENABLE_MONITOR_COMMANDS
  if (request.items.size() > pending_random_items_.size()) {
    return make_status(StatusCode::InvalidArgument, "Monitor item count exceeds the client limit");
  }
  pending_random_item_count_ = request.items.size();
  std::copy(request.items.begin(), request.items.end(), pending_random_items_.begin());

  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_register_monitor(config_, request, request_data_, request_size);
  if (!status.ok()) {
    clear_pending_copies();
    return status;
  }
  return start_request(now_ms, OperationKind::RegisterMonitor, request_size, callback, user);
#else
  (void)now_ms;
  (void)request;
  (void)callback;
  (void)user;
  return feature_disabled("Monitor commands are disabled at build time");
#endif
}

Status MelsecSerialClient::async_read_monitor(
    std::uint32_t now_ms,
    std::span<std::uint32_t> out_values,
    CompletionHandler callback,
    void* user) noexcept {
#if MCPROTOCOL_SERIAL_ENABLE_MONITOR_COMMANDS
  if (!monitor_registered_) {
    return make_status(StatusCode::InvalidArgument, "Monitor data has not been registered");
  }
  if (out_values.size() < monitor_item_count_) {
    return make_status(StatusCode::BufferTooSmall, "Monitor output buffer is too small");
  }
  out_values_ = out_values;

  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_read_monitor(config_, request_data_, request_size);
  if (!status.ok()) {
    clear_pending_outputs();
    return status;
  }
  return start_request(now_ms, OperationKind::ReadMonitor, request_size, callback, user);
#else
  (void)now_ms;
  (void)out_values;
  (void)callback;
  (void)user;
  return feature_disabled("Monitor commands are disabled at build time");
#endif
}

Status MelsecSerialClient::async_read_host_buffer(
    std::uint32_t now_ms,
    const HostBufferReadRequest& request,
    std::span<std::uint16_t> out_words,
    CompletionHandler callback,
    void* user) noexcept {
#if MCPROTOCOL_SERIAL_ENABLE_HOST_BUFFER_COMMANDS
  host_buffer_read_request_ = request;
  out_words_ = out_words;

  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_read_host_buffer(config_, request, request_data_, request_size);
  if (!status.ok()) {
    clear_pending_outputs();
    return status;
  }
  return start_request(now_ms, OperationKind::ReadHostBuffer, request_size, callback, user);
#else
  (void)now_ms;
  (void)request;
  (void)out_words;
  (void)callback;
  (void)user;
  return feature_disabled("Host-buffer commands are disabled at build time");
#endif
}

Status MelsecSerialClient::async_write_host_buffer(
    std::uint32_t now_ms,
    const HostBufferWriteRequest& request,
    CompletionHandler callback,
    void* user) noexcept {
#if MCPROTOCOL_SERIAL_ENABLE_HOST_BUFFER_COMMANDS
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_write_host_buffer(config_, request, request_data_, request_size);
  if (!status.ok()) {
    return status;
  }
  return start_request(now_ms, OperationKind::WriteHostBuffer, request_size, callback, user);
#else
  (void)now_ms;
  (void)request;
  (void)callback;
  (void)user;
  return feature_disabled("Host-buffer commands are disabled at build time");
#endif
}

Status MelsecSerialClient::async_read_module_buffer(
    std::uint32_t now_ms,
    const ModuleBufferReadRequest& request,
    std::span<std::byte> out_bytes,
    CompletionHandler callback,
    void* user) noexcept {
#if MCPROTOCOL_SERIAL_ENABLE_MODULE_BUFFER_COMMANDS
  module_buffer_read_request_ = request;
  out_bytes_ = out_bytes;

  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_read_module_buffer(config_, request, request_data_, request_size);
  if (!status.ok()) {
    clear_pending_outputs();
    return status;
  }
  return start_request(now_ms, OperationKind::ReadModuleBuffer, request_size, callback, user);
#else
  (void)now_ms;
  (void)request;
  (void)out_bytes;
  (void)callback;
  (void)user;
  return feature_disabled("Module-buffer commands are disabled at build time");
#endif
}

Status MelsecSerialClient::async_write_module_buffer(
    std::uint32_t now_ms,
    const ModuleBufferWriteRequest& request,
    CompletionHandler callback,
    void* user) noexcept {
#if MCPROTOCOL_SERIAL_ENABLE_MODULE_BUFFER_COMMANDS
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_write_module_buffer(config_, request, request_data_, request_size);
  if (!status.ok()) {
    return status;
  }
  return start_request(now_ms, OperationKind::WriteModuleBuffer, request_size, callback, user);
#else
  (void)now_ms;
  (void)request;
  (void)callback;
  (void)user;
  return feature_disabled("Module-buffer commands are disabled at build time");
#endif
}

Status MelsecSerialClient::async_read_cpu_model(
    std::uint32_t now_ms,
    CpuModelInfo& out_info,
    CompletionHandler callback,
    void* user) noexcept {
#if MCPROTOCOL_SERIAL_ENABLE_CPU_MODEL_COMMANDS
  out_cpu_model_ = &out_info;

  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_read_cpu_model(config_, request_data_, request_size);
  if (!status.ok()) {
    clear_pending_outputs();
    return status;
  }
  return start_request(now_ms, OperationKind::ReadCpuModel, request_size, callback, user);
#else
  (void)now_ms;
  (void)out_info;
  (void)callback;
  (void)user;
  return feature_disabled("CPU-model commands are disabled at build time");
#endif
}

Status MelsecSerialClient::async_loopback(
    std::uint32_t now_ms,
    std::span<const char> hex_ascii,
    std::span<char> out_echoed,
    CompletionHandler callback,
    void* user) noexcept {
#if MCPROTOCOL_SERIAL_ENABLE_LOOPBACK_COMMANDS
  if (hex_ascii.size() > pending_loopback_.size()) {
    return make_status(StatusCode::InvalidArgument, "Loopback request exceeds the client limit");
  }
  out_chars_ = out_echoed;
  pending_loopback_size_ = hex_ascii.size();
  std::copy(hex_ascii.begin(), hex_ascii.end(), pending_loopback_.begin());

  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_loopback(config_, hex_ascii, request_data_, request_size);
  if (!status.ok()) {
    clear_pending_outputs();
    clear_pending_copies();
    return status;
  }
  return start_request(now_ms, OperationKind::Loopback, request_size, callback, user);
#else
  (void)now_ms;
  (void)hex_ascii;
  (void)out_echoed;
  (void)callback;
  (void)user;
  return feature_disabled("Loopback commands are disabled at build time");
#endif
}

}  // namespace mcprotocol::serial
