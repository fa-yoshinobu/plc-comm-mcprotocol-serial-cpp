#ifndef MCPROTOCOL_SERIAL_HOST_NOW_MS_HPP_
#define MCPROTOCOL_SERIAL_HOST_NOW_MS_HPP_

#include <cstdint>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <time.h>
#endif

namespace mcprotocol::serial {

[[nodiscard]] inline std::uint32_t now_ms() noexcept {
#if defined(_WIN32)
  return static_cast<std::uint32_t>(GetTickCount64());
#else
  struct timespec ts {};
  if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
    return 0U;
  }
  return static_cast<std::uint32_t>(
      (static_cast<std::uint64_t>(ts.tv_sec) * 1000ULL) +
      (static_cast<std::uint64_t>(ts.tv_nsec) / 1000000ULL));
#endif
}

}  // namespace mcprotocol::serial

#endif  // MCPROTOCOL_SERIAL_HOST_NOW_MS_HPP_
