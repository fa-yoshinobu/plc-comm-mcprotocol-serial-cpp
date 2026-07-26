#pragma once

// Keep bundled standard-library fallbacks behind a library-specific include path. Define
// MCPROTOCOL_SERIAL_USE_BUNDLED_STDLIB_COMPAT=1 to force the fallback on a constrained toolchain,
// or =0 to require the toolchain's standard header. The default probes header availability.
#if defined(MCPROTOCOL_SERIAL_USE_BUNDLED_STDLIB_COMPAT)
#if MCPROTOCOL_SERIAL_USE_BUNDLED_STDLIB_COMPAT
#define MCPROTOCOL_SERIAL_BUNDLED_ALGORITHM 1
#else
#include <algorithm>
#endif
#elif defined(__has_include)
#if __has_include(<algorithm>)
#include <algorithm>
#else
#define MCPROTOCOL_SERIAL_BUNDLED_ALGORITHM 1
#endif
#else
#include <algorithm>
#endif
#if defined(MCPROTOCOL_SERIAL_BUNDLED_ALGORITHM)
#include "mcprotocol/serial/compat/cstddef.hpp"

namespace std {

template <typename InputIt, typename OutputIt>
constexpr OutputIt copy(InputIt first, InputIt last, OutputIt dest) {
  while (first != last) {
    *dest = *first;
    ++first;
    ++dest;
  }
  return dest;
}

template <typename InputIt, typename Size, typename OutputIt>
constexpr OutputIt copy_n(InputIt first, Size count, OutputIt dest) {
  for (Size index = 0; index < count; ++index) {
    *dest = *first;
    ++first;
    ++dest;
  }
  return dest;
}

template <typename InputIt, typename T>
constexpr InputIt find(InputIt first, InputIt last, const T& value) {
  while (first != last) {
    if (*first == value) {
      break;
    }
    ++first;
  }
  return first;
}

template <typename InputIt, typename OutputIt, typename UnaryOperation>
constexpr OutputIt transform(InputIt first, InputIt last, OutputIt dest, UnaryOperation operation) {
  while (first != last) {
    *dest = operation(*first);
    ++first;
    ++dest;
  }
  return dest;
}

template <typename InputIt1, typename InputIt2>
constexpr bool equal(InputIt1 first1, InputIt1 last1, InputIt2 first2) {
  while (first1 != last1) {
    if (!(*first1 == *first2)) {
      return false;
    }
    ++first1;
    ++first2;
  }
  return true;
}

template <typename InputIt>
constexpr ptrdiff_t distance(InputIt first, InputIt last) {
  ptrdiff_t result = 0;
  while (first != last) {
    ++first;
    ++result;
  }
  return result;
}

}  // namespace std
#endif
