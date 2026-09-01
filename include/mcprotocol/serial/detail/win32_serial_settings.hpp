#pragma once

#if defined(_WIN32)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include "mcprotocol/serial/posix_serial.hpp"

#include <cstdint>

namespace mcprotocol::serial::detail {

[[nodiscard]] inline Status validate_win32_io_size(
    std::uint64_t size,
    const char* message) noexcept {
  if (size > static_cast<std::uint64_t>(MAXDWORD)) {
    return make_status(StatusCode::InvalidArgument, message);
  }
  return ok_status();
}

[[nodiscard]] inline COMMTIMEOUTS build_win32_deadline_timeouts(
    DWORD timeout_ms) noexcept {
  COMMTIMEOUTS timeouts {};
  timeouts.ReadIntervalTimeout = MAXDWORD;
  timeouts.ReadTotalTimeoutMultiplier = MAXDWORD;
  timeouts.ReadTotalTimeoutConstant = timeout_ms;
  timeouts.WriteTotalTimeoutMultiplier = 0;
  timeouts.WriteTotalTimeoutConstant = timeout_ms;
  return timeouts;
}

/// \brief Overwrites every behavior field while preserving driver-reserved DCB state.
[[nodiscard]] inline Status build_win32_dcb(
    DCB& dcb,
    const PosixSerialConfig& config) noexcept {
  dcb.DCBlength = sizeof(dcb);
  dcb.BaudRate = static_cast<DWORD>(config.baud_rate);

  switch (config.data_bits) {
    case 7:
      dcb.ByteSize = static_cast<BYTE>(7);
      break;
    case 8:
      dcb.ByteSize = static_cast<BYTE>(8);
      break;
    default:
      return make_status(StatusCode::InvalidArgument, "Unsupported data bit width");
  }

  switch (config.stop_bits) {
    case 1:
      dcb.StopBits = ONESTOPBIT;
      break;
    case 2:
      dcb.StopBits = TWOSTOPBITS;
      break;
    default:
      return make_status(StatusCode::InvalidArgument, "Unsupported stop bit width");
  }

  switch (config.parity) {
    case SerialParity::None:
      dcb.Parity = NOPARITY;
      dcb.fParity = FALSE;
      break;
    case SerialParity::Even:
      dcb.Parity = EVENPARITY;
      dcb.fParity = TRUE;
      break;
    case SerialParity::Odd:
      dcb.Parity = ODDPARITY;
      dcb.fParity = TRUE;
      break;
    default:
      return make_status(StatusCode::InvalidArgument, "Unsupported parity");
  }

  BOOL use_rts_cts = FALSE;
  switch (config.hardware_flow_control) {
    case HardwareFlowControl::None:
      break;
    case HardwareFlowControl::RtsCts:
      use_rts_cts = TRUE;
      break;
    default:
      return make_status(StatusCode::InvalidArgument, "Unsupported hardware flow control");
  }
  dcb.fBinary = TRUE;
  dcb.fOutxCtsFlow = use_rts_cts;
  dcb.fOutxDsrFlow = FALSE;
  dcb.fDtrControl = DTR_CONTROL_DISABLE;
  dcb.fDsrSensitivity = FALSE;
  dcb.fTXContinueOnXoff = FALSE;
  dcb.fOutX = FALSE;
  dcb.fInX = FALSE;
  dcb.fErrorChar = FALSE;
  dcb.fNull = FALSE;
  dcb.fRtsControl = use_rts_cts ? RTS_CONTROL_HANDSHAKE : RTS_CONTROL_DISABLE;
  dcb.fAbortOnError = FALSE;
  dcb.XonLim = 0;
  dcb.XoffLim = 0;
  dcb.XonChar = static_cast<char>(0x11);
  dcb.XoffChar = static_cast<char>(0x13);
  dcb.ErrorChar = 0;
  dcb.EofChar = 0;
  dcb.EvtChar = 0;
  return ok_status();
}

}  // namespace mcprotocol::serial::detail

#endif
