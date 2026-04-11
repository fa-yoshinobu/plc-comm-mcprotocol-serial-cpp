#pragma once

#include <array>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <cctype>
#include <string_view>

#include "mcprotocol/serial/span_compat.hpp"
#include "mcprotocol/serial/status.hpp"
#include "mcprotocol/serial/types.hpp"

namespace mcprotocol::serial::highlevel {

/// \file high_level.hpp
/// \brief Optional string-address helpers and protocol presets for common library entry paths.
///
/// This layer does not replace `MelsecSerialClient`. It only reduces setup and request-building
/// friction for the most common cases:
///
/// - choose a baseline `ProtocolConfig`
/// - parse `D100`, `M100`, `X10`, `B20` style addresses
/// - build contiguous and sparse request items without hand-filling `DeviceAddress`

namespace detail {

struct DeviceParseSpec {
  std::string_view prefix;
  DeviceCode code;
  int base;
};

constexpr std::array<DeviceParseSpec, 26> kDeviceParseSpecs {{
    {"STS", DeviceCode::STS, 10},
    {"STC", DeviceCode::STC, 10},
    {"STN", DeviceCode::STN, 10},
    {"TS", DeviceCode::TS, 10},
    {"TC", DeviceCode::TC, 10},
    {"TN", DeviceCode::TN, 10},
    {"CS", DeviceCode::CS, 10},
    {"CC", DeviceCode::CC, 10},
    {"CN", DeviceCode::CN, 10},
    {"SB", DeviceCode::SB, 16},
    {"SW", DeviceCode::SW, 16},
    {"DX", DeviceCode::DX, 16},
    {"DY", DeviceCode::DY, 16},
    {"ZR", DeviceCode::ZR, 16},
    {"X", DeviceCode::X, 16},
    {"Y", DeviceCode::Y, 16},
    {"M", DeviceCode::M, 10},
    {"L", DeviceCode::L, 10},
    {"F", DeviceCode::F, 10},
    {"V", DeviceCode::V, 10},
    {"B", DeviceCode::B, 16},
    {"D", DeviceCode::D, 10},
    {"W", DeviceCode::W, 16},
    {"S", DeviceCode::S, 10},
    {"Z", DeviceCode::Z, 10},
    {"R", DeviceCode::R, 10},
}};

[[nodiscard]] constexpr char ascii_upper(char value) noexcept {
  return (value >= 'a' && value <= 'z') ? static_cast<char>(value - ('a' - 'A')) : value;
}

[[nodiscard]] inline bool parse_u32(
    std::string_view text,
    std::uint32_t& out_value,
    int base) noexcept {
  const auto* begin = text.data();
  const auto* end = text.data() + text.size();
  const auto result = std::from_chars(begin, end, out_value, base);
  return result.ec == std::errc() && result.ptr == end;
}

}  // namespace detail

/// \brief Returns a practical default for Q/L-era `Format5 / Binary / C4`.
[[nodiscard]] constexpr ProtocolConfig make_c4_binary_protocol(
    PlcSeries series = PlcSeries::Q_L) noexcept {
  ProtocolConfig config {};
  config.frame_kind = FrameKind::C4;
  config.code_mode = CodeMode::Binary;
  config.ascii_format = AsciiFormat::Format3;
  config.target_series = series;
  config.sum_check_enabled = true;
  config.route.kind = RouteKind::HostStation;
  config.route.station_no = 0x00;
  config.route.network_no = 0x00;
  config.route.pc_no = 0xFF;
  config.route.request_destination_module_io_no = 0x03FF;
  config.route.request_destination_module_station_no = 0x00;
  config.route.self_station_enabled = false;
  config.route.self_station_no = 0x00;
  config.timeout.response_timeout_ms = 5000;
  config.timeout.inter_byte_timeout_ms = 250;
  return config;
}

/// \brief Returns a practical default for `Format4 / ASCII / C4`.
[[nodiscard]] constexpr ProtocolConfig make_c4_ascii_format4_protocol(
    PlcSeries series = PlcSeries::Q_L) noexcept {
  ProtocolConfig config {};
  config.frame_kind = FrameKind::C4;
  config.code_mode = CodeMode::Ascii;
  config.ascii_format = AsciiFormat::Format4;
  config.target_series = series;
  config.sum_check_enabled = false;
  config.route.kind = RouteKind::MultidropStation;
  config.route.station_no = 0x00;
  config.route.network_no = 0x00;
  config.route.pc_no = 0xFF;
  config.route.request_destination_module_io_no = 0x03FF;
  config.route.request_destination_module_station_no = 0x00;
  config.route.self_station_enabled = false;
  config.route.self_station_no = 0x00;
  config.timeout.response_timeout_ms = 5000;
  config.timeout.inter_byte_timeout_ms = 250;
  return config;
}

/// \brief Parses a plain MC device string such as `D100`, `M100`, `X10`, or `B20`.
[[nodiscard]] inline Status parse_device_address(
    std::string_view text,
    DeviceAddress& out_device) noexcept {
  for (const auto& spec : detail::kDeviceParseSpecs) {
    if (text.size() <= spec.prefix.size()) {
      continue;
    }

    bool prefix_match = true;
    for (std::size_t index = 0; index < spec.prefix.size(); ++index) {
      const char lhs = detail::ascii_upper(text[index]);
      if (lhs != spec.prefix[index]) {
        prefix_match = false;
        break;
      }
    }
    if (!prefix_match) {
      continue;
    }

    std::uint32_t number = 0;
    if (!detail::parse_u32(text.substr(spec.prefix.size()), number, spec.base)) {
      return make_status(StatusCode::InvalidArgument, "Device address number is invalid");
    }

    out_device = DeviceAddress {
        .code = spec.code,
        .number = number,
    };
    return ok_status();
  }

  return make_status(StatusCode::InvalidArgument, "Device address prefix is not supported");
}

/// \brief Builds a contiguous word-read request from a string address such as `D100`.
[[nodiscard]] inline Status make_batch_read_words_request(
    std::string_view head_device,
    std::uint16_t points,
    BatchReadWordsRequest& out_request) noexcept {
  DeviceAddress parsed {};
  const Status status = parse_device_address(head_device, parsed);
  if (!status.ok()) {
    return status;
  }
  out_request = BatchReadWordsRequest {
      .head_device = parsed,
      .points = points,
  };
  return ok_status();
}

/// \brief Builds a contiguous bit-read request from a string address such as `M100`.
[[nodiscard]] inline Status make_batch_read_bits_request(
    std::string_view head_device,
    std::uint16_t points,
    BatchReadBitsRequest& out_request) noexcept {
  DeviceAddress parsed {};
  const Status status = parse_device_address(head_device, parsed);
  if (!status.ok()) {
    return status;
  }
  out_request = BatchReadBitsRequest {
      .head_device = parsed,
      .points = points,
  };
  return ok_status();
}

/// \brief Builds a contiguous word-write request from a string address such as `D100`.
[[nodiscard]] inline Status make_batch_write_words_request(
    std::string_view head_device,
    std::span<const std::uint16_t> words,
    BatchWriteWordsRequest& out_request) noexcept {
  DeviceAddress parsed {};
  const Status status = parse_device_address(head_device, parsed);
  if (!status.ok()) {
    return status;
  }
  out_request = BatchWriteWordsRequest {
      .head_device = parsed,
      .words = words,
  };
  return ok_status();
}

/// \brief Builds a contiguous bit-write request from a string address such as `M100`.
[[nodiscard]] inline Status make_batch_write_bits_request(
    std::string_view head_device,
    std::span<const BitValue> bits,
    BatchWriteBitsRequest& out_request) noexcept {
  DeviceAddress parsed {};
  const Status status = parse_device_address(head_device, parsed);
  if (!status.ok()) {
    return status;
  }
  out_request = BatchWriteBitsRequest {
      .head_device = parsed,
      .bits = bits,
  };
  return ok_status();
}

/// \brief Builds one sparse random-read item from a string address.
[[nodiscard]] inline Status make_random_read_item(
    std::string_view device,
    RandomReadItem& out_item,
    bool double_word = false) noexcept {
  DeviceAddress parsed {};
  const Status status = parse_device_address(device, parsed);
  if (!status.ok()) {
    return status;
  }
  out_item = RandomReadItem {
      .device = parsed,
      .double_word = double_word,
  };
  return ok_status();
}

/// \brief Builds one sparse random word-write item from a string address.
[[nodiscard]] inline Status make_random_write_word_item(
    std::string_view device,
    std::uint32_t value,
    RandomWriteWordItem& out_item,
    bool double_word = false) noexcept {
  DeviceAddress parsed {};
  const Status status = parse_device_address(device, parsed);
  if (!status.ok()) {
    return status;
  }
  out_item = RandomWriteWordItem {
      .device = parsed,
      .value = value,
      .double_word = double_word,
  };
  return ok_status();
}

/// \brief Builds one sparse random bit-write item from a string address.
[[nodiscard]] inline Status make_random_write_bit_item(
    std::string_view device,
    BitValue value,
    RandomWriteBitItem& out_item) noexcept {
  DeviceAddress parsed {};
  const Status status = parse_device_address(device, parsed);
  if (!status.ok()) {
    return status;
  }
  out_item = RandomWriteBitItem {
      .device = parsed,
      .value = value,
  };
  return ok_status();
}

}  // namespace mcprotocol::serial::highlevel
