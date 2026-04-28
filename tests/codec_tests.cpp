#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <span>
#include <string_view>

#include "mcprotocol/serial/client.hpp"
#include "mcprotocol/serial/high_level.hpp"
#include "mcprotocol/serial/link_direct.hpp"
#include "mcprotocol/serial/qualified_buffer.hpp"

namespace {

using mcprotocol::serial::AsciiFormat;
using mcprotocol::serial::BatchReadBitsRequest;
using mcprotocol::serial::BatchReadWordsRequest;
using mcprotocol::serial::BatchWriteBitsRequest;
using mcprotocol::serial::BatchWriteWordsRequest;
using mcprotocol::serial::BitValue;
using mcprotocol::serial::CodeMode;
using mcprotocol::serial::CompletionHandler;
using mcprotocol::serial::CpuModelInfo;
using mcprotocol::serial::DecodeStatus;
using mcprotocol::serial::ExtendedFileRegisterAddress;
using mcprotocol::serial::ExtendedFileRegisterBatchReadWordsRequest;
using mcprotocol::serial::ExtendedFileRegisterBatchWriteWordsRequest;
using mcprotocol::serial::ExtendedFileRegisterDirectBatchReadWordsRequest;
using mcprotocol::serial::ExtendedFileRegisterDirectBatchWriteWordsRequest;
using mcprotocol::serial::ExtendedFileRegisterMonitorRegistration;
using mcprotocol::serial::ExtendedFileRegisterRandomWriteWordItem;
using mcprotocol::serial::FrameCodec;
using mcprotocol::serial::FrameKind;
using mcprotocol::serial::GlobalSignalControlRequest;
using mcprotocol::serial::GlobalSignalTarget;
using mcprotocol::serial::LinkDirectDevice;
using mcprotocol::serial::LinkDirectMonitorRegistration;
using mcprotocol::serial::LinkDirectMultiBlockReadBlock;
using mcprotocol::serial::LinkDirectMultiBlockReadRequest;
using mcprotocol::serial::LinkDirectMultiBlockWriteBlock;
using mcprotocol::serial::LinkDirectMultiBlockWriteRequest;
using mcprotocol::serial::LinkDirectRandomReadItem;
using mcprotocol::serial::LinkDirectRandomWriteBitItem;
using mcprotocol::serial::LinkDirectRandomWriteWordItem;
using mcprotocol::serial::MelsecSerialClient;
using mcprotocol::serial::MonitorRegistration;
using mcprotocol::serial::ModuleBufferReadRequest;
using mcprotocol::serial::ModuleBufferWriteRequest;
using mcprotocol::serial::MultiBlockReadBlock;
using mcprotocol::serial::MultiBlockReadBlockResult;
using mcprotocol::serial::MultiBlockReadRequest;
using mcprotocol::serial::MultiBlockWriteBlock;
using mcprotocol::serial::MultiBlockWriteRequest;
using mcprotocol::serial::PlcSeries;
using mcprotocol::serial::ProtocolConfig;
using mcprotocol::serial::QualifiedBufferDeviceKind;
using mcprotocol::serial::QualifiedBufferWordDevice;
using mcprotocol::serial::RandomReadItem;
using mcprotocol::serial::RandomReadRequest;
using mcprotocol::serial::RandomWriteBitItem;
using mcprotocol::serial::RandomWriteWordItem;
using mcprotocol::serial::RouteConfig;
using mcprotocol::serial::RouteKind;
using mcprotocol::serial::sparse_native_mask_word;
using mcprotocol::serial::sparse_native_requested_bit_value;
using mcprotocol::serial::Status;
using mcprotocol::serial::StatusCode;
using mcprotocol::serial::UserFrameDeleteRequest;
using mcprotocol::serial::UserFrameReadRequest;
using mcprotocol::serial::UserFrameRegistrationData;
using mcprotocol::serial::UserFrameWriteRequest;
using mcprotocol::serial::decode_qualified_buffer_word_values;
using mcprotocol::serial::highlevel::make_batch_read_words_request;
using mcprotocol::serial::highlevel::make_batch_write_bits_request;
using mcprotocol::serial::highlevel::make_c4_binary_protocol;
using mcprotocol::serial::highlevel::make_monitor_registration;
using mcprotocol::serial::highlevel::make_random_read_item;
using mcprotocol::serial::highlevel::make_random_read_request;
using mcprotocol::serial::highlevel::make_random_write_bit_item;
using mcprotocol::serial::highlevel::make_random_write_bit_items;
using mcprotocol::serial::highlevel::make_random_write_word_item;
using mcprotocol::serial::highlevel::make_random_write_word_items;
using mcprotocol::serial::highlevel::parse_device_address;
using mcprotocol::serial::highlevel::RandomReadSpec;
using mcprotocol::serial::highlevel::RandomWriteBitSpec;
using mcprotocol::serial::highlevel::RandomWriteWordSpec;
using mcprotocol::serial::make_qualified_buffer_read_words_request;
using mcprotocol::serial::make_qualified_buffer_write_words_request;
using mcprotocol::serial::parse_link_direct_device;
using mcprotocol::serial::parse_qualified_buffer_word_device;

namespace CommandCodec = mcprotocol::serial::CommandCodec;

ProtocolConfig make_binary_c4_config() {
  ProtocolConfig config;
  config.frame_kind = FrameKind::C4;
  config.code_mode = CodeMode::Binary;
  config.ascii_format = AsciiFormat::Format3;
  config.target_series = PlcSeries::Q_L;
  config.sum_check_enabled = true;
  config.route = RouteConfig {
      .kind = RouteKind::HostStation,
      .station_no = 0x00,
      .network_no = 0x00,
      .pc_no = 0xFF,
      .request_destination_module_io_no = 0x03FF,
      .request_destination_module_station_no = 0x00,
      .self_station_enabled = false,
      .self_station_no = 0x00,
  };
  return config;
}

ProtocolConfig make_binary_c4_iqr_config() {
  ProtocolConfig config = make_binary_c4_config();
  config.target_series = PlcSeries::IQ_R;
  return config;
}

ProtocolConfig make_ascii_c3_format3_config() {
  ProtocolConfig config;
  config.frame_kind = FrameKind::C3;
  config.code_mode = CodeMode::Ascii;
  config.ascii_format = AsciiFormat::Format3;
  config.target_series = PlcSeries::Q_L;
  config.sum_check_enabled = true;
  config.route = RouteConfig {
      .kind = RouteKind::HostStation,
      .station_no = 0x00,
      .network_no = 0x00,
      .pc_no = 0xFF,
      .request_destination_module_io_no = 0x03FF,
      .request_destination_module_station_no = 0x00,
      .self_station_enabled = false,
      .self_station_no = 0x00,
  };
  return config;
}

ProtocolConfig make_ascii_c2_format3_config() {
  ProtocolConfig config = make_ascii_c3_format3_config();
  config.frame_kind = FrameKind::C2;
  return config;
}

ProtocolConfig make_ascii_c4_format2_config() {
  ProtocolConfig config;
  config.frame_kind = FrameKind::C4;
  config.code_mode = CodeMode::Ascii;
  config.ascii_format = AsciiFormat::Format2;
  config.ascii_block_number = 0x00U;
  config.target_series = PlcSeries::Q_L;
  config.sum_check_enabled = true;
  config.route = RouteConfig {
      .kind = RouteKind::HostStation,
      .station_no = 0x00,
      .network_no = 0x00,
      .pc_no = 0xFF,
      .request_destination_module_io_no = 0x03FF,
      .request_destination_module_station_no = 0x00,
      .self_station_enabled = false,
      .self_station_no = 0x00,
  };
  return config;
}

ProtocolConfig make_ascii_c2_format2_config() {
  ProtocolConfig config = make_ascii_c4_format2_config();
  config.frame_kind = FrameKind::C2;
  return config;
}

ProtocolConfig make_ascii_c4_format4_config() {
  ProtocolConfig config;
  config.frame_kind = FrameKind::C4;
  config.code_mode = CodeMode::Ascii;
  config.ascii_format = AsciiFormat::Format4;
  config.target_series = PlcSeries::Q_L;
  config.sum_check_enabled = false;
  config.route = RouteConfig {
      .kind = RouteKind::MultidropStation,
      .station_no = 0x01,
      .network_no = 0x00,
      .pc_no = 0xFF,
      .request_destination_module_io_no = 0x03FF,
      .request_destination_module_station_no = 0x00,
      .self_station_enabled = false,
      .self_station_no = 0x00,
  };
  return config;
}

ProtocolConfig make_ascii_c2_format4_config() {
  ProtocolConfig config = make_ascii_c4_format4_config();
  config.frame_kind = FrameKind::C2;
  return config;
}

ProtocolConfig make_ascii_c1_format4_qna_config() {
  ProtocolConfig config = make_ascii_c4_format4_config();
  config.frame_kind = FrameKind::C1;
  config.target_series = PlcSeries::QnA;
  config.route = RouteConfig {
      .kind = RouteKind::HostStation,
      .station_no = 0x00,
      .network_no = 0x00,
      .pc_no = 0xFF,
      .request_destination_module_io_no = 0x03FF,
      .request_destination_module_station_no = 0x00,
      .self_station_enabled = false,
      .self_station_no = 0x00,
  };
  return config;
}

ProtocolConfig make_ascii_c1_format4_a_config() {
  ProtocolConfig config = make_ascii_c1_format4_qna_config();
  config.target_series = PlcSeries::A;
  return config;
}

ProtocolConfig make_ascii_e1_a_config() {
  ProtocolConfig config = make_ascii_c4_format4_config();
  config.frame_kind = FrameKind::E1;
  config.target_series = PlcSeries::A;
  config.route = RouteConfig {
      .kind = RouteKind::HostStation,
      .station_no = 0x00,
      .network_no = 0x00,
      .pc_no = 0xFF,
      .request_destination_module_io_no = 0x03FF,
      .request_destination_module_station_no = 0x00,
      .self_station_enabled = false,
      .self_station_no = 0x00,
  };
  config.timeout.response_timeout_ms = 4000;
  return config;
}

ProtocolConfig make_binary_e1_a_config() {
  ProtocolConfig config = make_binary_c4_config();
  config.frame_kind = FrameKind::E1;
  config.target_series = PlcSeries::A;
  config.timeout.response_timeout_ms = 4000;
  return config;
}

ProtocolConfig make_ascii_c4_format4_iqr_config() {
  ProtocolConfig config = make_ascii_c4_format4_config();
  config.target_series = PlcSeries::IQ_R;
  return config;
}

void test_format5_batch_read_request_matches_manual() {
  const auto config = make_binary_c4_config();
  const BatchReadWordsRequest request {
      .head_device = {.code = mcprotocol::serial::DeviceCode::M, .number = 100},
      .points = 2,
  };

  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_data_size = 0;
  Status status = CommandCodec::encode_batch_read_words(config, request, request_data, request_data_size);
  assert(status.ok());

  std::array<std::uint8_t, 128> frame {};
  std::size_t frame_size = 0;
  status = FrameCodec::encode_request(
      config,
      std::span<const std::uint8_t>(request_data.data(), request_data_size),
      frame,
      frame_size);
  assert(status.ok());

  const std::array<std::uint8_t, 18> expected {
      0x10, 0x02, 0x12, 0x00, 0xF8, 0x00, 0x00, 0xFF, 0xFF, 0x03, 0x00, 0x00,
      0x01, 0x04, 0x00, 0x00, 0x64, 0x00,
  };

  assert(frame_size > expected.size());
  assert(std::equal(expected.begin(), expected.end(), frame.begin()));
  assert(frame[18] == 0x00);
  assert(frame[19] == 0x90);
  assert(frame[20] == 0x02);
  assert(frame[21] == 0x00);
  assert(frame[22] == 0x10);
  assert(frame[23] == 0x03);
  assert(frame[24] == '0');
  assert(frame[25] == '6');
}

void test_decode_binary_cpu_model_response() {
  const auto config = make_binary_c4_config();
  const std::array<std::uint8_t, 18> response_data {
      'Q', '0', '2', 'U', 'C', 'P', 'U', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
      0x63, 0x02,
  };

  std::array<std::uint8_t, 128> frame {};
  std::size_t frame_size = 0;
  Status status = FrameCodec::encode_success_response(config, response_data, frame, frame_size);
  assert(status.ok());

  const auto decode = FrameCodec::decode_response(
      config,
      std::span<const std::uint8_t>(frame.data(), frame_size));
  assert(decode.status == DecodeStatus::Complete);
  assert(decode.frame.response_size == response_data.size());

  CpuModelInfo info;
  status = CommandCodec::parse_read_cpu_model_response(
      config,
      std::span<const std::uint8_t>(decode.frame.response_data.data(), decode.frame.response_size),
      info);
  assert(status.ok());
  assert(std::string_view(info.model_name.data()) == "Q02UCPU");
  assert(info.model_code == 0x0263);
}

void test_encode_remote_reset_binary_request() {
  const auto config = make_binary_c4_config();
  std::array<std::uint8_t, 16> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_remote_reset(config, request_data, request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 6> expected {0x06, 0x10, 0x00, 0x00, 0x01, 0x00};
  assert(request_size == expected.size());
  assert(std::equal(expected.begin(), expected.end(), request_data.begin()));
}

void test_encode_remote_run_binary_request() {
  const auto config = make_binary_c4_config();
  std::array<std::uint8_t, 16> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_remote_run(
      config,
      mcprotocol::serial::RemoteOperationMode::DoNotExecuteForcibly,
      mcprotocol::serial::RemoteRunClearMode::AllClear,
      request_data,
      request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 8> expected {0x01, 0x10, 0x00, 0x00, 0x01, 0x00, 0x02, 0x00};
  assert(request_size == expected.size());
  assert(std::equal(expected.begin(), expected.end(), request_data.begin()));
}

void test_encode_remote_stop_binary_request() {
  const auto config = make_binary_c4_config();
  std::array<std::uint8_t, 16> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_remote_stop(config, request_data, request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 6> expected {0x02, 0x10, 0x00, 0x00, 0x01, 0x00};
  assert(request_size == expected.size());
  assert(std::equal(expected.begin(), expected.end(), request_data.begin()));
}

void test_encode_remote_pause_binary_request() {
  const auto config = make_binary_c4_config();
  std::array<std::uint8_t, 16> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_remote_pause(
      config,
      mcprotocol::serial::RemoteOperationMode::DoNotExecuteForcibly,
      request_data,
      request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 6> expected {0x03, 0x10, 0x00, 0x00, 0x01, 0x00};
  assert(request_size == expected.size());
  assert(std::equal(expected.begin(), expected.end(), request_data.begin()));
}

void test_encode_remote_latch_clear_binary_request() {
  const auto config = make_binary_c4_config();
  std::array<std::uint8_t, 16> request_data {};
  std::size_t request_size = 0;
  const Status status =
      CommandCodec::encode_remote_latch_clear(config, request_data, request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 6> expected {0x05, 0x10, 0x00, 0x00, 0x01, 0x00};
  assert(request_size == expected.size());
  assert(std::equal(expected.begin(), expected.end(), request_data.begin()));
}

void test_encode_unlock_remote_password_binary_q_l_request() {
  const auto config = make_binary_c4_config();
  std::array<std::uint8_t, 32> request_data {};
  std::size_t request_size = 0;
  const Status status =
      CommandCodec::encode_unlock_remote_password(config, "1234", request_data, request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 10> expected {
      0x30, 0x16, 0x00, 0x00, 0x04, 0x00, 0x31, 0x32, 0x33, 0x34};
  assert(request_size == expected.size());
  assert(std::equal(expected.begin(), expected.end(), request_data.begin()));
}

void test_encode_lock_remote_password_binary_q_l_request() {
  const auto config = make_binary_c4_config();
  std::array<std::uint8_t, 32> request_data {};
  std::size_t request_size = 0;
  const Status status =
      CommandCodec::encode_lock_remote_password(config, "1234", request_data, request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 10> expected {
      0x31, 0x16, 0x00, 0x00, 0x04, 0x00, 0x31, 0x32, 0x33, 0x34};
  assert(request_size == expected.size());
  assert(std::equal(expected.begin(), expected.end(), request_data.begin()));
}

void test_encode_unlock_remote_password_rejects_invalid_lengths() {
  {
    const auto config = make_binary_c4_config();
    std::array<std::uint8_t, 32> request_data {};
    std::size_t request_size = 0;
    const Status status =
        CommandCodec::encode_unlock_remote_password(config, "12345", request_data, request_size);
    assert(!status.ok());
    assert(status.code == StatusCode::InvalidArgument);
  }
  {
    const auto config = make_binary_c4_iqr_config();
    std::array<std::uint8_t, 32> request_data {};
    std::size_t request_size = 0;
    const Status status =
        CommandCodec::encode_unlock_remote_password(config, "1234", request_data, request_size);
    assert(!status.ok());
    assert(status.code == StatusCode::InvalidArgument);
  }
}

void test_encode_clear_error_information_binary_q_l_request() {
  const auto config = make_binary_c4_config();
  std::array<std::uint8_t, 16> request_data {};
  std::size_t request_size = 0;
  const Status status =
      CommandCodec::encode_clear_error_information(config, request_data, request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 8> expected {0x17, 0x16, 0x0F, 0x00, 0xFF, 0x00, 0xFF, 0x00};
  assert(request_size == expected.size());
  assert(std::equal(expected.begin(), expected.end(), request_data.begin()));
}

void test_encode_clear_error_information_binary_iqr_request() {
  const auto config = make_binary_c4_iqr_config();
  std::array<std::uint8_t, 16> request_data {};
  std::size_t request_size = 0;
  const Status status =
      CommandCodec::encode_clear_error_information(config, request_data, request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 8> expected {0x17, 0x16, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00};
  assert(request_size == expected.size());
  assert(std::equal(expected.begin(), expected.end(), request_data.begin()));
}

void test_encode_initialize_transmission_sequence_binary_request() {
  const auto config = make_binary_c4_config();
  std::array<std::uint8_t, 16> request_data {};
  std::size_t request_size = 0;
  const Status status =
      CommandCodec::encode_initialize_transmission_sequence(config, request_data, request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 4> expected {0x15, 0x16, 0x00, 0x00};
  assert(request_size == expected.size());
  assert(std::equal(expected.begin(), expected.end(), request_data.begin()));
}

void test_encode_initialize_transmission_sequence_rejects_ascii() {
  const auto config = make_ascii_c3_format3_config();
  std::array<std::uint8_t, 16> request_data {};
  std::size_t request_size = 0;
  const Status status =
      CommandCodec::encode_initialize_transmission_sequence(config, request_data, request_size);
  assert(!status.ok());
  assert(status.code == StatusCode::UnsupportedConfiguration);
}

void test_encode_control_global_signal_binary_request() {
  const auto config = make_binary_c4_config();
  std::array<std::uint8_t, 16> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_control_global_signal(
      config,
      GlobalSignalControlRequest {
          .target = GlobalSignalTarget::X1A,
          .turn_on = true,
          .station_no = 0,
      },
      request_data,
      request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 6> expected {0x18, 0x16, 0x01, 0x00, 0x01, 0x00};
  assert(request_size == expected.size());
  assert(std::equal(expected.begin(), expected.end(), request_data.begin()));
}

void test_encode_deregister_cpu_monitoring_binary_request() {
  const auto config = make_binary_c4_config();
  std::array<std::uint8_t, 16> request_data {};
  std::size_t request_size = 0;
  const Status status =
      CommandCodec::encode_deregister_cpu_monitoring(config, request_data, request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 4> expected {0x31, 0x06, 0x00, 0x00};
  assert(request_size == expected.size());
  assert(std::equal(expected.begin(), expected.end(), request_data.begin()));
}

void test_decode_ascii_loopback_response() {
  const auto config = make_ascii_c3_format3_config();
  const std::array<std::uint8_t, 9> response_data {'0', '0', '0', '5', 'A', 'B', 'C', 'D', 'E'};

  std::array<std::uint8_t, 128> frame {};
  std::size_t frame_size = 0;
  Status status = FrameCodec::encode_success_response(config, response_data, frame, frame_size);
  assert(status.ok());

  const auto decode = FrameCodec::decode_response(
      config,
      std::span<const std::uint8_t>(frame.data(), frame_size));
  assert(decode.status == DecodeStatus::Complete);

  std::array<char, 8> echoed {};
  status = CommandCodec::parse_loopback_response(
      config,
      std::span<const std::uint8_t>(decode.frame.response_data.data(), decode.frame.response_size),
      echoed);
  assert(status.ok());
  assert(std::string_view(echoed.data(), 5) == "ABCDE");
}

void test_encode_ascii_read_user_frame_request_shape() {
  const auto config = make_ascii_c3_format3_config();
  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_read_user_frame(
      config,
      UserFrameReadRequest {.frame_no = 0x03E8U},
      request_data,
      request_size);
  assert(status.ok());

  constexpr std::string_view expected = "0610000003E8";
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_parse_ascii_read_user_frame_response() {
  const auto config = make_ascii_c3_format3_config();
  const std::array<std::uint8_t, 18> response_data {
      '0', '0', '0', '5', '0', '0', '0', '4', '0', '3', 'F', 'F', 'F', '1', '0', 'D', '0', 'A',
  };

  std::array<std::uint8_t, 128> frame {};
  std::size_t frame_size = 0;
  Status status = FrameCodec::encode_success_response(config, response_data, frame, frame_size);
  assert(status.ok());

  const auto decode = FrameCodec::decode_response(
      config,
      std::span<const std::uint8_t>(frame.data(), frame_size));
  assert(decode.status == DecodeStatus::Complete);

  UserFrameRegistrationData out_data {};
  status = CommandCodec::parse_read_user_frame_response(
      config,
      std::span<const std::uint8_t>(decode.frame.response_data.data(), decode.frame.response_size),
      out_data);
  assert(status.ok());
  assert(out_data.registration_data_bytes == 5U);
  assert(out_data.frame_bytes == 4U);
  const std::array<std::byte, 5> expected {
      std::byte {0x03}, std::byte {0xFF}, std::byte {0xF1}, std::byte {0x0D}, std::byte {0x0A},
  };
  assert(std::memcmp(out_data.registration_data.data(), expected.data(), expected.size()) == 0);
}

void test_parse_binary_read_user_frame_response_accepts_zero_frame_bytes() {
  const auto config = make_binary_c4_iqr_config();
  const std::array<std::uint8_t, 9> response_data {
      0x05, 0x00, 0x00, 0x00, 0x03, 0xFF, 0xF1, 0x0D, 0x0A,
  };

  UserFrameRegistrationData out_data {};
  const Status status = CommandCodec::parse_read_user_frame_response(
      config,
      response_data,
      out_data);
  assert(status.ok());
  assert(out_data.registration_data_bytes == 5U);
  assert(out_data.frame_bytes == 0U);
  const std::array<std::byte, 5> expected {
      std::byte {0x03}, std::byte {0xFF}, std::byte {0xF1}, std::byte {0x0D}, std::byte {0x0A},
  };
  assert(std::memcmp(out_data.registration_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_binary_write_user_frame_request_shape() {
  const auto config = make_binary_c4_config();
  const std::array<std::byte, 5> registration_data {
      std::byte {0x03}, std::byte {0xFF}, std::byte {0xF1}, std::byte {0x0D}, std::byte {0x0A},
  };
  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_write_user_frame(
      config,
      UserFrameWriteRequest {
          .frame_no = 0x03E8U,
          .frame_bytes = 4U,
          .registration_data = registration_data,
      },
      request_data,
      request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 15> expected {
      0x10U, 0x16U, 0x00U, 0x00U, 0xE8U, 0x03U, 0x05U, 0x00U, 0x04U, 0x00U,
      0x03U, 0xFFU, 0xF1U, 0x0DU, 0x0AU,
  };
  assert(request_size == expected.size());
  assert(std::equal(expected.begin(), expected.end(), request_data.begin()));
}

void test_encode_binary_delete_user_frame_request_shape() {
  const auto config = make_binary_c4_config();
  std::array<std::uint8_t, 32> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_delete_user_frame(
      config,
      UserFrameDeleteRequest {.frame_no = 0x03E8U},
      request_data,
      request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 10> expected {
      0x10U, 0x16U, 0x01U, 0x00U, 0xE8U, 0x03U, 0x00U, 0x00U, 0x00U, 0x00U,
  };
  assert(request_size == expected.size());
  assert(std::equal(expected.begin(), expected.end(), request_data.begin()));
}

void test_validate_ascii_c2_config_and_reject_binary() {
  ProtocolConfig config = make_ascii_c2_format3_config();
  Status status = FrameCodec::validate_config(config);
  assert(status.ok());

  config.ascii_format = AsciiFormat::Format2;
  status = FrameCodec::validate_config(config);
  assert(status.ok());

  config.code_mode = CodeMode::Binary;
  status = FrameCodec::validate_config(config);
  assert(!status.ok());
  assert(status.code == StatusCode::UnsupportedConfiguration);
}

void test_validate_ascii_c1_config_and_reject_binary() {
  ProtocolConfig config = make_ascii_c1_format4_qna_config();
  Status status = FrameCodec::validate_config(config);
  assert(status.ok());

  config.ascii_format = AsciiFormat::Format2;
  status = FrameCodec::validate_config(config);
  assert(!status.ok());
  assert(status.code == StatusCode::UnsupportedConfiguration);

  config.code_mode = CodeMode::Binary;
  status = FrameCodec::validate_config(config);
  assert(!status.ok());
  assert(status.code == StatusCode::UnsupportedConfiguration);
}

void test_encode_ascii_c2_format3_request_uses_short_route_without_frame_id() {
  auto config = make_ascii_c2_format3_config();
  config.route.kind = RouteKind::MultidropStation;
  config.route.station_no = 0x11;
  config.route.self_station_enabled = true;
  config.route.self_station_no = 0x05;

  const BatchReadWordsRequest request {
      .head_device = {.code = mcprotocol::serial::DeviceCode::D, .number = 100},
      .points = 1,
  };

  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_data_size = 0;
  Status status = CommandCodec::encode_batch_read_words(config, request, request_data, request_data_size);
  assert(status.ok());

  std::array<std::uint8_t, 128> frame {};
  std::size_t frame_size = 0;
  status = FrameCodec::encode_request(
      config,
      std::span<const std::uint8_t>(request_data.data(), request_data_size),
      frame,
      frame_size);
  assert(status.ok());

  constexpr std::string_view expected_body = "110504010000D*0001000001";
  assert(frame_size == (1U + expected_body.size() + 1U + 2U));
  assert(frame[0] == 0x02);
  assert(std::memcmp(frame.data() + 1, expected_body.data(), expected_body.size()) == 0);
  assert(frame[1U + expected_body.size()] == 0x03);
}

void test_decode_ascii_c2_format3_data_response() {
  auto config = make_ascii_c2_format3_config();
  config.route.kind = RouteKind::MultidropStation;
  config.route.station_no = 0x11;
  config.route.self_station_enabled = true;
  config.route.self_station_no = 0x05;
  const std::array<std::uint8_t, 4> response_data {'1', '2', '3', '4'};

  std::array<std::uint8_t, 64> frame {};
  std::size_t frame_size = 0;
  Status status = FrameCodec::encode_success_response(config, response_data, frame, frame_size);
  assert(status.ok());

  constexpr std::string_view expected_prefix = "1105QACK";
  assert(frame[0] == 0x02);
  assert(std::memcmp(frame.data() + 1, expected_prefix.data(), expected_prefix.size()) == 0);

  const auto decode = FrameCodec::decode_response(
      config,
      std::span<const std::uint8_t>(frame.data(), frame_size));
  assert(decode.status == DecodeStatus::Complete);
  assert(decode.frame.type == mcprotocol::serial::ResponseType::SuccessData);
  assert(decode.frame.response_size == response_data.size());
  assert(std::memcmp(decode.frame.response_data.data(), response_data.data(), response_data.size()) == 0);
}

void test_decode_ascii_c2_format3_two_digit_error_response() {
  auto config = make_ascii_c2_format3_config();
  config.route.kind = RouteKind::MultidropStation;
  config.route.station_no = 0x11;
  config.route.self_station_enabled = true;
  config.route.self_station_no = 0x05;
  config.sum_check_enabled = false;

  constexpr std::string_view frame = "\x02""1105NN06\x03";
  const auto decode = FrameCodec::decode_response(
      config,
      std::span<const std::uint8_t>(
          reinterpret_cast<const std::uint8_t*>(frame.data()),
          frame.size()));
  assert(decode.status == DecodeStatus::Complete);
  assert(decode.frame.type == mcprotocol::serial::ResponseType::PlcError);
  assert(decode.frame.error_code == 0x0006U);
  assert(decode.bytes_consumed == frame.size());
}

void test_encode_ascii_format2_request_inserts_block_number() {
  auto config = make_ascii_c4_format2_config();
  config.sum_check_enabled = false;
  config.ascii_block_number = 0x7AU;

  const BatchReadWordsRequest request {
      .head_device = {.code = mcprotocol::serial::DeviceCode::D, .number = 100},
      .points = 1,
  };

  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_data_size = 0;
  Status status = CommandCodec::encode_batch_read_words(config, request, request_data, request_data_size);
  assert(status.ok());

  std::array<std::uint8_t, 128> frame {};
  std::size_t frame_size = 0;
  status = FrameCodec::encode_request(
      config,
      std::span<const std::uint8_t>(request_data.data(), request_data_size),
      frame,
      frame_size);
  assert(status.ok());

  constexpr std::string_view expected = "\x05""7AF80000FF03FF000004010000D*0001000001";
  assert(frame_size == expected.size());
  assert(std::memcmp(frame.data(), expected.data(), expected.size()) == 0);
}

void test_encode_ascii_c2_format2_request_uses_short_route_without_frame_id() {
  auto config = make_ascii_c2_format2_config();
  config.sum_check_enabled = false;
  config.ascii_block_number = 0x7AU;
  config.route.kind = RouteKind::MultidropStation;
  config.route.station_no = 0x11;
  config.route.self_station_enabled = true;
  config.route.self_station_no = 0x05;

  const BatchReadWordsRequest request {
      .head_device = {.code = mcprotocol::serial::DeviceCode::D, .number = 100},
      .points = 1,
  };

  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_data_size = 0;
  Status status = CommandCodec::encode_batch_read_words(config, request, request_data, request_data_size);
  assert(status.ok());

  std::array<std::uint8_t, 128> frame {};
  std::size_t frame_size = 0;
  status = FrameCodec::encode_request(
      config,
      std::span<const std::uint8_t>(request_data.data(), request_data_size),
      frame,
      frame_size);
  assert(status.ok());

  constexpr std::string_view expected = "\x05""7A110504010000D*0001000001";
  assert(frame_size == expected.size());
  assert(std::memcmp(frame.data(), expected.data(), expected.size()) == 0);
}

void test_decode_ascii_format2_ack_response() {
  auto config = make_ascii_c4_format2_config();
  config.sum_check_enabled = false;
  config.ascii_block_number = 0x7AU;
  std::array<std::uint8_t, 32> frame {};
  std::size_t frame_size = 0;
  Status status = FrameCodec::encode_success_response(config, {}, frame, frame_size);
  assert(status.ok());

  constexpr std::string_view expected = "\x06""7AF80000FF03FF0000";
  assert(frame_size == expected.size());
  assert(std::memcmp(frame.data(), expected.data(), expected.size()) == 0);

  const auto decode = FrameCodec::decode_response(
      config,
      std::span<const std::uint8_t>(frame.data(), frame_size));
  assert(decode.status == DecodeStatus::Complete);
  assert(decode.frame.type == mcprotocol::serial::ResponseType::SuccessNoData);
  assert(decode.bytes_consumed == frame_size);
}

void test_decode_ascii_c2_format2_two_digit_error_response() {
  auto config = make_ascii_c2_format2_config();
  config.sum_check_enabled = false;
  config.ascii_block_number = 0x7AU;

  constexpr std::string_view frame = "\x15""7A000006";
  const auto decode = FrameCodec::decode_response(
      config,
      std::span<const std::uint8_t>(
          reinterpret_cast<const std::uint8_t*>(frame.data()),
          frame.size()));
  assert(decode.status == DecodeStatus::Complete);
  assert(decode.frame.type == mcprotocol::serial::ResponseType::PlcError);
  assert(decode.frame.error_code == 0x0006U);
  assert(decode.bytes_consumed == frame.size());
}

void test_encode_ascii_c1_batch_read_words_qna_request_shape() {
  const auto config = make_ascii_c1_format4_qna_config();
  const BatchReadWordsRequest request {
      .head_device = {.code = mcprotocol::serial::DeviceCode::D, .number = 100},
      .points = 1,
  };

  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_data_size = 0;
  Status status = CommandCodec::encode_batch_read_words(config, request, request_data, request_data_size);
  assert(status.ok());
  constexpr std::string_view expected_request_data = "QR0D00010001";
  assert(request_data_size == expected_request_data.size());
  assert(std::memcmp(request_data.data(), expected_request_data.data(), expected_request_data.size()) == 0);

  std::array<std::uint8_t, 128> frame {};
  std::size_t frame_size = 0;
  status = FrameCodec::encode_request(
      config,
      std::span<const std::uint8_t>(request_data.data(), request_data_size),
      frame,
      frame_size);
  assert(status.ok());
  constexpr std::string_view expected_frame = "\x05""00QR0D00010001FF\r\n";
  assert(frame_size == expected_frame.size());
  assert(std::memcmp(frame.data(), expected_frame.data(), frame_size) == 0);
}

void test_encode_ascii_c1_batch_read_bits_a_request_shape() {
  const auto config = make_ascii_c1_format4_a_config();
  const BatchReadBitsRequest request {
      .head_device = {.code = mcprotocol::serial::DeviceCode::X, .number = 0x40},
      .points = 5,
  };

  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_data_size = 0;
  Status status = CommandCodec::encode_batch_read_bits(config, request, request_data, request_data_size);
  assert(status.ok());
  constexpr std::string_view expected_request_data = "BR0X004005";
  assert(request_data_size == expected_request_data.size());
  assert(std::memcmp(request_data.data(), expected_request_data.data(), expected_request_data.size()) == 0);
}

void test_encode_ascii_c1_batch_write_words_qna_request_shape() {
  const auto config = make_ascii_c1_format4_qna_config();
  const std::array<std::uint16_t, 2> values {0x1234U, 0x5678U};
  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_batch_write_words(
      config,
      BatchWriteWordsRequest {
          .head_device = {.code = mcprotocol::serial::DeviceCode::D, .number = 100},
          .words = values,
      },
      request_data,
      request_size);
  assert(status.ok());

  constexpr std::string_view expected = "QW0D0001000212345678";
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_decode_ascii_c1_ack_response() {
  const auto config = make_ascii_c1_format4_qna_config();
  std::array<std::uint8_t, 32> frame {};
  std::size_t frame_size = 0;
  Status status = FrameCodec::encode_success_response(config, {}, frame, frame_size);
  assert(status.ok());

  constexpr std::string_view expected = "\x06""00FF\r\n";
  assert(frame_size == expected.size());
  assert(std::memcmp(frame.data(), expected.data(), frame_size) == 0);

  const auto decode = FrameCodec::decode_response(
      config,
      std::span<const std::uint8_t>(frame.data(), frame_size));
  assert(decode.status == DecodeStatus::Complete);
  assert(decode.frame.type == mcprotocol::serial::ResponseType::SuccessNoData);
  assert(decode.bytes_consumed == frame_size);
}

void test_encode_ascii_c1_format3_uses_gg_end_code() {
  auto config = make_ascii_c1_format4_qna_config();
  config.ascii_format = AsciiFormat::Format3;
  const std::array<std::uint8_t, 4> response_data {'1', '2', '3', '4'};
  std::array<std::uint8_t, 32> frame {};
  std::size_t frame_size = 0;
  Status status = FrameCodec::encode_success_response(config, response_data, frame, frame_size);
  assert(status.ok());

  constexpr std::string_view expected = "\x02""00FFGG1234\x03";
  assert(frame_size == expected.size());
  assert(std::memcmp(frame.data(), expected.data(), frame_size) == 0);

  const auto decode = FrameCodec::decode_response(
      config,
      std::span<const std::uint8_t>(frame.data(), frame_size));
  assert(decode.status == DecodeStatus::Complete);
  assert(decode.frame.type == mcprotocol::serial::ResponseType::SuccessData);
  assert(decode.frame.response_size == response_data.size());
}

void test_decode_ascii_c1_error_uses_two_digit_code() {
  const auto config = make_ascii_c1_format4_qna_config();
  std::array<std::uint8_t, 32> frame {};
  std::size_t frame_size = 0;
  Status status = FrameCodec::encode_error_response(config, 0x05U, frame, frame_size);
  assert(status.ok());

  constexpr std::string_view expected = "\x15""00FF05\r\n";
  assert(frame_size == expected.size());
  assert(std::memcmp(frame.data(), expected.data(), frame_size) == 0);

  const auto decode = FrameCodec::decode_response(
      config,
      std::span<const std::uint8_t>(frame.data(), frame_size));
  assert(decode.status == DecodeStatus::Complete);
  assert(decode.frame.type == mcprotocol::serial::ResponseType::PlcError);
  assert(decode.frame.error_code == 0x05U);
}

void test_encode_ascii_c1_rejects_unsupported_series() {
  ProtocolConfig config = make_ascii_c1_format4_qna_config();
  config.target_series = PlcSeries::Q_L;
  const BatchReadWordsRequest request {
      .head_device = {.code = mcprotocol::serial::DeviceCode::D, .number = 100},
      .points = 1,
  };
  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_batch_read_words(config, request, request_data, request_size);
  assert(!status.ok());
  assert(status.code == StatusCode::UnsupportedConfiguration);
}

void test_encode_ascii_c1_random_write_bits_qna_request_shape() {
  const auto config = make_ascii_c1_format4_qna_config();
  const std::array<RandomWriteBitItem, 3> items {{
      {.device = {.code = mcprotocol::serial::DeviceCode::M, .number = 50}, .value = BitValue::On},
      {.device = {.code = mcprotocol::serial::DeviceCode::B, .number = 0x31AU}, .value = BitValue::Off},
      {.device = {.code = mcprotocol::serial::DeviceCode::Y, .number = 0x2FU}, .value = BitValue::On},
  }};
  std::array<std::uint8_t, 96> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_random_write_bits(
      config,
      std::span<const RandomWriteBitItem>(items.data(), items.size()),
      request_data,
      request_size);
  assert(status.ok());

  constexpr std::string_view expected = "JT003M0000501B00031A0Y00002F1";
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_ascii_c1_random_write_words_qna_request_shape() {
  const auto config = make_ascii_c1_format4_qna_config();
  const std::array<RandomWriteWordItem, 3> items {{
      {.device = {.code = mcprotocol::serial::DeviceCode::D, .number = 500}, .value = 0x1234U},
      {.device = {.code = mcprotocol::serial::DeviceCode::Y, .number = 0x100U}, .value = 0xBCA9U},
      {.device = {.code = mcprotocol::serial::DeviceCode::CN, .number = 100}, .value = 0x0064U},
  }};
  std::array<std::uint8_t, 128> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_random_write_words(
      config,
      std::span<const RandomWriteWordItem>(items.data(), items.size()),
      request_data,
      request_size);
  assert(status.ok());

  constexpr std::string_view expected = "QT003D0005001234Y000100BCA9CN001000064";
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_ascii_c1_register_monitor_bits_and_read_request_shape() {
  const auto config = make_ascii_c1_format4_qna_config();
  const std::array<RandomReadItem, 3> items {{
      {.device = {.code = mcprotocol::serial::DeviceCode::X, .number = 0x40U}},
      {.device = {.code = mcprotocol::serial::DeviceCode::Y, .number = 0x60U}},
      {.device = {.code = mcprotocol::serial::DeviceCode::TS, .number = 123U}},
  }};
  std::array<std::uint8_t, 96> request_data {};
  std::size_t request_size = 0;
  Status status = CommandCodec::encode_register_monitor(
      config,
      MonitorRegistration {.items = std::span<const RandomReadItem>(items.data(), items.size())},
      request_data,
      request_size);
  assert(status.ok());

  constexpr std::string_view expected_register = "JM003X000040Y000060TS00123";
  assert(request_size == expected_register.size());
  assert(std::memcmp(request_data.data(), expected_register.data(), expected_register.size()) == 0);

  request_size = 0;
  status = CommandCodec::encode_read_monitor(
      config,
      std::span<const RandomReadItem>(items.data(), items.size()),
      request_data,
      request_size);
  assert(status.ok());
  constexpr std::string_view expected_read = "MJ0";
  assert(request_size == expected_read.size());
  assert(std::memcmp(request_data.data(), expected_read.data(), expected_read.size()) == 0);

  std::array<std::uint32_t, 3> values {};
  status = CommandCodec::parse_read_monitor_response(
      config,
      std::span<const RandomReadItem>(items.data(), items.size()),
      std::span<const std::uint8_t>(
          reinterpret_cast<const std::uint8_t*>("101"),
          3U),
      values);
  assert(status.ok());
  assert(values[0] == 1U);
  assert(values[1] == 0U);
  assert(values[2] == 1U);
}

void test_encode_ascii_c1_register_monitor_words_and_read_request_shape() {
  const auto config = make_ascii_c1_format4_qna_config();
  const std::array<RandomReadItem, 4> items {{
      {.device = {.code = mcprotocol::serial::DeviceCode::D, .number = 15U}},
      {.device = {.code = mcprotocol::serial::DeviceCode::W, .number = 0x11EU}},
      {.device = {.code = mcprotocol::serial::DeviceCode::TN, .number = 123U}},
      {.device = {.code = mcprotocol::serial::DeviceCode::Y, .number = 0x60U}},
  }};
  std::array<std::uint8_t, 128> request_data {};
  std::size_t request_size = 0;
  Status status = CommandCodec::encode_register_monitor(
      config,
      MonitorRegistration {.items = std::span<const RandomReadItem>(items.data(), items.size())},
      request_data,
      request_size);
  assert(status.ok());

  constexpr std::string_view expected_register = "QM004D000015W00011ETN00123Y000060";
  assert(request_size == expected_register.size());
  assert(std::memcmp(request_data.data(), expected_register.data(), expected_register.size()) == 0);

  request_size = 0;
  status = CommandCodec::encode_read_monitor(
      config,
      std::span<const RandomReadItem>(items.data(), items.size()),
      request_data,
      request_size);
  assert(status.ok());
  constexpr std::string_view expected_read = "MQ0";
  assert(request_size == expected_read.size());
  assert(std::memcmp(request_data.data(), expected_read.data(), expected_read.size()) == 0);
}

void test_client_ascii_c1_register_monitor_roundtrip() {
  struct LocalCapture {
    bool called = false;
    Status status {};
  };
  const auto local_callback = +[](void* user, Status status) {
    auto* capture = static_cast<LocalCapture*>(user);
    capture->called = true;
    capture->status = status;
  };

  const auto config = make_ascii_c1_format4_qna_config();
  MelsecSerialClient client;
  Status status = client.configure(config);
  assert(status.ok());

  const std::array<RandomReadItem, 3> items {{
      {.device = {.code = mcprotocol::serial::DeviceCode::X, .number = 0x40U}},
      {.device = {.code = mcprotocol::serial::DeviceCode::Y, .number = 0x60U}},
      {.device = {.code = mcprotocol::serial::DeviceCode::TS, .number = 123U}},
  }};
  LocalCapture register_capture;
  status = client.async_register_monitor(
      0,
      MonitorRegistration {.items = std::span<const RandomReadItem>(items.data(), items.size())},
      local_callback,
      &register_capture);
  assert(status.ok());
  status = client.notify_tx_complete(1);
  assert(status.ok());

  std::array<std::uint8_t, 32> register_frame {};
  std::size_t register_frame_size = 0;
  status = FrameCodec::encode_success_response(config, {}, register_frame, register_frame_size);
  assert(status.ok());
  client.on_rx_bytes(
      2,
      std::span<const std::byte>(
          reinterpret_cast<const std::byte*>(register_frame.data()),
          register_frame_size));
  assert(register_capture.called);
  assert(register_capture.status.ok());

  std::array<std::uint32_t, 3> values {};
  LocalCapture read_capture;
  status = client.async_read_monitor(10, values, local_callback, &read_capture);
  assert(status.ok());
  status = client.notify_tx_complete(11);
  assert(status.ok());

  const std::array<std::uint8_t, 3> response_data {'1', '0', '1'};
  std::array<std::uint8_t, 64> response_frame {};
  std::size_t response_frame_size = 0;
  status = FrameCodec::encode_success_response(config, response_data, response_frame, response_frame_size);
  assert(status.ok());
  client.on_rx_bytes(
      12,
      std::span<const std::byte>(
          reinterpret_cast<const std::byte*>(response_frame.data()),
          response_frame_size));
  assert(read_capture.called);
  assert(read_capture.status.ok());
  assert(values[0] == 1U);
  assert(values[1] == 0U);
  assert(values[2] == 1U);
}

void test_encode_ascii_c1_read_module_buffer_request_shape() {
  const auto config = make_ascii_c1_format4_qna_config();
  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_read_module_buffer(
      config,
      ModuleBufferReadRequest {
          .start_address = 0x07F0U,
          .bytes = 0x04U,
          .module_number = 0x13U,
      },
      request_data,
      request_size);
  assert(status.ok());

  constexpr std::string_view expected = "TR0007F00413";
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_ascii_c1_write_module_buffer_request_shape() {
  const auto config = make_ascii_c1_format4_qna_config();
  const std::array<std::byte, 4> bytes {
      std::byte {0xCD},
      std::byte {0x01},
      std::byte {0xEF},
      std::byte {0xAB},
  };
  std::array<std::uint8_t, 96> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_write_module_buffer(
      config,
      ModuleBufferWriteRequest {
          .start_address = 0x27FAU,
          .module_number = 0x13U,
          .bytes = std::span<const std::byte>(bytes.data(), bytes.size()),
      },
      request_data,
      request_size);
  assert(status.ok());

  constexpr std::string_view expected = "TW0027FA0413CD01EFAB";
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_ascii_c1_loopback_request_shape() {
  const auto config = make_ascii_c1_format4_qna_config();
  constexpr std::string_view loopback = "ABCDE";
  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;
  Status status = CommandCodec::encode_loopback(config, std::span<const char>(loopback.data(), loopback.size()), request_data, request_size);
  assert(status.ok());

  constexpr std::string_view expected_request_data = "TT005ABCDE";
  assert(request_size == expected_request_data.size());
  assert(std::memcmp(request_data.data(), expected_request_data.data(), expected_request_data.size()) == 0);

  std::array<std::uint8_t, 128> frame {};
  std::size_t frame_size = 0;
  status = FrameCodec::encode_request(
      config,
      std::span<const std::uint8_t>(request_data.data(), request_size),
      frame,
      frame_size);
  assert(status.ok());

  constexpr std::string_view expected_frame = "\x05""00TT005ABCDEFF\r\n";
  assert(frame_size == expected_frame.size());
  assert(std::memcmp(frame.data(), expected_frame.data(), frame_size) == 0);
}

void test_decode_ascii_c1_loopback_response() {
  const auto config = make_ascii_c1_format4_qna_config();
  const std::array<std::uint8_t, 7> response_data {'0', '5', 'A', 'B', 'C', 'D', 'E'};

  std::array<std::uint8_t, 128> frame {};
  std::size_t frame_size = 0;
  Status status = FrameCodec::encode_success_response(config, response_data, frame, frame_size);
  assert(status.ok());

  const auto decode = FrameCodec::decode_response(
      config,
      std::span<const std::uint8_t>(frame.data(), frame_size));
  assert(decode.status == DecodeStatus::Complete);

  std::array<char, 8> echoed {};
  status = CommandCodec::parse_loopback_response(
      config,
      std::span<const std::uint8_t>(decode.frame.response_data.data(), decode.frame.response_size),
      echoed);
  assert(status.ok());
  assert(std::string_view(echoed.data(), 5) == "ABCDE");
}

void test_encode_ascii_c1_loopback_rejects_non_ff_pc_no() {
  auto config = make_ascii_c1_format4_qna_config();
  config.route.pc_no = 0x01U;
  constexpr std::string_view loopback = "ABCDE";
  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_loopback(
      config,
      std::span<const char>(loopback.data(), loopback.size()),
      request_data,
      request_size);
  assert(!status.ok());
  assert(status.code == StatusCode::InvalidArgument);
}

void test_encode_ascii_c1_extended_file_register_read_a_request_shape() {
  const auto config = make_ascii_c1_format4_a_config();
  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_read_extended_file_register_words(
      config,
      ExtendedFileRegisterBatchReadWordsRequest {
          .head_device = {.block_number = 12U, .word_number = 8190U},
          .points = 2U,
      },
      request_data,
      request_size);
  assert(status.ok());

  constexpr std::string_view expected = "ER012R819002";
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_ascii_c1_direct_extended_file_register_read_qna_request_shape() {
  const auto config = make_ascii_c1_format4_qna_config();
  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_direct_read_extended_file_register_words(
      config,
      ExtendedFileRegisterDirectBatchReadWordsRequest {
          .head_device_number = 16382U,
          .points = 2U,
      },
      request_data,
      request_size);
  assert(status.ok());

  constexpr std::string_view expected = "NR0001638202";
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_ascii_c1_extended_file_register_write_a_request_shape() {
  const auto config = make_ascii_c1_format4_a_config();
  const std::array<std::uint16_t, 3> values {0x0123U, 0xABC7U, 0x3322U};
  std::array<std::uint8_t, 96> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_write_extended_file_register_words(
      config,
      ExtendedFileRegisterBatchWriteWordsRequest {
          .head_device = {.block_number = 5U, .word_number = 7010U},
          .words = values,
      },
      request_data,
      request_size);
  assert(status.ok());

  constexpr std::string_view expected = "EW005R7010030123ABC73322";
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_ascii_c1_direct_extended_file_register_write_qna_request_shape() {
  const auto config = make_ascii_c1_format4_qna_config();
  const std::array<std::uint16_t, 3> values {0x0123U, 0xABC7U, 0x3322U};
  std::array<std::uint8_t, 96> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_direct_write_extended_file_register_words(
      config,
      ExtendedFileRegisterDirectBatchWriteWordsRequest {
          .head_device_number = 90110U,
          .words = values,
      },
      request_data,
      request_size);
  assert(status.ok());

  constexpr std::string_view expected = "NW00090110030123ABC73322";
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_ascii_c1_extended_file_register_random_write_a_request_shape() {
  const auto config = make_ascii_c1_format4_a_config();
  const std::array<ExtendedFileRegisterRandomWriteWordItem, 3> items {{
      {.device = {.block_number = 5U, .word_number = 1050U}, .value = 0x1234U},
      {.device = {.block_number = 7U, .word_number = 2121U}, .value = 0x1A1BU},
      {.device = {.block_number = 10U, .word_number = 3210U}, .value = 0x0506U},
  }};
  std::array<std::uint8_t, 128> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_random_write_extended_file_register_words(
      config,
      std::span<const ExtendedFileRegisterRandomWriteWordItem>(items.data(), items.size()),
      request_data,
      request_size);
  assert(status.ok());

  constexpr std::string_view expected = "ET00305R1050123407R21211A1B10R32100506";
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_ascii_c1_extended_file_register_monitor_a_request_shape() {
  const auto config = make_ascii_c1_format4_a_config();
  const std::array<ExtendedFileRegisterAddress, 4> items {{
      {.block_number = 5U, .word_number = 1234U},
      {.block_number = 6U, .word_number = 2345U},
      {.block_number = 15U, .word_number = 3055U},
      {.block_number = 17U, .word_number = 8000U},
  }};
  std::array<std::uint8_t, 128> request_data {};
  std::size_t request_size = 0;
  Status status = CommandCodec::encode_register_extended_file_register_monitor(
      config,
      ExtendedFileRegisterMonitorRegistration {
          .items = std::span<const ExtendedFileRegisterAddress>(items.data(), items.size()),
      },
      request_data,
      request_size);
  assert(status.ok());

  constexpr std::string_view expected_register = "EM00405R123406R234515R305517R8000";
  assert(request_size == expected_register.size());
  assert(std::memcmp(request_data.data(), expected_register.data(), expected_register.size()) == 0);

  request_size = 0;
  status = CommandCodec::encode_read_extended_file_register_monitor(
      config,
      std::span<const ExtendedFileRegisterAddress>(items.data(), items.size()),
      request_data,
      request_size);
  assert(status.ok());
  constexpr std::string_view expected_read = "ME0";
  assert(request_size == expected_read.size());
  assert(std::memcmp(request_data.data(), expected_read.data(), expected_read.size()) == 0);

  std::array<std::uint16_t, 4> values {};
  constexpr std::array<std::uint8_t, 16> response_data {
      '3','5','0','1','4','F','5','B','0','1','6','E','0','1','6','E'
  };
  status = CommandCodec::parse_read_extended_file_register_monitor_response(
      config,
      std::span<const ExtendedFileRegisterAddress>(items.data(), items.size()),
      std::span<const std::uint8_t>(response_data.data(), response_data.size()),
      values);
  assert(status.ok());
  assert(values[0] == 0x3501U);
  assert(values[1] == 0x4F5BU);
  assert(values[2] == 0x016EU);
  assert(values[3] == 0x016EU);
}

void test_client_ascii_c1_extended_file_register_monitor_roundtrip() {
  struct LocalCapture {
    bool called = false;
    Status status {};
  };
  const auto local_callback = +[](void* user, Status status) {
    auto* capture = static_cast<LocalCapture*>(user);
    capture->called = true;
    capture->status = status;
  };

  const auto config = make_ascii_c1_format4_a_config();
  MelsecSerialClient client;
  Status status = client.configure(config);
  assert(status.ok());

  const std::array<ExtendedFileRegisterAddress, 2> items {{
      {.block_number = 5U, .word_number = 1234U},
      {.block_number = 6U, .word_number = 2345U},
  }};
  LocalCapture register_capture;
  status = client.async_register_extended_file_register_monitor(
      0,
      ExtendedFileRegisterMonitorRegistration {
          .items = std::span<const ExtendedFileRegisterAddress>(items.data(), items.size()),
      },
      local_callback,
      &register_capture);
  assert(status.ok());
  status = client.notify_tx_complete(1);
  assert(status.ok());

  std::array<std::uint8_t, 32> register_frame {};
  std::size_t register_frame_size = 0;
  status = FrameCodec::encode_success_response(config, {}, register_frame, register_frame_size);
  assert(status.ok());
  client.on_rx_bytes(
      2,
      std::span<const std::byte>(
          reinterpret_cast<const std::byte*>(register_frame.data()),
          register_frame_size));
  assert(register_capture.called);
  assert(register_capture.status.ok());

  std::array<std::uint16_t, 2> values {};
  LocalCapture read_capture;
  status = client.async_read_extended_file_register_monitor(10, values, local_callback, &read_capture);
  assert(status.ok());
  status = client.notify_tx_complete(11);
  assert(status.ok());

  const std::array<std::uint8_t, 8> response_data {'1','2','3','4','A','B','C','D'};
  std::array<std::uint8_t, 64> response_frame {};
  std::size_t response_frame_size = 0;
  status = FrameCodec::encode_success_response(config, response_data, response_frame, response_frame_size);
  assert(status.ok());
  client.on_rx_bytes(
      12,
      std::span<const std::byte>(
          reinterpret_cast<const std::byte*>(response_frame.data()),
          response_frame_size));
  assert(read_capture.called);
  assert(read_capture.status.ok());
  assert(values[0] == 0x1234U);
  assert(values[1] == 0xABCDU);
}

void test_validate_e1_config_and_route_constraints() {
  auto ascii_config = make_ascii_e1_a_config();
  Status status = FrameCodec::validate_config(ascii_config);
  assert(status.ok());

  auto binary_config = make_binary_e1_a_config();
  status = FrameCodec::validate_config(binary_config);
  assert(status.ok());

  ascii_config.route.station_no = 0x01U;
  status = FrameCodec::validate_config(ascii_config);
  assert(!status.ok());
  assert(status.code == StatusCode::InvalidArgument);

  ascii_config = make_ascii_e1_a_config();
  ascii_config.route.pc_no = 0x00U;
  status = FrameCodec::validate_config(ascii_config);
  assert(!status.ok());
  assert(status.code == StatusCode::InvalidArgument);
}

void test_encode_ascii_e1_batch_read_words_request_shape() {
  const auto config = make_ascii_e1_a_config();
  const BatchReadWordsRequest request {
      .head_device = {.code = mcprotocol::serial::DeviceCode::Y, .number = 0x40U},
      .points = 2,
  };

  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;
  Status status = CommandCodec::encode_batch_read_words(config, request, request_data, request_size);
  assert(status.ok());

  std::array<std::uint8_t, 64> frame {};
  std::size_t frame_size = 0;
  status = FrameCodec::encode_request(
      config,
      std::span<const std::uint8_t>(request_data.data(), request_size),
      frame,
      frame_size);
  assert(status.ok());

  constexpr std::string_view expected = "01FF00105920000000400200";
  assert(frame_size == expected.size());
  assert(std::memcmp(frame.data(), expected.data(), expected.size()) == 0);
}

void test_encode_binary_e1_batch_read_bits_request_shape() {
  const auto config = make_binary_e1_a_config();
  const BatchReadBitsRequest request {
      .head_device = {.code = mcprotocol::serial::DeviceCode::M, .number = 100U},
      .points = 12,
  };

  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;
  Status status = CommandCodec::encode_batch_read_bits(config, request, request_data, request_size);
  assert(status.ok());

  std::array<std::uint8_t, 64> frame {};
  std::size_t frame_size = 0;
  status = FrameCodec::encode_request(
      config,
      std::span<const std::uint8_t>(request_data.data(), request_size),
      frame,
      frame_size);
  assert(status.ok());

  const std::array<std::uint8_t, 12> expected {
      0x00U, 0xFFU, 0x10U, 0x00U, 0x64U, 0x00U, 0x00U, 0x00U, 0x20U, 0x4DU, 0x0CU, 0x00U,
  };
  assert(frame_size == expected.size());
  assert(std::equal(expected.begin(), expected.end(), frame.begin()));
}

void test_decode_ascii_e1_success_response() {
  const auto config = make_ascii_e1_a_config();
  constexpr std::string_view response = "810012348765";
  const auto decode = FrameCodec::decode_response(
      config,
      std::span<const std::uint8_t>(
          reinterpret_cast<const std::uint8_t*>(response.data()),
          response.size()));
  assert(decode.status == DecodeStatus::Complete);
  assert(decode.frame.type == mcprotocol::serial::ResponseType::SuccessData);
  assert(decode.frame.response_size == 8U);
  assert(std::memcmp(decode.frame.response_data.data(), response.data() + 4U, 8U) == 0);
}

void test_decode_binary_e1_error_response_with_abnormal_code() {
  const auto config = make_binary_e1_a_config();
  const std::array<std::uint8_t, 3> response {0x81U, 0x5BU, 0x10U};
  const auto decode = FrameCodec::decode_response(
      config,
      std::span<const std::uint8_t>(response.data(), response.size()));
  assert(decode.status == DecodeStatus::Complete);
  assert(decode.frame.type == mcprotocol::serial::ResponseType::PlcError);
  assert(decode.frame.error_code == 0x5B10U);
}

void test_encode_binary_e1_random_write_words_request_shape() {
  const auto config = make_binary_e1_a_config();
  const std::array<RandomWriteWordItem, 3> items {{
      {.device = {.code = mcprotocol::serial::DeviceCode::Y, .number = 0x80U}, .value = 0x7B29U},
      {.device = {.code = mcprotocol::serial::DeviceCode::W, .number = 0x26U}, .value = 0x1234U},
      {.device = {.code = mcprotocol::serial::DeviceCode::CN, .number = 0x12U}, .value = 0x0050U},
  }};

  std::array<std::uint8_t, 128> request_data {};
  std::size_t request_size = 0;
  Status status = CommandCodec::encode_random_write_words(
      config,
      std::span<const RandomWriteWordItem>(items.data(), items.size()),
      request_data,
      request_size);
  assert(status.ok());

  std::array<std::uint8_t, 128> frame {};
  std::size_t frame_size = 0;
  status = FrameCodec::encode_request(
      config,
      std::span<const std::uint8_t>(request_data.data(), request_size),
      frame,
      frame_size);
  assert(status.ok());

  const std::array<std::uint8_t, 30> expected {
      0x05U, 0xFFU, 0x10U, 0x00U, 0x03U, 0x00U,
      0x80U, 0x00U, 0x00U, 0x00U, 0x20U, 0x59U, 0x29U, 0x7BU,
      0x26U, 0x00U, 0x00U, 0x00U, 0x20U, 0x57U, 0x34U, 0x12U,
      0x12U, 0x00U, 0x00U, 0x00U, 0x4EU, 0x43U, 0x50U, 0x00U,
  };
  assert(frame_size == 30U);
  assert(std::equal(expected.begin(), expected.end(), frame.begin()));
}

void test_encode_ascii_e1_extended_file_register_read_request_shape() {
  const auto config = make_ascii_e1_a_config();
  const ExtendedFileRegisterBatchReadWordsRequest request {
      .head_device = {.block_number = 2U, .word_number = 70U},
      .points = 3,
  };

  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;
  Status status = CommandCodec::encode_read_extended_file_register_words(
      config,
      request,
      request_data,
      request_size);
  assert(status.ok());

  std::array<std::uint8_t, 64> frame {};
  std::size_t frame_size = 0;
  status = FrameCodec::encode_request(
      config,
      std::span<const std::uint8_t>(request_data.data(), request_size),
      frame,
      frame_size);
  assert(status.ok());

  constexpr std::string_view expected = "17FF001052200000004600020300";
  assert(frame_size == expected.size());
  assert(std::memcmp(frame.data(), expected.data(), expected.size()) == 0);
}

void test_encode_ascii_e1_direct_extended_file_register_read_request_shape() {
  const auto config = make_ascii_e1_a_config();
  const ExtendedFileRegisterDirectBatchReadWordsRequest request {
      .head_device_number = 0x01234567U,
      .points = 3U,
  };

  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;
  Status status = CommandCodec::encode_direct_read_extended_file_register_words(
      config,
      request,
      request_data,
      request_size);
  assert(status.ok());

  std::array<std::uint8_t, 64> frame {};
  std::size_t frame_size = 0;
  status = FrameCodec::encode_request(
      config,
      std::span<const std::uint8_t>(request_data.data(), request_size),
      frame,
      frame_size);
  assert(status.ok());

  constexpr std::string_view expected = "3BFF00105220012345670300";
  assert(frame_size == expected.size());
  assert(std::memcmp(frame.data(), expected.data(), expected.size()) == 0);
}

void test_encode_binary_e1_extended_file_register_monitor_registration_request_shape() {
  const auto config = make_binary_e1_a_config();
  const std::array<ExtendedFileRegisterAddress, 2> items {{
      {.block_number = 2U, .word_number = 70U},
      {.block_number = 3U, .word_number = 71U},
  }};

  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;
  Status status = CommandCodec::encode_register_extended_file_register_monitor(
      config,
      ExtendedFileRegisterMonitorRegistration {.items = items},
      request_data,
      request_size);
  assert(status.ok());

  std::array<std::uint8_t, 64> frame {};
  std::size_t frame_size = 0;
  status = FrameCodec::encode_request(
      config,
      std::span<const std::uint8_t>(request_data.data(), request_size),
      frame,
      frame_size);
  assert(status.ok());

  const std::array<std::uint8_t, 22> expected {
      0x1AU, 0xFFU, 0x10U, 0x00U, 0x02U, 0x00U,
      0x46U, 0x00U, 0x00U, 0x00U, 0x20U, 0x52U, 0x02U, 0x00U,
      0x47U, 0x00U, 0x00U, 0x00U, 0x20U, 0x52U, 0x03U, 0x00U,
  };
  assert(frame_size == expected.size());
  assert(std::equal(expected.begin(), expected.end(), frame.begin()));
}

void test_encode_ascii_e1_extended_file_register_monitor_read_request_shape() {
  const auto config = make_ascii_e1_a_config();
  const std::array<ExtendedFileRegisterAddress, 2> items {{
      {.block_number = 2U, .word_number = 70U},
      {.block_number = 3U, .word_number = 71U},
  }};

  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;
  Status status = CommandCodec::encode_read_extended_file_register_monitor(
      config,
      items,
      request_data,
      request_size);
  assert(status.ok());

  std::array<std::uint8_t, 64> frame {};
  std::size_t frame_size = 0;
  status = FrameCodec::encode_request(
      config,
      std::span<const std::uint8_t>(request_data.data(), request_size),
      frame,
      frame_size);
  assert(status.ok());

  constexpr std::string_view expected = "1BFF0010";
  assert(frame_size == expected.size());
  assert(std::memcmp(frame.data(), expected.data(), expected.size()) == 0);
}

void test_encode_binary_e1_module_buffer_read_request_shape() {
  const auto config = make_binary_e1_a_config();
  const ModuleBufferReadRequest request {
      .start_address = 0x07F0U,
      .bytes = 4U,
      .module_number = 0x13U,
  };

  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;
  Status status = CommandCodec::encode_read_module_buffer(config, request, request_data, request_size);
  assert(status.ok());

  std::array<std::uint8_t, 64> frame {};
  std::size_t frame_size = 0;
  status = FrameCodec::encode_request(
      config,
      std::span<const std::uint8_t>(request_data.data(), request_size),
      frame,
      frame_size);
  assert(status.ok());

  const std::array<std::uint8_t, 10> expected {
      0x0EU, 0xFFU, 0x10U, 0x00U, 0xF0U, 0x07U, 0x00U, 0x04U, 0x13U, 0x00U,
  };
  assert(frame_size == expected.size());
  assert(std::equal(expected.begin(), expected.end(), frame.begin()));
}

void test_encode_ascii_format4_request_appends_crlf() {
  const auto config = make_ascii_c4_format4_config();
  const BatchReadWordsRequest request {
      .head_device = {.code = mcprotocol::serial::DeviceCode::D, .number = 100},
      .points = 1,
  };

  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_data_size = 0;
  Status status = CommandCodec::encode_batch_read_words(config, request, request_data, request_data_size);
  assert(status.ok());

  std::array<std::uint8_t, 128> frame {};
  std::size_t frame_size = 0;
  status = FrameCodec::encode_request(
      config,
      std::span<const std::uint8_t>(request_data.data(), request_data_size),
      frame,
      frame_size);
  assert(status.ok());
  assert(frame_size >= 2);
  assert(frame[0] == 0x05);
  assert(frame[frame_size - 2] == 0x0D);
  assert(frame[frame_size - 1] == 0x0A);
}

void test_decode_ascii_c2_format4_ack_response() {
  auto config = make_ascii_c2_format4_config();
  config.route.self_station_enabled = true;
  config.route.self_station_no = 0x02;
  std::array<std::uint8_t, 32> frame {};
  std::size_t frame_size = 0;
  Status status = FrameCodec::encode_success_response(config, {}, frame, frame_size);
  assert(status.ok());

  assert(frame_size == 7U);
  assert(frame[0] == 0x06);
  assert(std::memcmp(frame.data() + 1, "0102", 4U) == 0);
  assert(frame[5] == 0x0D);
  assert(frame[6] == 0x0A);

  const auto decode = FrameCodec::decode_response(
      config,
      std::span<const std::uint8_t>(frame.data(), frame_size));
  assert(decode.status == DecodeStatus::Complete);
  assert(decode.frame.type == mcprotocol::serial::ResponseType::SuccessNoData);
  assert(decode.bytes_consumed == frame_size);
}

void test_decode_ascii_c2_format4_two_digit_error_response() {
  auto config = make_ascii_c2_format4_config();
  config.route.self_station_enabled = false;
  config.sum_check_enabled = true;

  constexpr std::string_view frame = "\x15""000006\r\n";
  const auto decode = FrameCodec::decode_response(
      config,
      std::span<const std::uint8_t>(
          reinterpret_cast<const std::uint8_t*>(frame.data()),
          frame.size()));
  assert(decode.status == DecodeStatus::Complete);
  assert(decode.frame.type == mcprotocol::serial::ResponseType::PlcError);
  assert(decode.frame.error_code == 0x0006U);
  assert(decode.bytes_consumed == frame.size());
}

void test_decode_ascii_format4_ack_response() {
  const auto config = make_ascii_c4_format4_config();
  std::array<std::uint8_t, 32> frame {};
  std::size_t frame_size = 0;
  Status status = FrameCodec::encode_success_response(config, {}, frame, frame_size);
  assert(status.ok());

  const auto decode = FrameCodec::decode_response(
      config,
      std::span<const std::uint8_t>(frame.data(), frame_size));
  assert(decode.status == DecodeStatus::Complete);
  assert(decode.frame.type == mcprotocol::serial::ResponseType::SuccessNoData);
  assert(decode.bytes_consumed == frame_size);
}

void test_high_level_parse_device_address() {
  mcprotocol::serial::DeviceAddress address {};
  Status status = parse_device_address("D100", address);
  assert(status.ok());
  assert(address.code == mcprotocol::serial::DeviceCode::D);
  assert(address.number == 100U);

  status = parse_device_address("X1A", address);
  assert(status.ok());
  assert(address.code == mcprotocol::serial::DeviceCode::X);
  assert(address.number == 0x1AU);

  status = parse_device_address("XFF", address);
  assert(status.ok());
  assert(address.code == mcprotocol::serial::DeviceCode::X);
  assert(address.number == 0xFFU);

  status = parse_device_address("SM100", address);
  assert(status.ok());
  assert(address.code == mcprotocol::serial::DeviceCode::SM);
  assert(address.number == 100U);

  status = parse_device_address("SWFF", address);
  assert(status.ok());
  assert(address.code == mcprotocol::serial::DeviceCode::SW);
  assert(address.number == 0xFFU);

  status = parse_device_address("SD200", address);
  assert(status.ok());
  assert(address.code == mcprotocol::serial::DeviceCode::SD);
  assert(address.number == 200U);

  status = parse_device_address("LZ10", address);
  assert(status.ok());
  assert(address.code == mcprotocol::serial::DeviceCode::LZ);
  assert(address.number == 10U);

  status = parse_device_address("RD20", address);
  assert(status.ok());
  assert(address.code == mcprotocol::serial::DeviceCode::RD);
  assert(address.number == 20U);

  status = parse_device_address("LTN0", address);
  assert(status.ok());
  assert(address.code == mcprotocol::serial::DeviceCode::LTN);
  assert(address.number == 0U);

  status = parse_device_address("LSTN1", address);
  assert(status.ok());
  assert(address.code == mcprotocol::serial::DeviceCode::LSTN);
  assert(address.number == 1U);

  status = parse_device_address("LCN2", address);
  assert(status.ok());
  assert(address.code == mcprotocol::serial::DeviceCode::LCN);
  assert(address.number == 2U);

  status = parse_device_address("LTS3", address);
  assert(status.ok());
  assert(address.code == mcprotocol::serial::DeviceCode::LTS);
  assert(address.number == 3U);

  status = parse_device_address("LTC4", address);
  assert(status.ok());
  assert(address.code == mcprotocol::serial::DeviceCode::LTC);
  assert(address.number == 4U);

  status = parse_device_address("LSTS5", address);
  assert(status.ok());
  assert(address.code == mcprotocol::serial::DeviceCode::LSTS);
  assert(address.number == 5U);

  status = parse_device_address("LSTC6", address);
  assert(status.ok());
  assert(address.code == mcprotocol::serial::DeviceCode::LSTC);
  assert(address.number == 6U);

  status = parse_device_address("LCS7", address);
  assert(status.ok());
  assert(address.code == mcprotocol::serial::DeviceCode::LCS);
  assert(address.number == 7U);

  status = parse_device_address("LCC8", address);
  assert(status.ok());
  assert(address.code == mcprotocol::serial::DeviceCode::LCC);
  assert(address.number == 8U);

  status = parse_device_address("ZR10", address);
  assert(status.ok());
  assert(address.code == mcprotocol::serial::DeviceCode::ZR);
  assert(address.number == 10U);

  status = parse_device_address("", address);
  assert(!status.ok());
  assert(status.code == StatusCode::InvalidArgument);

  status = parse_device_address("DFFFF", address);
  assert(!status.ok());
  assert(status.code == StatusCode::InvalidArgument);
  assert(std::strcmp(status.message, "Device address number is invalid") == 0);

  status = parse_device_address("D4294967296", address);
  assert(!status.ok());
  assert(status.code == StatusCode::InvalidArgument);

  status = parse_device_address("SW100000000", address);
  assert(!status.ok());
  assert(status.code == StatusCode::InvalidArgument);
}

void test_parse_link_direct_device_surface() {
  LinkDirectDevice device {};
  Status status = parse_link_direct_device("J1\\W100", device);
  assert(status.ok());
  assert(device.network_number == 0x0001U);
  assert(device.device.code == mcprotocol::serial::DeviceCode::W);
  assert(device.device.number == 0x0100U);

  status = parse_link_direct_device("J00A\\X10", device);
  assert(status.ok());
  assert(device.network_number == 0x000AU);
  assert(device.device.code == mcprotocol::serial::DeviceCode::X);
  assert(device.device.number == 0x0010U);

  status = parse_link_direct_device("J1\\D100", device);
  assert(!status.ok());

  status = parse_link_direct_device("J100000000\\W10", device);
  assert(!status.ok());
}

void test_parse_qualified_buffer_word_device_rejects_overflow() {
  QualifiedBufferWordDevice device {};
  Status status = parse_qualified_buffer_word_device("U100000000\\G0", device);
  assert(!status.ok());
  assert(status.code == StatusCode::InvalidArgument);

  status = parse_qualified_buffer_word_device("U3E0\\G4294967296", device);
  assert(!status.ok());
  assert(status.code == StatusCode::InvalidArgument);
}

void test_high_level_make_contiguous_requests() {
  BatchReadWordsRequest read_request {};
  Status status = make_batch_read_words_request("D100", 2U, read_request);
  assert(status.ok());
  assert(read_request.head_device.code == mcprotocol::serial::DeviceCode::D);
  assert(read_request.head_device.number == 100U);
  assert(read_request.points == 2U);

  const std::array<BitValue, 2> bits {BitValue::On, BitValue::Off};
  BatchWriteBitsRequest write_request {};
  status = make_batch_write_bits_request("M100", bits, write_request);
  assert(status.ok());
  assert(write_request.head_device.code == mcprotocol::serial::DeviceCode::M);
  assert(write_request.head_device.number == 100U);
  assert(write_request.bits.size() == 2U);
}

void test_high_level_protocol_presets() {
  const ProtocolConfig config = make_c4_binary_protocol();
  assert(config.frame_kind == FrameKind::C4);
  assert(config.code_mode == CodeMode::Binary);
  assert(config.target_series == PlcSeries::Q_L);
  assert(config.sum_check_enabled);
  assert(config.route.station_no == 0x00);

  const ProtocolConfig ascii_config = mcprotocol::serial::highlevel::make_c4_ascii_format2_protocol();
  assert(ascii_config.frame_kind == FrameKind::C4);
  assert(ascii_config.code_mode == CodeMode::Ascii);
  assert(ascii_config.ascii_format == AsciiFormat::Format2);
  assert(ascii_config.ascii_block_number == 0x00U);
  assert(ascii_config.sum_check_enabled);
}

void test_high_level_make_random_bit_item() {
  RandomWriteBitItem item {};
  Status status = make_random_write_bit_item("M105", BitValue::On, item);
  assert(status.ok());
  assert(item.device.code == mcprotocol::serial::DeviceCode::M);
  assert(item.device.number == 105U);
  assert(item.value == BitValue::On);
}

void test_high_level_make_random_dword_item_defaults() {
  RandomReadItem read_item {};
  Status status = make_random_read_item("LZ1", read_item);
  assert(status.ok());
  assert(read_item.device.code == mcprotocol::serial::DeviceCode::LZ);
  assert(read_item.device.number == 1U);
  assert(read_item.double_word);

  RandomWriteWordItem write_item {};
  status = make_random_write_word_item("LZ0", 0x12345678U, write_item);
  assert(status.ok());
  assert(write_item.device.code == mcprotocol::serial::DeviceCode::LZ);
  assert(write_item.device.number == 0U);
  assert(write_item.double_word);

  status = make_random_read_item("LTN1", read_item);
  assert(status.ok());
  assert(read_item.device.code == mcprotocol::serial::DeviceCode::LTN);
  assert(read_item.device.number == 1U);
  assert(read_item.double_word);

  status = make_random_read_item("LSTN2", read_item);
  assert(status.ok());
  assert(read_item.device.code == mcprotocol::serial::DeviceCode::LSTN);
  assert(read_item.device.number == 2U);
  assert(read_item.double_word);

  status = make_random_write_word_item("LCN3", 0x12345678U, write_item);
  assert(status.ok());
  assert(write_item.device.code == mcprotocol::serial::DeviceCode::LCN);
  assert(write_item.device.number == 3U);
  assert(write_item.double_word);
}

void test_high_level_make_random_request_from_specs() {
  const std::array<RandomReadSpec, 3> specs {{
      {.device = "D100"},
      {.device = "LZ0"},
      {.device = "LTN1"},
  }};
  std::array<RandomReadItem, 3> items {};
  RandomReadRequest request {};
  Status status = make_random_read_request(specs, items, request);
  assert(status.ok());
  assert(request.items.size() == specs.size());
  assert(request.items[0].device.code == mcprotocol::serial::DeviceCode::D);
  assert(!request.items[0].double_word);
  assert(request.items[1].device.code == mcprotocol::serial::DeviceCode::LZ);
  assert(request.items[1].double_word);
  assert(request.items[2].device.code == mcprotocol::serial::DeviceCode::LTN);
  assert(request.items[2].double_word);

  std::array<RandomReadItem, 2> too_small {};
  status = make_random_read_request(specs, too_small, request);
  assert(!status.ok());
  assert(status.code == StatusCode::BufferTooSmall);
}

void test_high_level_make_random_write_items_from_specs() {
  const std::array<RandomWriteWordSpec, 2> word_specs {{
      {.device = "D100", .value = 0x1234U},
      {.device = "LZ0", .value = 0x12345678U},
  }};
  std::array<RandomWriteWordItem, 2> word_items {};
  std::span<const RandomWriteWordItem> word_view {};
  Status status = make_random_write_word_items(word_specs, word_items, word_view);
  assert(status.ok());
  assert(word_view.size() == word_specs.size());
  assert(word_view[0].device.code == mcprotocol::serial::DeviceCode::D);
  assert(word_view[0].value == 0x1234U);
  assert(!word_view[0].double_word);
  assert(word_view[1].device.code == mcprotocol::serial::DeviceCode::LZ);
  assert(word_view[1].value == 0x12345678U);
  assert(word_view[1].double_word);

  const std::array<RandomWriteBitSpec, 2> bit_specs {{
      {.device = "M100", .value = BitValue::On},
      {.device = "Y2F", .value = BitValue::Off},
  }};
  std::array<RandomWriteBitItem, 2> bit_items {};
  std::span<const RandomWriteBitItem> bit_view {};
  status = make_random_write_bit_items(bit_specs, bit_items, bit_view);
  assert(status.ok());
  assert(bit_view.size() == bit_specs.size());
  assert(bit_view[0].device.code == mcprotocol::serial::DeviceCode::M);
  assert(bit_view[0].value == BitValue::On);
  assert(bit_view[1].device.code == mcprotocol::serial::DeviceCode::Y);
  assert(bit_view[1].device.number == 0x2FU);
  assert(bit_view[1].value == BitValue::Off);
}

void test_high_level_make_monitor_registration_from_specs() {
  const std::array<RandomReadSpec, 2> specs {{
      {.device = "D100"},
      {.device = "LZ0"},
  }};
  std::array<RandomReadItem, 2> items {};
  MonitorRegistration request {};
  Status status = make_monitor_registration(specs, items, request);
  assert(status.ok());
  assert(request.items.size() == specs.size());
  assert(request.items[0].device.code == mcprotocol::serial::DeviceCode::D);
  assert(!request.items[0].double_word);
  assert(request.items[1].device.code == mcprotocol::serial::DeviceCode::LZ);
  assert(request.items[1].double_word);
}

void test_encode_sm_sd_and_lz_device_codes() {
  const auto config = make_binary_c4_config();

  {
    const BatchReadBitsRequest request {
        .head_device = {.code = mcprotocol::serial::DeviceCode::SM, .number = 100},
        .points = 1,
    };
    std::array<std::uint8_t, 32> request_data {};
    std::size_t request_data_size = 0;
    Status status = CommandCodec::encode_batch_read_bits(config, request, request_data, request_data_size);
    assert(status.ok());
    const std::array<std::uint8_t, 10> expected {0x01, 0x04, 0x01, 0x00, 0x64, 0x00, 0x00, 0x91, 0x01, 0x00};
    assert(request_data_size == expected.size());
    assert(std::equal(expected.begin(), expected.end(), request_data.begin()));
  }

  {
    const BatchReadBitsRequest request {
        .head_device = {.code = mcprotocol::serial::DeviceCode::LTS, .number = 3},
        .points = 1,
    };
    std::array<std::uint8_t, 32> request_data {};
    std::size_t request_data_size = 0;
    Status status = CommandCodec::encode_batch_read_bits(config, request, request_data, request_data_size);
    assert(status.ok());
    const std::array<std::uint8_t, 10> expected {0x01, 0x04, 0x01, 0x00, 0x03, 0x00, 0x00, 0x51, 0x01, 0x00};
    assert(request_data_size == expected.size());
    assert(std::equal(expected.begin(), expected.end(), request_data.begin()));
  }

  {
    const BatchReadWordsRequest request {
        .head_device = {.code = mcprotocol::serial::DeviceCode::SD, .number = 100},
        .points = 1,
    };
    std::array<std::uint8_t, 32> request_data {};
    std::size_t request_data_size = 0;
    Status status = CommandCodec::encode_batch_read_words(config, request, request_data, request_data_size);
    assert(status.ok());
    const std::array<std::uint8_t, 10> expected {0x01, 0x04, 0x00, 0x00, 0x64, 0x00, 0x00, 0xA9, 0x01, 0x00};
    assert(request_data_size == expected.size());
    assert(std::equal(expected.begin(), expected.end(), request_data.begin()));
  }

  {
    const BatchReadWordsRequest request {
        .head_device = {.code = mcprotocol::serial::DeviceCode::RD, .number = 20},
        .points = 1,
    };
    std::array<std::uint8_t, 32> request_data {};
    std::size_t request_data_size = 0;
    Status status = CommandCodec::encode_batch_read_words(config, request, request_data, request_data_size);
    assert(status.ok());
    const std::array<std::uint8_t, 10> expected {0x01, 0x04, 0x00, 0x00, 0x14, 0x00, 0x00, 0x2C, 0x01, 0x00};
    assert(request_data_size == expected.size());
    assert(std::equal(expected.begin(), expected.end(), request_data.begin()));
  }

  {
    const BatchReadWordsRequest request {
        .head_device = {.code = mcprotocol::serial::DeviceCode::ZR, .number = 10},
        .points = 1,
    };
    std::array<std::uint8_t, 32> request_data {};
    std::size_t request_data_size = 0;
    Status status = CommandCodec::encode_batch_read_words(config, request, request_data, request_data_size);
    assert(status.ok());
    const std::array<std::uint8_t, 10> expected {0x01, 0x04, 0x00, 0x00, 0x0A, 0x00, 0x00, 0xB0, 0x01, 0x00};
    assert(request_data_size == expected.size());
    assert(std::equal(expected.begin(), expected.end(), request_data.begin()));
  }

  {
    const std::array<RandomReadItem, 1> items {{
        {.device = {.code = mcprotocol::serial::DeviceCode::LZ, .number = 10}, .double_word = true},
    }};
    const RandomReadRequest request {.items = items};
    std::array<std::uint8_t, 32> request_data {};
    std::size_t request_data_size = 0;
    Status status = CommandCodec::encode_random_read(make_binary_c4_iqr_config(), request, request_data, request_data_size);
    assert(status.ok());
    const std::array<std::uint8_t, 12> expected {
        0x03, 0x04, 0x02, 0x00, 0x00, 0x01, 0x0A, 0x00, 0x00, 0x00, 0x62, 0x00};
    assert(request_data_size == expected.size());
    assert(std::equal(expected.begin(), expected.end(), request_data.begin()));

    request_data_size = 0;
    status = CommandCodec::encode_random_read(config, request, request_data, request_data_size);
    assert(!status.ok());
    assert(status.code == StatusCode::InvalidArgument);
  }

  {
    const std::array<RandomReadItem, 1> items {{
        {.device = {.code = mcprotocol::serial::DeviceCode::LTN, .number = 0}, .double_word = true},
    }};
    const RandomReadRequest request {.items = items};
    std::array<std::uint8_t, 32> request_data {};
    std::size_t request_data_size = 0;
    Status status = CommandCodec::encode_random_read(make_binary_c4_iqr_config(), request, request_data, request_data_size);
    assert(status.ok());
    const std::array<std::uint8_t, 12> expected {
        0x03, 0x04, 0x02, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x52, 0x00};
    assert(request_data_size == expected.size());
    assert(std::equal(expected.begin(), expected.end(), request_data.begin()));
  }

  {
    const std::array<RandomReadItem, 1> items {{
        {.device = {.code = mcprotocol::serial::DeviceCode::LCN, .number = 3}, .double_word = true},
    }};
    const RandomReadRequest request {.items = items};
    std::array<std::uint8_t, 32> request_data {};
    std::size_t request_data_size = 0;
    Status status = CommandCodec::encode_random_read(make_binary_c4_iqr_config(), request, request_data, request_data_size);
    assert(status.ok());
    const std::array<std::uint8_t, 12> expected {
        0x03, 0x04, 0x02, 0x00, 0x00, 0x01, 0x03, 0x00, 0x00, 0x00, 0x56, 0x00};
    assert(request_data_size == expected.size());
    assert(std::equal(expected.begin(), expected.end(), request_data.begin()));
  }
}

void test_encode_batch_write_words_ascii_order() {
  const auto config = make_ascii_c4_format4_config();
  const std::array<std::uint16_t, 2> values {0x007BU, 0x01C8U};
  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;

  const Status status = CommandCodec::encode_batch_write_words(
      config,
      BatchWriteWordsRequest {
          .head_device = {.code = mcprotocol::serial::DeviceCode::D, .number = 100},
          .words = values,
      },
      request_data,
      request_size);
  assert(status.ok());

  constexpr std::string_view expected = "14010000D*0001000002007B01C8";
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_extended_batch_read_words_ascii_matches_manual_shape() {
  const auto config = make_ascii_c4_format4_iqr_config();
  const QualifiedBufferWordDevice device {
      .kind = QualifiedBufferDeviceKind::G,
      .module_number = 0x03E0U,
      .word_address = 1U,
  };
  std::array<std::uint8_t, 96> request_data {};
  std::size_t request_size = 0;

  const Status status = CommandCodec::encode_extended_batch_read_words(
      config,
      device,
      1U,
      request_data,
      request_size);
  assert(status.ok());

  constexpr std::string_view expected_prefix = "0401008200U3E00000G***0000000001";
  assert(request_size == expected_prefix.size() + 4U);
  assert(std::memcmp(request_data.data(), expected_prefix.data(), expected_prefix.size()) == 0);
  assert(std::memcmp(request_data.data() + expected_prefix.size(), "0001", 4U) == 0);
}

void test_encode_extended_batch_read_words_binary_matches_capture_shape() {
  const auto config = make_binary_c4_iqr_config();
  const QualifiedBufferWordDevice device {
      .kind = QualifiedBufferDeviceKind::G,
      .module_number = 0x03E0U,
      .word_address = 10U,
  };
  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;

  const Status status = CommandCodec::encode_extended_batch_read_words(
      config,
      device,
      1U,
      request_data,
      request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 19> expected {
      0x01, 0x04, 0x82, 0x00,
      0x00, 0x00,
      0x0A, 0x00, 0x00, 0x00,
      0xAB, 0x00,
      0x00, 0x00,
      0xE0, 0x03,
      0xFA,
      0x01, 0x00,
  };
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_extended_batch_read_words_binary_hg_matches_capture_shape() {
  const auto config = make_binary_c4_iqr_config();
  const QualifiedBufferWordDevice device {
      .kind = QualifiedBufferDeviceKind::HG,
      .module_number = 0x03E0U,
      .word_address = 20U,
  };
  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;

  const Status status = CommandCodec::encode_extended_batch_read_words(
      config,
      device,
      1U,
      request_data,
      request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 19> expected {
      0x01, 0x04, 0x82, 0x00,
      0x00, 0x00,
      0x14, 0x00, 0x00, 0x00,
      0x2E, 0x00,
      0x00, 0x00,
      0xE0, 0x03,
      0xFA,
      0x01, 0x00,
  };
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_extended_batch_read_words_binary_module_access_ql_shape() {
  const auto config = make_binary_c4_config();
  const QualifiedBufferWordDevice device {
      .kind = QualifiedBufferDeviceKind::G,
      .module_number = 0x0003U,
      .word_address = 1U,
  };
  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;

  const Status status = CommandCodec::encode_extended_batch_read_words(
      config,
      device,
      1U,
      request_data,
      request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 15> expected {
      0x01, 0x04, 0x80, 0x00,
      0x00, 0x00,
      0x01, 0x00, 0x00,
      0xAB,
      0x00, 0x00,
      0x03, 0x00,
      0xF8,
  };
  assert(request_size == expected.size() + 2U);
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
  assert(request_data[expected.size()] == 0x01);
  assert(request_data[expected.size() + 1U] == 0x00);
}

void test_encode_link_direct_batch_read_words_binary_iqr_matches_manual_shape() {
  const auto config = make_binary_c4_iqr_config();
  const LinkDirectDevice device {
      .network_number = 0x0001U,
      .device = {.code = mcprotocol::serial::DeviceCode::W, .number = 0x0100U},
  };
  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;

  const Status status = CommandCodec::encode_link_direct_batch_read_words(
      config,
      device,
      1U,
      request_data,
      request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 19> expected {
      0x01, 0x04, 0x82, 0x00,
      0x00, 0x00,
      0x00, 0x01, 0x00, 0x00,
      0xB4, 0x00,
      0x00, 0x00,
      0x01, 0x00,
      0xF9,
      0x01, 0x00,
  };
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_link_direct_batch_read_bits_binary_iqr_matches_manual_shape() {
  const auto config = make_binary_c4_iqr_config();
  const LinkDirectDevice device {
      .network_number = 0x0001U,
      .device = {.code = mcprotocol::serial::DeviceCode::X, .number = 0x0010U},
  };
  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;

  const Status status = CommandCodec::encode_link_direct_batch_read_bits(
      config,
      device,
      4U,
      request_data,
      request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 19> expected {
      0x01, 0x04, 0x83, 0x00,
      0x00, 0x00,
      0x10, 0x00, 0x00, 0x00,
      0x9C, 0x00,
      0x00, 0x00,
      0x01, 0x00,
      0xF9,
      0x04, 0x00,
  };
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_link_direct_batch_read_bits_binary_single_uses_addressed_point() {
  const auto config = make_binary_c4_iqr_config();
  const LinkDirectDevice device {
      .network_number = 0x0001U,
      .device = {.code = mcprotocol::serial::DeviceCode::B, .number = 0x0010U},
  };
  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;

  const Status status = CommandCodec::encode_link_direct_batch_read_bits(
      config,
      device,
      1U,
      request_data,
      request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 19> expected {
      0x01, 0x04, 0x83, 0x00,
      0x00, 0x00,
      0x10, 0x00, 0x00, 0x00,
      0xA0, 0x00,
      0x00, 0x00,
      0x01, 0x00,
      0xF9,
      0x01, 0x00,
  };
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_batch_read_bits_binary_single_uses_addressed_point() {
  const auto config = make_binary_c4_iqr_config();
  std::array<std::uint8_t, 32> request_data {};
  std::size_t request_size = 0;

  const Status status = CommandCodec::encode_batch_read_bits(
      config,
      BatchReadBitsRequest {
          .head_device = {.code = mcprotocol::serial::DeviceCode::B, .number = 0x0010U},
          .points = 1U,
      },
      request_data,
      request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 12> expected {
      0x01, 0x04, 0x03, 0x00,
      0x10, 0x00, 0x00, 0x00, 0xA0, 0x00,
      0x01, 0x00,
  };
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_link_direct_batch_write_words_binary_iqr_shape() {
  const auto config = make_binary_c4_iqr_config();
  const LinkDirectDevice device {
      .network_number = 0x0001U,
      .device = {.code = mcprotocol::serial::DeviceCode::W, .number = 0x0100U},
  };
  const std::array<std::uint16_t, 2> words {0x1234U, 0xABCDU};
  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;

  const Status status = CommandCodec::encode_link_direct_batch_write_words(
      config,
      device,
      words,
      request_data,
      request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 23> expected {
      0x01, 0x14, 0x82, 0x00,
      0x00, 0x00,
      0x00, 0x01, 0x00, 0x00,
      0xB4, 0x00,
      0x00, 0x00,
      0x01, 0x00,
      0xF9,
      0x02, 0x00,
      0x34, 0x12,
      0xCD, 0xAB,
  };
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_link_direct_batch_write_bits_binary_iqr_shape() {
  const auto config = make_binary_c4_iqr_config();
  const LinkDirectDevice device {
      .network_number = 0x0001U,
      .device = {.code = mcprotocol::serial::DeviceCode::B, .number = 0x0000U},
  };
  const std::array<BitValue, 4> bits {BitValue::On, BitValue::Off, BitValue::On, BitValue::Off};
  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;

  const Status status = CommandCodec::encode_link_direct_batch_write_bits(
      config,
      device,
      bits,
      request_data,
      request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 21> expected {
      0x01, 0x14, 0x83, 0x00,
      0x00, 0x00,
      0x00, 0x00, 0x00, 0x00,
      0xA0, 0x00,
      0x00, 0x00,
      0x01, 0x00,
      0xF9,
      0x04, 0x00,
      0x10, 0x10,
  };
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_link_direct_random_read_binary_iqr_shape() {
  const auto config = make_binary_c4_iqr_config();
  const std::array<LinkDirectRandomReadItem, 2> items {{
      {.device = {.network_number = 0x0001U, .device = {.code = mcprotocol::serial::DeviceCode::W, .number = 0x0100U}}},
      {.device = {.network_number = 0x0001U, .device = {.code = mcprotocol::serial::DeviceCode::SW, .number = 0x0000U}}},
  }};

  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_link_direct_random_read(
      config,
      std::span<const LinkDirectRandomReadItem>(items.data(), items.size()),
      request_data,
      request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 28> expected {
      0x03, 0x04, 0x80, 0x00,
      0x02, 0x00,
      0x00, 0x00, 0x00, 0x01, 0x00, 0xB4, 0x00, 0x00, 0x01, 0x00, 0xF9,
      0x00, 0x00, 0x00, 0x00, 0x00, 0xB5, 0x00, 0x00, 0x01, 0x00, 0xF9,
  };
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_link_direct_random_write_words_binary_iqr_shape() {
  const auto config = make_binary_c4_iqr_config();
  const std::array<LinkDirectRandomWriteWordItem, 2> items {{
      {.device = {.network_number = 0x0001U, .device = {.code = mcprotocol::serial::DeviceCode::W, .number = 0x0100U}},
       .value = 0x1234U},
      {.device = {.network_number = 0x0001U, .device = {.code = mcprotocol::serial::DeviceCode::SW, .number = 0x0000U}},
       .value = 0xABCDU},
  }};

  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_link_direct_random_write_words(
      config,
      std::span<const LinkDirectRandomWriteWordItem>(items.data(), items.size()),
      request_data,
      request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 32> expected {
      0x02, 0x14, 0x80, 0x00,
      0x02, 0x00,
      0x00, 0x00, 0x00, 0x01, 0x00, 0xB4, 0x00, 0x00, 0x01, 0x00, 0xF9, 0x34, 0x12,
      0x00, 0x00, 0x00, 0x00, 0x00, 0xB5, 0x00, 0x00, 0x01, 0x00, 0xF9, 0xCD, 0xAB,
  };
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_link_direct_random_write_bits_binary_iqr_shape() {
  const auto config = make_binary_c4_iqr_config();
  const std::array<LinkDirectRandomWriteBitItem, 2> items {{
      {.device = {.network_number = 0x0001U, .device = {.code = mcprotocol::serial::DeviceCode::B, .number = 0x0010U}},
       .value = BitValue::On},
      {.device = {.network_number = 0x0001U, .device = {.code = mcprotocol::serial::DeviceCode::SB, .number = 0x0010U}},
       .value = BitValue::Off},
  }};

  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_link_direct_random_write_bits(
      config,
      std::span<const LinkDirectRandomWriteBitItem>(items.data(), items.size()),
      request_data,
      request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 29> expected {
      0x02, 0x14, 0x81, 0x00,
      0x02,
      0x00, 0x00, 0x10, 0x00, 0x00, 0xA0, 0x00, 0x00, 0x01, 0x00, 0xF9, 0x01,
      0x00, 0x00, 0x10, 0x00, 0x00, 0xA1, 0x00, 0x00, 0x01, 0x00, 0xF9, 0x00,
  };
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_link_direct_multi_block_read_binary_iqr_shape() {
  const auto config = make_binary_c4_iqr_config();
  const std::array<LinkDirectMultiBlockReadBlock, 2> blocks {{
      {.head_device = {.network_number = 0x0001U, .device = {.code = mcprotocol::serial::DeviceCode::W, .number = 0x0100U}},
       .points = 2U,
       .bit_block = false},
      {.head_device = {.network_number = 0x0001U, .device = {.code = mcprotocol::serial::DeviceCode::B, .number = 0x0010U}},
       .points = 1U,
       .bit_block = true},
  }};

  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_link_direct_multi_block_read(
      config,
      LinkDirectMultiBlockReadRequest {
          .blocks = std::span<const LinkDirectMultiBlockReadBlock>(blocks.data(), blocks.size()),
      },
      request_data,
      request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 36> expected {
      0x06, 0x04, 0x82, 0x00,
      0x01, 0x01,
      0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0xB4, 0x00, 0x00, 0x00, 0x01, 0x00, 0xF9, 0x02, 0x00,
      0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0xA0, 0x00, 0x00, 0x00, 0x01, 0x00, 0xF9, 0x01, 0x00,
  };
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_link_direct_multi_block_write_binary_iqr_shape() {
  const auto config = make_binary_c4_iqr_config();
  const std::array<std::uint16_t, 2> word_values {0x1234U, 0xABCDU};
  const std::array<BitValue, 16> bit_values {{
      BitValue::On, BitValue::Off, BitValue::On, BitValue::Off,
      BitValue::On, BitValue::Off, BitValue::On, BitValue::Off,
      BitValue::Off, BitValue::On, BitValue::Off, BitValue::On,
      BitValue::Off, BitValue::On, BitValue::Off, BitValue::On,
  }};
  const std::array<LinkDirectMultiBlockWriteBlock, 2> blocks {{
      {.head_device = {.network_number = 0x0001U, .device = {.code = mcprotocol::serial::DeviceCode::W, .number = 0x0100U}},
       .points = 2U,
       .bit_block = false,
       .words = std::span<const std::uint16_t>(word_values.data(), word_values.size())},
      {.head_device = {.network_number = 0x0001U, .device = {.code = mcprotocol::serial::DeviceCode::B, .number = 0x0010U}},
       .points = 1U,
       .bit_block = true,
       .bits = std::span<const BitValue>(bit_values.data(), bit_values.size())},
  }};

  std::array<std::uint8_t, 96> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_link_direct_multi_block_write(
      config,
      LinkDirectMultiBlockWriteRequest {
          .blocks = std::span<const LinkDirectMultiBlockWriteBlock>(blocks.data(), blocks.size()),
      },
      request_data,
      request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 42> expected {
      0x06, 0x14, 0x82, 0x00,
      0x01, 0x01,
      0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0xB4, 0x00, 0x00, 0x00, 0x01, 0x00, 0xF9, 0x02, 0x00, 0x34, 0x12, 0xCD, 0xAB,
      0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0xA0, 0x00, 0x00, 0x00, 0x01, 0x00, 0xF9, 0x01, 0x00, 0x55, 0xAA,
  };
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_link_direct_multi_block_write_binary_bit_order() {
  const auto config = make_binary_c4_iqr_config();
  const std::array<BitValue, 16> bit_values {{
      BitValue::On, BitValue::Off, BitValue::Off, BitValue::Off,
      BitValue::Off, BitValue::Off, BitValue::Off, BitValue::Off,
      BitValue::Off, BitValue::Off, BitValue::Off, BitValue::Off,
      BitValue::Off, BitValue::Off, BitValue::Off, BitValue::Off,
  }};
  const std::array<LinkDirectMultiBlockWriteBlock, 1> blocks {{
      {.head_device = {.network_number = 0x0001U, .device = {.code = mcprotocol::serial::DeviceCode::B, .number = 0x0010U}},
       .points = 1U,
       .bit_block = true,
       .bits = std::span<const BitValue>(bit_values.data(), bit_values.size())},
  }};

  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_link_direct_multi_block_write(
      config,
      LinkDirectMultiBlockWriteRequest {
          .blocks = std::span<const LinkDirectMultiBlockWriteBlock>(blocks.data(), blocks.size()),
      },
      request_data,
      request_size);
  assert(status.ok());
  assert(request_size >= 2U);
  assert(request_data[request_size - 2U] == 0x01U);
  assert(request_data[request_size - 1U] == 0x00U);
}

void test_encode_link_direct_register_monitor_binary_iqr_shape() {
  const auto config = make_binary_c4_iqr_config();
  const std::array<LinkDirectRandomReadItem, 2> items {{
      {.device = {.network_number = 0x0001U, .device = {.code = mcprotocol::serial::DeviceCode::W, .number = 0x0100U}}},
      {.device = {.network_number = 0x0001U, .device = {.code = mcprotocol::serial::DeviceCode::SW, .number = 0x0000U}}},
  }};

  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_link_direct_register_monitor(
      config,
      LinkDirectMonitorRegistration {
          .items = std::span<const LinkDirectRandomReadItem>(items.data(), items.size()),
      },
      request_data,
      request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 28> expected {
      0x01, 0x08, 0x80, 0x00,
      0x02, 0x00,
      0x00, 0x00, 0x00, 0x01, 0x00, 0xB4, 0x00, 0x00, 0x01, 0x00, 0xF9,
      0x00, 0x00, 0x00, 0x00, 0x00, 0xB5, 0x00, 0x00, 0x01, 0x00, 0xF9,
  };
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_batch_write_bits_binary_single_even_uses_addressed_point_and_high_nibble() {
  const auto config = make_binary_c4_iqr_config();
  std::array<std::uint8_t, 32> request_data {};
  std::size_t request_size = 0;

  const Status status = CommandCodec::encode_batch_write_bits(
      config,
      BatchWriteBitsRequest {
          .head_device = {.code = mcprotocol::serial::DeviceCode::B, .number = 0x0010U},
          .bits = std::array<BitValue, 1> {BitValue::On},
      },
      request_data,
      request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 13> expected {
      0x01, 0x14, 0x03, 0x00,
      0x10, 0x00, 0x00, 0x00, 0xA0, 0x00,
      0x01, 0x00,
      0x10,
  };
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_batch_write_bits_binary_single_odd_uses_addressed_point_and_high_nibble() {
  const auto config = make_binary_c4_iqr_config();
  std::array<std::uint8_t, 32> request_data {};
  std::size_t request_size = 0;

  const Status status = CommandCodec::encode_batch_write_bits(
      config,
      BatchWriteBitsRequest {
          .head_device = {.code = mcprotocol::serial::DeviceCode::B, .number = 0x0011U},
          .bits = std::array<BitValue, 1> {BitValue::On},
      },
      request_data,
      request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 13> expected {
      0x01, 0x14, 0x03, 0x00,
      0x11, 0x00, 0x00, 0x00, 0xA0, 0x00,
      0x01, 0x00,
      0x10,
  };
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_link_direct_batch_write_bits_binary_single_even_uses_addressed_point_and_high_nibble() {
  const auto config = make_binary_c4_iqr_config();
  const LinkDirectDevice device {
      .network_number = 0x0001U,
      .device = {.code = mcprotocol::serial::DeviceCode::B, .number = 0x0010U},
  };
  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;

  const Status status = CommandCodec::encode_link_direct_batch_write_bits(
      config,
      device,
      std::array<BitValue, 1> {BitValue::On},
      request_data,
      request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 20> expected {
      0x01, 0x14, 0x83, 0x00,
      0x00, 0x00,
      0x10, 0x00, 0x00, 0x00,
      0xA0, 0x00,
      0x00, 0x00,
      0x01, 0x00,
      0xF9,
      0x01, 0x00,
      0x10,
  };
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_parse_batch_read_bits_binary_single_uses_high_nibble() {
  const auto config = make_binary_c4_iqr_config();
  const BatchReadBitsRequest request {
      .head_device = {.code = mcprotocol::serial::DeviceCode::B, .number = 0x0010U},
      .points = 1U,
  };
  const std::array<std::uint8_t, 1> response {0x10U};
  std::array<BitValue, 1> bits {};
  const Status status = CommandCodec::parse_batch_read_bits_response(config, request, response, bits);
  assert(status.ok());
  assert(bits[0] == BitValue::On);
}

void test_encode_batch_write_bits_binary_two_points_use_high_then_low_nibbles() {
  const auto config = make_binary_c4_iqr_config();
  std::array<std::uint8_t, 32> request_data {};
  std::size_t request_size = 0;

  const Status status = CommandCodec::encode_batch_write_bits(
      config,
      BatchWriteBitsRequest {
          .head_device = {.code = mcprotocol::serial::DeviceCode::B, .number = 0x0010U},
          .bits = std::array<BitValue, 2> {BitValue::On, BitValue::Off},
      },
      request_data,
      request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 13> expected {
      0x01, 0x14, 0x03, 0x00,
      0x10, 0x00, 0x00, 0x00, 0xA0, 0x00,
      0x02, 0x00,
      0x10,
  };
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_parse_batch_read_bits_binary_two_points_use_high_then_low_nibbles() {
  const auto config = make_binary_c4_iqr_config();
  const BatchReadBitsRequest request {
      .head_device = {.code = mcprotocol::serial::DeviceCode::B, .number = 0x0010U},
      .points = 2U,
  };
  const std::array<std::uint8_t, 1> response {0x10U};
  std::array<BitValue, 2> bits {};
  const Status status = CommandCodec::parse_batch_read_bits_response(config, request, response, bits);
  assert(status.ok());
  assert(bits[0] == BitValue::On);
  assert(bits[1] == BitValue::Off);
}

void test_encode_batch_write_words_ascii_limit_matches_buffer() {
  const auto config = make_ascii_c4_format4_config();
  std::array<std::uint16_t, 870> ok_values {};
  std::array<std::uint16_t, 871> too_many_values {};
  std::array<std::uint8_t, mcprotocol::serial::kMaxRequestDataBytes> request_data {};
  std::size_t request_size = 0;

  Status status = CommandCodec::encode_batch_write_words(
      config,
      BatchWriteWordsRequest {
          .head_device = {.code = mcprotocol::serial::DeviceCode::D, .number = 100},
          .words = ok_values,
      },
      request_data,
      request_size);
  assert(status.ok());

  status = CommandCodec::encode_batch_write_words(
      config,
      BatchWriteWordsRequest {
          .head_device = {.code = mcprotocol::serial::DeviceCode::D, .number = 100},
          .words = too_many_values,
      },
      request_data,
      request_size);
  assert(status.code == StatusCode::InvalidArgument);
}

void test_encode_batch_write_bits_ascii_limit_matches_buffer() {
  const auto config = make_ascii_c4_format4_config();
  std::array<BitValue, 3480> ok_values {};
  std::array<BitValue, 3481> too_many_values {};
  std::array<std::uint8_t, mcprotocol::serial::kMaxRequestDataBytes> request_data {};
  std::size_t request_size = 0;

  Status status = CommandCodec::encode_batch_write_bits(
      config,
      BatchWriteBitsRequest {
          .head_device = {.code = mcprotocol::serial::DeviceCode::M, .number = 100},
          .bits = ok_values,
      },
      request_data,
      request_size);
  assert(status.ok());

  status = CommandCodec::encode_batch_write_bits(
      config,
      BatchWriteBitsRequest {
          .head_device = {.code = mcprotocol::serial::DeviceCode::M, .number = 100},
          .bits = too_many_values,
      },
      request_data,
      request_size);
  assert(status.code == StatusCode::InvalidArgument);
}

void test_encode_random_write_words_ascii_matches_manual() {
  const auto config = make_ascii_c4_format4_config();
  const std::array<RandomWriteWordItem, 7> items {{
      {.device = {.code = mcprotocol::serial::DeviceCode::D, .number = 0}, .value = 0x0550U, .double_word = false},
      {.device = {.code = mcprotocol::serial::DeviceCode::D, .number = 1}, .value = 0x0575U, .double_word = false},
      {.device = {.code = mcprotocol::serial::DeviceCode::M, .number = 100}, .value = 0x0540U, .double_word = false},
      {.device = {.code = mcprotocol::serial::DeviceCode::X, .number = 0x20}, .value = 0x0583U, .double_word = false},
      {.device = {.code = mcprotocol::serial::DeviceCode::D, .number = 1500}, .value = 0x12024391U, .double_word = true},
      {.device = {.code = mcprotocol::serial::DeviceCode::Y, .number = 0x160}, .value = 0x23752607U, .double_word = true},
      {.device = {.code = mcprotocol::serial::DeviceCode::M, .number = 1111}, .value = 0x04250475U, .double_word = true},
  }};

  std::array<std::uint8_t, 256> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_random_write_words(
      config,
      std::span<const RandomWriteWordItem>(items.data(), items.size()),
      request_data,
      request_size);
  assert(status.ok());

  constexpr std::string_view expected =
      "1402000000040003"
      "D*0000000550"
      "D*0000010575"
      "M*0001000540"
      "X*0000200583"
      "D*00150012024391"
      "Y*00016023752607"
      "M*00111104250475";
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_random_read_binary_iqr_layout() {
  const auto config = make_binary_c4_iqr_config();
  const std::array<mcprotocol::serial::RandomReadItem, 2> items {{
      {.device = {.code = mcprotocol::serial::DeviceCode::D, .number = 100}, .double_word = false},
      {.device = {.code = mcprotocol::serial::DeviceCode::M, .number = 100}, .double_word = false},
  }};

  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_random_read(
      config,
      mcprotocol::serial::RandomReadRequest {
          .items = std::span<const mcprotocol::serial::RandomReadItem>(items.data(), items.size()),
      },
      request_data,
      request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 18> expected {
      0x03, 0x04, 0x02, 0x00,
      0x02, 0x00,
      0x64, 0x00, 0x00, 0x00, 0xA8, 0x00,
      0x64, 0x00, 0x00, 0x00, 0x90, 0x00,
  };
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_random_read_binary_ql_layout() {
  const auto config = make_binary_c4_config();
  const std::array<mcprotocol::serial::RandomReadItem, 2> items {{
      {.device = {.code = mcprotocol::serial::DeviceCode::D, .number = 100}, .double_word = false},
      {.device = {.code = mcprotocol::serial::DeviceCode::M, .number = 100}, .double_word = false},
  }};

  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_random_read(
      config,
      mcprotocol::serial::RandomReadRequest {
          .items = std::span<const mcprotocol::serial::RandomReadItem>(items.data(), items.size()),
      },
      request_data,
      request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 14> expected {
      0x03, 0x04, 0x00, 0x00,
      0x02, 0x00,
      0x64, 0x00, 0x00, 0xA8,
      0x64, 0x00, 0x00, 0x90,
  };
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_random_write_words_binary_ql_layout() {
  const auto config = make_binary_c4_config();
  const std::array<RandomWriteWordItem, 2> items {{
      {.device = {.code = mcprotocol::serial::DeviceCode::D, .number = 100}, .value = 0x0001U, .double_word = false},
      {.device = {.code = mcprotocol::serial::DeviceCode::D, .number = 101}, .value = 0x0002U, .double_word = false},
  }};

  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_random_write_words(
      config,
      std::span<const RandomWriteWordItem>(items.data(), items.size()),
      request_data,
      request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 18> expected {
      0x02, 0x14, 0x00, 0x00,
      0x02, 0x00,
      0x64, 0x00, 0x00, 0xA8, 0x01, 0x00,
      0x65, 0x00, 0x00, 0xA8, 0x02, 0x00,
  };
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_random_write_words_binary_iqr_layout() {
  const auto config = make_binary_c4_iqr_config();
  const std::array<RandomWriteWordItem, 2> items {{
      {.device = {.code = mcprotocol::serial::DeviceCode::D, .number = 100}, .value = 0x0001U, .double_word = false},
      {.device = {.code = mcprotocol::serial::DeviceCode::D, .number = 101}, .value = 0x0002U, .double_word = false},
  }};

  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_random_write_words(
      config,
      std::span<const RandomWriteWordItem>(items.data(), items.size()),
      request_data,
      request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 22> expected {
      0x02, 0x14, 0x02, 0x00,
      0x02, 0x00,
      0x64, 0x00, 0x00, 0x00, 0xA8, 0x00, 0x01, 0x00,
      0x65, 0x00, 0x00, 0x00, 0xA8, 0x00, 0x02, 0x00,
  };
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_random_write_bits_ascii_matches_manual() {
  const auto config = make_ascii_c4_format4_config();
  const std::array<RandomWriteBitItem, 2> items {{
      {.device = {.code = mcprotocol::serial::DeviceCode::M, .number = 50}, .value = BitValue::Off},
      {.device = {.code = mcprotocol::serial::DeviceCode::Y, .number = 0x2F}, .value = BitValue::On},
  }};

  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_random_write_bits(
      config,
      std::span<const RandomWriteBitItem>(items.data(), items.size()),
      request_data,
      request_size);
  assert(status.ok());

  constexpr std::string_view expected = "140200010002M*00005000Y*00002F01";
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_random_write_bits_ascii_iqr_shape() {
  const auto config = make_ascii_c4_format4_iqr_config();
  const std::array<RandomWriteBitItem, 2> items {{
      {.device = {.code = mcprotocol::serial::DeviceCode::M, .number = 50}, .value = BitValue::Off},
      {.device = {.code = mcprotocol::serial::DeviceCode::Y, .number = 0x2F}, .value = BitValue::On},
  }};

  std::array<std::uint8_t, 96> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_random_write_bits(
      config,
      std::span<const RandomWriteBitItem>(items.data(), items.size()),
      request_data,
      request_size);
  assert(status.ok());

  constexpr std::string_view expected = "140200030002M***000000500000Y***0000002F0001";
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_random_write_bits_binary_iqr_layout() {
  const auto config = make_binary_c4_iqr_config();
  const std::array<RandomWriteBitItem, 2> items {{
      {.device = {.code = mcprotocol::serial::DeviceCode::M, .number = 50}, .value = BitValue::Off},
      {.device = {.code = mcprotocol::serial::DeviceCode::Y, .number = 0x2F}, .value = BitValue::On},
  }};

  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_random_write_bits(
      config,
      std::span<const RandomWriteBitItem>(items.data(), items.size()),
      request_data,
      request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 21> expected {
      0x02, 0x14, 0x03, 0x00, 0x02,
      0x32, 0x00, 0x00, 0x00, 0x90, 0x00, 0x00, 0x00,
      0x2F, 0x00, 0x00, 0x00, 0x9D, 0x00, 0x01, 0x00,
  };
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_random_write_bits_binary_ql_keeps_device_numbers() {
  const auto config = make_binary_c4_config();
  const std::array<RandomWriteBitItem, 2> items {{
      {.device = {.code = mcprotocol::serial::DeviceCode::M, .number = 50}, .value = BitValue::Off},
      {.device = {.code = mcprotocol::serial::DeviceCode::Y, .number = 0x2F}, .value = BitValue::On},
  }};

  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_random_write_bits(
      config,
      std::span<const RandomWriteBitItem>(items.data(), items.size()),
      request_data,
      request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 15> expected {
      0x02, 0x14, 0x01, 0x00, 0x02, 0x32, 0x00,
      0x00, 0x90, 0x00, 0x2F, 0x00, 0x00, 0x9D, 0x01,
  };
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_multi_block_read_ascii_matches_manual() {
  const auto config = make_ascii_c4_format4_config();
  const std::array<MultiBlockReadBlock, 5> blocks {{
      {.head_device = {.code = mcprotocol::serial::DeviceCode::D, .number = 0}, .points = 4, .bit_block = false},
      {.head_device = {.code = mcprotocol::serial::DeviceCode::W, .number = 0x100}, .points = 8, .bit_block = false},
      {.head_device = {.code = mcprotocol::serial::DeviceCode::M, .number = 0}, .points = 2, .bit_block = true},
      {.head_device = {.code = mcprotocol::serial::DeviceCode::M, .number = 128}, .points = 2, .bit_block = true},
      {.head_device = {.code = mcprotocol::serial::DeviceCode::B, .number = 0x100}, .points = 3, .bit_block = true},
  }};

  std::array<std::uint8_t, 128> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_multi_block_read(
      config,
      MultiBlockReadRequest {
          .blocks = std::span<const MultiBlockReadBlock>(blocks.data(), blocks.size()),
      },
      request_data,
      request_size);
  assert(status.ok());

  constexpr std::string_view expected =
      "0406000000020003"
      "D*0000000004"
      "W*0001000008"
      "M*0000000002"
      "M*0001280002"
      "B*0001000003";
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_multi_block_read_binary_matches_capture_counts() {
  const auto config = make_binary_c4_config();

  const std::array<MultiBlockReadBlock, 3> bit_blocks {{
      {.head_device = {.code = mcprotocol::serial::DeviceCode::X, .number = 0}, .points = 1, .bit_block = true},
      {.head_device = {.code = mcprotocol::serial::DeviceCode::Y, .number = 0}, .points = 1, .bit_block = true},
      {.head_device = {.code = mcprotocol::serial::DeviceCode::B, .number = 0}, .points = 1, .bit_block = true},
  }};

  std::array<std::uint8_t, 128> request_data {};
  std::size_t request_size = 0;
  Status status = CommandCodec::encode_multi_block_read(
      config,
      MultiBlockReadRequest {
          .blocks = std::span<const MultiBlockReadBlock>(bit_blocks.data(), bit_blocks.size()),
      },
      request_data,
      request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 24> expected_bit_request {
      0x06, 0x04, 0x00, 0x00,
      0x00, 0x03,
      0x00, 0x00, 0x00, 0x9C, 0x01, 0x00,
      0x00, 0x00, 0x00, 0x9D, 0x01, 0x00,
      0x00, 0x00, 0x00, 0xA0, 0x01, 0x00,
  };
  assert(request_size == expected_bit_request.size());
  assert(std::memcmp(request_data.data(), expected_bit_request.data(), expected_bit_request.size()) == 0);

  const std::array<MultiBlockReadBlock, 3> word_blocks {{
      {.head_device = {.code = mcprotocol::serial::DeviceCode::D, .number = 0}, .points = 1, .bit_block = false},
      {.head_device = {.code = mcprotocol::serial::DeviceCode::D, .number = 10}, .points = 1, .bit_block = false},
      {.head_device = {.code = mcprotocol::serial::DeviceCode::D, .number = 100}, .points = 1, .bit_block = false},
  }};

  request_size = 0;
  status = CommandCodec::encode_multi_block_read(
      config,
      MultiBlockReadRequest {
          .blocks = std::span<const MultiBlockReadBlock>(word_blocks.data(), word_blocks.size()),
      },
      request_data,
      request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 24> expected_word_request {
      0x06, 0x04, 0x00, 0x00,
      0x03, 0x00,
      0x00, 0x00, 0x00, 0xA8, 0x01, 0x00,
      0x0A, 0x00, 0x00, 0xA8, 0x01, 0x00,
      0x64, 0x00, 0x00, 0xA8, 0x01, 0x00,
  };
  assert(request_size == expected_word_request.size());
  assert(std::memcmp(request_data.data(), expected_word_request.data(), expected_word_request.size()) == 0);
}

void test_encode_multi_block_write_binary_uses_single_byte_block_counts() {
  const auto config = make_binary_c4_config();

  const std::array<std::uint16_t, 2> word_values {0x1234U, 0x5678U};
  const std::array<BitValue, 16> bit_values {{
      BitValue::On, BitValue::Off, BitValue::On, BitValue::Off,
      BitValue::On, BitValue::Off, BitValue::On, BitValue::Off,
      BitValue::Off, BitValue::On, BitValue::Off, BitValue::On,
      BitValue::Off, BitValue::On, BitValue::Off, BitValue::On,
  }};
  const std::array<MultiBlockWriteBlock, 2> blocks {{
      {.head_device = {.code = mcprotocol::serial::DeviceCode::D, .number = 0},
       .points = 2,
       .bit_block = false,
       .words = std::span<const std::uint16_t>(word_values.data(), word_values.size())},
      {.head_device = {.code = mcprotocol::serial::DeviceCode::M, .number = 100},
       .points = 1,
       .bit_block = true,
       .bits = std::span<const BitValue>(bit_values.data(), bit_values.size())},
  }};

  std::array<std::uint8_t, 128> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_multi_block_write(
      config,
      MultiBlockWriteRequest {
          .blocks = std::span<const MultiBlockWriteBlock>(blocks.data(), blocks.size()),
      },
      request_data,
      request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 24> expected {
      0x06, 0x14, 0x00, 0x00,
      0x01, 0x01,
      0x00, 0x00, 0x00, 0xA8, 0x02, 0x00, 0x34, 0x12, 0x78, 0x56,
      0x64, 0x00, 0x00, 0x90, 0x01, 0x00, 0x55, 0xAA,
  };
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_multi_block_write_binary_bit_blocks_use_lsb_first_word_packing() {
  const auto config = make_binary_c4_config();
  const std::array<BitValue, 32> bit_values {{
      BitValue::Off, BitValue::Off, BitValue::Off, BitValue::Off,
      BitValue::Off, BitValue::Off, BitValue::Off, BitValue::Off,
      BitValue::Off, BitValue::Off, BitValue::Off, BitValue::Off,
      BitValue::Off, BitValue::Off, BitValue::Off, BitValue::Off,
      BitValue::Off, BitValue::Off, BitValue::Off, BitValue::On,
      BitValue::Off, BitValue::Off, BitValue::On, BitValue::Off,
      BitValue::Off, BitValue::Off, BitValue::On, BitValue::On,
      BitValue::Off, BitValue::On, BitValue::Off, BitValue::Off,
  }};
  const std::array<MultiBlockWriteBlock, 1> blocks {{
      {.head_device = {.code = mcprotocol::serial::DeviceCode::M, .number = 200},
       .points = 2,
       .bit_block = true,
       .bits = std::span<const BitValue>(bit_values.data(), bit_values.size())},
  }};

  std::array<std::uint8_t, 128> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_multi_block_write(
      config,
      MultiBlockWriteRequest {
          .blocks = std::span<const MultiBlockWriteBlock>(blocks.data(), blocks.size()),
      },
      request_data,
      request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 12> expected {
      0x06, 0x14, 0x00, 0x00,
      0x00, 0x01,
      0xC8, 0x00, 0x00, 0x90, 0x02, 0x00,
  };
  const std::array<std::uint8_t, 4> expected_bit_words {
      0x00, 0x00, 0x48, 0x2C,
  };
  assert(request_size == expected.size() + expected_bit_words.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
  assert(std::memcmp(request_data.data() + expected.size(), expected_bit_words.data(), expected_bit_words.size()) == 0);
}

void test_encode_register_monitor_ascii_reuses_random_read_layout() {
  const auto config = make_ascii_c4_format4_config();
  const std::array<mcprotocol::serial::RandomReadItem, 4> items {{
      {.device = {.code = mcprotocol::serial::DeviceCode::D, .number = 100}, .double_word = false},
      {.device = {.code = mcprotocol::serial::DeviceCode::D, .number = 105}, .double_word = false},
      {.device = {.code = mcprotocol::serial::DeviceCode::M, .number = 100}, .double_word = false},
      {.device = {.code = mcprotocol::serial::DeviceCode::M, .number = 105}, .double_word = false},
  }};

  std::array<std::uint8_t, 128> random_read_request {};
  std::size_t random_read_size = 0;
  Status status = CommandCodec::encode_random_read(
      config,
      mcprotocol::serial::RandomReadRequest {
          .items = std::span<const mcprotocol::serial::RandomReadItem>(items.data(), items.size()),
      },
      random_read_request,
      random_read_size);
  assert(status.ok());

  std::array<std::uint8_t, 128> monitor_request {};
  std::size_t monitor_size = 0;
  status = CommandCodec::encode_register_monitor(
      config,
      mcprotocol::serial::MonitorRegistration {
          .items = std::span<const mcprotocol::serial::RandomReadItem>(items.data(), items.size()),
      },
      monitor_request,
      monitor_size);
  assert(status.ok());

  assert(monitor_size == random_read_size);
  constexpr std::string_view expected_prefix = "08010000";
  assert(std::memcmp(monitor_request.data(), expected_prefix.data(), expected_prefix.size()) == 0);
  assert(std::memcmp(
             monitor_request.data() + expected_prefix.size(),
             random_read_request.data() + expected_prefix.size(),
             random_read_size - expected_prefix.size()) == 0);
}

void test_encode_register_monitor_binary_iqr_layout() {
  const auto config = make_binary_c4_iqr_config();
  const std::array<mcprotocol::serial::RandomReadItem, 2> items {{
      {.device = {.code = mcprotocol::serial::DeviceCode::D, .number = 100}, .double_word = false},
      {.device = {.code = mcprotocol::serial::DeviceCode::M, .number = 100}, .double_word = false},
  }};

  std::array<std::uint8_t, 64> random_read_request {};
  std::size_t random_read_size = 0;
  Status status = CommandCodec::encode_random_read(
      config,
      mcprotocol::serial::RandomReadRequest {
          .items = std::span<const mcprotocol::serial::RandomReadItem>(items.data(), items.size()),
      },
      random_read_request,
      random_read_size);
  assert(status.ok());

  std::array<std::uint8_t, 64> monitor_request {};
  std::size_t monitor_size = 0;
  status = CommandCodec::encode_register_monitor(
      config,
      mcprotocol::serial::MonitorRegistration {
          .items = std::span<const mcprotocol::serial::RandomReadItem>(items.data(), items.size()),
      },
      monitor_request,
      monitor_size);
  assert(status.ok());

  const std::array<std::uint8_t, 4> expected_prefix {0x01, 0x08, 0x02, 0x00};
  assert(monitor_size == random_read_size);
  assert(std::memcmp(monitor_request.data(), expected_prefix.data(), expected_prefix.size()) == 0);
  assert(std::memcmp(
             monitor_request.data() + expected_prefix.size(),
             random_read_request.data() + expected_prefix.size(),
             random_read_size - expected_prefix.size()) == 0);
}

void test_encode_register_monitor_binary_iqr_allows_lz_shape() {
  const auto config = make_binary_c4_iqr_config();
  const std::array<mcprotocol::serial::RandomReadItem, 1> items {{
      {.device = {.code = mcprotocol::serial::DeviceCode::LZ, .number = 1}, .double_word = true},
  }};

  std::array<std::uint8_t, 64> monitor_request {};
  std::size_t monitor_size = 0;
  Status status = CommandCodec::encode_register_monitor(
      config,
      mcprotocol::serial::MonitorRegistration {
          .items = std::span<const mcprotocol::serial::RandomReadItem>(items.data(), items.size()),
      },
      monitor_request,
      monitor_size);
  assert(status.ok());

  const std::array<std::uint8_t, 12> expected {
      0x01, 0x08, 0x02, 0x00,
      0x00, 0x01,
      0x01, 0x00, 0x00, 0x00, 0x62, 0x00,
  };
  assert(monitor_size == expected.size());
  assert(std::memcmp(monitor_request.data(), expected.data(), expected.size()) == 0);
}

void test_encode_read_monitor_ascii_matches_manual() {
  const auto config = make_ascii_c4_format4_config();
  std::array<std::uint8_t, 16> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_read_monitor(config, request_data, request_size);
  assert(status.ok());

  constexpr std::string_view expected = "08020000";
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_sparse_native_bit_helpers_match_batch_random_and_monitor_values() {
  const std::array<BitValue, 16> batch_bits_on_on {{
      BitValue::On, BitValue::Off, BitValue::Off, BitValue::Off,
      BitValue::Off, BitValue::On, BitValue::Off, BitValue::Off,
      BitValue::Off, BitValue::Off, BitValue::Off, BitValue::Off,
      BitValue::Off, BitValue::Off, BitValue::Off, BitValue::Off,
  }};
  const std::array<std::uint32_t, 2> random_raw_on_on {{
      0x0021U,
      0x0001U,
  }};
  const std::array<std::uint32_t, 2> monitor_raw_on_on = random_raw_on_on;
  assert(sparse_native_mask_word(random_raw_on_on[0]) == 0x0021U);
  assert(sparse_native_mask_word(random_raw_on_on[1]) == 0x0001U);
  assert(sparse_native_requested_bit_value(random_raw_on_on[0]) == batch_bits_on_on[0]);
  assert(sparse_native_requested_bit_value(random_raw_on_on[1]) == batch_bits_on_on[5]);
  assert(sparse_native_requested_bit_value(monitor_raw_on_on[0]) == batch_bits_on_on[0]);
  assert(sparse_native_requested_bit_value(monitor_raw_on_on[1]) == batch_bits_on_on[5]);

  const std::array<BitValue, 16> batch_bits_off_on {{
      BitValue::Off, BitValue::Off, BitValue::Off, BitValue::Off,
      BitValue::Off, BitValue::On, BitValue::Off, BitValue::Off,
      BitValue::Off, BitValue::Off, BitValue::Off, BitValue::Off,
      BitValue::Off, BitValue::Off, BitValue::Off, BitValue::Off,
  }};
  const std::array<std::uint32_t, 2> random_raw_off_on {{
      0x0020U,
      0x0001U,
  }};
  const std::array<std::uint32_t, 2> monitor_raw_off_on = random_raw_off_on;
  assert(sparse_native_mask_word(random_raw_off_on[0]) == 0x0020U);
  assert(sparse_native_mask_word(random_raw_off_on[1]) == 0x0001U);
  assert(sparse_native_requested_bit_value(random_raw_off_on[0]) == batch_bits_off_on[0]);
  assert(sparse_native_requested_bit_value(random_raw_off_on[1]) == batch_bits_off_on[5]);
  assert(sparse_native_requested_bit_value(monitor_raw_off_on[0]) == batch_bits_off_on[0]);
  assert(sparse_native_requested_bit_value(monitor_raw_off_on[1]) == batch_bits_off_on[5]);
}

void test_parse_multi_block_read_response_ascii_mixed_blocks() {
  const auto config = make_ascii_c4_format4_config();
  const std::array<MultiBlockReadBlock, 3> blocks {{
      {.head_device = {.code = mcprotocol::serial::DeviceCode::D, .number = 100}, .points = 2, .bit_block = false},
      {.head_device = {.code = mcprotocol::serial::DeviceCode::ZR, .number = 200}, .points = 1, .bit_block = false},
      {.head_device = {.code = mcprotocol::serial::DeviceCode::M, .number = 300}, .points = 1, .bit_block = true},
  }};
  const std::array<std::uint8_t, 16> response_data {
      '1', '2', '3', '4',
      'A', 'B', 'C', 'D',
      '0', 'F', '0', 'F',
      'A', '5', '5', 'A',
  };

  std::array<std::uint16_t, 3> words {};
  std::array<BitValue, 16> bits {};
  std::array<MultiBlockReadBlockResult, 3> results {};

  Status status = CommandCodec::parse_multi_block_read_response(
      config,
      std::span<const MultiBlockReadBlock>(blocks.data(), blocks.size()),
      std::span<const std::uint8_t>(response_data.data(), response_data.size()),
      std::span<std::uint16_t>(words.data(), words.size()),
      std::span<BitValue>(bits.data(), bits.size()),
      std::span<MultiBlockReadBlockResult>(results.data(), results.size()));
  assert(status.ok());

  assert(words[0] == 0x1234U);
  assert(words[1] == 0xABCDU);
  assert(words[2] == 0x0F0FU);

  assert(!results[0].bit_block);
  assert(results[0].data_offset == 0U);
  assert(results[0].data_count == 2U);
  assert(!results[1].bit_block);
  assert(results[1].data_offset == 2U);
  assert(results[1].data_count == 1U);
  assert(results[2].bit_block);
  assert(results[2].data_offset == 0U);
  assert(results[2].data_count == 16U);

  const std::array<BitValue, 16> expected_bits {{
      BitValue::Off, BitValue::On, BitValue::Off, BitValue::On,
      BitValue::On, BitValue::Off, BitValue::On, BitValue::Off,
      BitValue::On, BitValue::Off, BitValue::On, BitValue::Off,
      BitValue::Off, BitValue::On, BitValue::Off, BitValue::On,
  }};
  for (std::size_t index = 0; index < bits.size(); ++index) {
    assert(bits[index] == expected_bits[index]);
  }
}

void test_parse_qualified_buffer_word_device_accepts_g_and_hg() {
  QualifiedBufferWordDevice g_device {};
  Status status = parse_qualified_buffer_word_device("U3E0\\G10", g_device);
  assert(status.ok());
  assert(g_device.kind == QualifiedBufferDeviceKind::G);
  assert(g_device.module_number == 0x03E0U);
  assert(g_device.word_address == 10U);

  QualifiedBufferWordDevice hg_device {};
  status = parse_qualified_buffer_word_device("u3e0/hg20", hg_device);
  assert(status.ok());
  assert(hg_device.kind == QualifiedBufferDeviceKind::HG);
  assert(hg_device.module_number == 0x03E0U);
  assert(hg_device.word_address == 20U);
}

void test_make_qualified_buffer_read_words_request_maps_to_module_buffer() {
  const QualifiedBufferWordDevice device {
      .kind = QualifiedBufferDeviceKind::G,
      .module_number = 0x03E0U,
      .word_address = 10U,
  };

  ModuleBufferReadRequest request {};
  const Status status = make_qualified_buffer_read_words_request(device, 4U, request);
  assert(status.ok());
  assert(request.start_address == 20U);
  assert(request.bytes == 8U);
  assert(request.module_number == 0x03E0U);
}

void test_make_qualified_buffer_write_words_request_encodes_little_endian_bytes() {
  const QualifiedBufferWordDevice device {
      .kind = QualifiedBufferDeviceKind::HG,
      .module_number = 0x03E0U,
      .word_address = 20U,
  };
  const std::array<std::uint16_t, 2> words {0x1234U, 0xABCDU};
  std::array<std::byte, 8> byte_storage {};
  ModuleBufferWriteRequest request {};
  std::size_t byte_count = 0U;

  const Status status = make_qualified_buffer_write_words_request(
      device,
      words,
      byte_storage,
      request,
      byte_count);
  assert(status.ok());
  assert(byte_count == 4U);
  assert(request.start_address == 40U);
  assert(request.module_number == 0x03E0U);
  assert(request.bytes.size() == 4U);
  assert(std::to_integer<std::uint8_t>(request.bytes[0]) == 0x34U);
  assert(std::to_integer<std::uint8_t>(request.bytes[1]) == 0x12U);
  assert(std::to_integer<std::uint8_t>(request.bytes[2]) == 0xCDU);
  assert(std::to_integer<std::uint8_t>(request.bytes[3]) == 0xABU);
}

void test_decode_qualified_buffer_word_values_decodes_little_endian_bytes() {
  const std::array<std::byte, 4> bytes {
      std::byte {0x34},
      std::byte {0x12},
      std::byte {0xCD},
      std::byte {0xAB},
  };
  std::array<std::uint16_t, 2> words {};

  const Status status = decode_qualified_buffer_word_values(bytes, words);
  assert(status.ok());
  assert(words[0] == 0x1234U);
  assert(words[1] == 0xABCDU);
}

struct CallbackCapture {
  bool called = false;
  Status status {};
};

void completion_callback(void* user, Status status) {
  auto* capture = static_cast<CallbackCapture*>(user);
  capture->called = true;
  capture->status = status;
}

void test_client_binary_cpu_model_roundtrip() {
  const auto config = make_binary_c4_config();
  MelsecSerialClient client;
  Status status = client.configure(config);
  assert(status.ok());

  CpuModelInfo info;
  CallbackCapture capture;
  status = client.async_read_cpu_model(0, info, completion_callback, &capture);
  assert(status.ok());
  assert(client.busy());
  assert(!client.pending_tx_frame().empty());

  status = client.notify_tx_complete(10);
  assert(status.ok());

  const std::array<std::uint8_t, 18> response_data {
      'Q', '0', '2', 'H', 'C', 'P', 'U', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
      0x41, 0x00,
  };
  std::array<std::uint8_t, 128> response_frame {};
  std::size_t response_frame_size = 0;
  status = FrameCodec::encode_success_response(config, response_data, response_frame, response_frame_size);
  assert(status.ok());

  client.on_rx_bytes(
      20,
      std::span<const std::byte>(
          reinterpret_cast<const std::byte*>(response_frame.data()),
          7));
  assert(!capture.called);

  client.on_rx_bytes(
      25,
      std::span<const std::byte>(
          reinterpret_cast<const std::byte*>(response_frame.data() + 7),
          response_frame_size - 7));
  assert(capture.called);
  assert(capture.status.ok());
  assert(std::string_view(info.model_name.data()) == "Q02HCPU");
  assert(info.model_code == 0x0041);
  assert(!client.busy());
}

void test_client_binary_read_user_frame_roundtrip() {
  const auto config = make_binary_c4_config();
  MelsecSerialClient client;
  Status status = client.configure(config);
  assert(status.ok());

  UserFrameRegistrationData out_data {};
  CallbackCapture capture;
  status = client.async_read_user_frame(
      0,
      UserFrameReadRequest {.frame_no = 0x03E8U},
      out_data,
      completion_callback,
      &capture);
  assert(status.ok());
  status = client.notify_tx_complete(1);
  assert(status.ok());

  const std::array<std::uint8_t, 9> response_data {
      0x05U, 0x00U, 0x04U, 0x00U, 0x03U, 0xFFU, 0xF1U, 0x0DU, 0x0AU,
  };
  std::array<std::uint8_t, 128> response_frame {};
  std::size_t response_frame_size = 0;
  status = FrameCodec::encode_success_response(config, response_data, response_frame, response_frame_size);
  assert(status.ok());

  client.on_rx_bytes(
      2,
      std::span<const std::byte>(
          reinterpret_cast<const std::byte*>(response_frame.data()),
          8U));
  assert(!capture.called);
  client.on_rx_bytes(
      3,
      std::span<const std::byte>(
          reinterpret_cast<const std::byte*>(response_frame.data() + 8U),
          response_frame_size - 8U));
  assert(capture.called);
  assert(capture.status.ok());
  assert(out_data.registration_data_bytes == 5U);
  assert(out_data.frame_bytes == 4U);
  const std::array<std::byte, 5> expected {
      std::byte {0x03}, std::byte {0xFF}, std::byte {0xF1}, std::byte {0x0D}, std::byte {0x0A},
  };
  assert(std::memcmp(out_data.registration_data.data(), expected.data(), expected.size()) == 0);
  assert(!client.busy());
}

void test_client_binary_write_user_frame_roundtrip() {
  const auto config = make_binary_c4_config();
  MelsecSerialClient client;
  Status status = client.configure(config);
  assert(status.ok());

  const std::array<std::byte, 5> registration_data {
      std::byte {0x03}, std::byte {0xFF}, std::byte {0xF1}, std::byte {0x0D}, std::byte {0x0A},
  };
  CallbackCapture capture;
  status = client.async_write_user_frame(
      0,
      UserFrameWriteRequest {
          .frame_no = 0x03E8U,
          .frame_bytes = 4U,
          .registration_data = registration_data,
      },
      completion_callback,
      &capture);
  assert(status.ok());
  status = client.notify_tx_complete(1);
  assert(status.ok());

  std::array<std::uint8_t, 64> response_frame {};
  std::size_t response_frame_size = 0;
  status = FrameCodec::encode_success_response(config, {}, response_frame, response_frame_size);
  assert(status.ok());

  client.on_rx_bytes(
      2,
      std::span<const std::byte>(
          reinterpret_cast<const std::byte*>(response_frame.data()),
          response_frame_size));
  assert(capture.called);
  assert(capture.status.ok());
  assert(!client.busy());
}

void test_client_binary_e1_batch_read_words_roundtrip() {
  const auto config = make_binary_e1_a_config();
  MelsecSerialClient client;
  Status status = client.configure(config);
  assert(status.ok());

  std::array<std::uint16_t, 2> values {};
  CallbackCapture capture;
  status = client.async_batch_read_words(
      0,
      BatchReadWordsRequest {
          .head_device = {.code = mcprotocol::serial::DeviceCode::D, .number = 100U},
          .points = 2U,
      },
      values,
      completion_callback,
      &capture);
  assert(status.ok());
  status = client.notify_tx_complete(1);
  assert(status.ok());

  const std::array<std::uint8_t, 6> response_frame {
      0x81U, 0x00U, 0x34U, 0x12U, 0x78U, 0x56U,
  };
  client.on_rx_bytes(
      2,
      std::span<const std::byte>(
          reinterpret_cast<const std::byte*>(response_frame.data()),
          2U));
  assert(!capture.called);
  client.on_rx_bytes(
      3,
      std::span<const std::byte>(
          reinterpret_cast<const std::byte*>(response_frame.data() + 2U),
          response_frame.size() - 2U));
  assert(capture.called);
  assert(capture.status.ok());
  assert(values[0] == 0x1234U);
  assert(values[1] == 0x5678U);
  assert(!client.busy());
}

void test_client_ascii_e1_batch_read_bits_odd_roundtrip() {
  const auto config = make_ascii_e1_a_config();
  MelsecSerialClient client;
  Status status = client.configure(config);
  assert(status.ok());

  std::array<BitValue, 3> bits {};
  CallbackCapture capture;
  status = client.async_batch_read_bits(
      0,
      BatchReadBitsRequest {
          .head_device = {.code = mcprotocol::serial::DeviceCode::M, .number = 100U},
          .points = 3U,
      },
      bits,
      completion_callback,
      &capture);
  assert(status.ok());
  status = client.notify_tx_complete(1);
  assert(status.ok());

  constexpr std::string_view response = "80001010";
  client.on_rx_bytes(
      2,
      std::span<const std::byte>(
          reinterpret_cast<const std::byte*>(response.data()),
          3U));
  assert(!capture.called);
  client.on_rx_bytes(
      3,
      std::span<const std::byte>(
          reinterpret_cast<const std::byte*>(response.data() + 3U),
          response.size() - 3U));
  assert(capture.called);
  assert(capture.status.ok());
  assert(bits[0] == BitValue::On);
  assert(bits[1] == BitValue::Off);
  assert(bits[2] == BitValue::On);
  assert(!client.busy());
}

void test_client_e1_rejects_cpu_model() {
  const auto config = make_binary_e1_a_config();
  MelsecSerialClient client;
  Status status = client.configure(config);
  assert(status.ok());

  CpuModelInfo info;
  CallbackCapture capture;
  status = client.async_read_cpu_model(0, info, completion_callback, &capture);
  assert(!status.ok());
  assert(status.code == StatusCode::UnsupportedConfiguration);
}

void test_client_ascii_c1_loopback_roundtrip() {
  const auto config = make_ascii_c1_format4_qna_config();
  MelsecSerialClient client;
  Status status = client.configure(config);
  assert(status.ok());

  std::array<char, 8> echoed {};
  CallbackCapture capture;
  constexpr std::string_view loopback = "ABCDE";
  status = client.async_loopback(
      0,
      std::span<const char>(loopback.data(), loopback.size()),
      echoed,
      completion_callback,
      &capture);
  assert(status.ok());
  status = client.notify_tx_complete(1);
  assert(status.ok());

  const std::array<std::uint8_t, 7> response_data {'0', '5', 'A', 'B', 'C', 'D', 'E'};
  std::array<std::uint8_t, 64> response_frame {};
  std::size_t response_frame_size = 0;
  status = FrameCodec::encode_success_response(config, response_data, response_frame, response_frame_size);
  assert(status.ok());
  client.on_rx_bytes(
      2,
      std::span<const std::byte>(
          reinterpret_cast<const std::byte*>(response_frame.data()),
          6U));
  assert(!capture.called);
  client.on_rx_bytes(
      3,
      std::span<const std::byte>(
          reinterpret_cast<const std::byte*>(response_frame.data() + 6U),
          response_frame_size - 6U));
  assert(capture.called);
  assert(capture.status.ok());
  assert(std::string_view(echoed.data(), 5U) == "ABCDE");
  assert(!client.busy());
}

void test_client_binary_e1_extended_file_register_monitor_roundtrip() {
  const auto config = make_binary_e1_a_config();
  MelsecSerialClient client;
  Status status = client.configure(config);
  assert(status.ok());

  const std::array<ExtendedFileRegisterAddress, 2> items {{
      {.block_number = 2U, .word_number = 70U},
      {.block_number = 3U, .word_number = 71U},
  }};
  CallbackCapture register_capture;
  status = client.async_register_extended_file_register_monitor(
      0,
      ExtendedFileRegisterMonitorRegistration {.items = items},
      completion_callback,
      &register_capture);
  assert(status.ok());
  status = client.notify_tx_complete(1);
  assert(status.ok());

  const std::array<std::uint8_t, 2> register_response {0x9AU, 0x00U};
  client.on_rx_bytes(
      2,
      std::span<const std::byte>(
          reinterpret_cast<const std::byte*>(register_response.data()),
          1U));
  assert(!register_capture.called);
  client.on_rx_bytes(
      3,
      std::span<const std::byte>(
          reinterpret_cast<const std::byte*>(register_response.data() + 1U),
          1U));
  assert(register_capture.called);
  assert(register_capture.status.ok());

  std::array<std::uint16_t, 2> values {};
  CallbackCapture read_capture;
  status = client.async_read_extended_file_register_monitor(
      4,
      values,
      completion_callback,
      &read_capture);
  assert(status.ok());
  status = client.notify_tx_complete(5);
  assert(status.ok());

  const std::array<std::uint8_t, 6> read_response {0x9BU, 0x00U, 0x34U, 0x12U, 0x78U, 0x56U};
  client.on_rx_bytes(
      6,
      std::span<const std::byte>(
          reinterpret_cast<const std::byte*>(read_response.data()),
          2U));
  assert(!read_capture.called);
  client.on_rx_bytes(
      7,
      std::span<const std::byte>(
          reinterpret_cast<const std::byte*>(read_response.data() + 2U),
          read_response.size() - 2U));
  assert(read_capture.called);
  assert(read_capture.status.ok());
  assert(values[0] == 0x1234U);
  assert(values[1] == 0x5678U);
  assert(!client.busy());
}

void test_client_remote_reset_roundtrip() {
  const auto config = make_binary_c4_config();
  MelsecSerialClient client;
  Status status = client.configure(config);
  assert(status.ok());

  CallbackCapture capture;
  status = client.async_remote_reset(0, completion_callback, &capture);
  assert(status.ok());
  assert(client.busy());

  status = client.notify_tx_complete(10);
  assert(status.ok());

  std::array<std::uint8_t, 64> response_frame {};
  std::size_t response_frame_size = 0;
  status = FrameCodec::encode_success_response(config, {}, response_frame, response_frame_size);
  assert(status.ok());

  client.on_rx_bytes(
      20,
      std::span<const std::byte>(
          reinterpret_cast<const std::byte*>(response_frame.data()),
          response_frame_size));
  assert(capture.called);
  assert(capture.status.ok());
  assert(std::string_view(capture.status.message) == "ok");
  assert(!client.busy());
}

void test_client_remote_control_and_password_roundtrips() {
  const auto config = make_binary_c4_iqr_config();
  std::array<std::uint8_t, 64> response_frame {};
  std::size_t response_frame_size = 0;
  Status status = FrameCodec::encode_success_response(config, {}, response_frame, response_frame_size);
  assert(status.ok());

  {
    MelsecSerialClient client;
    status = client.configure(config);
    assert(status.ok());
    CallbackCapture capture;
    status = client.async_remote_run(
        0,
        mcprotocol::serial::RemoteOperationMode::DoNotExecuteForcibly,
        mcprotocol::serial::RemoteRunClearMode::DoNotClear,
        completion_callback,
        &capture);
    assert(status.ok());
    status = client.notify_tx_complete(1);
    assert(status.ok());
    client.on_rx_bytes(
        2,
        std::span<const std::byte>(
            reinterpret_cast<const std::byte*>(response_frame.data()),
            response_frame_size));
    assert(capture.called);
    assert(capture.status.ok());
  }
  {
    MelsecSerialClient client;
    status = client.configure(config);
    assert(status.ok());
    CallbackCapture capture;
    status = client.async_remote_stop(0, completion_callback, &capture);
    assert(status.ok());
    status = client.notify_tx_complete(1);
    assert(status.ok());
    client.on_rx_bytes(
        2,
        std::span<const std::byte>(
            reinterpret_cast<const std::byte*>(response_frame.data()),
            response_frame_size));
    assert(capture.called);
    assert(capture.status.ok());
  }
  {
    MelsecSerialClient client;
    status = client.configure(config);
    assert(status.ok());
    CallbackCapture capture;
    status = client.async_remote_pause(
        0,
        mcprotocol::serial::RemoteOperationMode::ExecuteForcibly,
        completion_callback,
        &capture);
    assert(status.ok());
    status = client.notify_tx_complete(1);
    assert(status.ok());
    client.on_rx_bytes(
        2,
        std::span<const std::byte>(
            reinterpret_cast<const std::byte*>(response_frame.data()),
            response_frame_size));
    assert(capture.called);
    assert(capture.status.ok());
  }
  {
    MelsecSerialClient client;
    status = client.configure(config);
    assert(status.ok());
    CallbackCapture capture;
    status = client.async_remote_latch_clear(0, completion_callback, &capture);
    assert(status.ok());
    status = client.notify_tx_complete(1);
    assert(status.ok());
    client.on_rx_bytes(
        2,
        std::span<const std::byte>(
            reinterpret_cast<const std::byte*>(response_frame.data()),
            response_frame_size));
    assert(capture.called);
    assert(capture.status.ok());
  }
  {
    MelsecSerialClient client;
    status = client.configure(config);
    assert(status.ok());
    CallbackCapture capture;
    status = client.async_unlock_remote_password(0, "abcdef", completion_callback, &capture);
    assert(status.ok());
    status = client.notify_tx_complete(1);
    assert(status.ok());
    client.on_rx_bytes(
        2,
        std::span<const std::byte>(
            reinterpret_cast<const std::byte*>(response_frame.data()),
            response_frame_size));
    assert(capture.called);
    assert(capture.status.ok());
  }
  {
    MelsecSerialClient client;
    status = client.configure(config);
    assert(status.ok());
    CallbackCapture capture;
    status = client.async_lock_remote_password(0, "abcdef", completion_callback, &capture);
    assert(status.ok());
    status = client.notify_tx_complete(1);
    assert(status.ok());
    client.on_rx_bytes(
        2,
        std::span<const std::byte>(
            reinterpret_cast<const std::byte*>(response_frame.data()),
            response_frame_size));
    assert(capture.called);
    assert(capture.status.ok());
  }
}

void test_client_clear_error_information_roundtrip() {
  const auto config = make_binary_c4_iqr_config();
  MelsecSerialClient client;
  Status status = client.configure(config);
  assert(status.ok());

  CallbackCapture capture;
  status = client.async_clear_error_information(0, completion_callback, &capture);
  assert(status.ok());
  assert(client.busy());

  status = client.notify_tx_complete(10);
  assert(status.ok());

  std::array<std::uint8_t, 64> response_frame {};
  std::size_t response_frame_size = 0;
  status = FrameCodec::encode_success_response(config, {}, response_frame, response_frame_size);
  assert(status.ok());

  client.on_rx_bytes(
      20,
      std::span<const std::byte>(
          reinterpret_cast<const std::byte*>(response_frame.data()),
          response_frame_size));
  assert(capture.called);
  assert(capture.status.ok());
  assert(std::string_view(capture.status.message) == "ok");
  assert(!client.busy());
}

void test_client_c24_small_command_roundtrips() {
  const auto config = make_binary_c4_config();
  std::array<std::uint8_t, 64> response_frame {};
  std::size_t response_frame_size = 0;
  Status status = FrameCodec::encode_success_response(config, {}, response_frame, response_frame_size);
  assert(status.ok());

  {
    MelsecSerialClient client;
    status = client.configure(config);
    assert(status.ok());
    CallbackCapture capture;
    status = client.async_control_global_signal(
        0,
        GlobalSignalControlRequest {
            .target = GlobalSignalTarget::X1B,
            .turn_on = false,
            .station_no = 3,
        },
        completion_callback,
        &capture);
    assert(status.ok());
    status = client.notify_tx_complete(1);
    assert(status.ok());
    client.on_rx_bytes(
        2,
        std::span<const std::byte>(
            reinterpret_cast<const std::byte*>(response_frame.data()),
            response_frame_size));
    assert(capture.called);
    assert(capture.status.ok());
  }
  {
    MelsecSerialClient client;
    status = client.configure(config);
    assert(status.ok());
    CallbackCapture capture;
    status = client.async_initialize_c24_transmission_sequence(0, completion_callback, &capture);
    assert(status.ok());
    status = client.notify_tx_complete(1);
    assert(status.ok());
    client.on_rx_bytes(
        2,
        std::span<const std::byte>(
            reinterpret_cast<const std::byte*>(response_frame.data()),
            response_frame_size));
    assert(capture.called);
    assert(capture.status.ok());
  }
  {
    MelsecSerialClient client;
    status = client.configure(config);
    assert(status.ok());
    CallbackCapture capture;
    status = client.async_deregister_cpu_monitoring(0, completion_callback, &capture);
    assert(status.ok());
    status = client.notify_tx_complete(1);
    assert(status.ok());
    client.on_rx_bytes(
        2,
        std::span<const std::byte>(
            reinterpret_cast<const std::byte*>(response_frame.data()),
            response_frame_size));
    assert(capture.called);
    assert(capture.status.ok());
  }
}

void test_client_remote_reset_timeout_without_response_is_success() {
  auto config = make_binary_c4_config();
  config.timeout.response_timeout_ms = 5;

  MelsecSerialClient client;
  Status status = client.configure(config);
  assert(status.ok());

  CallbackCapture capture;
  status = client.async_remote_reset(0, completion_callback, &capture);
  assert(status.ok());

  status = client.notify_tx_complete(0);
  assert(status.ok());

  client.poll(6);
  assert(capture.called);
  assert(capture.status.ok());
  assert(std::string_view(capture.status.message) == "Remote RESET completed without a response");
  assert(!client.busy());
}

void test_client_init_sequence_timeout_without_response_is_success() {
  auto config = make_binary_c4_config();
  config.timeout.response_timeout_ms = 5;

  MelsecSerialClient client;
  Status status = client.configure(config);
  assert(status.ok());

  CallbackCapture capture;
  status = client.async_initialize_c24_transmission_sequence(0, completion_callback, &capture);
  assert(status.ok());
  assert(client.busy());

  status = client.notify_tx_complete(1);
  assert(status.ok());

  client.poll(10);
  assert(capture.called);
  assert(capture.status.ok());
  assert(
      std::string_view(capture.status.message) ==
      "Transmission-sequence initialization completed without a response");
  assert(!client.busy());
}

void test_client_global_signal_timeout_without_response_is_success() {
  auto config = make_binary_c4_config();
  config.timeout.response_timeout_ms = 5;

  MelsecSerialClient client;
  Status status = client.configure(config);
  assert(status.ok());

  CallbackCapture capture;
  status = client.async_control_global_signal(
      0,
      GlobalSignalControlRequest {
          .target = GlobalSignalTarget::ReceivedSide,
          .turn_on = true,
          .station_no = 0,
      },
      completion_callback,
      &capture);
  assert(status.ok());
  assert(client.busy());

  status = client.notify_tx_complete(1);
  assert(status.ok());

  client.poll(10);
  assert(capture.called);
  assert(capture.status.ok());
  assert(std::string_view(capture.status.message) == "Global signal control completed without a response");
  assert(!client.busy());
}

void test_client_timeout() {
  const auto config = make_binary_c4_config();
  MelsecSerialClient client;
  Status status = client.configure(config);
  assert(status.ok());

  CpuModelInfo info;
  CallbackCapture capture;
  status = client.async_read_cpu_model(0, info, completion_callback, &capture);
  assert(status.ok());

  status = client.notify_tx_complete(0);
  assert(status.ok());

  client.poll(config.timeout.response_timeout_ms + 1);
  assert(capture.called);
  assert(capture.status.code == StatusCode::Timeout);
}

void test_client_ascii_format4_resynchronizes_on_stale_ack() {
  const auto config = make_ascii_c4_format4_config();
  MelsecSerialClient client;
  Status status = client.configure(config);
  assert(status.ok());

  const std::array<std::uint16_t, 1> values {0x1234U};
  CallbackCapture capture;
  status = client.async_batch_write_words(
      0,
      BatchWriteWordsRequest {
          .head_device = {.code = mcprotocol::serial::DeviceCode::D, .number = 100},
          .words = values,
      },
      completion_callback,
      &capture);
  assert(status.ok());

  status = client.notify_tx_complete(1);
  assert(status.ok());

  std::array<std::uint8_t, 32> ack_frame {};
  std::size_t ack_size = 0;
  status = FrameCodec::encode_success_response(config, {}, ack_frame, ack_size);
  assert(status.ok());

  std::array<std::uint8_t, 64> noisy_frame {};
  noisy_frame[0] = 0x06U;
  std::memcpy(noisy_frame.data() + 1U, ack_frame.data(), ack_size);
  client.on_rx_bytes(
      2,
      std::span<const std::byte>(
          reinterpret_cast<const std::byte*>(noisy_frame.data()),
          ack_size + 1U));

  assert(capture.called);
  assert(capture.status.ok());
  assert(!client.busy());
}

void test_client_write_rejects_unexpected_success_data() {
  const auto config = make_ascii_c4_format4_config();
  MelsecSerialClient client;
  Status status = client.configure(config);
  assert(status.ok());

  const std::array<std::uint16_t, 1> values {0x1234U};
  CallbackCapture capture;
  status = client.async_batch_write_words(
      0,
      BatchWriteWordsRequest {
          .head_device = {.code = mcprotocol::serial::DeviceCode::D, .number = 100},
          .words = values,
      },
      completion_callback,
      &capture);
  assert(status.ok());

  status = client.notify_tx_complete(1);
  assert(status.ok());

  const std::array<std::uint8_t, 4> unexpected_data {'0', '0', '0', '0'};
  std::array<std::uint8_t, 64> response_frame {};
  std::size_t response_size = 0;
  status = FrameCodec::encode_success_response(config, unexpected_data, response_frame, response_size);
  assert(status.ok());

  client.on_rx_bytes(
      2,
      std::span<const std::byte>(
          reinterpret_cast<const std::byte*>(response_frame.data()),
          response_size));

  assert(capture.called);
  assert(capture.status.code == StatusCode::Parse);
  assert(std::string_view(capture.status.message) == "Write response must not contain response data");
  assert(!client.busy());
}

void test_client_link_direct_random_read_roundtrip() {
  const auto config = make_binary_c4_iqr_config();
  MelsecSerialClient client;
  Status status = client.configure(config);
  assert(status.ok());

  const std::array<LinkDirectRandomReadItem, 2> items {{
      {.device = {.network_number = 0x0001U, .device = {.code = mcprotocol::serial::DeviceCode::W, .number = 0x0100U}}},
      {.device = {.network_number = 0x0001U, .device = {.code = mcprotocol::serial::DeviceCode::B, .number = 0x0010U}}},
  }};
  std::array<std::uint32_t, 2> values {};
  CallbackCapture capture;

  status = client.async_link_direct_random_read(
      0,
      std::span<const LinkDirectRandomReadItem>(items.data(), items.size()),
      values,
      completion_callback,
      &capture);
  assert(status.ok());
  status = client.notify_tx_complete(1);
  assert(status.ok());

  const std::array<std::uint8_t, 4> response_data {0x34, 0x12, 0x01, 0x00};
  std::array<std::uint8_t, 64> response_frame {};
  std::size_t response_frame_size = 0;
  status = FrameCodec::encode_success_response(config, response_data, response_frame, response_frame_size);
  assert(status.ok());

  client.on_rx_bytes(
      2,
      std::span<const std::byte>(
          reinterpret_cast<const std::byte*>(response_frame.data()),
          response_frame_size));
  assert(capture.called);
  assert(capture.status.ok());
  assert(values[0] == 0x1234U);
  assert(values[1] == 0x0001U);
  assert(!client.busy());
}

void test_client_link_direct_register_monitor_roundtrip() {
  const auto config = make_binary_c4_iqr_config();
  MelsecSerialClient client;
  Status status = client.configure(config);
  assert(status.ok());

  const std::array<LinkDirectRandomReadItem, 2> items {{
      {.device = {.network_number = 0x0001U, .device = {.code = mcprotocol::serial::DeviceCode::W, .number = 0x0100U}}},
      {.device = {.network_number = 0x0001U, .device = {.code = mcprotocol::serial::DeviceCode::B, .number = 0x0010U}}},
  }};
  CallbackCapture register_capture;
  status = client.async_link_direct_register_monitor(
      0,
      LinkDirectMonitorRegistration {
          .items = std::span<const LinkDirectRandomReadItem>(items.data(), items.size()),
      },
      completion_callback,
      &register_capture);
  assert(status.ok());
  status = client.notify_tx_complete(1);
  assert(status.ok());

  std::array<std::uint8_t, 32> register_frame {};
  std::size_t register_frame_size = 0;
  status = FrameCodec::encode_success_response(config, {}, register_frame, register_frame_size);
  assert(status.ok());
  client.on_rx_bytes(
      2,
      std::span<const std::byte>(
          reinterpret_cast<const std::byte*>(register_frame.data()),
          register_frame_size));
  assert(register_capture.called);
  assert(register_capture.status.ok());

  std::array<std::uint32_t, 2> values {};
  CallbackCapture read_capture;
  status = client.async_read_monitor(10, values, completion_callback, &read_capture);
  assert(status.ok());
  status = client.notify_tx_complete(11);
  assert(status.ok());

  const std::array<std::uint8_t, 4> response_data {0x78, 0x56, 0x01, 0x00};
  std::array<std::uint8_t, 64> response_frame {};
  std::size_t response_frame_size = 0;
  status = FrameCodec::encode_success_response(config, response_data, response_frame, response_frame_size);
  assert(status.ok());
  client.on_rx_bytes(
      12,
      std::span<const std::byte>(
          reinterpret_cast<const std::byte*>(response_frame.data()),
          response_frame_size));
  assert(read_capture.called);
  assert(read_capture.status.ok());
  assert(values[0] == 0x5678U);
  assert(values[1] == 0x0001U);
  assert(!client.busy());
}

// Validate that 0403 random read rejects long timer/counter contact/coil devices
// (LTS, LTC, LSTS, LSTC, LCS, LCC). These are not allowed per the serial manual restriction.
void test_encode_random_read_rejects_long_contact_coil_devices() {
  const auto config = make_binary_c4_config();
  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;

  const mcprotocol::serial::DeviceCode excluded[] = {
      mcprotocol::serial::DeviceCode::LTS,
      mcprotocol::serial::DeviceCode::LTC,
      mcprotocol::serial::DeviceCode::LSTS,
      mcprotocol::serial::DeviceCode::LSTC,
      mcprotocol::serial::DeviceCode::LCS,
      mcprotocol::serial::DeviceCode::LCC,
  };
  for (const auto code : excluded) {
    const RandomReadItem item {.device = {.code = code, .number = 0}, .double_word = false};
    const Status status = CommandCodec::encode_random_read(
        config,
        mcprotocol::serial::RandomReadRequest {
            .items = std::span<const mcprotocol::serial::RandomReadItem>(&item, 1),
        },
        request_data,
        request_size);
    assert(!status.ok());
    assert(status.code == StatusCode::InvalidArgument);
  }
}

// Validate that 1402 random write words supports LTN/LSTN as double-word devices on iQ-R.
void test_encode_random_write_words_allows_ltn_and_lstn() {
  const auto config = make_binary_c4_iqr_config();
  std::array<std::uint8_t, 32> request_data {};
  std::size_t request_size = 0;

  struct ExpectedCase {
    mcprotocol::serial::DeviceCode code;
    std::array<std::uint8_t, 16> expected;
  };
  const std::array<ExpectedCase, 2> cases {{
      {.code = mcprotocol::serial::DeviceCode::LTN,
       .expected = {0x02, 0x14, 0x02, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x52, 0x00, 0x40, 0xE2, 0x01, 0x00}},
      {.code = mcprotocol::serial::DeviceCode::LSTN,
       .expected = {0x02, 0x14, 0x02, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x5A, 0x00, 0x47, 0x94, 0x03, 0x00}},
  }};
  for (const auto& entry : cases) {
    const RandomWriteWordItem item {
        .device = {.code = entry.code, .number = 0},
        .value = entry.code == mcprotocol::serial::DeviceCode::LTN ? 123456U : 234567U,
        .double_word = true,
    };
    const Status status = CommandCodec::encode_random_write_words(
        config,
        std::span<const RandomWriteWordItem>(&item, 1),
        request_data,
        request_size);
    assert(status.ok());
    assert(request_size == entry.expected.size());
    assert(std::memcmp(request_data.data(), entry.expected.data(), entry.expected.size()) == 0);
  }
}

// Validate that 1402 random write words rejects long timer/counter contact/coil devices
// (LTS, LTC, LSTS, LSTC, LCS, LCC).
void test_encode_random_write_words_rejects_long_contact_coil_devices() {
  const auto config = make_binary_c4_config();
  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;

  const mcprotocol::serial::DeviceCode excluded[] = {
      mcprotocol::serial::DeviceCode::LTS,
      mcprotocol::serial::DeviceCode::LTC,
      mcprotocol::serial::DeviceCode::LSTS,
      mcprotocol::serial::DeviceCode::LSTC,
      mcprotocol::serial::DeviceCode::LCS,
      mcprotocol::serial::DeviceCode::LCC,
  };
  for (const auto code : excluded) {
    const RandomWriteWordItem item {.device = {.code = code, .number = 0}, .value = 0, .double_word = false};
    const Status status = CommandCodec::encode_random_write_words(
        config,
        std::span<const RandomWriteWordItem>(&item, 1),
        request_data,
        request_size);
    assert(!status.ok());
    assert(status.code == StatusCode::InvalidArgument);
  }
}

// Validate that 1402 random write bits supports the long timer/counter contact/coil devices
// that the serial manual lists for random bit write and rejects only LCS.
void test_encode_random_write_bits_long_device_rules() {
  const auto config = make_binary_c4_iqr_config();
  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;

  const mcprotocol::serial::DeviceCode allowed[] = {
      mcprotocol::serial::DeviceCode::LTS,
      mcprotocol::serial::DeviceCode::LTC,
      mcprotocol::serial::DeviceCode::LSTS,
      mcprotocol::serial::DeviceCode::LSTC,
      mcprotocol::serial::DeviceCode::LCC,
  };
  for (const auto code : allowed) {
    const RandomWriteBitItem item {.device = {.code = code, .number = 0}, .value = BitValue::Off};
    const Status status = CommandCodec::encode_random_write_bits(
        config,
        std::span<const RandomWriteBitItem>(&item, 1),
        request_data,
        request_size);
    assert(status.ok());
  }

  const RandomWriteBitItem rejected {.device = {.code = mcprotocol::serial::DeviceCode::LCS, .number = 0},
                                     .value = BitValue::Off};
  const Status rejected_status = CommandCodec::encode_random_write_bits(
      config,
      std::span<const RandomWriteBitItem>(&rejected, 1),
      request_data,
      request_size);
  assert(!rejected_status.ok());
  assert(rejected_status.code == StatusCode::InvalidArgument);
}

void test_encode_random_write_bits_binary_iqr_lcc_layout() {
  const auto config = make_binary_c4_iqr_config();
  const std::array<RandomWriteBitItem, 1> items {{
      {.device = {.code = mcprotocol::serial::DeviceCode::LCC, .number = 10}, .value = BitValue::On},
  }};

  std::array<std::uint8_t, 32> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_random_write_bits(
      config,
      std::span<const RandomWriteBitItem>(items.data(), items.size()),
      request_data,
      request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 13> expected {
      0x02, 0x14, 0x03, 0x00, 0x01,
      0x0A, 0x00, 0x00, 0x00, 0x54, 0x00, 0x01, 0x00,
  };
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

// Validate that 0406 multi-block read rejects all long timer/counter/index devices
// as head devices. Per the serial manual: LTS/LTC/LTN/LSTS/LSTC/LSTN/LCS/LCC/LCN/LZ all excluded.
void test_encode_multi_block_read_rejects_long_devices_as_head() {
  const auto config = make_binary_c4_config();
  std::array<std::uint8_t, 256> request_data {};
  std::size_t request_size = 0;

  const mcprotocol::serial::DeviceCode excluded[] = {
      mcprotocol::serial::DeviceCode::LTS,
      mcprotocol::serial::DeviceCode::LTC,
      mcprotocol::serial::DeviceCode::LTN,
      mcprotocol::serial::DeviceCode::LSTS,
      mcprotocol::serial::DeviceCode::LSTC,
      mcprotocol::serial::DeviceCode::LSTN,
      mcprotocol::serial::DeviceCode::LCS,
      mcprotocol::serial::DeviceCode::LCC,
      mcprotocol::serial::DeviceCode::LCN,
      mcprotocol::serial::DeviceCode::LZ,
  };
  for (const auto code : excluded) {
    const MultiBlockReadBlock block {.head_device = {.code = code, .number = 0}, .points = 1, .bit_block = false};
    const Status status = CommandCodec::encode_multi_block_read(
        config,
        mcprotocol::serial::MultiBlockReadRequest {
            .blocks = std::span<const MultiBlockReadBlock>(&block, 1),
        },
        request_data,
        request_size);
    assert(!status.ok());
    assert(status.code == StatusCode::InvalidArgument);
  }
}

// Validate that 1406 multi-block write rejects all long timer/counter/index devices
// as head devices. Per the serial manual: LTS/LTC/LTN/LSTS/LSTC/LSTN/LCS/LCC/LCN/LZ all excluded.
void test_encode_multi_block_write_rejects_long_devices_as_head() {
  const auto config = make_binary_c4_config();
  std::array<std::uint8_t, 256> request_data {};
  std::size_t request_size = 0;

  const mcprotocol::serial::DeviceCode excluded[] = {
      mcprotocol::serial::DeviceCode::LTS,
      mcprotocol::serial::DeviceCode::LTC,
      mcprotocol::serial::DeviceCode::LTN,
      mcprotocol::serial::DeviceCode::LSTS,
      mcprotocol::serial::DeviceCode::LSTC,
      mcprotocol::serial::DeviceCode::LSTN,
      mcprotocol::serial::DeviceCode::LCS,
      mcprotocol::serial::DeviceCode::LCC,
      mcprotocol::serial::DeviceCode::LCN,
      mcprotocol::serial::DeviceCode::LZ,
  };
  const std::array<std::uint16_t, 1> dummy_words {0};
  for (const auto code : excluded) {
    const MultiBlockWriteBlock block {
        .head_device = {.code = code, .number = 0},
        .points = 1,
        .bit_block = false,
        .words = std::span<const std::uint16_t>(dummy_words.data(), 1),
    };
    const Status status = CommandCodec::encode_multi_block_write(
        config,
        mcprotocol::serial::MultiBlockWriteRequest {
            .blocks = std::span<const MultiBlockWriteBlock>(&block, 1),
        },
        request_data,
        request_size);
    assert(!status.ok());
    assert(status.code == StatusCode::InvalidArgument);
  }
}

}  // namespace

int main() {
  test_format5_batch_read_request_matches_manual();
  test_decode_binary_cpu_model_response();
  test_encode_remote_run_binary_request();
  test_encode_remote_stop_binary_request();
  test_encode_remote_pause_binary_request();
  test_encode_remote_latch_clear_binary_request();
  test_encode_remote_reset_binary_request();
  test_encode_unlock_remote_password_binary_q_l_request();
  test_encode_lock_remote_password_binary_q_l_request();
  test_encode_unlock_remote_password_rejects_invalid_lengths();
  test_encode_clear_error_information_binary_q_l_request();
  test_encode_clear_error_information_binary_iqr_request();
  test_encode_initialize_transmission_sequence_binary_request();
  test_encode_initialize_transmission_sequence_rejects_ascii();
  test_encode_control_global_signal_binary_request();
  test_encode_deregister_cpu_monitoring_binary_request();
  test_decode_ascii_loopback_response();
  test_encode_ascii_read_user_frame_request_shape();
  test_parse_ascii_read_user_frame_response();
  test_parse_binary_read_user_frame_response_accepts_zero_frame_bytes();
  test_encode_binary_write_user_frame_request_shape();
  test_encode_binary_delete_user_frame_request_shape();
  test_validate_ascii_c2_config_and_reject_binary();
  test_validate_ascii_c1_config_and_reject_binary();
  test_encode_ascii_format2_request_inserts_block_number();
  test_encode_ascii_c2_format2_request_uses_short_route_without_frame_id();
  test_decode_ascii_format2_ack_response();
  test_decode_ascii_c2_format2_two_digit_error_response();
  test_encode_ascii_c2_format3_request_uses_short_route_without_frame_id();
  test_decode_ascii_c2_format3_data_response();
  test_decode_ascii_c2_format3_two_digit_error_response();
  test_encode_ascii_c1_batch_read_words_qna_request_shape();
  test_encode_ascii_c1_batch_read_bits_a_request_shape();
  test_encode_ascii_c1_batch_write_words_qna_request_shape();
  test_decode_ascii_c1_ack_response();
  test_encode_ascii_c1_format3_uses_gg_end_code();
  test_decode_ascii_c1_error_uses_two_digit_code();
  test_encode_ascii_c1_rejects_unsupported_series();
  test_encode_ascii_c1_random_write_bits_qna_request_shape();
  test_encode_ascii_c1_random_write_words_qna_request_shape();
  test_encode_ascii_c1_register_monitor_bits_and_read_request_shape();
  test_encode_ascii_c1_register_monitor_words_and_read_request_shape();
  test_encode_ascii_c1_read_module_buffer_request_shape();
  test_encode_ascii_c1_write_module_buffer_request_shape();
  test_encode_ascii_c1_loopback_request_shape();
  test_decode_ascii_c1_loopback_response();
  test_encode_ascii_c1_loopback_rejects_non_ff_pc_no();
  test_encode_ascii_c1_extended_file_register_read_a_request_shape();
  test_encode_ascii_c1_direct_extended_file_register_read_qna_request_shape();
  test_encode_ascii_c1_extended_file_register_write_a_request_shape();
  test_encode_ascii_c1_direct_extended_file_register_write_qna_request_shape();
  test_encode_ascii_c1_extended_file_register_random_write_a_request_shape();
  test_encode_ascii_c1_extended_file_register_monitor_a_request_shape();
  test_validate_e1_config_and_route_constraints();
  test_encode_ascii_e1_batch_read_words_request_shape();
  test_encode_binary_e1_batch_read_bits_request_shape();
  test_decode_ascii_e1_success_response();
  test_decode_binary_e1_error_response_with_abnormal_code();
  test_encode_binary_e1_random_write_words_request_shape();
  test_encode_ascii_e1_extended_file_register_read_request_shape();
  test_encode_ascii_e1_direct_extended_file_register_read_request_shape();
  test_encode_binary_e1_extended_file_register_monitor_registration_request_shape();
  test_encode_ascii_e1_extended_file_register_monitor_read_request_shape();
  test_encode_binary_e1_module_buffer_read_request_shape();
  test_encode_ascii_format4_request_appends_crlf();
  test_decode_ascii_c2_format4_ack_response();
  test_decode_ascii_c2_format4_two_digit_error_response();
  test_decode_ascii_format4_ack_response();
  test_high_level_parse_device_address();
  test_parse_link_direct_device_surface();
  test_parse_qualified_buffer_word_device_rejects_overflow();
  test_high_level_make_contiguous_requests();
  test_high_level_protocol_presets();
  test_high_level_make_random_bit_item();
  test_high_level_make_random_dword_item_defaults();
  test_high_level_make_random_request_from_specs();
  test_high_level_make_random_write_items_from_specs();
  test_high_level_make_monitor_registration_from_specs();
  test_encode_sm_sd_and_lz_device_codes();
  test_encode_batch_write_words_ascii_order();
  test_encode_extended_batch_read_words_ascii_matches_manual_shape();
  test_encode_extended_batch_read_words_binary_matches_capture_shape();
  test_encode_extended_batch_read_words_binary_hg_matches_capture_shape();
  test_encode_extended_batch_read_words_binary_module_access_ql_shape();
  test_encode_link_direct_batch_read_words_binary_iqr_matches_manual_shape();
  test_encode_link_direct_batch_read_bits_binary_iqr_matches_manual_shape();
  test_encode_batch_read_bits_binary_single_uses_addressed_point();
  test_encode_link_direct_batch_read_bits_binary_single_uses_addressed_point();
  test_encode_link_direct_batch_write_words_binary_iqr_shape();
  test_encode_link_direct_batch_write_bits_binary_iqr_shape();
  test_encode_link_direct_random_read_binary_iqr_shape();
  test_encode_link_direct_random_write_words_binary_iqr_shape();
  test_encode_link_direct_random_write_bits_binary_iqr_shape();
  test_encode_link_direct_multi_block_read_binary_iqr_shape();
  test_encode_link_direct_multi_block_write_binary_iqr_shape();
  test_encode_link_direct_multi_block_write_binary_bit_order();
  test_encode_link_direct_register_monitor_binary_iqr_shape();
  test_encode_batch_write_bits_binary_single_even_uses_addressed_point_and_high_nibble();
  test_encode_batch_write_bits_binary_single_odd_uses_addressed_point_and_high_nibble();
  test_encode_link_direct_batch_write_bits_binary_single_even_uses_addressed_point_and_high_nibble();
  test_parse_batch_read_bits_binary_single_uses_high_nibble();
  test_encode_batch_write_bits_binary_two_points_use_high_then_low_nibbles();
  test_parse_batch_read_bits_binary_two_points_use_high_then_low_nibbles();
  test_encode_batch_write_words_ascii_limit_matches_buffer();
  test_encode_batch_write_bits_ascii_limit_matches_buffer();
  test_encode_random_write_words_ascii_matches_manual();
  test_encode_random_read_binary_iqr_layout();
  test_encode_random_read_binary_ql_layout();
  test_encode_random_write_words_binary_iqr_layout();
  test_encode_random_write_words_binary_ql_layout();
  test_encode_random_write_bits_ascii_matches_manual();
  test_encode_random_write_bits_ascii_iqr_shape();
  test_encode_random_write_bits_binary_iqr_layout();
  test_encode_random_write_bits_binary_ql_keeps_device_numbers();
  test_encode_multi_block_read_ascii_matches_manual();
  test_encode_multi_block_read_binary_matches_capture_counts();
  test_encode_multi_block_write_binary_uses_single_byte_block_counts();
  test_encode_multi_block_write_binary_bit_blocks_use_lsb_first_word_packing();
  test_encode_register_monitor_ascii_reuses_random_read_layout();
  test_encode_register_monitor_binary_iqr_layout();
  test_encode_register_monitor_binary_iqr_allows_lz_shape();
  test_encode_read_monitor_ascii_matches_manual();
  test_sparse_native_bit_helpers_match_batch_random_and_monitor_values();
  test_parse_multi_block_read_response_ascii_mixed_blocks();
  test_parse_qualified_buffer_word_device_accepts_g_and_hg();
  test_make_qualified_buffer_read_words_request_maps_to_module_buffer();
  test_make_qualified_buffer_write_words_request_encodes_little_endian_bytes();
  test_decode_qualified_buffer_word_values_decodes_little_endian_bytes();
  test_client_binary_cpu_model_roundtrip();
  test_client_binary_read_user_frame_roundtrip();
  test_client_binary_write_user_frame_roundtrip();
  test_client_binary_e1_batch_read_words_roundtrip();
  test_client_ascii_e1_batch_read_bits_odd_roundtrip();
  test_client_e1_rejects_cpu_model();
  test_client_ascii_c1_loopback_roundtrip();
  test_client_binary_e1_extended_file_register_monitor_roundtrip();
  test_client_remote_control_and_password_roundtrips();
  test_client_clear_error_information_roundtrip();
  test_client_c24_small_command_roundtrips();
  test_client_remote_reset_roundtrip();
  test_client_remote_reset_timeout_without_response_is_success();
  test_client_init_sequence_timeout_without_response_is_success();
  test_client_global_signal_timeout_without_response_is_success();
  test_client_link_direct_random_read_roundtrip();
  test_client_link_direct_register_monitor_roundtrip();
  test_client_ascii_c1_register_monitor_roundtrip();
  test_client_ascii_c1_extended_file_register_monitor_roundtrip();
  test_client_timeout();
  test_client_ascii_format4_resynchronizes_on_stale_ack();
  test_client_write_rejects_unexpected_success_data();
  test_encode_random_read_rejects_long_contact_coil_devices();
  test_encode_random_write_words_allows_ltn_and_lstn();
  test_encode_random_write_words_rejects_long_contact_coil_devices();
  test_encode_random_write_bits_long_device_rules();
  test_encode_random_write_bits_binary_iqr_lcc_layout();
  test_encode_multi_block_read_rejects_long_devices_as_head();
  test_encode_multi_block_write_rejects_long_devices_as_head();

  std::cout << "codec_tests: ok\n";
  return 0;
}
