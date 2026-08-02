#include <array>
#include <cassert>
#include <cstdint>

#include "host_sync_runner.hpp"
#include "mcprotocol/serial/high_level.hpp"

namespace {

using mcprotocol::serial::Byte;
using mcprotocol::serial::CodeMode;
using mcprotocol::serial::DeviceAddress;
using mcprotocol::serial::DeviceCode;
using mcprotocol::serial::ExtendedFileRegisterAddress;
using mcprotocol::serial::ExtendedFileRegisterMonitorRegistration;
using mcprotocol::serial::HostStationRoute;
using mcprotocol::serial::MelsecSerialClient;
using mcprotocol::serial::MonitorRegistration;
using mcprotocol::serial::PlcProfile;
using mcprotocol::serial::ProtocolConfig;
using mcprotocol::serial::RandomReadWordItem;
using mcprotocol::serial::RouteConfig;
using mcprotocol::serial::Span;
using mcprotocol::serial::Status;
using mcprotocol::serial::StatusCode;
using mcprotocol::serial::SumCheckMode;

enum class FailureStage {
  None,
  Flush,
  Write,
  Drain,
  ReadTransport,
  ReadTimeout,
  CoreTimeout,
};

struct CompletionState {
  bool done = false;
  Status status {};
};

void capture_completion(void* user, Status status) noexcept {
  auto* completion = static_cast<CompletionState*>(user);
  completion->done = true;
  completion->status = status;
}

struct FakeClock {
  bool expire = false;
  std::uint32_t calls = 0;

  std::uint32_t operator()() noexcept {
    ++calls;
    if (expire && calls >= 3U) {
      return 101U;
    }
    return calls;
  }
};

struct FakePort {
  FailureStage failure = FailureStage::None;
  bool closed = false;

  Status flush_rx() noexcept {
    if (failure == FailureStage::Flush) {
      return mcprotocol::serial::make_status(StatusCode::Transport, "simulated flush failure");
    }
    return mcprotocol::serial::ok_status();
  }

  Status write_all_until(Span<const Byte>, std::uint32_t) noexcept {
    if (failure == FailureStage::Write) {
      return mcprotocol::serial::make_status(StatusCode::Transport, "simulated write failure");
    }
    return mcprotocol::serial::ok_status();
  }

  Status drain_tx_until(std::uint32_t) noexcept {
    if (failure == FailureStage::Drain) {
      return mcprotocol::serial::make_status(StatusCode::Timeout, "simulated drain timeout");
    }
    return mcprotocol::serial::ok_status();
  }

  Status read_some_until(Span<Byte>, std::uint32_t, std::size_t& out_size) noexcept {
    out_size = 0U;
    if (failure == FailureStage::ReadTransport) {
      return mcprotocol::serial::make_status(StatusCode::Transport, "simulated read failure");
    }
    if (failure == FailureStage::ReadTimeout) {
      return mcprotocol::serial::make_status(StatusCode::Timeout, "simulated receive timeout");
    }
    return mcprotocol::serial::ok_status();
  }

  void close() noexcept {
    closed = true;
  }
};

struct ScriptedClock {
  std::array<std::uint32_t, 7> values {};
  std::size_t index = 0U;

  std::uint32_t operator()() noexcept {
    const std::size_t selected = index < values.size() ? index : values.size() - 1U;
    ++index;
    return values[selected];
  }
};

struct PartialResponsePort {
  Byte first_byte {};
  std::size_t read_calls = 0U;
  std::array<std::uint32_t, 2> read_deadlines {};
  bool closed = false;

  Status flush_rx() noexcept { return mcprotocol::serial::ok_status(); }
  Status write_all_until(Span<const Byte>, std::uint32_t) noexcept {
    return mcprotocol::serial::ok_status();
  }
  Status drain_tx_until(std::uint32_t) noexcept { return mcprotocol::serial::ok_status(); }
  Status read_some_until(
      Span<Byte> out,
      std::uint32_t deadline,
      std::size_t& out_size) noexcept {
    if (read_calls < read_deadlines.size()) {
      read_deadlines[read_calls] = deadline;
    }
    ++read_calls;
    if (read_calls == 1U) {
      out[0] = first_byte;
      out_size = 1U;
      return mcprotocol::serial::ok_status();
    }
    out_size = 0U;
    return mcprotocol::serial::make_status(StatusCode::Timeout, "bounded receive wait expired");
  }
  void close() noexcept { closed = true; }
};

ProtocolConfig normal_monitor_config() {
  return ProtocolConfig::c4_binary(
             PlcProfile::MelsecIqR,
             SumCheckMode::Enabled,
             RouteConfig {HostStationRoute {}})
      .with_response_timeout_ms(100U);
}

ProtocolConfig extended_monitor_config() {
  return ProtocolConfig::e1(
             CodeMode::Binary,
             PlcProfile::MelsecA,
             RouteConfig {HostStationRoute {}})
      .with_response_timeout_ms(100U);
}

Status run_normal_monitor(FailureStage failure, bool read_only = false) {
  MelsecSerialClient client;
  const ProtocolConfig config = normal_monitor_config();
  assert(client.configure(config).ok());

  CompletionState completion;
  std::array<std::uint16_t, 1> read_words {};
  if (read_only) {
    const mcprotocol::serial::BatchReadWordsRequest request(
        DeviceAddress {DeviceCode::D, 100U},
        1U);
    assert(client.async_batch_read_words(
        0U,
        request,
        read_words,
        capture_completion,
        &completion).ok());
  } else {
    const RandomReadWordItem item {DeviceAddress {DeviceCode::D, 100U}};
    assert(client.async_register_monitor(
        0U,
        MonitorRegistration(Span<const RandomReadWordItem>(&item, 1U), {}),
        capture_completion,
        &completion).ok());
  }

  FakePort port {failure};
  FakeClock clock {failure == FailureStage::CoreTimeout};
  std::array<Byte, 256> rx_buffer {};
  return mcprotocol::serial::detail::run_synchronous_request(
      client,
      port,
      config,
      rx_buffer,
      completion,
      clock,
      [](const char*, Span<const Byte>) noexcept {});
}

Status run_extended_monitor(FailureStage failure) {
  MelsecSerialClient client;
  const ProtocolConfig config = extended_monitor_config();
  assert(client.configure(config).ok());

  CompletionState completion;
  const ExtendedFileRegisterAddress item {2U, 70U};
  assert(client.async_register_extended_file_register_monitor(
      0U,
      ExtendedFileRegisterMonitorRegistration(
          Span<const ExtendedFileRegisterAddress>(&item, 1U)),
      capture_completion,
      &completion).ok());

  FakePort port {failure};
  FakeClock clock {failure == FailureStage::CoreTimeout};
  std::array<Byte, 256> rx_buffer {};
  return mcprotocol::serial::detail::run_synchronous_request(
      client,
      port,
      config,
      rx_buffer,
      completion,
      clock,
      [](const char*, Span<const Byte>) noexcept {});
}

void test_monitor_registration_uses_core_completion_status() {
  Status status = run_normal_monitor(FailureStage::Flush);
  assert(status.code == StatusCode::Transport);

  status = run_normal_monitor(FailureStage::Write);
  assert(status.code == StatusCode::OperationOutcomeUnknown);
  assert(status.cause == StatusCode::Transport);

  status = run_normal_monitor(FailureStage::Drain);
  assert(status.code == StatusCode::OperationOutcomeUnknown);
  assert(status.cause == StatusCode::Timeout);

  status = run_normal_monitor(FailureStage::ReadTransport);
  assert(status.code == StatusCode::OperationOutcomeUnknown);
  assert(status.cause == StatusCode::Transport);

  status = run_normal_monitor(FailureStage::ReadTimeout);
  assert(status.code == StatusCode::OperationOutcomeUnknown);
  assert(status.cause == StatusCode::Timeout);

  status = run_normal_monitor(FailureStage::CoreTimeout);
  assert(status.code == StatusCode::OperationOutcomeUnknown);
  assert(status.cause == StatusCode::Timeout);

  status = run_extended_monitor(FailureStage::Write);
  assert(status.code == StatusCode::OperationOutcomeUnknown);
  assert(status.cause == StatusCode::Transport);

  status = run_extended_monitor(FailureStage::ReadTransport);
  assert(status.code == StatusCode::OperationOutcomeUnknown);
  assert(status.cause == StatusCode::Transport);

  status = run_extended_monitor(FailureStage::ReadTimeout);
  assert(status.code == StatusCode::OperationOutcomeUnknown);
  assert(status.cause == StatusCode::Timeout);
}

void test_read_only_failures_keep_core_classification() {
  Status status = run_normal_monitor(FailureStage::Write, true);
  assert(status.code == StatusCode::Transport);
  assert(status.cause == StatusCode::Ok);

  status = run_normal_monitor(FailureStage::ReadTransport, true);
  assert(status.code == StatusCode::Transport);
  assert(status.cause == StatusCode::Ok);

  status = run_normal_monitor(FailureStage::ReadTimeout, true);
  assert(status.code == StatusCode::Timeout);
  assert(status.cause == StatusCode::Ok);

  status = run_normal_monitor(FailureStage::CoreTimeout, true);
  assert(status.code == StatusCode::Timeout);
  assert(status.cause == StatusCode::Ok);
}

void test_partial_response_uses_inter_byte_deadline_in_host_runner() {
  const ProtocolConfig config = normal_monitor_config()
                                    .with_response_timeout_ms(1000U)
                                    .with_inter_byte_timeout_ms(250U);
  MelsecSerialClient client;
  assert(client.configure(config).ok());

  CompletionState completion;
  std::array<std::uint16_t, 1> words {};
  const mcprotocol::serial::BatchReadWordsRequest request(
      DeviceAddress {DeviceCode::D, 100U}, 1U);
  assert(client.async_batch_read_words(
                   0U, request, words, capture_completion, &completion)
             .ok());

  const std::array<std::uint8_t, 2> response_data {0x34U, 0x12U};
  std::array<std::uint8_t, 64> response_frame {};
  std::size_t response_size = 0U;
  assert(mcprotocol::serial::FrameCodec::encode_success_response(
             config, response_data, response_frame, response_size)
             .ok());

  PartialResponsePort port {
      static_cast<Byte>(response_frame[0])};
  ScriptedClock clock {{0U, 0U, 0U, 100U, 100U, 100U, 350U}};
  std::array<Byte, 256> rx_buffer {};
  const Status status = mcprotocol::serial::detail::run_synchronous_request(
      client,
      port,
      config,
      rx_buffer,
      completion,
      clock,
      [](const char*, Span<const Byte>) noexcept {});

  assert(status.code == StatusCode::Timeout);
  assert(port.read_calls == 2U);
  assert(port.read_deadlines[0] == 1000U);
  assert(port.read_deadlines[1] == 350U);
  assert(port.closed);
}

void test_noise_does_not_shorten_host_runner_read_deadline() {
  const ProtocolConfig config = normal_monitor_config()
                                    .with_response_timeout_ms(1000U)
                                    .with_inter_byte_timeout_ms(250U);
  MelsecSerialClient client;
  assert(client.configure(config).ok());

  CompletionState completion;
  std::array<std::uint16_t, 1> words {};
  const mcprotocol::serial::BatchReadWordsRequest request(
      DeviceAddress {DeviceCode::D, 100U}, 1U);
  assert(client.async_batch_read_words(
                   0U, request, words, capture_completion, &completion)
             .ok());

  PartialResponsePort port {static_cast<Byte>(0x55U)};
  ScriptedClock clock {{0U, 0U, 0U, 100U, 100U, 100U, 350U}};
  std::array<Byte, 256> rx_buffer {};
  const Status status = mcprotocol::serial::detail::run_synchronous_request(
      client,
      port,
      config,
      rx_buffer,
      completion,
      clock,
      [](const char*, Span<const Byte>) noexcept {});

  assert(status.code == StatusCode::Timeout);
  assert(port.read_calls == 2U);
  assert(port.read_deadlines[0] == 1000U);
  assert(port.read_deadlines[1] == 1000U);
  assert(port.closed);
}

void test_partial_response_inter_byte_deadline_wraps_in_host_runner() {
  const ProtocolConfig config = normal_monitor_config()
                                    .with_response_timeout_ms(1000U)
                                    .with_inter_byte_timeout_ms(10U);
  MelsecSerialClient client;
  assert(client.configure(config).ok());

  CompletionState completion;
  std::array<std::uint16_t, 1> words {};
  const mcprotocol::serial::BatchReadWordsRequest request(
      DeviceAddress {DeviceCode::D, 100U}, 1U);
  assert(client.async_batch_read_words(
                   0U, request, words, capture_completion, &completion)
             .ok());

  const std::array<std::uint8_t, 2> response_data {0x34U, 0x12U};
  std::array<std::uint8_t, 64> response_frame {};
  std::size_t response_size = 0U;
  assert(mcprotocol::serial::FrameCodec::encode_success_response(
             config, response_data, response_frame, response_size)
             .ok());

  PartialResponsePort port {static_cast<Byte>(response_frame[0])};
  ScriptedClock clock {{
      0xFFFFFF00U,
      0xFFFFFF00U,
      0xFFFFFF00U,
      0xFFFFFFFAU,
      0xFFFFFFFAU,
      0xFFFFFFFAU,
      0x00000004U}};
  std::array<Byte, 256> rx_buffer {};
  const Status status = mcprotocol::serial::detail::run_synchronous_request(
      client,
      port,
      config,
      rx_buffer,
      completion,
      clock,
      [](const char*, Span<const Byte>) noexcept {});

  assert(status.code == StatusCode::Timeout);
  assert(port.read_calls == 2U);
  assert(port.read_deadlines[0] == 0x000002E8U);
  assert(port.read_deadlines[1] == 0x00000004U);
  assert(port.closed);
}

}  // namespace

int main() {
  test_monitor_registration_uses_core_completion_status();
  test_read_only_failures_keep_core_classification();
  test_partial_response_uses_inter_byte_deadline_in_host_runner();
  test_noise_does_not_shorten_host_runner_read_deadline();
  test_partial_response_inter_byte_deadline_wraps_in_host_runner();
  return 0;
}
