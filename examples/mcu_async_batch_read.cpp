#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "mcprotocol/serial/client.hpp"
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

struct ExampleApp {
  MelsecSerialClient client;
  std::array<std::uint16_t, 2> out_words {};
  std::array<std::byte, mcprotocol::serial::kMaxResponseFrameBytes> rx_chunk {};
  std::size_t rx_chunk_size = 0;
  bool tx_started = false;
  bool tx_completed = false;
  bool rx_ready = false;
  bool request_done = false;
  Status completion_status {};
};

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
  auto* app = static_cast<ExampleApp*>(user);
  app->request_done = true;
  app->completion_status = status;
}

void uart_start_tx_async(ExampleApp& app, std::span<const std::byte> frame) {
  (void)frame;
  app.tx_started = true;
}

void simulate_plc_response(ExampleApp& app, const ProtocolConfig& config) {
  const std::array<std::uint8_t, 8> response_data {'1', '2', '3', '4', '5', '6', '7', '8'};
  std::array<std::uint8_t, mcprotocol::serial::kMaxResponseFrameBytes> response_frame {};
  std::size_t response_frame_size = 0;
  const Status status = FrameCodec::encode_success_response(
      config,
      std::span<const std::uint8_t>(response_data.data(), response_data.size()),
      response_frame,
      response_frame_size);
  if (!status.ok()) {
    app.request_done = true;
    app.completion_status = status;
    return;
  }

  std::memcpy(app.rx_chunk.data(), response_frame.data(), response_frame_size);
  app.rx_chunk_size = response_frame_size;
  app.rx_ready = true;
}

}  // namespace

int main() {
  ExampleApp app;
  const ProtocolConfig config = make_protocol();

  Status status = app.client.configure(config);
  if (!status.ok()) {
    std::fprintf(stderr, "configure failed: %s\n", status.message);
    return 1;
  }

  status = app.client.async_batch_read_words(
      0,
      BatchReadWordsRequest {
          .head_device = {.code = mcprotocol::serial::DeviceCode::D, .number = 100},
          .points = static_cast<std::uint16_t>(app.out_words.size()),
      },
      std::span<std::uint16_t>(app.out_words.data(), app.out_words.size()),
      on_request_complete,
      &app);
  if (!status.ok()) {
    std::fprintf(stderr, "request start failed: %s\n", status.message);
    return 1;
  }

  uart_start_tx_async(app, app.client.pending_tx_frame());

  for (std::uint32_t tick = 1; tick <= 4 && !app.request_done; ++tick) {
    if (app.tx_started && !app.tx_completed) {
      status = app.client.notify_tx_complete(tick);
      if (!status.ok()) {
        std::fprintf(stderr, "notify_tx_complete failed: %s\n", status.message);
        return 1;
      }
      app.tx_completed = true;
      simulate_plc_response(app, config);
    }

    if (app.rx_ready) {
      app.client.on_rx_bytes(
          tick,
          std::span<const std::byte>(app.rx_chunk.data(), app.rx_chunk_size));
      app.rx_ready = false;
    }

    app.client.poll(tick);
  }

  if (!app.request_done || !app.completion_status.ok()) {
    std::fprintf(stderr, "request did not complete successfully\n");
    return 1;
  }

  if (app.out_words[0] != 0x1234U || app.out_words[1] != 0x5678U) {
    std::fprintf(stderr, "unexpected read data: %04X %04X\n", app.out_words[0], app.out_words[1]);
    return 1;
  }

  std::printf("example read ok: D100=0x%04X D101=0x%04X\n", app.out_words[0], app.out_words[1]);
  return 0;
}
