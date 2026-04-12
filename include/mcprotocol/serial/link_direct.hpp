#pragma once

#include <cstddef>
#include <cstdint>

#include "mcprotocol/serial/span_compat.hpp"
#include "mcprotocol/serial/status.hpp"
#include "mcprotocol/serial/string_view_compat.hpp"
#include "mcprotocol/serial/types.hpp"

namespace mcprotocol::serial {

/// \brief Parsed `Jn\\...` link-direct device reference such as `J1\\W100`.
struct LinkDirectDevice {
  std::uint16_t network_number = 0;
  DeviceAddress device {};
};

/// \brief One sparse `Jn\\...` item used by native random-read and monitor registration.
struct LinkDirectRandomReadItem {
  LinkDirectDevice device {};
  bool double_word = false;
};

/// \brief One sparse `Jn\\...` word item used by native random word-write.
struct LinkDirectRandomWriteWordItem {
  LinkDirectDevice device {};
  std::uint32_t value = 0;
  bool double_word = false;
};

/// \brief One sparse `Jn\\...` bit item used by native random bit-write.
struct LinkDirectRandomWriteBitItem {
  LinkDirectDevice device {};
  BitValue value = BitValue::Off;
};

/// \brief One `Jn\\...` block used by native multi-block read.
struct LinkDirectMultiBlockReadBlock {
  LinkDirectDevice head_device {};
  std::uint16_t points = 0;
  bool bit_block = false;
};

/// \brief `Jn\\...` native multi-block read request.
struct LinkDirectMultiBlockReadRequest {
  std::span<const LinkDirectMultiBlockReadBlock> blocks {};
};

/// \brief One `Jn\\...` block used by native multi-block write.
struct LinkDirectMultiBlockWriteBlock {
  LinkDirectDevice head_device {};
  std::uint16_t points = 0;
  bool bit_block = false;
  std::span<const std::uint16_t> words {};
  std::span<const BitValue> bits {};
};

/// \brief `Jn\\...` native multi-block write request.
struct LinkDirectMultiBlockWriteRequest {
  std::span<const LinkDirectMultiBlockWriteBlock> blocks {};
};

/// \brief `Jn\\...` monitor registration payload (`0801` + `00C0`).
struct LinkDirectMonitorRegistration {
  std::span<const LinkDirectRandomReadItem> items {};
};

namespace link_direct_detail {

[[nodiscard]] constexpr char ascii_upper(char value) noexcept {
  return (value >= 'a' && value <= 'z') ? static_cast<char>(value - ('a' - 'A')) : value;
}

[[nodiscard]] constexpr bool is_separator(char value) noexcept {
  return value == '\\' || value == '/';
}

[[nodiscard]] inline bool parse_u32_chars(
    std::string_view text,
    int base,
    std::uint32_t& out_value) noexcept {
  if (text.empty()) {
    return false;
  }

  std::uint32_t value = 0U;
  for (char ch : text) {
    std::uint32_t digit = 0U;
    if (ch >= '0' && ch <= '9') {
      digit = static_cast<std::uint32_t>(ch - '0');
    } else if (base == 16 && ch >= 'A' && ch <= 'F') {
      digit = static_cast<std::uint32_t>(ch - 'A' + 10);
    } else if (base == 16 && ch >= 'a' && ch <= 'f') {
      digit = static_cast<std::uint32_t>(ch - 'a' + 10);
    } else {
      return false;
    }

    if (digit >= static_cast<std::uint32_t>(base)) {
      return false;
    }

    value = static_cast<std::uint32_t>(value * static_cast<std::uint32_t>(base) + digit);
  }

  out_value = value;
  return true;
}

struct LinkDirectParseSpec {
  const char* prefix;
  std::size_t prefix_length;
  DeviceCode code;
  int base;
};

constexpr LinkDirectParseSpec kLinkDirectParseSpecs[] = {
    {"SB", 2U, DeviceCode::SB, 16},
    {"SW", 2U, DeviceCode::SW, 16},
    {"X", 1U, DeviceCode::X, 16},
    {"Y", 1U, DeviceCode::Y, 16},
    {"B", 1U, DeviceCode::B, 16},
    {"W", 1U, DeviceCode::W, 16},
};

[[nodiscard]] inline bool parse_link_direct_inner_device(
    std::string_view text,
    DeviceAddress& out_device) noexcept {
  for (const auto& spec : kLinkDirectParseSpecs) {
    if (text.size() <= spec.prefix_length) {
      continue;
    }

    bool prefix_match = true;
    for (std::size_t index = 0; index < spec.prefix_length; ++index) {
      if (ascii_upper(text[index]) != spec.prefix[index]) {
        prefix_match = false;
        break;
      }
    }
    if (!prefix_match) {
      continue;
    }

    std::uint32_t number = 0;
    if (!parse_u32_chars(text.substr(spec.prefix_length), spec.base, number)) {
      return false;
    }

    out_device = DeviceAddress {
        .code = spec.code,
        .number = number,
    };
    return true;
  }

  return false;
}

}  // namespace link_direct_detail

/// \brief Parses a `Jn\\...` link-direct device string such as `J1\\W100` or `J1\\X10`.
[[nodiscard]] inline Status parse_link_direct_device(
    std::string_view text,
    LinkDirectDevice& out_device) noexcept {
  if (text.size() < 4U || link_direct_detail::ascii_upper(text.front()) != 'J') {
    return make_status(
        StatusCode::InvalidArgument,
        "Link direct device must begin with J");
  }

  std::size_t separator = std::string_view::npos;
  for (std::size_t index = 1U; index < text.size(); ++index) {
    if (link_direct_detail::is_separator(text[index])) {
      separator = index;
      break;
    }
  }
  if (separator == std::string_view::npos || separator <= 1U || separator >= (text.size() - 1U)) {
    return make_status(
        StatusCode::InvalidArgument,
        "Link direct device must look like J1\\W100");
  }

  std::uint32_t network_number = 0;
  if (!link_direct_detail::parse_u32_chars(text.substr(1U, separator - 1U), 16, network_number) ||
      network_number > 0xFFFFU) {
    return make_status(
        StatusCode::InvalidArgument,
        "Link direct network number must be a hexadecimal value in range 0x0000..0xFFFF");
  }

  DeviceAddress device {};
  if (!link_direct_detail::parse_link_direct_inner_device(text.substr(separator + 1U), device)) {
    return make_status(
        StatusCode::InvalidArgument,
        "Link direct device must be X, Y, B, W, SB, or SW");
  }

  out_device = LinkDirectDevice {
      .network_number = static_cast<std::uint16_t>(network_number),
      .device = device,
  };
  return ok_status();
}

}  // namespace mcprotocol::serial
