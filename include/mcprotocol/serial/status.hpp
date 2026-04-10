#pragma once

#include <cstdint>

namespace mcprotocol::serial {

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

struct Status {
  StatusCode code = StatusCode::Ok;
  std::uint16_t plc_error_code = 0;
  const char* message = "ok";

  [[nodiscard]] constexpr bool ok() const noexcept {
    return code == StatusCode::Ok;
  }
};

[[nodiscard]] constexpr inline Status ok_status() noexcept {
  return {};
}

[[nodiscard]] constexpr inline Status make_status(
    StatusCode code,
    const char* message,
    std::uint16_t plc_error_code = 0) noexcept {
  return Status {.code = code, .plc_error_code = plc_error_code, .message = message};
}

}  // namespace mcprotocol::serial
