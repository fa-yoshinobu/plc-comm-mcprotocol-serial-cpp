#include "mcprotocol/serial/host_sync.hpp"

#include <chrono>

namespace mcprotocol::serial {
namespace {

[[nodiscard]] std::uint32_t now_ms() noexcept {
  const auto now = std::chrono::steady_clock::now().time_since_epoch();
  return static_cast<std::uint32_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(now).count());
}

[[nodiscard]] Status span_size_to_points(
    std::size_t size,
    std::uint16_t& out_points,
    const char* what) noexcept {
  if (size > 0xFFFFU) {
    return make_status(StatusCode::InvalidArgument, what);
  }

  out_points = static_cast<std::uint16_t>(size);
  return ok_status();
}

}  // namespace

void PosixSyncClient::on_request_complete(void* user, Status status) noexcept {
  auto* state = static_cast<CompletionState*>(user);
  state->done = true;
  state->status = status;
}

Status PosixSyncClient::open(
    const PosixSerialConfig& serial_config,
    const ProtocolConfig& protocol_config) noexcept {
  close();

  Status status = client_.configure(protocol_config);
  if (!status.ok()) {
    return status;
  }

  status = port_.open(serial_config);
  if (!status.ok()) {
    return status;
  }

  protocol_config_ = protocol_config;
  return ok_status();
}

void PosixSyncClient::close() noexcept {
  client_.cancel();
  port_.close();
}

bool PosixSyncClient::is_open() const noexcept {
  return port_.is_open();
}

const ProtocolConfig& PosixSyncClient::protocol_config() const noexcept {
  return protocol_config_;
}

Status PosixSyncClient::run_until_complete() noexcept {
  completion_ = {};

  Status status = port_.flush_rx();
  if (!status.ok()) {
    client_.cancel();
    return status;
  }

  status = port_.write_all(client_.pending_tx_frame());
  if (!status.ok()) {
    client_.cancel();
    return status;
  }

  status = port_.drain_tx();
  if (!status.ok()) {
    client_.cancel();
    return status;
  }

  status = client_.notify_tx_complete(now_ms());
  if (!status.ok()) {
    client_.cancel();
    return status;
  }
  if (completion_.done) {
    return completion_.status;
  }

  while (!completion_.done) {
    std::size_t received = 0;
    status = port_.read_some(rx_buffer_, 20, received);
    if (!status.ok()) {
      client_.cancel();
      return status;
    }

    if (received > 0U) {
      client_.on_rx_bytes(
          now_ms(),
          std::span<const std::byte>(rx_buffer_.data(), received));
      if (completion_.done) {
        break;
      }
    }

    client_.poll(now_ms());
  }

  return completion_.status;
}

Status PosixSyncClient::read_cpu_model(CpuModelInfo& out_info) noexcept {
  const Status status = client_.async_read_cpu_model(
      now_ms(),
      out_info,
      &PosixSyncClient::on_request_complete,
      &completion_);
  if (!status.ok()) {
    return status;
  }
  return run_until_complete();
}

Status PosixSyncClient::remote_run(
    RemoteOperationMode mode,
    RemoteRunClearMode clear_mode) noexcept {
  const Status status = client_.async_remote_run(
      now_ms(),
      mode,
      clear_mode,
      &PosixSyncClient::on_request_complete,
      &completion_);
  if (!status.ok()) {
    return status;
  }
  return run_until_complete();
}

Status PosixSyncClient::remote_stop() noexcept {
  const Status status = client_.async_remote_stop(
      now_ms(),
      &PosixSyncClient::on_request_complete,
      &completion_);
  if (!status.ok()) {
    return status;
  }
  return run_until_complete();
}

Status PosixSyncClient::remote_pause(RemoteOperationMode mode) noexcept {
  const Status status = client_.async_remote_pause(
      now_ms(),
      mode,
      &PosixSyncClient::on_request_complete,
      &completion_);
  if (!status.ok()) {
    return status;
  }
  return run_until_complete();
}

Status PosixSyncClient::remote_latch_clear() noexcept {
  const Status status = client_.async_remote_latch_clear(
      now_ms(),
      &PosixSyncClient::on_request_complete,
      &completion_);
  if (!status.ok()) {
    return status;
  }
  return run_until_complete();
}

Status PosixSyncClient::unlock_remote_password(std::string_view remote_password) noexcept {
  const Status status = client_.async_unlock_remote_password(
      now_ms(),
      remote_password,
      &PosixSyncClient::on_request_complete,
      &completion_);
  if (!status.ok()) {
    return status;
  }
  return run_until_complete();
}

Status PosixSyncClient::lock_remote_password(std::string_view remote_password) noexcept {
  const Status status = client_.async_lock_remote_password(
      now_ms(),
      remote_password,
      &PosixSyncClient::on_request_complete,
      &completion_);
  if (!status.ok()) {
    return status;
  }
  return run_until_complete();
}

Status PosixSyncClient::clear_error_information() noexcept {
  const Status status = client_.async_clear_error_information(
      now_ms(),
      &PosixSyncClient::on_request_complete,
      &completion_);
  if (!status.ok()) {
    return status;
  }
  return run_until_complete();
}

Status PosixSyncClient::remote_reset() noexcept {
  const Status status = client_.async_remote_reset(
      now_ms(),
      &PosixSyncClient::on_request_complete,
      &completion_);
  if (!status.ok()) {
    return status;
  }
  return run_until_complete();
}

Status PosixSyncClient::read_user_frame(
    const UserFrameReadRequest& request,
    UserFrameRegistrationData& out_data) noexcept {
  const Status status = client_.async_read_user_frame(
      now_ms(),
      request,
      out_data,
      &PosixSyncClient::on_request_complete,
      &completion_);
  if (!status.ok()) {
    return status;
  }
  return run_until_complete();
}

Status PosixSyncClient::write_user_frame(const UserFrameWriteRequest& request) noexcept {
  const Status status = client_.async_write_user_frame(
      now_ms(),
      request,
      &PosixSyncClient::on_request_complete,
      &completion_);
  if (!status.ok()) {
    return status;
  }
  return run_until_complete();
}

Status PosixSyncClient::delete_user_frame(const UserFrameDeleteRequest& request) noexcept {
  const Status status = client_.async_delete_user_frame(
      now_ms(),
      request,
      &PosixSyncClient::on_request_complete,
      &completion_);
  if (!status.ok()) {
    return status;
  }
  return run_until_complete();
}

Status PosixSyncClient::control_global_signal(
    const GlobalSignalControlRequest& request) noexcept {
  const Status status = client_.async_control_global_signal(
      now_ms(),
      request,
      &PosixSyncClient::on_request_complete,
      &completion_);
  if (!status.ok()) {
    return status;
  }
  return run_until_complete();
}

Status PosixSyncClient::initialize_c24_transmission_sequence() noexcept {
  const Status status = client_.async_initialize_c24_transmission_sequence(
      now_ms(),
      &PosixSyncClient::on_request_complete,
      &completion_);
  if (!status.ok()) {
    return status;
  }
  return run_until_complete();
}

Status PosixSyncClient::deregister_cpu_monitoring() noexcept {
  const Status status = client_.async_deregister_cpu_monitoring(
      now_ms(),
      &PosixSyncClient::on_request_complete,
      &completion_);
  if (!status.ok()) {
    return status;
  }
  return run_until_complete();
}

Status PosixSyncClient::read_words(
    std::string_view head_device,
    std::uint16_t points,
    std::span<std::uint16_t> out_words) noexcept {
  BatchReadWordsRequest request {};
  Status status = highlevel::make_batch_read_words_request(head_device, points, request);
  if (!status.ok()) {
    return status;
  }

  status = client_.async_batch_read_words(
      now_ms(),
      request,
      out_words,
      &PosixSyncClient::on_request_complete,
      &completion_);
  if (!status.ok()) {
    return status;
  }
  return run_until_complete();
}

Status PosixSyncClient::read_words(
    std::string_view head_device,
    std::span<std::uint16_t> out_words) noexcept {
  std::uint16_t points = 0;
  const Status status = span_size_to_points(
      out_words.size(),
      points,
      "Word-read output span exceeds the protocol point limit");
  if (!status.ok()) {
    return status;
  }
  return read_words(head_device, points, out_words);
}

Status PosixSyncClient::read_extended_file_register_words(
    const ExtendedFileRegisterBatchReadWordsRequest& request,
    std::span<std::uint16_t> out_words) noexcept {
  const Status status = client_.async_read_extended_file_register_words(
      now_ms(),
      request,
      out_words,
      &PosixSyncClient::on_request_complete,
      &completion_);
  if (!status.ok()) {
    return status;
  }
  return run_until_complete();
}

Status PosixSyncClient::direct_read_extended_file_register_words(
    const ExtendedFileRegisterDirectBatchReadWordsRequest& request,
    std::span<std::uint16_t> out_words) noexcept {
  const Status status = client_.async_direct_read_extended_file_register_words(
      now_ms(),
      request,
      out_words,
      &PosixSyncClient::on_request_complete,
      &completion_);
  if (!status.ok()) {
    return status;
  }
  return run_until_complete();
}

Status PosixSyncClient::read_bits(
    std::string_view head_device,
    std::uint16_t points,
    std::span<BitValue> out_bits) noexcept {
  BatchReadBitsRequest request {};
  Status status = highlevel::make_batch_read_bits_request(head_device, points, request);
  if (!status.ok()) {
    return status;
  }

  status = client_.async_batch_read_bits(
      now_ms(),
      request,
      out_bits,
      &PosixSyncClient::on_request_complete,
      &completion_);
  if (!status.ok()) {
    return status;
  }
  return run_until_complete();
}

Status PosixSyncClient::read_bits(
    std::string_view head_device,
    std::span<BitValue> out_bits) noexcept {
  std::uint16_t points = 0;
  const Status status = span_size_to_points(
      out_bits.size(),
      points,
      "Bit-read output span exceeds the protocol point limit");
  if (!status.ok()) {
    return status;
  }
  return read_bits(head_device, points, out_bits);
}

Status PosixSyncClient::write_words(
    std::string_view head_device,
    std::span<const std::uint16_t> words) noexcept {
  BatchWriteWordsRequest request {};
  Status status = highlevel::make_batch_write_words_request(head_device, words, request);
  if (!status.ok()) {
    return status;
  }

  status = client_.async_batch_write_words(
      now_ms(),
      request,
      &PosixSyncClient::on_request_complete,
      &completion_);
  if (!status.ok()) {
    return status;
  }
  return run_until_complete();
}

Status PosixSyncClient::write_extended_file_register_words(
    const ExtendedFileRegisterBatchWriteWordsRequest& request) noexcept {
  const Status status = client_.async_write_extended_file_register_words(
      now_ms(),
      request,
      &PosixSyncClient::on_request_complete,
      &completion_);
  if (!status.ok()) {
    return status;
  }
  return run_until_complete();
}

Status PosixSyncClient::direct_write_extended_file_register_words(
    const ExtendedFileRegisterDirectBatchWriteWordsRequest& request) noexcept {
  const Status status = client_.async_direct_write_extended_file_register_words(
      now_ms(),
      request,
      &PosixSyncClient::on_request_complete,
      &completion_);
  if (!status.ok()) {
    return status;
  }
  return run_until_complete();
}

Status PosixSyncClient::random_read(
    std::span<const highlevel::RandomReadSpec> items,
    std::span<std::uint32_t> out_values) noexcept {
  std::array<RandomReadItem, kMaxRandomAccessItems> parsed_items {};
  RandomReadRequest request {};
  Status status = highlevel::make_random_read_request(items, parsed_items, request);
  if (!status.ok()) {
    return status;
  }

  status = client_.async_random_read(
      now_ms(),
      request,
      out_values,
      &PosixSyncClient::on_request_complete,
      &completion_);
  if (!status.ok()) {
    return status;
  }
  return run_until_complete();
}

Status PosixSyncClient::random_read(
    std::string_view device,
    std::uint32_t& out_value,
    bool double_word) noexcept {
  const std::array<highlevel::RandomReadSpec, 1> items {{
      {.device = device, .double_word = double_word},
  }};
  return random_read(items, std::span<std::uint32_t>(&out_value, 1U));
}

Status PosixSyncClient::random_write_words(
    std::span<const highlevel::RandomWriteWordSpec> items) noexcept {
  std::array<RandomWriteWordItem, kMaxRandomAccessItems> parsed_items {};
  std::span<const RandomWriteWordItem> request_items {};
  Status status = highlevel::make_random_write_word_items(items, parsed_items, request_items);
  if (!status.ok()) {
    return status;
  }

  status = client_.async_random_write_words(
      now_ms(),
      request_items,
      &PosixSyncClient::on_request_complete,
      &completion_);
  if (!status.ok()) {
    return status;
  }
  return run_until_complete();
}

Status PosixSyncClient::random_write_extended_file_register_words(
    std::span<const ExtendedFileRegisterRandomWriteWordItem> items) noexcept {
  const Status status = client_.async_random_write_extended_file_register_words(
      now_ms(),
      items,
      &PosixSyncClient::on_request_complete,
      &completion_);
  if (!status.ok()) {
    return status;
  }
  return run_until_complete();
}

Status PosixSyncClient::random_write_word(
    std::string_view device,
    std::uint32_t value,
    bool double_word) noexcept {
  const std::array<highlevel::RandomWriteWordSpec, 1> items {{
      {.device = device, .value = value, .double_word = double_word},
  }};
  return random_write_words(items);
}

Status PosixSyncClient::random_write_bits(
    std::span<const highlevel::RandomWriteBitSpec> items) noexcept {
  std::array<RandomWriteBitItem, kMaxRandomAccessItems> parsed_items {};
  std::span<const RandomWriteBitItem> request_items {};
  Status status = highlevel::make_random_write_bit_items(items, parsed_items, request_items);
  if (!status.ok()) {
    return status;
  }

  status = client_.async_random_write_bits(
      now_ms(),
      request_items,
      &PosixSyncClient::on_request_complete,
      &completion_);
  if (!status.ok()) {
    return status;
  }
  return run_until_complete();
}

Status PosixSyncClient::random_write_bit(
    std::string_view device,
    BitValue value) noexcept {
  const std::array<highlevel::RandomWriteBitSpec, 1> items {{
      {.device = device, .value = value},
  }};
  return random_write_bits(items);
}

Status PosixSyncClient::register_monitor(
    std::span<const highlevel::RandomReadSpec> items) noexcept {
  std::array<RandomReadItem, kMaxMonitorItems> parsed_items {};
  MonitorRegistration request {};
  Status status = highlevel::make_monitor_registration(items, parsed_items, request);
  if (!status.ok()) {
    return status;
  }

  status = client_.async_register_monitor(
      now_ms(),
      request,
      &PosixSyncClient::on_request_complete,
      &completion_);
  if (!status.ok()) {
    return status;
  }
  return run_until_complete();
}

Status PosixSyncClient::register_monitor(
    std::string_view device,
    bool double_word) noexcept {
  const std::array<highlevel::RandomReadSpec, 1> items {{
      {.device = device, .double_word = double_word},
  }};
  return register_monitor(items);
}

Status PosixSyncClient::register_extended_file_register_monitor(
    const ExtendedFileRegisterMonitorRegistration& request) noexcept {
  const Status status = client_.async_register_extended_file_register_monitor(
      now_ms(),
      request,
      &PosixSyncClient::on_request_complete,
      &completion_);
  if (!status.ok()) {
    return status;
  }
  return run_until_complete();
}

Status PosixSyncClient::read_monitor(std::span<std::uint32_t> out_values) noexcept {
  const Status status = client_.async_read_monitor(
      now_ms(),
      out_values,
      &PosixSyncClient::on_request_complete,
      &completion_);
  if (!status.ok()) {
    return status;
  }
  return run_until_complete();
}

Status PosixSyncClient::read_monitor(std::uint32_t& out_value) noexcept {
  return read_monitor(std::span<std::uint32_t>(&out_value, 1U));
}

Status PosixSyncClient::read_extended_file_register_monitor(
    std::span<std::uint16_t> out_words) noexcept {
  const Status status = client_.async_read_extended_file_register_monitor(
      now_ms(),
      out_words,
      &PosixSyncClient::on_request_complete,
      &completion_);
  if (!status.ok()) {
    return status;
  }
  return run_until_complete();
}

Status PosixSyncClient::write_bits(
    std::string_view head_device,
    std::span<const BitValue> bits) noexcept {
  BatchWriteBitsRequest request {};
  Status status = highlevel::make_batch_write_bits_request(head_device, bits, request);
  if (!status.ok()) {
    return status;
  }

  status = client_.async_batch_write_bits(
      now_ms(),
      request,
      &PosixSyncClient::on_request_complete,
      &completion_);
  if (!status.ok()) {
    return status;
  }
  return run_until_complete();
}

}  // namespace mcprotocol::serial
