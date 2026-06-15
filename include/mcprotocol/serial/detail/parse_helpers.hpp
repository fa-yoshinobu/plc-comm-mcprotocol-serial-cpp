#pragma once

#include <cstdint>

#include "mcprotocol/serial/string_view_compat.hpp"

namespace mcprotocol::serial::detail {

[[nodiscard]] constexpr char ascii_upper(char value) noexcept {
  return (value >= 'a' && value <= 'z') ? static_cast<char>(value - ('a' - 'A')) : value;
}

[[nodiscard]] constexpr bool is_separator(char value) noexcept {
  return value == '\\' || value == '/';
}

[[nodiscard]] inline bool parse_u32(
    std::string_view text,
    std::uint32_t& out_value,
    int base) noexcept {
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

    const std::uint32_t radix = static_cast<std::uint32_t>(base);
    constexpr std::uint32_t max_u32 = static_cast<std::uint32_t>(0xFFFFFFFFULL);
    if (value > ((max_u32 - digit) / radix)) {
      return false;
    }
    value = static_cast<std::uint32_t>(value * radix + digit);
  }

  out_value = value;
  return true;
}

}  // namespace mcprotocol::serial::detail
