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

#ifndef MCPROTOCOL_SERIAL_ENABLE_HOST_API
#if defined(ARDUINO)
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
