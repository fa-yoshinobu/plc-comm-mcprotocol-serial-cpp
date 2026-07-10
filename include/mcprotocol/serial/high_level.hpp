#pragma once

#include "mcprotocol/serial/compat/array.hpp"
#include "mcprotocol/serial/compat/cstddef.hpp"
#include "mcprotocol/serial/compat/cstdint.hpp"

#include "mcprotocol/serial/detail/parse_helpers.hpp"
#include "mcprotocol/serial/span_compat.hpp"
#include "mcprotocol/serial/status.hpp"
#include "mcprotocol/serial/string_view_compat.hpp"
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
///
/// The current parser accepts plain device strings only. If this layer grows typed named-address
/// views later, `:` is reserved for data types such as `D100:D`, while `.` is reserved for
/// bit-in-word access such as `D100.D` meaning bit `0xD` / bit 13.

namespace detail {

struct DeviceParseSpec {
  const char* prefix;
  std::size_t prefix_length;
  DeviceCode code;
  int base;
};

constexpr std::array<DeviceParseSpec, 38> kDeviceParseSpecs {{
    {"STS", 3U, DeviceCode::STS, 10},
    {"STC", 3U, DeviceCode::STC, 10},
    {"STN", 3U, DeviceCode::STN, 10},
    {"TS", 2U, DeviceCode::TS, 10},
    {"TC", 2U, DeviceCode::TC, 10},
    {"TN", 2U, DeviceCode::TN, 10},
    {"CS", 2U, DeviceCode::CS, 10},
    {"CC", 2U, DeviceCode::CC, 10},
    {"CN", 2U, DeviceCode::CN, 10},
    {"SB", 2U, DeviceCode::SB, 16},
    {"SW", 2U, DeviceCode::SW, 16},
    {"SM", 2U, DeviceCode::SM, 10},
    {"SD", 2U, DeviceCode::SD, 10},
    {"DX", 2U, DeviceCode::DX, 16},
    {"DY", 2U, DeviceCode::DY, 16},
    {"LTS", 3U, DeviceCode::LTS, 10},
    {"LTC", 3U, DeviceCode::LTC, 10},
    {"LTN", 3U, DeviceCode::LTN, 10},
    {"LSTS", 4U, DeviceCode::LSTS, 10},
    {"LSTC", 4U, DeviceCode::LSTC, 10},
    {"LSTN", 4U, DeviceCode::LSTN, 10},
    {"LCS", 3U, DeviceCode::LCS, 10},
    {"LCC", 3U, DeviceCode::LCC, 10},
    {"LCN", 3U, DeviceCode::LCN, 10},
    {"LZ", 2U, DeviceCode::LZ, 10},
    {"RD", 2U, DeviceCode::RD, 10},
    {"ZR", 2U, DeviceCode::ZR, 10},
    {"X", 1U, DeviceCode::X, 16},
    {"Y", 1U, DeviceCode::Y, 16},
    {"M", 1U, DeviceCode::M, 10},
    {"L", 1U, DeviceCode::L, 10},
    {"F", 1U, DeviceCode::F, 10},
    {"V", 1U, DeviceCode::V, 10},
    {"B", 1U, DeviceCode::B, 16},
    {"D", 1U, DeviceCode::D, 10},
    {"W", 1U, DeviceCode::W, 16},
    {"Z", 1U, DeviceCode::Z, 10},
    {"R", 1U, DeviceCode::R, 10},
}};

using mcprotocol::serial::detail::ascii_upper;
using mcprotocol::serial::detail::parse_u32;

[[nodiscard]] constexpr bool is_double_word_device(DeviceCode code) noexcept {
  switch (code) {
    case DeviceCode::LTN:
    case DeviceCode::LSTN:
    case DeviceCode::LCN:
    case DeviceCode::LZ:
      return true;
    default:
      return false;
  }
}

}  // namespace detail

/// \brief Returns a practical `Format5 / Binary / C4` configuration for an explicit PLC profile.
[[nodiscard]] constexpr ProtocolConfig make_c4_binary_protocol(PlcProfile profile) noexcept {
  ProtocolConfig config {};
  config.frame_kind = FrameKind::C4;
  config.code_mode = CodeMode::Binary;
  config.ascii_format = AsciiFormat::Format3;
  config.plc_profile = profile;
  config.sum_check_enabled = true;
  config.route.kind = RouteKind::HostStation;
  config.route.station_no = 0x00;
  config.route.network_no = 0x00;
  config.route.pc_no = 0xFF;
  config.route.request_destination_module_io_no = module_io::OwnStation;
  config.route.request_destination_module_station_no = 0x00;
  config.route.self_station_enabled = false;
  config.route.self_station_no = 0x00;
  config.timeout.response_timeout_ms = 5000;
  config.timeout.inter_byte_timeout_ms = 250;
  return config;
}

/// \brief Returns a practical `Format4 / ASCII / C4` configuration for an explicit PLC profile.
[[nodiscard]] constexpr ProtocolConfig make_c4_ascii_format4_protocol(PlcProfile profile) noexcept {
  ProtocolConfig config {};
  config.frame_kind = FrameKind::C4;
  config.code_mode = CodeMode::Ascii;
  config.ascii_format = AsciiFormat::Format4;
  config.plc_profile = profile;
  config.sum_check_enabled = false;
  config.route.kind = RouteKind::MultidropStation;
  config.route.station_no = 0x00;
  config.route.network_no = 0x00;
  config.route.pc_no = 0xFF;
  config.route.request_destination_module_io_no = module_io::OwnStation;
  config.route.request_destination_module_station_no = 0x00;
  config.route.self_station_enabled = false;
  config.route.self_station_no = 0x00;
  config.timeout.response_timeout_ms = 5000;
  config.timeout.inter_byte_timeout_ms = 250;
  return config;
}

/// \brief Returns a practical default for `Format2 / ASCII / C4`.
///
/// `Format2` is the `Format1` style `ENQ/ACK/NAK/STX/ETX` link with an extra 1-byte block
/// number inserted before the frame ID. The default block number is `0x00`; change
/// `ProtocolConfig::ascii_block_number` if the host side needs a different value.
[[nodiscard]] constexpr ProtocolConfig make_c4_ascii_format2_protocol(PlcProfile profile) noexcept {
  ProtocolConfig config = make_c4_ascii_format4_protocol(profile);
  config.ascii_format = AsciiFormat::Format2;
  config.sum_check_enabled = true;
  config.route.kind = RouteKind::HostStation;
  config.route.station_no = 0x00;
  config.ascii_block_number = 0x00;
  return config;
}

/// \brief String-address spec used to build sparse random-read or monitor requests.
struct RandomReadSpec {
  /// Plain device string such as `D100`, `LZ0`, or `LCN10`.
  std::string_view device {};
  /// `true` when the target should be encoded as a double-word sparse item.
  bool double_word = false;
};

/// \brief String-address spec used to build sparse random word-write items.
struct RandomWriteWordSpec {
  /// Plain device string such as `D100` or `LZ0`.
  std::string_view device {};
  /// Word or double-word value written to `device`.
  std::uint32_t value = 0;
  /// `true` when the item should be encoded as a double-word sparse write.
  bool double_word = false;
};

/// \brief String-address spec used to build sparse random bit-write items.
struct RandomWriteBitSpec {
  /// Plain bit-device string such as `M100` or `X10`.
  std::string_view device {};
  /// Bit value written to `device`.
  BitValue value = BitValue::Off;
};

/// \brief Logical state selected from a long timer/counter status block.
enum class LongStateReadKind : std::uint8_t {
  Contact,
  Coil
};

enum class LongStateReadRoute : std::uint8_t {
  StatusBlock,
  DirectBits
};

/// \brief Mapping from a long-family state device to the helper's internal read route.
struct LongStateReadSpec {
  /// Read route used internally by the long-state helper.
  LongStateReadRoute route = LongStateReadRoute::StatusBlock;
  /// Base current-value device read with `0401` word access, or direct bit device for DirectBits.
  DeviceCode base_code = DeviceCode::LTN;
  /// Status bit selected from the third word of the block.
  LongStateReadKind kind = LongStateReadKind::Contact;
};

/// \brief Parses a plain MC device string such as `D100`, `M100`, `X10`, or `B20`.
///
/// This helper is intentionally limited to plain device syntax. It does not parse `Jn\\...` link-
/// direct addresses, helper-qualified `U...\\G...` addresses, or standalone `G` / `HG`.
[[nodiscard]] inline Status parse_device_address(
    std::string_view text,
    DeviceAddress& out_device) noexcept {
  for (const auto& spec : detail::kDeviceParseSpecs) {
    if (text.size() <= spec.prefix_length) {
      continue;
    }

    bool prefix_match = true;
    for (std::size_t index = 0; index < spec.prefix_length; ++index) {
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
    if (!detail::parse_u32(text.substr(spec.prefix_length), number, spec.base)) {
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

/// \brief Resolves the dedicated read path for long timer/counter state devices.
///
/// `LTS/LTC/LSTS/LSTC/LCS/LCC` are read through this helper. Timer state devices use the
/// corresponding `LTN/LSTN` 4-word status block; long counter contacts/coils use direct bit access.
[[nodiscard]] inline Status get_long_state_read_spec(
    DeviceCode code,
    LongStateReadSpec& out_spec) noexcept {
  switch (code) {
    case DeviceCode::LTS:
      out_spec = LongStateReadSpec {
          .route = LongStateReadRoute::StatusBlock,
          .base_code = DeviceCode::LTN,
          .kind = LongStateReadKind::Contact};
      return ok_status();
    case DeviceCode::LTC:
      out_spec = LongStateReadSpec {
          .route = LongStateReadRoute::StatusBlock,
          .base_code = DeviceCode::LTN,
          .kind = LongStateReadKind::Coil};
      return ok_status();
    case DeviceCode::LSTS:
      out_spec = LongStateReadSpec {
          .route = LongStateReadRoute::StatusBlock,
          .base_code = DeviceCode::LSTN,
          .kind = LongStateReadKind::Contact};
      return ok_status();
    case DeviceCode::LSTC:
      out_spec = LongStateReadSpec {
          .route = LongStateReadRoute::StatusBlock,
          .base_code = DeviceCode::LSTN,
          .kind = LongStateReadKind::Coil};
      return ok_status();
    case DeviceCode::LCS:
      out_spec = LongStateReadSpec {
          .route = LongStateReadRoute::DirectBits,
          .base_code = DeviceCode::LCS,
          .kind = LongStateReadKind::Contact};
      return ok_status();
    case DeviceCode::LCC:
      out_spec = LongStateReadSpec {
          .route = LongStateReadRoute::DirectBits,
          .base_code = DeviceCode::LCC,
          .kind = LongStateReadKind::Coil};
      return ok_status();
    default:
      return make_status(StatusCode::InvalidArgument, "Device is not a long timer/counter state device");
  }
}

/// \brief Decodes the contact/coil bit from a long-family 4-word status block.
[[nodiscard]] inline Status decode_long_state_bit(
    const LongStateReadSpec& spec,
    std::span<const std::uint16_t> status_block_words,
    BitValue& out_value) noexcept {
  if (status_block_words.size() < 4U) {
    return make_status(StatusCode::BufferTooSmall, "Long state status block requires 4 words");
  }

  const std::uint16_t status_word = status_block_words[2];
  const std::uint16_t mask =
      spec.kind == LongStateReadKind::Contact ? static_cast<std::uint16_t>(0x0002U)
                                              : static_cast<std::uint16_t>(0x0001U);
  out_value = (status_word & mask) == 0U ? BitValue::Off : BitValue::On;
  return ok_status();
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
      .double_word = double_word || detail::is_double_word_device(parsed.code),
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
      .double_word = double_word || detail::is_double_word_device(parsed.code),
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

/// \brief Builds a sparse random-read request from string-address specs.
///
/// Use this when you want `0403` style sparse addressing without hand-filling `RandomReadItem`
/// entries.
[[nodiscard]] inline Status make_random_read_request(
    std::span<const RandomReadSpec> specs,
    std::span<RandomReadItem> out_items,
    RandomReadRequest& out_request) noexcept {
  if (out_items.size() < specs.size()) {
    return make_status(StatusCode::BufferTooSmall, "Random read output item buffer is too small");
  }

  for (std::size_t index = 0; index < specs.size(); ++index) {
    const Status status =
        make_random_read_item(specs[index].device, out_items[index], specs[index].double_word);
    if (!status.ok()) {
      return status;
    }
  }

  out_request = RandomReadRequest {
      .items = std::span<const RandomReadItem>(out_items.data(), specs.size()),
  };
  return ok_status();
}

/// \brief Builds a sparse monitor registration payload from string-address specs.
///
/// The resulting payload is intended for `0801`. Readback still happens through the normal monitor
/// read API.
[[nodiscard]] inline Status make_monitor_registration(
    std::span<const RandomReadSpec> specs,
    std::span<RandomReadItem> out_items,
    MonitorRegistration& out_request) noexcept {
  RandomReadRequest request {};
  const Status status = make_random_read_request(specs, out_items, request);
  if (!status.ok()) {
    return status;
  }

  out_request = MonitorRegistration {
      .items = request.items,
  };
  return ok_status();
}

/// \brief Builds sparse random word-write items from string-address specs.
[[nodiscard]] inline Status make_random_write_word_items(
    std::span<const RandomWriteWordSpec> specs,
    std::span<RandomWriteWordItem> out_items,
    std::span<const RandomWriteWordItem>& out_item_view) noexcept {
  if (out_items.size() < specs.size()) {
    return make_status(StatusCode::BufferTooSmall, "Random write word output item buffer is too small");
  }

  for (std::size_t index = 0; index < specs.size(); ++index) {
    const Status status = make_random_write_word_item(
        specs[index].device,
        specs[index].value,
        out_items[index],
        specs[index].double_word);
    if (!status.ok()) {
      return status;
    }
  }

  out_item_view = std::span<const RandomWriteWordItem>(out_items.data(), specs.size());
  return ok_status();
}

/// \brief Builds sparse random bit-write items from string-address specs.
[[nodiscard]] inline Status make_random_write_bit_items(
    std::span<const RandomWriteBitSpec> specs,
    std::span<RandomWriteBitItem> out_items,
    std::span<const RandomWriteBitItem>& out_item_view) noexcept {
  if (out_items.size() < specs.size()) {
    return make_status(StatusCode::BufferTooSmall, "Random write bit output item buffer is too small");
  }

  for (std::size_t index = 0; index < specs.size(); ++index) {
    const Status status =
        make_random_write_bit_item(specs[index].device, specs[index].value, out_items[index]);
    if (!status.ok()) {
      return status;
    }
  }

  out_item_view = std::span<const RandomWriteBitItem>(out_items.data(), specs.size());
  return ok_status();
}

}  // namespace mcprotocol::serial::highlevel
