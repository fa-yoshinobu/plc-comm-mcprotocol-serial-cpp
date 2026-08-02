#pragma once

#include "mcprotocol/serial/compat/cstdint.hpp"

namespace mcprotocol::serial {

/// \brief Library-level status code returned by encode, decode, transport, and client operations.
enum class StatusCode : std::uint8_t {
  Ok = 0,
  InvalidArgument,
  Busy,
  Timeout,
  /// The operation requires an established/configured connection or serial session.
  NotConnected,
  /// A local lifecycle close interrupted or rejected the operation.
  Closed,
  Transport,
  Framing,
  SumCheckMismatch,
  Parse,
  UnsupportedConfiguration,
  PlcError,
  BufferTooSmall,
  Cancelled,
  /// A state-changing request may have reached the PLC, but its result was not confirmed.
  OperationOutcomeUnknown,
  /// Host-side temporary storage could not be allocated before communication started.
  OutOfMemory
};

/// \brief Result object returned by most public APIs.
///
/// `plc_error_code` is meaningful when `code == StatusCode::PlcError`. `cause` records the
/// underlying failure when `code == StatusCode::OperationOutcomeUnknown`.
struct Status {
  StatusCode code = StatusCode::Ok;
  std::uint16_t plc_error_code = 0;
  const char* message = "ok";
  /// Machine-readable originating reason when `code == OperationOutcomeUnknown`.
  StatusCode cause = StatusCode::Ok;

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
    std::uint16_t plc_error_code = 0,
    StatusCode cause = StatusCode::Ok) noexcept {
  return Status {code, plc_error_code, message, cause};
}

/// \brief Builds an outcome-unknown status while retaining its machine-readable root reason.
[[nodiscard]] constexpr inline Status make_outcome_unknown_status(
    StatusCode cause,
    const char* message) noexcept {
  return make_status(StatusCode::OperationOutcomeUnknown, message, 0U, cause);
}

}  // namespace mcprotocol::serial
