#pragma once

#if defined(MCPROTOCOL_SERIAL_USE_BUNDLED_STDLIB_COMPAT)
#if MCPROTOCOL_SERIAL_USE_BUNDLED_STDLIB_COMPAT
#define MCPROTOCOL_SERIAL_BUNDLED_CSTDINT 1
#else
#include <cstdint>
#endif
#elif defined(__has_include)
#if __has_include(<cstdint>)
#include <cstdint>
#else
#define MCPROTOCOL_SERIAL_BUNDLED_CSTDINT 1
#endif
#else
#include <cstdint>
#endif
#if defined(MCPROTOCOL_SERIAL_BUNDLED_CSTDINT)
#include <stdint.h>

namespace std {

using ::int8_t;
using ::int16_t;
using ::int32_t;
using ::int64_t;
using ::intmax_t;
using ::intptr_t;
using ::uint8_t;
using ::uint16_t;
using ::uint32_t;
using ::uint64_t;
using ::uintmax_t;
using ::uintptr_t;

}  // namespace std
#endif
