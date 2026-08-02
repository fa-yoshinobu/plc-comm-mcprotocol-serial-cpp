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

void test_raw_cli_redecodes_remaining_bytes_after_foreign_format2_frame() {
  const ProtocolConfig config = ProtocolConfig::ascii(
      mcprotocol::serial::AsciiFrameKind::C4,
      mcprotocol::serial::AsciiFormat::Format2,
      PlcProfile::MelsecQ,
      SumCheckMode::Enabled,
      RouteConfig {HostStationRoute {}});
  const auto expected_context = mcprotocol::serial::FrameCodecContext::format2(7U);
  const auto foreign_context = mcprotocol::serial::FrameCodecContext::format2(6U);

  std::array<std::uint8_t, 128> foreign {};
  std::array<std::uint8_t, 128> expected {};
  std::size_t foreign_size = 0U;
  std::size_t expected_size = 0U;
  assert(mcprotocol::serial::FrameCodec::encode_success_response(
             config,
             foreign_context,
             mcprotocol::serial::Span<const std::uint8_t> {},
             foreign,
             foreign_size)
             .ok());
  assert(mcprotocol::serial::FrameCodec::encode_success_response(
             config,
             expected_context,
             mcprotocol::serial::Span<const std::uint8_t> {},
             expected,
             expected_size)
             .ok());

  {
    std::array<std::uint8_t, 256> buffer {};
    std::memcpy(buffer.data(), foreign.data(), foreign_size);
    std::memcpy(buffer.data() + foreign_size, expected.data(), expected_size);
    std::size_t size = foreign_size + expected_size;
    const RawResponseBufferResult result =
        decode_raw_response_buffer(config, expected_context, buffer.data(), size);
    assert(result.status == mcprotocol::serial::DecodeStatus::Complete);
    assert(!result.candidate_retained);
    assert(size == expected_size);
  }

  {
    std::array<std::uint8_t, 256> buffer {};
    std::memcpy(buffer.data(), foreign.data(), foreign_size);
    std::memcpy(buffer.data() + foreign_size, expected.data(), 1U);
    std::size_t size = foreign_size + 1U;
    const RawResponseBufferResult result =
        decode_raw_response_buffer(config, expected_context, buffer.data(), size);
    assert(result.status == mcprotocol::serial::DecodeStatus::Incomplete);
    assert(result.candidate_retained);
    assert(size == 1U);
    assert(buffer[0] == expected[0]);
  }

  {
    std::array<std::uint8_t, 128> buffer = foreign;
    std::size_t size = foreign_size;
    const RawResponseBufferResult result =
        decode_raw_response_buffer(config, expected_context, buffer.data(), size);
    assert(result.status == mcprotocol::serial::DecodeStatus::Incomplete);
    assert(!result.candidate_retained);
    assert(size == 0U);
  }
}

void test_raw_cli_discards_noise_before_starting_candidate_timeout() {
  const ProtocolConfig ascii_config = ProtocolConfig::ascii(
      mcprotocol::serial::AsciiFrameKind::C4,
      mcprotocol::serial::AsciiFormat::Format2,
      PlcProfile::MelsecQ,
      SumCheckMode::Enabled,
      RouteConfig {HostStationRoute {}});
  const auto ascii_context = mcprotocol::serial::FrameCodecContext::format2(7U);
  std::array<std::uint8_t, 64> ascii_frame {};
  std::size_t ascii_frame_size = 0U;
  assert(mcprotocol::serial::FrameCodec::encode_success_response(
             ascii_config,
             ascii_context,
             mcprotocol::serial::Span<const std::uint8_t> {},
             ascii_frame,
             ascii_frame_size)
             .ok());

  {
    std::array<std::uint8_t, 4> noise {0x55U};
    std::size_t size = 1U;
    const RawResponseBufferResult result =
        decode_raw_response_buffer(ascii_config, ascii_context, noise.data(), size);
    assert(result.status == mcprotocol::serial::DecodeStatus::Incomplete);
    assert(!result.candidate_retained);
    assert(size == 0U);
  }
  {
    std::array<std::uint8_t, 4> noise_then_start {0x55U, ascii_frame[0]};
    std::size_t size = 2U;
    const RawResponseBufferResult result = decode_raw_response_buffer(
        ascii_config, ascii_context, noise_then_start.data(), size);
    assert(result.status == mcprotocol::serial::DecodeStatus::Incomplete);
    assert(result.candidate_retained);
    assert(size == 1U);
    assert(noise_then_start[0] == ascii_frame[0]);
  }

  const ProtocolConfig binary_config = ProtocolConfig::c4_binary(
      PlcProfile::MelsecQ,
      SumCheckMode::Enabled,
      RouteConfig {HostStationRoute {}});
  {
    std::array<std::uint8_t, 4> noise_then_start {0x55U, 0x10U};
    std::size_t size = 2U;
    const RawResponseBufferResult result = decode_raw_response_buffer(
        binary_config,
        mcprotocol::serial::FrameCodecContext::none(),
        noise_then_start.data(),
        size);
    assert(result.status == mcprotocol::serial::DecodeStatus::Incomplete);
    assert(result.candidate_retained);
    assert(size == 1U);
    assert(noise_then_start[0] == 0x10U);
  }
}

void test_cli_bit_device_classification_includes_long_state_families() {
  const std::array<DeviceCode, 6> long_state_bits {{
      DeviceCode::LTS,
      DeviceCode::LTC,
      DeviceCode::LSTS,
      DeviceCode::LSTC,
      DeviceCode::LCS,
      DeviceCode::LCC,
  }};
  for (const DeviceCode code : long_state_bits) {
    assert(is_bit_device(code));
  }
  assert(!is_bit_device(DeviceCode::LTN));
  assert(!is_bit_device(DeviceCode::LCN));
  assert(!is_bit_device(DeviceCode::G));
  assert(!is_bit_device(DeviceCode::HG));
}

}  // namespace

int main() {
  test_cli_returns_completed_core_status();
  test_cli_diagnostic_includes_classification_and_cause();
  test_raw_cli_redecodes_remaining_bytes_after_foreign_format2_frame();
  test_raw_cli_discards_noise_before_starting_candidate_timeout();
  test_cli_bit_device_classification_includes_long_state_families();
  return 0;
}
