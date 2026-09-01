#pragma once

#include <cstdio>
#include <cstdlib>

#ifdef assert
#undef assert
#endif

namespace mcprotocol::serial::test {

[[noreturn]] inline void assertion_failed(
    const char* expression,
    const char* file,
    int line) noexcept {
  std::fprintf(stderr, "%s:%d: assertion failed: %s\n", file, line, expression);
  std::fflush(stderr);
  std::exit(EXIT_FAILURE);
}

}  // namespace mcprotocol::serial::test

#define assert(expression)                                                        \
  do {                                                                            \
    if (!(expression)) {                                                          \
      ::mcprotocol::serial::test::assertion_failed(#expression, __FILE__, __LINE__); \
    }                                                                             \
  } while (false)
