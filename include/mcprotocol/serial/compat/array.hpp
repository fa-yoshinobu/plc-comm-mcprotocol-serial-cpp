#pragma once

#if defined(MCPROTOCOL_SERIAL_USE_BUNDLED_STDLIB_COMPAT)
#if MCPROTOCOL_SERIAL_USE_BUNDLED_STDLIB_COMPAT
#define MCPROTOCOL_SERIAL_BUNDLED_ARRAY 1
#else
#include <array>
#endif
#elif defined(__has_include)
#if __has_include(<array>)
#include <array>
#else
#define MCPROTOCOL_SERIAL_BUNDLED_ARRAY 1
#endif
#else
#include <array>
#endif
#if defined(MCPROTOCOL_SERIAL_BUNDLED_ARRAY)
#include "mcprotocol/serial/compat/cstddef.hpp"

namespace std {

template <typename T, size_t N>
struct array {
  using value_type = T;
  using size_type = size_t;
  using difference_type = ptrdiff_t;
  using reference = value_type&;
  using const_reference = const value_type&;
  using pointer = value_type*;
  using const_pointer = const value_type*;
  using iterator = value_type*;
  using const_iterator = const value_type*;

  value_type elems[N == 0 ? 1 : N];

  [[nodiscard]] constexpr iterator begin() noexcept { return elems; }
  [[nodiscard]] constexpr const_iterator begin() const noexcept { return elems; }
  [[nodiscard]] constexpr iterator end() noexcept { return elems + N; }
  [[nodiscard]] constexpr const_iterator end() const noexcept { return elems + N; }
  [[nodiscard]] constexpr reference operator[](size_type index) noexcept { return elems[index]; }
  [[nodiscard]] constexpr const_reference operator[](size_type index) const noexcept {
    return elems[index];
  }
  [[nodiscard]] constexpr pointer data() noexcept { return elems; }
  [[nodiscard]] constexpr const_pointer data() const noexcept { return elems; }
  [[nodiscard]] constexpr size_type size() const noexcept { return N; }
  [[nodiscard]] constexpr bool empty() const noexcept { return N == 0; }
  [[nodiscard]] constexpr reference front() noexcept { return elems[0]; }
  [[nodiscard]] constexpr const_reference front() const noexcept { return elems[0]; }
  [[nodiscard]] constexpr reference back() noexcept { return elems[N - 1]; }
  [[nodiscard]] constexpr const_reference back() const noexcept { return elems[N - 1]; }

  constexpr void fill(const value_type& value) {
    for (size_type index = 0; index < N; ++index) {
      elems[index] = value;
    }
  }
};

template <typename T, size_t N>
[[nodiscard]] constexpr bool operator==(const array<T, N>& lhs, const array<T, N>& rhs) {
  for (size_t index = 0; index < N; ++index) {
    if (!(lhs[index] == rhs[index])) {
      return false;
    }
  }
  return true;
}

template <typename T, size_t N>
[[nodiscard]] constexpr bool operator!=(const array<T, N>& lhs, const array<T, N>& rhs) {
  return !(lhs == rhs);
}

}  // namespace std
#endif
