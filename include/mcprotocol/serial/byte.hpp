#pragma once

#include "mcprotocol/serial/compat/cstdint.hpp"

namespace mcprotocol::serial {

/// \brief One raw, non-arithmetic octet used by the public C++17 API.
///
/// Convert deliberately with `byte_to_integer<Integer>()`; no implicit numeric conversion or
/// arithmetic operator is provided.
enum class Byte : std::uint8_t {};

template <typename Integer>
[[nodiscard]] constexpr Integer byte_to_integer(Byte value) noexcept {
  return static_cast<Integer>(value);
}

}  // namespace mcprotocol::serial
