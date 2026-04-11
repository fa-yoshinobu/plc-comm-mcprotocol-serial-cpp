#pragma once

#include <cstddef>
#include <cstring>

#if defined(__has_include)
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

  string_view(const char* text) noexcept
      : data_(text), size_(text ? std::strlen(text) : 0U) {}

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

 private:
  const char* data_ = "";
  size_type size_ = 0U;
};

}  // namespace std
#endif
