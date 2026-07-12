#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <chrono>

#include "mcprotocol_serial.hpp"

namespace {

using mcprotocol::serial::PlcProfile;
using mcprotocol::serial::PosixSerialConfig;
using mcprotocol::serial::PosixSyncClient;
using mcprotocol::serial::ProtocolConfig;
using mcprotocol::serial::Status;
using mcprotocol::serial::SumCheckMode;
using mcprotocol::serial::highlevel::make_c4_binary_protocol;
using mcprotocol::serial::highlevel::make_c4_ascii_format4_protocol;

constexpr unsigned kPollIntervalMs = 1000;
constexpr unsigned kInitialBackoffMs = 1000;
constexpr unsigned kMaxBackoffMs = 30000;

enum class ProtocolSelection {
  Format4Ascii,
  Format5Binary,
};

struct Options {
  const char* serial_device = nullptr;
  const char* plc_profile_text = nullptr;
  const char* head_device = nullptr;
  unsigned points = 0;
  unsigned baud = 0;
  ProtocolSelection protocol = static_cast<ProtocolSelection>(0xFF);
  SumCheckMode sum_check_mode = static_cast<SumCheckMode>(0xFF);
};

void print_usage(const char* argv0) {
  std::fprintf(
      stderr,
      "Usage: %s serial-device plc-profile head-device points baud format4|format5 sum-check\n"
      "\n"
      "Example:  %s COM3 melsec:qcpu D100 4 19200 format5 off\n"
      "\n"
      "This sample is read-only. It logs connected/lost/reconnecting/recovered/read\n"
      "while polling words through the Windows/POSIX host serial backend.\n",
      argv0,
      argv0);
}

bool parse_unsigned(const char* text, unsigned& out_value) {
  if (text == nullptr || *text == '\0') {
    return false;
  }
  char* end = nullptr;
  const unsigned long parsed = std::strtoul(text, &end, 10);
  if (end == text || *end != '\0' || parsed > 0xFFFFUL) {
    return false;
  }
  out_value = static_cast<unsigned>(parsed);
  return true;
}

bool parse_protocol(const char* text, ProtocolSelection& out_protocol) {
  if (text == nullptr) {
    return false;
  }
  if (std::strcmp(text, "4") == 0 || std::strcmp(text, "format4") == 0 ||
      std::strcmp(text, "c4-ascii-f4") == 0) {
    out_protocol = ProtocolSelection::Format4Ascii;
    return true;
  }
  if (std::strcmp(text, "5") == 0 || std::strcmp(text, "format5") == 0 ||
      std::strcmp(text, "c4-binary") == 0 || std::strcmp(text, "binary") == 0) {
    out_protocol = ProtocolSelection::Format5Binary;
    return true;
  }
  return false;
}

bool parse_sum_check_mode(const char* text, SumCheckMode& out_value) {
  if (text == nullptr) {
    return false;
  }
  if (std::strcmp(text, "on") == 0 || std::strcmp(text, "true") == 0 || std::strcmp(text, "1") == 0) {
    out_value = SumCheckMode::Enabled;
    return true;
  }
  if (std::strcmp(text, "off") == 0 || std::strcmp(text, "false") == 0 || std::strcmp(text, "0") == 0) {
    out_value = SumCheckMode::Disabled;
    return true;
  }
  return false;
}

bool parse_args(int argc, char** argv, Options& out_options) {
  if (argc > 1 && (std::strcmp(argv[1], "--help") == 0 || std::strcmp(argv[1], "-h") == 0)) {
    print_usage(argv[0]);
    return false;
  }
  if (argc != 8) {
    print_usage(argv[0]);
    return false;
  }
  out_options.serial_device = argv[1];
  out_options.plc_profile_text = argv[2];
  out_options.head_device = argv[3];
  if (!parse_unsigned(argv[4], out_options.points)) {
    std::fprintf(stderr, "Invalid points: %s\n", argv[4]);
    return false;
  }
  if (!parse_unsigned(argv[5], out_options.baud)) {
    std::fprintf(stderr, "Invalid baud: %s\n", argv[5]);
    return false;
  }
  if (!parse_protocol(argv[6], out_options.protocol)) {
    std::fprintf(stderr, "Invalid protocol format: %s\n", argv[6]);
    return false;
  }
  if (!parse_sum_check_mode(argv[7], out_options.sum_check_mode)) {
    std::fprintf(stderr, "Invalid sum-check value: %s\n", argv[7]);
    return false;
  }
  if (out_options.points == 0 || out_options.points > 32) {
    std::fprintf(stderr, "points must be in range 1..32\n");
    return false;
  }
  return true;
}

const char* protocol_name(ProtocolSelection protocol) {
  switch (protocol) {
    case ProtocolSelection::Format4Ascii:
      return "format4";
    case ProtocolSelection::Format5Binary:
      return "format5";
  }
  return "unknown";
}

void sleep_ms(unsigned ms) {
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void log_state(const char* state, const char* message) {
  std::printf("[%s] %s\n", state, message);
  std::fflush(stdout);
}

bool make_profile(const char* text, PlcProfile& out_profile) {
  return mcprotocol::serial::parse_plc_profile(text, std::strlen(text), out_profile);
}

ProtocolConfig make_protocol(
    ProtocolSelection selection,
    PlcProfile profile,
    SumCheckMode sum_check_mode) {
  const mcprotocol::serial::RouteConfig route {mcprotocol::serial::HostStationRoute {}};
  ProtocolConfig protocol = selection == ProtocolSelection::Format5Binary
                                ? make_c4_binary_protocol(profile, sum_check_mode, route)
                                : make_c4_ascii_format4_protocol(profile, sum_check_mode, route);
  return protocol;
}

PosixSerialConfig make_serial_config(const Options& options) {
  return PosixSerialConfig(
      options.serial_device,
      static_cast<std::uint32_t>(options.baud),
      8,
      1,
      mcprotocol::serial::SerialParity::Even,
      mcprotocol::serial::HardwareFlowControl::None);
}

void print_read_values(const Options& options, const std::array<std::uint16_t, 32>& words) {
  std::printf("[read] %s", options.head_device);
  for (unsigned index = 0; index < options.points; ++index) {
    std::printf("%s%04X", index == 0 ? "=" : ",", static_cast<unsigned>(words[index]));
  }
  std::printf("\n");
  std::fflush(stdout);
}

}  // namespace

int main(int argc, char** argv) {
  Options options {};
  if (!parse_args(argc, argv, options)) {
    return argc > 1 && (std::strcmp(argv[1], "--help") == 0 || std::strcmp(argv[1], "-h") == 0) ? 0 : 2;
  }

  PlcProfile profile = PlcProfile::Unspecified;
  if (!make_profile(options.plc_profile_text, profile)) {
    std::fprintf(stderr, "Invalid PLC profile: %s\n", options.plc_profile_text);
    return 2;
  }

  auto protocol = make_protocol(options.protocol, profile, options.sum_check_mode);

  const PosixSerialConfig serial = make_serial_config(options);
  bool connected_once = false;
  bool online = false;
  unsigned backoff_ms = kInitialBackoffMs;

  char start_message[192] {};
  std::snprintf(
      start_message,
      sizeof(start_message),
      "serial=%s profile=%s baud=%u protocol=%s sum_check=%s target=%s points=%u",
      options.serial_device,
      options.plc_profile_text,
      options.baud,
      protocol_name(options.protocol),
      options.sum_check_mode == SumCheckMode::Enabled ? "on" : "off",
      options.head_device,
      options.points);
  log_state("reconnecting", start_message);

  while (true) {
    PosixSyncClient plc;
    Status status = plc.open(serial, protocol);
    if (status.ok()) {
      std::array<std::uint16_t, 32> words {};
      status = plc.read_words(
          options.head_device,
          static_cast<std::uint16_t>(options.points),
          std::span<std::uint16_t>(words.data(), options.points));
      plc.close();

      if (status.ok()) {
        if (!online) {
          log_state(connected_once ? "recovered" : "connected", options.head_device);
          connected_once = true;
          online = true;
          backoff_ms = kInitialBackoffMs;
        }
        print_read_values(options, words);
        sleep_ms(kPollIntervalMs);
        continue;
      }
    }

    if (online) {
      char lost_message[160] {};
      std::snprintf(lost_message, sizeof(lost_message), "request failed: %s", status.message);
      log_state("lost", lost_message);
    } else {
      char retry_message[160] {};
      std::snprintf(retry_message, sizeof(retry_message), "request failed: %s", status.message);
      log_state("reconnecting", retry_message);
    }

    online = false;
    char reconnect_message[80] {};
    std::snprintf(reconnect_message, sizeof(reconnect_message), "retry in %u ms", backoff_ms);
    log_state("reconnecting", reconnect_message);
    sleep_ms(backoff_ms);
    backoff_ms = backoff_ms >= (kMaxBackoffMs / 2U) ? kMaxBackoffMs : (backoff_ms * 2U);
  }
}
