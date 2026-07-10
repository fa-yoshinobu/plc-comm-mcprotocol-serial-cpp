#pragma once

#include "mcprotocol/serial/compat/cstddef.hpp"
#include "mcprotocol/serial/compat/cstring.hpp"

#if (!defined(MCPROTOCOL_SERIAL_USE_BUNDLED_STDLIB_COMPAT) || \
     !MCPROTOCOL_SERIAL_USE_BUNDLED_STDLIB_COMPAT) && \
    defined(__has_include)
#if __has_include(<string_view>)
#include <string_view>
#endif
#endif

#if !defined(__cpp_lib_string_view) || (__cpp_lib_string_view < 201606L)
namespace std {

class string_view {
 public:
  using value_type = char;
  using size_type = size_t;
  using pointer = const char*;
  using const_pointer = const char*;
  using const_reference = const char&;
  using const_iterator = const char*;

  static constexpr size_type npos = static_cast<size_type>(-1);

  constexpr string_view() noexcept = default;

  constexpr string_view(const char* text, size_type size) noexcept : data_(text), size_(size) {}

  constexpr string_view(const char* text) noexcept
      : data_(text ? text : ""), size_(literal_length(text)) {}

  [[nodiscard]] constexpr const_iterator begin() const noexcept { return data_; }
  [[nodiscard]] constexpr const_iterator end() const noexcept { return data_ + size_; }
  [[nodiscard]] constexpr const_pointer data() const noexcept { return data_; }
  [[nodiscard]] constexpr size_type size() const noexcept { return size_; }
  [[nodiscard]] constexpr bool empty() const noexcept { return size_ == 0U; }
  [[nodiscard]] constexpr const_reference front() const noexcept { return data_[0]; }
  [[nodiscard]] constexpr const_reference operator[](size_type index) const noexcept {
    return data_[index];
  }

  [[nodiscard]] constexpr string_view substr(
      size_type pos,
      size_type count = npos) const noexcept {
    if (pos > size_) {
      return string_view();
    }
    const size_type remaining = size_ - pos;
    const size_type actual = (count == npos || count > remaining) ? remaining : count;
    return string_view(data_ + pos, actual);
  }

  constexpr void remove_prefix(size_type count) noexcept {
    if (count > size_) {
      count = size_;
    }
    data_ += count;
    size_ -= count;
  }

  [[nodiscard]] constexpr size_type find(value_type ch, size_type pos = 0U) const noexcept {
    for (size_type index = pos; index < size_; ++index) {
      if (data_[index] == ch) {
        return index;
      }
    }
    return npos;
  }

 private:
  [[nodiscard]] static constexpr size_type literal_length(const char* text) noexcept {
    if (text == nullptr) {
      return 0U;
    }
    size_type size = 0U;
    while (text[size] != '\0') {
      ++size;
    }
    return size;
  }

  const char* data_ = "";
  size_type size_ = 0U;
};

[[nodiscard]] constexpr bool operator==(string_view lhs, string_view rhs) noexcept {
  if (lhs.size() != rhs.size()) {
    return false;
  }
  for (string_view::size_type index = 0U; index < lhs.size(); ++index) {
    if (lhs[index] != rhs[index]) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] constexpr bool operator!=(string_view lhs, string_view rhs) noexcept {
  return !(lhs == rhs);
}

}  // namespace std
#endif
