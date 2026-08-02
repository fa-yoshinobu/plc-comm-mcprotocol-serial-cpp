#if defined(__unix__) || defined(__APPLE__)

#include "mcprotocol/serial/posix_serial.hpp"
#include "mcprotocol/serial/detail/posix_serial_settings.hpp"
#include "mcprotocol/serial/detail/yield_first_wait.hpp"
#include "host_now_ms.hpp"

#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <thread>

#include <fcntl.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

namespace mcprotocol::serial {
namespace {

#if defined(__linux__)
using LinuxCc = unsigned char;
using LinuxSpeed = unsigned int;
using LinuxTcflag = unsigned int;

struct LinuxTermios2 {
  LinuxTcflag c_iflag;
  LinuxTcflag c_oflag;
  LinuxTcflag c_cflag;
  LinuxTcflag c_lflag;
  LinuxCc c_line;
  LinuxCc c_cc[19];
  LinuxSpeed c_ispeed;
  LinuxSpeed c_ospeed;
};

constexpr LinuxTcflag kLinuxCbaud = 0x0000100fU;
constexpr LinuxTcflag kLinuxCibaud = 0x100f0000U;
constexpr LinuxTcflag kLinuxBother = 0x00001000U;
constexpr unsigned long kLinuxTcgets2 = _IOR('T', 0x2A, LinuxTermios2);
constexpr unsigned long kLinuxTcsets2 = _IOW('T', 0x2B, LinuxTermios2);
#endif

[[nodiscard]] Status transport_error(const char* message) noexcept {
  return make_status(StatusCode::Transport, message);
}

[[nodiscard]] bool deadline_reached(std::uint32_t now, std::uint32_t deadline) noexcept {
  return static_cast<std::int32_t>(now - deadline) >= 0;
}

[[nodiscard]] int remaining_timeout_ms(std::uint32_t deadline) noexcept {
  const std::uint32_t now = now_ms();
  if (deadline_reached(now, deadline)) {
    return 0;
  }
  const std::uint32_t remaining = deadline - now;
  return remaining > 0x7FFFFFFFU ? 0x7FFFFFFF : static_cast<int>(remaining);
}

[[nodiscard]] Status deadline_timeout(const char* message) noexcept {
  return make_status(StatusCode::Timeout, message);
}

[[nodiscard]] Status make_device_path(
    char* output,
    std::size_t output_size,
    std::string_view device_path) noexcept {
  if (output == nullptr || output_size == 0U || device_path.size() >= output_size) {
    return make_status(StatusCode::InvalidArgument, "Device path is too long");
  }
  std::memcpy(output, device_path.data(), device_path.size());
  output[device_path.size()] = '\0';
  return ok_status();
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
#if defined(B28800)
    case 28800:
      return B28800;
#endif
    case 38400:
      return B38400;
    case 57600:
      return B57600;
    case 115200:
      return B115200;
    case 230400:
      return B230400;
#if defined(B460800)
    case 460800:
      return B460800;
#endif
#if defined(B921600)
    case 921600:
      return B921600;
#endif
    default:
      return 0;
  }
}

#if defined(__linux__)
[[nodiscard]] Status configure_custom_baud_linux(int fd, std::uint32_t baud_rate) noexcept {
  LinuxTermios2 tty {};
  if (::ioctl(fd, kLinuxTcgets2, &tty) != 0) {
    return make_status(StatusCode::InvalidArgument, "Unsupported baud rate");
  }

  tty.c_cflag &= ~(kLinuxCbaud | kLinuxCibaud);
  tty.c_cflag |= kLinuxBother;
  tty.c_ispeed = baud_rate;
  tty.c_ospeed = baud_rate;

  if (::ioctl(fd, kLinuxTcsets2, &tty) != 0) {
    return make_status(StatusCode::InvalidArgument, "Unsupported baud rate");
  }
  return ok_status();
}
#endif

[[nodiscard]] Status configure_termios(int fd, const PosixSerialConfig& config) noexcept {
  termios tty {};
  if (::tcgetattr(fd, &tty) != 0) {
    return transport_error("tcgetattr failed");
  }

  const speed_t speed = to_baud_constant(config.baud_rate);

  const speed_t configured_speed = speed != 0 ? speed : B38400;
  const Status settings_status =
      detail::build_posix_termios(tty, config, configured_speed);
  if (!settings_status.ok()) {
    return settings_status;
  }
  if (::tcsetattr(fd, TCSANOW, &tty) != 0) {
    return transport_error("tcsetattr failed");
  }
#if defined(__linux__)
  if (speed == 0) {
    const Status custom_baud_status = configure_custom_baud_linux(fd, config.baud_rate);
    if (!custom_baud_status.ok()) {
      return custom_baud_status;
    }
  }
#else
  if (speed == 0) {
    return make_status(StatusCode::InvalidArgument, "Unsupported baud rate");
  }
#endif
  if (::tcflush(fd, TCIOFLUSH) != 0) {
    return transport_error("tcflush failed");
  }
  return ok_status();
}

[[nodiscard]] Status enable_exclusive_access(int fd) noexcept {
#if defined(TIOCEXCL)
  if (::ioctl(fd, TIOCEXCL) != 0) {
    if (errno == ENOTTY || errno == EINVAL || errno == ENOSYS) {
      return ok_status();
    }
    return transport_errno("exclusive access failed");
  }
#else
  (void)fd;
#endif
  return ok_status();
}

}  // namespace

PosixSerialPort::~PosixSerialPort() {
  close();
}

Status PosixSerialPort::open(const PosixSerialConfig& config) noexcept {
  const Status config_status = validate_serial_config(config);
  if (!config_status.ok()) {
    return config_status;
  }
  if (fd_ >= 0) {
    close();
  }

  char path_buf[4096];
  const Status path_status = make_device_path(path_buf, sizeof(path_buf), config.device_path);
  if (!path_status.ok()) {
    return path_status;
  }

  int open_flags = O_RDWR | O_NOCTTY | O_SYNC | O_NONBLOCK;
#if defined(O_CLOEXEC)
  open_flags |= O_CLOEXEC;
#endif
  fd_ = static_cast<std::intptr_t>(::open(path_buf, open_flags));
  if (fd_ < 0) {
    return transport_errno("open failed");
  }

  const Status exclusive_status = enable_exclusive_access(static_cast<int>(fd_));
  if (!exclusive_status.ok()) {
    close();
    return exclusive_status;
  }
  const Status status = configure_termios(static_cast<int>(fd_), config);
  if (!status.ok()) {
    close();
    return status;
  }
  return ok_status();
}

void PosixSerialPort::close() noexcept {
  if (fd_ >= 0) {
    const int native_fd = static_cast<int>(fd_);
#if defined(TIOCNXCL)
    (void)::ioctl(native_fd, TIOCNXCL);
#endif
    ::close(native_fd);
    fd_ = -1;
  }
}

bool PosixSerialPort::is_open() const noexcept {
  return fd_ >= 0;
}

std::intptr_t PosixSerialPort::native_handle() const noexcept {
  return fd_;
}

Status PosixSerialPort::write_all_until(
    mcprotocol::serial::Span<const mcprotocol::serial::Byte> bytes,
    std::uint32_t absolute_deadline_ms) noexcept {
  if (fd_ < 0) {
    return make_status(StatusCode::NotConnected, "Serial port is not open");
  }

  const auto* data = reinterpret_cast<const std::uint8_t*>(bytes.data());
  const int native_fd = static_cast<int>(fd_);
  std::size_t total_written = 0;
  while (total_written < bytes.size()) {
    const int remaining = remaining_timeout_ms(absolute_deadline_ms);
    if (remaining == 0) {
      return deadline_timeout("Serial transaction deadline expired during write");
    }
    const ssize_t written = ::write(native_fd, data + total_written, bytes.size() - total_written);
    if (written < 0) {
      if (errno == EINTR) {
        continue;
      }
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        pollfd descriptor {};
        descriptor.fd = native_fd;
        descriptor.events = POLLOUT;
        const int poll_result = ::poll(&descriptor, 1, remaining);
        if (poll_result == 0) {
          return deadline_timeout("Serial transaction deadline expired during write");
        }
        if (poll_result < 0 && errno == EINTR) {
          continue;
        }
        if (poll_result < 0 || (descriptor.revents & (POLLERR | POLLHUP | POLLNVAL)) != 0) {
          return transport_errno("write readiness wait failed");
        }
        continue;
      }
      return transport_errno("write failed");
    }
    if (written == 0) {
      return transport_error("write returned zero bytes");
    }
    total_written += static_cast<std::size_t>(written);
  }
  return ok_status();
}

Status PosixSerialPort::read_some_until(
    mcprotocol::serial::Span<mcprotocol::serial::Byte> buffer,
    std::uint32_t absolute_deadline_ms,
    std::size_t& out_size) noexcept {
  out_size = 0;
  if (fd_ < 0) {
    return make_status(StatusCode::NotConnected, "Serial port is not open");
  }

  const int native_fd = static_cast<int>(fd_);
  pollfd descriptor {};
  descriptor.fd = native_fd;
  descriptor.events = POLLIN;
  const int remaining = remaining_timeout_ms(absolute_deadline_ms);
  if (remaining == 0) {
    return deadline_timeout("Serial transaction deadline expired during receive");
  }
  const int poll_result = ::poll(&descriptor, 1, remaining);
  if (poll_result < 0) {
    if (errno == EINTR) {
      return ok_status();
    }
    return transport_errno("poll failed");
  }
  if (poll_result == 0) {
    return deadline_timeout("Serial transaction deadline expired during receive");
  }
  if ((descriptor.revents & (POLLERR | POLLHUP | POLLNVAL)) != 0) {
    return transport_error("Serial port reported an error");
  }

  const ssize_t received = ::read(native_fd, buffer.data(), buffer.size());
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
    return make_status(StatusCode::NotConnected, "Serial port is not open");
  }
  if (::tcflush(static_cast<int>(fd_), TCIFLUSH) != 0) {
    return transport_error("tcflush failed");
  }
  return ok_status();
}

Status PosixSerialPort::drain_tx_until(std::uint32_t absolute_deadline_ms) noexcept {
  if (fd_ < 0) {
    return make_status(StatusCode::NotConnected, "Serial port is not open");
  }
  const int native_fd = static_cast<int>(fd_);
#if defined(TIOCOUTQ)
  auto remaining_timeout = [](std::uint32_t deadline) noexcept -> std::uint32_t {
    return static_cast<std::uint32_t>(remaining_timeout_ms(deadline));
  };
  auto query_pending = [native_fd](std::uint64_t& out_pending) noexcept -> Status {
    int pending = 0;
    if (::ioctl(native_fd, TIOCOUTQ, &pending) != 0) {
      return transport_errno("TIOCOUTQ failed");
    }
    out_pending = static_cast<std::uint64_t>(pending);
    return ok_status();
  };
  auto yield_now = []() noexcept { std::this_thread::yield(); };
  auto sleep_one = [](std::uint32_t delay_ms) noexcept {
    (void)::poll(nullptr, 0, static_cast<int>(delay_ms));
  };
  return detail::drain_tx_with_yield_first(
      absolute_deadline_ms,
      "Serial transaction deadline expired during drain",
      remaining_timeout,
      query_pending,
      yield_now,
      sleep_one);
#else
  (void)absolute_deadline_ms;
  return make_status(
      StatusCode::UnsupportedConfiguration,
      "Deadline-bounded serial drain is unavailable on this POSIX platform");
#endif
}

Status PosixSerialPort::set_rts(bool enabled) noexcept {
  if (fd_ < 0) {
    return make_status(StatusCode::NotConnected, "Serial port is not open");
  }

  const int native_fd = static_cast<int>(fd_);
  int modem_bits = 0;
  if (::ioctl(native_fd, TIOCMGET, &modem_bits) != 0) {
    return transport_error("TIOCMGET failed");
  }
  if (enabled) {
    modem_bits |= TIOCM_RTS;
  } else {
    modem_bits &= ~TIOCM_RTS;
  }
  if (::ioctl(native_fd, TIOCMSET, &modem_bits) != 0) {
    return transport_error("TIOCMSET failed");
  }
  return ok_status();
}

}  // namespace mcprotocol::serial

#endif
