#pragma once

#include <array>
#include <cstddef>
#include <type_traits>

#if defined(__has_include)
#if __has_include(<span>)
#include <span>
#endif
#endif

#if !defined(__cpp_lib_span) || (__cpp_lib_span < 202002L)
namespace std {

constexpr size_t dynamic_extent = static_cast<size_t>(-1);

#if !defined(__cpp_lib_byte) || (__cpp_lib_byte < 201603L)
enum class byte : unsigned char {};
#endif

template <typename T, size_t Extent = dynamic_extent>
class span {
 public:
  using element_type = T;
  using value_type = typename remove_cv<T>::type;
  using size_type = size_t;
  using difference_type = ptrdiff_t;
  using pointer = element_type*;
  using reference = element_type&;
  using iterator = pointer;

  static constexpr size_type extent = Extent;

  constexpr span() noexcept = default;

  constexpr span(pointer ptr, size_type count) noexcept : data_(ptr), size_(count) {}

  constexpr span(pointer first, pointer last) noexcept
      : data_(first), size_(static_cast<size_type>(last - first)) {}

  template <size_t N,
            typename enable_if<(Extent == dynamic_extent || Extent == N), int>::type = 0>
  constexpr span(element_type (&arr)[N]) noexcept : data_(arr), size_(N) {}

  template <typename U,
            size_t N,
            typename enable_if<is_convertible<U (*)[], element_type (*)[]>::value &&
                                   (Extent == dynamic_extent || Extent == N),
                               int>::type = 0>
  constexpr span(array<U, N>& arr) noexcept : data_(arr.data()), size_(N) {}

  template <typename U,
            size_t N,
            typename enable_if<is_convertible<const U (*)[], element_type (*)[]>::value &&
                                   (Extent == dynamic_extent || Extent == N),
                               int>::type = 0>
  constexpr span(const array<U, N>& arr) noexcept : data_(arr.data()), size_(N) {}

  template <typename U,
            size_t OtherExtent,
            typename enable_if<is_convertible<U (*)[], element_type (*)[]>::value, int>::type = 0>
  constexpr span(const span<U, OtherExtent>& other) noexcept
      : data_(other.data()), size_(other.size()) {}

  [[nodiscard]] constexpr iterator begin() const noexcept { return data_; }
  [[nodiscard]] constexpr iterator end() const noexcept { return data_ + size_; }
  [[nodiscard]] constexpr reference operator[](size_type index) const noexcept { return data_[index]; }
  [[nodiscard]] constexpr pointer data() const noexcept { return data_; }
  [[nodiscard]] constexpr size_type size() const noexcept { return size_; }
  [[nodiscard]] constexpr bool empty() const noexcept { return size_ == 0; }

  [[nodiscard]] constexpr span first(size_type count) const noexcept { return span(data_, count); }

  [[nodiscard]] constexpr span subspan(size_type offset) const noexcept {
    return span(data_ + offset, size_ - offset);
  }

  [[nodiscard]] constexpr span subspan(size_type offset, size_type count) const noexcept {
    return span(data_ + offset, count);
  }

 private:
  pointer data_ = nullptr;
  size_type size_ = 0;
};

}  // namespace std
#endif
