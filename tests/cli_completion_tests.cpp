#include <array>
#include <cassert>
#include <cstdio>
#include <string>

#define MCPROTOCOL_SERIAL_CLI_TESTING 1
#include "../tools/mcprotocol_cli.cpp"

namespace {

enum class CliFailureStage {
  Write,
  Drain,
  ReadTransport,
  ReadTimeout,
};

struct FakeCliPort {
  CliFailureStage failure;
  int read_calls = 0;

  Status flush_rx() noexcept {
    return mcprotocol::serial::ok_status();
  }

  Status write_all_until(
      mcprotocol::serial::Span<const mcprotocol::serial::Byte>,
      std::uint32_t) noexcept {
    if (failure == CliFailureStage::Write) {
      return mcprotocol::serial::make_status(StatusCode::Transport, "simulated CLI write failure");
    }
    return mcprotocol::serial::ok_status();
  }

  Status drain_tx_until(std::uint32_t) noexcept {
    if (failure == CliFailureStage::Drain) {
      return mcprotocol::serial::make_status(StatusCode::Timeout, "simulated CLI drain timeout");
    }
    return mcprotocol::serial::ok_status();
  }

  Status read_some_until(
      mcprotocol::serial::Span<mcprotocol::serial::Byte>,
      std::uint32_t,
      std::size_t& out_size) noexcept {
    out_size = 0U;
    if (read_calls++ < 2) {
      return mcprotocol::serial::make_status(StatusCode::Timeout, "stale RX quiet period");
    }
    if (failure == CliFailureStage::ReadTransport) {
      return mcprotocol::serial::make_status(StatusCode::Transport, "simulated CLI read failure");
    }
    assert(failure == CliFailureStage::ReadTimeout);
    return mcprotocol::serial::make_status(StatusCode::Timeout, "simulated CLI receive timeout");
  }
};

ProtocolConfig cli_test_config() {
  return ProtocolConfig::c4_binary(
      PlcProfile::MelsecQ,
      SumCheckMode::Enabled,
      RouteConfig {HostStationRoute {}});
}

Status run_cli_write(CliFailureStage failure) {
  MelsecSerialClient client;
  assert(client.configure(cli_test_config()).ok());

  CommandState state;
  const std::array<std::uint16_t, 1> values {0x1234U};
  assert(client.async_batch_write_words(
      now_ms(),
      BatchWriteWordsRequest(DeviceAddress {DeviceCode::D, 100U}, values),
      request_complete,
      &state).ok());

  FakeCliPort port {failure};
  return drive_request(client, port, state);
}

Status run_cli_read(CliFailureStage failure) {
  MelsecSerialClient client;
  assert(client.configure(cli_test_config()).ok());

  CommandState state;
  std::array<std::uint16_t, 1> values {};
  assert(client.async_batch_read_words(
      now_ms(),
      BatchReadWordsRequest(DeviceAddress {DeviceCode::D, 100U}, 1U),
      values,
      request_complete,
      &state).ok());

  FakeCliPort port {failure};
  return drive_request(client, port, state);
}

std::string render_status(Status status) {
  std::FILE* stream = std::tmpfile();
  assert(stream != nullptr);
  print_status_value(stream, status);
  assert(std::fflush(stream) == 0);
  assert(std::fseek(stream, 0, SEEK_SET) == 0);

  std::array<char, 512> buffer {};
  const std::size_t size = std::fread(buffer.data(), 1U, buffer.size() - 1U, stream);
  assert(std::fclose(stream) == 0);
  return std::string(buffer.data(), size);
}

void test_cli_returns_completed_core_status() {
  Status status = run_cli_write(CliFailureStage::Write);
  assert(status.code == StatusCode::OperationOutcomeUnknown);
  assert(status.cause == StatusCode::Transport);

  status = run_cli_write(CliFailureStage::Drain);
  assert(status.code == StatusCode::OperationOutcomeUnknown);
  assert(status.cause == StatusCode::Timeout);

  status = run_cli_write(CliFailureStage::ReadTransport);
  assert(status.code == StatusCode::OperationOutcomeUnknown);
  assert(status.cause == StatusCode::Transport);

  status = run_cli_write(CliFailureStage::ReadTimeout);
  assert(status.code == StatusCode::OperationOutcomeUnknown);
  assert(status.cause == StatusCode::Timeout);

  status = run_cli_read(CliFailureStage::Write);
  assert(status.code == StatusCode::Transport);
  assert(status.cause == StatusCode::Ok);

  status = run_cli_read(CliFailureStage::Drain);
  assert(status.code == StatusCode::Timeout);
  assert(status.cause == StatusCode::Ok);

  status = run_cli_read(CliFailureStage::ReadTransport);
  assert(status.code == StatusCode::Transport);
  assert(status.cause == StatusCode::Ok);

  status = run_cli_read(CliFailureStage::ReadTimeout);
  assert(status.code == StatusCode::Timeout);
  assert(status.cause == StatusCode::Ok);
}

void test_cli_diagnostic_includes_classification_and_cause() {
  const Status status = mcprotocol::serial::make_outcome_unknown_status(
      StatusCode::Timeout,
      "simulated unknown outcome");
  const std::string rendered = render_status(status);
  assert(rendered.find("OperationOutcomeUnknown") != std::string::npos);
  assert(rendered.find("cause=Timeout") != std::string::npos);
}

}  // namespace

int main() {
  test_cli_returns_completed_core_status();
  test_cli_diagnostic_includes_classification_and_cause();
  return 0;
}
