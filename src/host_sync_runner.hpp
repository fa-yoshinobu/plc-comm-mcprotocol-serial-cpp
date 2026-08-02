#pragma once

#include "mcprotocol/serial/client.hpp"
#include "mcprotocol/serial/posix_serial.hpp"

#include <cstdint>

namespace mcprotocol::serial::detail {

struct ClientAccess {
  [[nodiscard]] static bool inter_byte_deadline(
      const MelsecSerialClient& client,
      std::uint32_t& out_deadline_ms) noexcept {
    if (!client.inter_byte_deadline_active_) {
      out_deadline_ms = 0U;
      return false;
    }
    out_deadline_ms = client.inter_byte_deadline_ms_;
    return true;
  }
};

template <typename Port, typename CompletionState, typename Clock, typename Trace>
[[nodiscard]] Status run_synchronous_request(
    MelsecSerialClient& client,
    Port& port,
    const ProtocolConfig& protocol_config,
    mcprotocol::serial::Span<mcprotocol::serial::Byte> rx_buffer,
    CompletionState& completion,
    Clock&& clock,
    Trace&& trace) noexcept {
  completion = {};

  Status status = port.flush_rx();
  if (!status.ok()) {
    client.cancel();
    port.close();
    if (client.busy()) {
      (void)client.notify_tx_complete(clock(), status);
    }
    return status;
  }

  const std::uint32_t transaction_start_ms = clock();
  const std::uint32_t transaction_deadline_ms =
      transaction_start_ms + protocol_config.timeout().response_timeout_ms;
  status = client.notify_tx_started(transaction_start_ms);
  if (!status.ok()) {
    client.cancel();
    port.close();
    return status;
  }

  trace("MC TX", client.pending_tx_frame());
  status = port.write_all_until(client.pending_tx_frame(), transaction_deadline_ms);
  if (!status.ok()) {
    const Status notify_status = client.notify_tx_complete(clock(), status);
    port.close();
    if (completion.done) {
      return completion.status;
    }
    return notify_status.ok() ? status : notify_status;
  }

  status = port.drain_tx_until(transaction_deadline_ms);
  if (!status.ok()) {
    const Status notify_status = client.notify_tx_complete(clock(), status);
    port.close();
    if (completion.done) {
      return completion.status;
    }
    return notify_status.ok() ? status : notify_status;
  }

  status = client.notify_tx_complete(clock(), mcprotocol::serial::ok_status());
  if (!status.ok()) {
    client.cancel();
    port.close();
    if (completion.done) {
      return completion.status;
    }
    return status;
  }
  if (completion.done) {
    return completion.status;
  }

  while (!completion.done) {
    const std::uint32_t before_read_ms = clock();
    client.poll(before_read_ms);
    if (completion.done) {
      break;
    }
    std::uint32_t read_deadline_ms = transaction_deadline_ms;
    bool inter_byte_wakeup = false;
    std::uint32_t possible_inter_byte_deadline = 0U;
    if (ClientAccess::inter_byte_deadline(client, possible_inter_byte_deadline)) {
      if (static_cast<std::int32_t>(possible_inter_byte_deadline - before_read_ms) <
          static_cast<std::int32_t>(transaction_deadline_ms - before_read_ms)) {
        read_deadline_ms = possible_inter_byte_deadline;
        inter_byte_wakeup = true;
      }
    }

    std::size_t received = 0;
    status = port.read_some_until(rx_buffer, read_deadline_ms, received);
    if (!status.ok()) {
      if (status.code == StatusCode::Timeout && inter_byte_wakeup) {
        client.poll(clock());
        if (completion.done) {
          break;
        }
        continue;
      }
      const Status notify_status = client.notify_rx_failure(status);
      port.close();
      if (completion.done) {
        return completion.status;
      }
      return notify_status.ok() ? status : notify_status;
    }

    if (received > 0U) {
      const std::uint32_t received_ms = clock();
      const auto received_bytes =
          mcprotocol::serial::Span<const mcprotocol::serial::Byte>(rx_buffer.data(), received);
      trace("MC RX", received_bytes);
      client.on_rx_bytes(received_ms, received_bytes);
      if (completion.done) {
        break;
      }
    }

    client.poll(clock());
  }

  if (completion.status.code == StatusCode::Timeout ||
      completion.status.code == StatusCode::OperationOutcomeUnknown ||
      completion.status.code == StatusCode::Transport ||
      client.requires_transport_reset()) {
    port.close();
  }
  return completion.status;
}

}  // namespace mcprotocol::serial::detail
