#if defined(_WIN32)

#include "mcprotocol/serial/posix_serial.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

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

[[nodiscard]] Status set_read_timeout(HANDLE h, int timeout_ms) noexcept {
  COMMTIMEOUTS timeouts {};
  timeouts.ReadIntervalTimeout = MAXDWORD;
  timeouts.ReadTotalTimeoutMultiplier = 0;
  timeouts.ReadTotalTimeoutConstant = timeout_ms > 0 ? static_cast<DWORD>(timeout_ms) : 0;
  timeouts.WriteTotalTimeoutMultiplier = 0;
  timeouts.WriteTotalTimeoutConstant = 0;
  if (!SetCommTimeouts(h, &timeouts)) {
    return transport_last_error(GetLastError(), "SetCommTimeouts failed");
  }
  return ok_status();
}

[[nodiscard]] Status configure_comm(HANDLE h, const PosixSerialConfig& config) noexcept {
  DCB dcb {};
  dcb.DCBlength = sizeof(dcb);
  if (!GetCommState(h, &dcb)) {
    return transport_error("GetCommState failed");
  }

  dcb.BaudRate = static_cast<DWORD>(config.baud_rate);

  switch (config.data_bits) {
    case 5: dcb.ByteSize = 5; break;
    case 6: dcb.ByteSize = 6; break;
    case 7: dcb.ByteSize = 7; break;
    case 8: dcb.ByteSize = 8; break;
    default: return make_status(StatusCode::InvalidArgument, "Unsupported data bit width");
  }

  switch (config.stop_bits) {
    case 1: dcb.StopBits = ONESTOPBIT;  break;
    case 2: dcb.StopBits = TWOSTOPBITS; break;
    default: return make_status(StatusCode::InvalidArgument, "Unsupported stop bit width");
  }

  switch (config.parity) {
    case 'N': case 'n': dcb.Parity = NOPARITY;   dcb.fParity = FALSE; break;
    case 'E': case 'e': dcb.Parity = EVENPARITY;  dcb.fParity = TRUE;  break;
    case 'O': case 'o': dcb.Parity = ODDPARITY;   dcb.fParity = TRUE;  break;
    default: return make_status(StatusCode::InvalidArgument, "Unsupported parity");
  }

  dcb.fBinary      = TRUE;
  dcb.fOutxCtsFlow = config.rts_cts ? TRUE : FALSE;
  dcb.fRtsControl  = config.rts_cts ? RTS_CONTROL_HANDSHAKE : RTS_CONTROL_DISABLE;
  dcb.fOutX        = FALSE;
  dcb.fInX         = FALSE;
  dcb.fNull        = FALSE;
  dcb.fAbortOnError = FALSE;

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
  if (config.device_path.empty()) {
    return make_status(StatusCode::InvalidArgument, "Device path must not be empty");
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

Status PosixSerialPort::write_all(std::span<const std::byte> bytes) noexcept {
  if (fd_ < 0) {
    return transport_error("Serial port is not open");
  }
  const HANDLE h = to_handle(fd_);
  const auto* data = reinterpret_cast<const BYTE*>(bytes.data());
  DWORD total_written = 0;
  while (total_written < static_cast<DWORD>(bytes.size())) {
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
      return transport_error("write failed");
    }
    total_written += written;
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
  const HANDLE h = to_handle(fd_);

  Status status = set_read_timeout(h, timeout_ms);
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
  out_size = static_cast<std::size_t>(received);
  return ok_status();
}

Status PosixSerialPort::flush_rx() noexcept {
  if (fd_ < 0) {
    return transport_error("Serial port is not open");
  }
  if (!PurgeComm(to_handle(fd_), PURGE_RXABORT | PURGE_RXCLEAR)) {
    return transport_last_error(GetLastError(), "PurgeComm failed");
  }
  return ok_status();
}

Status PosixSerialPort::drain_tx() noexcept {
  if (fd_ < 0) {
    return transport_error("Serial port is not open");
  }
  if (!FlushFileBuffers(to_handle(fd_))) {
    return transport_last_error(GetLastError(), "FlushFileBuffers failed");
  }
  return ok_status();
}

Status PosixSerialPort::set_rts(bool enabled) noexcept {
  if (fd_ < 0) {
    return transport_error("Serial port is not open");
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
