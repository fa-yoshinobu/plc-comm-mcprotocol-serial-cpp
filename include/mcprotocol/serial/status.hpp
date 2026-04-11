#pragma once

#include <cstdint>

namespace mcprotocol::serial {

/// \brief Library-level status code returned by encode, decode, transport, and client operations.
enum class StatusCode : std::uint8_t {
  Ok = 0,
  InvalidArgument,
  Busy,
  Timeout,
  Transport,
  Framing,
  SumCheckMismatch,
  Parse,
  UnsupportedConfiguration,
  PlcError,
  BufferTooSmall,
  Cancelled
};

/// \brief Result object returned by most public APIs.
///
/// `plc_error_code` is meaningful when `code == StatusCode::PlcError`.
struct Status {
  StatusCode code = StatusCode::Ok;
  std::uint16_t plc_error_code = 0;
  const char* message = "ok";

  [[nodiscard]] constexpr bool ok() const noexcept {
    return code == StatusCode::Ok;
  }
};

/// \brief Returns the default success status.
[[nodiscard]] constexpr inline Status ok_status() noexcept {
  return {};
}

/// \brief Builds a status value with an optional PLC end code.
[[nodiscard]] constexpr inline Status make_status(
    StatusCode code,
    const char* message,
    std::uint16_t plc_error_code = 0) noexcept {
  return Status {.code = code, .plc_error_code = plc_error_code, .message = message};
}

}  // namespace mcprotocol::serial
