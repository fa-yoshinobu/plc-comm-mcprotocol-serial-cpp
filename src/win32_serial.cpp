#if defined(_WIN32)

#include "mcprotocol/serial/posix_serial.hpp"
#include "mcprotocol/serial/detail/win32_serial_settings.hpp"
#include "mcprotocol/serial/detail/yield_first_wait.hpp"
#include "host_now_ms.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <thread>

// Avoid including winsock2 conflicts; use minimal Windows headers.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace mcprotocol::serial {
namespace {

[[nodiscard]] Status transport_error(const char* message) noexcept {
  return make_status(StatusCode::Transport, message);
}

[[nodiscard]] bool deadline_reached(std::uint32_t now, std::uint32_t deadline) noexcept {
  return static_cast<std::int32_t>(now - deadline) >= 0;
}

[[nodiscard]] DWORD remaining_timeout_ms(std::uint32_t deadline) noexcept {
  const std::uint32_t now = now_ms();
  if (deadline_reached(now, deadline)) {
    return 0U;
  }
  return static_cast<DWORD>(deadline - now);
}

[[nodiscard]] Status deadline_timeout(const char* message) noexcept {
  return make_status(StatusCode::Timeout, message);
}

[[nodiscard]] HANDLE to_handle(std::intptr_t fd) noexcept {
  return reinterpret_cast<HANDLE>(fd);
}

[[nodiscard]] constexpr bool has_windows_device_prefix(std::string_view path) noexcept {
  return path.size() >= 4 &&
         path[0] == '\\' &&
         path[1] == '\\' &&
         path[2] == '.' &&
         path[3] == '\\';
}

// Build a proper \\.\COMn path. mcprotocol_cli passes bare "COM3" style strings.
// COM ports above COM9 require the \\.\COMn form; always use it for safety.
[[nodiscard]] Status make_device_path(
    char* buf,
    std::size_t buf_size,
    std::string_view path) noexcept {
  if (path.empty()) {
    return make_status(StatusCode::InvalidArgument, "Device path must not be empty");
  }
  if (path.find('\0') != std::string_view::npos) {
    return make_status(StatusCode::InvalidArgument, "Device path must not contain embedded NUL");
  }

  const std::string_view normalized = has_windows_device_prefix(path) ? path : std::string_view {};
  const std::size_t required_size =
      normalized.empty() ? (4U + path.size() + 1U) : (normalized.size() + 1U);
  if (required_size > buf_size) {
    return make_status(StatusCode::InvalidArgument, "Device path is too long");
  }

  if (!normalized.empty()) {
    std::memcpy(buf, normalized.data(), normalized.size());
    buf[normalized.size()] = '\0';
    return ok_status();
  }

  const int written = std::snprintf(buf, buf_size, "\\\\.\\%.*s", static_cast<int>(path.size()), path.data());
  if (written < 0 || static_cast<std::size_t>(written) >= buf_size) {
    return make_status(StatusCode::InvalidArgument, "Device path is too long");
  }
  return ok_status();
}

[[nodiscard]] Status transport_last_error(DWORD error, const char* fallback_message) noexcept {
  switch (error) {
    case ERROR_FILE_NOT_FOUND:
    case ERROR_PATH_NOT_FOUND:
    case ERROR_INVALID_NAME:
      return transport_error("device not found");
    case ERROR_ACCESS_DENIED:
      return transport_error("permission denied");
    case ERROR_SHARING_VIOLATION:
    case ERROR_BUSY:
      return transport_error("device busy");
    default:
      return transport_error(fallback_message);
  }
}

[[nodiscard]] Status set_nonblocking_read_timeouts(HANDLE h) noexcept {
  COMMTIMEOUTS timeouts {};
  timeouts.ReadIntervalTimeout = MAXDWORD;
  timeouts.ReadTotalTimeoutMultiplier = 0;
  timeouts.ReadTotalTimeoutConstant = 0;
  timeouts.WriteTotalTimeoutMultiplier = 0;
  timeouts.WriteTotalTimeoutConstant = 0;
  if (!SetCommTimeouts(h, &timeouts)) {
    return transport_last_error(GetLastError(), "SetCommTimeouts failed");
  }
  return ok_status();
}

[[nodiscard]] Status set_io_deadline_timeouts(HANDLE h, DWORD timeout_ms) noexcept {
  COMMTIMEOUTS timeouts {};
  timeouts.ReadIntervalTimeout = MAXDWORD;
  timeouts.ReadTotalTimeoutMultiplier = MAXDWORD;
  timeouts.ReadTotalTimeoutConstant = timeout_ms;
  timeouts.WriteTotalTimeoutMultiplier = 0;
  timeouts.WriteTotalTimeoutConstant = timeout_ms;
  if (!SetCommTimeouts(h, &timeouts)) {
    return transport_last_error(GetLastError(), "SetCommTimeouts failed");
  }
  return ok_status();
}

[[nodiscard]] Status configure_comm(HANDLE h, const PosixSerialConfig& config) noexcept {
  DCB dcb {};
  dcb.DCBlength = sizeof(dcb);
  if (!GetCommState(h, &dcb)) {
    return transport_last_error(GetLastError(), "GetCommState failed");
  }
  const Status dcb_status = detail::build_win32_dcb(dcb, config);
  if (!dcb_status.ok()) {
    return dcb_status;
  }

  if (!SetCommState(h, &dcb)) {
    return transport_last_error(GetLastError(), "SetCommState failed");
  }

  const Status timeout_status = set_nonblocking_read_timeouts(h);
  if (!timeout_status.ok()) {
    return timeout_status;
  }

  if (!PurgeComm(h, PURGE_RXCLEAR | PURGE_TXCLEAR)) {
    return transport_last_error(GetLastError(), "PurgeComm failed");
  }
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

  char path_buf[64];
  const Status path_status = make_device_path(path_buf, sizeof(path_buf), config.device_path);
  if (!path_status.ok()) {
    return path_status;
  }

  HANDLE h = CreateFileA(
      path_buf,
      GENERIC_READ | GENERIC_WRITE,
      0,
      nullptr,
      OPEN_EXISTING,
      FILE_ATTRIBUTE_NORMAL,
      nullptr);
  if (h == INVALID_HANDLE_VALUE) {
    return transport_last_error(GetLastError(), "open failed");
  }

  fd_ = reinterpret_cast<std::intptr_t>(h);
  const Status status = configure_comm(h, config);
  if (!status.ok()) {
    close();
    return status;
  }
  return ok_status();
}

void PosixSerialPort::close() noexcept {
  if (fd_ >= 0) {
    CloseHandle(to_handle(fd_));
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
  const Status size_status = detail::validate_win32_io_size(
      static_cast<std::uint64_t>(bytes.size()),
      "Serial write size exceeds the Win32 DWORD limit");
  if (!size_status.ok()) {
    return size_status;
  }
  const HANDLE h = to_handle(fd_);
  const auto* data = reinterpret_cast<const BYTE*>(bytes.data());
  DWORD total_written = 0;
  while (total_written < static_cast<DWORD>(bytes.size())) {
    const DWORD remaining = remaining_timeout_ms(absolute_deadline_ms);
    if (remaining == 0U) {
      return deadline_timeout("Serial transaction deadline expired during write");
    }
    const Status timeout_status = set_io_deadline_timeouts(h, remaining);
    if (!timeout_status.ok()) {
      return timeout_status;
    }
    DWORD written = 0;
    if (!WriteFile(
            h,
            data + total_written,
            static_cast<DWORD>(bytes.size()) - total_written,
            &written,
            nullptr)) {
      return transport_last_error(GetLastError(), "write failed");
    }
    if (written == 0) {
      return deadline_reached(now_ms(), absolute_deadline_ms)
                 ? deadline_timeout("Serial transaction deadline expired during write")
                 : transport_error("write made no progress");
    }
    total_written += written;
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
  const Status size_status = detail::validate_win32_io_size(
      static_cast<std::uint64_t>(buffer.size()),
      "Serial read size exceeds the Win32 DWORD limit");
  if (!size_status.ok()) {
    return size_status;
  }
  const HANDLE h = to_handle(fd_);

  const DWORD remaining = remaining_timeout_ms(absolute_deadline_ms);
  if (remaining == 0U) {
    return deadline_timeout("Serial transaction deadline expired during receive");
  }
  Status status = set_io_deadline_timeouts(h, remaining);
  if (!status.ok()) {
    return status;
  }

  DWORD received = 0;
  const BOOL ok = ReadFile(h, buffer.data(), static_cast<DWORD>(buffer.size()), &received, nullptr);
  const DWORD read_error = ok ? ERROR_SUCCESS : GetLastError();

  const Status restore_status = set_nonblocking_read_timeouts(h);
  if (!restore_status.ok()) {
    return restore_status;
  }

  if (!ok) {
    return transport_last_error(read_error, "read failed");
  }
  if (received == 0U && deadline_reached(now_ms(), absolute_deadline_ms)) {
    return deadline_timeout("Serial transaction deadline expired during receive");
  }
  out_size = static_cast<std::size_t>(received);
  return ok_status();
}

Status PosixSerialPort::flush_rx() noexcept {
  if (fd_ < 0) {
    return make_status(StatusCode::NotConnected, "Serial port is not open");
  }
  if (!PurgeComm(to_handle(fd_), PURGE_RXABORT | PURGE_RXCLEAR)) {
    return transport_last_error(GetLastError(), "PurgeComm failed");
  }
  return ok_status();
}

Status PosixSerialPort::drain_tx_until(std::uint32_t absolute_deadline_ms) noexcept {
  if (fd_ < 0) {
    return make_status(StatusCode::NotConnected, "Serial port is not open");
  }
  const HANDLE h = to_handle(fd_);
  auto remaining_timeout = [](std::uint32_t deadline) noexcept -> std::uint32_t {
    return static_cast<std::uint32_t>(remaining_timeout_ms(deadline));
  };
  auto query_pending = [h](std::uint64_t& out_pending) noexcept -> Status {
    DWORD errors = 0U;
    COMSTAT status {};
    if (!ClearCommError(h, &errors, &status)) {
      return transport_last_error(GetLastError(), "ClearCommError failed during drain");
    }
    out_pending = static_cast<std::uint64_t>(status.cbOutQue);
    return ok_status();
  };
  auto yield_now = []() noexcept { std::this_thread::yield(); };
  auto sleep_one = [](std::uint32_t delay_ms) noexcept { Sleep(static_cast<DWORD>(delay_ms)); };
  return detail::drain_tx_with_yield_first(
      absolute_deadline_ms,
      "Serial transaction deadline expired during drain",
      remaining_timeout,
      query_pending,
      yield_now,
      sleep_one);
}

Status PosixSerialPort::set_rts(bool enabled) noexcept {
  if (fd_ < 0) {
    return make_status(StatusCode::NotConnected, "Serial port is not open");
  }
  const BOOL ok = EscapeCommFunction(
      to_handle(fd_),
      enabled ? SETRTS : CLRRTS);
  if (!ok) {
    return transport_last_error(GetLastError(), "EscapeCommFunction RTS failed");
  }
  return ok_status();
}

}  // namespace mcprotocol::serial

#endif  // _WIN32
