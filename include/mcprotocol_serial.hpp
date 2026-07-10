#pragma once

/// \file mcprotocol_serial.hpp
/// \brief Single-include entry point for the public serial client API.
///
/// Include this header when you want the complete public surface:
///
/// - protocol and request/response types
/// - frame and command codec helpers
/// - the asynchronous serial client
/// - link-direct `Jn\\...` parser helpers
/// - qualified-buffer helper utilities
///
/// The PlatformIO package contains only the transport-agnostic core. Host serial backends and the
/// synchronous host facade are built by CMake from the source repository. Define
/// `MCPROTOCOL_SERIAL_ENABLE_HOST_API=1` only when linking that host-enabled CMake target.

#ifndef MCPROTOCOL_SERIAL_ENABLE_HOST_API
#if defined(ARDUINO) || defined(PLATFORMIO)
#define MCPROTOCOL_SERIAL_ENABLE_HOST_API 0
#else
#define MCPROTOCOL_SERIAL_ENABLE_HOST_API 1
#endif
#endif

#include "mcprotocol/serial/client.hpp"
#include "mcprotocol/serial/codec.hpp"
#include "mcprotocol/serial/high_level.hpp"
#if MCPROTOCOL_SERIAL_ENABLE_HOST_API
#include "mcprotocol/serial/host_sync.hpp"
#endif
#include "mcprotocol/serial/link_direct.hpp"
#include "mcprotocol/serial/qualified_buffer.hpp"
#include "mcprotocol/serial/types.hpp"
#include "mcprotocol/serial/version.hpp"
