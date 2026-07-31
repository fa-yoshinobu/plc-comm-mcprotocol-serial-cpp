#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "mcprotocol/serial/client.hpp"
#include "mcprotocol/serial/span.hpp"

namespace {

using mcprotocol::serial::AsciiFormat;
using mcprotocol::serial::BatchReadWordsRequest;
using mcprotocol::serial::CodeMode;
using mcprotocol::serial::FrameCodec;
using mcprotocol::serial::FrameKind;
using mcprotocol::serial::MelsecSerialClient;
using mcprotocol::serial::PlcProfile;
using mcprotocol::serial::ProtocolConfig;
using mcprotocol::serial::RouteConfig;
using mcprotocol::serial::HostStationRoute;
using mcprotocol::serial::Status;
using mcprotocol::serial::SumCheckMode;

struct ExampleApp {
  MelsecSerialClient client;
  std::array<std::uint16_t, 2> out_words {};
  std::array<mcprotocol::serial::Byte, mcprotocol::serial::kMaxResponseFrameBytes> rx_chunk {};
  std::size_t rx_chunk_size = 0;
  bool tx_started = false;
  bool tx_completed = false;
  bool rx_ready = false;
  bool request_done = false;
  Status completion_status {};
};

ProtocolConfig make_protocol() {
  // Keep frame/profile explicit. See docsrc/user/GOTCHAS.md before changing them.
  return ProtocolConfig::ascii(
      mcprotocol::serial::AsciiFrameKind::C4,
      AsciiFormat::Format4,
      PlcProfile::MelsecQ,
      SumCheckMode::Disabled,
      RouteConfig {HostStationRoute {}});
}

void on_request_complete(void* user, Status status) {
  auto* app = static_cast<ExampleApp*>(user);
  app->request_done = true;
  app->completion_status = status;
}

void uart_start_tx_async(ExampleApp& app, mcprotocol::serial::Span<const mcprotocol::serial::Byte> frame) {
  // A real UART driver would start DMA or interrupt-driven transmission here.
  (void)frame;
  app.tx_started = true;
}

void simulate_plc_response(ExampleApp& app, const ProtocolConfig& config) {
  // This host-runnable example feeds a generated success frame into the decoder.
  const std::array<std::uint8_t, 8> response_data {'1', '2', '3', '4', '5', '6', '7', '8'};
  std::array<std::uint8_t, mcprotocol::serial::kMaxResponseFrameBytes> response_frame {};
  std::size_t response_frame_size = 0;
  const Status status = FrameCodec::encode_success_response(
      config,
      mcprotocol::serial::Span<const std::uint8_t>(response_data.data(), response_data.size()),
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

  // Configure the async client once before starting any request.
  Status status = app.client.configure(config);
  if (!status.ok()) {
    std::fprintf(stderr, "configure failed: %s\n", status.message);
    return 1;
  }

  // Start a read-only batch request; the transport sends pending_tx_frame().
  status = app.client.async_batch_read_words(
      0,
      BatchReadWordsRequest({mcprotocol::serial::DeviceCode::D, 100}, static_cast<std::uint16_t>(app.out_words.size())),
      mcprotocol::serial::Span<std::uint16_t>(app.out_words.data(), app.out_words.size()),
      on_request_complete,
      &app);
  if (!status.ok()) {
    std::fprintf(stderr, "request start failed: %s\n", status.message);
    return 1;
  }

  status = app.client.notify_tx_started(0U);
  if (!status.ok()) {
    std::fprintf(stderr, "notify_tx_started failed: %s\n", status.message);
    return 1;
  }
  uart_start_tx_async(app, app.client.pending_tx_frame());

  for (std::uint32_t tick = 1; tick <= 4 && !app.request_done; ++tick) {
    if (app.tx_started && !app.tx_completed) {
      // Notify the client after the transport reports the frame was sent.
      status = app.client.notify_tx_complete(tick, mcprotocol::serial::ok_status());
      if (!status.ok()) {
        std::fprintf(stderr, "notify_tx_complete failed: %s\n", status.message);
        return 1;
      }
      app.tx_completed = true;
      simulate_plc_response(app, config);
    }

    if (app.rx_ready) {
      // Feed received bytes back to the state machine.
      app.client.on_rx_bytes(
          tick,
          mcprotocol::serial::Span<const mcprotocol::serial::Byte>(app.rx_chunk.data(), app.rx_chunk_size));
      app.rx_ready = false;
    }

    // Poll drives request timeout handling between transport events.
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
