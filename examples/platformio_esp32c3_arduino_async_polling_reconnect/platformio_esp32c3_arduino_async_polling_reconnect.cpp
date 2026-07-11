#include <Arduino.h>

#include "mcprotocol/serial/compat/array.hpp"
#include "mcprotocol/serial/compat/cstddef.hpp"
#include "mcprotocol/serial/compat/cstdint.hpp"

#include "mcprotocol_serial.hpp"
#include "mcprotocol/serial/span_compat.hpp"

#ifndef MCPROTOCOL_EXAMPLE_PLC_BAUD
#define MCPROTOCOL_EXAMPLE_PLC_BAUD 19200
#endif

#ifndef MCPROTOCOL_EXAMPLE_DEBUG_BAUD
#define MCPROTOCOL_EXAMPLE_DEBUG_BAUD 115200
#endif

namespace {

using mcprotocol::serial::AsciiFormat;
using mcprotocol::serial::BatchReadWordsRequest;
using mcprotocol::serial::CodeMode;
using mcprotocol::serial::DeviceAddress;
using mcprotocol::serial::DeviceCode;
using mcprotocol::serial::FrameKind;
using mcprotocol::serial::MelsecSerialClient;
using mcprotocol::serial::PlcProfile;
using mcprotocol::serial::ProtocolConfig;
using mcprotocol::serial::RouteConfig;
using mcprotocol::serial::HostStationRoute;
using mcprotocol::serial::Status;
using mcprotocol::serial::StatusCode;

constexpr std::uint32_t kPollIntervalMs = 1000;
constexpr std::uint32_t kInitialBackoffMs = 1000;
constexpr std::uint32_t kMaxBackoffMs = 30000;
constexpr std::uint32_t kPlcBaud = MCPROTOCOL_EXAMPLE_PLC_BAUD;
constexpr int kRxPin = 6;
constexpr int kTxPin = 7;
constexpr DeviceAddress kHeadDevice {.code = DeviceCode::D, .number = 100};

struct AppState {
  MelsecSerialClient client;
  std::array<std::uint16_t, 4> out_words {};
  bool request_started = false;
  bool tx_sent = false;
  bool request_done = false;
  bool connected_once = false;
  bool online = false;
  std::uint32_t next_request_ms = 0;
  std::uint32_t reconnect_at_ms = 0;
  std::uint32_t backoff_ms = kInitialBackoffMs;
  Status completion_status {};
};

AppState g_app;
HardwareSerial& g_plc_serial = Serial1;

void log_state(const char* state, const char* message) {
  Serial.printf("%lu [%s] %s\n", static_cast<unsigned long>(millis()), state, message);
}

ProtocolConfig make_protocol() {
  ProtocolConfig config;
  config.frame_kind = FrameKind::C4;
  config.code_mode = CodeMode::Ascii;
  config.ascii_format = AsciiFormat::Format4;
  config.plc_profile = PlcProfile::MelsecQ;
  config.sum_check_mode = mcprotocol::serial::SumCheckMode::Disabled;
  config.route = RouteConfig {HostStationRoute {}};
  return config;
}

bool retryable(Status status) {
  return status.code == StatusCode::Timeout
      || status.code == StatusCode::Transport
      || status.code == StatusCode::Framing
      || status.code == StatusCode::Parse;
}

void reset_request_state() {
  g_app.client.cancel();
  g_app.request_started = false;
  g_app.tx_sent = false;
  g_app.request_done = false;
}

void schedule_reconnect(const char* reason, Status status) {
  if (g_app.online) {
    Serial.printf("%lu [lost] %s: %s\n", static_cast<unsigned long>(millis()), reason, status.message);
  }
  reset_request_state();
  while (g_plc_serial.available() > 0) {
    (void)g_plc_serial.read();
  }
  const Status configure_status = g_app.client.configure(make_protocol());
  if (!configure_status.ok()) {
    Serial.print("configure failed: ");
    Serial.println(configure_status.message);
  }
  g_app.online = false;
  g_app.reconnect_at_ms = millis() + g_app.backoff_ms;
  Serial.printf("%lu [reconnecting] retry in %lu ms\n", static_cast<unsigned long>(millis()), static_cast<unsigned long>(g_app.backoff_ms));
  g_app.backoff_ms = (g_app.backoff_ms >= (kMaxBackoffMs / 2U)) ? kMaxBackoffMs : (g_app.backoff_ms * 2U);
}

void on_request_complete(void* user, Status status) {
  auto* app = static_cast<AppState*>(user);
  app->request_done = true;
  app->completion_status = status;
  app->request_started = false;
  app->tx_sent = false;
}

void configure_plc_uart() {
  g_plc_serial.begin(kPlcBaud, SERIAL_8E1, kRxPin, kTxPin);
}

void maybe_mark_connected() {
  if (!g_app.online) {
    log_state(g_app.connected_once ? "recovered" : "connected", "D100 x4");
    g_app.connected_once = true;
    g_app.online = true;
    g_app.backoff_ms = kInitialBackoffMs;
  }
}

void pump_uart_tx(std::uint32_t now_ms) {
  if (!g_app.request_started || g_app.tx_sent) {
    return;
  }

  const std::span<const std::byte> frame = g_app.client.pending_tx_frame();
  if (frame.empty()) {
    return;
  }

  const std::size_t written = g_plc_serial.write(
      reinterpret_cast<const std::uint8_t*>(frame.data()),
      frame.size());
  g_plc_serial.flush();
  const Status tx_status = (written == frame.size())
      ? g_app.client.notify_tx_complete(now_ms)
      : mcprotocol::serial::make_status(StatusCode::Transport, "UART write failed");
  if (!tx_status.ok()) {
    on_request_complete(&g_app, tx_status);
    return;
  }

  g_app.tx_sent = true;
}

void pump_uart_rx(std::uint32_t now_ms) {
  std::array<char, 64> rx_chunk {};
  while (g_plc_serial.available() > 0) {
    const int available = g_plc_serial.available();
    const std::size_t request_size = static_cast<std::size_t>(available > 0 ? available : 0);
    const std::size_t read_size = request_size < rx_chunk.size() ? request_size : rx_chunk.size();
    const std::size_t bytes_read = static_cast<std::size_t>(
        g_plc_serial.readBytes(rx_chunk.data(), read_size));
    if (bytes_read == 0) {
      break;
    }

    g_app.client.on_rx_bytes(
        now_ms,
        std::span<const std::byte>(
            reinterpret_cast<const std::byte*>(rx_chunk.data()),
            bytes_read));
  }
}

void start_read_if_due(std::uint32_t now_ms) {
  if (g_app.request_started || g_app.client.busy() || now_ms < g_app.next_request_ms || now_ms < g_app.reconnect_at_ms) {
    return;
  }

  if (!g_app.online) {
    log_state("reconnecting", "starting UART read");
  }

  g_app.request_done = false;
  g_app.tx_sent = false;
  const Status status = g_app.client.async_batch_read_words(
      now_ms,
      BatchReadWordsRequest {
          .head_device = kHeadDevice,
          .points = static_cast<std::uint16_t>(g_app.out_words.size()),
      },
      std::span<std::uint16_t>(g_app.out_words.data(), g_app.out_words.size()),
      on_request_complete,
      &g_app);
  if (!status.ok()) {
    schedule_reconnect("request start failed", status);
    return;
  }

  g_app.request_started = true;
}

void report_done() {
  if (!g_app.request_done) {
    return;
  }

  g_app.request_done = false;
  if (!g_app.completion_status.ok()) {
    if (retryable(g_app.completion_status)) {
      schedule_reconnect("request failed", g_app.completion_status);
    } else {
      Serial.print("read failed: ");
      Serial.println(g_app.completion_status.message);
      g_app.next_request_ms = millis() + kPollIntervalMs;
    }
    return;
  }

  maybe_mark_connected();
  Serial.printf(
      "%lu [read] D100=%04X D101=%04X D102=%04X D103=%04X\n",
      static_cast<unsigned long>(millis()),
      static_cast<unsigned>(g_app.out_words[0]),
      static_cast<unsigned>(g_app.out_words[1]),
      static_cast<unsigned>(g_app.out_words[2]),
      static_cast<unsigned>(g_app.out_words[3]));
  g_app.next_request_ms = millis() + kPollIntervalMs;
}

}  // namespace

void setup() {
  Serial.begin(MCPROTOCOL_EXAMPLE_DEBUG_BAUD);
  configure_plc_uart();
  const Status status = g_app.client.configure(make_protocol());
  if (!status.ok()) {
    g_app.completion_status = status;
    g_app.request_done = true;
  }
  log_state("reconnecting", "waiting for first UART read");
}

void loop() {
  const std::uint32_t now = millis();
  pump_uart_rx(now);
  pump_uart_tx(now);
  if (g_app.request_started) {
    g_app.client.poll(now);
  }
  report_done();
  start_read_if_due(now);
  delay(5);
}
