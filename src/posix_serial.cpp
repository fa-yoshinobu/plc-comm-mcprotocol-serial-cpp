#include "mcprotocol/serial/posix_serial.hpp"

#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include <fcntl.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

namespace mcprotocol::serial {
namespace {

[[nodiscard]] Status transport_error(const char* message) noexcept {
  return make_status(StatusCode::Transport, message);
}

[[nodiscard]] Status transport_errno(const char* action) noexcept {
  switch (errno) {
    case EACCES:
      return transport_error(action == nullptr ? "permission denied" : "permission denied");
    case ENOENT:
      return transport_error(action == nullptr ? "device not found" : "device not found");
    case EBUSY:
      return transport_error(action == nullptr ? "device busy" : "device busy");
    case ENODEV:
      return transport_error(action == nullptr ? "device unavailable" : "device unavailable");
    default:
      return transport_error(action == nullptr ? "transport error" : action);
  }
}

[[nodiscard]] speed_t to_baud_constant(std::uint32_t baud_rate) noexcept {
  switch (baud_rate) {
    case 1200:
      return B1200;
    case 2400:
      return B2400;
    case 4800:
      return B4800;
    case 9600:
      return B9600;
    case 19200:
      return B19200;
    case 38400:
      return B38400;
    case 57600:
      return B57600;
    case 115200:
      return B115200;
    case 230400:
      return B230400;
    case 460800:
      return B460800;
    case 921600:
      return B921600;
    default:
      return 0;
  }
}

[[nodiscard]] Status configure_termios(int fd, const PosixSerialConfig& config) noexcept {
  termios tty {};
  if (::tcgetattr(fd, &tty) != 0) {
    return transport_error("tcgetattr failed");
  }

  const speed_t speed = to_baud_constant(config.baud_rate);
  if (speed == 0) {
    return make_status(StatusCode::InvalidArgument, "Unsupported baud rate");
  }

  ::cfmakeraw(&tty);
  tty.c_cflag |= CLOCAL | CREAD;
  tty.c_cflag &= ~CSIZE;

  switch (config.data_bits) {
    case 5:
      tty.c_cflag |= CS5;
      break;
    case 6:
      tty.c_cflag |= CS6;
      break;
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
  } else if (config.stop_bits == 1) {
    tty.c_cflag &= ~CSTOPB;
  } else {
    return make_status(StatusCode::InvalidArgument, "Unsupported stop bit width");
  }

  tty.c_cflag &= ~(PARENB | PARODD);
  switch (config.parity) {
    case 'N':
    case 'n':
      break;
    case 'E':
    case 'e':
      tty.c_cflag |= PARENB;
      break;
    case 'O':
    case 'o':
      tty.c_cflag |= PARENB;
      tty.c_cflag |= PARODD;
      break;
    default:
      return make_status(StatusCode::InvalidArgument, "Unsupported parity");
  }

  if (config.rts_cts) {
    tty.c_cflag |= CRTSCTS;
  } else {
    tty.c_cflag &= ~CRTSCTS;
  }

  tty.c_cc[VMIN] = 0;
  tty.c_cc[VTIME] = 0;

  if (::cfsetispeed(&tty, speed) != 0 || ::cfsetospeed(&tty, speed) != 0) {
    return transport_error("Failed to configure baud rate");
  }
  if (::tcsetattr(fd, TCSANOW, &tty) != 0) {
    return transport_error("tcsetattr failed");
  }
  if (::tcflush(fd, TCIOFLUSH) != 0) {
    return transport_error("tcflush failed");
  }
  return ok_status();
}

}  // namespace

PosixSerialPort::~PosixSerialPort() {
  close();
}

Status PosixSerialPort::open(const PosixSerialConfig& config) noexcept {
  if (config.device_path.empty()) {
    return make_status(StatusCode::InvalidArgument, "Device path must not be empty");
  }
  if (fd_ >= 0) {
    close();
  }

  fd_ = ::open(config.device_path.data(), O_RDWR | O_NOCTTY | O_SYNC);
  if (fd_ < 0) {
    return transport_errno("open failed");
  }

  const Status status = configure_termios(fd_, config);
  if (!status.ok()) {
    close();
    return status;
  }
  return ok_status();
}

void PosixSerialPort::close() noexcept {
  if (fd_ >= 0) {
    ::close(fd_);
    fd_ = -1;
  }
}

bool PosixSerialPort::is_open() const noexcept {
  return fd_ >= 0;
}

int PosixSerialPort::native_handle() const noexcept {
  return fd_;
}

Status PosixSerialPort::write_all(std::span<const std::byte> bytes) noexcept {
  if (fd_ < 0) {
    return transport_error("Serial port is not open");
  }

  const auto* data = reinterpret_cast<const std::uint8_t*>(bytes.data());
  std::size_t total_written = 0;
  while (total_written < bytes.size()) {
    const ssize_t written = ::write(fd_, data + total_written, bytes.size() - total_written);
    if (written < 0) {
      if (errno == EINTR) {
        continue;
      }
      return transport_errno("write failed");
    }
    total_written += static_cast<std::size_t>(written);
  }
  return ok_status();
}

Status PosixSerialPort::read_some(
    std::span<std::byte> buffer,
    int timeout_ms,
    std::size_t& out_size) noexcept {
  out_size = 0;
  if (fd_ < 0) {
    return transport_error("Serial port is not open");
  }

  pollfd descriptor {};
  descriptor.fd = fd_;
  descriptor.events = POLLIN;
  const int poll_result = ::poll(&descriptor, 1, timeout_ms);
  if (poll_result < 0) {
    if (errno == EINTR) {
      return ok_status();
    }
    return transport_errno("poll failed");
  }
  if (poll_result == 0) {
    return ok_status();
  }
  if ((descriptor.revents & (POLLERR | POLLHUP | POLLNVAL)) != 0) {
    return transport_error("Serial port reported an error");
  }

  const ssize_t received = ::read(fd_, buffer.data(), buffer.size());
  if (received < 0) {
    if (errno == EINTR || errno == EAGAIN) {
      return ok_status();
    }
    return transport_errno("read failed");
  }

  out_size = static_cast<std::size_t>(received);
  return ok_status();
}

Status PosixSerialPort::flush_rx() noexcept {
  if (fd_ < 0) {
    return transport_error("Serial port is not open");
  }
  if (::tcflush(fd_, TCIFLUSH) != 0) {
    return transport_error("tcflush failed");
  }
  return ok_status();
}

Status PosixSerialPort::drain_tx() noexcept {
  if (fd_ < 0) {
    return transport_error("Serial port is not open");
  }
  if (::tcdrain(fd_) != 0) {
    return transport_error("tcdrain failed");
  }
  return ok_status();
}

Status PosixSerialPort::set_rts(bool enabled) noexcept {
  if (fd_ < 0) {
    return transport_error("Serial port is not open");
  }

  int modem_bits = 0;
  if (::ioctl(fd_, TIOCMGET, &modem_bits) != 0) {
    return transport_error("TIOCMGET failed");
  }
  if (enabled) {
    modem_bits |= TIOCM_RTS;
  } else {
    modem_bits &= ~TIOCM_RTS;
  }
  if (::ioctl(fd_, TIOCMSET, &modem_bits) != 0) {
    return transport_error("TIOCMSET failed");
  }
  return ok_status();
}

}  // namespace mcprotocol::serial
