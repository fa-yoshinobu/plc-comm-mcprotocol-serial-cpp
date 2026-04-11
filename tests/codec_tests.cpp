#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <span>
#include <string_view>

#include "mcprotocol/serial/client.hpp"
#include "mcprotocol/serial/qualified_buffer.hpp"

namespace {

using mcprotocol::serial::AsciiFormat;
using mcprotocol::serial::BatchReadWordsRequest;
using mcprotocol::serial::BatchWriteBitsRequest;
using mcprotocol::serial::BatchWriteWordsRequest;
using mcprotocol::serial::BitValue;
using mcprotocol::serial::CodeMode;
using mcprotocol::serial::CompletionHandler;
using mcprotocol::serial::CpuModelInfo;
using mcprotocol::serial::DecodeStatus;
using mcprotocol::serial::FrameCodec;
using mcprotocol::serial::FrameKind;
using mcprotocol::serial::MelsecSerialClient;
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
using mcprotocol::serial::RandomWriteBitItem;
using mcprotocol::serial::RandomWriteWordItem;
using mcprotocol::serial::RouteConfig;
using mcprotocol::serial::RouteKind;
using mcprotocol::serial::Status;
using mcprotocol::serial::StatusCode;
using mcprotocol::serial::decode_qualified_buffer_word_values;
using mcprotocol::serial::make_qualified_buffer_read_words_request;
using mcprotocol::serial::make_qualified_buffer_write_words_request;
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

void test_encode_random_write_bits_binary_matches_observed_ql_wire_behavior() {
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
      0x02, 0x14, 0x01, 0x00, 0x02, 0x33, 0x00,
      0x00, 0x90, 0x00, 0x2E, 0x00, 0x00, 0x9D, 0x01,
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
      0x64, 0x00, 0x00, 0x90, 0x01, 0x00, 0xAA, 0x55,
  };
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_multi_block_write_binary_bit_blocks_reverse_pair_order() {
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
      0x00, 0x00, 0x84, 0x1C,
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

}  // namespace

int main() {
  test_format5_batch_read_request_matches_manual();
  test_decode_binary_cpu_model_response();
  test_decode_ascii_loopback_response();
  test_encode_ascii_format4_request_appends_crlf();
  test_decode_ascii_format4_ack_response();
  test_encode_batch_write_words_ascii_order();
  test_encode_extended_batch_read_words_ascii_matches_manual_shape();
  test_encode_extended_batch_read_words_binary_matches_capture_shape();
  test_encode_extended_batch_read_words_binary_hg_matches_capture_shape();
  test_encode_extended_batch_read_words_binary_module_access_ql_shape();
  test_encode_batch_write_words_ascii_limit_matches_buffer();
  test_encode_batch_write_bits_ascii_limit_matches_buffer();
  test_encode_random_write_words_ascii_matches_manual();
  test_encode_random_read_binary_ql_layout();
  test_encode_random_write_words_binary_ql_layout();
  test_encode_random_write_bits_ascii_matches_manual();
  test_encode_random_write_bits_ascii_iqr_shape();
  test_encode_random_write_bits_binary_matches_observed_ql_wire_behavior();
  test_encode_multi_block_read_ascii_matches_manual();
  test_encode_multi_block_read_binary_matches_capture_counts();
  test_encode_multi_block_write_binary_uses_single_byte_block_counts();
  test_encode_multi_block_write_binary_bit_blocks_reverse_pair_order();
  test_encode_register_monitor_ascii_reuses_random_read_layout();
  test_encode_read_monitor_ascii_matches_manual();
  test_parse_multi_block_read_response_ascii_mixed_blocks();
  test_parse_qualified_buffer_word_device_accepts_g_and_hg();
  test_make_qualified_buffer_read_words_request_maps_to_module_buffer();
  test_make_qualified_buffer_write_words_request_encodes_little_endian_bytes();
  test_decode_qualified_buffer_word_values_decodes_little_endian_bytes();
  test_client_binary_cpu_model_roundtrip();
  test_client_timeout();
  test_client_ascii_format4_resynchronizes_on_stale_ack();
  test_client_write_rejects_unexpected_success_data();

  std::cout << "codec_tests: ok\n";
  return 0;
}
