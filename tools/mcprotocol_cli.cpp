#include <array>
#include <charconv>
#include <chrono>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "mcprotocol/serial/span_compat.hpp"
#include <string_view>

#include "mcprotocol/serial/client.hpp"
#include "mcprotocol/serial/posix_serial.hpp"
#include "mcprotocol/serial/qualified_buffer.hpp"

namespace {

bool g_dump_frames = false;

using mcprotocol::serial::AsciiFormat;
using mcprotocol::serial::BatchReadBitsRequest;
using mcprotocol::serial::BatchReadWordsRequest;
using mcprotocol::serial::BatchWriteBitsRequest;
using mcprotocol::serial::BatchWriteWordsRequest;
using mcprotocol::serial::BitValue;
using mcprotocol::serial::CodeMode;
using mcprotocol::serial::CpuModelInfo;
using mcprotocol::serial::DeviceAddress;
using mcprotocol::serial::DeviceCode;
using mcprotocol::serial::FrameKind;
using mcprotocol::serial::HostBufferReadRequest;
using mcprotocol::serial::HostBufferWriteRequest;
using mcprotocol::serial::MelsecSerialClient;
using mcprotocol::serial::ModuleBufferReadRequest;
using mcprotocol::serial::ModuleBufferWriteRequest;
using mcprotocol::serial::MultiBlockReadBlock;
using mcprotocol::serial::MultiBlockReadBlockResult;
using mcprotocol::serial::MultiBlockReadRequest;
using mcprotocol::serial::MultiBlockWriteBlock;
using mcprotocol::serial::MultiBlockWriteRequest;
using mcprotocol::serial::PlcSeries;
using mcprotocol::serial::PosixSerialConfig;
using mcprotocol::serial::PosixSerialPort;
using mcprotocol::serial::ProtocolConfig;
using mcprotocol::serial::QualifiedBufferWordDevice;
using mcprotocol::serial::RandomReadItem;
using mcprotocol::serial::RandomReadRequest;
using mcprotocol::serial::RandomWriteBitItem;
using mcprotocol::serial::RandomWriteWordItem;
using mcprotocol::serial::RouteKind;
using mcprotocol::serial::Rs485Hooks;
using mcprotocol::serial::Status;
using mcprotocol::serial::StatusCode;
using mcprotocol::serial::decode_qualified_buffer_word_values;
using mcprotocol::serial::make_qualified_buffer_read_words_request;
using mcprotocol::serial::make_qualified_buffer_write_words_request;
using mcprotocol::serial::parse_qualified_buffer_word_device;
using mcprotocol::serial::qualified_buffer_kind_name;

enum class CommandKind : std::uint8_t {
  None,
  CpuModel,
  Loopback,
  ReadWords,
  ReadBits,
  ReadHostBuffer,
  ReadModuleBuffer,
  ReadQualifiedWords,
  ReadNativeQualifiedWords,
  WriteHostBuffer,
  WriteModuleBuffer,
  WriteQualifiedWords,
  WriteNativeQualifiedWords,
  RandomRead,
  RandomWriteWords,
  RandomWriteBits,
  WriteWords,
  WriteBits,
  ProbeAll,
  ProbeWriteAll,
  ProbeMultiBlock,
  ProbeMonitor,
  ProbeHostBuffer,
  ProbeModuleBuffer,
  ProbeWriteHostBuffer,
  ProbeWriteModuleBuffer
};

struct CommandState {
  bool done = false;
  Status status {};
};

struct CliOptions {
  PosixSerialConfig serial {};
  ProtocolConfig protocol {};
  bool rts_toggle = false;
  bool dump_frames = false;
  CommandKind command = CommandKind::None;
  int command_argc = 0;
  char** command_argv = nullptr;
};

struct DeviceParseSpec {
  std::string_view prefix;
  DeviceCode code;
  int base;
};

struct ProbeTarget {
  std::string_view label;
  DeviceAddress device;
};

constexpr std::array<DeviceParseSpec, 26> kDeviceParseSpecs {{
    {"STS", DeviceCode::STS, 10},
    {"STC", DeviceCode::STC, 10},
    {"STN", DeviceCode::STN, 10},
    {"TS", DeviceCode::TS, 10},
    {"TC", DeviceCode::TC, 10},
    {"TN", DeviceCode::TN, 10},
    {"CS", DeviceCode::CS, 10},
    {"CC", DeviceCode::CC, 10},
    {"CN", DeviceCode::CN, 10},
    {"SB", DeviceCode::SB, 16},
    {"SW", DeviceCode::SW, 16},
    {"DX", DeviceCode::DX, 16},
    {"DY", DeviceCode::DY, 16},
    {"ZR", DeviceCode::ZR, 16},
    {"X", DeviceCode::X, 16},
    {"Y", DeviceCode::Y, 16},
    {"M", DeviceCode::M, 10},
    {"L", DeviceCode::L, 10},
    {"F", DeviceCode::F, 10},
    {"V", DeviceCode::V, 10},
    {"B", DeviceCode::B, 16},
    {"D", DeviceCode::D, 10},
    {"W", DeviceCode::W, 16},
    {"S", DeviceCode::S, 10},
    {"Z", DeviceCode::Z, 10},
    {"R", DeviceCode::R, 10},
}};

constexpr std::array<ProbeTarget, 26> kProbeTargets {{
    {"STS0", {.code = DeviceCode::STS, .number = 0}},
    {"STC0", {.code = DeviceCode::STC, .number = 0}},
    {"STN0", {.code = DeviceCode::STN, .number = 0}},
    {"TS0", {.code = DeviceCode::TS, .number = 0}},
    {"TC0", {.code = DeviceCode::TC, .number = 0}},
    {"TN0", {.code = DeviceCode::TN, .number = 0}},
    {"CS0", {.code = DeviceCode::CS, .number = 0}},
    {"CC0", {.code = DeviceCode::CC, .number = 0}},
    {"CN0", {.code = DeviceCode::CN, .number = 0}},
    {"SB0", {.code = DeviceCode::SB, .number = 0}},
    {"SW0", {.code = DeviceCode::SW, .number = 0}},
    {"DX0", {.code = DeviceCode::DX, .number = 0}},
    {"DY0", {.code = DeviceCode::DY, .number = 0}},
    {"ZR0", {.code = DeviceCode::ZR, .number = 0}},
    {"X0", {.code = DeviceCode::X, .number = 0}},
    {"Y0", {.code = DeviceCode::Y, .number = 0}},
    {"M0", {.code = DeviceCode::M, .number = 0}},
    {"L0", {.code = DeviceCode::L, .number = 0}},
    {"F0", {.code = DeviceCode::F, .number = 0}},
    {"V0", {.code = DeviceCode::V, .number = 0}},
    {"B0", {.code = DeviceCode::B, .number = 0}},
    {"D0", {.code = DeviceCode::D, .number = 0}},
    {"W0", {.code = DeviceCode::W, .number = 0}},
    {"S0", {.code = DeviceCode::S, .number = 0}},
    {"Z0", {.code = DeviceCode::Z, .number = 0}},
    {"R0", {.code = DeviceCode::R, .number = 0}},
}};

constexpr std::array<ProbeTarget, 25> kProbeWriteTargets {{
    {"STS0", {.code = DeviceCode::STS, .number = 0}},
    {"STC0", {.code = DeviceCode::STC, .number = 0}},
    {"STN0", {.code = DeviceCode::STN, .number = 0}},
    {"TS0", {.code = DeviceCode::TS, .number = 0}},
    {"TC0", {.code = DeviceCode::TC, .number = 0}},
    {"TN0", {.code = DeviceCode::TN, .number = 0}},
    {"CS0", {.code = DeviceCode::CS, .number = 0}},
    {"CC0", {.code = DeviceCode::CC, .number = 0}},
    {"CN0", {.code = DeviceCode::CN, .number = 0}},
    {"SB0", {.code = DeviceCode::SB, .number = 0}},
    {"SW0", {.code = DeviceCode::SW, .number = 0}},
    {"DX0", {.code = DeviceCode::DX, .number = 0}},
    {"DY0", {.code = DeviceCode::DY, .number = 0}},
    {"ZR0", {.code = DeviceCode::ZR, .number = 0}},
    {"X0", {.code = DeviceCode::X, .number = 0}},
    {"Y0", {.code = DeviceCode::Y, .number = 0}},
    {"M0", {.code = DeviceCode::M, .number = 0}},
    {"L0", {.code = DeviceCode::L, .number = 0}},
    {"F100", {.code = DeviceCode::F, .number = 100}},
    {"V0", {.code = DeviceCode::V, .number = 0}},
    {"B0", {.code = DeviceCode::B, .number = 0}},
    {"D0", {.code = DeviceCode::D, .number = 0}},
    {"W0", {.code = DeviceCode::W, .number = 0}},
    {"Z0", {.code = DeviceCode::Z, .number = 0}},
    {"R0", {.code = DeviceCode::R, .number = 0}},
}};

constexpr std::size_t kCliMaxBatchWordPoints = mcprotocol::serial::kMaxBatchWordPoints;
constexpr std::size_t kCliMaxBatchBitPoints = mcprotocol::serial::kMaxBatchBitPointsAscii;

[[nodiscard]] std::uint32_t now_ms() {
  const auto now = std::chrono::steady_clock::now().time_since_epoch();
  return static_cast<std::uint32_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(now).count());
}

[[nodiscard]] constexpr std::size_t cli_request_command_header_size(const ProtocolConfig& config) {
  return config.code_mode == CodeMode::Ascii ? 8U : 4U;
}

[[nodiscard]] constexpr std::size_t cli_request_word_count_size(const ProtocolConfig& config) {
  return config.code_mode == CodeMode::Ascii ? 4U : 2U;
}

[[nodiscard]] constexpr std::size_t cli_request_device_reference_size(const ProtocolConfig& config) {
  if (config.code_mode == CodeMode::Ascii) {
    return config.target_series == PlcSeries::IQ_R ? 12U : 8U;
  }
  return config.target_series == PlcSeries::IQ_R ? 6U : 4U;
}

[[nodiscard]] constexpr std::size_t cli_max_batch_write_words_points(const ProtocolConfig& config) {
  const std::size_t overhead =
      cli_request_command_header_size(config) + cli_request_device_reference_size(config) +
      cli_request_word_count_size(config);
  if (mcprotocol::serial::kMaxRequestDataBytes <= overhead) {
    return 0U;
  }
  const std::size_t buffer_limit =
      (mcprotocol::serial::kMaxRequestDataBytes - overhead) / (config.code_mode == CodeMode::Ascii ? 4U : 2U);
  return buffer_limit < mcprotocol::serial::kMaxBatchWordPoints ? buffer_limit : mcprotocol::serial::kMaxBatchWordPoints;
}

[[nodiscard]] constexpr std::size_t cli_max_batch_write_bits_points(const ProtocolConfig& config) {
  const std::size_t overhead =
      cli_request_command_header_size(config) + cli_request_device_reference_size(config) +
      cli_request_word_count_size(config);
  if (mcprotocol::serial::kMaxRequestDataBytes <= overhead) {
    return 0U;
  }
  const std::size_t remaining = mcprotocol::serial::kMaxRequestDataBytes - overhead;
  const std::size_t buffer_limit = config.code_mode == CodeMode::Ascii ? remaining : (remaining * 2U);
  const std::size_t protocol_limit =
      config.code_mode == CodeMode::Ascii ? mcprotocol::serial::kMaxBatchBitPointsAscii
                                          : mcprotocol::serial::kMaxBatchBitPointsBinary;
  return buffer_limit < protocol_limit ? buffer_limit : protocol_limit;
}

void print_usage() {
  std::fprintf(
      stderr,
      "Usage:\n"
      "  mcprotocol_cli [options] cpu-model\n"
      "  mcprotocol_cli [options] loopback HEXASCII\n"
      "  mcprotocol_cli [options] read-words DEVICE POINTS\n"
      "  mcprotocol_cli [options] read-bits DEVICE POINTS\n"
      "  mcprotocol_cli [options] read-host-buffer START WORDS\n"
      "  mcprotocol_cli [options] read-module-buffer START BYTES MODULE\n"
      "  mcprotocol_cli [options] read-qualified-words U3E0\\\\G0 POINTS\n"
      "  mcprotocol_cli [options] read-native-qualified-words U3E0\\\\G0 POINTS\n"
      "  mcprotocol_cli [options] write-host-buffer START VALUE [VALUE ...]\n"
      "  mcprotocol_cli [options] write-module-buffer START MODULE BYTE [BYTE ...]\n"
      "  mcprotocol_cli [options] write-qualified-words U3E0\\\\G0 VALUE [VALUE ...]\n"
      "  mcprotocol_cli [options] write-native-qualified-words U3E0\\\\G0 VALUE [VALUE ...]\n"
      "  mcprotocol_cli [options] random-read DEVICE [DEVICE ...]\n"
      "  mcprotocol_cli [options] random-write-words DEVICE=VALUE [DEVICE=VALUE ...]\n"
      "  mcprotocol_cli [options] random-write-bits DEVICE=0|1 [DEVICE=0|1 ...]\n"
      "  mcprotocol_cli [options] write-words DEVICE=VALUE [DEVICE=VALUE ...]\n"
      "  mcprotocol_cli [options] write-bits DEVICE=0|1 [DEVICE=0|1 ...]\n"
      "  mcprotocol_cli [options] probe-all\n"
      "  mcprotocol_cli [options] probe-write-all\n"
      "  mcprotocol_cli [options] probe-multi-block\n"
      "  mcprotocol_cli [options] probe-monitor\n"
      "  mcprotocol_cli [options] probe-host-buffer\n"
      "  mcprotocol_cli [options] probe-module-buffer\n"
      "  mcprotocol_cli [options] probe-write-host-buffer\n"
      "  mcprotocol_cli [options] probe-write-module-buffer\n"
      "\n"
      "Options:\n"
      "  --device PATH               Serial device path (default: /dev/ttyUSB0)\n"
      "  --baud RATE                Baud rate (default: 9600)\n"
      "  --data-bits N              Data bits: 5/6/7/8 (default: 8)\n"
      "  --stop-bits N              Stop bits: 1/2 (default: 1)\n"
      "  --parity N|E|O             Parity (default: N)\n"
      "  --rts-cts on|off           Hardware flow control (default: off)\n"
      "  --rts-toggle on|off        Toggle RTS during TX for RS-485 DE control\n"
      "  --dump-frames on|off       Print raw TX/RX frame bytes to stderr (default: off)\n"
      "  --frame MODE               c4-binary | c4-ascii-f1 | c4-ascii-f3 | c4-ascii-f4 | c3-ascii-f1 | c3-ascii-f3 | c3-ascii-f4\n"
      "  --series ql|iqr            Target PLC family for device encoding (default: ql)\n"
      "  --station N                Station number; non-zero implies multidrop\n"
      "  --self-station N           Self-station number for m:n connections\n"
      "  --sum-check on|off         Enable or disable sum-check (default: on)\n"
      "  --response-timeout-ms N    Response timeout in milliseconds (default: 5000)\n"
      "  --inter-byte-timeout-ms N  Inter-byte timeout in milliseconds (default: 250)\n");
}

[[nodiscard]] bool parse_u32(std::string_view text, std::uint32_t& out_value, int base = 10) {
  const char* begin = text.data();
  const char* end = text.data() + text.size();
  const auto result = std::from_chars(begin, end, out_value, base);
  return result.ec == std::errc() && result.ptr == end;
}

[[nodiscard]] bool parse_u32_auto(std::string_view text, std::uint32_t& out_value) {
  if (text.size() > 2U && text[0] == '0' && (text[1] == 'x' || text[1] == 'X')) {
    return parse_u32(text.substr(2), out_value, 16);
  }
  return parse_u32(text, out_value, 10);
}

[[nodiscard]] bool parse_on_off(std::string_view text, bool& out_value) {
  if (text == "on") {
    out_value = true;
    return true;
  }
  if (text == "off") {
    out_value = false;
    return true;
  }
  return false;
}

[[nodiscard]] bool parse_frame_mode(std::string_view text, ProtocolConfig& config) {
  if (text == "c4-binary") {
    config.frame_kind = FrameKind::C4;
    config.code_mode = CodeMode::Binary;
    config.ascii_format = AsciiFormat::Format3;
    return true;
  }
  if (text == "c4-ascii-f1") {
    config.frame_kind = FrameKind::C4;
    config.code_mode = CodeMode::Ascii;
    config.ascii_format = AsciiFormat::Format1;
    return true;
  }
  if (text == "c4-ascii-f3") {
    config.frame_kind = FrameKind::C4;
    config.code_mode = CodeMode::Ascii;
    config.ascii_format = AsciiFormat::Format3;
    return true;
  }
  if (text == "c4-ascii-f4") {
    config.frame_kind = FrameKind::C4;
    config.code_mode = CodeMode::Ascii;
    config.ascii_format = AsciiFormat::Format4;
    return true;
  }
  if (text == "c3-ascii-f1") {
    config.frame_kind = FrameKind::C3;
    config.code_mode = CodeMode::Ascii;
    config.ascii_format = AsciiFormat::Format1;
    return true;
  }
  if (text == "c3-ascii-f3") {
    config.frame_kind = FrameKind::C3;
    config.code_mode = CodeMode::Ascii;
    config.ascii_format = AsciiFormat::Format3;
    return true;
  }
  if (text == "c3-ascii-f4") {
    config.frame_kind = FrameKind::C3;
    config.code_mode = CodeMode::Ascii;
    config.ascii_format = AsciiFormat::Format4;
    return true;
  }
  return false;
}

[[nodiscard]] bool parse_series(std::string_view text, ProtocolConfig& config) {
  if (text == "ql") {
    config.target_series = PlcSeries::Q_L;
    return true;
  }
  if (text == "iqr") {
    config.target_series = PlcSeries::IQ_R;
    return true;
  }
  return false;
}

[[nodiscard]] bool parse_device_address(std::string_view text, DeviceAddress& out_device) {
  for (const auto& spec : kDeviceParseSpecs) {
    if (text.size() <= spec.prefix.size()) {
      continue;
    }

    bool prefix_match = true;
    for (std::size_t index = 0; index < spec.prefix.size(); ++index) {
      const char lhs = static_cast<char>(std::toupper(static_cast<unsigned char>(text[index])));
      if (lhs != spec.prefix[index]) {
        prefix_match = false;
        break;
      }
    }
    if (!prefix_match) {
      continue;
    }

    std::uint32_t number = 0;
    if (!parse_u32(text.substr(spec.prefix.size()), number, spec.base)) {
      return false;
    }
    out_device.code = spec.code;
    out_device.number = number;
    return true;
  }
  return false;
}

[[nodiscard]] bool parse_args(int argc, char** argv, CliOptions& options) {
  options.serial.device_path = "/dev/ttyUSB0";
  options.protocol.frame_kind = FrameKind::C4;
  options.protocol.code_mode = CodeMode::Binary;
  options.protocol.ascii_format = AsciiFormat::Format3;
  options.protocol.target_series = PlcSeries::Q_L;
  options.protocol.sum_check_enabled = true;
  options.protocol.route.kind = RouteKind::HostStation;
  options.protocol.route.station_no = 0x00;
  options.protocol.route.network_no = 0x00;
  options.protocol.route.pc_no = 0xFF;
  options.protocol.route.request_destination_module_io_no = 0x03FF;
  options.protocol.route.request_destination_module_station_no = 0x00;
  options.protocol.route.self_station_enabled = false;
  options.protocol.timeout.response_timeout_ms = 5000;
  options.protocol.timeout.inter_byte_timeout_ms = 250;

  int index = 1;
  while (index < argc) {
    const std::string_view arg(argv[index]);
    if (arg == "--device" && (index + 1) < argc) {
      options.serial.device_path = argv[++index];
    } else if (arg == "--baud" && (index + 1) < argc) {
      std::uint32_t value = 0;
      if (!parse_u32(argv[++index], value)) {
        return false;
      }
      options.serial.baud_rate = value;
    } else if (arg == "--data-bits" && (index + 1) < argc) {
      std::uint32_t value = 0;
      if (!parse_u32(argv[++index], value) || value > 8U) {
        return false;
      }
      options.serial.data_bits = static_cast<std::uint8_t>(value);
    } else if (arg == "--stop-bits" && (index + 1) < argc) {
      std::uint32_t value = 0;
      if (!parse_u32(argv[++index], value) || value > 2U) {
        return false;
      }
      options.serial.stop_bits = static_cast<std::uint8_t>(value);
    } else if (arg == "--parity" && (index + 1) < argc) {
      options.serial.parity = argv[++index][0];
    } else if (arg == "--rts-cts" && (index + 1) < argc) {
      if (!parse_on_off(argv[++index], options.serial.rts_cts)) {
        return false;
      }
    } else if (arg == "--rts-toggle" && (index + 1) < argc) {
      if (!parse_on_off(argv[++index], options.rts_toggle)) {
        return false;
      }
    } else if (arg == "--dump-frames" && (index + 1) < argc) {
      if (!parse_on_off(argv[++index], options.dump_frames)) {
        return false;
      }
    } else if (arg == "--frame" && (index + 1) < argc) {
      if (!parse_frame_mode(argv[++index], options.protocol)) {
        return false;
      }
    } else if (arg == "--series" && (index + 1) < argc) {
      if (!parse_series(argv[++index], options.protocol)) {
        return false;
      }
    } else if (arg == "--station" && (index + 1) < argc) {
      std::uint32_t value = 0;
      if (!parse_u32_auto(argv[++index], value)) {
        return false;
      }
      options.protocol.route.station_no = static_cast<std::uint8_t>(value);
      options.protocol.route.kind = value == 0U ? RouteKind::HostStation : RouteKind::MultidropStation;
    } else if (arg == "--self-station" && (index + 1) < argc) {
      std::uint32_t value = 0;
      if (!parse_u32_auto(argv[++index], value)) {
        return false;
      }
      options.protocol.route.self_station_enabled = true;
      options.protocol.route.self_station_no = static_cast<std::uint8_t>(value);
    } else if (arg == "--sum-check" && (index + 1) < argc) {
      if (!parse_on_off(argv[++index], options.protocol.sum_check_enabled)) {
        return false;
      }
    } else if (arg == "--response-timeout-ms" && (index + 1) < argc) {
      std::uint32_t value = 0;
      if (!parse_u32(argv[++index], value)) {
        return false;
      }
      options.protocol.timeout.response_timeout_ms = value;
    } else if (arg == "--inter-byte-timeout-ms" && (index + 1) < argc) {
      std::uint32_t value = 0;
      if (!parse_u32(argv[++index], value)) {
        return false;
      }
      options.protocol.timeout.inter_byte_timeout_ms = value;
    } else if (!arg.empty() && arg.front() != '-') {
      if (arg == "cpu-model" || arg == "read-cpu-model") {
        options.command = CommandKind::CpuModel;
      } else if (arg == "loopback") {
        options.command = CommandKind::Loopback;
      } else if (arg == "read-words") {
        options.command = CommandKind::ReadWords;
      } else if (arg == "read-bits") {
        options.command = CommandKind::ReadBits;
      } else if (arg == "read-host-buffer") {
        options.command = CommandKind::ReadHostBuffer;
      } else if (arg == "read-module-buffer") {
        options.command = CommandKind::ReadModuleBuffer;
      } else if (arg == "read-qualified-words") {
        options.command = CommandKind::ReadQualifiedWords;
      } else if (arg == "read-native-qualified-words") {
        options.command = CommandKind::ReadNativeQualifiedWords;
      } else if (arg == "write-host-buffer") {
        options.command = CommandKind::WriteHostBuffer;
      } else if (arg == "write-module-buffer") {
        options.command = CommandKind::WriteModuleBuffer;
      } else if (arg == "write-qualified-words") {
        options.command = CommandKind::WriteQualifiedWords;
      } else if (arg == "write-native-qualified-words") {
        options.command = CommandKind::WriteNativeQualifiedWords;
      } else if (arg == "random-read") {
        options.command = CommandKind::RandomRead;
      } else if (arg == "random-write-words") {
        options.command = CommandKind::RandomWriteWords;
      } else if (arg == "random-write-bits") {
        options.command = CommandKind::RandomWriteBits;
      } else if (arg == "write-words") {
        options.command = CommandKind::WriteWords;
      } else if (arg == "write-bits") {
        options.command = CommandKind::WriteBits;
      } else if (arg == "probe-all") {
        options.command = CommandKind::ProbeAll;
      } else if (arg == "probe-write-all") {
        options.command = CommandKind::ProbeWriteAll;
      } else if (arg == "probe-multi-block") {
        options.command = CommandKind::ProbeMultiBlock;
      } else if (arg == "probe-monitor") {
        options.command = CommandKind::ProbeMonitor;
      } else if (arg == "probe-host-buffer") {
        options.command = CommandKind::ProbeHostBuffer;
      } else if (arg == "probe-module-buffer") {
        options.command = CommandKind::ProbeModuleBuffer;
      } else if (arg == "probe-write-host-buffer") {
        options.command = CommandKind::ProbeWriteHostBuffer;
      } else if (arg == "probe-write-module-buffer") {
        options.command = CommandKind::ProbeWriteModuleBuffer;
      } else {
        return false;
      }
      options.command_argc = argc - (index + 1);
      options.command_argv = &argv[index + 1];
      index = argc;
      break;
    } else {
      return false;
    }
    ++index;
  }

  if (options.command == CommandKind::None || index != argc) {
    return false;
  }

  switch (options.command) {
    case CommandKind::CpuModel:
    case CommandKind::ProbeAll:
    case CommandKind::ProbeWriteAll:
    case CommandKind::ProbeMultiBlock:
    case CommandKind::ProbeMonitor:
    case CommandKind::ProbeHostBuffer:
    case CommandKind::ProbeModuleBuffer:
    case CommandKind::ProbeWriteHostBuffer:
    case CommandKind::ProbeWriteModuleBuffer:
      return options.command_argc == 0;
    case CommandKind::Loopback:
      return options.command_argc == 1;
    case CommandKind::ReadWords:
    case CommandKind::ReadBits:
    case CommandKind::ReadHostBuffer:
    case CommandKind::ReadQualifiedWords:
    case CommandKind::ReadNativeQualifiedWords:
      return options.command_argc == 2;
    case CommandKind::ReadModuleBuffer:
      return options.command_argc == 3;
    case CommandKind::WriteHostBuffer:
      return options.command_argc >= 2;
    case CommandKind::WriteModuleBuffer:
      return options.command_argc >= 3;
    case CommandKind::WriteQualifiedWords:
    case CommandKind::WriteNativeQualifiedWords:
      return options.command_argc >= 2;
    case CommandKind::RandomRead:
    case CommandKind::RandomWriteWords:
    case CommandKind::RandomWriteBits:
    case CommandKind::WriteWords:
    case CommandKind::WriteBits:
      return options.command_argc >= 1;
    case CommandKind::None:
      return false;
  }

  return false;
}

void request_complete(void* user, Status status) {
  auto* state = static_cast<CommandState*>(user);
  state->done = true;
  state->status = status;
}

void on_tx_begin(void* user) {
  auto* port = static_cast<PosixSerialPort*>(user);
  (void)port->set_rts(true);
}

void on_tx_end(void* user) {
  auto* port = static_cast<PosixSerialPort*>(user);
  (void)port->set_rts(false);
}

[[nodiscard]] Status discard_stale_rx(PosixSerialPort& port) {
  Status status = port.flush_rx();
  if (!status.ok()) {
    return status;
  }

  std::array<std::byte, 256> drain_buffer {};
  int quiet_polls = 0;
  while (quiet_polls < 2) {
    std::size_t bytes_read = 0;
    status = port.read_some(drain_buffer, 30, bytes_read);
    if (!status.ok()) {
      return status;
    }
    if (bytes_read == 0U) {
      ++quiet_polls;
      continue;
    }
    quiet_polls = 0;
  }
  return mcprotocol::serial::ok_status();
}

void dump_frame_bytes(std::string_view label, std::span<const std::byte> bytes) {
  std::fprintf(stderr, "%.*s[%zu] hex:", static_cast<int>(label.size()), label.data(), bytes.size());
  for (const std::byte value : bytes) {
    std::fprintf(stderr, " %02X", static_cast<unsigned>(std::to_integer<std::uint8_t>(value)));
  }
  std::fprintf(stderr, "\n%.*s[%zu] ascii:", static_cast<int>(label.size()), label.data(), bytes.size());
  for (const std::byte value : bytes) {
    const unsigned char ch = std::to_integer<unsigned char>(value);
    std::fputc(std::isprint(ch) ? static_cast<int>(ch) : '.', stderr);
  }
  std::fputc('\n', stderr);
}

[[nodiscard]] Status drive_request(
    MelsecSerialClient& client,
    PosixSerialPort& port,
    CommandState& state,
    bool dump_frames = false) {
  Status status = discard_stale_rx(port);
  if (!status.ok()) {
    return status;
  }

  if (dump_frames || g_dump_frames) {
    dump_frame_bytes("tx", client.pending_tx_frame());
  }
  status = port.write_all(client.pending_tx_frame());
  if (status.ok()) {
    status = port.drain_tx();
  }

  Status notify_status = client.notify_tx_complete(now_ms(), status);
  if (!notify_status.ok()) {
    return notify_status;
  }
  if (!status.ok()) {
    return status;
  }

  std::array<std::byte, 256> rx_buffer {};
  while (!state.done) {
    std::size_t bytes_read = 0;
    status = port.read_some(rx_buffer, 50, bytes_read);
    if (!status.ok()) {
      client.cancel();
      return status;
    }
    if (bytes_read != 0U) {
      if (dump_frames || g_dump_frames) {
        dump_frame_bytes("rx", std::span<const std::byte>(rx_buffer.data(), bytes_read));
      }
      client.on_rx_bytes(now_ms(), std::span<const std::byte>(rx_buffer.data(), bytes_read));
    }
    client.poll(now_ms());
  }

  return state.status;
}

void print_status_error(const char* prefix, Status status) {
  if (status.code == StatusCode::PlcError) {
    std::fprintf(stderr, "%s: %s (0x%04X)\n", prefix, status.message, status.plc_error_code);
    return;
  }
  std::fprintf(stderr, "%s: %s\n", prefix, status.message);
}

void print_probe_status(std::string_view label, Status status) {
  if (status.code == StatusCode::PlcError) {
    std::printf("%-5.*s error 0x%04X\n",
                static_cast<int>(label.size()),
                label.data(),
                status.plc_error_code);
    return;
  }
  std::printf("%-5.*s error %s\n",
              static_cast<int>(label.size()),
              label.data(),
              status.message);
}

void print_probe_write_status(std::string_view label, const char* stage, Status status) {
  if (status.code == StatusCode::PlcError) {
    std::printf("%-5.*s %s error 0x%04X\n",
                static_cast<int>(label.size()),
                label.data(),
                stage,
                status.plc_error_code);
    return;
  }
  std::printf("%-5.*s %s error %s\n",
              static_cast<int>(label.size()),
              label.data(),
              stage,
              status.message);
}

[[nodiscard]] bool parse_bit_value(std::string_view text, BitValue& out_value) {
  if (text == "0" || text == "off" || text == "OFF") {
    out_value = BitValue::Off;
    return true;
  }
  if (text == "1" || text == "on" || text == "ON") {
    out_value = BitValue::On;
    return true;
  }
  return false;
}

[[nodiscard]] bool is_bit_device(DeviceCode code) {
  switch (code) {
    case DeviceCode::X:
    case DeviceCode::Y:
    case DeviceCode::M:
    case DeviceCode::L:
    case DeviceCode::F:
    case DeviceCode::V:
    case DeviceCode::B:
    case DeviceCode::TS:
    case DeviceCode::TC:
    case DeviceCode::STS:
    case DeviceCode::STC:
    case DeviceCode::CS:
    case DeviceCode::CC:
    case DeviceCode::SB:
    case DeviceCode::S:
    case DeviceCode::DX:
    case DeviceCode::DY:
      return true;
    case DeviceCode::D:
    case DeviceCode::W:
    case DeviceCode::TN:
    case DeviceCode::STN:
    case DeviceCode::CN:
    case DeviceCode::SW:
    case DeviceCode::Z:
    case DeviceCode::R:
    case DeviceCode::ZR:
      return false;
  }
  return false;
}

[[nodiscard]] bool parse_word_write_arg(std::string_view text, RandomWriteWordItem& out_item) {
  const std::size_t equal_pos = text.find('=');
  if (equal_pos == std::string_view::npos || equal_pos == 0U || equal_pos == (text.size() - 1U)) {
    return false;
  }
  std::uint32_t value = 0;
  DeviceAddress device {};
  if (!parse_device_address(text.substr(0, equal_pos), device) ||
      !parse_u32_auto(text.substr(equal_pos + 1U), value)) {
    return false;
  }
  out_item.device = device;
  out_item.value = value;
  out_item.double_word = false;
  return true;
}

[[nodiscard]] bool parse_bit_write_arg(std::string_view text, RandomWriteBitItem& out_item) {
  const std::size_t equal_pos = text.find('=');
  if (equal_pos == std::string_view::npos || equal_pos == 0U || equal_pos == (text.size() - 1U)) {
    return false;
  }
  DeviceAddress device {};
  BitValue value = BitValue::Off;
  if (!parse_device_address(text.substr(0, equal_pos), device) ||
      !parse_bit_value(text.substr(equal_pos + 1U), value)) {
    return false;
  }
  out_item.device = device;
  out_item.value = value;
  return true;
}

[[nodiscard]] Status run_batch_write_words_group(
    MelsecSerialClient& client,
    PosixSerialPort& port,
    CommandState& command_state,
    const DeviceAddress& head_device,
    std::span<const std::uint16_t> values) {
  command_state.done = false;
  command_state.status = Status {};

  const Status start_status = client.async_batch_write_words(
      now_ms(),
      BatchWriteWordsRequest {
          .head_device = head_device,
          .words = values,
      },
      request_complete,
      &command_state);
  if (!start_status.ok()) {
    return start_status;
  }
  return drive_request(client, port, command_state);
}

[[nodiscard]] Status run_extended_batch_read_words(
    MelsecSerialClient& client,
    PosixSerialPort& port,
    CommandState& command_state,
    const QualifiedBufferWordDevice& device,
    std::uint16_t points,
    std::span<std::uint16_t> out_values) {
  command_state.done = false;
  command_state.status = Status {};

  const Status start_status = client.async_extended_batch_read_words(
      now_ms(),
      device,
      points,
      out_values,
      request_complete,
      &command_state);
  if (!start_status.ok()) {
    return start_status;
  }
  return drive_request(client, port, command_state);
}

[[nodiscard]] Status run_extended_batch_write_words(
    MelsecSerialClient& client,
    PosixSerialPort& port,
    CommandState& command_state,
    const QualifiedBufferWordDevice& device,
    std::span<const std::uint16_t> values) {
  command_state.done = false;
  command_state.status = Status {};

  const Status start_status = client.async_extended_batch_write_words(
      now_ms(),
      device,
      values,
      request_complete,
      &command_state);
  if (!start_status.ok()) {
    return start_status;
  }
  return drive_request(client, port, command_state);
}

[[nodiscard]] Status run_batch_read_words_group(
    MelsecSerialClient& client,
    PosixSerialPort& port,
    CommandState& command_state,
    const DeviceAddress& head_device,
    std::uint16_t points,
    std::span<std::uint16_t> out_values) {
  command_state.done = false;
  command_state.status = Status {};

  const Status start_status = client.async_batch_read_words(
      now_ms(),
      BatchReadWordsRequest {
          .head_device = head_device,
          .points = points,
      },
      out_values,
      request_complete,
      &command_state);
  if (!start_status.ok()) {
    return start_status;
  }
  return drive_request(client, port, command_state);
}

[[nodiscard]] Status run_batch_read_word(
    MelsecSerialClient& client,
    PosixSerialPort& port,
    CommandState& command_state,
    const DeviceAddress& device,
    std::uint16_t& out_value) {
  command_state.done = false;
  command_state.status = Status {};

  const Status start_status = client.async_batch_read_words(
      now_ms(),
      BatchReadWordsRequest {
          .head_device = device,
          .points = 1,
      },
      std::span<std::uint16_t>(&out_value, 1U),
      request_complete,
      &command_state);
  if (!start_status.ok()) {
    return start_status;
  }
  return drive_request(client, port, command_state);
}

[[nodiscard]] Status run_batch_read_bit(
    MelsecSerialClient& client,
    PosixSerialPort& port,
    CommandState& command_state,
    const DeviceAddress& device,
    BitValue& out_value) {
  command_state.done = false;
  command_state.status = Status {};

  const Status start_status = client.async_batch_read_bits(
      now_ms(),
      BatchReadBitsRequest {
          .head_device = device,
          .points = 1,
      },
      std::span<BitValue>(&out_value, 1U),
      request_complete,
      &command_state);
  if (!start_status.ok()) {
    return start_status;
  }
  return drive_request(client, port, command_state);
}

[[nodiscard]] Status run_batch_read_bits_group(
    MelsecSerialClient& client,
    PosixSerialPort& port,
    CommandState& command_state,
    const DeviceAddress& head_device,
    std::uint16_t points,
    std::span<BitValue> out_values) {
  command_state.done = false;
  command_state.status = Status {};

  const Status start_status = client.async_batch_read_bits(
      now_ms(),
      BatchReadBitsRequest {
          .head_device = head_device,
          .points = points,
      },
      out_values,
      request_complete,
      &command_state);
  if (!start_status.ok()) {
    return start_status;
  }
  return drive_request(client, port, command_state);
}

[[nodiscard]] Status run_batch_write_word(
    MelsecSerialClient& client,
    PosixSerialPort& port,
    CommandState& command_state,
    const DeviceAddress& device,
    std::uint16_t value) {
  const std::array<std::uint16_t, 1> values {value};
  return run_batch_write_words_group(client, port, command_state, device, values);
}

[[nodiscard]] Status run_batch_write_bits_group(
    MelsecSerialClient& client,
    PosixSerialPort& port,
    CommandState& command_state,
    const DeviceAddress& head_device,
    std::span<const BitValue> values) {
  command_state.done = false;
  command_state.status = Status {};

  const Status start_status = client.async_batch_write_bits(
      now_ms(),
      BatchWriteBitsRequest {
          .head_device = head_device,
          .bits = values,
      },
      request_complete,
      &command_state);
  if (!start_status.ok()) {
    return start_status;
  }
  return drive_request(client, port, command_state);
}

[[nodiscard]] Status run_batch_write_bit(
    MelsecSerialClient& client,
    PosixSerialPort& port,
    CommandState& command_state,
    const DeviceAddress& device,
    BitValue value) {
  const std::array<BitValue, 1> values {value};
  return run_batch_write_bits_group(client, port, command_state, device, values);
}

[[nodiscard]] Status run_random_write_words(
    MelsecSerialClient& client,
    PosixSerialPort& port,
    CommandState& command_state,
    std::span<const RandomWriteWordItem> items) {
  command_state.done = false;
  command_state.status = Status {};

  const Status start_status = client.async_random_write_words(
      now_ms(),
      items,
      request_complete,
      &command_state);
  if (!start_status.ok()) {
    return start_status;
  }
  return drive_request(client, port, command_state);
}

[[nodiscard]] Status run_random_write_bits(
    MelsecSerialClient& client,
    PosixSerialPort& port,
    CommandState& command_state,
    std::span<const RandomWriteBitItem> items) {
  command_state.done = false;
  command_state.status = Status {};

  const Status start_status = client.async_random_write_bits(
      now_ms(),
      items,
      request_complete,
      &command_state);
  if (!start_status.ok()) {
    return start_status;
  }
  return drive_request(client, port, command_state);
}

[[nodiscard]] Status run_multi_block_read(
    MelsecSerialClient& client,
    PosixSerialPort& port,
    CommandState& command_state,
    const MultiBlockReadRequest& request,
    std::span<std::uint16_t> out_words,
    std::span<BitValue> out_bits,
    std::span<MultiBlockReadBlockResult> out_results) {
  command_state.done = false;
  command_state.status = Status {};

  const Status start_status = client.async_multi_block_read(
      now_ms(),
      request,
      out_words,
      out_bits,
      out_results,
      request_complete,
      &command_state);
  if (!start_status.ok()) {
    return start_status;
  }
  return drive_request(client, port, command_state);
}

[[nodiscard]] Status run_multi_block_write(
    MelsecSerialClient& client,
    PosixSerialPort& port,
    CommandState& command_state,
    const MultiBlockWriteRequest& request) {
  command_state.done = false;
  command_state.status = Status {};

  const Status start_status = client.async_multi_block_write(
      now_ms(),
      request,
      request_complete,
      &command_state);
  if (!start_status.ok()) {
    return start_status;
  }
  return drive_request(client, port, command_state);
}

[[nodiscard]] Status run_register_monitor(
    MelsecSerialClient& client,
    PosixSerialPort& port,
    CommandState& command_state,
    std::span<const RandomReadItem> items) {
  command_state.done = false;
  command_state.status = Status {};

  const Status start_status = client.async_register_monitor(
      now_ms(),
      mcprotocol::serial::MonitorRegistration {.items = items},
      request_complete,
      &command_state);
  if (!start_status.ok()) {
    return start_status;
  }
  return drive_request(client, port, command_state);
}

[[nodiscard]] Status run_read_monitor(
    MelsecSerialClient& client,
    PosixSerialPort& port,
    CommandState& command_state,
    std::span<std::uint32_t> out_values) {
  command_state.done = false;
  command_state.status = Status {};

  const Status start_status = client.async_read_monitor(
      now_ms(),
      out_values,
      request_complete,
      &command_state);
  if (!start_status.ok()) {
    return start_status;
  }
  return drive_request(client, port, command_state);
}

[[nodiscard]] Status run_read_host_buffer(
    MelsecSerialClient& client,
    PosixSerialPort& port,
    CommandState& command_state,
    const HostBufferReadRequest& request,
    std::span<std::uint16_t> out_words) {
  command_state.done = false;
  command_state.status = Status {};

  const Status start_status = client.async_read_host_buffer(
      now_ms(),
      request,
      out_words,
      request_complete,
      &command_state);
  if (!start_status.ok()) {
    return start_status;
  }
  return drive_request(client, port, command_state);
}

[[nodiscard]] Status run_read_module_buffer(
    MelsecSerialClient& client,
    PosixSerialPort& port,
    CommandState& command_state,
    const ModuleBufferReadRequest& request,
    std::span<std::byte> out_bytes) {
  command_state.done = false;
  command_state.status = Status {};

  const Status start_status = client.async_read_module_buffer(
      now_ms(),
      request,
      out_bytes,
      request_complete,
      &command_state);
  if (!start_status.ok()) {
    return start_status;
  }
  return drive_request(client, port, command_state);
}

[[nodiscard]] Status run_write_host_buffer(
    MelsecSerialClient& client,
    PosixSerialPort& port,
    CommandState& command_state,
    const HostBufferWriteRequest& request) {
  command_state.done = false;
  command_state.status = Status {};

  const Status start_status = client.async_write_host_buffer(
      now_ms(),
      request,
      request_complete,
      &command_state);
  if (!start_status.ok()) {
    return start_status;
  }
  return drive_request(client, port, command_state);
}

[[nodiscard]] Status run_write_module_buffer(
    MelsecSerialClient& client,
    PosixSerialPort& port,
    CommandState& command_state,
    const ModuleBufferWriteRequest& request) {
  command_state.done = false;
  command_state.status = Status {};

  const Status start_status = client.async_write_module_buffer(
      now_ms(),
      request,
      request_complete,
      &command_state);
  if (!start_status.ok()) {
    return start_status;
  }
  return drive_request(client, port, command_state);
}

}  // namespace

int main(int argc, char** argv) {
  CliOptions options;
  if (!parse_args(argc, argv, options)) {
    print_usage();
    return 2;
  }
  g_dump_frames = options.dump_frames;

  PosixSerialPort port;
  Status status = port.open(options.serial);
  if (!status.ok()) {
    print_status_error("Failed to open serial port", status);
    return 1;
  }

  MelsecSerialClient client;
  if (options.rts_toggle) {
    client.set_rs485_hooks(Rs485Hooks {
        .on_tx_begin = on_tx_begin,
        .on_tx_end = on_tx_end,
        .user = &port,
    });
  }

  status = client.configure(options.protocol);
  if (!status.ok()) {
    print_status_error("Invalid protocol configuration", status);
    return 1;
  }

  CommandState command_state;
  switch (options.command) {
    case CommandKind::CpuModel: {
      CpuModelInfo info {};
      status = client.async_read_cpu_model(now_ms(), info, request_complete, &command_state);
      if (!status.ok()) {
        print_status_error("Failed to start cpu-model request", status);
        return 1;
      }
      status = drive_request(client, port, command_state);
      if (!status.ok()) {
        print_status_error("cpu-model request failed", status);
        return 1;
      }
      std::printf("model_name=%s\nmodel_code=0x%04X\n", info.model_name.data(), info.model_code);
      return 0;
    }

    case CommandKind::Loopback: {
      std::array<char, mcprotocol::serial::kMaxLoopbackBytes + 1U> echoed {};
      status = client.async_loopback(
          now_ms(),
          std::string_view(options.command_argv[0]),
          std::span<char>(echoed.data(), echoed.size()),
          request_complete,
          &command_state);
      if (!status.ok()) {
        print_status_error("Failed to start loopback request", status);
        return 1;
      }
      status = drive_request(client, port, command_state);
      if (!status.ok()) {
        print_status_error("loopback request failed", status);
        return 1;
      }
      std::printf("loopback=%s\n", echoed.data());
      return 0;
    }

    case CommandKind::ProbeAll: {
      std::size_t success_count = 0;
      std::size_t failure_count = 0;
      for (const ProbeTarget& target : kProbeTargets) {
        if (is_bit_device(target.device.code)) {
          BitValue bit = BitValue::Off;
          status = run_batch_read_bit(client, port, command_state, target.device, bit);
          if (!status.ok()) {
            print_probe_status(target.label, status);
            ++failure_count;
            continue;
          }
          std::printf("%-5.*s %u\n",
                      static_cast<int>(target.label.size()),
                      target.label.data(),
                      bit == BitValue::On ? 1U : 0U);
        } else {
          std::uint16_t word = 0;
          status = run_batch_read_word(client, port, command_state, target.device, word);
          if (!status.ok()) {
            print_probe_status(target.label, status);
            ++failure_count;
            continue;
          }
          std::printf("%-5.*s 0x%04X %u\n",
                      static_cast<int>(target.label.size()),
                      target.label.data(),
                      word,
                      word);
        }
        ++success_count;
      }
      std::printf("probe-all: success=%zu failed=%zu\n", success_count, failure_count);
      return failure_count == 0U ? 0 : 1;
    }

    case CommandKind::ProbeWriteAll: {
      std::size_t success_count = 0;
      std::size_t failure_count = 0;
      for (const ProbeTarget& target : kProbeWriteTargets) {
        if (is_bit_device(target.device.code)) {
          BitValue original = BitValue::Off;
          status = run_batch_read_bit(client, port, command_state, target.device, original);
          if (!status.ok()) {
            print_probe_write_status(target.label, "read", status);
            ++failure_count;
            continue;
          }

          const BitValue test_value = original == BitValue::On ? BitValue::Off : BitValue::On;
          status = run_batch_write_bit(client, port, command_state, target.device, test_value);
          if (!status.ok()) {
            print_probe_write_status(target.label, "write", status);
            ++failure_count;
            continue;
          }

          BitValue readback = BitValue::Off;
          status = run_batch_read_bit(client, port, command_state, target.device, readback);
          if (!status.ok()) {
            print_probe_write_status(target.label, "verify", status);
            ++failure_count;
            continue;
          }
          if (readback != test_value) {
            std::printf("%-5.*s verify mismatch wrote=%u read=%u\n",
                        static_cast<int>(target.label.size()),
                        target.label.data(),
                        test_value == BitValue::On ? 1U : 0U,
                        readback == BitValue::On ? 1U : 0U);
            ++failure_count;
            continue;
          }

          status = run_batch_write_bit(client, port, command_state, target.device, original);
          if (!status.ok()) {
            print_probe_write_status(target.label, "restore", status);
            ++failure_count;
            continue;
          }

          BitValue restored = BitValue::Off;
          status = run_batch_read_bit(client, port, command_state, target.device, restored);
          if (!status.ok()) {
            print_probe_write_status(target.label, "re-read", status);
            ++failure_count;
            continue;
          }
          if (restored != original) {
            std::printf("%-5.*s restore mismatch expected=%u read=%u\n",
                        static_cast<int>(target.label.size()),
                        target.label.data(),
                        original == BitValue::On ? 1U : 0U,
                        restored == BitValue::On ? 1U : 0U);
            ++failure_count;
            continue;
          }

          std::printf("%-5.*s ok %u->%u->%u\n",
                      static_cast<int>(target.label.size()),
                      target.label.data(),
                      original == BitValue::On ? 1U : 0U,
                      test_value == BitValue::On ? 1U : 0U,
                      restored == BitValue::On ? 1U : 0U);
        } else {
          std::uint16_t original = 0;
          status = run_batch_read_word(client, port, command_state, target.device, original);
          if (!status.ok()) {
            print_probe_write_status(target.label, "read", status);
            ++failure_count;
            continue;
          }

          const std::uint16_t test_value = static_cast<std::uint16_t>(original ^ 0x0001U);
          status = run_batch_write_word(client, port, command_state, target.device, test_value);
          if (!status.ok()) {
            print_probe_write_status(target.label, "write", status);
            ++failure_count;
            continue;
          }

          std::uint16_t readback = 0;
          status = run_batch_read_word(client, port, command_state, target.device, readback);
          if (!status.ok()) {
            print_probe_write_status(target.label, "verify", status);
            ++failure_count;
            continue;
          }
          if (readback != test_value) {
            std::printf("%-5.*s verify mismatch wrote=0x%04X read=0x%04X\n",
                        static_cast<int>(target.label.size()),
                        target.label.data(),
                        test_value,
                        readback);
            ++failure_count;
            continue;
          }

          status = run_batch_write_word(client, port, command_state, target.device, original);
          if (!status.ok()) {
            print_probe_write_status(target.label, "restore", status);
            ++failure_count;
            continue;
          }

          std::uint16_t restored = 0;
          status = run_batch_read_word(client, port, command_state, target.device, restored);
          if (!status.ok()) {
            print_probe_write_status(target.label, "re-read", status);
            ++failure_count;
            continue;
          }
          if (restored != original) {
            std::printf("%-5.*s restore mismatch expected=0x%04X read=0x%04X\n",
                        static_cast<int>(target.label.size()),
                        target.label.data(),
                        original,
                        restored);
            ++failure_count;
            continue;
          }

          std::printf("%-5.*s ok 0x%04X->0x%04X->0x%04X\n",
                      static_cast<int>(target.label.size()),
                      target.label.data(),
                      original,
                      test_value,
                      restored);
        }
        ++success_count;
      }
      std::printf("probe-write-all: success=%zu failed=%zu\n", success_count, failure_count);
      return failure_count == 0U ? 0 : 1;
    }

    case CommandKind::ProbeMultiBlock: {
      constexpr DeviceAddress kWordBlockADevice {.code = mcprotocol::serial::DeviceCode::D, .number = 100};
      constexpr DeviceAddress kWordBlockBDevice {.code = mcprotocol::serial::DeviceCode::D, .number = 110};
      constexpr DeviceAddress kBitBlockADevice {.code = mcprotocol::serial::DeviceCode::M, .number = 100};
      constexpr DeviceAddress kBitBlockBDevice {.code = mcprotocol::serial::DeviceCode::M, .number = 200};

      std::array<std::uint16_t, 2> backup_word_block_a {};
      std::array<std::uint16_t, 3> backup_word_block_b {};
      std::array<BitValue, 16> backup_bit_block_a {};
      std::array<BitValue, 32> backup_bit_block_b {};
      bool backups_valid = false;

      const auto restore_originals = [&]() -> bool {
        if (!backups_valid) {
          return true;
        }
        status = run_batch_write_words_group(
            client,
            port,
            command_state,
            kWordBlockADevice,
            std::span<const std::uint16_t>(backup_word_block_a.data(), backup_word_block_a.size()));
        if (!status.ok()) {
          print_status_error("probe-multi-block restore words A failed", status);
          return false;
        }
        status = run_batch_write_words_group(
            client,
            port,
            command_state,
            kWordBlockBDevice,
            std::span<const std::uint16_t>(backup_word_block_b.data(), backup_word_block_b.size()));
        if (!status.ok()) {
          print_status_error("probe-multi-block restore words B failed", status);
          return false;
        }
        status = run_batch_write_bits_group(
            client,
            port,
            command_state,
            kBitBlockADevice,
            std::span<const BitValue>(backup_bit_block_a.data(), backup_bit_block_a.size()));
        if (!status.ok()) {
          print_status_error("probe-multi-block restore bits A failed", status);
          return false;
        }
        status = run_batch_write_bits_group(
            client,
            port,
            command_state,
            kBitBlockBDevice,
            std::span<const BitValue>(backup_bit_block_b.data(), backup_bit_block_b.size()));
        if (!status.ok()) {
          print_status_error("probe-multi-block restore bits B failed", status);
          return false;
        }
        return true;
      };

      status = run_batch_read_words_group(
          client,
          port,
          command_state,
          kWordBlockADevice,
          static_cast<std::uint16_t>(backup_word_block_a.size()),
          std::span<std::uint16_t>(backup_word_block_a.data(), backup_word_block_a.size()));
      if (!status.ok()) {
        print_status_error("probe-multi-block backup words A failed", status);
        return 1;
      }
      status = run_batch_read_words_group(
          client,
          port,
          command_state,
          kWordBlockBDevice,
          static_cast<std::uint16_t>(backup_word_block_b.size()),
          std::span<std::uint16_t>(backup_word_block_b.data(), backup_word_block_b.size()));
      if (!status.ok()) {
        print_status_error("probe-multi-block backup words B failed", status);
        return 1;
      }
      status = run_batch_read_bits_group(
          client,
          port,
          command_state,
          kBitBlockADevice,
          static_cast<std::uint16_t>(backup_bit_block_a.size()),
          std::span<BitValue>(backup_bit_block_a.data(), backup_bit_block_a.size()));
      if (!status.ok()) {
        print_status_error("probe-multi-block backup bits A failed", status);
        return 1;
      }
      status = run_batch_read_bits_group(
          client,
          port,
          command_state,
          kBitBlockBDevice,
          static_cast<std::uint16_t>(backup_bit_block_b.size()),
          std::span<BitValue>(backup_bit_block_b.data(), backup_bit_block_b.size()));
      if (!status.ok()) {
        print_status_error("probe-multi-block backup bits B failed", status);
        return 1;
      }
      backups_valid = true;

      const std::array<MultiBlockReadBlock, 4> read_blocks {{
          {.head_device = kWordBlockADevice, .points = 2, .bit_block = false},
          {.head_device = kWordBlockBDevice, .points = 3, .bit_block = false},
          {.head_device = kBitBlockADevice, .points = 1, .bit_block = true},
          {.head_device = kBitBlockBDevice, .points = 2, .bit_block = true},
      }};

      std::array<std::uint16_t, 5> multi_read_words {};
      std::array<BitValue, 48> multi_read_bits {};
      std::array<MultiBlockReadBlockResult, 4> multi_read_results {};
      bool multi_read_ok = false;

      status = run_multi_block_read(
          client,
          port,
          command_state,
          MultiBlockReadRequest {
              .blocks = std::span<const MultiBlockReadBlock>(read_blocks.data(), read_blocks.size()),
          },
          std::span<std::uint16_t>(multi_read_words.data(), multi_read_words.size()),
          std::span<BitValue>(multi_read_bits.data(), multi_read_bits.size()),
          std::span<MultiBlockReadBlockResult>(multi_read_results.data(), multi_read_results.size()));
      if (!status.ok()) {
        std::printf("multi-block-read=skip ");
        if (status.code == StatusCode::PlcError) {
          std::printf("0x%04X\n", status.plc_error_code);
        } else {
          std::printf("%s\n", status.message);
        }
      } else {
        bool matches_backup = true;
        for (std::size_t index = 0; index < backup_word_block_a.size(); ++index) {
          if (multi_read_words[index] != backup_word_block_a[index]) {
            matches_backup = false;
            break;
          }
        }
        for (std::size_t index = 0; matches_backup && index < backup_word_block_b.size(); ++index) {
          if (multi_read_words[backup_word_block_a.size() + index] != backup_word_block_b[index]) {
            matches_backup = false;
            break;
          }
        }
        for (std::size_t index = 0; matches_backup && index < backup_bit_block_a.size(); ++index) {
          if (multi_read_bits[index] != backup_bit_block_a[index]) {
            matches_backup = false;
            break;
          }
        }
        for (std::size_t index = 0; matches_backup && index < backup_bit_block_b.size(); ++index) {
          if (multi_read_bits[backup_bit_block_a.size() + index] != backup_bit_block_b[index]) {
            matches_backup = false;
            break;
          }
        }
        if (!matches_backup) {
          std::printf("multi-block-read=skip verify-mismatch\n");
        } else {
          multi_read_ok = true;
          std::printf("multi-block-read=ok native\n");
        }
      }

      const std::array<std::uint16_t, 2> test_word_block_a {0x1357U, 0x2468U};
      const std::array<std::uint16_t, 3> test_word_block_b {0x1111U, 0x2222U, 0x3333U};
      const std::array<BitValue, 16> test_bit_block_a {{
          BitValue::On, BitValue::Off, BitValue::On, BitValue::Off,
          BitValue::On, BitValue::Off, BitValue::On, BitValue::Off,
          BitValue::On, BitValue::Off, BitValue::On, BitValue::Off,
          BitValue::On, BitValue::Off, BitValue::On, BitValue::Off,
      }};
      const std::array<BitValue, 32> test_bit_block_b {{
          BitValue::Off, BitValue::Off, BitValue::On, BitValue::On,
          BitValue::Off, BitValue::Off, BitValue::On, BitValue::On,
          BitValue::On, BitValue::On, BitValue::Off, BitValue::Off,
          BitValue::On, BitValue::On, BitValue::Off, BitValue::Off,
          BitValue::Off, BitValue::On, BitValue::Off, BitValue::On,
          BitValue::Off, BitValue::On, BitValue::Off, BitValue::On,
          BitValue::On, BitValue::Off, BitValue::On, BitValue::Off,
          BitValue::On, BitValue::Off, BitValue::On, BitValue::Off,
      }};
      const std::array<MultiBlockWriteBlock, 4> write_blocks {{
          {.head_device = kWordBlockADevice,
           .points = static_cast<std::uint16_t>(test_word_block_a.size()),
           .bit_block = false,
           .words = std::span<const std::uint16_t>(test_word_block_a.data(), test_word_block_a.size())},
          {.head_device = kWordBlockBDevice,
           .points = static_cast<std::uint16_t>(test_word_block_b.size()),
           .bit_block = false,
           .words = std::span<const std::uint16_t>(test_word_block_b.data(), test_word_block_b.size())},
          {.head_device = kBitBlockADevice,
           .points = 1,
           .bit_block = true,
           .bits = std::span<const BitValue>(test_bit_block_a.data(), test_bit_block_a.size())},
          {.head_device = kBitBlockBDevice,
           .points = 2,
           .bit_block = true,
           .bits = std::span<const BitValue>(test_bit_block_b.data(), test_bit_block_b.size())},
      }};

      bool multi_write_ok = false;
      status = run_multi_block_write(
          client,
          port,
          command_state,
          MultiBlockWriteRequest {
              .blocks = std::span<const MultiBlockWriteBlock>(write_blocks.data(), write_blocks.size()),
          });
      if (!status.ok()) {
        std::printf("multi-block-write=skip ");
        if (status.code == StatusCode::PlcError) {
          std::printf("0x%04X\n", status.plc_error_code);
        } else {
          std::printf("%s\n", status.message);
        }
      } else {
        std::array<std::uint16_t, 2> verify_word_block_a {};
        std::array<std::uint16_t, 3> verify_word_block_b {};
        std::array<BitValue, 16> verify_bit_block_a {};
        std::array<BitValue, 32> verify_bit_block_b {};

        status = run_batch_read_words_group(
            client,
            port,
            command_state,
            kWordBlockADevice,
            static_cast<std::uint16_t>(verify_word_block_a.size()),
            std::span<std::uint16_t>(verify_word_block_a.data(), verify_word_block_a.size()));
        if (status.ok()) {
          status = run_batch_read_words_group(
              client,
              port,
              command_state,
              kWordBlockBDevice,
              static_cast<std::uint16_t>(verify_word_block_b.size()),
              std::span<std::uint16_t>(verify_word_block_b.data(), verify_word_block_b.size()));
        }
        if (status.ok()) {
          status = run_batch_read_bits_group(
              client,
              port,
              command_state,
              kBitBlockADevice,
              static_cast<std::uint16_t>(verify_bit_block_a.size()),
              std::span<BitValue>(verify_bit_block_a.data(), verify_bit_block_a.size()));
        }
        if (status.ok()) {
          status = run_batch_read_bits_group(
              client,
              port,
              command_state,
              kBitBlockBDevice,
              static_cast<std::uint16_t>(verify_bit_block_b.size()),
              std::span<BitValue>(verify_bit_block_b.data(), verify_bit_block_b.size()));
        }

        bool matches_test = status.ok();
        for (std::size_t index = 0; matches_test && index < test_word_block_a.size(); ++index) {
          if (verify_word_block_a[index] != test_word_block_a[index]) {
            matches_test = false;
          }
        }
        for (std::size_t index = 0; matches_test && index < test_word_block_b.size(); ++index) {
          if (verify_word_block_b[index] != test_word_block_b[index]) {
            matches_test = false;
          }
        }
        for (std::size_t index = 0; matches_test && index < test_bit_block_a.size(); ++index) {
          if (verify_bit_block_a[index] != test_bit_block_a[index]) {
            matches_test = false;
          }
        }
        for (std::size_t index = 0; matches_test && index < test_bit_block_b.size(); ++index) {
          if (verify_bit_block_b[index] != test_bit_block_b[index]) {
            matches_test = false;
          }
        }

        if (!matches_test) {
          if (status.ok()) {
            std::printf("multi-block-write=skip verify-mismatch\n");
          } else if (status.code == StatusCode::PlcError) {
            std::printf("multi-block-write=skip verify-0x%04X\n", status.plc_error_code);
          } else {
            std::printf("multi-block-write=skip verify-%s\n", status.message);
          }
        } else {
          multi_write_ok = true;
          std::printf("multi-block-write=ok native\n");
        }
      }

      if (!restore_originals()) {
        return 1;
      }

      std::array<std::uint16_t, 2> restore_word_block_a {};
      std::array<std::uint16_t, 3> restore_word_block_b {};
      std::array<BitValue, 16> restore_bit_block_a {};
      std::array<BitValue, 32> restore_bit_block_b {};
      status = run_batch_read_words_group(
          client,
          port,
          command_state,
          kWordBlockADevice,
          static_cast<std::uint16_t>(restore_word_block_a.size()),
          std::span<std::uint16_t>(restore_word_block_a.data(), restore_word_block_a.size()));
      if (!status.ok()) {
        print_status_error("probe-multi-block restore verify words A failed", status);
        return 1;
      }
      status = run_batch_read_words_group(
          client,
          port,
          command_state,
          kWordBlockBDevice,
          static_cast<std::uint16_t>(restore_word_block_b.size()),
          std::span<std::uint16_t>(restore_word_block_b.data(), restore_word_block_b.size()));
      if (!status.ok()) {
        print_status_error("probe-multi-block restore verify words B failed", status);
        return 1;
      }
      status = run_batch_read_bits_group(
          client,
          port,
          command_state,
          kBitBlockADevice,
          static_cast<std::uint16_t>(restore_bit_block_a.size()),
          std::span<BitValue>(restore_bit_block_a.data(), restore_bit_block_a.size()));
      if (!status.ok()) {
        print_status_error("probe-multi-block restore verify bits A failed", status);
        return 1;
      }
      status = run_batch_read_bits_group(
          client,
          port,
          command_state,
          kBitBlockBDevice,
          static_cast<std::uint16_t>(restore_bit_block_b.size()),
          std::span<BitValue>(restore_bit_block_b.data(), restore_bit_block_b.size()));
      if (!status.ok()) {
        print_status_error("probe-multi-block restore verify bits B failed", status);
        return 1;
      }
      for (std::size_t index = 0; index < backup_word_block_a.size(); ++index) {
        if (restore_word_block_a[index] != backup_word_block_a[index]) {
          std::fprintf(stderr, "probe-multi-block restore mismatch words A\n");
          return 1;
        }
      }
      for (std::size_t index = 0; index < backup_word_block_b.size(); ++index) {
        if (restore_word_block_b[index] != backup_word_block_b[index]) {
          std::fprintf(stderr, "probe-multi-block restore mismatch words B\n");
          return 1;
        }
      }
      for (std::size_t index = 0; index < backup_bit_block_a.size(); ++index) {
        if (restore_bit_block_a[index] != backup_bit_block_a[index]) {
          std::fprintf(stderr, "probe-multi-block restore mismatch bits A\n");
          return 1;
        }
      }
      for (std::size_t index = 0; index < backup_bit_block_b.size(); ++index) {
        if (restore_bit_block_b[index] != backup_bit_block_b[index]) {
          std::fprintf(stderr, "probe-multi-block restore mismatch bits B\n");
          return 1;
        }
      }

      std::printf("probe-multi-block: read=%s write=%s restore=ok\n",
                  multi_read_ok ? "native" : "skip",
                  multi_write_ok ? "native" : "skip");
      return (multi_read_ok || multi_write_ok) ? 0 : 1;
    }

    case CommandKind::ProbeMonitor: {
      const std::array<RandomReadItem, 4> items {{
          {.device = {.code = mcprotocol::serial::DeviceCode::D, .number = 100}, .double_word = false},
          {.device = {.code = mcprotocol::serial::DeviceCode::D, .number = 105}, .double_word = false},
          {.device = {.code = mcprotocol::serial::DeviceCode::M, .number = 100}, .double_word = false},
          {.device = {.code = mcprotocol::serial::DeviceCode::M, .number = 105}, .double_word = false},
      }};

      std::array<std::uint32_t, items.size()> monitor_values {};
      status = run_register_monitor(
          client,
          port,
          command_state,
          std::span<const RandomReadItem>(items.data(), items.size()));
      if (!status.ok()) {
        std::printf("probe-monitor: skip register ");
        if (status.code == StatusCode::PlcError) {
          std::printf("0x%04X\n", status.plc_error_code);
        } else {
          std::printf("%s\n", status.message);
        }
        return 1;
      }

      status = run_read_monitor(
          client,
          port,
          command_state,
          std::span<std::uint32_t>(monitor_values.data(), monitor_values.size()));
      if (!status.ok()) {
        std::printf("probe-monitor: skip read ");
        if (status.code == StatusCode::PlcError) {
          std::printf("0x%04X\n", status.plc_error_code);
        } else {
          std::printf("%s\n", status.message);
        }
        return 1;
      }

      std::uint16_t expected_d100 = 0;
      std::uint16_t expected_d105 = 0;
      BitValue expected_m100 = BitValue::Off;
      BitValue expected_m105 = BitValue::Off;

      status = run_batch_read_word(client, port, command_state, items[0].device, expected_d100);
      if (status.ok()) {
        status = run_batch_read_word(client, port, command_state, items[1].device, expected_d105);
      }
      if (status.ok()) {
        status = run_batch_read_bit(client, port, command_state, items[2].device, expected_m100);
      }
      if (status.ok()) {
        status = run_batch_read_bit(client, port, command_state, items[3].device, expected_m105);
      }
      if (!status.ok()) {
        print_status_error("probe-monitor verify read failed", status);
        return 1;
      }

      const std::array<std::uint32_t, items.size()> expected_values {
          expected_d100,
          expected_d105,
          expected_m100 == BitValue::On ? 1U : 0U,
          expected_m105 == BitValue::On ? 1U : 0U,
      };
      for (std::size_t index = 0; index < expected_values.size(); ++index) {
        if (monitor_values[index] != expected_values[index]) {
          std::printf("probe-monitor: skip verify-mismatch index=%zu expected=%u read=%u\n",
                      index,
                      expected_values[index],
                      monitor_values[index]);
          return 1;
        }
      }

      std::printf("probe-monitor=ok mode=native D100=%u D105=%u M100=%u M105=%u\n",
                  static_cast<unsigned>(monitor_values[0]),
                  static_cast<unsigned>(monitor_values[1]),
                  static_cast<unsigned>(monitor_values[2]),
                  static_cast<unsigned>(monitor_values[3]));
      return 0;
    }

    case CommandKind::ProbeHostBuffer: {
      std::array<std::uint16_t, 1> words {};
      status = run_read_host_buffer(
          client,
          port,
          command_state,
          HostBufferReadRequest {
              .start_address = 0U,
              .word_length = 1U,
          },
          std::span<std::uint16_t>(words.data(), words.size()));
      if (!status.ok()) {
        std::printf("probe-host-buffer: skip ");
        if (status.code == StatusCode::PlcError) {
          std::printf("0x%04X\n", status.plc_error_code);
        } else {
          std::printf("%s\n", status.message);
        }
        return 1;
      }
      std::printf("probe-host-buffer=ok start=0 words=1 value=0x%04X %u\n", words[0], words[0]);
      return 0;
    }

    case CommandKind::ProbeModuleBuffer: {
      std::array<std::byte, 2> bytes {};
      status = run_read_module_buffer(
          client,
          port,
          command_state,
          ModuleBufferReadRequest {
              .start_address = 0U,
              .bytes = static_cast<std::uint16_t>(bytes.size()),
              .module_number = 0U,
          },
          std::span<std::byte>(bytes.data(), bytes.size()));
      if (!status.ok()) {
        std::printf("probe-module-buffer: skip ");
        if (status.code == StatusCode::PlcError) {
          std::printf("0x%04X\n", status.plc_error_code);
        } else {
          std::printf("%s\n", status.message);
        }
        return 1;
      }
      std::printf("probe-module-buffer=ok start=0 bytes=2 module=0 value=%02X %02X\n",
                  static_cast<unsigned>(bytes[0]),
                  static_cast<unsigned>(bytes[1]));
      return 0;
    }

    case CommandKind::ProbeWriteHostBuffer: {
      std::array<std::uint16_t, 1> original {};
      status = run_read_host_buffer(
          client,
          port,
          command_state,
          HostBufferReadRequest {.start_address = 0U, .word_length = 1U},
          std::span<std::uint16_t>(original.data(), original.size()));
      if (!status.ok()) {
        print_status_error("probe-write-host-buffer backup failed", status);
        return 1;
      }

      const std::array<std::uint16_t, 1> test_value {
          static_cast<std::uint16_t>(original[0] ^ 0x0001U),
      };
      status = run_write_host_buffer(
          client,
          port,
          command_state,
          HostBufferWriteRequest {
              .start_address = 0U,
              .words = std::span<const std::uint16_t>(test_value.data(), test_value.size()),
          });
      if (!status.ok()) {
        std::printf("probe-write-host-buffer: skip ");
        if (status.code == StatusCode::PlcError) {
          std::printf("0x%04X\n", status.plc_error_code);
        } else {
          std::printf("%s\n", status.message);
        }
        return 1;
      }

      std::array<std::uint16_t, 1> verify {};
      status = run_read_host_buffer(
          client,
          port,
          command_state,
          HostBufferReadRequest {.start_address = 0U, .word_length = 1U},
          std::span<std::uint16_t>(verify.data(), verify.size()));
      if (!status.ok()) {
        print_status_error("probe-write-host-buffer verify failed", status);
        return 1;
      }
      if (verify[0] != test_value[0]) {
        std::printf("probe-write-host-buffer: skip verify-mismatch expected=0x%04X read=0x%04X\n",
                    test_value[0],
                    verify[0]);
        return 1;
      }

      status = run_write_host_buffer(
          client,
          port,
          command_state,
          HostBufferWriteRequest {
              .start_address = 0U,
              .words = std::span<const std::uint16_t>(original.data(), original.size()),
          });
      if (!status.ok()) {
        print_status_error("probe-write-host-buffer restore failed", status);
        return 1;
      }

      std::array<std::uint16_t, 1> restored {};
      status = run_read_host_buffer(
          client,
          port,
          command_state,
          HostBufferReadRequest {.start_address = 0U, .word_length = 1U},
          std::span<std::uint16_t>(restored.data(), restored.size()));
      if (!status.ok()) {
        print_status_error("probe-write-host-buffer re-read failed", status);
        return 1;
      }
      if (restored[0] != original[0]) {
        std::printf("probe-write-host-buffer restore-mismatch expected=0x%04X read=0x%04X\n",
                    original[0],
                    restored[0]);
        return 1;
      }

      std::printf("probe-write-host-buffer=ok 0x%04X->0x%04X->0x%04X\n",
                  original[0],
                  test_value[0],
                  restored[0]);
      return 0;
    }

    case CommandKind::ProbeWriteModuleBuffer: {
      std::array<std::byte, 2> original {};
      status = run_read_module_buffer(
          client,
          port,
          command_state,
          ModuleBufferReadRequest {
              .start_address = 0U,
              .bytes = static_cast<std::uint16_t>(original.size()),
              .module_number = 0U,
          },
          std::span<std::byte>(original.data(), original.size()));
      if (!status.ok()) {
        print_status_error("probe-write-module-buffer backup failed", status);
        return 1;
      }

      const std::array<std::byte, 2> test_value {
          static_cast<std::byte>(static_cast<unsigned>(original[0]) ^ 0x55U),
          static_cast<std::byte>(static_cast<unsigned>(original[1]) ^ 0xAAU),
      };
      status = run_write_module_buffer(
          client,
          port,
          command_state,
          ModuleBufferWriteRequest {
              .start_address = 0U,
              .module_number = 0U,
              .bytes = std::span<const std::byte>(test_value.data(), test_value.size()),
          });
      if (!status.ok()) {
        std::printf("probe-write-module-buffer: skip ");
        if (status.code == StatusCode::PlcError) {
          std::printf("0x%04X\n", status.plc_error_code);
        } else {
          std::printf("%s\n", status.message);
        }
        return 1;
      }

      std::array<std::byte, 2> verify {};
      status = run_read_module_buffer(
          client,
          port,
          command_state,
          ModuleBufferReadRequest {
              .start_address = 0U,
              .bytes = static_cast<std::uint16_t>(verify.size()),
              .module_number = 0U,
          },
          std::span<std::byte>(verify.data(), verify.size()));
      if (!status.ok()) {
        print_status_error("probe-write-module-buffer verify failed", status);
        return 1;
      }
      if (verify != test_value) {
        std::printf("probe-write-module-buffer: skip verify-mismatch expected=%02X %02X read=%02X %02X\n",
                    static_cast<unsigned>(test_value[0]),
                    static_cast<unsigned>(test_value[1]),
                    static_cast<unsigned>(verify[0]),
                    static_cast<unsigned>(verify[1]));
        return 1;
      }

      status = run_write_module_buffer(
          client,
          port,
          command_state,
          ModuleBufferWriteRequest {
              .start_address = 0U,
              .module_number = 0U,
              .bytes = std::span<const std::byte>(original.data(), original.size()),
          });
      if (!status.ok()) {
        print_status_error("probe-write-module-buffer restore failed", status);
        return 1;
      }

      std::array<std::byte, 2> restored {};
      status = run_read_module_buffer(
          client,
          port,
          command_state,
          ModuleBufferReadRequest {
              .start_address = 0U,
              .bytes = static_cast<std::uint16_t>(restored.size()),
              .module_number = 0U,
          },
          std::span<std::byte>(restored.data(), restored.size()));
      if (!status.ok()) {
        print_status_error("probe-write-module-buffer re-read failed", status);
        return 1;
      }
      if (restored != original) {
        std::printf("probe-write-module-buffer restore-mismatch expected=%02X %02X read=%02X %02X\n",
                    static_cast<unsigned>(original[0]),
                    static_cast<unsigned>(original[1]),
                    static_cast<unsigned>(restored[0]),
                    static_cast<unsigned>(restored[1]));
        return 1;
      }

      std::printf("probe-write-module-buffer=ok %02X %02X -> %02X %02X -> %02X %02X\n",
                  static_cast<unsigned>(original[0]),
                  static_cast<unsigned>(original[1]),
                  static_cast<unsigned>(test_value[0]),
                  static_cast<unsigned>(test_value[1]),
                  static_cast<unsigned>(restored[0]),
                  static_cast<unsigned>(restored[1]));
      return 0;
    }

    case CommandKind::ReadHostBuffer: {
      const std::string_view start_arg(options.command_argv[0]);
      const std::string_view words_arg(options.command_argv[1]);
      std::uint32_t start_address = 0;
      std::uint32_t word_length = 0;
      if (!parse_u32_auto(start_arg, start_address) ||
          !parse_u32(words_arg, word_length) ||
          word_length == 0U ||
          word_length > 480U) {
        std::fprintf(stderr, "Invalid read-host-buffer arguments\n");
        return 2;
      }

      std::array<std::uint16_t, 480> words {};
      status = run_read_host_buffer(
          client,
          port,
          command_state,
          HostBufferReadRequest {
              .start_address = start_address,
              .word_length = static_cast<std::uint16_t>(word_length),
          },
          std::span<std::uint16_t>(words.data(), static_cast<std::size_t>(word_length)));
      if (!status.ok()) {
        print_status_error("read-host-buffer request failed", status);
        return 1;
      }
      for (std::uint32_t index = 0; index < word_length; ++index) {
        std::printf("[%u] 0x%04X %u\n", index, words[index], words[index]);
      }
      return 0;
    }

    case CommandKind::WriteHostBuffer: {
      const std::string_view start_arg(options.command_argv[0]);
      std::uint32_t start_address = 0;
      if (!parse_u32_auto(start_arg, start_address)) {
        std::fprintf(stderr, "Invalid write-host-buffer start address\n");
        return 2;
      }

      std::array<std::uint16_t, 480> words {};
      const int word_count = options.command_argc - 1;
      if (word_count <= 0 || word_count > static_cast<int>(words.size())) {
        std::fprintf(stderr, "Invalid write-host-buffer word count\n");
        return 2;
      }
      for (int index = 0; index < word_count; ++index) {
        std::uint32_t value = 0;
        if (!parse_u32_auto(options.command_argv[index + 1], value) || value > 0xFFFFU) {
          std::fprintf(stderr, "Invalid write-host-buffer value: %s\n", options.command_argv[index + 1]);
          return 2;
        }
        words[static_cast<std::size_t>(index)] = static_cast<std::uint16_t>(value);
      }

      status = run_write_host_buffer(
          client,
          port,
          command_state,
          HostBufferWriteRequest {
              .start_address = start_address,
              .words = std::span<const std::uint16_t>(words.data(), static_cast<std::size_t>(word_count)),
          });
      if (!status.ok()) {
        print_status_error("write-host-buffer request failed", status);
        return 1;
      }
      std::printf("write-host-buffer=ok\n");
      return 0;
    }

    case CommandKind::ReadModuleBuffer: {
      const std::string_view start_arg(options.command_argv[0]);
      const std::string_view bytes_arg(options.command_argv[1]);
      const std::string_view module_arg(options.command_argv[2]);
      std::uint32_t start_address = 0;
      std::uint32_t byte_length = 0;
      std::uint32_t module_number = 0;
      if (!parse_u32_auto(start_arg, start_address) ||
          !parse_u32(bytes_arg, byte_length) ||
          !parse_u32_auto(module_arg, module_number) ||
          byte_length < 2U ||
          byte_length > 1920U) {
        std::fprintf(stderr, "Invalid read-module-buffer arguments\n");
        return 2;
      }

      std::array<std::byte, 1920> bytes {};
      status = run_read_module_buffer(
          client,
          port,
          command_state,
          ModuleBufferReadRequest {
              .start_address = start_address,
              .bytes = static_cast<std::uint16_t>(byte_length),
              .module_number = static_cast<std::uint16_t>(module_number),
          },
          std::span<std::byte>(bytes.data(), static_cast<std::size_t>(byte_length)));
      if (!status.ok()) {
        print_status_error("read-module-buffer request failed", status);
        return 1;
      }
      for (std::uint32_t index = 0; index < byte_length; ++index) {
        std::printf("[%u] %02X\n", index, static_cast<unsigned>(bytes[index]));
      }
      return 0;
    }

    case CommandKind::ReadQualifiedWords: {
      QualifiedBufferWordDevice device {};
      const std::string_view device_arg(options.command_argv[0]);
      const std::string_view points_arg(options.command_argv[1]);
      status = parse_qualified_buffer_word_device(device_arg, device);
      if (!status.ok()) {
        print_status_error("Invalid qualified buffer device", status);
        return 2;
      }

      std::uint32_t points = 0;
      if (!parse_u32(points_arg, points) || points == 0U || points > 960U) {
        std::fprintf(stderr, "Invalid qualified buffer word count: %.*s\n",
                     static_cast<int>(points_arg.size()),
                     points_arg.data());
        return 2;
      }

      ModuleBufferReadRequest request {};
      status = make_qualified_buffer_read_words_request(
          device,
          static_cast<std::uint16_t>(points),
          request);
      if (!status.ok()) {
        print_status_error("Failed to build read-qualified-words request", status);
        return 2;
      }

      std::array<std::byte, 1920> bytes {};
      status = run_read_module_buffer(
          client,
          port,
          command_state,
          request,
          std::span<std::byte>(bytes.data(), static_cast<std::size_t>(points * 2U)));
      if (!status.ok()) {
        print_status_error("read-qualified-words request failed", status);
        return 1;
      }

      std::array<std::uint16_t, 960> words {};
      status = decode_qualified_buffer_word_values(
          std::span<const std::byte>(bytes.data(), static_cast<std::size_t>(points * 2U)),
          std::span<std::uint16_t>(words.data(), points));
      if (!status.ok()) {
        print_status_error("Failed to decode read-qualified-words response", status);
        return 1;
      }

      const auto kind = qualified_buffer_kind_name(device.kind);
      for (std::uint32_t index = 0; index < points; ++index) {
        const auto value = words[index];
        std::printf("U%X\\%.*s%u=0x%04X %u\n",
                    device.module_number,
                    static_cast<int>(kind.size()),
                    kind.data(),
                    static_cast<unsigned>(device.word_address + index),
                    value,
                    value);
      }
      return 0;
    }

    case CommandKind::ReadNativeQualifiedWords: {
      QualifiedBufferWordDevice device {};
      const std::string_view device_arg(options.command_argv[0]);
      const std::string_view points_arg(options.command_argv[1]);
      status = parse_qualified_buffer_word_device(device_arg, device);
      if (!status.ok()) {
        print_status_error("Invalid qualified buffer device", status);
        return 2;
      }

      std::uint32_t points = 0;
      if (!parse_u32(points_arg, points) || points == 0U || points > 960U) {
        std::fprintf(stderr, "Invalid native qualified word count: %.*s\n",
                     static_cast<int>(points_arg.size()),
                     points_arg.data());
        return 2;
      }

      std::array<std::uint16_t, 960> words {};
      status = run_extended_batch_read_words(
          client,
          port,
          command_state,
          device,
          static_cast<std::uint16_t>(points),
          std::span<std::uint16_t>(words.data(), points));
      if (!status.ok()) {
        print_status_error("read-native-qualified-words request failed", status);
        return 1;
      }

      const auto kind = qualified_buffer_kind_name(device.kind);
      for (std::uint32_t index = 0; index < points; ++index) {
        const auto value = words[index];
        std::printf("U%X\\%.*s%u=0x%04X %u\n",
                    device.module_number,
                    static_cast<int>(kind.size()),
                    kind.data(),
                    static_cast<unsigned>(device.word_address + index),
                    value,
                    value);
      }
      return 0;
    }

    case CommandKind::WriteModuleBuffer: {
      const std::string_view start_arg(options.command_argv[0]);
      const std::string_view module_arg(options.command_argv[1]);
      std::uint32_t start_address = 0;
      std::uint32_t module_number = 0;
      if (!parse_u32_auto(start_arg, start_address) || !parse_u32_auto(module_arg, module_number)) {
        std::fprintf(stderr, "Invalid write-module-buffer start/module\n");
        return 2;
      }

      std::array<std::byte, 1920> bytes {};
      const int byte_count = options.command_argc - 2;
      if (byte_count < 2 || byte_count > static_cast<int>(bytes.size())) {
        std::fprintf(stderr, "Invalid write-module-buffer byte count\n");
        return 2;
      }
      for (int index = 0; index < byte_count; ++index) {
        std::uint32_t value = 0;
        if (!parse_u32_auto(options.command_argv[index + 2], value) || value > 0xFFU) {
          std::fprintf(stderr, "Invalid write-module-buffer byte: %s\n", options.command_argv[index + 2]);
          return 2;
        }
        bytes[static_cast<std::size_t>(index)] = static_cast<std::byte>(value);
      }

      status = run_write_module_buffer(
          client,
          port,
          command_state,
          ModuleBufferWriteRequest {
              .start_address = start_address,
              .module_number = static_cast<std::uint16_t>(module_number),
              .bytes = std::span<const std::byte>(bytes.data(), static_cast<std::size_t>(byte_count)),
          });
      if (!status.ok()) {
        print_status_error("write-module-buffer request failed", status);
        return 1;
      }
      std::printf("write-module-buffer=ok\n");
      return 0;
    }

    case CommandKind::WriteQualifiedWords: {
      QualifiedBufferWordDevice device {};
      const std::string_view device_arg(options.command_argv[0]);
      status = parse_qualified_buffer_word_device(device_arg, device);
      if (!status.ok()) {
        print_status_error("Invalid qualified buffer device", status);
        return 2;
      }

      const int word_count = options.command_argc - 1;
      if (word_count < 1 || word_count > 960) {
        std::fprintf(stderr, "Invalid write-qualified-words word count\n");
        return 2;
      }

      std::array<std::uint16_t, 960> words {};
      for (int index = 0; index < word_count; ++index) {
        std::uint32_t value = 0;
        if (!parse_u32_auto(options.command_argv[index + 1], value) || value > 0xFFFFU) {
          std::fprintf(stderr, "Invalid write-qualified-words value: %s\n", options.command_argv[index + 1]);
          return 2;
        }
        words[static_cast<std::size_t>(index)] = static_cast<std::uint16_t>(value);
      }

      std::array<std::byte, 1920> byte_storage {};
      ModuleBufferWriteRequest request {};
      std::size_t byte_count = 0U;
      status = make_qualified_buffer_write_words_request(
          device,
          std::span<const std::uint16_t>(words.data(), static_cast<std::size_t>(word_count)),
          std::span<std::byte>(byte_storage),
          request,
          byte_count);
      if (!status.ok()) {
        print_status_error("Failed to build write-qualified-words request", status);
        return 2;
      }

      status = run_write_module_buffer(client, port, command_state, request);
      if (!status.ok()) {
        print_status_error("write-qualified-words request failed", status);
        return 1;
      }
      std::printf("write-qualified-words=ok bytes=%zu\n", byte_count);
      return 0;
    }

    case CommandKind::WriteNativeQualifiedWords: {
      QualifiedBufferWordDevice device {};
      const std::string_view device_arg(options.command_argv[0]);
      status = parse_qualified_buffer_word_device(device_arg, device);
      if (!status.ok()) {
        print_status_error("Invalid qualified buffer device", status);
        return 2;
      }

      const int word_count = options.command_argc - 1;
      if (word_count < 1 || word_count > 960) {
        std::fprintf(stderr, "Invalid write-native-qualified-words word count\n");
        return 2;
      }

      std::array<std::uint16_t, 960> words {};
      for (int index = 0; index < word_count; ++index) {
        std::uint32_t value = 0;
        if (!parse_u32_auto(options.command_argv[index + 1], value) || value > 0xFFFFU) {
          std::fprintf(stderr, "Invalid write-native-qualified-words value: %s\n", options.command_argv[index + 1]);
          return 2;
        }
        words[static_cast<std::size_t>(index)] = static_cast<std::uint16_t>(value);
      }

      status = run_extended_batch_write_words(
          client,
          port,
          command_state,
          device,
          std::span<const std::uint16_t>(words.data(), static_cast<std::size_t>(word_count)));
      if (!status.ok()) {
        print_status_error("write-native-qualified-words request failed", status);
        return 1;
      }
      std::printf("write-native-qualified-words=ok\n");
      return 0;
    }

    case CommandKind::ReadWords: {
      DeviceAddress device {};
      const std::string_view device_arg(options.command_argv[0]);
      const std::string_view points_arg(options.command_argv[1]);
      if (!parse_device_address(device_arg, device)) {
        std::fprintf(stderr, "Invalid device address: %.*s\n",
                     static_cast<int>(device_arg.size()),
                     device_arg.data());
        return 2;
      }
      std::uint32_t points = 0;
      if (!parse_u32(points_arg, points) || points == 0U || points > 960U) {
        std::fprintf(stderr, "Invalid word count: %.*s\n",
                     static_cast<int>(points_arg.size()),
                     points_arg.data());
        return 2;
      }

      std::array<std::uint16_t, 960> words {};
      status = client.async_batch_read_words(
          now_ms(),
          BatchReadWordsRequest {.head_device = device, .points = static_cast<std::uint16_t>(points)},
          std::span<std::uint16_t>(words.data(), points),
          request_complete,
          &command_state);
      if (!status.ok()) {
        print_status_error("Failed to start read-words request", status);
        return 1;
      }
      status = drive_request(client, port, command_state);
      if (!status.ok()) {
        print_status_error("read-words request failed", status);
        return 1;
      }
      for (std::uint32_t index = 0; index < points; ++index) {
        std::printf("[%u] 0x%04X %u\n", index, words[index], words[index]);
      }
      return 0;
    }

    case CommandKind::ReadBits: {
      DeviceAddress device {};
      const std::string_view device_arg(options.command_argv[0]);
      const std::string_view points_arg(options.command_argv[1]);
      if (!parse_device_address(device_arg, device)) {
        std::fprintf(stderr, "Invalid device address: %.*s\n",
                     static_cast<int>(device_arg.size()),
                     device_arg.data());
        return 2;
      }
      std::uint32_t points = 0;
      if (!parse_u32(points_arg, points) || points == 0U || points > 7904U) {
        std::fprintf(stderr, "Invalid bit count: %.*s\n",
                     static_cast<int>(points_arg.size()),
                     points_arg.data());
        return 2;
      }

      std::array<BitValue, 7904> bits {};
      status = client.async_batch_read_bits(
          now_ms(),
          BatchReadBitsRequest {.head_device = device, .points = static_cast<std::uint16_t>(points)},
          std::span<BitValue>(bits.data(), points),
          request_complete,
          &command_state);
      if (!status.ok()) {
        print_status_error("Failed to start read-bits request", status);
        return 1;
      }
      status = drive_request(client, port, command_state);
      if (!status.ok()) {
        print_status_error("read-bits request failed", status);
        return 1;
      }
      for (std::uint32_t index = 0; index < points; ++index) {
        std::printf("[%u] %u\n", index, bits[index] == BitValue::On ? 1U : 0U);
      }
      return 0;
    }

    case CommandKind::RandomRead: {
      if (options.command_argc > static_cast<int>(mcprotocol::serial::kMaxRandomAccessItems)) {
        std::fprintf(stderr, "Too many random-read items; max is %zu\n", mcprotocol::serial::kMaxRandomAccessItems);
        return 2;
      }

      std::array<RandomReadItem, mcprotocol::serial::kMaxRandomAccessItems> items {};
      for (int index = 0; index < options.command_argc; ++index) {
        DeviceAddress device {};
        const std::string_view device_arg(options.command_argv[index]);
        if (!parse_device_address(device_arg, device)) {
          std::fprintf(stderr, "Invalid random-read item: %s\n", options.command_argv[index]);
          return 2;
        }
        items[static_cast<std::size_t>(index)] = RandomReadItem {
            .device = device,
            .double_word = false,
        };
      }

      std::array<std::uint32_t, mcprotocol::serial::kMaxRandomAccessItems> values {};
      status = client.async_random_read(
          now_ms(),
          RandomReadRequest {
              .items = std::span<const RandomReadItem>(items.data(), static_cast<std::size_t>(options.command_argc)),
          },
          std::span<std::uint32_t>(values.data(), static_cast<std::size_t>(options.command_argc)),
          request_complete,
          &command_state);
      if (!status.ok()) {
        print_status_error("Failed to start random-read request", status);
        return 1;
      }
      status = drive_request(client, port, command_state);
      if (!status.ok()) {
        print_status_error("random-read request failed", status);
        return 1;
      }
      for (int index = 0; index < options.command_argc; ++index) {
        const auto value = values[static_cast<std::size_t>(index)];
        if (is_bit_device(items[static_cast<std::size_t>(index)].device.code)) {
          std::printf("%s=%u\n", options.command_argv[index], value != 0U ? 1U : 0U);
        } else {
          const unsigned word = static_cast<unsigned>(value & 0xFFFFU);
          std::printf("%s=0x%04X %u\n", options.command_argv[index], word, word);
        }
      }
      return 0;
    }

    case CommandKind::RandomWriteWords: {
      if (options.command_argc > static_cast<int>(mcprotocol::serial::kMaxRandomAccessItems)) {
        std::fprintf(stderr,
                     "Too many random-write-words items; max is %zu\n",
                     mcprotocol::serial::kMaxRandomAccessItems);
        return 2;
      }

      std::array<RandomWriteWordItem, mcprotocol::serial::kMaxRandomAccessItems> items {};
      for (int index = 0; index < options.command_argc; ++index) {
        if (!parse_word_write_arg(options.command_argv[index], items[static_cast<std::size_t>(index)])) {
          std::fprintf(stderr, "Invalid random-write-words item: %s\n", options.command_argv[index]);
          return 2;
        }
        if (items[static_cast<std::size_t>(index)].value > 0xFFFFU) {
          std::fprintf(stderr, "random-write-words value must be 0..65535: %s\n", options.command_argv[index]);
          return 2;
        }
      }

      status = run_random_write_words(
          client,
          port,
          command_state,
          std::span<const RandomWriteWordItem>(items.data(), static_cast<std::size_t>(options.command_argc)));
      if (!status.ok()) {
        print_status_error("random-write-words request failed", status);
        return 1;
      }
      std::printf("random-write-words=ok mode=native\n");
      return 0;
    }

    case CommandKind::RandomWriteBits: {
      constexpr std::size_t kMaxRandomWriteBitItems = 188;
      if (options.command_argc > static_cast<int>(kMaxRandomWriteBitItems)) {
        std::fprintf(stderr, "Too many random-write-bits items; max is %zu\n", kMaxRandomWriteBitItems);
        return 2;
      }

      std::array<RandomWriteBitItem, kMaxRandomWriteBitItems> items {};
      for (int index = 0; index < options.command_argc; ++index) {
        if (!parse_bit_write_arg(options.command_argv[index], items[static_cast<std::size_t>(index)])) {
          std::fprintf(stderr, "Invalid random-write-bits item: %s\n", options.command_argv[index]);
          return 2;
        }
      }

      status = run_random_write_bits(
          client,
          port,
          command_state,
          std::span<const RandomWriteBitItem>(items.data(), static_cast<std::size_t>(options.command_argc)));
      if (!status.ok()) {
        print_status_error("random-write-bits request failed", status);
        return 1;
      }
      std::printf("random-write-bits=ok mode=native\n");
      return 0;
    }

    case CommandKind::WriteWords: {
      std::array<std::uint16_t, kCliMaxBatchWordPoints> group_values {};
      const std::size_t max_group_points = cli_max_batch_write_words_points(options.protocol);
      DeviceAddress group_head {};
      DeviceAddress previous_device {};
      bool have_group = false;
      std::size_t group_size = 0;

      const auto flush_group = [&]() -> bool {
        if (!have_group || group_size == 0U) {
          return true;
        }
        status = run_batch_write_words_group(
            client,
            port,
            command_state,
            group_head,
            std::span<const std::uint16_t>(group_values.data(), group_size));
        if (!status.ok()) {
          print_status_error("write-words request failed", status);
          return false;
        }
        have_group = false;
        group_size = 0U;
        return true;
      };

      for (int index = 0; index < options.command_argc; ++index) {
        RandomWriteWordItem item {};
        if (!parse_word_write_arg(options.command_argv[index], item)) {
          std::fprintf(stderr, "Invalid write-words item: %s\n", options.command_argv[index]);
          return 2;
        }
        if (item.value > 0xFFFFU) {
          std::fprintf(stderr, "write-words value must be 0..65535: %s\n", options.command_argv[index]);
          return 2;
        }
        const bool can_append = have_group &&
                                item.device.code == previous_device.code &&
                                item.device.number == (previous_device.number + 1U) &&
                                group_size < max_group_points;
        if (!can_append) {
          if (!flush_group()) {
            return 1;
          }
          group_head = item.device;
          have_group = true;
          group_size = 0U;
        }
        group_values[group_size++] = static_cast<std::uint16_t>(item.value);
        previous_device = item.device;
      }
      if (!flush_group()) {
        return 1;
      }
      std::printf("write-words=ok\n");
      return 0;
    }

    case CommandKind::WriteBits: {
      const std::size_t max_group_points = cli_max_batch_write_bits_points(options.protocol);
      std::array<BitValue, kCliMaxBatchBitPoints> group_values {};
      DeviceAddress group_head {};
      DeviceAddress previous_device {};
      bool have_group = false;
      std::size_t group_size = 0;

      const auto flush_group = [&]() -> bool {
        if (!have_group || group_size == 0U) {
          return true;
        }
        status = run_batch_write_bits_group(
            client,
            port,
            command_state,
            group_head,
            std::span<const BitValue>(group_values.data(), group_size));
        if (!status.ok()) {
          print_status_error("write-bits request failed", status);
          return false;
        }
        have_group = false;
        group_size = 0U;
        return true;
      };

      for (int index = 0; index < options.command_argc; ++index) {
        RandomWriteBitItem item {};
        if (!parse_bit_write_arg(options.command_argv[index], item)) {
          std::fprintf(stderr, "Invalid write-bits item: %s\n", options.command_argv[index]);
          return 2;
        }

        const bool can_append = have_group &&
                                item.device.code == previous_device.code &&
                                item.device.number == (previous_device.number + 1U) &&
                                group_size < max_group_points;
        if (!can_append) {
          if (!flush_group()) {
            return 1;
          }
          group_head = item.device;
          have_group = true;
          group_size = 0U;
        }
        group_values[group_size++] = item.value;
        previous_device = item.device;
      }
      if (!flush_group()) {
        return 1;
      }
      std::printf("write-bits=ok\n");
      return 0;
    }

    case CommandKind::None:
      break;
  }

  return 2;
}
