#pragma once

// Opt-in only: mcprotocol_serial.hpp deliberately does not include this header.
#if !defined(ARDUINO) || !defined(ARDUINO_ARCH_ESP32)
#error "mcprotocol_serial_arduino_esp32.hpp requires Arduino-ESP32"
#else

#if __cplusplus < 201703L
#error "Esp32UartClient requires C++17; enable -std=gnu++17 and do not remove it in build_unflags"
#endif

#include <Arduino.h>
#include <HardwareSerial.h>
#include <optional>
#include <driver/gpio.h>
#include <driver/uart.h>
#include "mcprotocol/serial/detail/uart_client.hpp"

namespace mcprotocol::serial {

enum class Esp32Direction { External, Rs485Rts };

struct Esp32UartConfig {
  std::uint32_t baud = 19200;
  std::uint32_t format = SERIAL_8E1;
  int rx_pin = -1;
  int tx_pin = -1;
  Esp32Direction direction = Esp32Direction::External;
  int rts_pin = -1;
  std::size_t rx_buffer_bytes = 1024;
};

namespace detail {
class Esp32UartTransport {
 public:
  explicit Esp32UartTransport(std::uint8_t port) : port_(port) {}
  Esp32UartTransport(const Esp32UartTransport&) = delete;
  Esp32UartTransport& operator=(const Esp32UartTransport&) = delete;
  ~Esp32UartTransport() { abort(); }

  Status begin(const Esp32UartConfig& config) {
    if (open_) return make_status(StatusCode::Busy, "UART is already open");
    if (port_ >= SOC_UART_NUM || !GPIO_IS_VALID_GPIO(config.rx_pin)
        || !GPIO_IS_VALID_OUTPUT_GPIO(config.tx_pin) || config.rx_pin == config.tx_pin
        || config.baud < 300 || config.baud > 115200
        || config.rx_buffer_bytes < 256 || config.rx_buffer_bytes > 65536
        || (config.direction != Esp32Direction::External && config.direction != Esp32Direction::Rs485Rts)
        || (config.direction == Esp32Direction::External && config.rts_pin != -1)
        || (config.direction == Esp32Direction::Rs485Rts
            && (!GPIO_IS_VALID_OUTPUT_GPIO(config.rts_pin)
                || config.rts_pin == config.tx_pin || config.rts_pin == config.rx_pin)))
      return make_status(StatusCode::InvalidArgument, "Invalid UART port, pins, baud, buffer or direction");
    switch (config.format) {
      case SERIAL_8N1: case SERIAL_8N2: case SERIAL_8E1: case SERIAL_8E2:
      case SERIAL_8O1: case SERIAL_8O2: case SERIAL_7N1: case SERIAL_7N2:
      case SERIAL_7E1: case SERIAL_7E2: case SERIAL_7O1: case SERIAL_7O2: break;
      default: return make_status(StatusCode::InvalidArgument, "UART requires 7 or 8 data bits");
    }
    // Do not take over Serial1, a console, Modbus, or another adapter's UART.
    if (uart_is_driver_installed(port()))
      return make_status(StatusCode::Busy, "UART port is owned by another driver");
    config_ = config;
    // Construct only after checking ownership: HardwareSerial's destructor calls
    // end() even when its instance never opened the port (Arduino-ESP32 2.x).
    serial_.emplace(port_);
    serial_->setRxBufferSize(config.rx_buffer_bytes);
    serial_->setTxBufferSize(256);
    serial_->begin(config.baud, config.format, config.rx_pin, config.tx_pin);
    open_ = uart_is_driver_installed(port());
    if (!open_) {
      serial_.reset();
      return make_status(StatusCode::Transport, "UART initialization failed");
    }
    const bool pins = serial_->setPins(config.rx_pin, config.tx_pin, -1, config.rts_pin);
    const bool flow = serial_->setHwFlowCtrlMode(UART_HW_FLOWCTRL_DISABLE);
    const bool mode = serial_->setMode(config.direction == Esp32Direction::Rs485Rts
        ? UART_MODE_RS485_HALF_DUPLEX : UART_MODE_UART);
    if (!pins || !flow || !mode) {
      abort();
      return make_status(StatusCode::Transport, "UART direction setup failed");
    }
    return ok_status();
  }
  bool ready() const noexcept { return open_ && uart_is_driver_installed(port()); }
  std::uint32_t now_ms() const noexcept { return millis(); }
  void idle() const { delay(1); }
  int write_some(const std::uint8_t* data, std::size_t size) {
    if (!ready()) return -1;
    std::size_t available = 0;
    if (uart_get_tx_buffer_free_size(port(), &available) != ESP_OK) return -1;
    // Reserve ring-buffer item overhead; exclusive single-task ownership required.
    available = available > 32 ? available - 32 : 0;
    if (size > 64) size = 64;
    if (size > available) size = available;
    if (!size) return 0;
    return uart_write_bytes(port(), reinterpret_cast<const char*>(data), size);
  }
  Status tx_complete(bool& done) {
    done = false;
    if (!ready()) return make_status(StatusCode::Transport, "UART driver is closed");
    const esp_err_t result = uart_wait_tx_done(port(), 0);
    if (result == ESP_OK) done = true;
    else if (result != ESP_ERR_TIMEOUT)
      return make_status(StatusCode::Transport, "UART TX completion check failed");
    return ok_status();
  }
  int read_some(std::uint8_t* data, std::size_t size) {
    return ready() ? uart_read_bytes(port(), data, size, 0) : -1;
  }
  void abort() {
    if (!open_) return;
    // Stop driving the wire before deleting the UART, including timeout/cancel.
    if (config_.direction == Esp32Direction::Rs485Rts) {
      drive_gpio(config_.rts_pin, 0);
    }
    drive_gpio(config_.tx_pin, 1);
    serial_.reset(); // ends/deletes UART while this instance still owns the port
    open_ = false;
    if (config_.direction == Esp32Direction::Rs485Rts) {
      drive_gpio(config_.rts_pin, 0);
    }
  }
  const Esp32UartConfig& config() const noexcept { return config_; }
 private:
  uart_port_t port() const noexcept { return static_cast<uart_port_t>(port_); }
  static void drive_gpio(int pin, int level) {
    const auto gpio = static_cast<gpio_num_t>(pin);
    gpio_reset_pin(gpio); // detach UART matrix signal before software control
    gpio_set_level(gpio, level);
    gpio_set_direction(gpio, GPIO_MODE_OUTPUT);
  }
  std::uint8_t port_;
  std::optional<HardwareSerial> serial_;
  Esp32UartConfig config_ {};
  bool open_ = false;
};
} // namespace detail

// Owns a UART port and one core client. Single task, no concurrent Serial use.
// Destruction requires no active request; call cancel() before releasing buffers.
class Esp32UartClient : private detail::Esp32UartTransport,
                        public detail::UartClient<detail::Esp32UartTransport> {
  using Transport = detail::Esp32UartTransport;
  using Client = detail::UartClient<Transport>;
 public:
  explicit Esp32UartClient(std::uint8_t uart_port = 1)
      : Transport(uart_port), Client(static_cast<Transport&>(*this)) {}
  ~Esp32UartClient() { Client::cancel(); }
  using Client::configure;

  [[nodiscard]] Status begin(const Esp32UartConfig& uart, const ProtocolConfig& protocol) {
    if (Client::busy()) return make_status(StatusCode::Busy, "UART client is busy");
    if (Client::requires_transport_reset() && Transport::ready())
      return make_status(StatusCode::NotConnected, "Use recover after a transport failure");
    auto s = validate_format(uart, protocol);
    if (!s.ok()) return s;
    s = Transport::begin(uart);
    if (!s.ok()) return s;
    s = Client::configure(protocol);
    if (!s.ok()) Transport::abort();
    else begun_ = true;
    return s;
  }
  [[nodiscard]] Status configure(const ProtocolConfig& protocol) {
    auto s = validate_format(Transport::config(), protocol);
    return s.ok() ? Client::configure(protocol) : s;
  }
  // Caller must FIRST ensure no response from the previous transaction can arrive.
  // Reopening a UART cannot by itself establish that condition on an RS-485 bus.
  [[nodiscard]] Status recover(const ProtocolConfig& protocol) {
    if (Client::busy()) return make_status(StatusCode::Busy, "UART client is busy");
    if (!begun_) return make_status(StatusCode::NotConnected, "Call begin first");
    const auto uart = Transport::config();
    auto s = validate_format(uart, protocol);
    if (!s.ok()) return s;
    Transport::abort();
    s = Transport::begin(uart);
    if (!s.ok()) return s;
    s = Client::configure_after_recovery(protocol);
    if (!s.ok()) Transport::abort();
    return s;
  }
  [[nodiscard]] Status end() {
    if (Client::busy()) return make_status(StatusCode::Busy, "Cancel active request before end");
    Transport::abort();
    return ok_status();
  }
 private:
  // Only the facade may acknowledge a transport reset.
  using Client::configure_after_recovery;
  bool begun_ = false;
  static Status validate_format(const Esp32UartConfig& uart, const ProtocolConfig& protocol) {
    if (protocol.code_mode() == CodeMode::Binary && ((uart.format >> 2) & 3U) != 3U)
      return make_status(StatusCode::InvalidArgument, "Binary MC protocol requires 8 data bits");
    return ok_status();
  }
};
} // namespace mcprotocol::serial
#endif
