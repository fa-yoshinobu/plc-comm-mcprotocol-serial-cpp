#pragma once

#if defined(MCPROTOCOL_SERIAL_USE_BUNDLED_STDLIB_COMPAT)
#if MCPROTOCOL_SERIAL_USE_BUNDLED_STDLIB_COMPAT
#define MCPROTOCOL_SERIAL_BUNDLED_CSTDDEF 1
#else
#include <cstddef>
#endif
#elif defined(__has_include)
#if __has_include(<cstddef>)
#include <cstddef>
#else
#define MCPROTOCOL_SERIAL_BUNDLED_CSTDDEF 1
#endif
#else
#include <cstddef>
#endif
#if defined(MCPROTOCOL_SERIAL_BUNDLED_CSTDDEF)
#include <stddef.h>

namespace std {

using ::ptrdiff_t;
using ::size_t;
using nullptr_t = decltype(nullptr);

}  // namespace std
#endif
