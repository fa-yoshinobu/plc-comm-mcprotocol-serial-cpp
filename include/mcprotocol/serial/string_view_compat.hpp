#pragma once

#include "mcprotocol/serial/compat/cstddef.hpp"

#if (!defined(MCPROTOCOL_SERIAL_USE_BUNDLED_STDLIB_COMPAT) || \
     !MCPROTOCOL_SERIAL_USE_BUNDLED_STDLIB_COMPAT) && \
    defined(__has_include)
#if __has_include(<string_view>)
#include <string_view>
#endif
#endif

// GCC 8 supplies every string_view operation used here with the 201603 value.
// Prefer the standard implementation; keep the fallback out of namespace std.
#if !defined(__cpp_lib_string_view) || (__cpp_lib_string_view < 201603L)
namespace mcprotocol::serial::detail {

class StringView {
 public:
  using value_type = char;
  using size_type = std::size_t;
  using pointer = const char*;
  using const_pointer = const char*;
  using const_reference = const char&;
  using const_iterator = const char*;

  static constexpr size_type npos = static_cast<size_type>(-1);

  constexpr StringView() noexcept = default;

  constexpr StringView(const char* text, size_type size) noexcept : data_(text), size_(size) {}

  constexpr StringView(const char* text) noexcept
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

  [[nodiscard]] constexpr StringView substr(
      size_type pos,
      size_type count = npos) const noexcept {
    if (pos > size_) {
      return StringView();
    }
    const size_type remaining = size_ - pos;
    const size_type actual = (count == npos || count > remaining) ? remaining : count;
    return StringView(data_ + pos, actual);
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

[[nodiscard]] constexpr bool operator==(StringView lhs, StringView rhs) noexcept {
  if (lhs.size() != rhs.size()) {
    return false;
  }
  for (StringView::size_type index = 0U; index < lhs.size(); ++index) {
    if (lhs[index] != rhs[index]) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] constexpr bool operator!=(StringView lhs, StringView rhs) noexcept {
  return !(lhs == rhs);
}

}  // namespace mcprotocol::serial::detail
namespace mcprotocol::serial {
using StringView = detail::StringView;
}
#else
namespace mcprotocol::serial {
// Preserve the standard type and existing API signatures on C++17 hosts.
using StringView = std::string_view;
}
#endif
