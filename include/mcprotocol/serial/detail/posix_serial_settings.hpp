#pragma once

#if defined(__unix__) || defined(__APPLE__)

#include <cstring>
#include <termios.h>

#include "mcprotocol/serial/posix_serial.hpp"

namespace mcprotocol::serial::detail {

/// \brief Overwrites every behavior-affecting termios field for one configured speed.
[[nodiscard]] inline Status build_posix_termios(
    termios& tty,
    const PosixSerialConfig& config,
    speed_t configured_speed) noexcept {
  tty.c_iflag = 0;
  tty.c_oflag = 0;
  tty.c_lflag = 0;
  tty.c_cflag = CLOCAL | CREAD;
#if defined(__linux__)
  tty.c_line = 0;
#endif
  std::memset(tty.c_cc, 0, sizeof(tty.c_cc));

  switch (config.data_bits) {
    case 7:
      tty.c_cflag |= CS7;
      break;
    case 8:
      tty.c_cflag |= CS8;
      break;
    default:
      return make_status(StatusCode::InvalidArgument, "Unsupported data bit width");
  }

  if (config.stop_bits == 2) {
    tty.c_cflag |= CSTOPB;
  } else if (config.stop_bits != 1) {
    return make_status(StatusCode::InvalidArgument, "Unsupported stop bit width");
  }

  switch (config.parity) {
    case SerialParity::None:
      break;
    case SerialParity::Even:
      tty.c_cflag |= PARENB;
      break;
    case SerialParity::Odd:
      tty.c_cflag |= PARENB | PARODD;
      break;
    default:
      return make_status(StatusCode::InvalidArgument, "Unsupported parity");
  }

  switch (config.hardware_flow_control) {
    case HardwareFlowControl::None:
      break;
    case HardwareFlowControl::RtsCts:
#if defined(CRTSCTS)
      tty.c_cflag |= CRTSCTS;
#elif defined(CCTS_OFLOW) && defined(CRTS_IFLOW)
      tty.c_cflag |= CCTS_OFLOW | CRTS_IFLOW;
#else
      return make_status(StatusCode::InvalidArgument, "RTS/CTS flow control is unavailable");
#endif
      break;
    default:
      return make_status(StatusCode::InvalidArgument, "Unsupported hardware flow control");
  }

  if (::cfsetispeed(&tty, configured_speed) != 0 ||
      ::cfsetospeed(&tty, configured_speed) != 0) {
    return make_status(StatusCode::Transport, "Failed to configure baud rate");
  }
  return ok_status();
}

}  // namespace mcprotocol::serial::detail

#endif
