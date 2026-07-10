#pragma once

#if defined(MCPROTOCOL_SERIAL_USE_BUNDLED_STDLIB_COMPAT)
#if MCPROTOCOL_SERIAL_USE_BUNDLED_STDLIB_COMPAT
#define MCPROTOCOL_SERIAL_BUNDLED_CCTYPE 1
#else
#include <cctype>
#endif
#elif defined(__has_include)
#if __has_include(<cctype>)
#include <cctype>
#else
#define MCPROTOCOL_SERIAL_BUNDLED_CCTYPE 1
#endif
#else
#include <cctype>
#endif
#if defined(MCPROTOCOL_SERIAL_BUNDLED_CCTYPE)
#include <ctype.h>

namespace std {

using ::isalnum;
using ::isalpha;
using ::isdigit;
using ::isprint;
using ::isspace;
using ::isxdigit;
using ::tolower;
using ::toupper;

}  // namespace std
#endif
