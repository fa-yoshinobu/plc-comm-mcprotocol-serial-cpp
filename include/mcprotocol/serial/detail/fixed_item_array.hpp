#pragma once

#include "mcprotocol/serial/compat/array.hpp"
#include "mcprotocol/serial/compat/cstddef.hpp"

namespace mcprotocol::serial::detail {

template <std::size_t... Indices>
struct FixedItemIndexSequence {};

template <typename Left, typename Right>
struct FixedItemIndexSequenceConcat;

template <std::size_t... Left, std::size_t... Right>
struct FixedItemIndexSequenceConcat<
    FixedItemIndexSequence<Left...>,
    FixedItemIndexSequence<Right...>> {
  using type = FixedItemIndexSequence<Left..., (sizeof...(Left) + Right)...>;
};

template <std::size_t Size>
struct MakeFixedItemIndexSequence {
 private:
  using left = typename MakeFixedItemIndexSequence<Size / 2U>::type;
  using right = typename MakeFixedItemIndexSequence<Size - (Size / 2U)>::type;

 public:
  using type = typename FixedItemIndexSequenceConcat<left, right>::type;
};

template <>
struct MakeFixedItemIndexSequence<0U> {
  using type = FixedItemIndexSequence<>;
};

template <>
struct MakeFixedItemIndexSequence<1U> {
  using type = FixedItemIndexSequence<0U>;
};

template <typename T, std::size_t... Indices>
[[nodiscard]] constexpr std::array<T, sizeof...(Indices)> make_filled_array_impl(
    const T& value,
    FixedItemIndexSequence<Indices...>) {
  return {{(static_cast<void>(Indices), value)...}};
}

template <typename T, std::size_t Size>
[[nodiscard]] constexpr std::array<T, Size> make_filled_array(const T& value) {
  return make_filled_array_impl(value, typename MakeFixedItemIndexSequence<Size>::type {});
}

}  // namespace mcprotocol::serial::detail
