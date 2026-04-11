#include <Arduino.h>

#include <array>
#include <cstddef>
#include <cstdint>

#include "mcprotocol_serial.hpp"
#include "mcprotocol/serial/span_compat.hpp"

#ifndef MCPROTOCOL_EXAMPLE_PLC_BAUD
#define MCPROTOCOL_EXAMPLE_PLC_BAUD 19200
#endif

#ifndef MCPROTOCOL_EXAMPLE_DEBUG_BAUD
#define MCPROTOCOL_EXAMPLE_DEBUG_BAUD 115200
#endif

#ifndef MCPROTOCOL_EXAMPLE_POLL_INTERVAL_MS
#define MCPROTOCOL_EXAMPLE_POLL_INTERVAL_MS 1000U
#endif

namespace {

using mcprotocol::serial::AsciiFormat;
using mcprotocol::serial::BatchReadWordsRequest;
using mcprotocol::serial::CodeMode;
using mcprotocol::serial::DeviceAddress;
using mcprotocol::serial::DeviceCode;
using mcprotocol::serial::FrameKind;
using mcprotocol::serial::MelsecSerialClient;
using mcprotocol::serial::PlcSeries;
using mcprotocol::serial::ProtocolConfig;
using mcprotocol::serial::RouteConfig;
using mcprotocol::serial::RouteKind;
using mcprotocol::serial::Status;

constexpr std::uint32_t kPollIntervalMs = MCPROTOCOL_EXAMPLE_POLL_INTERVAL_MS;
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
  bool request_reported = false;
  std::uint32_t next_request_ms = 0;
  Status completion_status {};
};

AppState g_app;
HardwareSerial& g_plc_serial = Serial1;

ProtocolConfig make_protocol() {
  ProtocolConfig config;
  config.frame_kind = FrameKind::C4;
  config.code_mode = CodeMode::Ascii;
  config.ascii_format = AsciiFormat::Format4;
  config.target_series = PlcSeries::Q_L;
  config.sum_check_enabled = false;
  config.route = RouteConfig {
      .kind = RouteKind::HostStation,
      .station_no = 0x00,
      .network_no = 0x00,
      .pc_no = 0xFF,
      .request_destination_module_io_no = 0x03FF,
      .request_destination_module_station_no = 0x00,
      .self_station_enabled = false,
      .self_station_no = 0x00,
  };
  return config;
}

void on_request_complete(void* user, Status status) {
  auto* app = static_cast<AppState*>(user);
  app->request_done = true;
  app->completion_status = status;
  app->request_started = false;
  app->tx_sent = false;
  app->next_request_ms = millis() + kPollIntervalMs;
}

void configure_plc_uart() {
  g_plc_serial.begin(kPlcBaud, SERIAL_8E1, kRxPin, kTxPin);
}

void pump_uart_tx(std::uint32_t now_ms) {
  if (!g_app.request_started || g_app.tx_sent) {
    return;
  }

  const std::span<const std::byte> frame = g_app.client.pending_tx_frame();
  if (frame.empty()) {
    return;
  }

  g_plc_serial.write(reinterpret_cast<const std::uint8_t*>(frame.data()), frame.size());
  g_plc_serial.flush();
  const Status status = g_app.client.notify_tx_complete(now_ms);
  if (!status.ok()) {
    on_request_complete(&g_app, status);
    return;
  }

  g_app.tx_sent = true;
}

void pump_uart_rx(std::uint32_t now_ms) {
  std::array<char, 64> rx_chunk {};
  while (g_plc_serial.available() > 0) {
    const int available = g_plc_serial.available();
    const std::size_t request_size =
        static_cast<std::size_t>(available > 0 ? available : 0);
    const std::size_t read_size =
        request_size < rx_chunk.size() ? request_size : rx_chunk.size();
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
  if (g_app.request_started || g_app.client.busy() || now_ms < g_app.next_request_ms) {
    return;
  }

  g_app.request_done = false;
  g_app.request_reported = false;
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
    on_request_complete(&g_app, status);
    return;
  }

  g_app.request_started = true;
}

void report_once() {
  if (!g_app.request_done || g_app.request_reported) {
    return;
  }

  g_app.request_reported = true;
  if (!g_app.completion_status.ok()) {
    Serial.print("esp32c3 uart example failed: ");
    Serial.println(g_app.completion_status.message);
    return;
  }

  Serial.print("esp32c3 uart read ok: D100=");
  Serial.print(g_app.out_words[0], HEX);
  Serial.print(" D101=");
  Serial.print(g_app.out_words[1], HEX);
  Serial.print(" D102=");
  Serial.print(g_app.out_words[2], HEX);
  Serial.print(" D103=");
  Serial.println(g_app.out_words[3], HEX);
}

}  // namespace

void setup() {
  Serial.begin(MCPROTOCOL_EXAMPLE_DEBUG_BAUD);
  configure_plc_uart();

  const Status status = g_app.client.configure(make_protocol());
  if (!status.ok()) {
    on_request_complete(&g_app, status);
    return;
  }

  g_app.next_request_ms = 0;
  Serial.println("esp32c3 uart example: read-only D100-D103 via Serial1");
}

void loop() {
  const std::uint32_t now_ms = millis();
  start_read_if_due(now_ms);
  pump_uart_tx(now_ms);
  pump_uart_rx(now_ms);
  g_app.client.poll(now_ms);
  report_once();
}
