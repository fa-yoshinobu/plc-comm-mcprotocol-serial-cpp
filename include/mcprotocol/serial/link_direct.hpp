#pragma once

#include "mcprotocol/serial/compat/cstddef.hpp"
#include "mcprotocol/serial/compat/cstdint.hpp"

#include "mcprotocol/serial/detail/parse_helpers.hpp"
#include "mcprotocol/serial/span.hpp"
#include "mcprotocol/serial/status.hpp"
#include "mcprotocol/serial/string_view_compat.hpp"
#include "mcprotocol/serial/types.hpp"

namespace mcprotocol::serial {

/// \brief Parsed `Jn\\...` link-direct device reference such as `J1\\W100`.
struct LinkDirectDevice {
  LinkDirectDevice() = delete;
  constexpr LinkDirectDevice(
      std::uint16_t target_network_number,
      DeviceAddress target_device) noexcept
      : network_number(target_network_number), device(target_device) {}

  std::uint16_t network_number;
  DeviceAddress device;
};

/// \brief One sparse `Jn\\...` item used by native random-read and monitor registration.
struct LinkDirectRandomReadWordItem {
  LinkDirectDevice device;
};

/// \brief One sparse `Jn\\...` word item used by native random word-write.
///
/// Device and value must be supplied together. Explicit zero is valid.
struct LinkDirectRandomWriteWordItem {
  LinkDirectRandomWriteWordItem() = delete;
  constexpr LinkDirectRandomWriteWordItem(
      LinkDirectDevice target_device,
      std::uint16_t write_value) noexcept
      : device(target_device), value(write_value) {}

  LinkDirectDevice device;
  std::uint16_t value;
};

/// \brief One sparse `Jn\\...` bit item used by native random bit-write.
///
/// Device and value must be supplied together. Explicit `Off` is valid.
struct LinkDirectRandomWriteBitItem {
  LinkDirectRandomWriteBitItem() = delete;
  constexpr LinkDirectRandomWriteBitItem(
      LinkDirectDevice target_device,
      BitValue write_value) noexcept
      : device(target_device), value(write_value) {}

  LinkDirectDevice device;
  BitValue value;
};

/// \brief One `Jn\\...` block used by native multi-block read.
struct LinkDirectMultiBlockReadBlock {
  LinkDirectMultiBlockReadBlock() = delete;
  constexpr LinkDirectMultiBlockReadBlock(
      LinkDirectDevice first_device,
      std::uint16_t point_count,
      bool use_bit_block) noexcept
      : head_device(first_device), points(point_count), bit_block(use_bit_block) {}

  LinkDirectDevice head_device;
  std::uint16_t points;
  bool bit_block;
};

/// \brief `Jn\\...` native multi-block read request.
struct LinkDirectMultiBlockReadRequest {
  LinkDirectMultiBlockReadRequest() = delete;
  constexpr explicit LinkDirectMultiBlockReadRequest(
      mcprotocol::serial::Span<const LinkDirectMultiBlockReadBlock> request_blocks) noexcept
      : blocks(request_blocks) {}

  mcprotocol::serial::Span<const LinkDirectMultiBlockReadBlock> blocks;
};

/// \brief One `Jn\\...` block used by native multi-block write.
struct LinkDirectMultiBlockWriteBlock {
  LinkDirectMultiBlockWriteBlock() = delete;
  constexpr LinkDirectMultiBlockWriteBlock(
      LinkDirectDevice first_device,
      std::uint16_t point_count,
      bool use_bit_block,
      mcprotocol::serial::Span<const std::uint16_t> write_words,
      mcprotocol::serial::Span<const BitValue> write_bits) noexcept
      : head_device(first_device),
        points(point_count),
        bit_block(use_bit_block),
        words(write_words),
        bits(write_bits) {}
  constexpr LinkDirectMultiBlockWriteBlock(
      LinkDirectDevice first_device,
      std::uint16_t point_count,
      mcprotocol::serial::Span<const std::uint16_t> write_words) noexcept
      : LinkDirectMultiBlockWriteBlock(first_device, point_count, false, write_words, {}) {}
  constexpr LinkDirectMultiBlockWriteBlock(
      LinkDirectDevice first_device,
      std::uint16_t point_count,
      mcprotocol::serial::Span<const BitValue> write_bits) noexcept
      : LinkDirectMultiBlockWriteBlock(first_device, point_count, true, {}, write_bits) {}

  LinkDirectDevice head_device;
  std::uint16_t points;
  bool bit_block;
  mcprotocol::serial::Span<const std::uint16_t> words;
  mcprotocol::serial::Span<const BitValue> bits;
};

/// \brief `Jn\\...` native multi-block write request.
struct LinkDirectMultiBlockWriteRequest {
  LinkDirectMultiBlockWriteRequest() = delete;
  constexpr explicit LinkDirectMultiBlockWriteRequest(
      mcprotocol::serial::Span<const LinkDirectMultiBlockWriteBlock> request_blocks) noexcept
      : blocks(request_blocks) {}

  mcprotocol::serial::Span<const LinkDirectMultiBlockWriteBlock> blocks;
};

/// \brief `Jn\\...` monitor registration payload (`0801` + device extension specification).
struct LinkDirectMonitorRegistration {
  LinkDirectMonitorRegistration() = delete;
  constexpr explicit LinkDirectMonitorRegistration(
      mcprotocol::serial::Span<const LinkDirectRandomReadWordItem> monitor_items) noexcept
      : word_items(monitor_items) {}

  mcprotocol::serial::Span<const LinkDirectRandomReadWordItem> word_items;
};

namespace link_direct_detail {

using mcprotocol::serial::detail::ascii_upper;
using mcprotocol::serial::detail::is_separator;
using mcprotocol::serial::detail::parse_u32;

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
    if (!parse_u32(text.substr(spec.prefix_length), number, spec.base)) {
      return false;
    }

    out_device = DeviceAddress(spec.code, number);
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
  if (!link_direct_detail::parse_u32(text.substr(1U, separator - 1U), network_number, 16) ||
      network_number > 0xFFFFU) {
    return make_status(
        StatusCode::InvalidArgument,
        "Link direct network number must be a hexadecimal value in range 0x0000..0xFFFF");
  }

  DeviceAddress device(DeviceCode::D, 0U);
  if (!link_direct_detail::parse_link_direct_inner_device(text.substr(separator + 1U), device)) {
    return make_status(
        StatusCode::InvalidArgument,
        "Link direct device must be X, Y, B, W, SB, or SW");
  }

  out_device = LinkDirectDevice(static_cast<std::uint16_t>(network_number), device);
  return ok_status();
}

}  // namespace mcprotocol::serial
