#pragma once

#include <array>
#include <cstddef>
#include <utility>

namespace mcprotocol::serial::detail {

template <typename T, std::size_t... Indices>
[[nodiscard]] constexpr std::array<T, sizeof...(Indices)> make_filled_array_impl(
    const T& value,
    std::index_sequence<Indices...>) {
  return {{(static_cast<void>(Indices), value)...}};
}

template <typename T, std::size_t Size>
[[nodiscard]] constexpr std::array<T, Size> make_filled_array(const T& value) {
  return make_filled_array_impl(value, std::make_index_sequence<Size> {});
}

}  // namespace mcprotocol::serial::detail
