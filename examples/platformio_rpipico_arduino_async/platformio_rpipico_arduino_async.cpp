#include <Arduino.h>

#include <array>
#include <cstddef>
#include <cstdint>

#include "mcprotocol_serial.hpp"
#include "mcprotocol/serial/span_compat.hpp"

namespace {

using mcprotocol::serial::AsciiFormat;
using mcprotocol::serial::BatchReadWordsRequest;
using mcprotocol::serial::CodeMode;
using mcprotocol::serial::FrameCodec;
using mcprotocol::serial::FrameKind;
using mcprotocol::serial::MelsecSerialClient;
using mcprotocol::serial::PlcSeries;
using mcprotocol::serial::ProtocolConfig;
using mcprotocol::serial::RouteConfig;
using mcprotocol::serial::RouteKind;
using mcprotocol::serial::Status;

struct AppState {
  MelsecSerialClient client;
  std::array<std::uint16_t, 2> out_words {};
  bool started = false;
  bool tx_started = false;
  bool tx_completed = false;
  bool done = false;
  bool reported = false;
  Status completion_status {};
};

AppState g_app;

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
  app->done = true;
  app->completion_status = status;
}

void start_uart_tx(AppState& app, std::span<const std::byte> frame) {
  (void)frame;
  app.tx_started = true;
}

void simulate_response(AppState& app, std::uint32_t now_ms) {
  const ProtocolConfig config = make_protocol();
  const std::array<std::uint8_t, 8> response_data {'1', '2', '3', '4', '5', '6', '7', '8'};
  std::array<std::uint8_t, mcprotocol::serial::kMaxResponseFrameBytes> response_frame {};
  std::size_t response_frame_size = 0;
  const Status status = FrameCodec::encode_success_response(
      config,
      std::span<const std::uint8_t>(response_data.data(), response_data.size()),
      response_frame,
      response_frame_size);
  if (!status.ok()) {
    app.done = true;
    app.completion_status = status;
    return;
  }

  app.client.on_rx_bytes(
      now_ms,
      std::span<const std::byte>(
          reinterpret_cast<const std::byte*>(response_frame.data()),
          response_frame_size));
}

void report_once(AppState& app) {
  if (app.reported || !app.done) {
    return;
  }

  app.reported = true;
  if (!app.completion_status.ok()) {
    Serial.print("rpipico async example failed: ");
    Serial.println(app.completion_status.message);
    return;
  }

  Serial.print("rpipico async example read ok: D100=0x");
  Serial.print(app.out_words[0], HEX);
  Serial.print(" D101=0x");
  Serial.println(app.out_words[1], HEX);
}

}  // namespace

void setup() {
  Serial.begin(115200);
  const Status status = g_app.client.configure(make_protocol());
  if (!status.ok()) {
    g_app.done = true;
    g_app.completion_status = status;
  }
}

void loop() {
  const std::uint32_t now = millis();

  if (!g_app.done && !g_app.started) {
    const Status status = g_app.client.async_batch_read_words(
        now,
        BatchReadWordsRequest {
            .head_device = {.code = mcprotocol::serial::DeviceCode::D, .number = 100},
            .points = static_cast<std::uint16_t>(g_app.out_words.size()),
        },
        std::span<std::uint16_t>(g_app.out_words.data(), g_app.out_words.size()),
        on_request_complete,
        &g_app);
    if (status.ok()) {
      start_uart_tx(g_app, g_app.client.pending_tx_frame());
      g_app.started = true;
    } else {
      g_app.done = true;
      g_app.completion_status = status;
    }
  }

  if (!g_app.done && g_app.tx_started && !g_app.tx_completed) {
    const Status status = g_app.client.notify_tx_complete(now);
    if (status.ok()) {
      g_app.tx_completed = true;
      simulate_response(g_app, now);
    } else {
      g_app.done = true;
      g_app.completion_status = status;
    }
  }

  if (!g_app.done) {
    g_app.client.poll(now);
  }

  report_once(g_app);
}
