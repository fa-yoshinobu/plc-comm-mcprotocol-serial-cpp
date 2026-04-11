#include "mcprotocol/serial/host_sync.hpp"

#include <chrono>

namespace mcprotocol::serial {
namespace {

[[nodiscard]] std::uint32_t now_ms() noexcept {
  const auto now = std::chrono::steady_clock::now().time_since_epoch();
  return static_cast<std::uint32_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(now).count());
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
