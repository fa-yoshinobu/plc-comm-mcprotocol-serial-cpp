#pragma once

#include "mcprotocol/serial/compat/array.hpp"
#include "mcprotocol/serial/compat/cstddef.hpp"

namespace mcprotocol::serial {

namespace detail {

template <bool Condition, typename T = void>
struct SpanEnableIf {};

template <typename T>
struct SpanEnableIf<true, T> {
  using type = T;
};

template <typename T>
struct SpanIsConst {
  static constexpr bool value = false;
};

template <typename T>
struct SpanIsConst<const T> {
  static constexpr bool value = true;
};

template <typename T>
struct SpanValueType {
  using type = T;
};

template <typename T>
struct SpanValueType<const T> {
  using type = T;
};

template <typename T>
struct SpanValueType<volatile T> {
  using type = T;
};

template <typename T>
struct SpanValueType<const volatile T> {
  using type = T;
};

}  // namespace detail

/// \brief Non-owning contiguous range used by the public C++17 API.
///
/// The library owns this type instead of adding a pre-C++20 `span` implementation to
/// `namespace std`, which is undefined behavior. The pointed-to storage must outlive the Span.
template <typename T>
class Span {
 public:
  using element_type = T;
  using value_type = typename detail::SpanValueType<T>::type;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using pointer = element_type*;
  using reference = element_type&;
  using iterator = pointer;

  constexpr Span() noexcept = default;

  /// \brief Creates a view over `count` live elements beginning at `ptr`.
  ///
  /// `ptr` must be non-null when `count != 0`; the caller owns that storage and lifetime.
  constexpr Span(pointer ptr, size_type count) noexcept : data_(ptr), size_(count) {}

  /// \brief Creates a view over the valid half-open range `[first, last)`.
  ///
  /// Both pointers must belong to the same live array object, and `last` must not precede `first`.
  constexpr Span(pointer first, pointer last) noexcept
      : data_(first), size_(static_cast<size_type>(last - first)) {}

  template <std::size_t N>
  constexpr Span(element_type (&values)[N]) noexcept : data_(values), size_(N) {}

  template <std::size_t N>
  constexpr Span(std::array<value_type, N>& values) noexcept : data_(values.data()), size_(N) {}

  template <
      std::size_t N,
      typename U = T,
      typename detail::SpanEnableIf<detail::SpanIsConst<U>::value, int>::type = 0>
  constexpr Span(const std::array<value_type, N>& values) noexcept
      : data_(values.data()), size_(N) {}

  template <std::size_t N>
  Span(std::array<value_type, N>&&) = delete;

  template <std::size_t N>
  Span(const std::array<value_type, N>&&) = delete;

  template <
      typename U = T,
      typename detail::SpanEnableIf<detail::SpanIsConst<U>::value, int>::type = 0>
  constexpr Span(const Span<value_type>& other) noexcept
      : data_(other.data()), size_(other.size()) {}

  [[nodiscard]] constexpr iterator begin() const noexcept { return data_; }
  [[nodiscard]] constexpr iterator end() const noexcept {
    return size_ == 0U ? data_ : data_ + size_;
  }
  /// \brief Returns one element; `index < size()` is a caller precondition.
  [[nodiscard]] constexpr reference operator[](size_type index) const noexcept { return data_[index]; }
  /// \brief Returns a pointer to one element, or null when `index` is outside the view.
  [[nodiscard]] constexpr pointer try_at(size_type index) const noexcept {
    return index < size_ ? data_ + index : nullptr;
  }
  [[nodiscard]] constexpr pointer data() const noexcept { return data_; }
  [[nodiscard]] constexpr size_type size() const noexcept { return size_; }
  [[nodiscard]] constexpr bool empty() const noexcept { return size_ == 0U; }

  /// \brief Returns the first `count` elements through `out`, or false without an invalid view.
  [[nodiscard]] constexpr bool try_first(size_type count, Span& out) const noexcept {
    out = Span {};
    if (count > size_) {
      return false;
    }
    out = Span(data_, count);
    return true;
  }

  /// \brief Returns the suffix at `offset` through `out`, or false when offset exceeds size.
  [[nodiscard]] constexpr bool try_subspan(size_type offset, Span& out) const noexcept {
    out = Span {};
    if (offset > size_) {
      return false;
    }
    out = Span(offset == 0U ? data_ : data_ + offset, size_ - offset);
    return true;
  }

  /// \brief Returns the checked `[offset, offset + count)` range through `out`.
  [[nodiscard]] constexpr bool try_subspan(
      size_type offset,
      size_type count,
      Span& out) const noexcept {
    out = Span {};
    if (offset > size_ || count > (size_ - offset)) {
      return false;
    }
    out = Span(offset == 0U ? data_ : data_ + offset, count);
    return true;
  }

 private:
  pointer data_ = nullptr;
  size_type size_ = 0U;
};

}  // namespace mcprotocol::serial
