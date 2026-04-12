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

[[nodiscard]] constexpr Status remote_reset_no_response_status() noexcept {
  return make_status(StatusCode::Ok, "Remote RESET completed without a response");
}

[[nodiscard]] constexpr Status init_sequence_no_response_status() noexcept {
  return make_status(StatusCode::Ok, "Transmission-sequence initialization completed without a response");
}

[[nodiscard]] constexpr Status global_signal_no_response_status() noexcept {
  return make_status(StatusCode::Ok, "Global signal control completed without a response");
}

[[nodiscard]] constexpr Status cancelled_status() noexcept {
  return make_status(StatusCode::Cancelled, "The active request was cancelled");
}

[[nodiscard]] constexpr Status feature_disabled(const char* message) noexcept {
  return make_status(StatusCode::UnsupportedConfiguration, message);
}

[[nodiscard]] constexpr bool is_ascii_mode(const ProtocolConfig& config) noexcept {
#if MCPROTOCOL_SERIAL_ENABLE_ASCII_MODE
  return config.code_mode == CodeMode::Ascii;
#else
  (void)config;
  return false;
#endif
}

[[nodiscard]] constexpr bool is_binary_mode(const ProtocolConfig& config) noexcept {
#if MCPROTOCOL_SERIAL_ENABLE_BINARY_MODE
  return config.code_mode == CodeMode::Binary;
#else
  (void)config;
  return false;
#endif
}

[[nodiscard]] constexpr bool is_c1_frame(const ProtocolConfig& config) noexcept {
#if MCPROTOCOL_SERIAL_ENABLE_FRAME_C1
  return config.frame_kind == FrameKind::C1;
#else
  (void)config;
  return false;
#endif
}

[[nodiscard]] constexpr bool is_e1_frame(const ProtocolConfig& config) noexcept {
#if MCPROTOCOL_SERIAL_ENABLE_FRAME_E1
  return config.frame_kind == FrameKind::E1;
#else
  (void)config;
  return false;
#endif
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
  if (is_e1_frame(config)) {
    return false;
  }
  if (is_ascii_mode(config)) {
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

[[nodiscard]] bool is_e1_bit_device(DeviceCode code) noexcept {
  switch (code) {
    case DeviceCode::X:
    case DeviceCode::Y:
    case DeviceCode::M:
    case DeviceCode::L:
    case DeviceCode::SM:
    case DeviceCode::F:
    case DeviceCode::V:
    case DeviceCode::B:
    case DeviceCode::TS:
    case DeviceCode::TC:
    case DeviceCode::STS:
    case DeviceCode::STC:
    case DeviceCode::CS:
    case DeviceCode::CC:
    case DeviceCode::SB:
    case DeviceCode::S:
    case DeviceCode::DX:
    case DeviceCode::DY:
    case DeviceCode::LTS:
    case DeviceCode::LTC:
    case DeviceCode::LSTS:
    case DeviceCode::LSTC:
    case DeviceCode::LCS:
    case DeviceCode::LCC:
      return true;
    default:
      return false;
  }
}

[[nodiscard]] std::array<std::uint8_t, 2> ascii_hex_byte(std::uint8_t value) noexcept {
  const auto nibble = [](std::uint8_t part) noexcept -> std::uint8_t {
    return static_cast<std::uint8_t>(part < 10U ? ('0' + part) : ('A' + part - 10U));
  };
  return {
      nibble(static_cast<std::uint8_t>((value >> 4U) & 0x0FU)),
      nibble(static_cast<std::uint8_t>(value & 0x0FU)),
  };
}

[[nodiscard]] StreamDecodeResult decode_stream_buffer(
    const ProtocolConfig& config,
    std::uint8_t e1_response_subheader,
    std::size_t e1_success_response_data_size,
    std::span<const std::uint8_t> bytes) noexcept {
  StreamDecodeResult result {};
  if (bytes.empty()) {
    return result;
  }

  if (is_e1_frame(config)) {
    const std::size_t subheader_size = is_ascii_mode(config) ? 2U : 1U;
    const std::size_t minimum_frame_size = subheader_size + (is_ascii_mode(config) ? 2U : 1U);
    if (is_ascii_mode(config)) {
      const auto expected = ascii_hex_byte(e1_response_subheader);
      for (std::size_t offset = 0; offset < bytes.size(); ++offset) {
        const std::size_t remaining = bytes.size() - offset;
        if (remaining == 1U && bytes[offset] == expected[0]) {
          result.discard_prefix = offset;
          return result;
        }
        if (remaining < 2U) {
          break;
        }
        if (bytes[offset] == expected[0] && bytes[offset + 1U] == expected[1]) {
          if (offset != 0U) {
            result.discard_prefix = offset;
            return result;
          }
          if (bytes.size() < minimum_frame_size) {
            return result;
          }
          std::uint32_t end_code = 0;
          const auto nibble = [](std::uint8_t value) noexcept -> int {
            if (value >= '0' && value <= '9') {
              return value - '0';
            }
            if (value >= 'A' && value <= 'F') {
              return 10 + (value - 'A');
            }
            if (value >= 'a' && value <= 'f') {
              return 10 + (value - 'a');
            }
            return -1;
          };
          const int upper = nibble(bytes[2]);
          const int lower = nibble(bytes[3]);
          if (upper < 0 || lower < 0) {
            result.status = DecodeStatus::Error;
            result.decode = DecodeResult {
                .status = DecodeStatus::Error,
                .frame = RawResponseFrame {},
                .error = make_status(StatusCode::Parse, "Failed to parse 1E ASCII end code"),
                .bytes_consumed = 4U,
            };
            return result;
          }
          end_code = static_cast<std::uint32_t>((upper << 4U) | lower);
          std::size_t total_size = 4U;
          if (end_code == 0U) {
            total_size += e1_success_response_data_size;
          } else if (end_code == 0x5BU) {
            total_size += 2U;
          }
          if (bytes.size() < total_size) {
            return result;
          }
          DecodeResult candidate = FrameCodec::decode_response(config, bytes.first(total_size));
          candidate.bytes_consumed = total_size;
          result.status = candidate.status;
          result.decode = candidate;
          return result;
        }
      }
      result.discard_prefix = bytes.size();
      return result;
    }

    for (std::size_t offset = 0; offset < bytes.size(); ++offset) {
      if (bytes[offset] != e1_response_subheader) {
        continue;
      }
      if (offset != 0U) {
        result.discard_prefix = offset;
        return result;
      }
      if (bytes.size() < minimum_frame_size) {
        return result;
      }
      const std::uint8_t end_code = bytes[1];
      std::size_t total_size = 2U;
      if (end_code == 0x00U) {
        total_size += e1_success_response_data_size;
      } else if (end_code == 0x5BU) {
        total_size += 1U;
      }
      if (bytes.size() < total_size) {
        return result;
      }
      DecodeResult candidate = FrameCodec::decode_response(config, bytes.first(total_size));
      candidate.bytes_consumed = total_size;
      result.status = candidate.status;
      result.decode = candidate;
      return result;
    }

    result.discard_prefix = bytes.size();
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
        if (is_binary_mode(config)) {
          break;
        }
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
        expected_e1_response_subheader(),
        expected_e1_success_response_data_size(),
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
      if (operation_ == OperationKind::RemoteReset) {
        complete(remote_reset_no_response_status());
      } else if (operation_ == OperationKind::InitializeTransmissionSequence) {
        complete(init_sequence_no_response_status());
      } else if (operation_ == OperationKind::ControlGlobalSignal) {
        complete(global_signal_no_response_status());
      } else {
        complete(timeout_status());
      }
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

std::uint8_t MelsecSerialClient::expected_e1_response_subheader() const noexcept {
  switch (operation_) {
    case OperationKind::BatchReadBits:
      return 0x80U;
    case OperationKind::BatchReadWords:
      return 0x81U;
    case OperationKind::BatchWriteBits:
      return 0x82U;
    case OperationKind::BatchWriteWords:
      return 0x83U;
    case OperationKind::ReadExtendedFileRegisterWords:
      return 0x97U;
    case OperationKind::WriteExtendedFileRegisterWords:
      return 0x98U;
#if MCPROTOCOL_SERIAL_ENABLE_RANDOM_COMMANDS
    case OperationKind::RandomWriteExtendedFileRegisterWords:
      return 0x99U;
    case OperationKind::RandomWriteBits:
      return 0x84U;
    case OperationKind::RandomWriteWords:
      return 0x85U;
#endif
#if MCPROTOCOL_SERIAL_ENABLE_MONITOR_COMMANDS
    case OperationKind::RegisterMonitor:
      if (pending_random_item_count_ == 0U) {
        return 0x87U;
      }
      for (std::size_t index = 0; index < pending_random_item_count_; ++index) {
        if (pending_random_items_[index].double_word || !is_e1_bit_device(pending_random_items_[index].device.code)) {
          return 0x87U;
        }
      }
      return 0x86U;
    case OperationKind::ReadMonitor: {
      bool bit_units = monitor_item_count_ != 0U;
      for (std::size_t index = 0; index < monitor_item_count_; ++index) {
        if (monitor_items_[index].double_word || !is_e1_bit_device(monitor_items_[index].device.code)) {
          bit_units = false;
          break;
        }
      }
      return bit_units ? 0x88U : 0x89U;
    }
    case OperationKind::RegisterExtendedFileRegisterMonitor:
      return 0x9AU;
    case OperationKind::ReadExtendedFileRegisterMonitor:
      return 0x9BU;
#endif
    case OperationKind::DirectReadExtendedFileRegisterWords:
      return 0xBBU;
    case OperationKind::DirectWriteExtendedFileRegisterWords:
      return 0xBCU;
#if MCPROTOCOL_SERIAL_ENABLE_MODULE_BUFFER_COMMANDS
    case OperationKind::ReadModuleBuffer:
      return 0x8EU;
    case OperationKind::WriteModuleBuffer:
      return 0x8FU;
#endif
    default:
      return 0x00U;
  }
}

std::size_t MelsecSerialClient::expected_e1_success_response_data_size() const noexcept {
  const std::size_t ascii_word_size = config_.code_mode == CodeMode::Ascii ? 4U : 2U;
  switch (operation_) {
    case OperationKind::BatchReadWords:
      return static_cast<std::size_t>(batch_read_words_request_.points) * ascii_word_size;
    case OperationKind::ReadExtendedFileRegisterWords:
      return static_cast<std::size_t>(extended_file_register_read_request_.points) * ascii_word_size;
    case OperationKind::DirectReadExtendedFileRegisterWords:
      return static_cast<std::size_t>(direct_extended_file_register_read_request_.points) * ascii_word_size;
    case OperationKind::BatchReadBits:
      if (config_.code_mode == CodeMode::Ascii) {
        return static_cast<std::size_t>(batch_read_bits_request_.points) +
               ((batch_read_bits_request_.points % 2U) == 0U ? 0U : 1U);
      }
      return static_cast<std::size_t>((batch_read_bits_request_.points + 1U) / 2U);
    case OperationKind::BatchWriteWords:
    case OperationKind::WriteExtendedFileRegisterWords:
    case OperationKind::DirectWriteExtendedFileRegisterWords:
    case OperationKind::BatchWriteBits:
    case OperationKind::ExtendedBatchWriteWords:
#if MCPROTOCOL_SERIAL_ENABLE_RANDOM_COMMANDS
    case OperationKind::RandomWriteWords:
    case OperationKind::RandomWriteExtendedFileRegisterWords:
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
      return 0U;
#if MCPROTOCOL_SERIAL_ENABLE_MONITOR_COMMANDS
    case OperationKind::RegisterMonitor:
    case OperationKind::RegisterExtendedFileRegisterMonitor:
      return 0U;
    case OperationKind::ReadMonitor: {
      bool bit_units = monitor_item_count_ != 0U;
      for (std::size_t index = 0; index < monitor_item_count_; ++index) {
        if (monitor_items_[index].double_word || !is_e1_bit_device(monitor_items_[index].device.code)) {
          bit_units = false;
          break;
        }
      }
      if (bit_units) {
        if (config_.code_mode == CodeMode::Ascii) {
          return monitor_item_count_ + ((monitor_item_count_ % 2U) == 0U ? 0U : 1U);
        }
        return (monitor_item_count_ + 1U) / 2U;
      }
      return monitor_item_count_ * ascii_word_size;
    }
    case OperationKind::ReadExtendedFileRegisterMonitor:
      return extended_file_register_monitor_item_count_ * ascii_word_size;
#endif
#if MCPROTOCOL_SERIAL_ENABLE_MODULE_BUFFER_COMMANDS
    case OperationKind::ReadModuleBuffer:
      return module_buffer_read_request_.bytes;
#endif
    default:
      return 0U;
  }
}

Status MelsecSerialClient::handle_response(std::span<const std::uint8_t> response_data) noexcept {
  switch (operation_) {
    case OperationKind::BatchReadWords:
      return CommandCodec::parse_batch_read_words_response(
          config_,
          batch_read_words_request_,
          response_data,
          out_words_);
    case OperationKind::ReadUserFrame:
      return CommandCodec::parse_read_user_frame_response(
          config_,
          response_data,
          *out_user_frame_data_);
    case OperationKind::ReadExtendedFileRegisterWords:
      return CommandCodec::parse_read_extended_file_register_words_response(
          config_,
          extended_file_register_read_request_.points,
          response_data,
          out_words_);
    case OperationKind::DirectReadExtendedFileRegisterWords:
      return CommandCodec::parse_read_extended_file_register_words_response(
          config_,
          direct_extended_file_register_read_request_.points,
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
    case OperationKind::WriteExtendedFileRegisterWords:
    case OperationKind::DirectWriteExtendedFileRegisterWords:
    case OperationKind::BatchWriteBits:
    case OperationKind::ExtendedBatchWriteWords:
#if MCPROTOCOL_SERIAL_ENABLE_RANDOM_COMMANDS
    case OperationKind::RandomWriteWords:
    case OperationKind::RandomWriteExtendedFileRegisterWords:
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
    case OperationKind::RemoteRun:
    case OperationKind::RemoteStop:
    case OperationKind::RemotePause:
    case OperationKind::RemoteLatchClear:
    case OperationKind::UnlockRemotePassword:
    case OperationKind::LockRemotePassword:
    case OperationKind::ClearErrorInformation:
    case OperationKind::RemoteReset:
    case OperationKind::WriteUserFrame:
    case OperationKind::DeleteUserFrame:
    case OperationKind::ControlGlobalSignal:
    case OperationKind::InitializeTransmissionSequence:
    case OperationKind::DeregisterCpuMonitoring:
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
    case OperationKind::RegisterExtendedFileRegisterMonitor:
      if (!response_data.empty()) {
        return make_status(
            StatusCode::Parse,
            "Extended file-register monitor registration response must not contain response data");
      }
      std::copy_n(
          pending_extended_file_register_items_.begin(),
          pending_extended_file_register_item_count_,
          extended_file_register_monitor_items_.begin());
      extended_file_register_monitor_item_count_ = pending_extended_file_register_item_count_;
      extended_file_register_monitor_registered_ = true;
      return ok_status();
    case OperationKind::ReadMonitor:
      return CommandCodec::parse_read_monitor_response(
          config_,
          std::span<const RandomReadItem>(monitor_items_.data(), monitor_item_count_),
          response_data,
          out_values_);
    case OperationKind::ReadExtendedFileRegisterMonitor:
      return CommandCodec::parse_read_extended_file_register_monitor_response(
          config_,
          std::span<const ExtendedFileRegisterAddress>(
              extended_file_register_monitor_items_.data(),
              extended_file_register_monitor_item_count_),
          response_data,
          out_words_);
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
  out_user_frame_data_ = nullptr;
  batch_read_words_request_ = {};
  extended_file_register_read_request_ = {};
  direct_extended_file_register_read_request_ = {};
  batch_read_bits_request_ = {};
  user_frame_read_request_ = {};
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
#if MCPROTOCOL_SERIAL_ENABLE_MONITOR_COMMANDS
  pending_extended_file_register_item_count_ = 0;
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

Status MelsecSerialClient::async_read_extended_file_register_words(
    std::uint32_t now_ms,
    const ExtendedFileRegisterBatchReadWordsRequest& request,
    std::span<std::uint16_t> out_words,
    CompletionHandler callback,
    void* user) noexcept {
  extended_file_register_read_request_ = request;
  out_words_ = out_words;
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_read_extended_file_register_words(
      config_,
      request,
      request_data_,
      request_size);
  if (!status.ok()) {
    clear_pending_outputs();
    return status;
  }
  return start_request(
      now_ms,
      OperationKind::ReadExtendedFileRegisterWords,
      request_size,
      callback,
      user);
}

Status MelsecSerialClient::async_direct_read_extended_file_register_words(
    std::uint32_t now_ms,
    const ExtendedFileRegisterDirectBatchReadWordsRequest& request,
    std::span<std::uint16_t> out_words,
    CompletionHandler callback,
    void* user) noexcept {
  direct_extended_file_register_read_request_ = request;
  out_words_ = out_words;
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_direct_read_extended_file_register_words(
      config_,
      request,
      request_data_,
      request_size);
  if (!status.ok()) {
    clear_pending_outputs();
    return status;
  }
  return start_request(
      now_ms,
      OperationKind::DirectReadExtendedFileRegisterWords,
      request_size,
      callback,
      user);
}

Status MelsecSerialClient::async_link_direct_batch_read_words(
    std::uint32_t now_ms,
    const LinkDirectDevice& device,
    std::uint16_t points,
    std::span<std::uint16_t> out_words,
    CompletionHandler callback,
    void* user) noexcept {
  batch_read_words_request_ = BatchReadWordsRequest {
      .head_device = device.device,
      .points = points,
  };
  out_words_ = out_words;
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_link_direct_batch_read_words(
      config_,
      device,
      points,
      request_data_,
      request_size);
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

Status MelsecSerialClient::async_link_direct_batch_read_bits(
    std::uint32_t now_ms,
    const LinkDirectDevice& device,
    std::uint16_t points,
    std::span<BitValue> out_bits,
    CompletionHandler callback,
    void* user) noexcept {
  batch_read_bits_request_ = BatchReadBitsRequest {
      .head_device = device.device,
      .points = points,
  };
  out_bits_ = out_bits;
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_link_direct_batch_read_bits(
      config_,
      device,
      points,
      request_data_,
      request_size);
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

Status MelsecSerialClient::async_write_extended_file_register_words(
    std::uint32_t now_ms,
    const ExtendedFileRegisterBatchWriteWordsRequest& request,
    CompletionHandler callback,
    void* user) noexcept {
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_write_extended_file_register_words(
      config_,
      request,
      request_data_,
      request_size);
  if (!status.ok()) {
    return status;
  }
  return start_request(
      now_ms,
      OperationKind::WriteExtendedFileRegisterWords,
      request_size,
      callback,
      user);
}

Status MelsecSerialClient::async_direct_write_extended_file_register_words(
    std::uint32_t now_ms,
    const ExtendedFileRegisterDirectBatchWriteWordsRequest& request,
    CompletionHandler callback,
    void* user) noexcept {
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_direct_write_extended_file_register_words(
      config_,
      request,
      request_data_,
      request_size);
  if (!status.ok()) {
    return status;
  }
  return start_request(
      now_ms,
      OperationKind::DirectWriteExtendedFileRegisterWords,
      request_size,
      callback,
      user);
}

Status MelsecSerialClient::async_link_direct_batch_write_words(
    std::uint32_t now_ms,
    const LinkDirectDevice& device,
    std::span<const std::uint16_t> words,
    CompletionHandler callback,
    void* user) noexcept {
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_link_direct_batch_write_words(
      config_,
      device,
      words,
      request_data_,
      request_size);
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

Status MelsecSerialClient::async_link_direct_batch_write_bits(
    std::uint32_t now_ms,
    const LinkDirectDevice& device,
    std::span<const BitValue> bits,
    CompletionHandler callback,
    void* user) noexcept {
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_link_direct_batch_write_bits(
      config_,
      device,
      bits,
      request_data_,
      request_size);
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

Status MelsecSerialClient::async_link_direct_random_read(
    std::uint32_t now_ms,
    std::span<const LinkDirectRandomReadItem> items,
    std::span<std::uint32_t> out_values,
    CompletionHandler callback,
    void* user) noexcept {
#if MCPROTOCOL_SERIAL_ENABLE_RANDOM_COMMANDS
  if (items.size() > pending_random_items_.size()) {
    return make_status(StatusCode::InvalidArgument, "Link direct random read item count exceeds the client limit");
  }
  pending_random_item_count_ = items.size();
  for (std::size_t index = 0; index < items.size(); ++index) {
    pending_random_items_[index] = RandomReadItem {
        .device = items[index].device.device,
        .double_word = items[index].double_word,
    };
  }
  out_values_ = out_values;

  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_link_direct_random_read(config_, items, request_data_, request_size);
  if (!status.ok()) {
    clear_pending_outputs();
    clear_pending_copies();
    return status;
  }
  return start_request(now_ms, OperationKind::RandomRead, request_size, callback, user);
#else
  (void)now_ms;
  (void)items;
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

Status MelsecSerialClient::async_random_write_extended_file_register_words(
    std::uint32_t now_ms,
    std::span<const ExtendedFileRegisterRandomWriteWordItem> items,
    CompletionHandler callback,
    void* user) noexcept {
#if MCPROTOCOL_SERIAL_ENABLE_RANDOM_COMMANDS
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_random_write_extended_file_register_words(
      config_,
      items,
      request_data_,
      request_size);
  if (!status.ok()) {
    return status;
  }
  return start_request(
      now_ms,
      OperationKind::RandomWriteExtendedFileRegisterWords,
      request_size,
      callback,
      user);
#else
  (void)now_ms;
  (void)items;
  (void)callback;
  (void)user;
  return feature_disabled("Random commands are disabled at build time");
#endif
}

Status MelsecSerialClient::async_link_direct_random_write_words(
    std::uint32_t now_ms,
    std::span<const LinkDirectRandomWriteWordItem> items,
    CompletionHandler callback,
    void* user) noexcept {
#if MCPROTOCOL_SERIAL_ENABLE_RANDOM_COMMANDS
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_link_direct_random_write_words(config_, items, request_data_, request_size);
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

Status MelsecSerialClient::async_link_direct_random_write_bits(
    std::uint32_t now_ms,
    std::span<const LinkDirectRandomWriteBitItem> items,
    CompletionHandler callback,
    void* user) noexcept {
#if MCPROTOCOL_SERIAL_ENABLE_RANDOM_COMMANDS
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_link_direct_random_write_bits(config_, items, request_data_, request_size);
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

Status MelsecSerialClient::async_link_direct_multi_block_read(
    std::uint32_t now_ms,
    const LinkDirectMultiBlockReadRequest& request,
    std::span<std::uint16_t> out_words,
    std::span<BitValue> out_bits,
    std::span<MultiBlockReadBlockResult> out_results,
    CompletionHandler callback,
    void* user) noexcept {
#if MCPROTOCOL_SERIAL_ENABLE_MULTI_BLOCK_COMMANDS
  if (request.blocks.size() > pending_multi_blocks_.size()) {
    return make_status(StatusCode::InvalidArgument, "Link direct multi-block read block count exceeds the client limit");
  }
  pending_multi_block_count_ = request.blocks.size();
  for (std::size_t index = 0; index < request.blocks.size(); ++index) {
    pending_multi_blocks_[index] = MultiBlockReadBlock {
        .head_device = request.blocks[index].head_device.device,
        .points = request.blocks[index].points,
        .bit_block = request.blocks[index].bit_block,
    };
  }
  out_words_ = out_words;
  out_bits_ = out_bits;
  out_block_results_ = out_results;

  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_link_direct_multi_block_read(
      config_,
      request,
      request_data_,
      request_size);
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

Status MelsecSerialClient::async_link_direct_multi_block_write(
    std::uint32_t now_ms,
    const LinkDirectMultiBlockWriteRequest& request,
    CompletionHandler callback,
    void* user) noexcept {
#if MCPROTOCOL_SERIAL_ENABLE_MULTI_BLOCK_COMMANDS
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_link_direct_multi_block_write(
      config_,
      request,
      request_data_,
      request_size);
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

Status MelsecSerialClient::async_register_extended_file_register_monitor(
    std::uint32_t now_ms,
    const ExtendedFileRegisterMonitorRegistration& request,
    CompletionHandler callback,
    void* user) noexcept {
#if MCPROTOCOL_SERIAL_ENABLE_MONITOR_COMMANDS
  if (request.items.size() > pending_extended_file_register_items_.size()) {
    return make_status(
        StatusCode::InvalidArgument,
        "Extended file-register monitor item count exceeds the client limit");
  }
  pending_extended_file_register_item_count_ = request.items.size();
  std::copy(
      request.items.begin(),
      request.items.end(),
      pending_extended_file_register_items_.begin());

  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_register_extended_file_register_monitor(
      config_,
      request,
      request_data_,
      request_size);
  if (!status.ok()) {
    clear_pending_copies();
    return status;
  }
  return start_request(
      now_ms,
      OperationKind::RegisterExtendedFileRegisterMonitor,
      request_size,
      callback,
      user);
#else
  (void)now_ms;
  (void)request;
  (void)callback;
  (void)user;
  return feature_disabled("Monitor commands are disabled at build time");
#endif
}

Status MelsecSerialClient::async_link_direct_register_monitor(
    std::uint32_t now_ms,
    const LinkDirectMonitorRegistration& request,
    CompletionHandler callback,
    void* user) noexcept {
#if MCPROTOCOL_SERIAL_ENABLE_MONITOR_COMMANDS
  if (request.items.size() > pending_random_items_.size()) {
    return make_status(StatusCode::InvalidArgument, "Link direct monitor item count exceeds the client limit");
  }
  pending_random_item_count_ = request.items.size();
  for (std::size_t index = 0; index < request.items.size(); ++index) {
    pending_random_items_[index] = RandomReadItem {
        .device = request.items[index].device.device,
        .double_word = request.items[index].double_word,
    };
  }

  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_link_direct_register_monitor(config_, request, request_data_, request_size);
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
  const Status status = CommandCodec::encode_read_monitor(
      config_,
      std::span<const RandomReadItem>(monitor_items_.data(), monitor_item_count_),
      request_data_,
      request_size);
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

Status MelsecSerialClient::async_read_extended_file_register_monitor(
    std::uint32_t now_ms,
    std::span<std::uint16_t> out_words,
    CompletionHandler callback,
    void* user) noexcept {
#if MCPROTOCOL_SERIAL_ENABLE_MONITOR_COMMANDS
  if (!extended_file_register_monitor_registered_) {
    return make_status(
        StatusCode::InvalidArgument,
        "Extended file-register monitor data has not been registered");
  }
  if (out_words.size() < extended_file_register_monitor_item_count_) {
    return make_status(
        StatusCode::BufferTooSmall,
        "Extended file-register monitor output buffer is too small");
  }
  out_words_ = out_words;

  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_read_extended_file_register_monitor(
      config_,
      std::span<const ExtendedFileRegisterAddress>(
          extended_file_register_monitor_items_.data(),
          extended_file_register_monitor_item_count_),
      request_data_,
      request_size);
  if (!status.ok()) {
    clear_pending_outputs();
    return status;
  }
  return start_request(
      now_ms,
      OperationKind::ReadExtendedFileRegisterMonitor,
      request_size,
      callback,
      user);
#else
  (void)now_ms;
  (void)out_words;
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

Status MelsecSerialClient::async_remote_run(
    std::uint32_t now_ms,
    RemoteOperationMode mode,
    RemoteRunClearMode clear_mode,
    CompletionHandler callback,
    void* user) noexcept {
  std::size_t request_size = 0;
  const Status status =
      CommandCodec::encode_remote_run(config_, mode, clear_mode, request_data_, request_size);
  if (!status.ok()) {
    return status;
  }
  return start_request(now_ms, OperationKind::RemoteRun, request_size, callback, user);
}

Status MelsecSerialClient::async_remote_stop(
    std::uint32_t now_ms,
    CompletionHandler callback,
    void* user) noexcept {
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_remote_stop(config_, request_data_, request_size);
  if (!status.ok()) {
    return status;
  }
  return start_request(now_ms, OperationKind::RemoteStop, request_size, callback, user);
}

Status MelsecSerialClient::async_remote_pause(
    std::uint32_t now_ms,
    RemoteOperationMode mode,
    CompletionHandler callback,
    void* user) noexcept {
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_remote_pause(config_, mode, request_data_, request_size);
  if (!status.ok()) {
    return status;
  }
  return start_request(now_ms, OperationKind::RemotePause, request_size, callback, user);
}

Status MelsecSerialClient::async_remote_latch_clear(
    std::uint32_t now_ms,
    CompletionHandler callback,
    void* user) noexcept {
  std::size_t request_size = 0;
  const Status status =
      CommandCodec::encode_remote_latch_clear(config_, request_data_, request_size);
  if (!status.ok()) {
    return status;
  }
  return start_request(now_ms, OperationKind::RemoteLatchClear, request_size, callback, user);
}

Status MelsecSerialClient::async_unlock_remote_password(
    std::uint32_t now_ms,
    std::string_view remote_password,
    CompletionHandler callback,
    void* user) noexcept {
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_unlock_remote_password(
      config_,
      remote_password,
      request_data_,
      request_size);
  if (!status.ok()) {
    return status;
  }
  return start_request(now_ms, OperationKind::UnlockRemotePassword, request_size, callback, user);
}

Status MelsecSerialClient::async_lock_remote_password(
    std::uint32_t now_ms,
    std::string_view remote_password,
    CompletionHandler callback,
    void* user) noexcept {
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_lock_remote_password(
      config_,
      remote_password,
      request_data_,
      request_size);
  if (!status.ok()) {
    return status;
  }
  return start_request(now_ms, OperationKind::LockRemotePassword, request_size, callback, user);
}

Status MelsecSerialClient::async_remote_reset(
    std::uint32_t now_ms,
    CompletionHandler callback,
    void* user) noexcept {
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_remote_reset(config_, request_data_, request_size);
  if (!status.ok()) {
    return status;
  }
  return start_request(now_ms, OperationKind::RemoteReset, request_size, callback, user);
}

Status MelsecSerialClient::async_clear_error_information(
    std::uint32_t now_ms,
    CompletionHandler callback,
    void* user) noexcept {
  std::size_t request_size = 0;
  const Status status =
      CommandCodec::encode_clear_error_information(config_, request_data_, request_size);
  if (!status.ok()) {
    return status;
  }
  return start_request(now_ms, OperationKind::ClearErrorInformation, request_size, callback, user);
}

Status MelsecSerialClient::async_read_user_frame(
    std::uint32_t now_ms,
    const UserFrameReadRequest& request,
    UserFrameRegistrationData& out_data,
    CompletionHandler callback,
    void* user) noexcept {
  user_frame_read_request_ = request;
  out_user_frame_data_ = &out_data;
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_read_user_frame(
      config_,
      request,
      request_data_,
      request_size);
  if (!status.ok()) {
    clear_pending_outputs();
    return status;
  }
  return start_request(now_ms, OperationKind::ReadUserFrame, request_size, callback, user);
}

Status MelsecSerialClient::async_write_user_frame(
    std::uint32_t now_ms,
    const UserFrameWriteRequest& request,
    CompletionHandler callback,
    void* user) noexcept {
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_write_user_frame(
      config_,
      request,
      request_data_,
      request_size);
  if (!status.ok()) {
    return status;
  }
  return start_request(now_ms, OperationKind::WriteUserFrame, request_size, callback, user);
}

Status MelsecSerialClient::async_delete_user_frame(
    std::uint32_t now_ms,
    const UserFrameDeleteRequest& request,
    CompletionHandler callback,
    void* user) noexcept {
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_delete_user_frame(
      config_,
      request,
      request_data_,
      request_size);
  if (!status.ok()) {
    return status;
  }
  return start_request(now_ms, OperationKind::DeleteUserFrame, request_size, callback, user);
}

Status MelsecSerialClient::async_control_global_signal(
    std::uint32_t now_ms,
    const GlobalSignalControlRequest& request,
    CompletionHandler callback,
    void* user) noexcept {
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_control_global_signal(
      config_,
      request,
      request_data_,
      request_size);
  if (!status.ok()) {
    return status;
  }
  return start_request(now_ms, OperationKind::ControlGlobalSignal, request_size, callback, user);
}

Status MelsecSerialClient::async_initialize_c24_transmission_sequence(
    std::uint32_t now_ms,
    CompletionHandler callback,
    void* user) noexcept {
  std::size_t request_size = 0;
  const Status status =
      CommandCodec::encode_initialize_transmission_sequence(config_, request_data_, request_size);
  if (!status.ok()) {
    return status;
  }
  return start_request(
      now_ms,
      OperationKind::InitializeTransmissionSequence,
      request_size,
      callback,
      user);
}

Status MelsecSerialClient::async_deregister_cpu_monitoring(
    std::uint32_t now_ms,
    CompletionHandler callback,
    void* user) noexcept {
  std::size_t request_size = 0;
  const Status status =
      CommandCodec::encode_deregister_cpu_monitoring(config_, request_data_, request_size);
  if (!status.ok()) {
    return status;
  }
  return start_request(now_ms, OperationKind::DeregisterCpuMonitoring, request_size, callback, user);
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

