#pragma once

/// \file mcprotocol_serial.hpp
/// \brief Single-include entry point for the public serial client API.
///
/// Include this header when you want the complete public surface:
///
/// - protocol and request/response types
/// - frame and command codec helpers
/// - the asynchronous serial client
/// - qualified-buffer helper utilities

#include "mcprotocol/serial/client.hpp"
#include "mcprotocol/serial/codec.hpp"
#include "mcprotocol/serial/high_level.hpp"
#include "mcprotocol/serial/host_sync.hpp"
#include "mcprotocol/serial/qualified_buffer.hpp"
#include "mcprotocol/serial/types.hpp"
#include "mcprotocol/serial/version.hpp"
