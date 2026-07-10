#pragma once

#if defined(MCPROTOCOL_SERIAL_USE_BUNDLED_STDLIB_COMPAT)
#if MCPROTOCOL_SERIAL_USE_BUNDLED_STDLIB_COMPAT
#define MCPROTOCOL_SERIAL_BUNDLED_CSTRING 1
#else
#include <cstring>
#endif
#elif defined(__has_include)
#if __has_include(<cstring>)
#include <cstring>
#else
#define MCPROTOCOL_SERIAL_BUNDLED_CSTRING 1
#endif
#else
#include <cstring>
#endif
#if defined(MCPROTOCOL_SERIAL_BUNDLED_CSTRING)
#include <string.h>

namespace std {

using ::memcmp;
using ::memcpy;
using ::memmove;
using ::memset;
using ::strcmp;
using ::strlen;

}  // namespace std
#endif
