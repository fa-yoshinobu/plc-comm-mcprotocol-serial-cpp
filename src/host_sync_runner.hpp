#pragma once

#include "mcprotocol/serial/client.hpp"
#include "mcprotocol/serial/posix_serial.hpp"

#include <cstdint>

namespace mcprotocol::serial::detail {

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
    std::size_t received = 0;
    status = port.read_some_until(rx_buffer, transaction_deadline_ms, received);
    if (!status.ok()) {
      const Status notify_status = client.notify_rx_failure(status);
      port.close();
      if (completion.done) {
        return completion.status;
      }
      return notify_status.ok() ? status : notify_status;
    }

    if (received > 0U) {
      const auto received_bytes =
          mcprotocol::serial::Span<const mcprotocol::serial::Byte>(rx_buffer.data(), received);
      trace("MC RX", received_bytes);
      client.on_rx_bytes(clock(), received_bytes);
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
