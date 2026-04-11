#pragma once

#include <array>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "mcprotocol/serial/span_compat.hpp"
#include "mcprotocol/serial/status.hpp"
#include "mcprotocol/serial/types.hpp"

namespace mcprotocol::serial {

/// \brief Qualified buffer-memory family used by helper `U...` accessors.
enum class QualifiedBufferDeviceKind : std::uint8_t {
  G,
  HG
};

/// \brief Parsed `U...\\G...` or `U...\\HG...` qualified word device.
struct QualifiedBufferWordDevice {
  QualifiedBufferDeviceKind kind = QualifiedBufferDeviceKind::G;
  std::uint16_t module_number = 0;
  std::uint32_t word_address = 0;
};

/// \brief Returns `"G"` or `"HG"` for the helper device kind.
[[nodiscard]] constexpr std::string_view qualified_buffer_kind_name(
    QualifiedBufferDeviceKind kind) noexcept {
  return kind == QualifiedBufferDeviceKind::HG ? std::string_view("HG") : std::string_view("G");
}

/// \brief Converts a qualified word address to the corresponding module-buffer byte address.
[[nodiscard]] constexpr std::uint32_t qualified_buffer_word_to_byte_address(
    std::uint32_t word_address) noexcept {
  return word_address * 2U;
}

namespace detail {

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
  const auto* begin = text.data();
  const auto* end = text.data() + text.size();
  const auto result = std::from_chars(begin, end, out_value, base);
  return result.ec == std::errc() && result.ptr == end;
}

[[nodiscard]] inline bool parse_u32_auto(
    std::string_view text,
    std::uint32_t& out_value) noexcept {
  if (text.size() > 2U && text[0] == '0' &&
      (text[1] == 'x' || text[1] == 'X')) {
    return parse_u32_chars(text.substr(2U), 16, out_value);
  }
  return parse_u32_chars(text, 10, out_value);
}

}  // namespace detail

/// \brief Parses a helper qualified device string such as `U3E0\\G10` or `U3E0\\HG20`.
[[nodiscard]] inline Status parse_qualified_buffer_word_device(
    std::string_view text,
    QualifiedBufferWordDevice& out_device) noexcept {
  if (text.size() < 4U || detail::ascii_upper(text.front()) != 'U') {
    return make_status(
        StatusCode::InvalidArgument,
        "Qualified buffer device must begin with U");
  }

  std::size_t separator = std::string_view::npos;
  for (std::size_t index = 1U; index < text.size(); ++index) {
    if (detail::is_separator(text[index])) {
      separator = index;
      break;
    }
  }
  if (separator == std::string_view::npos || separator <= 1U ||
      separator >= (text.size() - 1U)) {
    return make_status(
        StatusCode::InvalidArgument,
        "Qualified buffer device must look like U3E0\\G0 or U3E0\\HG0");
  }

  std::uint32_t module_number = 0;
  if (!detail::parse_u32_chars(text.substr(1U, separator - 1U), 16, module_number) ||
      module_number > 0xFFFFU) {
    return make_status(
        StatusCode::InvalidArgument,
        "Qualified buffer module number must be a 16-bit hexadecimal value");
  }

  const std::string_view suffix = text.substr(separator + 1U);
  QualifiedBufferDeviceKind kind = QualifiedBufferDeviceKind::G;
  std::size_t prefix_length = 0U;
  if (suffix.size() >= 2U &&
      detail::ascii_upper(suffix[0]) == 'H' &&
      detail::ascii_upper(suffix[1]) == 'G') {
    kind = QualifiedBufferDeviceKind::HG;
    prefix_length = 2U;
  } else if (!suffix.empty() && detail::ascii_upper(suffix[0]) == 'G') {
    kind = QualifiedBufferDeviceKind::G;
    prefix_length = 1U;
  } else {
    return make_status(
        StatusCode::InvalidArgument,
        "Qualified buffer device suffix must start with G or HG");
  }

  if (suffix.size() <= prefix_length) {
    return make_status(
        StatusCode::InvalidArgument,
        "Qualified buffer device must include a word address");
  }

  std::uint32_t word_address = 0;
  if (!detail::parse_u32_auto(suffix.substr(prefix_length), word_address)) {
    return make_status(
        StatusCode::InvalidArgument,
        "Qualified buffer device word address is invalid");
  }

  out_device = QualifiedBufferWordDevice {
      .kind = kind,
      .module_number = static_cast<std::uint16_t>(module_number),
      .word_address = word_address,
  };
  return ok_status();
}

/// \brief Builds a module-buffer read request for a helper qualified word range.
[[nodiscard]] inline Status make_qualified_buffer_read_words_request(
    const QualifiedBufferWordDevice& device,
    std::uint16_t word_length,
    ModuleBufferReadRequest& out_request) noexcept {
  if (word_length == 0U) {
    return make_status(
        StatusCode::InvalidArgument,
        "Qualified buffer read length must be at least 1 word");
  }
  if (word_length > 960U) {
    return make_status(
        StatusCode::InvalidArgument,
        "Qualified buffer read length must be in range 1..960 words");
  }

  out_request = ModuleBufferReadRequest {
      .start_address = qualified_buffer_word_to_byte_address(device.word_address),
      .bytes = static_cast<std::uint16_t>(word_length * 2U),
      .module_number = device.module_number,
  };
  return ok_status();
}

/// \brief Encodes helper qualified word values into little-endian module-buffer bytes.
[[nodiscard]] inline Status encode_qualified_buffer_word_values(
    std::span<const std::uint16_t> words,
    std::span<std::byte> out_bytes,
    std::size_t& out_size) noexcept {
  out_size = 0U;
  if (words.empty()) {
    return make_status(
        StatusCode::InvalidArgument,
        "Qualified buffer write values must not be empty");
  }
  const std::size_t required = words.size() * 2U;
  if (out_bytes.size() < required) {
    return make_status(
        StatusCode::BufferTooSmall,
        "Qualified buffer byte buffer is too small");
  }

  for (std::size_t index = 0; index < words.size(); ++index) {
    const std::uint16_t value = words[index];
    out_bytes[index * 2U] = static_cast<std::byte>(value & 0xFFU);
    out_bytes[index * 2U + 1U] = static_cast<std::byte>((value >> 8U) & 0xFFU);
  }
  out_size = required;
  return ok_status();
}

/// \brief Builds a module-buffer write request for helper qualified word access.
[[nodiscard]] inline Status make_qualified_buffer_write_words_request(
    const QualifiedBufferWordDevice& device,
    std::span<const std::uint16_t> words,
    std::span<std::byte> byte_storage,
    ModuleBufferWriteRequest& out_request,
    std::size_t& out_byte_count) noexcept {
  out_byte_count = 0U;
  if (words.empty()) {
    return make_status(
        StatusCode::InvalidArgument,
        "Qualified buffer write values must not be empty");
  }
  if (words.size() > 960U) {
    return make_status(
        StatusCode::InvalidArgument,
        "Qualified buffer write length must be in range 1..960 words");
  }

  const Status encode_status =
      encode_qualified_buffer_word_values(words, byte_storage, out_byte_count);
  if (!encode_status.ok()) {
    return encode_status;
  }

  out_request = ModuleBufferWriteRequest {
      .start_address = qualified_buffer_word_to_byte_address(device.word_address),
      .module_number = device.module_number,
      .bytes = byte_storage.first(out_byte_count),
  };
  return ok_status();
}

/// \brief Decodes little-endian module-buffer bytes into helper qualified word values.
[[nodiscard]] inline Status decode_qualified_buffer_word_values(
    std::span<const std::byte> bytes,
    std::span<std::uint16_t> out_words) noexcept {
  if ((bytes.size() % 2U) != 0U) {
    return make_status(
        StatusCode::Parse,
        "Qualified buffer byte payload must contain an even number of bytes");
  }
  const std::size_t word_count = bytes.size() / 2U;
  if (out_words.size() < word_count) {
    return make_status(
        StatusCode::BufferTooSmall,
        "Qualified buffer word output buffer is too small");
  }

  for (std::size_t index = 0; index < word_count; ++index) {
    const auto low = static_cast<std::uint8_t>(bytes[index * 2U]);
    const auto high = static_cast<std::uint8_t>(bytes[index * 2U + 1U]);
    out_words[index] = static_cast<std::uint16_t>(low | (high << 8U));
  }
  return ok_status();
}

}  // namespace mcprotocol::serial
