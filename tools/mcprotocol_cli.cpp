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
#include "mcprotocol/serial/link_direct.hpp"
#include "mcprotocol/serial/posix_serial.hpp"
#include "mcprotocol/serial/qualified_buffer.hpp"

namespace {

bool g_dump_frames = false;

#if defined(_WIN32)
constexpr const char* kDefaultSerialDevicePath = "COM1";
#else
constexpr const char* kDefaultSerialDevicePath = "/dev/ttyUSB0";
#endif

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
using mcprotocol::serial::ExtendedFileRegisterAddress;
using mcprotocol::serial::ExtendedFileRegisterBatchReadWordsRequest;
using mcprotocol::serial::ExtendedFileRegisterBatchWriteWordsRequest;
using mcprotocol::serial::ExtendedFileRegisterDirectBatchReadWordsRequest;
using mcprotocol::serial::ExtendedFileRegisterDirectBatchWriteWordsRequest;
using mcprotocol::serial::ExtendedFileRegisterMonitorRegistration;
using mcprotocol::serial::ExtendedFileRegisterRandomWriteWordItem;
using mcprotocol::serial::FrameKind;
using mcprotocol::serial::GlobalSignalControlRequest;
using mcprotocol::serial::GlobalSignalTarget;
using mcprotocol::serial::HostBufferReadRequest;
using mcprotocol::serial::HostBufferWriteRequest;
using mcprotocol::serial::LinkDirectDevice;
using mcprotocol::serial::LinkDirectMonitorRegistration;
using mcprotocol::serial::LinkDirectMultiBlockReadBlock;
using mcprotocol::serial::LinkDirectMultiBlockReadRequest;
using mcprotocol::serial::LinkDirectMultiBlockWriteBlock;
using mcprotocol::serial::LinkDirectMultiBlockWriteRequest;
using mcprotocol::serial::LinkDirectRandomReadItem;
using mcprotocol::serial::LinkDirectRandomWriteBitItem;
using mcprotocol::serial::LinkDirectRandomWriteWordItem;
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
using mcprotocol::serial::RemoteOperationMode;
using mcprotocol::serial::RemoteRunClearMode;
using mcprotocol::serial::RouteKind;
using mcprotocol::serial::Rs485Hooks;
using mcprotocol::serial::Status;
using mcprotocol::serial::StatusCode;
using mcprotocol::serial::UserFrameDeleteRequest;
using mcprotocol::serial::UserFrameReadRequest;
using mcprotocol::serial::UserFrameRegistrationData;
using mcprotocol::serial::UserFrameWriteRequest;
using mcprotocol::serial::decode_qualified_buffer_word_values;
using mcprotocol::serial::make_qualified_buffer_read_words_request;
using mcprotocol::serial::make_qualified_buffer_write_words_request;
using mcprotocol::serial::parse_qualified_buffer_word_device;
using mcprotocol::serial::parse_link_direct_device;
using mcprotocol::serial::qualified_buffer_kind_name;

enum class CommandKind : std::uint8_t {
  None,
  CpuModel,
  RemoteRun,
  RemoteStop,
  RemotePause,
  RemoteLatchClear,
  UnlockRemotePassword,
  LockRemotePassword,
  ClearError,
  RemoteReset,
  GlobalSignal,
  InitializeTransmissionSequence,
  DeregisterCpuMonitoring,
  RecoverC24,
  Loopback,
  ReadUserFrame,
  WriteUserFrame,
  DeleteUserFrame,
  ReadWords,
  ReadFileRegisterWords,
  ReadFileRegisterWordsDirect,
  ReadBits,
  ReadLinkDirectWords,
  ReadLinkDirectBits,
  WriteLinkDirectWords,
  WriteLinkDirectBits,
  RandomReadLinkDirect,
  RandomWriteLinkDirectWords,
  RandomWriteLinkDirectBits,
  MultiBlockReadLinkDirectWords,
  MultiBlockReadLinkDirectBits,
  MultiBlockWriteLinkDirectWords,
  MultiBlockWriteLinkDirectBits,
  MonitorLinkDirect,
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
  RandomWriteFileRegisterWords,
  RandomWriteBits,
  WriteWords,
  WriteFileRegisterWords,
  WriteFileRegisterWordsDirect,
  WriteBits,
  MonitorFileRegister,
  ProbeAll,
  ProbeWriteAll,
  ProbeRandomRead,
  ProbeRandomWriteWords,
  ProbeRandomWriteBits,
  ProbeMultiBlock,
  ProbeMonitor,
  ProbeHostBuffer,
  ProbeModuleBuffer,
  ProbeWriteHostBuffer,
  ProbeWriteModuleBuffer
};

enum class ProbeMultiBlockMode : std::uint8_t {
  Mixed,
  WordOnly,
  BitOnly,
  WordA,
  WordB,
  BitA,
  BitB,
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

constexpr std::array<DeviceParseSpec, 39> kDeviceParseSpecs {{
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
    {"SM", DeviceCode::SM, 10},
    {"SD", DeviceCode::SD, 10},
    {"DX", DeviceCode::DX, 16},
    {"DY", DeviceCode::DY, 16},
    {"LTS", DeviceCode::LTS, 10},
    {"LTC", DeviceCode::LTC, 10},
    {"LTN", DeviceCode::LTN, 10},
    {"LSTS", DeviceCode::LSTS, 10},
    {"LSTC", DeviceCode::LSTC, 10},
    {"LSTN", DeviceCode::LSTN, 10},
    {"LCS", DeviceCode::LCS, 10},
    {"LCC", DeviceCode::LCC, 10},
    {"LCN", DeviceCode::LCN, 10},
    {"LZ", DeviceCode::LZ, 10},
    {"RD", DeviceCode::RD, 10},
    {"ZR", DeviceCode::ZR, 10},
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
constexpr std::size_t kCliMaxExtendedFileRegisterWordPoints = 256U;
constexpr std::size_t kCliMaxExtendedFileRegisterRandomWriteItems = 40U;

[[nodiscard]] std::uint32_t now_ms() {
  const auto now = std::chrono::steady_clock::now().time_since_epoch();
  return static_cast<std::uint32_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(now).count());
}

[[nodiscard]] bool equals_ignore_case(std::string_view lhs, std::string_view rhs) {
  if (lhs.size() != rhs.size()) {
    return false;
  }
  for (std::size_t index = 0; index < lhs.size(); ++index) {
    const auto lhs_ch = static_cast<unsigned char>(lhs[index]);
    const auto rhs_ch = static_cast<unsigned char>(rhs[index]);
    if (std::tolower(lhs_ch) != std::tolower(rhs_ch)) {
      return false;
    }
  }
  return true;
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

[[nodiscard]] constexpr bool cli_is_e1_frame(const ProtocolConfig& config) {
  return config.frame_kind == FrameKind::E1;
}

[[nodiscard]] constexpr std::size_t cli_max_extended_file_register_word_points(const ProtocolConfig& config) {
  return cli_is_e1_frame(config) ? 256U : 64U;
}

[[nodiscard]] constexpr std::size_t cli_max_extended_file_register_random_write_items(
    const ProtocolConfig& config) {
  return cli_is_e1_frame(config) ? 40U : 10U;
}

[[nodiscard]] constexpr std::uint32_t cli_max_extended_file_register_block_number(
    const ProtocolConfig& config) {
  return cli_is_e1_frame(config) ? 0xFFFFU : 999U;
}

[[nodiscard]] constexpr std::uint32_t cli_max_extended_file_register_word_number(
    const ProtocolConfig& config) {
  return cli_is_e1_frame(config) ? 0xFFFFU : 8191U;
}

[[nodiscard]] constexpr std::uint32_t cli_max_direct_extended_file_register_head_device_number(
    const ProtocolConfig& config) {
  return cli_is_e1_frame(config) ? 0xFFFFFFFFU : 9999999U;
}

void print_usage() {
  std::fprintf(
      stderr,
      "Usage:\n"
      "\n"
      "  Basic:\n"
      "  mcprotocol_cli [options] cpu-model\n"
      "  mcprotocol_cli [options] read-words DEVICE POINTS\n"
      "  mcprotocol_cli [options] read-bits DEVICE POINTS\n"
      "  mcprotocol_cli [options] write-words DEVICE=VALUE [DEVICE=VALUE ...]\n"
      "  mcprotocol_cli [options] write-bits DEVICE=0|1 [DEVICE=0|1 ...]\n"
      "  mcprotocol_cli [options] loopback HEXASCII\n"
      "\n"
      "  Control and Recovery:\n"
      "  mcprotocol_cli [options] remote-run [no-force|force] [no-clear|outside-latch|all-clear]\n"
      "  mcprotocol_cli [options] remote-stop\n"
      "  mcprotocol_cli [options] remote-pause [no-force|force]\n"
      "  mcprotocol_cli [options] latch-clear\n"
      "  mcprotocol_cli [options] unlock PASSWORD\n"
      "  mcprotocol_cli [options] lock PASSWORD\n"
      "  mcprotocol_cli [options] error-clear\n"
      "  mcprotocol_cli [options] remote-reset\n"
      "  mcprotocol_cli [options] global-signal on|off current|x1a|x1b [STATION]\n"
      "  mcprotocol_cli [options] init-sequence\n"
      "  mcprotocol_cli [options] deregister-cpu-monitor\n"
      "  mcprotocol_cli [options] recover-c24 [eot|cl]\n"
      "\n"
      "  User Frame and File Register:\n"
      "  mcprotocol_cli [options] read-user-frame FRAME_NO\n"
      "  mcprotocol_cli [options] register-user-frame FRAME_NO FRAME_BYTES HEXBYTES\n"
      "  mcprotocol_cli [options] delete-user-frame FRAME_NO\n"
      "  mcprotocol_cli [options] read-file-register BLOCK:RDEVICE POINTS\n"
      "  mcprotocol_cli [options] read-file-register-direct DEVICE_NO POINTS\n"
      "  mcprotocol_cli [options] random-write-file-register BLOCK:RDEVICE=VALUE [BLOCK:RDEVICE=VALUE ...]\n"
      "  mcprotocol_cli [options] write-file-register BLOCK:RDEVICE VALUE [VALUE ...]\n"
      "  mcprotocol_cli [options] write-file-register-direct DEVICE_NO VALUE [VALUE ...]\n"
      "  mcprotocol_cli [options] monitor-file-register BLOCK:RDEVICE [BLOCK:RDEVICE ...]\n"
      "\n"
      "  Link-Direct (Jn\\\\...):\n"
      "  mcprotocol_cli [options] read-link-direct-words J1\\\\W100 POINTS\n"
      "  mcprotocol_cli [options] read-link-direct-bits J1\\\\X10 POINTS\n"
      "  mcprotocol_cli [options] write-link-direct-words J1\\\\W100 VALUE [VALUE ...]\n"
      "  mcprotocol_cli [options] write-link-direct-bits J1\\\\B0 0|1 [0|1 ...]\n"
      "  mcprotocol_cli [options] random-read-link-direct J1\\\\W100 [J1\\\\B10 ...]\n"
      "  mcprotocol_cli [options] random-write-link-direct-words J1\\\\W100=VALUE [J1\\\\SW0=VALUE ...]\n"
      "  mcprotocol_cli [options] random-write-link-direct-bits J1\\\\B10=0|1 [J1\\\\SB10=0|1 ...]\n"
      "  mcprotocol_cli [options] multi-block-read-link-direct-words J1\\\\W100:POINTS [J1\\\\SW0:POINTS ...]\n"
      "  mcprotocol_cli [options] multi-block-read-link-direct-bits J1\\\\B10:POINTS [J1\\\\X10:POINTS ...]\n"
      "  mcprotocol_cli [options] multi-block-write-link-direct-words J1\\\\W100=V0,V1 [J1\\\\SW0=V0 ...]\n"
      "  mcprotocol_cli [options] multi-block-write-link-direct-bits J1\\\\B10=0101... [J1\\\\SB10=0011...]\n"
      "  mcprotocol_cli [options] monitor-link-direct J1\\\\W100 [J1\\\\B10 ...]\n"
      "\n"
      "  Buffer and Qualified:\n"
      "  mcprotocol_cli [options] read-host-buffer START WORDS\n"
      "  mcprotocol_cli [options] read-module-buffer START BYTES MODULE\n"
      "  mcprotocol_cli [options] read-qualified-words U3E0\\\\G0 POINTS\n"
      "  mcprotocol_cli [options] read-native-qualified-words U3E0\\\\G0 POINTS\n"
      "  mcprotocol_cli [options] write-host-buffer START VALUE [VALUE ...]\n"
      "  mcprotocol_cli [options] write-module-buffer START MODULE BYTE [BYTE ...]\n"
      "  mcprotocol_cli [options] write-qualified-words U3E0\\\\G0 VALUE [VALUE ...]\n"
      "  mcprotocol_cli [options] write-native-qualified-words U3E0\\\\G0 VALUE [VALUE ...]\n"
      "\n"
      "  Native Random and Probe:\n"
      "  mcprotocol_cli [options] random-read DEVICE [DEVICE ...]\n"
      "  mcprotocol_cli [options] random-write-words DEVICE=VALUE [DEVICE=VALUE ...]\n"
      "  mcprotocol_cli [options] random-write-bits DEVICE=0|1 [DEVICE=0|1 ...]\n"
      "  mcprotocol_cli [options] probe-all\n"
      "  mcprotocol_cli [options] probe-write-all\n"
      "  mcprotocol_cli [options] probe-random-read\n"
      "  mcprotocol_cli [options] probe-random-write-words\n"
      "  mcprotocol_cli [options] probe-random-write-bits\n"
      "  mcprotocol_cli [options] probe-multi-block [mixed|word-only|bit-only|word-a|word-b|bit-a|bit-b]\n"
      "  mcprotocol_cli [options] probe-monitor [read-only]\n"
      "  mcprotocol_cli [options] probe-host-buffer\n"
      "  mcprotocol_cli [options] probe-module-buffer\n"
      "  mcprotocol_cli [options] probe-write-host-buffer\n"
      "  mcprotocol_cli [options] probe-write-module-buffer\n"
      "\n"
      "Options:\n"
#if defined(_WIN32)
      "  --device PATH               Serial device path (default: COM1)\n"
#else
      "  --device PATH               Serial device path (default: /dev/ttyUSB0)\n"
#endif
      "  --baud RATE                Baud rate (default: 9600)\n"
      "  --data-bits N              Data bits: 5/6/7/8 (default: 8)\n"
      "  --stop-bits N              Stop bits: 1/2 (default: 1)\n"
      "  --parity N|E|O             Parity (default: N)\n"
      "  --rts-cts on|off           Hardware flow control (default: off)\n"
      "  --rts-toggle on|off        Toggle RTS during TX for RS-485 DE control\n"
      "  --dump-frames on|off       Print raw TX/RX frame bytes to stderr (default: off)\n"
      "  --frame MODE               c4-binary | c4-ascii-f1 | c4-ascii-f2 | c4-ascii-f3 | c4-ascii-f4 | c3-ascii-f1 | c3-ascii-f2 | c3-ascii-f3 | c3-ascii-f4 | c2-ascii-f1 | c2-ascii-f2 | c2-ascii-f3 | c2-ascii-f4 | c1-ascii-f1 | c1-ascii-f3 | c1-ascii-f4 | e1-binary | e1-ascii\n"
      "  --series ql|iqr|qna|a      Target PLC family for device encoding (default: ql)\n"
      "  --block-no N               ASCII Format2 block number 0..255 (default: 0)\n"
      "  --station N                Station number; non-zero implies multidrop\n"
      "  --self-station N           Self-station number for m:n connections\n"
      "  --sum-check on|off         Enable or disable sum-check (default: on)\n"
      "  --response-timeout-ms N    Response timeout in milliseconds (default: 5000)\n"
      "  --inter-byte-timeout-ms N  Inter-byte timeout in milliseconds (default: 250)\n"
      "\n"
      "Notes:\n"
      "  remote-run defaults to no-force + no-clear. Use force or all-clear only when you intend that effect.\n"
      "  remote-pause defaults to no-force. remote-stop and latch-clear change PLC state.\n"
      "  unlock/lock send the configured remote password as ASCII bytes; non-iQ-R targets use exactly 4 chars, iQ-R uses 6..32.\n"
      "  error-clear sends C24 clear-error-information (1617); it is not the E71 COM.ERR-only variant.\n"
      "  remote-reset may complete without a response; this CLI treats a pure response-timeout after TX as success.\n"
      "  global-signal maps to C24 command 1618 on 2C/3C/4C; STATION defaults to 0.\n"
      "  init-sequence maps to 1615 and is binary 4C format-5 only; some targets complete without replying.\n"
      "  deregister-cpu-monitor maps to 0631 on 2C/3C/4C and stops chapter-13 CPU monitoring.\n"
      "  recover-c24 sends ASCII EOT CRLF by default; pass cl to send CL CRLF.\n"
      "  Use recover-c24 after timeout or mixed-response states on C24 ASCII links; no reply is expected.\n"
      "  read-qualified-words / write-qualified-words use the practical 0601/1601 helper path.\n"
      "  read-native-qualified-words / write-native-qualified-words are unsupported diagnostic probes, not a supported workflow.\n"
      "  link-direct commands use binary-only device extension specification for Jn\\\\X/Y/B/W/SB/SW.\n"
      "  c1-ascii-* targets --series a or --series qna. File-register commands also map onto e1-* where chapter-18 supports them.\n"
      "  loopback maps to TT on c1-ascii-* and 0619 on 2C/3C/4C.\n"
      "  read/register/delete-user-frame map to 0610/1610 on 2C/3C/4C only; HEXBYTES is raw registration data in hexadecimal.\n"
      "  e1-* targets --series a or --series qna. E1 exposes chapter-18 device-memory, extended-file-register, and special-function-module commands only.\n"
      "  read/write/random-write/monitor-file-register use BLOCK:RDEVICE. On c1-ascii-* this is ER/EW/ET/EM/ME; on e1-* it is the chapter-18 block-addressed path.\n"
      "  read/write-file-register-direct use a direct R-device number. On c1-ascii-* this is QnA-common NR/NW; on e1-* it is the chapter-18 direct path.\n"
      "  multi-block-read-link-direct-bits uses POINTS in 16-bit units.\n"
      "  multi-block-write-link-direct-bits expects a 0/1 bit string whose length is a multiple of 16.\n"
      "  probe-multi-block defaults to mixed; pass word-only/bit-only or a single block mode to isolate 1406 verification.\n"
      "  probe-monitor read-only sends raw 0802 without client-side monitor registration state.\n"
      "  random-read / monitor bit items print the requested point plus raw=0x.... for the native 16-point mask.\n"
      "  random-read / random-write-* / probe-random-* / probe-multi-block / probe-monitor expose native probe results directly.\n");
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

[[nodiscard]] bool parse_hex_byte_string(
    std::string_view text,
    std::span<std::byte> out_bytes,
    std::size_t& out_size) {
  out_size = 0U;
  if (text.empty() || (text.size() % 2U) != 0U) {
    return false;
  }
  const std::size_t byte_count = text.size() / 2U;
  if (byte_count > out_bytes.size()) {
    return false;
  }
  const auto nibble = [](char ch) noexcept -> int {
    if (ch >= '0' && ch <= '9') {
      return ch - '0';
    }
    if (ch >= 'A' && ch <= 'F') {
      return 10 + (ch - 'A');
    }
    if (ch >= 'a' && ch <= 'f') {
      return 10 + (ch - 'a');
    }
    return -1;
  };
  for (std::size_t index = 0; index < byte_count; ++index) {
    const int upper = nibble(text[index * 2U]);
    const int lower = nibble(text[index * 2U + 1U]);
    if (upper < 0 || lower < 0) {
      return false;
    }
    out_bytes[index] = std::byte {static_cast<std::uint8_t>((upper << 4U) | lower)};
  }
  out_size = byte_count;
  return true;
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

[[nodiscard]] bool parse_global_signal_target(
    std::string_view text,
    GlobalSignalTarget& out_target) {
  if (text == "current" || text == "received" || text == "local") {
    out_target = GlobalSignalTarget::ReceivedSide;
    return true;
  }
  if (text == "x1a") {
    out_target = GlobalSignalTarget::X1A;
    return true;
  }
  if (text == "x1b") {
    out_target = GlobalSignalTarget::X1B;
    return true;
  }
  return false;
}

[[nodiscard]] const char* global_signal_target_name(GlobalSignalTarget target) {
  switch (target) {
    case GlobalSignalTarget::ReceivedSide:
      return "current";
    case GlobalSignalTarget::X1A:
      return "x1a";
    case GlobalSignalTarget::X1B:
      return "x1b";
  }
  return "unknown";
}

[[nodiscard]] bool parse_remote_operation_mode(
    std::string_view text,
    RemoteOperationMode& out_mode) {
  if (text == "no-force" || text == "normal" || text == "safe" || text == "0001" || text == "1") {
    out_mode = RemoteOperationMode::DoNotExecuteForcibly;
    return true;
  }
  if (text == "force" || text == "0003" || text == "3") {
    out_mode = RemoteOperationMode::ExecuteForcibly;
    return true;
  }
  return false;
}

[[nodiscard]] bool parse_remote_run_clear_mode(
    std::string_view text,
    RemoteRunClearMode& out_clear_mode) {
  if (text == "no-clear" || text == "none" || text == "00" || text == "0") {
    out_clear_mode = RemoteRunClearMode::DoNotClear;
    return true;
  }
  if (text == "outside-latch" || text == "outside" || text == "01" || text == "1") {
    out_clear_mode = RemoteRunClearMode::ClearOutsideLatchRange;
    return true;
  }
  if (text == "all-clear" || text == "all" || text == "02" || text == "2") {
    out_clear_mode = RemoteRunClearMode::AllClear;
    return true;
  }
  return false;
}

[[nodiscard]] bool parse_probe_multi_block_mode(std::string_view text, ProbeMultiBlockMode& out_mode) {
  if (text.empty() || text == "mixed") {
    out_mode = ProbeMultiBlockMode::Mixed;
    return true;
  }
  if (text == "word-only") {
    out_mode = ProbeMultiBlockMode::WordOnly;
    return true;
  }
  if (text == "bit-only") {
    out_mode = ProbeMultiBlockMode::BitOnly;
    return true;
  }
  if (text == "word-a") {
    out_mode = ProbeMultiBlockMode::WordA;
    return true;
  }
  if (text == "word-b") {
    out_mode = ProbeMultiBlockMode::WordB;
    return true;
  }
  if (text == "bit-a") {
    out_mode = ProbeMultiBlockMode::BitA;
    return true;
  }
  if (text == "bit-b") {
    out_mode = ProbeMultiBlockMode::BitB;
    return true;
  }
  return false;
}

[[nodiscard]] constexpr std::string_view probe_multi_block_mode_name(ProbeMultiBlockMode mode) {
  switch (mode) {
    case ProbeMultiBlockMode::Mixed:
      return "mixed";
    case ProbeMultiBlockMode::WordOnly:
      return "word-only";
    case ProbeMultiBlockMode::BitOnly:
      return "bit-only";
    case ProbeMultiBlockMode::WordA:
      return "word-a";
    case ProbeMultiBlockMode::WordB:
      return "word-b";
    case ProbeMultiBlockMode::BitA:
      return "bit-a";
    case ProbeMultiBlockMode::BitB:
      return "bit-b";
  }
  return "mixed";
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
  if (text == "c4-ascii-f2") {
    config.frame_kind = FrameKind::C4;
    config.code_mode = CodeMode::Ascii;
    config.ascii_format = AsciiFormat::Format2;
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
  if (text == "c3-ascii-f2") {
    config.frame_kind = FrameKind::C3;
    config.code_mode = CodeMode::Ascii;
    config.ascii_format = AsciiFormat::Format2;
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
  if (text == "c2-ascii-f1") {
    config.frame_kind = FrameKind::C2;
    config.code_mode = CodeMode::Ascii;
    config.ascii_format = AsciiFormat::Format1;
    return true;
  }
  if (text == "c2-ascii-f2") {
    config.frame_kind = FrameKind::C2;
    config.code_mode = CodeMode::Ascii;
    config.ascii_format = AsciiFormat::Format2;
    return true;
  }
  if (text == "c2-ascii-f3") {
    config.frame_kind = FrameKind::C2;
    config.code_mode = CodeMode::Ascii;
    config.ascii_format = AsciiFormat::Format3;
    return true;
  }
  if (text == "c2-ascii-f4") {
    config.frame_kind = FrameKind::C2;
    config.code_mode = CodeMode::Ascii;
    config.ascii_format = AsciiFormat::Format4;
    return true;
  }
  if (text == "c1-ascii-f1") {
    config.frame_kind = FrameKind::C1;
    config.code_mode = CodeMode::Ascii;
    config.ascii_format = AsciiFormat::Format1;
    return true;
  }
  if (text == "c1-ascii-f3") {
    config.frame_kind = FrameKind::C1;
    config.code_mode = CodeMode::Ascii;
    config.ascii_format = AsciiFormat::Format3;
    return true;
  }
  if (text == "c1-ascii-f4") {
    config.frame_kind = FrameKind::C1;
    config.code_mode = CodeMode::Ascii;
    config.ascii_format = AsciiFormat::Format4;
    return true;
  }
  if (text == "e1-binary") {
    config.frame_kind = FrameKind::E1;
    config.code_mode = CodeMode::Binary;
    config.ascii_format = AsciiFormat::Format3;
    return true;
  }
  if (text == "e1-ascii") {
    config.frame_kind = FrameKind::E1;
    config.code_mode = CodeMode::Ascii;
    config.ascii_format = AsciiFormat::Format1;
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
  if (text == "qna") {
    config.target_series = PlcSeries::QnA;
    return true;
  }
  if (text == "a") {
    config.target_series = PlcSeries::A;
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

[[nodiscard]] bool split_once(
    std::string_view text,
    char delimiter,
    std::string_view& left,
    std::string_view& right) {
  const std::size_t position = text.find(delimiter);
  if (position == std::string_view::npos || position == 0U || position == (text.size() - 1U)) {
    return false;
  }
  left = text.substr(0U, position);
  right = text.substr(position + 1U);
  return true;
}

[[nodiscard]] bool parse_link_direct_random_read_item(
    std::string_view text,
    LinkDirectRandomReadItem& out_item) {
  LinkDirectDevice device {};
  const Status status = parse_link_direct_device(text, device);
  if (!status.ok()) {
    return false;
  }
  out_item = LinkDirectRandomReadItem {
      .device = device,
      .double_word = false,
  };
  return true;
}

[[nodiscard]] bool parse_link_direct_random_write_word_item(
    std::string_view text,
    LinkDirectRandomWriteWordItem& out_item) {
  std::string_view device_text;
  std::string_view value_text;
  if (!split_once(text, '=', device_text, value_text)) {
    return false;
  }

  LinkDirectDevice device {};
  const Status status = parse_link_direct_device(device_text, device);
  if (!status.ok()) {
    return false;
  }

  std::uint32_t value = 0;
  if (!parse_u32_auto(value_text, value) || value > 0xFFFFU) {
    return false;
  }

  out_item = LinkDirectRandomWriteWordItem {
      .device = device,
      .value = value,
      .double_word = false,
  };
  return true;
}

[[nodiscard]] bool parse_link_direct_random_write_bit_item(
    std::string_view text,
    LinkDirectRandomWriteBitItem& out_item) {
  std::string_view device_text;
  std::string_view value_text;
  if (!split_once(text, '=', device_text, value_text)) {
    return false;
  }

  LinkDirectDevice device {};
  const Status status = parse_link_direct_device(device_text, device);
  if (!status.ok()) {
    return false;
  }

  BitValue value {};
  if (value_text == "0") {
    value = BitValue::Off;
  } else if (value_text == "1") {
    value = BitValue::On;
  } else {
    return false;
  }

  out_item = LinkDirectRandomWriteBitItem {
      .device = device,
      .value = value,
  };
  return true;
}

[[nodiscard]] bool parse_link_direct_multi_block_read_spec(
    std::string_view text,
    bool bit_block,
    LinkDirectMultiBlockReadBlock& out_block) {
  std::string_view device_text;
  std::string_view points_text;
  if (!split_once(text, ':', device_text, points_text)) {
    return false;
  }

  LinkDirectDevice device {};
  const Status status = parse_link_direct_device(device_text, device);
  if (!status.ok()) {
    return false;
  }

  std::uint32_t points = 0;
  if (!parse_u32(points_text, points) || points == 0U || points > 960U) {
    return false;
  }

  out_block = LinkDirectMultiBlockReadBlock {
      .head_device = device,
      .points = static_cast<std::uint16_t>(points),
      .bit_block = bit_block,
  };
  return true;
}

[[nodiscard]] bool parse_csv_u16_values(
    std::string_view text,
    std::span<std::uint16_t> storage,
    std::size_t& out_count) {
  out_count = 0U;
  while (!text.empty()) {
    const std::size_t comma = text.find(',');
    const std::string_view token = comma == std::string_view::npos ? text : text.substr(0U, comma);
    if (token.empty() || out_count >= storage.size()) {
      return false;
    }
    std::uint32_t value = 0;
    if (!parse_u32_auto(token, value) || value > 0xFFFFU) {
      return false;
    }
    storage[out_count++] = static_cast<std::uint16_t>(value);
    if (comma == std::string_view::npos) {
      break;
    }
    text.remove_prefix(comma + 1U);
  }
  return out_count > 0U;
}

[[nodiscard]] bool parse_link_direct_multi_block_write_word_spec(
    std::string_view text,
    std::span<std::uint16_t> storage,
    std::size_t& out_count,
    LinkDirectDevice& out_device) {
  std::string_view device_text;
  std::string_view values_text;
  if (!split_once(text, '=', device_text, values_text)) {
    return false;
  }
  const Status status = parse_link_direct_device(device_text, out_device);
  if (!status.ok()) {
    return false;
  }
  return parse_csv_u16_values(values_text, storage, out_count);
}

[[nodiscard]] bool parse_link_direct_bit_string(
    std::string_view text,
    std::span<BitValue> storage,
    std::size_t& out_count) {
  if (text.empty() || (text.size() % 16U) != 0U || text.size() > storage.size()) {
    return false;
  }
  for (std::size_t index = 0; index < text.size(); ++index) {
    if (text[index] == '0') {
      storage[index] = BitValue::Off;
    } else if (text[index] == '1') {
      storage[index] = BitValue::On;
    } else {
      return false;
    }
  }
  out_count = text.size();
  return true;
}

[[nodiscard]] bool parse_link_direct_multi_block_write_bit_spec(
    std::string_view text,
    std::span<BitValue> storage,
    std::size_t& out_count,
    LinkDirectDevice& out_device) {
  std::string_view device_text;
  std::string_view bits_text;
  if (!split_once(text, '=', device_text, bits_text)) {
    return false;
  }
  const Status status = parse_link_direct_device(device_text, out_device);
  if (!status.ok()) {
    return false;
  }
  return parse_link_direct_bit_string(bits_text, storage, out_count);
}

[[nodiscard]] bool parse_args(int argc, char** argv, CliOptions& options) {
  options.serial.device_path = kDefaultSerialDevicePath;
  options.protocol.frame_kind = FrameKind::C4;
  options.protocol.code_mode = CodeMode::Binary;
  options.protocol.ascii_format = AsciiFormat::Format3;
  options.protocol.ascii_block_number = 0x00;
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
    } else if (arg == "--block-no" && (index + 1) < argc) {
      std::uint32_t value = 0;
      if (!parse_u32_auto(argv[++index], value) || value > 0xFFU) {
        return false;
      }
      options.protocol.ascii_block_number = static_cast<std::uint8_t>(value);
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
      } else if (arg == "remote-run" || arg == "run") {
        options.command = CommandKind::RemoteRun;
      } else if (arg == "remote-stop" || arg == "stop") {
        options.command = CommandKind::RemoteStop;
      } else if (arg == "remote-pause" || arg == "pause") {
        options.command = CommandKind::RemotePause;
      } else if (arg == "latch-clear" || arg == "remote-latch-clear") {
        options.command = CommandKind::RemoteLatchClear;
      } else if (arg == "unlock" || arg == "password-unlock") {
        options.command = CommandKind::UnlockRemotePassword;
      } else if (arg == "lock" || arg == "password-lock") {
        options.command = CommandKind::LockRemotePassword;
      } else if (arg == "error-clear" || arg == "clear-error") {
        options.command = CommandKind::ClearError;
      } else if (arg == "remote-reset" || arg == "reset") {
        options.command = CommandKind::RemoteReset;
      } else if (arg == "global-signal") {
        options.command = CommandKind::GlobalSignal;
      } else if (arg == "init-sequence" || arg == "initialize-sequence") {
        options.command = CommandKind::InitializeTransmissionSequence;
      } else if (arg == "deregister-cpu-monitor" || arg == "cpu-monitor-deregister") {
        options.command = CommandKind::DeregisterCpuMonitoring;
      } else if (arg == "recover-c24") {
        options.command = CommandKind::RecoverC24;
      } else if (arg == "loopback") {
        options.command = CommandKind::Loopback;
      } else if (arg == "read-user-frame") {
        options.command = CommandKind::ReadUserFrame;
      } else if (arg == "register-user-frame") {
        options.command = CommandKind::WriteUserFrame;
      } else if (arg == "delete-user-frame") {
        options.command = CommandKind::DeleteUserFrame;
      } else if (arg == "read-words") {
        options.command = CommandKind::ReadWords;
      } else if (arg == "read-file-register") {
        options.command = CommandKind::ReadFileRegisterWords;
      } else if (arg == "read-file-register-direct") {
        options.command = CommandKind::ReadFileRegisterWordsDirect;
      } else if (arg == "read-bits") {
        options.command = CommandKind::ReadBits;
      } else if (arg == "read-link-direct-words") {
        options.command = CommandKind::ReadLinkDirectWords;
      } else if (arg == "read-link-direct-bits") {
        options.command = CommandKind::ReadLinkDirectBits;
      } else if (arg == "write-link-direct-words") {
        options.command = CommandKind::WriteLinkDirectWords;
      } else if (arg == "write-link-direct-bits") {
        options.command = CommandKind::WriteLinkDirectBits;
      } else if (arg == "random-read-link-direct") {
        options.command = CommandKind::RandomReadLinkDirect;
      } else if (arg == "random-write-link-direct-words") {
        options.command = CommandKind::RandomWriteLinkDirectWords;
      } else if (arg == "random-write-link-direct-bits") {
        options.command = CommandKind::RandomWriteLinkDirectBits;
      } else if (arg == "multi-block-read-link-direct-words") {
        options.command = CommandKind::MultiBlockReadLinkDirectWords;
      } else if (arg == "multi-block-read-link-direct-bits") {
        options.command = CommandKind::MultiBlockReadLinkDirectBits;
      } else if (arg == "multi-block-write-link-direct-words") {
        options.command = CommandKind::MultiBlockWriteLinkDirectWords;
      } else if (arg == "multi-block-write-link-direct-bits") {
        options.command = CommandKind::MultiBlockWriteLinkDirectBits;
      } else if (arg == "monitor-link-direct") {
        options.command = CommandKind::MonitorLinkDirect;
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
      } else if (arg == "random-write-file-register") {
        options.command = CommandKind::RandomWriteFileRegisterWords;
      } else if (arg == "random-write-bits") {
        options.command = CommandKind::RandomWriteBits;
      } else if (arg == "write-words") {
        options.command = CommandKind::WriteWords;
      } else if (arg == "write-file-register") {
        options.command = CommandKind::WriteFileRegisterWords;
      } else if (arg == "write-file-register-direct") {
        options.command = CommandKind::WriteFileRegisterWordsDirect;
      } else if (arg == "write-bits") {
        options.command = CommandKind::WriteBits;
      } else if (arg == "monitor-file-register") {
        options.command = CommandKind::MonitorFileRegister;
      } else if (arg == "probe-all") {
        options.command = CommandKind::ProbeAll;
      } else if (arg == "probe-write-all") {
        options.command = CommandKind::ProbeWriteAll;
      } else if (arg == "probe-random-read") {
        options.command = CommandKind::ProbeRandomRead;
      } else if (arg == "probe-random-write-words") {
        options.command = CommandKind::ProbeRandomWriteWords;
      } else if (arg == "probe-random-write-bits") {
        options.command = CommandKind::ProbeRandomWriteBits;
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
    case CommandKind::RemoteStop:
    case CommandKind::RemoteLatchClear:
    case CommandKind::ClearError:
    case CommandKind::RemoteReset:
    case CommandKind::InitializeTransmissionSequence:
    case CommandKind::DeregisterCpuMonitoring:
    case CommandKind::ProbeAll:
    case CommandKind::ProbeWriteAll:
    case CommandKind::ProbeRandomRead:
    case CommandKind::ProbeRandomWriteWords:
    case CommandKind::ProbeRandomWriteBits:
    case CommandKind::ProbeMultiBlock:
    case CommandKind::ProbeMonitor:
    case CommandKind::ProbeHostBuffer:
    case CommandKind::ProbeModuleBuffer:
    case CommandKind::ProbeWriteHostBuffer:
    case CommandKind::ProbeWriteModuleBuffer:
      return options.command_argc <= 1;
    case CommandKind::RemoteRun:
      return options.command_argc <= 2;
    case CommandKind::RemotePause:
      return options.command_argc <= 1;
    case CommandKind::GlobalSignal:
      return options.command_argc == 2 || options.command_argc == 3;
    case CommandKind::UnlockRemotePassword:
    case CommandKind::LockRemotePassword:
      return options.command_argc == 1;
    case CommandKind::RecoverC24:
      return options.command_argc <= 1;
    case CommandKind::Loopback:
      return options.command_argc == 1;
    case CommandKind::ReadUserFrame:
    case CommandKind::DeleteUserFrame:
      return options.command_argc == 1;
    case CommandKind::WriteUserFrame:
      return options.command_argc == 3;
    case CommandKind::ReadWords:
    case CommandKind::ReadFileRegisterWords:
    case CommandKind::ReadFileRegisterWordsDirect:
    case CommandKind::ReadBits:
    case CommandKind::ReadLinkDirectWords:
    case CommandKind::ReadLinkDirectBits:
    case CommandKind::ReadHostBuffer:
    case CommandKind::ReadQualifiedWords:
    case CommandKind::ReadNativeQualifiedWords:
      return options.command_argc == 2;
    case CommandKind::ReadModuleBuffer:
      return options.command_argc == 3;
    case CommandKind::WriteLinkDirectWords:
    case CommandKind::WriteLinkDirectBits:
    case CommandKind::WriteHostBuffer:
      return options.command_argc >= 2;
    case CommandKind::WriteModuleBuffer:
      return options.command_argc >= 3;
    case CommandKind::WriteQualifiedWords:
    case CommandKind::WriteNativeQualifiedWords:
      return options.command_argc >= 2;
    case CommandKind::RandomReadLinkDirect:
    case CommandKind::RandomWriteLinkDirectWords:
    case CommandKind::RandomWriteLinkDirectBits:
    case CommandKind::MultiBlockReadLinkDirectWords:
    case CommandKind::MultiBlockReadLinkDirectBits:
    case CommandKind::MultiBlockWriteLinkDirectWords:
    case CommandKind::MultiBlockWriteLinkDirectBits:
    case CommandKind::MonitorLinkDirect:
    case CommandKind::RandomRead:
    case CommandKind::RandomWriteWords:
    case CommandKind::RandomWriteFileRegisterWords:
    case CommandKind::RandomWriteBits:
    case CommandKind::WriteWords:
    case CommandKind::WriteFileRegisterWords:
    case CommandKind::WriteFileRegisterWordsDirect:
    case CommandKind::WriteBits:
    case CommandKind::MonitorFileRegister:
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

[[nodiscard]] std::span<const std::byte> as_const_byte_span(std::span<const std::uint8_t> bytes) noexcept {
  return {
      reinterpret_cast<const std::byte*>(bytes.data()),
      bytes.size(),
  };
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

void print_hex_bytes(std::span<const std::byte> bytes) {
  for (const std::byte value : bytes) {
    std::printf("%02X", static_cast<unsigned>(std::to_integer<std::uint8_t>(value)));
  }
}

[[nodiscard]] bool parse_c24_recovery_kind(
    std::string_view arg,
    std::byte& out_control_code,
    std::string_view& out_name) {
  if (arg.empty() || equals_ignore_case(arg, "eot")) {
    out_control_code = std::byte {0x04U};
    out_name = "EOT";
    return true;
  }
  if (equals_ignore_case(arg, "cl")) {
    out_control_code = std::byte {0x0CU};
    out_name = "CL";
    return true;
  }
  return false;
}

[[nodiscard]] Status run_c24_recovery(
    PosixSerialPort& port,
    bool rts_toggle,
    std::byte control_code,
    bool dump_frames = false) {
  Status status = discard_stale_rx(port);
  if (!status.ok()) {
    return status;
  }

  const std::array<std::byte, 3> control_frame {
      control_code,
      std::byte {0x0DU},
      std::byte {0x0AU},
  };

  if (rts_toggle) {
    status = port.set_rts(true);
    if (!status.ok()) {
      return status;
    }
  }

  if (dump_frames || g_dump_frames) {
    dump_frame_bytes("tx", std::span<const std::byte>(control_frame.data(), control_frame.size()));
  }

  status = port.write_all(std::span<const std::byte>(control_frame.data(), control_frame.size()));
  if (status.ok()) {
    status = port.drain_tx();
  }

  if (rts_toggle) {
    const Status rts_status = port.set_rts(false);
    if (status.ok()) {
      status = rts_status;
    }
  }
  if (!status.ok()) {
    return status;
  }

  return discard_stale_rx(port);
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

[[nodiscard]] Status run_raw_request(
    const ProtocolConfig& config,
    PosixSerialPort& port,
    bool rts_toggle,
    std::span<const std::uint8_t> request_data,
    mcprotocol::serial::RawResponseFrame& out_frame,
    bool dump_frames = false) {
  std::array<std::uint8_t, mcprotocol::serial::kMaxRequestFrameBytes> tx_frame {};
  std::size_t tx_size = 0;
  Status status = mcprotocol::serial::FrameCodec::encode_request(config, request_data, tx_frame, tx_size);
  if (!status.ok()) {
    return status;
  }

  status = discard_stale_rx(port);
  if (!status.ok()) {
    return status;
  }

  const std::span<const std::uint8_t> tx_bytes(tx_frame.data(), tx_size);
  if (dump_frames || g_dump_frames) {
    dump_frame_bytes("tx", as_const_byte_span(tx_bytes));
  }

  if (rts_toggle) {
    status = port.set_rts(true);
    if (!status.ok()) {
      return status;
    }
  }

  status = port.write_all(as_const_byte_span(tx_bytes));
  if (status.ok()) {
    status = port.drain_tx();
  }

  if (rts_toggle) {
    const Status rts_status = port.set_rts(false);
    if (status.ok()) {
      status = rts_status;
    }
  }
  if (!status.ok()) {
    return status;
  }

  std::array<std::uint8_t, mcprotocol::serial::kMaxResponseFrameBytes> rx_frame {};
  std::size_t rx_size = 0;
  const std::uint32_t response_deadline_ms = now_ms() + config.timeout.response_timeout_ms;
  std::uint32_t inter_byte_deadline_ms = response_deadline_ms;
  bool saw_rx = false;

  std::array<std::byte, 256> rx_chunk {};
  while (true) {
    const std::uint32_t current_ms = now_ms();
    if ((!saw_rx && current_ms >= response_deadline_ms) || (saw_rx && current_ms >= inter_byte_deadline_ms)) {
      return mcprotocol::serial::make_status(
          StatusCode::Timeout,
          saw_rx ? "Timed out while waiting for the rest of the response"
                 : "Timed out while waiting for a response");
    }

    std::size_t bytes_read = 0;
    status = port.read_some(rx_chunk, 50, bytes_read);
    if (!status.ok()) {
      return status;
    }
    if (bytes_read == 0U) {
      continue;
    }

    const std::span<const std::byte> rx_chunk_bytes(rx_chunk.data(), bytes_read);
    if (dump_frames || g_dump_frames) {
      dump_frame_bytes("rx", rx_chunk_bytes);
    }

    if ((rx_size + bytes_read) > rx_frame.size()) {
      return mcprotocol::serial::make_status(StatusCode::BufferTooSmall, "Receive frame buffer overflow");
    }
    std::memcpy(
        rx_frame.data() + rx_size,
        reinterpret_cast<const std::uint8_t*>(rx_chunk_bytes.data()),
        bytes_read);
    rx_size += bytes_read;
    saw_rx = true;
    inter_byte_deadline_ms = now_ms() + config.timeout.inter_byte_timeout_ms;

    const mcprotocol::serial::DecodeResult decode =
        mcprotocol::serial::FrameCodec::decode_response(config, std::span<const std::uint8_t>(rx_frame.data(), rx_size));
    if (decode.status == mcprotocol::serial::DecodeStatus::Complete) {
      out_frame = decode.frame;
      if (decode.frame.type == mcprotocol::serial::ResponseType::PlcError) {
        return mcprotocol::serial::make_status(
            StatusCode::PlcError,
            "PLC returned an error",
            decode.frame.error_code);
      }
      return mcprotocol::serial::ok_status();
    }
    if (decode.status == mcprotocol::serial::DecodeStatus::Error) {
      return decode.error;
    }
  }
}

[[nodiscard]] Status run_read_monitor_raw(
    const ProtocolConfig& config,
    PosixSerialPort& port,
    bool rts_toggle,
    mcprotocol::serial::RawResponseFrame& out_frame,
    bool dump_frames = false) {
  std::array<std::uint8_t, mcprotocol::serial::kMaxRequestDataBytes> request_data {};
  std::size_t request_size = 0;
  const Status status = mcprotocol::serial::CommandCodec::encode_read_monitor(config, request_data, request_size);
  if (!status.ok()) {
    return status;
  }
  return run_raw_request(
      config,
      port,
      rts_toggle,
      std::span<const std::uint8_t>(request_data.data(), request_size),
      out_frame,
      dump_frames);
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
    case DeviceCode::SM:
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
    case DeviceCode::SD:
    case DeviceCode::W:
    case DeviceCode::TN:
    case DeviceCode::STN:
    case DeviceCode::CN:
    case DeviceCode::LTN:
    case DeviceCode::LSTN:
    case DeviceCode::LCN:
    case DeviceCode::SW:
    case DeviceCode::LZ:
    case DeviceCode::Z:
    case DeviceCode::R:
    case DeviceCode::RD:
    case DeviceCode::ZR:
      return false;
  }
  return false;
}

void print_sparse_native_bit_value(std::string_view label, std::uint32_t raw_value) {
  std::printf("%.*s=%u raw=0x%04X\n",
              static_cast<int>(label.size()),
              label.data(),
              mcprotocol::serial::sparse_native_requested_bit_value(raw_value) == BitValue::On ? 1U : 0U,
              static_cast<unsigned>(mcprotocol::serial::sparse_native_mask_word(raw_value)));
}

[[nodiscard]] constexpr bool is_double_word_device(DeviceCode code) {
  switch (code) {
    case DeviceCode::LTN:
    case DeviceCode::LSTN:
    case DeviceCode::LCN:
    case DeviceCode::LZ:
      return true;
    default:
      return false;
  }
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
  out_item.double_word = is_double_word_device(device.code);
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

[[nodiscard]] bool parse_extended_file_register_address(
    std::string_view text,
    const ProtocolConfig& config,
    ExtendedFileRegisterAddress& out_address) {
  const std::size_t separator_pos = text.find(':');
  const std::size_t slash_pos = text.find('/');
  const std::size_t split_pos =
      separator_pos != std::string_view::npos ? separator_pos : slash_pos;
  if (split_pos == std::string_view::npos || split_pos == 0U || split_pos == (text.size() - 1U)) {
    return false;
  }

  std::uint32_t block_number = 0;
  if (!parse_u32(text.substr(0, split_pos), block_number)) {
    return false;
  }
  const std::uint32_t max_block_number = cli_max_extended_file_register_block_number(config);
  if (cli_is_e1_frame(config)) {
    if (block_number > max_block_number) {
      return false;
    }
  } else if (block_number == 0U || block_number > max_block_number) {
    return false;
  }

  std::string_view right = text.substr(split_pos + 1U);
  if (!right.empty() && (right.front() == 'R' || right.front() == 'r')) {
    right.remove_prefix(1U);
  }
  std::uint32_t word_number = 0;
  if (!parse_u32(right, word_number) ||
      word_number > cli_max_extended_file_register_word_number(config)) {
    return false;
  }

  out_address.block_number = static_cast<std::uint16_t>(block_number);
  out_address.word_number = static_cast<std::uint16_t>(word_number);
  return true;
}

[[nodiscard]] bool parse_extended_file_register_write_arg(
    std::string_view text,
    const ProtocolConfig& config,
    ExtendedFileRegisterRandomWriteWordItem& out_item) {
  const std::size_t equal_pos = text.find('=');
  if (equal_pos == std::string_view::npos || equal_pos == 0U || equal_pos == (text.size() - 1U)) {
    return false;
  }
  std::uint32_t value = 0;
  ExtendedFileRegisterAddress address {};
  if (!parse_extended_file_register_address(text.substr(0, equal_pos), config, address) ||
      !parse_u32_auto(text.substr(equal_pos + 1U), value) ||
      value > 0xFFFFU) {
    return false;
  }
  out_item.device = address;
  out_item.value = static_cast<std::uint16_t>(value);
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

[[nodiscard]] Status run_read_extended_file_register_words(
    MelsecSerialClient& client,
    PosixSerialPort& port,
    CommandState& command_state,
    const ExtendedFileRegisterBatchReadWordsRequest& request,
    std::span<std::uint16_t> out_values) {
  command_state.done = false;
  command_state.status = Status {};

  const Status start_status = client.async_read_extended_file_register_words(
      now_ms(),
      request,
      out_values,
      request_complete,
      &command_state);
  if (!start_status.ok()) {
    return start_status;
  }
  return drive_request(client, port, command_state);
}

[[nodiscard]] Status run_direct_read_extended_file_register_words(
    MelsecSerialClient& client,
    PosixSerialPort& port,
    CommandState& command_state,
    const ExtendedFileRegisterDirectBatchReadWordsRequest& request,
    std::span<std::uint16_t> out_values) {
  command_state.done = false;
  command_state.status = Status {};

  const Status start_status = client.async_direct_read_extended_file_register_words(
      now_ms(),
      request,
      out_values,
      request_complete,
      &command_state);
  if (!start_status.ok()) {
    return start_status;
  }
  return drive_request(client, port, command_state);
}

[[nodiscard]] Status run_write_extended_file_register_words(
    MelsecSerialClient& client,
    PosixSerialPort& port,
    CommandState& command_state,
    const ExtendedFileRegisterBatchWriteWordsRequest& request) {
  command_state.done = false;
  command_state.status = Status {};

  const Status start_status = client.async_write_extended_file_register_words(
      now_ms(),
      request,
      request_complete,
      &command_state);
  if (!start_status.ok()) {
    return start_status;
  }
  return drive_request(client, port, command_state);
}

[[nodiscard]] Status run_direct_write_extended_file_register_words(
    MelsecSerialClient& client,
    PosixSerialPort& port,
    CommandState& command_state,
    const ExtendedFileRegisterDirectBatchWriteWordsRequest& request) {
  command_state.done = false;
  command_state.status = Status {};

  const Status start_status = client.async_direct_write_extended_file_register_words(
      now_ms(),
      request,
      request_complete,
      &command_state);
  if (!start_status.ok()) {
    return start_status;
  }
  return drive_request(client, port, command_state);
}

[[nodiscard]] Status run_random_write_extended_file_register_words(
    MelsecSerialClient& client,
    PosixSerialPort& port,
    CommandState& command_state,
    std::span<const ExtendedFileRegisterRandomWriteWordItem> items) {
  command_state.done = false;
  command_state.status = Status {};

  const Status start_status = client.async_random_write_extended_file_register_words(
      now_ms(),
      items,
      request_complete,
      &command_state);
  if (!start_status.ok()) {
    return start_status;
  }
  return drive_request(client, port, command_state);
}

[[nodiscard]] Status run_monitor_extended_file_register_words(
    MelsecSerialClient& client,
    PosixSerialPort& port,
    CommandState& command_state,
    const ExtendedFileRegisterMonitorRegistration& request,
    std::span<std::uint16_t> out_values) {
  command_state.done = false;
  command_state.status = Status {};

  Status status = client.async_register_extended_file_register_monitor(
      now_ms(),
      request,
      request_complete,
      &command_state);
  if (!status.ok()) {
    return status;
  }
  status = drive_request(client, port, command_state);
  if (!status.ok()) {
    return status;
  }

  command_state.done = false;
  command_state.status = Status {};
  status = client.async_read_extended_file_register_monitor(
      now_ms(),
      out_values,
      request_complete,
      &command_state);
  if (!status.ok()) {
    return status;
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

[[nodiscard]] Status run_link_direct_batch_read_words(
    MelsecSerialClient& client,
    PosixSerialPort& port,
    CommandState& command_state,
    const LinkDirectDevice& device,
    std::uint16_t points,
    std::span<std::uint16_t> out_values) {
  command_state.done = false;
  command_state.status = Status {};

  const Status start_status = client.async_link_direct_batch_read_words(
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

[[nodiscard]] Status run_link_direct_batch_read_bits(
    MelsecSerialClient& client,
    PosixSerialPort& port,
    CommandState& command_state,
    const LinkDirectDevice& device,
    std::uint16_t points,
    std::span<BitValue> out_values) {
  command_state.done = false;
  command_state.status = Status {};

  const Status start_status = client.async_link_direct_batch_read_bits(
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

[[nodiscard]] Status run_batch_write_word(
    MelsecSerialClient& client,
    PosixSerialPort& port,
    CommandState& command_state,
    const DeviceAddress& device,
    std::uint16_t value) {
  const std::array<std::uint16_t, 1> values {value};
  return run_batch_write_words_group(client, port, command_state, device, values);
}

[[nodiscard]] Status run_link_direct_batch_write_words(
    MelsecSerialClient& client,
    PosixSerialPort& port,
    CommandState& command_state,
    const LinkDirectDevice& device,
    std::span<const std::uint16_t> values) {
  command_state.done = false;
  command_state.status = Status {};

  const Status start_status = client.async_link_direct_batch_write_words(
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

[[nodiscard]] Status run_link_direct_batch_write_bits(
    MelsecSerialClient& client,
    PosixSerialPort& port,
    CommandState& command_state,
    const LinkDirectDevice& device,
    std::span<const BitValue> values) {
  command_state.done = false;
  command_state.status = Status {};

  const Status start_status = client.async_link_direct_batch_write_bits(
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

[[nodiscard]] Status run_link_direct_random_read(
    MelsecSerialClient& client,
    PosixSerialPort& port,
    CommandState& command_state,
    std::span<const LinkDirectRandomReadItem> items,
    std::span<std::uint32_t> out_values) {
  command_state.done = false;
  command_state.status = Status {};

  const Status start_status = client.async_link_direct_random_read(
      now_ms(),
      items,
      out_values,
      request_complete,
      &command_state);
  if (!start_status.ok()) {
    return start_status;
  }
  return drive_request(client, port, command_state);
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

[[nodiscard]] Status run_link_direct_random_write_words(
    MelsecSerialClient& client,
    PosixSerialPort& port,
    CommandState& command_state,
    std::span<const LinkDirectRandomWriteWordItem> items) {
  command_state.done = false;
  command_state.status = Status {};

  const Status start_status = client.async_link_direct_random_write_words(
      now_ms(),
      items,
      request_complete,
      &command_state);
  if (!start_status.ok()) {
    return start_status;
  }
  return drive_request(client, port, command_state);
}

[[nodiscard]] Status run_random_read(
    MelsecSerialClient& client,
    PosixSerialPort& port,
    CommandState& command_state,
    std::span<const RandomReadItem> items,
    std::span<std::uint32_t> out_values) {
  command_state.done = false;
  command_state.status = Status {};

  const Status start_status = client.async_random_read(
      now_ms(),
      RandomReadRequest {.items = items},
      out_values,
      request_complete,
      &command_state);
  if (!start_status.ok()) {
    return start_status;
  }
  return drive_request(client, port, command_state);
}

[[nodiscard]] Status run_link_direct_random_write_bits(
    MelsecSerialClient& client,
    PosixSerialPort& port,
    CommandState& command_state,
    std::span<const LinkDirectRandomWriteBitItem> items) {
  command_state.done = false;
  command_state.status = Status {};

  const Status start_status = client.async_link_direct_random_write_bits(
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

[[nodiscard]] Status run_link_direct_multi_block_read(
    MelsecSerialClient& client,
    PosixSerialPort& port,
    CommandState& command_state,
    const LinkDirectMultiBlockReadRequest& request,
    std::span<std::uint16_t> out_words,
    std::span<BitValue> out_bits,
    std::span<MultiBlockReadBlockResult> out_results) {
  command_state.done = false;
  command_state.status = Status {};

  const Status start_status = client.async_link_direct_multi_block_read(
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

[[nodiscard]] Status run_link_direct_multi_block_write(
    MelsecSerialClient& client,
    PosixSerialPort& port,
    CommandState& command_state,
    const LinkDirectMultiBlockWriteRequest& request) {
  command_state.done = false;
  command_state.status = Status {};

  const Status start_status = client.async_link_direct_multi_block_write(
      now_ms(),
      request,
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

[[nodiscard]] Status run_link_direct_register_monitor(
    MelsecSerialClient& client,
    PosixSerialPort& port,
    CommandState& command_state,
    std::span<const LinkDirectRandomReadItem> items) {
  command_state.done = false;
  command_state.status = Status {};

  const Status start_status = client.async_link_direct_register_monitor(
      now_ms(),
      LinkDirectMonitorRegistration {.items = items},
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
  if (options.command != CommandKind::RecoverC24) {
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

    case CommandKind::RemoteRun: {
      RemoteOperationMode mode = RemoteOperationMode::DoNotExecuteForcibly;
      RemoteRunClearMode clear_mode = RemoteRunClearMode::DoNotClear;
      if (options.command_argc >= 1 &&
          !parse_remote_operation_mode(options.command_argv[0], mode)) {
        std::fprintf(stderr, "remote-run mode must be no-force or force\n");
        return 1;
      }
      if (options.command_argc >= 2 &&
          !parse_remote_run_clear_mode(options.command_argv[1], clear_mode)) {
        std::fprintf(stderr, "remote-run clear mode must be no-clear, outside-latch, or all-clear\n");
        return 1;
      }
      status = client.async_remote_run(now_ms(), mode, clear_mode, request_complete, &command_state);
      if (!status.ok()) {
        print_status_error("Failed to start remote-run request", status);
        return 1;
      }
      status = drive_request(client, port, command_state);
      if (!status.ok()) {
        print_status_error("remote-run request failed", status);
        return 1;
      }
      std::printf("remote-run=ok mode=0x%04X clear=0x%02X\n",
                  static_cast<unsigned>(mode),
                  static_cast<unsigned>(clear_mode));
      return 0;
    }

    case CommandKind::RemoteStop: {
      status = client.async_remote_stop(now_ms(), request_complete, &command_state);
      if (!status.ok()) {
        print_status_error("Failed to start remote-stop request", status);
        return 1;
      }
      status = drive_request(client, port, command_state);
      if (!status.ok()) {
        print_status_error("remote-stop request failed", status);
        return 1;
      }
      std::printf("remote-stop=ok\n");
      return 0;
    }

    case CommandKind::RemotePause: {
      RemoteOperationMode mode = RemoteOperationMode::DoNotExecuteForcibly;
      if (options.command_argc >= 1 &&
          !parse_remote_operation_mode(options.command_argv[0], mode)) {
        std::fprintf(stderr, "remote-pause mode must be no-force or force\n");
        return 1;
      }
      status = client.async_remote_pause(now_ms(), mode, request_complete, &command_state);
      if (!status.ok()) {
        print_status_error("Failed to start remote-pause request", status);
        return 1;
      }
      status = drive_request(client, port, command_state);
      if (!status.ok()) {
        print_status_error("remote-pause request failed", status);
        return 1;
      }
      std::printf("remote-pause=ok mode=0x%04X\n", static_cast<unsigned>(mode));
      return 0;
    }

    case CommandKind::RemoteLatchClear: {
      status = client.async_remote_latch_clear(now_ms(), request_complete, &command_state);
      if (!status.ok()) {
        print_status_error("Failed to start latch-clear request", status);
        return 1;
      }
      status = drive_request(client, port, command_state);
      if (!status.ok()) {
        print_status_error("latch-clear request failed", status);
        return 1;
      }
      std::printf("latch-clear=ok\n");
      return 0;
    }

    case CommandKind::UnlockRemotePassword: {
      const std::string_view remote_password(options.command_argv[0]);
      status = client.async_unlock_remote_password(
          now_ms(),
          remote_password,
          request_complete,
          &command_state);
      if (!status.ok()) {
        print_status_error("Failed to start unlock request", status);
        return 1;
      }
      status = drive_request(client, port, command_state);
      if (!status.ok()) {
        print_status_error("unlock request failed", status);
        return 1;
      }
      std::printf("unlock=ok length=%zu\n", remote_password.size());
      return 0;
    }

    case CommandKind::LockRemotePassword: {
      const std::string_view remote_password(options.command_argv[0]);
      status = client.async_lock_remote_password(
          now_ms(),
          remote_password,
          request_complete,
          &command_state);
      if (!status.ok()) {
        print_status_error("Failed to start lock request", status);
        return 1;
      }
      status = drive_request(client, port, command_state);
      if (!status.ok()) {
        print_status_error("lock request failed", status);
        return 1;
      }
      std::printf("lock=ok length=%zu\n", remote_password.size());
      return 0;
    }

    case CommandKind::ClearError: {
      status = client.async_clear_error_information(now_ms(), request_complete, &command_state);
      if (!status.ok()) {
        print_status_error("Failed to start error-clear request", status);
        return 1;
      }
      status = drive_request(client, port, command_state);
      if (!status.ok()) {
        print_status_error("error-clear request failed", status);
        return 1;
      }
      std::printf("error-clear=ok\n");
      return 0;
    }

    case CommandKind::RemoteReset: {
      status = client.async_remote_reset(now_ms(), request_complete, &command_state);
      if (!status.ok()) {
        print_status_error("Failed to start remote-reset request", status);
        return 1;
      }
      status = drive_request(client, port, command_state);
      if (!status.ok()) {
        print_status_error("remote-reset request failed", status);
        return 1;
      }
      const bool no_response =
          std::strcmp(status.message, "Remote RESET completed without a response") == 0;
      std::printf("remote-reset=ok response=%s\n", no_response ? "none" : "ack");
      return 0;
    }

    case CommandKind::GlobalSignal: {
      bool turn_on = false;
      GlobalSignalTarget target = GlobalSignalTarget::ReceivedSide;
      std::uint8_t station_no = 0;
      if (!parse_on_off(std::string_view(options.command_argv[0]), turn_on)) {
        std::fprintf(stderr, "global-signal state must be on or off\n");
        return 1;
      }
      if (!parse_global_signal_target(std::string_view(options.command_argv[1]), target)) {
        std::fprintf(stderr, "global-signal target must be current, x1a, or x1b\n");
        return 1;
      }
      if (options.command_argc == 3) {
        std::uint32_t station_value = 0;
        if (!parse_u32_auto(std::string_view(options.command_argv[2]), station_value) || station_value > 31U) {
          std::fprintf(stderr, "global-signal station must be in range 0..31\n");
          return 1;
        }
        station_no = static_cast<std::uint8_t>(station_value);
      }

      status = client.async_control_global_signal(
          now_ms(),
          GlobalSignalControlRequest {
              .target = target,
              .turn_on = turn_on,
              .station_no = station_no,
          },
          request_complete,
          &command_state);
      if (!status.ok()) {
        print_status_error("Failed to start global-signal request", status);
        return 1;
      }
      status = drive_request(client, port, command_state);
      if (!status.ok()) {
        print_status_error("global-signal request failed", status);
        return 1;
      }
      const bool no_response =
          std::strcmp(
              status.message,
              "Global signal control completed without a response") == 0;
      std::printf(
          "global-signal=ok state=%s target=%s station=%u response=%s\n",
          turn_on ? "on" : "off",
          global_signal_target_name(target),
          static_cast<unsigned>(station_no),
          no_response ? "none" : "ack");
      return 0;
    }

    case CommandKind::InitializeTransmissionSequence: {
      status = client.async_initialize_c24_transmission_sequence(
          now_ms(),
          request_complete,
          &command_state);
      if (!status.ok()) {
        print_status_error("Failed to start init-sequence request", status);
        return 1;
      }
      status = drive_request(client, port, command_state);
      if (!status.ok()) {
        print_status_error("init-sequence request failed", status);
        return 1;
      }
      const bool no_response =
          std::strcmp(
              status.message,
              "Transmission-sequence initialization completed without a response") == 0;
      std::printf("init-sequence=ok response=%s\n", no_response ? "none" : "ack");
      return 0;
    }

    case CommandKind::DeregisterCpuMonitoring: {
      status = client.async_deregister_cpu_monitoring(
          now_ms(),
          request_complete,
          &command_state);
      if (!status.ok()) {
        print_status_error("Failed to start deregister-cpu-monitor request", status);
        return 1;
      }
      status = drive_request(client, port, command_state);
      if (!status.ok()) {
        print_status_error("deregister-cpu-monitor request failed", status);
        return 1;
      }
      std::printf("deregister-cpu-monitor=ok\n");
      return 0;
    }

    case CommandKind::RecoverC24: {
      std::byte control_code {};
      std::string_view control_name;
      const std::string_view requested_kind =
          options.command_argc == 0 ? std::string_view {} : std::string_view(options.command_argv[0]);
      if (!parse_c24_recovery_kind(requested_kind, control_code, control_name)) {
        std::fprintf(stderr, "recover-c24 mode must be 'eot' or 'cl'\n");
        return 1;
      }
      status = run_c24_recovery(port, options.rts_toggle, control_code, options.dump_frames);
      if (!status.ok()) {
        print_status_error("recover-c24 failed", status);
        return 1;
      }
      std::printf("recover-c24=ok code=%.*s response=none\n",
                  static_cast<int>(control_name.size()),
                  control_name.data());
      return 0;
    }

    case CommandKind::Loopback: {
      std::array<char, mcprotocol::serial::kMaxLoopbackBytes + 1U> echoed {};
      const char* loopback_text = options.command_argv[0];
      status = client.async_loopback(
          now_ms(),
          std::span<const char>(loopback_text, std::strlen(loopback_text)),
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

    case CommandKind::ReadUserFrame: {
      const std::string_view frame_arg(options.command_argv[0]);
      std::uint32_t frame_no = 0;
      if (!parse_u32_auto(frame_arg, frame_no) || frame_no > 0xFFFFU) {
        std::fprintf(stderr, "Invalid user-frame number: %.*s\n",
                     static_cast<int>(frame_arg.size()),
                     frame_arg.data());
        return 2;
      }

      UserFrameRegistrationData data {};
      status = client.async_read_user_frame(
          now_ms(),
          UserFrameReadRequest {.frame_no = static_cast<std::uint16_t>(frame_no)},
          data,
          request_complete,
          &command_state);
      if (!status.ok()) {
        print_status_error("Failed to start read-user-frame request", status);
        return 1;
      }
      status = drive_request(client, port, command_state);
      if (!status.ok()) {
        print_status_error("read-user-frame request failed", status);
        return 1;
      }

      std::printf("read-user-frame=ok frame-no=0x%04X registration-data-bytes=%u frame-bytes=%u\n",
                  static_cast<unsigned>(frame_no),
                  static_cast<unsigned>(data.registration_data_bytes),
                  static_cast<unsigned>(data.frame_bytes));
      std::printf("registration-data=");
      print_hex_bytes(std::span<const std::byte>(data.registration_data.data(), data.registration_data_bytes));
      std::printf("\n");
      return 0;
    }

    case CommandKind::WriteUserFrame: {
      const std::string_view frame_arg(options.command_argv[0]);
      const std::string_view frame_bytes_arg(options.command_argv[1]);
      const std::string_view registration_arg(options.command_argv[2]);
      std::uint32_t frame_no = 0;
      std::uint32_t frame_bytes = 0;
      if (!parse_u32_auto(frame_arg, frame_no) || frame_no > 0xFFFFU) {
        std::fprintf(stderr, "Invalid user-frame number: %.*s\n",
                     static_cast<int>(frame_arg.size()),
                     frame_arg.data());
        return 2;
      }
      if (!parse_u32_auto(frame_bytes_arg, frame_bytes) || frame_bytes > 0xFFFFU) {
        std::fprintf(stderr, "Invalid user-frame frame-bytes value: %.*s\n",
                     static_cast<int>(frame_bytes_arg.size()),
                     frame_bytes_arg.data());
        return 2;
      }

      std::array<std::byte, mcprotocol::serial::kMaxUserFrameRegistrationBytes> registration_data {};
      std::size_t registration_size = 0U;
      if (!parse_hex_byte_string(
              registration_arg,
              std::span<std::byte>(registration_data.data(), registration_data.size()),
              registration_size)) {
        std::fprintf(stderr, "Invalid user-frame registration data: %.*s\n",
                     static_cast<int>(registration_arg.size()),
                     registration_arg.data());
        return 2;
      }

      status = client.async_write_user_frame(
          now_ms(),
          UserFrameWriteRequest {
              .frame_no = static_cast<std::uint16_t>(frame_no),
              .frame_bytes = static_cast<std::uint16_t>(frame_bytes),
              .registration_data = std::span<const std::byte>(registration_data.data(), registration_size),
          },
          request_complete,
          &command_state);
      if (!status.ok()) {
        print_status_error("Failed to start register-user-frame request", status);
        return 1;
      }
      status = drive_request(client, port, command_state);
      if (!status.ok()) {
        print_status_error("register-user-frame request failed", status);
        return 1;
      }

      std::printf("register-user-frame=ok frame-no=0x%04X registration-data-bytes=%zu frame-bytes=%u\n",
                  static_cast<unsigned>(frame_no),
                  registration_size,
                  static_cast<unsigned>(frame_bytes));
      return 0;
    }

    case CommandKind::DeleteUserFrame: {
      const std::string_view frame_arg(options.command_argv[0]);
      std::uint32_t frame_no = 0;
      if (!parse_u32_auto(frame_arg, frame_no) || frame_no > 0xFFFFU) {
        std::fprintf(stderr, "Invalid user-frame number: %.*s\n",
                     static_cast<int>(frame_arg.size()),
                     frame_arg.data());
        return 2;
      }

      status = client.async_delete_user_frame(
          now_ms(),
          UserFrameDeleteRequest {.frame_no = static_cast<std::uint16_t>(frame_no)},
          request_complete,
          &command_state);
      if (!status.ok()) {
        print_status_error("Failed to start delete-user-frame request", status);
        return 1;
      }
      status = drive_request(client, port, command_state);
      if (!status.ok()) {
        print_status_error("delete-user-frame request failed", status);
        return 1;
      }

      std::printf("delete-user-frame=ok frame-no=0x%04X\n", static_cast<unsigned>(frame_no));
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

    case CommandKind::ProbeRandomRead: {
      constexpr DeviceAddress kWordHeadDevice {.code = mcprotocol::serial::DeviceCode::D, .number = 100};
      constexpr DeviceAddress kBitHeadDevice {.code = mcprotocol::serial::DeviceCode::M, .number = 100};
      std::array<std::uint16_t, 6> contiguous_words {};
      std::array<BitValue, 6> contiguous_bits {};

      const auto print_probe_status_line = [](std::string_view label, Status status) {
        std::printf("%.*s=skip ",
                    static_cast<int>(label.size()),
                    label.data());
        if (status.code == StatusCode::PlcError) {
          std::printf("0x%04X\n", status.plc_error_code);
        } else {
          std::printf("%s\n", status.message);
        }
      };

      const auto verify_random_word_values = [](std::string_view label,
                                                std::span<const std::uint16_t> expected,
                                                std::span<const std::uint32_t> actual) -> bool {
        for (std::size_t index = 0; index < expected.size(); ++index) {
          const std::uint16_t actual_word = static_cast<std::uint16_t>(actual[index] & 0xFFFFU);
          if (actual_word != expected[index]) {
            std::printf("%.*s-mismatch[%zu] expected=0x%04X read=0x%04X\n",
                        static_cast<int>(label.size()),
                        label.data(),
                        index,
                        expected[index],
                        actual_word);
            return false;
          }
        }
        return true;
      };

      const auto verify_random_bit_values = [](std::string_view label,
                                               std::span<const BitValue> expected,
                                               std::span<const std::uint32_t> actual) -> bool {
        for (std::size_t index = 0; index < expected.size(); ++index) {
          const BitValue actual_bit = mcprotocol::serial::sparse_native_requested_bit_value(actual[index]);
          if (actual_bit != expected[index]) {
            std::printf("%.*s-mismatch[%zu] expected=%u read=%u raw=0x%04X\n",
                        static_cast<int>(label.size()),
                        label.data(),
                        index,
                        expected[index] == BitValue::On ? 1U : 0U,
                        actual_bit == BitValue::On ? 1U : 0U,
                        static_cast<unsigned>(mcprotocol::serial::sparse_native_mask_word(actual[index])));
            return false;
          }
        }
        return true;
      };

      status = run_batch_read_words_group(
          client,
          port,
          command_state,
          kWordHeadDevice,
          static_cast<std::uint16_t>(contiguous_words.size()),
          std::span<std::uint16_t>(contiguous_words.data(), contiguous_words.size()));
      if (!status.ok()) {
        print_status_error("probe-random-read contiguous word baseline failed", status);
        return 1;
      }
      std::printf("batch-read-words=ok contiguous D100=0x%04X D105=0x%04X\n",
                  contiguous_words.front(),
                  contiguous_words.back());

      status = run_batch_read_bits_group(
          client,
          port,
          command_state,
          kBitHeadDevice,
          static_cast<std::uint16_t>(contiguous_bits.size()),
          std::span<BitValue>(contiguous_bits.data(), contiguous_bits.size()));
      if (!status.ok()) {
        print_status_error("probe-random-read contiguous bit baseline failed", status);
        return 1;
      }
      std::printf("batch-read-bits=ok contiguous M100=%u M105=%u\n",
                  contiguous_bits.front() == BitValue::On ? 1U : 0U,
                  contiguous_bits.back() == BitValue::On ? 1U : 0U);

      const std::array<RandomReadItem, 1> word_single_items {{
          {.device = DeviceAddress {.code = DeviceCode::D, .number = 100U}, .double_word = false},
      }};
      const std::array<std::uint16_t, 1> word_single_expected {{
          contiguous_words[0],
      }};
      std::array<std::uint32_t, 1> word_single_values {};
      bool word_single_ok = false;
      status = run_random_read(
          client,
          port,
          command_state,
          std::span<const RandomReadItem>(word_single_items.data(), word_single_items.size()),
          std::span<std::uint32_t>(word_single_values.data(), word_single_values.size()));
      if (!status.ok()) {
        print_probe_status_line("random-read-word-single", status);
      } else if (!verify_random_word_values(
                     "random-read-word-single",
                     std::span<const std::uint16_t>(word_single_expected.data(), word_single_expected.size()),
                     std::span<const std::uint32_t>(word_single_values.data(), word_single_values.size()))) {
        std::printf("random-read-word-single=skip verify-mismatch\n");
      } else {
        word_single_ok = true;
        std::printf("random-read-word-single=ok native\n");
      }

      const std::array<RandomReadItem, 2> word_dense_items {{
          {.device = DeviceAddress {.code = DeviceCode::D, .number = 100U}, .double_word = false},
          {.device = DeviceAddress {.code = DeviceCode::D, .number = 101U}, .double_word = false},
      }};
      const std::array<std::uint16_t, 2> word_dense_expected {{
          contiguous_words[0],
          contiguous_words[1],
      }};
      std::array<std::uint32_t, 2> word_dense_values {};
      bool word_dense_ok = false;
      status = run_random_read(
          client,
          port,
          command_state,
          std::span<const RandomReadItem>(word_dense_items.data(), word_dense_items.size()),
          std::span<std::uint32_t>(word_dense_values.data(), word_dense_values.size()));
      if (!status.ok()) {
        print_probe_status_line("random-read-word-dense", status);
      } else if (!verify_random_word_values(
                     "random-read-word-dense",
                     std::span<const std::uint16_t>(word_dense_expected.data(), word_dense_expected.size()),
                     std::span<const std::uint32_t>(word_dense_values.data(), word_dense_values.size()))) {
        std::printf("random-read-word-dense=skip verify-mismatch\n");
      } else {
        word_dense_ok = true;
        std::printf("random-read-word-dense=ok native\n");
      }

      const std::array<RandomReadItem, 2> word_sparse_items {{
          {.device = DeviceAddress {.code = DeviceCode::D, .number = 100U}, .double_word = false},
          {.device = DeviceAddress {.code = DeviceCode::D, .number = 105U}, .double_word = false},
      }};
      const std::array<std::uint16_t, 2> word_sparse_expected {{
          contiguous_words[0],
          contiguous_words[5],
      }};
      std::array<std::uint32_t, 2> word_sparse_values {};
      bool word_sparse_ok = false;
      status = run_random_read(
          client,
          port,
          command_state,
          std::span<const RandomReadItem>(word_sparse_items.data(), word_sparse_items.size()),
          std::span<std::uint32_t>(word_sparse_values.data(), word_sparse_values.size()));
      if (!status.ok()) {
        print_probe_status_line("random-read-word-sparse", status);
      } else if (!verify_random_word_values(
                     "random-read-word-sparse",
                     std::span<const std::uint16_t>(word_sparse_expected.data(), word_sparse_expected.size()),
                     std::span<const std::uint32_t>(word_sparse_values.data(), word_sparse_values.size()))) {
        std::printf("random-read-word-sparse=skip verify-mismatch\n");
      } else {
        word_sparse_ok = true;
        std::printf("random-read-word-sparse=ok native\n");
      }

      const std::array<RandomReadItem, 1> bit_single_items {{
          {.device = DeviceAddress {.code = DeviceCode::M, .number = 100U}, .double_word = false},
      }};
      const std::array<BitValue, 1> bit_single_expected {{
          contiguous_bits[0],
      }};
      std::array<std::uint32_t, 1> bit_single_values {};
      bool bit_single_ok = false;
      status = run_random_read(
          client,
          port,
          command_state,
          std::span<const RandomReadItem>(bit_single_items.data(), bit_single_items.size()),
          std::span<std::uint32_t>(bit_single_values.data(), bit_single_values.size()));
      if (!status.ok()) {
        print_probe_status_line("random-read-bit-single", status);
      } else if (!verify_random_bit_values(
                     "random-read-bit-single",
                     std::span<const BitValue>(bit_single_expected.data(), bit_single_expected.size()),
                     std::span<const std::uint32_t>(bit_single_values.data(), bit_single_values.size()))) {
        std::printf("random-read-bit-single=skip verify-mismatch\n");
      } else {
        bit_single_ok = true;
        std::printf("random-read-bit-single=ok native\n");
      }

      const std::array<RandomReadItem, 2> bit_dense_items {{
          {.device = DeviceAddress {.code = DeviceCode::M, .number = 100U}, .double_word = false},
          {.device = DeviceAddress {.code = DeviceCode::M, .number = 101U}, .double_word = false},
      }};
      const std::array<BitValue, 2> bit_dense_expected {{
          contiguous_bits[0],
          contiguous_bits[1],
      }};
      std::array<std::uint32_t, 2> bit_dense_values {};
      bool bit_dense_ok = false;
      status = run_random_read(
          client,
          port,
          command_state,
          std::span<const RandomReadItem>(bit_dense_items.data(), bit_dense_items.size()),
          std::span<std::uint32_t>(bit_dense_values.data(), bit_dense_values.size()));
      if (!status.ok()) {
        print_probe_status_line("random-read-bit-dense", status);
      } else if (!verify_random_bit_values(
                     "random-read-bit-dense",
                     std::span<const BitValue>(bit_dense_expected.data(), bit_dense_expected.size()),
                     std::span<const std::uint32_t>(bit_dense_values.data(), bit_dense_values.size()))) {
        std::printf("random-read-bit-dense=skip verify-mismatch\n");
      } else {
        bit_dense_ok = true;
        std::printf("random-read-bit-dense=ok native\n");
      }

      const std::array<RandomReadItem, 2> bit_sparse_items {{
          {.device = DeviceAddress {.code = DeviceCode::M, .number = 100U}, .double_word = false},
          {.device = DeviceAddress {.code = DeviceCode::M, .number = 105U}, .double_word = false},
      }};
      const std::array<BitValue, 2> bit_sparse_expected {{
          contiguous_bits[0],
          contiguous_bits[5],
      }};
      std::array<std::uint32_t, 2> bit_sparse_values {};
      bool bit_sparse_ok = false;
      status = run_random_read(
          client,
          port,
          command_state,
          std::span<const RandomReadItem>(bit_sparse_items.data(), bit_sparse_items.size()),
          std::span<std::uint32_t>(bit_sparse_values.data(), bit_sparse_values.size()));
      if (!status.ok()) {
        print_probe_status_line("random-read-bit-sparse", status);
      } else if (!verify_random_bit_values(
                     "random-read-bit-sparse",
                     std::span<const BitValue>(bit_sparse_expected.data(), bit_sparse_expected.size()),
                     std::span<const std::uint32_t>(bit_sparse_values.data(), bit_sparse_values.size()))) {
        std::printf("random-read-bit-sparse=skip verify-mismatch\n");
      } else {
        bit_sparse_ok = true;
        std::printf("random-read-bit-sparse=ok native\n");
      }

      std::printf("probe-random-read: word-single=%s word-dense=%s word-sparse=%s bit-single=%s bit-dense=%s bit-sparse=%s\n",
                  word_single_ok ? "native" : "skip",
                  word_dense_ok ? "native" : "skip",
                  word_sparse_ok ? "native" : "skip",
                  bit_single_ok ? "native" : "skip",
                  bit_dense_ok ? "native" : "skip",
                  bit_sparse_ok ? "native" : "skip");
      return (word_single_ok || word_dense_ok || word_sparse_ok || bit_single_ok || bit_dense_ok || bit_sparse_ok) ? 0 : 1;
    }

    case CommandKind::ProbeRandomWriteWords: {
      constexpr DeviceAddress kHeadDevice {.code = mcprotocol::serial::DeviceCode::D, .number = 100};
      std::array<std::uint16_t, 6> backup_words {};
      bool backups_valid = false;

      const auto verify_words = [](std::string_view context,
                                   std::span<const std::uint16_t> expected,
                                   std::span<const std::uint16_t> actual) -> bool {
        for (std::size_t index = 0; index < expected.size(); ++index) {
          if (actual[index] != expected[index]) {
            std::printf("%.*s-mismatch[%zu] expected=0x%04X read=0x%04X\n",
                        static_cast<int>(context.size()),
                        context.data(),
                        index,
                        expected[index],
                        actual[index]);
            return false;
          }
        }
        return true;
      };

      const auto print_probe_status_line = [](std::string_view label, Status status) {
        std::printf("%.*s=skip ",
                    static_cast<int>(label.size()),
                    label.data());
        if (status.code == StatusCode::PlcError) {
          std::printf("0x%04X\n", status.plc_error_code);
        } else {
          std::printf("%s\n", status.message);
        }
      };

      const auto restore_originals = [&]() -> bool {
        if (!backups_valid) {
          return true;
        }
        status = run_batch_write_words_group(
            client,
            port,
            command_state,
            kHeadDevice,
            std::span<const std::uint16_t>(backup_words.data(), backup_words.size()));
        if (!status.ok()) {
          print_status_error("probe-random-write-words restore failed", status);
          return false;
        }
        return true;
      };

      status = run_batch_read_words_group(
          client,
          port,
          command_state,
          kHeadDevice,
          static_cast<std::uint16_t>(backup_words.size()),
          std::span<std::uint16_t>(backup_words.data(), backup_words.size()));
      if (!status.ok()) {
        print_status_error("probe-random-write-words backup failed", status);
        return 1;
      }
      backups_valid = true;

      const std::array<std::uint16_t, 6> dense_pattern {{
          0x1111U, 0x2222U, 0x3333U, 0x4444U, 0x5555U, 0x6666U,
      }};
      const std::array<std::uint16_t, 2> random_values {{
          0x1357U, 0x2468U,
      }};
      std::array<std::uint16_t, 6> contiguous_verify {};
      std::array<std::uint16_t, 6> dense_verify {};
      std::array<std::uint16_t, 6> single_verify {};
      std::array<std::uint16_t, 6> sparse_verify {};
      std::array<std::uint16_t, 6> single_expected = backup_words;
      std::array<std::uint16_t, 6> dense_expected = backup_words;
      std::array<std::uint16_t, 6> sparse_expected = backup_words;
      single_expected[0] = random_values[0];
      dense_expected[0] = random_values[0];
      dense_expected[1] = random_values[1];
      sparse_expected[0] = random_values[0];
      sparse_expected[5] = random_values[1];

      bool contiguous_ok = false;
      status = run_batch_write_words_group(
          client,
          port,
          command_state,
          kHeadDevice,
          std::span<const std::uint16_t>(dense_pattern.data(), dense_pattern.size()));
      if (!status.ok()) {
        print_probe_status_line("batch-write-words", status);
      } else {
        status = run_batch_read_words_group(
            client,
            port,
            command_state,
            kHeadDevice,
            static_cast<std::uint16_t>(contiguous_verify.size()),
            std::span<std::uint16_t>(contiguous_verify.data(), contiguous_verify.size()));
        if (!status.ok()) {
          print_probe_status_line("batch-write-words-verify", status);
        } else if (!verify_words("batch-write-words", dense_pattern, contiguous_verify)) {
          std::printf("batch-write-words=skip verify-mismatch\n");
        } else {
          contiguous_ok = true;
          std::printf("batch-write-words=ok contiguous\n");
        }
      }

      if (!restore_originals()) {
        return 1;
      }

      const std::array<RandomWriteWordItem, 1> single_item {{
          {.device = DeviceAddress {.code = DeviceCode::D, .number = 100U}, .value = random_values[0], .double_word = false},
      }};
      bool random_single_ok = false;
      status = run_random_write_words(
          client,
          port,
          command_state,
          std::span<const RandomWriteWordItem>(single_item.data(), single_item.size()));
      if (!status.ok()) {
        print_probe_status_line("random-write-words-single", status);
      } else {
        status = run_batch_read_words_group(
            client,
            port,
            command_state,
            kHeadDevice,
            static_cast<std::uint16_t>(single_verify.size()),
            std::span<std::uint16_t>(single_verify.data(), single_verify.size()));
        if (!status.ok()) {
          print_probe_status_line("random-write-words-single-verify", status);
        } else if (!verify_words("random-write-words-single", single_expected, single_verify)) {
          std::printf("random-write-words-single=skip verify-mismatch\n");
        } else {
          random_single_ok = true;
          std::printf("random-write-words-single=ok native\n");
        }
      }

      if (!restore_originals()) {
        return 1;
      }

      const std::array<RandomWriteWordItem, 2> dense_items {{
          {.device = DeviceAddress {.code = DeviceCode::D, .number = 100U}, .value = random_values[0], .double_word = false},
          {.device = DeviceAddress {.code = DeviceCode::D, .number = 101U}, .value = random_values[1], .double_word = false},
      }};
      bool random_dense_ok = false;
      status = run_random_write_words(
          client,
          port,
          command_state,
          std::span<const RandomWriteWordItem>(dense_items.data(), dense_items.size()));
      if (!status.ok()) {
        print_probe_status_line("random-write-words-dense", status);
      } else {
        status = run_batch_read_words_group(
            client,
            port,
            command_state,
            kHeadDevice,
            static_cast<std::uint16_t>(dense_verify.size()),
            std::span<std::uint16_t>(dense_verify.data(), dense_verify.size()));
        if (!status.ok()) {
          print_probe_status_line("random-write-words-dense-verify", status);
        } else if (!verify_words("random-write-words-dense", dense_expected, dense_verify)) {
          std::printf("random-write-words-dense=skip verify-mismatch\n");
        } else {
          random_dense_ok = true;
          std::printf("random-write-words-dense=ok native\n");
        }
      }

      if (!restore_originals()) {
        return 1;
      }

      const std::array<RandomWriteWordItem, 2> sparse_items {{
          {.device = DeviceAddress {.code = DeviceCode::D, .number = 100U}, .value = random_values[0], .double_word = false},
          {.device = DeviceAddress {.code = DeviceCode::D, .number = 105U}, .value = random_values[1], .double_word = false},
      }};
      bool random_sparse_ok = false;
      status = run_random_write_words(
          client,
          port,
          command_state,
          std::span<const RandomWriteWordItem>(sparse_items.data(), sparse_items.size()));
      if (!status.ok()) {
        print_probe_status_line("random-write-words-sparse", status);
      } else {
        status = run_batch_read_words_group(
            client,
            port,
            command_state,
            kHeadDevice,
            static_cast<std::uint16_t>(sparse_verify.size()),
            std::span<std::uint16_t>(sparse_verify.data(), sparse_verify.size()));
        if (!status.ok()) {
          print_probe_status_line("random-write-words-sparse-verify", status);
        } else if (!verify_words("random-write-words-sparse", sparse_expected, sparse_verify)) {
          std::printf("random-write-words-sparse=skip verify-mismatch\n");
        } else {
          random_sparse_ok = true;
          std::printf("random-write-words-sparse=ok native\n");
        }
      }

      if (!restore_originals()) {
        return 1;
      }

      std::array<std::uint16_t, 6> restore_verify {};
      status = run_batch_read_words_group(
          client,
          port,
          command_state,
          kHeadDevice,
          static_cast<std::uint16_t>(restore_verify.size()),
          std::span<std::uint16_t>(restore_verify.data(), restore_verify.size()));
      if (!status.ok()) {
        print_status_error("probe-random-write-words restore verify failed", status);
        return 1;
      }
      if (!verify_words("probe-random-write-words-restore", backup_words, restore_verify)) {
        return 1;
      }

      std::printf("probe-random-write-words: contiguous=%s single=%s dense=%s sparse=%s restore=ok\n",
                  contiguous_ok ? "contiguous" : "skip",
                  random_single_ok ? "native" : "skip",
                  random_dense_ok ? "native" : "skip",
                  random_sparse_ok ? "native" : "skip");
      return (random_single_ok || random_dense_ok || random_sparse_ok) ? 0 : 1;
    }

    case CommandKind::ProbeRandomWriteBits: {
      constexpr DeviceAddress kHeadDevice {.code = mcprotocol::serial::DeviceCode::M, .number = 100};
      std::array<BitValue, 16> backup_bits {};
      bool backups_valid = false;

      const auto verify_bits = [](std::string_view context,
                                  std::span<const BitValue> expected,
                                  std::span<const BitValue> actual) -> bool {
        const auto pack_word = [](std::span<const BitValue> bits) -> std::uint16_t {
          std::uint16_t value = 0;
          for (std::size_t bit = 0; bit < bits.size() && bit < 16U; ++bit) {
            if (bits[bit] == BitValue::On) {
              value = static_cast<std::uint16_t>(value | (1U << (15U - bit)));
            }
          }
          return value;
        };
        const auto print_bit_string = [](std::span<const BitValue> bits) {
          for (std::size_t index = 0; index < bits.size(); ++index) {
            std::printf("%c", bits[index] == BitValue::On ? '1' : '0');
            if (((index + 1U) % 4U) == 0U && (index + 1U) != bits.size()) {
              std::printf("_");
            }
          }
        };
        for (std::size_t index = 0; index < expected.size(); ++index) {
          if (actual[index] != expected[index]) {
            std::printf("%.*s-mismatch[%zu] expected=%u read=%u",
                        static_cast<int>(context.size()),
                        context.data(),
                        index,
                        expected[index] == BitValue::On ? 1U : 0U,
                        actual[index] == BitValue::On ? 1U : 0U);
            if ((expected.size() % 16U) == 0U) {
              const std::size_t word_index = index / 16U;
              const auto expected_word = pack_word(expected.subspan(word_index * 16U, 16U));
              const auto actual_word = pack_word(actual.subspan(word_index * 16U, 16U));
              std::printf(" expected-word[%zu]=0x%04X read-word[%zu]=0x%04X",
                          word_index,
                          expected_word,
                          word_index,
                          actual_word);
            }
            std::printf("\n%.*s-expected-bits=",
                        static_cast<int>(context.size()),
                        context.data());
            print_bit_string(expected);
            std::printf("\n%.*s-read-bits=",
                        static_cast<int>(context.size()),
                        context.data());
            print_bit_string(actual);
            std::printf("\n");
            return false;
          }
        }
        return true;
      };

      const auto print_probe_status_line = [](std::string_view label, Status status) {
        std::printf("%.*s=skip ",
                    static_cast<int>(label.size()),
                    label.data());
        if (status.code == StatusCode::PlcError) {
          std::printf("0x%04X\n", status.plc_error_code);
        } else {
          std::printf("%s\n", status.message);
        }
      };

      const auto restore_originals = [&]() -> bool {
        if (!backups_valid) {
          return true;
        }
        status = run_batch_write_bits_group(
            client,
            port,
            command_state,
            kHeadDevice,
            std::span<const BitValue>(backup_bits.data(), backup_bits.size()));
        if (!status.ok()) {
          print_status_error("probe-random-write-bits restore failed", status);
          return false;
        }
        return true;
      };

      status = run_batch_read_bits_group(
          client,
          port,
          command_state,
          kHeadDevice,
          static_cast<std::uint16_t>(backup_bits.size()),
          std::span<BitValue>(backup_bits.data(), backup_bits.size()));
      if (!status.ok()) {
        print_status_error("probe-random-write-bits backup failed", status);
        return 1;
      }
      backups_valid = true;

      const std::array<BitValue, 16> dense_pattern {{
          BitValue::On, BitValue::Off, BitValue::On, BitValue::Off,
          BitValue::On, BitValue::Off, BitValue::On, BitValue::Off,
          BitValue::On, BitValue::Off, BitValue::On, BitValue::Off,
          BitValue::On, BitValue::Off, BitValue::On, BitValue::Off,
      }};
      const std::array<std::size_t, 4> sparse_offsets {0U, 5U, 10U, 15U};
      const std::array<BitValue, 4> sparse_values {{
          BitValue::On, BitValue::Off, BitValue::On, BitValue::Off,
      }};
      const std::size_t single_offset = 0U;
      const BitValue single_value = BitValue::On;
      std::array<BitValue, 16> dense_verify {};
      std::array<BitValue, 16> sparse_verify {};
      std::array<BitValue, 16> single_verify {};
      std::array<BitValue, 16> sparse_expected = backup_bits;
      std::array<BitValue, 16> single_expected = backup_bits;
      for (std::size_t index = 0; index < sparse_offsets.size(); ++index) {
        sparse_expected[sparse_offsets[index]] = sparse_values[index];
      }
      single_expected[single_offset] = single_value;

      bool contiguous_ok = false;
      status = run_batch_write_bits_group(
          client,
          port,
          command_state,
          kHeadDevice,
          std::span<const BitValue>(dense_pattern.data(), dense_pattern.size()));
      if (!status.ok()) {
        print_probe_status_line("batch-write-bits", status);
      } else {
        status = run_batch_read_bits_group(
            client,
            port,
            command_state,
            kHeadDevice,
            static_cast<std::uint16_t>(dense_verify.size()),
            std::span<BitValue>(dense_verify.data(), dense_verify.size()));
        if (!status.ok()) {
          print_probe_status_line("batch-write-bits-verify", status);
        } else if (!verify_bits("batch-write-bits", dense_pattern, dense_verify)) {
          std::printf("batch-write-bits=skip verify-mismatch\n");
        } else {
          contiguous_ok = true;
          std::printf("batch-write-bits=ok contiguous\n");
        }
      }

      if (!restore_originals()) {
        return 1;
      }

      std::array<RandomWriteBitItem, 16> dense_items {};
      for (std::size_t index = 0; index < dense_items.size(); ++index) {
        dense_items[index].device = DeviceAddress {.code = DeviceCode::M, .number = 100U + static_cast<std::uint32_t>(index)};
        dense_items[index].value = dense_pattern[index];
      }

      bool random_dense_ok = false;
      status = run_random_write_bits(
          client,
          port,
          command_state,
          std::span<const RandomWriteBitItem>(dense_items.data(), dense_items.size()));
      if (!status.ok()) {
        print_probe_status_line("random-write-bits-dense", status);
      } else {
        status = run_batch_read_bits_group(
            client,
            port,
            command_state,
            kHeadDevice,
            static_cast<std::uint16_t>(dense_verify.size()),
            std::span<BitValue>(dense_verify.data(), dense_verify.size()));
        if (!status.ok()) {
          print_probe_status_line("random-write-bits-dense-verify", status);
        } else if (!verify_bits("random-write-bits-dense", dense_pattern, dense_verify)) {
          std::printf("random-write-bits-dense=skip verify-mismatch\n");
        } else {
          random_dense_ok = true;
          std::printf("random-write-bits-dense=ok native\n");
        }
      }

      if (!restore_originals()) {
        return 1;
      }

      const std::array<RandomWriteBitItem, 1> single_item {{
          {.device = DeviceAddress {.code = DeviceCode::M, .number = 100U + static_cast<std::uint32_t>(single_offset)},
           .value = single_value},
      }};

      bool random_single_ok = false;
      status = run_random_write_bits(
          client,
          port,
          command_state,
          std::span<const RandomWriteBitItem>(single_item.data(), single_item.size()));
      if (!status.ok()) {
        print_probe_status_line("random-write-bits-single", status);
      } else {
        status = run_batch_read_bits_group(
            client,
            port,
            command_state,
            kHeadDevice,
            static_cast<std::uint16_t>(single_verify.size()),
            std::span<BitValue>(single_verify.data(), single_verify.size()));
        if (!status.ok()) {
          print_probe_status_line("random-write-bits-single-verify", status);
        } else if (!verify_bits("random-write-bits-single", single_expected, single_verify)) {
          std::printf("random-write-bits-single=skip verify-mismatch\n");
        } else {
          random_single_ok = true;
          std::printf("random-write-bits-single=ok native\n");
        }
      }

      if (!restore_originals()) {
        return 1;
      }

      std::array<RandomWriteBitItem, 4> sparse_items {};
      for (std::size_t index = 0; index < sparse_items.size(); ++index) {
        sparse_items[index].device = DeviceAddress {
            .code = DeviceCode::M,
            .number = 100U + static_cast<std::uint32_t>(sparse_offsets[index]),
        };
        sparse_items[index].value = sparse_values[index];
      }

      bool random_sparse_ok = false;
      status = run_random_write_bits(
          client,
          port,
          command_state,
          std::span<const RandomWriteBitItem>(sparse_items.data(), sparse_items.size()));
      if (!status.ok()) {
        print_probe_status_line("random-write-bits-sparse", status);
      } else {
        status = run_batch_read_bits_group(
            client,
            port,
            command_state,
            kHeadDevice,
            static_cast<std::uint16_t>(sparse_verify.size()),
            std::span<BitValue>(sparse_verify.data(), sparse_verify.size()));
        if (!status.ok()) {
          print_probe_status_line("random-write-bits-sparse-verify", status);
        } else if (!verify_bits("random-write-bits-sparse", sparse_expected, sparse_verify)) {
          std::printf("random-write-bits-sparse=skip verify-mismatch\n");
        } else {
          random_sparse_ok = true;
          std::printf("random-write-bits-sparse=ok native\n");
        }
      }

      if (!restore_originals()) {
        return 1;
      }

      std::array<BitValue, 16> restore_verify {};
      status = run_batch_read_bits_group(
          client,
          port,
          command_state,
          kHeadDevice,
          static_cast<std::uint16_t>(restore_verify.size()),
          std::span<BitValue>(restore_verify.data(), restore_verify.size()));
      if (!status.ok()) {
        print_status_error("probe-random-write-bits restore verify failed", status);
        return 1;
      }
      if (!verify_bits("probe-random-write-bits-restore", backup_bits, restore_verify)) {
        return 1;
      }

      std::printf("probe-random-write-bits: contiguous=%s single=%s dense=%s sparse=%s restore=ok\n",
                  contiguous_ok ? "contiguous" : "skip",
                  random_single_ok ? "native" : "skip",
                  random_dense_ok ? "native" : "skip",
                  random_sparse_ok ? "native" : "skip");
      return (random_single_ok || random_dense_ok || random_sparse_ok) ? 0 : 1;
    }

    case CommandKind::ProbeMultiBlock: {
      ProbeMultiBlockMode probe_mode = ProbeMultiBlockMode::Mixed;
      if (options.command_argc == 1 &&
          !parse_probe_multi_block_mode(std::string_view(options.command_argv[0]), probe_mode)) {
        std::fprintf(
            stderr,
            "probe-multi-block mode must be 'mixed', 'word-only', 'bit-only', 'word-a', 'word-b', 'bit-a', or 'bit-b'\n");
        return 1;
      }

      constexpr DeviceAddress kWordBlockADevice {.code = mcprotocol::serial::DeviceCode::D, .number = 100};
      constexpr DeviceAddress kWordBlockBDevice {.code = mcprotocol::serial::DeviceCode::D, .number = 110};
      constexpr DeviceAddress kBitBlockADevice {.code = mcprotocol::serial::DeviceCode::M, .number = 100};
      constexpr DeviceAddress kBitBlockBDevice {.code = mcprotocol::serial::DeviceCode::M, .number = 200};

      const std::string_view mode_name = probe_multi_block_mode_name(probe_mode);
      const bool verify_word_a = probe_mode == ProbeMultiBlockMode::Mixed ||
                                 probe_mode == ProbeMultiBlockMode::WordOnly ||
                                 probe_mode == ProbeMultiBlockMode::WordA;
      const bool verify_word_b = probe_mode == ProbeMultiBlockMode::Mixed ||
                                 probe_mode == ProbeMultiBlockMode::WordOnly ||
                                 probe_mode == ProbeMultiBlockMode::WordB;
      const bool verify_bit_a = probe_mode == ProbeMultiBlockMode::Mixed ||
                                probe_mode == ProbeMultiBlockMode::BitOnly ||
                                probe_mode == ProbeMultiBlockMode::BitA;
      const bool verify_bit_b = probe_mode == ProbeMultiBlockMode::Mixed ||
                                probe_mode == ProbeMultiBlockMode::BitOnly ||
                                probe_mode == ProbeMultiBlockMode::BitB;
      const bool include_words = verify_word_a || verify_word_b;
      const bool include_bits = verify_bit_a || verify_bit_b;

      std::array<std::uint16_t, 2> backup_word_block_a {};
      std::array<std::uint16_t, 3> backup_word_block_b {};
      std::array<BitValue, 16> backup_bit_block_a {};
      std::array<BitValue, 32> backup_bit_block_b {};
      bool backups_valid = false;

      const auto verify_word_block = [](std::string_view context,
                                        std::string_view label,
                                        std::span<const std::uint16_t> expected,
                                        std::span<const std::uint16_t> actual) -> bool {
        for (std::size_t index = 0; index < expected.size(); ++index) {
          if (actual[index] != expected[index]) {
            std::printf("%.*s-mismatch=%.*s[%zu] expected=0x%04X read=0x%04X\n",
                        static_cast<int>(context.size()),
                        context.data(),
                        static_cast<int>(label.size()),
                        label.data(),
                        index,
                        expected[index],
                        actual[index]);
            return false;
          }
        }
        return true;
      };
      const auto verify_bit_block = [](std::string_view context,
                                       std::string_view label,
                                       std::span<const BitValue> expected,
                                       std::span<const BitValue> actual) -> bool {
        const auto pack_word = [](std::span<const BitValue> bits) -> std::uint16_t {
          std::uint16_t value = 0;
          for (std::size_t bit = 0; bit < bits.size() && bit < 16U; ++bit) {
            if (bits[bit] == BitValue::On) {
              value = static_cast<std::uint16_t>(value | (1U << (15U - bit)));
            }
          }
          return value;
        };
        for (std::size_t index = 0; index < expected.size(); ++index) {
          if (actual[index] != expected[index]) {
            std::printf("%.*s-mismatch=%.*s[%zu] expected=%u read=%u",
                        static_cast<int>(context.size()),
                        context.data(),
                        static_cast<int>(label.size()),
                        label.data(),
                        index,
                        expected[index] == BitValue::On ? 1U : 0U,
                        actual[index] == BitValue::On ? 1U : 0U);
            if ((expected.size() % 16U) == 0U) {
              const std::size_t word_index = index / 16U;
              const auto expected_word = pack_word(expected.subspan(word_index * 16U, 16U));
              const auto actual_word = pack_word(actual.subspan(word_index * 16U, 16U));
              std::printf(" expected-word[%zu]=0x%04X read-word[%zu]=0x%04X",
                          word_index,
                          expected_word,
                          word_index,
                          actual_word);
            }
            std::printf("\n");
            return false;
          }
        }
        return true;
      };

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

      const std::array<MultiBlockReadBlock, 2> read_word_blocks {{
          {.head_device = kWordBlockADevice, .points = 2, .bit_block = false},
          {.head_device = kWordBlockBDevice, .points = 3, .bit_block = false},
      }};
      const std::array<MultiBlockReadBlock, 2> read_bit_blocks {{
          {.head_device = kBitBlockADevice, .points = 1, .bit_block = true},
          {.head_device = kBitBlockBDevice, .points = 2, .bit_block = true},
      }};
      const std::array<MultiBlockReadBlock, 4> read_mixed_blocks {{
          {.head_device = kWordBlockADevice, .points = 2, .bit_block = false},
          {.head_device = kWordBlockBDevice, .points = 3, .bit_block = false},
          {.head_device = kBitBlockADevice, .points = 1, .bit_block = true},
          {.head_device = kBitBlockBDevice, .points = 2, .bit_block = true},
      }};
      const std::array<MultiBlockReadBlock, 1> read_word_block_a_only {{
          {.head_device = kWordBlockADevice, .points = 2, .bit_block = false},
      }};
      const std::array<MultiBlockReadBlock, 1> read_word_block_b_only {{
          {.head_device = kWordBlockBDevice, .points = 3, .bit_block = false},
      }};
      const std::array<MultiBlockReadBlock, 1> read_bit_block_a_only {{
          {.head_device = kBitBlockADevice, .points = 1, .bit_block = true},
      }};
      const std::array<MultiBlockReadBlock, 1> read_bit_block_b_only {{
          {.head_device = kBitBlockBDevice, .points = 2, .bit_block = true},
      }};
      const std::span<const MultiBlockReadBlock> selected_read_blocks =
          probe_mode == ProbeMultiBlockMode::WordOnly
              ? std::span<const MultiBlockReadBlock>(read_word_blocks.data(), read_word_blocks.size())
              : (probe_mode == ProbeMultiBlockMode::BitOnly
                     ? std::span<const MultiBlockReadBlock>(read_bit_blocks.data(), read_bit_blocks.size())
                     : (probe_mode == ProbeMultiBlockMode::WordA
                            ? std::span<const MultiBlockReadBlock>(read_word_block_a_only.data(), read_word_block_a_only.size())
                            : (probe_mode == ProbeMultiBlockMode::WordB
                                   ? std::span<const MultiBlockReadBlock>(read_word_block_b_only.data(), read_word_block_b_only.size())
                                   : (probe_mode == ProbeMultiBlockMode::BitA
                                          ? std::span<const MultiBlockReadBlock>(read_bit_block_a_only.data(),
                                                                                  read_bit_block_a_only.size())
                                          : (probe_mode == ProbeMultiBlockMode::BitB
                                                 ? std::span<const MultiBlockReadBlock>(read_bit_block_b_only.data(),
                                                                                         read_bit_block_b_only.size())
                                                 : std::span<const MultiBlockReadBlock>(read_mixed_blocks.data(),
                                                                                         read_mixed_blocks.size()))))));

      std::array<std::uint16_t, 5> multi_read_words {};
      std::array<BitValue, 48> multi_read_bits {};
      std::array<MultiBlockReadBlockResult, 4> multi_read_results {};
      bool multi_read_ok = false;

      status = run_multi_block_read(
          client,
          port,
          command_state,
          MultiBlockReadRequest {
              .blocks = selected_read_blocks,
          },
          std::span<std::uint16_t>(multi_read_words.data(), include_words ? multi_read_words.size() : 0U),
          std::span<BitValue>(multi_read_bits.data(), include_bits ? multi_read_bits.size() : 0U),
          std::span<MultiBlockReadBlockResult>(multi_read_results.data(), selected_read_blocks.size()));
      if (!status.ok()) {
        std::printf("multi-block-read=skip ");
        if (status.code == StatusCode::PlcError) {
          std::printf("0x%04X\n", status.plc_error_code);
        } else {
          std::printf("%s\n", status.message);
        }
      } else {
        bool matches_backup = true;
        std::size_t word_cursor = 0;
        std::size_t bit_cursor = 0;
        if (verify_word_a) {
          matches_backup = verify_word_block(
              "multi-block-read",
              "words-a",
              backup_word_block_a,
              std::span<const std::uint16_t>(multi_read_words.data() + word_cursor, backup_word_block_a.size()));
          word_cursor += backup_word_block_a.size();
        }
        if (matches_backup && verify_word_b) {
          matches_backup = verify_word_block(
              "multi-block-read",
              "words-b",
              backup_word_block_b,
              std::span<const std::uint16_t>(multi_read_words.data() + word_cursor,
                                             backup_word_block_b.size()));
          word_cursor += backup_word_block_b.size();
        }
        if (matches_backup && verify_bit_a) {
          matches_backup = verify_bit_block(
              "multi-block-read",
              "bits-a",
              backup_bit_block_a,
              std::span<const BitValue>(multi_read_bits.data() + bit_cursor, backup_bit_block_a.size()));
          bit_cursor += backup_bit_block_a.size();
        }
        if (matches_backup && verify_bit_b) {
          matches_backup = verify_bit_block(
              "multi-block-read",
              "bits-b",
              backup_bit_block_b,
              std::span<const BitValue>(multi_read_bits.data() + bit_cursor, backup_bit_block_b.size()));
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
          BitValue::Off, BitValue::Off, BitValue::Off, BitValue::Off,
          BitValue::Off, BitValue::Off, BitValue::Off, BitValue::Off,
          BitValue::Off, BitValue::Off, BitValue::Off, BitValue::Off,
          BitValue::Off, BitValue::Off, BitValue::Off, BitValue::Off,
          BitValue::Off, BitValue::Off, BitValue::Off, BitValue::On,
          BitValue::Off, BitValue::Off, BitValue::On, BitValue::Off,
          BitValue::Off, BitValue::Off, BitValue::On, BitValue::On,
          BitValue::Off, BitValue::On, BitValue::Off, BitValue::Off,
      }};
      const std::array<MultiBlockWriteBlock, 2> write_word_blocks {{
          {.head_device = kWordBlockADevice,
           .points = static_cast<std::uint16_t>(test_word_block_a.size()),
           .bit_block = false,
           .words = std::span<const std::uint16_t>(test_word_block_a.data(), test_word_block_a.size())},
          {.head_device = kWordBlockBDevice,
           .points = static_cast<std::uint16_t>(test_word_block_b.size()),
           .bit_block = false,
           .words = std::span<const std::uint16_t>(test_word_block_b.data(), test_word_block_b.size())},
      }};
      const std::array<MultiBlockWriteBlock, 2> write_bit_blocks {{
          {.head_device = kBitBlockADevice,
           .points = 1,
           .bit_block = true,
           .bits = std::span<const BitValue>(test_bit_block_a.data(), test_bit_block_a.size())},
          {.head_device = kBitBlockBDevice,
           .points = 2,
           .bit_block = true,
           .bits = std::span<const BitValue>(test_bit_block_b.data(), test_bit_block_b.size())},
      }};
      const std::array<MultiBlockWriteBlock, 4> write_mixed_blocks {{
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
      const std::array<MultiBlockWriteBlock, 1> write_word_block_a_only {{
          {.head_device = kWordBlockADevice,
           .points = static_cast<std::uint16_t>(test_word_block_a.size()),
           .bit_block = false,
           .words = std::span<const std::uint16_t>(test_word_block_a.data(), test_word_block_a.size())},
      }};
      const std::array<MultiBlockWriteBlock, 1> write_word_block_b_only {{
          {.head_device = kWordBlockBDevice,
           .points = static_cast<std::uint16_t>(test_word_block_b.size()),
           .bit_block = false,
           .words = std::span<const std::uint16_t>(test_word_block_b.data(), test_word_block_b.size())},
      }};
      const std::array<MultiBlockWriteBlock, 1> write_bit_block_a_only {{
          {.head_device = kBitBlockADevice,
           .points = 1,
           .bit_block = true,
           .bits = std::span<const BitValue>(test_bit_block_a.data(), test_bit_block_a.size())},
      }};
      const std::array<MultiBlockWriteBlock, 1> write_bit_block_b_only {{
          {.head_device = kBitBlockBDevice,
           .points = 2,
           .bit_block = true,
           .bits = std::span<const BitValue>(test_bit_block_b.data(), test_bit_block_b.size())},
      }};
      const std::span<const MultiBlockWriteBlock> selected_write_blocks =
          probe_mode == ProbeMultiBlockMode::WordOnly
              ? std::span<const MultiBlockWriteBlock>(write_word_blocks.data(), write_word_blocks.size())
              : (probe_mode == ProbeMultiBlockMode::BitOnly
                     ? std::span<const MultiBlockWriteBlock>(write_bit_blocks.data(), write_bit_blocks.size())
                     : (probe_mode == ProbeMultiBlockMode::WordA
                            ? std::span<const MultiBlockWriteBlock>(write_word_block_a_only.data(),
                                                                    write_word_block_a_only.size())
                            : (probe_mode == ProbeMultiBlockMode::WordB
                                   ? std::span<const MultiBlockWriteBlock>(write_word_block_b_only.data(),
                                                                           write_word_block_b_only.size())
                                   : (probe_mode == ProbeMultiBlockMode::BitA
                                          ? std::span<const MultiBlockWriteBlock>(write_bit_block_a_only.data(),
                                                                                  write_bit_block_a_only.size())
                                          : (probe_mode == ProbeMultiBlockMode::BitB
                                                 ? std::span<const MultiBlockWriteBlock>(write_bit_block_b_only.data(),
                                                                                        write_bit_block_b_only.size())
                                                 : std::span<const MultiBlockWriteBlock>(write_mixed_blocks.data(),
                                                                                         write_mixed_blocks.size()))))));

      bool multi_write_ok = false;
      status = run_multi_block_write(
          client,
          port,
          command_state,
          MultiBlockWriteRequest {
              .blocks = selected_write_blocks,
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

        if (include_words) {
          status = run_batch_read_words_group(
              client,
              port,
              command_state,
              kWordBlockADevice,
              static_cast<std::uint16_t>(verify_word_block_a.size()),
              std::span<std::uint16_t>(verify_word_block_a.data(), verify_word_block_a.size()));
        }
        if (status.ok() && include_words) {
          status = run_batch_read_words_group(
              client,
              port,
              command_state,
              kWordBlockBDevice,
              static_cast<std::uint16_t>(verify_word_block_b.size()),
              std::span<std::uint16_t>(verify_word_block_b.data(), verify_word_block_b.size()));
        }
        if (status.ok() && include_bits) {
          status = run_batch_read_bits_group(
              client,
              port,
              command_state,
              kBitBlockADevice,
              static_cast<std::uint16_t>(verify_bit_block_a.size()),
              std::span<BitValue>(verify_bit_block_a.data(), verify_bit_block_a.size()));
        }
        if (status.ok() && include_bits) {
          status = run_batch_read_bits_group(
              client,
              port,
              command_state,
              kBitBlockBDevice,
              static_cast<std::uint16_t>(verify_bit_block_b.size()),
              std::span<BitValue>(verify_bit_block_b.data(), verify_bit_block_b.size()));
        }

        bool matches_test = status.ok();
        if (matches_test && verify_word_a) {
          matches_test =
              verify_word_block("multi-block-write", "words-a", test_word_block_a, verify_word_block_a);
        }
        if (matches_test && verify_word_b) {
          matches_test =
              verify_word_block("multi-block-write", "words-b", test_word_block_b, verify_word_block_b);
        }
        if (matches_test && verify_bit_a) {
          matches_test =
              verify_bit_block("multi-block-write", "bits-a", test_bit_block_a, verify_bit_block_a);
        }
        if (matches_test && verify_bit_b) {
          matches_test =
              verify_bit_block("multi-block-write", "bits-b", test_bit_block_b, verify_bit_block_b);
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
      if (!verify_word_block("probe-multi-block-restore", "words-a", backup_word_block_a, restore_word_block_a) ||
          !verify_word_block("probe-multi-block-restore", "words-b", backup_word_block_b, restore_word_block_b) ||
          !verify_bit_block("probe-multi-block-restore", "bits-a", backup_bit_block_a, restore_bit_block_a) ||
          !verify_bit_block("probe-multi-block-restore", "bits-b", backup_bit_block_b, restore_bit_block_b)) {
        return 1;
      }

      std::printf("probe-multi-block[%.*s]: read=%s write=%s restore=ok\n",
                  static_cast<int>(mode_name.size()),
                  mode_name.data(),
                  multi_read_ok ? "native" : "skip",
                  multi_write_ok ? "native" : "skip");
      return (multi_read_ok || multi_write_ok) ? 0 : 1;
    }

    case CommandKind::ProbeMonitor: {
      const std::string_view probe_mode =
          options.command_argc == 0 ? std::string_view {} : std::string_view(options.command_argv[0]);
      if (!probe_mode.empty() && !equals_ignore_case(probe_mode, "read-only") &&
          !equals_ignore_case(probe_mode, "read")) {
        std::fprintf(stderr, "probe-monitor mode must be omitted or 'read-only'\n");
        return 2;
      }
      if (!probe_mode.empty()) {
        mcprotocol::serial::RawResponseFrame frame {};
        status = run_read_monitor_raw(options.protocol, port, options.rts_toggle, frame, options.dump_frames);
        if (!status.ok()) {
          std::printf("probe-monitor[read-only]: skip ");
          if (status.code == StatusCode::PlcError) {
            std::printf("0x%04X\n", status.plc_error_code);
          } else {
            std::printf("%s\n", status.message);
          }
          return 1;
        }

        const char* response_kind = frame.type == mcprotocol::serial::ResponseType::SuccessData ? "data" : "nodata";
        std::printf("probe-monitor[read-only]=ok response=%s bytes=%zu\n", response_kind, frame.response_size);
        return 0;
      }

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
        const bool bit_item = is_bit_device(items[index].device.code);
        const std::uint32_t actual_value =
            bit_item ? (mcprotocol::serial::sparse_native_requested_bit_value(monitor_values[index]) == BitValue::On ? 1U : 0U)
                     : monitor_values[index];
        if (actual_value != expected_values[index]) {
          if (bit_item) {
            std::printf("probe-monitor: skip verify-mismatch index=%zu expected=%u read=%u raw=0x%04X\n",
                        index,
                        static_cast<unsigned>(expected_values[index]),
                        static_cast<unsigned>(actual_value),
                        static_cast<unsigned>(mcprotocol::serial::sparse_native_mask_word(monitor_values[index])));
          } else {
            std::printf("probe-monitor: skip verify-mismatch index=%zu expected=%u read=%u\n",
                        index,
                        static_cast<unsigned>(expected_values[index]),
                        static_cast<unsigned>(actual_value));
          }
          return 1;
        }
      }

      std::printf("probe-monitor=ok mode=native D100=%u D105=%u M100=%u(raw=0x%04X) M105=%u(raw=0x%04X)\n",
                  static_cast<unsigned>(monitor_values[0]),
                  static_cast<unsigned>(monitor_values[1]),
                  mcprotocol::serial::sparse_native_requested_bit_value(monitor_values[2]) == BitValue::On ? 1U : 0U,
                  static_cast<unsigned>(mcprotocol::serial::sparse_native_mask_word(monitor_values[2])),
                  mcprotocol::serial::sparse_native_requested_bit_value(monitor_values[3]) == BitValue::On ? 1U : 0U,
                  static_cast<unsigned>(mcprotocol::serial::sparse_native_mask_word(monitor_values[3])));
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
      constexpr std::uint16_t kProbeAddressLimit = 16U;
      Status first_skip_status {};
      bool recorded_skip_status = false;
      std::uint16_t first_mismatch_address = 0U;
      std::uint16_t first_mismatch_expected = 0U;
      std::uint16_t first_mismatch_read = 0U;
      bool recorded_mismatch = false;

      for (std::uint16_t start_address = 0U; start_address < kProbeAddressLimit; ++start_address) {
        std::array<std::uint16_t, 1> original {};
        status = run_read_host_buffer(
            client,
            port,
            command_state,
            HostBufferReadRequest {.start_address = start_address, .word_length = 1U},
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
                .start_address = start_address,
                .words = std::span<const std::uint16_t>(test_value.data(), test_value.size()),
            });
        if (!status.ok()) {
          if (!recorded_skip_status) {
            first_skip_status = status;
            recorded_skip_status = true;
          }
          continue;
        }

        std::array<std::uint16_t, 1> verify {};
        status = run_read_host_buffer(
            client,
            port,
            command_state,
            HostBufferReadRequest {.start_address = start_address, .word_length = 1U},
            std::span<std::uint16_t>(verify.data(), verify.size()));
        if (!status.ok()) {
          print_status_error("probe-write-host-buffer verify failed", status);
          return 1;
        }
        if (verify[0] != test_value[0]) {
          if (!recorded_mismatch) {
            first_mismatch_address = start_address;
            first_mismatch_expected = test_value[0];
            first_mismatch_read = verify[0];
            recorded_mismatch = true;
          }
          continue;
        }

        status = run_write_host_buffer(
            client,
            port,
            command_state,
            HostBufferWriteRequest {
                .start_address = start_address,
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
            HostBufferReadRequest {.start_address = start_address, .word_length = 1U},
            std::span<std::uint16_t>(restored.data(), restored.size()));
        if (!status.ok()) {
          print_status_error("probe-write-host-buffer re-read failed", status);
          return 1;
        }
        if (restored[0] != original[0]) {
          std::printf(
              "probe-write-host-buffer restore-mismatch start=%u expected=0x%04X read=0x%04X\n",
              static_cast<unsigned int>(start_address),
              original[0],
              restored[0]);
          return 1;
        }

        std::printf("probe-write-host-buffer=ok start=%u 0x%04X->0x%04X->0x%04X\n",
                    static_cast<unsigned int>(start_address),
                    original[0],
                    test_value[0],
                    restored[0]);
        return 0;
      }

      std::printf("probe-write-host-buffer: skip ");
      if (recorded_mismatch) {
        std::printf("verify-mismatch start=%u expected=0x%04X read=0x%04X\n",
                    static_cast<unsigned int>(first_mismatch_address),
                    first_mismatch_expected,
                    first_mismatch_read);
      } else if (recorded_skip_status) {
        if (first_skip_status.code == StatusCode::PlcError) {
          std::printf("0x%04X\n", first_skip_status.plc_error_code);
        } else {
          std::printf("%s\n", first_skip_status.message);
        }
      } else {
        std::printf("no writable host-buffer word found in range 0..%u\n",
                    static_cast<unsigned int>(kProbeAddressLimit - 1U));
      }
      return 1;
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
        std::printf("U%X\\%s%u=0x%04X %u\n",
                    device.module_number,
                    kind,
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
        std::printf("U%X\\%s%u=0x%04X %u\n",
                    device.module_number,
                    kind,
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

    case CommandKind::ReadFileRegisterWords: {
      ExtendedFileRegisterAddress address {};
      const std::string_view address_arg(options.command_argv[0]);
      const std::string_view points_arg(options.command_argv[1]);
      if (!parse_extended_file_register_address(address_arg, options.protocol, address)) {
        std::fprintf(
            stderr,
            "Invalid file-register address: %.*s\n",
            static_cast<int>(address_arg.size()),
            address_arg.data());
        return 2;
      }
      const std::size_t max_points = cli_max_extended_file_register_word_points(options.protocol);
      std::uint32_t points = 0;
      if (!parse_u32(points_arg, points) || points == 0U || points > max_points) {
        std::fprintf(stderr, "Invalid file-register word count: %.*s\n",
                     static_cast<int>(points_arg.size()),
                     points_arg.data());
        return 2;
      }

      std::array<std::uint16_t, kCliMaxExtendedFileRegisterWordPoints> words {};
      status = run_read_extended_file_register_words(
          client,
          port,
          command_state,
          ExtendedFileRegisterBatchReadWordsRequest {
              .head_device = address,
              .points = static_cast<std::uint16_t>(points),
          },
          std::span<std::uint16_t>(words.data(), points));
      if (!status.ok()) {
        print_status_error("read-file-register request failed", status);
        return 1;
      }
      for (std::uint32_t index = 0; index < points; ++index) {
        std::printf("[%u] 0x%04X %u\n", index, words[index], words[index]);
      }
      return 0;
    }

    case CommandKind::ReadFileRegisterWordsDirect: {
      const std::string_view head_arg(options.command_argv[0]);
      const std::string_view points_arg(options.command_argv[1]);
      const std::uint32_t max_head_device_number =
          cli_max_direct_extended_file_register_head_device_number(options.protocol);
      std::uint32_t head_device_number = 0;
      if (!parse_u32(head_arg, head_device_number) || head_device_number > max_head_device_number) {
        std::fprintf(
            stderr,
            "Invalid direct file-register head device number: %.*s\n",
            static_cast<int>(head_arg.size()),
            head_arg.data());
        return 2;
      }
      const std::size_t max_points = cli_max_extended_file_register_word_points(options.protocol);
      std::uint32_t points = 0;
      if (!parse_u32(points_arg, points) || points == 0U || points > max_points) {
        std::fprintf(stderr, "Invalid direct file-register word count: %.*s\n",
                     static_cast<int>(points_arg.size()),
                     points_arg.data());
        return 2;
      }

      std::array<std::uint16_t, kCliMaxExtendedFileRegisterWordPoints> words {};
      status = run_direct_read_extended_file_register_words(
          client,
          port,
          command_state,
          ExtendedFileRegisterDirectBatchReadWordsRequest {
              .head_device_number = head_device_number,
              .points = static_cast<std::uint16_t>(points),
          },
          std::span<std::uint16_t>(words.data(), points));
      if (!status.ok()) {
        print_status_error("read-file-register-direct request failed", status);
        return 1;
      }
      for (std::uint32_t index = 0; index < points; ++index) {
        std::printf("[%u] 0x%04X %u\n", index, words[index], words[index]);
      }
      return 0;
    }

    case CommandKind::ReadLinkDirectWords: {
      LinkDirectDevice device {};
      const std::string_view device_arg(options.command_argv[0]);
      const std::string_view points_arg(options.command_argv[1]);
      status = parse_link_direct_device(device_arg, device);
      if (!status.ok()) {
        print_status_error("Invalid link direct device", status);
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
      status = run_link_direct_batch_read_words(
          client,
          port,
          command_state,
          device,
          static_cast<std::uint16_t>(points),
          std::span<std::uint16_t>(words.data(), points));
      if (!status.ok()) {
        print_status_error("read-link-direct-words request failed", status);
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

    case CommandKind::ReadLinkDirectBits: {
      LinkDirectDevice device {};
      const std::string_view device_arg(options.command_argv[0]);
      const std::string_view points_arg(options.command_argv[1]);
      status = parse_link_direct_device(device_arg, device);
      if (!status.ok()) {
        print_status_error("Invalid link direct device", status);
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
      status = run_link_direct_batch_read_bits(
          client,
          port,
          command_state,
          device,
          static_cast<std::uint16_t>(points),
          std::span<BitValue>(bits.data(), points));
      if (!status.ok()) {
        print_status_error("read-link-direct-bits request failed", status);
        return 1;
      }
      for (std::uint32_t index = 0; index < points; ++index) {
        std::printf("[%u] %u\n", index, bits[index] == BitValue::On ? 1U : 0U);
      }
      return 0;
    }

    case CommandKind::WriteLinkDirectWords: {
      LinkDirectDevice device {};
      const std::string_view device_arg(options.command_argv[0]);
      status = parse_link_direct_device(device_arg, device);
      if (!status.ok()) {
        print_status_error("Invalid link direct device", status);
        return 2;
      }

      const int word_count = options.command_argc - 1;
      if (word_count < 1 || word_count > 960) {
        std::fprintf(stderr, "Invalid link direct word count\n");
        return 2;
      }

      std::array<std::uint16_t, 960> words {};
      for (int index = 0; index < word_count; ++index) {
        std::uint32_t value = 0;
        if (!parse_u32_auto(options.command_argv[index + 1], value) || value > 0xFFFFU) {
          std::fprintf(stderr, "Invalid write-link-direct-words value: %s\n", options.command_argv[index + 1]);
          return 2;
        }
        words[static_cast<std::size_t>(index)] = static_cast<std::uint16_t>(value);
      }

      status = run_link_direct_batch_write_words(
          client,
          port,
          command_state,
          device,
          std::span<const std::uint16_t>(words.data(), static_cast<std::size_t>(word_count)));
      if (!status.ok()) {
        print_status_error("write-link-direct-words request failed", status);
        return 1;
      }
      std::printf("write-link-direct-words=ok words=%d\n", word_count);
      return 0;
    }

    case CommandKind::WriteLinkDirectBits: {
      LinkDirectDevice device {};
      const std::string_view device_arg(options.command_argv[0]);
      status = parse_link_direct_device(device_arg, device);
      if (!status.ok()) {
        print_status_error("Invalid link direct device", status);
        return 2;
      }

      const int bit_count = options.command_argc - 1;
      if (bit_count < 1 || bit_count > 7904) {
        std::fprintf(stderr, "Invalid link direct bit count\n");
        return 2;
      }

      std::array<BitValue, 7904> bits {};
      for (int index = 0; index < bit_count; ++index) {
        const std::string_view value_arg(options.command_argv[index + 1]);
        if (value_arg == "0") {
          bits[static_cast<std::size_t>(index)] = BitValue::Off;
        } else if (value_arg == "1") {
          bits[static_cast<std::size_t>(index)] = BitValue::On;
        } else {
          std::fprintf(stderr, "Invalid write-link-direct-bits value: %s\n", options.command_argv[index + 1]);
          return 2;
        }
      }

      status = run_link_direct_batch_write_bits(
          client,
          port,
          command_state,
          device,
          std::span<const BitValue>(bits.data(), static_cast<std::size_t>(bit_count)));
      if (!status.ok()) {
        print_status_error("write-link-direct-bits request failed", status);
        return 1;
      }
      std::printf("write-link-direct-bits=ok bits=%d\n", bit_count);
      return 0;
    }

    case CommandKind::RandomReadLinkDirect: {
      if (options.command_argc > static_cast<int>(mcprotocol::serial::kMaxRandomAccessItems)) {
        std::fprintf(stderr,
                     "Too many random-read-link-direct items; max is %zu\n",
                     mcprotocol::serial::kMaxRandomAccessItems);
        return 2;
      }

      std::array<LinkDirectRandomReadItem, mcprotocol::serial::kMaxRandomAccessItems> items {};
      for (int index = 0; index < options.command_argc; ++index) {
        if (!parse_link_direct_random_read_item(options.command_argv[index], items[static_cast<std::size_t>(index)])) {
          std::fprintf(stderr, "Invalid random-read-link-direct item: %s\n", options.command_argv[index]);
          return 2;
        }
      }

      std::array<std::uint32_t, mcprotocol::serial::kMaxRandomAccessItems> values {};
      status = run_link_direct_random_read(
          client,
          port,
          command_state,
          std::span<const LinkDirectRandomReadItem>(items.data(), static_cast<std::size_t>(options.command_argc)),
          std::span<std::uint32_t>(values.data(), static_cast<std::size_t>(options.command_argc)));
      if (!status.ok()) {
        print_status_error("random-read-link-direct request failed", status);
        return 1;
      }
      for (int index = 0; index < options.command_argc; ++index) {
        const auto value = values[static_cast<std::size_t>(index)];
        if (is_bit_device(items[static_cast<std::size_t>(index)].device.device.code)) {
          print_sparse_native_bit_value(options.command_argv[index], value);
        } else {
          const unsigned word = static_cast<unsigned>(value & 0xFFFFU);
          std::printf("%s=0x%04X %u\n", options.command_argv[index], word, word);
        }
      }
      return 0;
    }

    case CommandKind::RandomWriteLinkDirectWords: {
      if (options.command_argc > static_cast<int>(mcprotocol::serial::kMaxRandomAccessItems)) {
        std::fprintf(stderr,
                     "Too many random-write-link-direct-words items; max is %zu\n",
                     mcprotocol::serial::kMaxRandomAccessItems);
        return 2;
      }

      std::array<LinkDirectRandomWriteWordItem, mcprotocol::serial::kMaxRandomAccessItems> items {};
      for (int index = 0; index < options.command_argc; ++index) {
        if (!parse_link_direct_random_write_word_item(
                options.command_argv[index],
                items[static_cast<std::size_t>(index)])) {
          std::fprintf(stderr, "Invalid random-write-link-direct-words item: %s\n", options.command_argv[index]);
          return 2;
        }
      }

      status = run_link_direct_random_write_words(
          client,
          port,
          command_state,
          std::span<const LinkDirectRandomWriteWordItem>(items.data(), static_cast<std::size_t>(options.command_argc)));
      if (!status.ok()) {
        print_status_error("random-write-link-direct-words request failed", status);
        return 1;
      }
      std::printf("random-write-link-direct-words=ok items=%d\n", options.command_argc);
      return 0;
    }

    case CommandKind::RandomWriteLinkDirectBits: {
      constexpr std::size_t kMaxRandomWriteBitItems = 94;
      if (options.command_argc > static_cast<int>(kMaxRandomWriteBitItems)) {
        std::fprintf(stderr,
                     "Too many random-write-link-direct-bits items; max is %zu\n",
                     kMaxRandomWriteBitItems);
        return 2;
      }

      std::array<LinkDirectRandomWriteBitItem, kMaxRandomWriteBitItems> items {};
      for (int index = 0; index < options.command_argc; ++index) {
        if (!parse_link_direct_random_write_bit_item(
                options.command_argv[index],
                items[static_cast<std::size_t>(index)])) {
          std::fprintf(stderr, "Invalid random-write-link-direct-bits item: %s\n", options.command_argv[index]);
          return 2;
        }
      }

      status = run_link_direct_random_write_bits(
          client,
          port,
          command_state,
          std::span<const LinkDirectRandomWriteBitItem>(items.data(), static_cast<std::size_t>(options.command_argc)));
      if (!status.ok()) {
        print_status_error("random-write-link-direct-bits request failed", status);
        return 1;
      }
      std::printf("random-write-link-direct-bits=ok items=%d\n", options.command_argc);
      return 0;
    }

    case CommandKind::MultiBlockReadLinkDirectWords: {
      if (options.command_argc > static_cast<int>(mcprotocol::serial::kMaxMultiBlockCount)) {
        std::fprintf(stderr,
                     "Too many multi-block-read-link-direct-words blocks; max is %zu\n",
                     mcprotocol::serial::kMaxMultiBlockCount);
        return 2;
      }

      std::array<LinkDirectMultiBlockReadBlock, mcprotocol::serial::kMaxMultiBlockCount> blocks {};
      for (int index = 0; index < options.command_argc; ++index) {
        if (!parse_link_direct_multi_block_read_spec(
                options.command_argv[index],
                false,
                blocks[static_cast<std::size_t>(index)])) {
          std::fprintf(stderr, "Invalid multi-block-read-link-direct-words block: %s\n", options.command_argv[index]);
          return 2;
        }
      }

      std::size_t total_words = 0U;
      for (int index = 0; index < options.command_argc; ++index) {
        total_words += blocks[static_cast<std::size_t>(index)].points;
      }
      if (total_words > 960U) {
        std::fprintf(stderr, "multi-block-read-link-direct-words total points exceed 960\n");
        return 2;
      }

      std::array<std::uint16_t, 960> words {};
      std::array<BitValue, 1> bits {};
      std::array<MultiBlockReadBlockResult, mcprotocol::serial::kMaxMultiBlockCount> results {};
      status = run_link_direct_multi_block_read(
          client,
          port,
          command_state,
          LinkDirectMultiBlockReadRequest {
              .blocks = std::span<const LinkDirectMultiBlockReadBlock>(
                  blocks.data(),
                  static_cast<std::size_t>(options.command_argc)),
          },
          std::span<std::uint16_t>(words.data(), total_words),
          std::span<BitValue>(bits.data(), bits.size()),
          std::span<MultiBlockReadBlockResult>(results.data(), static_cast<std::size_t>(options.command_argc)));
      if (!status.ok()) {
        print_status_error("multi-block-read-link-direct-words request failed", status);
        return 1;
      }

      for (int block_index = 0; block_index < options.command_argc; ++block_index) {
        const auto& result = results[static_cast<std::size_t>(block_index)];
        for (std::uint16_t point = 0; point < result.data_count; ++point) {
          const auto value = words[static_cast<std::size_t>(result.data_offset + point)];
          std::printf("%s[%u]=0x%04X %u\n",
                      options.command_argv[block_index],
                      static_cast<unsigned>(point),
                      value,
                      value);
        }
      }
      return 0;
    }

    case CommandKind::MultiBlockReadLinkDirectBits: {
      if (options.command_argc > static_cast<int>(mcprotocol::serial::kMaxMultiBlockCount)) {
        std::fprintf(stderr,
                     "Too many multi-block-read-link-direct-bits blocks; max is %zu\n",
                     mcprotocol::serial::kMaxMultiBlockCount);
        return 2;
      }

      std::array<LinkDirectMultiBlockReadBlock, mcprotocol::serial::kMaxMultiBlockCount> blocks {};
      for (int index = 0; index < options.command_argc; ++index) {
        if (!parse_link_direct_multi_block_read_spec(
                options.command_argv[index],
                true,
                blocks[static_cast<std::size_t>(index)])) {
          std::fprintf(stderr, "Invalid multi-block-read-link-direct-bits block: %s\n", options.command_argv[index]);
          return 2;
        }
      }

      std::size_t total_bit_values = 0U;
      for (int index = 0; index < options.command_argc; ++index) {
        total_bit_values += static_cast<std::size_t>(blocks[static_cast<std::size_t>(index)].points) * 16U;
      }
      if (total_bit_values > (960U * 16U)) {
        std::fprintf(stderr, "multi-block-read-link-direct-bits total points exceed 960 words\n");
        return 2;
      }

      std::array<std::uint16_t, 1> words {};
      std::array<BitValue, 960U * 16U> bits {};
      std::array<MultiBlockReadBlockResult, mcprotocol::serial::kMaxMultiBlockCount> results {};
      status = run_link_direct_multi_block_read(
          client,
          port,
          command_state,
          LinkDirectMultiBlockReadRequest {
              .blocks = std::span<const LinkDirectMultiBlockReadBlock>(
                  blocks.data(),
                  static_cast<std::size_t>(options.command_argc)),
          },
          std::span<std::uint16_t>(words.data(), words.size()),
          std::span<BitValue>(bits.data(), total_bit_values),
          std::span<MultiBlockReadBlockResult>(results.data(), static_cast<std::size_t>(options.command_argc)));
      if (!status.ok()) {
        print_status_error("multi-block-read-link-direct-bits request failed", status);
        return 1;
      }

      for (int block_index = 0; block_index < options.command_argc; ++block_index) {
        const auto& result = results[static_cast<std::size_t>(block_index)];
        for (std::uint16_t bit_index = 0; bit_index < result.data_count; ++bit_index) {
          std::printf("%s[%u]=%u\n",
                      options.command_argv[block_index],
                      static_cast<unsigned>(bit_index),
                      bits[static_cast<std::size_t>(result.data_offset + bit_index)] == BitValue::On ? 1U : 0U);
        }
      }
      return 0;
    }

    case CommandKind::MultiBlockWriteLinkDirectWords: {
      if (options.command_argc > static_cast<int>(mcprotocol::serial::kMaxMultiBlockCount)) {
        std::fprintf(stderr,
                     "Too many multi-block-write-link-direct-words blocks; max is %zu\n",
                     mcprotocol::serial::kMaxMultiBlockCount);
        return 2;
      }

      std::array<LinkDirectMultiBlockWriteBlock, mcprotocol::serial::kMaxMultiBlockCount> blocks {};
      std::array<std::uint16_t, 960> word_pool {};
      std::size_t word_offset = 0U;
      for (int index = 0; index < options.command_argc; ++index) {
        LinkDirectDevice device {};
        std::size_t count = 0U;
        if (!parse_link_direct_multi_block_write_word_spec(
                options.command_argv[index],
                std::span<std::uint16_t>(word_pool.data() + word_offset, word_pool.size() - word_offset),
                count,
                device)) {
          std::fprintf(stderr, "Invalid multi-block-write-link-direct-words block: %s\n", options.command_argv[index]);
          return 2;
        }
        blocks[static_cast<std::size_t>(index)] = LinkDirectMultiBlockWriteBlock {
            .head_device = device,
            .points = static_cast<std::uint16_t>(count),
            .bit_block = false,
            .words = std::span<const std::uint16_t>(word_pool.data() + word_offset, count),
        };
        word_offset += count;
      }

      status = run_link_direct_multi_block_write(
          client,
          port,
          command_state,
          LinkDirectMultiBlockWriteRequest {
              .blocks = std::span<const LinkDirectMultiBlockWriteBlock>(
                  blocks.data(),
                  static_cast<std::size_t>(options.command_argc)),
          });
      if (!status.ok()) {
        print_status_error("multi-block-write-link-direct-words request failed", status);
        return 1;
      }
      std::printf("multi-block-write-link-direct-words=ok blocks=%d words=%zu\n", options.command_argc, word_offset);
      return 0;
    }

    case CommandKind::MultiBlockWriteLinkDirectBits: {
      if (options.command_argc > static_cast<int>(mcprotocol::serial::kMaxMultiBlockCount)) {
        std::fprintf(stderr,
                     "Too many multi-block-write-link-direct-bits blocks; max is %zu\n",
                     mcprotocol::serial::kMaxMultiBlockCount);
        return 2;
      }

      std::array<LinkDirectMultiBlockWriteBlock, mcprotocol::serial::kMaxMultiBlockCount> blocks {};
      std::array<BitValue, 960U * 16U> bit_pool {};
      std::size_t bit_offset = 0U;
      for (int index = 0; index < options.command_argc; ++index) {
        LinkDirectDevice device {};
        std::size_t count = 0U;
        if (!parse_link_direct_multi_block_write_bit_spec(
                options.command_argv[index],
                std::span<BitValue>(bit_pool.data() + bit_offset, bit_pool.size() - bit_offset),
                count,
                device)) {
          std::fprintf(stderr, "Invalid multi-block-write-link-direct-bits block: %s\n", options.command_argv[index]);
          return 2;
        }
        blocks[static_cast<std::size_t>(index)] = LinkDirectMultiBlockWriteBlock {
            .head_device = device,
            .points = static_cast<std::uint16_t>(count / 16U),
            .bit_block = true,
            .bits = std::span<const BitValue>(bit_pool.data() + bit_offset, count),
        };
        bit_offset += count;
      }

      status = run_link_direct_multi_block_write(
          client,
          port,
          command_state,
          LinkDirectMultiBlockWriteRequest {
              .blocks = std::span<const LinkDirectMultiBlockWriteBlock>(
                  blocks.data(),
                  static_cast<std::size_t>(options.command_argc)),
          });
      if (!status.ok()) {
        print_status_error("multi-block-write-link-direct-bits request failed", status);
        return 1;
      }
      std::printf("multi-block-write-link-direct-bits=ok blocks=%d bits=%zu\n", options.command_argc, bit_offset);
      return 0;
    }

    case CommandKind::MonitorFileRegister: {
      if (options.command_argc > 20) {
        std::fprintf(stderr, "Too many monitor-file-register items; max is 20\n");
        return 2;
      }

      std::array<ExtendedFileRegisterAddress, 20> items {};
      for (int index = 0; index < options.command_argc; ++index) {
        if (!parse_extended_file_register_address(
                options.command_argv[index],
                options.protocol,
                items[static_cast<std::size_t>(index)])) {
          std::fprintf(stderr, "Invalid monitor-file-register item: %s\n", options.command_argv[index]);
          return 2;
        }
      }

      std::array<std::uint16_t, 20> values {};
      status = run_monitor_extended_file_register_words(
          client,
          port,
          command_state,
          ExtendedFileRegisterMonitorRegistration {
              .items = std::span<const ExtendedFileRegisterAddress>(
                  items.data(),
                  static_cast<std::size_t>(options.command_argc)),
          },
          std::span<std::uint16_t>(values.data(), static_cast<std::size_t>(options.command_argc)));
      if (!status.ok()) {
        print_status_error("monitor-file-register request failed", status);
        return 1;
      }

      for (int index = 0; index < options.command_argc; ++index) {
        const auto value = values[static_cast<std::size_t>(index)];
        std::printf("%s=0x%04X %u\n", options.command_argv[index], value, value);
      }
      return 0;
    }

    case CommandKind::MonitorLinkDirect: {
      if (options.command_argc > static_cast<int>(mcprotocol::serial::kMaxMonitorItems)) {
        std::fprintf(stderr, "Too many monitor-link-direct items; max is %zu\n", mcprotocol::serial::kMaxMonitorItems);
        return 2;
      }

      std::array<LinkDirectRandomReadItem, mcprotocol::serial::kMaxMonitorItems> items {};
      for (int index = 0; index < options.command_argc; ++index) {
        if (!parse_link_direct_random_read_item(options.command_argv[index], items[static_cast<std::size_t>(index)])) {
          std::fprintf(stderr, "Invalid monitor-link-direct item: %s\n", options.command_argv[index]);
          return 2;
        }
      }

      status = run_link_direct_register_monitor(
          client,
          port,
          command_state,
          std::span<const LinkDirectRandomReadItem>(items.data(), static_cast<std::size_t>(options.command_argc)));
      if (!status.ok()) {
        print_status_error("monitor-link-direct register failed", status);
        return 1;
      }

      std::array<std::uint32_t, mcprotocol::serial::kMaxMonitorItems> values {};
      status = run_read_monitor(
          client,
          port,
          command_state,
          std::span<std::uint32_t>(values.data(), static_cast<std::size_t>(options.command_argc)));
      if (!status.ok()) {
        print_status_error("monitor-link-direct read failed", status);
        return 1;
      }

      for (int index = 0; index < options.command_argc; ++index) {
        const auto value = values[static_cast<std::size_t>(index)];
        if (is_bit_device(items[static_cast<std::size_t>(index)].device.device.code)) {
          print_sparse_native_bit_value(options.command_argv[index], value);
        } else {
          const unsigned word = static_cast<unsigned>(value & 0xFFFFU);
          std::printf("%s=0x%04X %u\n", options.command_argv[index], word, word);
        }
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
            .double_word = is_double_word_device(device.code),
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
          print_sparse_native_bit_value(options.command_argv[index], value);
        } else if (items[static_cast<std::size_t>(index)].double_word) {
          std::printf("%s=0x%08X %u\n", options.command_argv[index], value, value);
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
        if (!items[static_cast<std::size_t>(index)].double_word &&
            items[static_cast<std::size_t>(index)].value > 0xFFFFU) {
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

    case CommandKind::RandomWriteFileRegisterWords: {
      const std::size_t max_items = cli_max_extended_file_register_random_write_items(options.protocol);
      if (options.command_argc > static_cast<int>(max_items)) {
        std::fprintf(stderr, "Too many random-write-file-register items; max is %zu\n", max_items);
        return 2;
      }

      std::array<ExtendedFileRegisterRandomWriteWordItem, kCliMaxExtendedFileRegisterRandomWriteItems> items {};
      for (int index = 0; index < options.command_argc; ++index) {
        if (!parse_extended_file_register_write_arg(
                options.command_argv[index],
                options.protocol,
                items[static_cast<std::size_t>(index)])) {
          std::fprintf(stderr, "Invalid random-write-file-register item: %s\n", options.command_argv[index]);
          return 2;
        }
      }

      status = run_random_write_extended_file_register_words(
          client,
          port,
          command_state,
          std::span<const ExtendedFileRegisterRandomWriteWordItem>(
              items.data(),
              static_cast<std::size_t>(options.command_argc)));
      if (!status.ok()) {
        print_status_error("random-write-file-register request failed", status);
        return 1;
      }
      std::printf("random-write-file-register=ok\n");
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

    case CommandKind::WriteFileRegisterWords: {
      ExtendedFileRegisterAddress head_address {};
      const std::string_view head_arg(options.command_argv[0]);
      if (!parse_extended_file_register_address(head_arg, options.protocol, head_address)) {
        std::fprintf(
            stderr,
            "Invalid file-register head address: %.*s\n",
            static_cast<int>(head_arg.size()),
            head_arg.data());
        return 2;
      }
      const int word_count = options.command_argc - 1;
      const std::size_t max_points = cli_max_extended_file_register_word_points(options.protocol);
      if (word_count <= 0 || static_cast<std::size_t>(word_count) > max_points) {
        std::fprintf(stderr, "write-file-register word count must be in range 1..%zu\n", max_points);
        return 2;
      }

      std::array<std::uint16_t, kCliMaxExtendedFileRegisterWordPoints> words {};
      for (int index = 0; index < word_count; ++index) {
        std::uint32_t value = 0;
        if (!parse_u32_auto(options.command_argv[index + 1], value) || value > 0xFFFFU) {
          std::fprintf(stderr, "Invalid write-file-register value: %s\n", options.command_argv[index + 1]);
          return 2;
        }
        words[static_cast<std::size_t>(index)] = static_cast<std::uint16_t>(value);
      }

      status = run_write_extended_file_register_words(
          client,
          port,
          command_state,
          ExtendedFileRegisterBatchWriteWordsRequest {
              .head_device = head_address,
              .words = std::span<const std::uint16_t>(
                  words.data(),
                  static_cast<std::size_t>(word_count)),
          });
      if (!status.ok()) {
        print_status_error("write-file-register request failed", status);
        return 1;
      }
      std::printf("write-file-register=ok\n");
      return 0;
    }

    case CommandKind::WriteFileRegisterWordsDirect: {
      const std::string_view head_arg(options.command_argv[0]);
      const std::uint32_t max_head_device_number =
          cli_max_direct_extended_file_register_head_device_number(options.protocol);
      std::uint32_t head_device_number = 0;
      if (!parse_u32(head_arg, head_device_number) || head_device_number > max_head_device_number) {
        std::fprintf(
            stderr,
            "Invalid direct file-register head device number: %.*s\n",
            static_cast<int>(head_arg.size()),
            head_arg.data());
        return 2;
      }
      const int word_count = options.command_argc - 1;
      const std::size_t max_points = cli_max_extended_file_register_word_points(options.protocol);
      if (word_count <= 0 || static_cast<std::size_t>(word_count) > max_points) {
        std::fprintf(stderr, "write-file-register-direct word count must be in range 1..%zu\n", max_points);
        return 2;
      }

      std::array<std::uint16_t, kCliMaxExtendedFileRegisterWordPoints> words {};
      for (int index = 0; index < word_count; ++index) {
        std::uint32_t value = 0;
        if (!parse_u32_auto(options.command_argv[index + 1], value) || value > 0xFFFFU) {
          std::fprintf(stderr, "Invalid write-file-register-direct value: %s\n", options.command_argv[index + 1]);
          return 2;
        }
        words[static_cast<std::size_t>(index)] = static_cast<std::uint16_t>(value);
      }

      status = run_direct_write_extended_file_register_words(
          client,
          port,
          command_state,
          ExtendedFileRegisterDirectBatchWriteWordsRequest {
              .head_device_number = head_device_number,
              .words = std::span<const std::uint16_t>(
                  words.data(),
                  static_cast<std::size_t>(word_count)),
          });
      if (!status.ok()) {
        print_status_error("write-file-register-direct request failed", status);
        return 1;
      }
      std::printf("write-file-register-direct=ok\n");
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
