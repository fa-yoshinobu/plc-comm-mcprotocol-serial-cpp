#pragma once

#include "mcprotocol/serial/compat/array.hpp"
#include "mcprotocol/serial/compat/cstddef.hpp"
#include "mcprotocol/serial/compat/cstdint.hpp"

#include "mcprotocol/serial/span.hpp"
#include "mcprotocol/serial/byte.hpp"

#include "mcprotocol/serial/status.hpp"

/// \file types.hpp
/// \brief Public request, response, configuration, and callback types for the serial MC protocol library.
///
/// This header is the main data-model reference for the public API. It defines:
///
/// - protocol-selection and routing enums
/// - static buffer and feature-limit constants
/// - request/response payload structs used by `MelsecSerialClient`, `HostSyncClient`, and `CommandCodec`
/// - callback and transport hook types used by host and MCU integrations

namespace mcprotocol::serial {

/// \name Compile-time Capacity And Feature Knobs
/// These macros are intended for footprint tuning on MCU builds.
/// @{
#ifndef MCPROTOCOL_SERIAL_MAX_REQUEST_FRAME_BYTES
#define MCPROTOCOL_SERIAL_MAX_REQUEST_FRAME_BYTES 4096U
#endif

#ifndef MCPROTOCOL_SERIAL_MAX_RESPONSE_FRAME_BYTES
#define MCPROTOCOL_SERIAL_MAX_RESPONSE_FRAME_BYTES 4096U
#endif

#ifndef MCPROTOCOL_SERIAL_MAX_REQUEST_DATA_BYTES
#define MCPROTOCOL_SERIAL_MAX_REQUEST_DATA_BYTES 3500U
#endif

#ifndef MCPROTOCOL_SERIAL_MAX_BATCH_WORD_POINTS
#define MCPROTOCOL_SERIAL_MAX_BATCH_WORD_POINTS 960U
#endif

#ifndef MCPROTOCOL_SERIAL_MAX_BATCH_BIT_POINTS_ASCII
#define MCPROTOCOL_SERIAL_MAX_BATCH_BIT_POINTS_ASCII 7904U
#endif

#ifndef MCPROTOCOL_SERIAL_MAX_BATCH_BIT_POINTS_BINARY
#define MCPROTOCOL_SERIAL_MAX_BATCH_BIT_POINTS_BINARY 7904U
#endif

#ifndef MCPROTOCOL_SERIAL_MAX_RANDOM_ACCESS_ITEMS
#define MCPROTOCOL_SERIAL_MAX_RANDOM_ACCESS_ITEMS 192U
#endif

#ifndef MCPROTOCOL_SERIAL_MAX_MULTI_BLOCK_COUNT
#define MCPROTOCOL_SERIAL_MAX_MULTI_BLOCK_COUNT 120U
#endif

#ifndef MCPROTOCOL_SERIAL_MAX_MONITOR_ITEMS
#define MCPROTOCOL_SERIAL_MAX_MONITOR_ITEMS 192U
#endif

#ifndef MCPROTOCOL_SERIAL_MAX_LOOPBACK_BYTES
#define MCPROTOCOL_SERIAL_MAX_LOOPBACK_BYTES 960U
#endif

#ifndef MCPROTOCOL_SERIAL_ENABLE_RANDOM_COMMANDS
#define MCPROTOCOL_SERIAL_ENABLE_RANDOM_COMMANDS 1
#endif

#ifndef MCPROTOCOL_SERIAL_ENABLE_MULTI_BLOCK_COMMANDS
#define MCPROTOCOL_SERIAL_ENABLE_MULTI_BLOCK_COMMANDS 1
#endif

#ifndef MCPROTOCOL_SERIAL_ENABLE_MONITOR_COMMANDS
#define MCPROTOCOL_SERIAL_ENABLE_MONITOR_COMMANDS 1
#endif

#ifndef MCPROTOCOL_SERIAL_ENABLE_HOST_BUFFER_COMMANDS
#define MCPROTOCOL_SERIAL_ENABLE_HOST_BUFFER_COMMANDS 1
#endif

#ifndef MCPROTOCOL_SERIAL_ENABLE_MODULE_BUFFER_COMMANDS
#define MCPROTOCOL_SERIAL_ENABLE_MODULE_BUFFER_COMMANDS 1
#endif

#ifndef MCPROTOCOL_SERIAL_ENABLE_CPU_MODEL_COMMANDS
#define MCPROTOCOL_SERIAL_ENABLE_CPU_MODEL_COMMANDS 1
#endif

#ifndef MCPROTOCOL_SERIAL_ENABLE_LOOPBACK_COMMANDS
#define MCPROTOCOL_SERIAL_ENABLE_LOOPBACK_COMMANDS 1
#endif

#ifndef MCPROTOCOL_SERIAL_ENABLE_ASCII_MODE
#define MCPROTOCOL_SERIAL_ENABLE_ASCII_MODE 1
#endif

#ifndef MCPROTOCOL_SERIAL_ENABLE_BINARY_MODE
#define MCPROTOCOL_SERIAL_ENABLE_BINARY_MODE 1
#endif

#ifndef MCPROTOCOL_SERIAL_ENABLE_FRAME_C4
#define MCPROTOCOL_SERIAL_ENABLE_FRAME_C4 1
#endif

#ifndef MCPROTOCOL_SERIAL_ENABLE_FRAME_C3
#define MCPROTOCOL_SERIAL_ENABLE_FRAME_C3 1
#endif

#ifndef MCPROTOCOL_SERIAL_ENABLE_FRAME_C2
#define MCPROTOCOL_SERIAL_ENABLE_FRAME_C2 1
#endif

#ifndef MCPROTOCOL_SERIAL_ENABLE_FRAME_C1
#define MCPROTOCOL_SERIAL_ENABLE_FRAME_C1 1
#endif

#ifdef MCPROTOCOL_SERIAL_ENABLE_FRAME_E1
#error "MCPROTOCOL_SERIAL_ENABLE_FRAME_E1 was removed with the unsupported MC Serial 1E surface"
#endif
/// @}

constexpr std::size_t kMaxRequestFrameBytes =
    static_cast<std::size_t>(MCPROTOCOL_SERIAL_MAX_REQUEST_FRAME_BYTES);
constexpr std::size_t kMaxResponseFrameBytes =
    static_cast<std::size_t>(MCPROTOCOL_SERIAL_MAX_RESPONSE_FRAME_BYTES);
constexpr std::size_t kMaxRequestDataBytes =
    static_cast<std::size_t>(MCPROTOCOL_SERIAL_MAX_REQUEST_DATA_BYTES);
constexpr std::size_t kMaxBatchWordPoints =
    static_cast<std::size_t>(MCPROTOCOL_SERIAL_MAX_BATCH_WORD_POINTS);
constexpr std::size_t kMaxBatchBitPointsAscii =
    static_cast<std::size_t>(MCPROTOCOL_SERIAL_MAX_BATCH_BIT_POINTS_ASCII);
constexpr std::size_t kMaxBatchBitPointsBinary =
    static_cast<std::size_t>(MCPROTOCOL_SERIAL_MAX_BATCH_BIT_POINTS_BINARY);
constexpr std::size_t kMaxRandomAccessItems =
    static_cast<std::size_t>(MCPROTOCOL_SERIAL_MAX_RANDOM_ACCESS_ITEMS);
constexpr std::size_t kMaxMultiBlockCount =
    static_cast<std::size_t>(MCPROTOCOL_SERIAL_MAX_MULTI_BLOCK_COUNT);
constexpr std::size_t kMaxMonitorItems =
    static_cast<std::size_t>(MCPROTOCOL_SERIAL_MAX_MONITOR_ITEMS);
constexpr std::size_t kMaxLoopbackBytes =
    static_cast<std::size_t>(MCPROTOCOL_SERIAL_MAX_LOOPBACK_BYTES);
constexpr std::size_t kMaxUserFrameRegistrationBytes = 80U;
constexpr std::size_t kCpuModelNameLength = 16;

/// \namespace mcprotocol::serial::module_io
/// \brief Named request-destination module I/O numbers used by `4C` serial routing.
///
/// The CPU constants are useful when a `4C` request is intentionally routed to a multi-CPU or
/// redundant-CPU target. The serial request header accepts the documented request-destination module I/O number
/// field; common CPU values are `0x03D0..0x03D3`, `0x03E0..0x03E3`, and own station `0x03FF`.
/// Remote-head names are provided as vocabulary aliases for parity with the other plc-comm
/// implementations; do not assume a remote-head route is valid on serial hardware unless the
/// selected module, PLC family, and configuration define it.
namespace module_io {

constexpr std::uint16_t ControlSystemCpu = 0x03D0; ///< Control system CPU in a redundant CPU system.
constexpr std::uint16_t StandbySystemCpu = 0x03D1; ///< Standby system CPU in a redundant CPU system.
constexpr std::uint16_t SystemACpu = 0x03D2;       ///< System A CPU in a redundant CPU system.
constexpr std::uint16_t SystemBCpu = 0x03D3;       ///< System B CPU in a redundant CPU system.
constexpr std::uint16_t MultipleCpu1 = 0x03E0;     ///< CPU No. 1 in a multi-CPU system.
constexpr std::uint16_t MultipleCpu2 = 0x03E1;     ///< CPU No. 2 in a multi-CPU system.
constexpr std::uint16_t MultipleCpu3 = 0x03E2;     ///< CPU No. 3 in a multi-CPU system.
constexpr std::uint16_t MultipleCpu4 = 0x03E3;     ///< CPU No. 4 in a multi-CPU system.
constexpr std::uint16_t RemoteHead1 = MultipleCpu1; ///< Remote head No. 1 route name.
constexpr std::uint16_t RemoteHead2 = MultipleCpu2; ///< Remote head No. 2 route name.
constexpr std::uint16_t ControlSystemRemoteHead = ControlSystemCpu; ///< Control system remote-head route name.
constexpr std::uint16_t StandbySystemRemoteHead = StandbySystemCpu; ///< Standby system remote-head route name.
constexpr std::uint16_t OwnStation = 0x03FF;       ///< Connected own-station route.

}  // namespace module_io

/// \brief MC protocol frame family used on the serial link.
enum class FrameKind : std::uint8_t {
  /// Chapter-8/10/11/13 oriented serial frame with the fullest feature coverage in this repository.
  C4,
  /// Shorter ASCII serial frame that reuses the `C4` payload codec.
  C3,
  /// Smallest ASCII serial frame: compact selectors are exactly `0401/0001=1`, `0401/0000=2`, `1401/0001=3`, `1401/0000=4`, `0403/0000=5`, `1402/0001=6`, `1402/0000=7`, `0801/0000=8`, and `0802/0000=9`; only loopback `0619/0000` uses the full command header; every other pair returns `UnsupportedConfiguration` before transmission, while the same public operation remains usable through 3C or 4C when supported there.
  C2,
  /// Legacy ASCII serial frame with its own command naming and routing rules.
  C1
};

/// \brief Frame families whose public configuration includes an explicit ASCII format.
enum class AsciiFrameKind : std::uint8_t {
  C4 = static_cast<std::uint8_t>(FrameKind::C4),
  C3 = static_cast<std::uint8_t>(FrameKind::C3),
  C2 = static_cast<std::uint8_t>(FrameKind::C2),
  C1 = static_cast<std::uint8_t>(FrameKind::C1),
};

[[nodiscard]] constexpr FrameKind frame_kind(AsciiFrameKind value) noexcept {
  return static_cast<FrameKind>(value);
}

/// \brief Returns whether `frame_kind` is a defined public frame-family value.
[[nodiscard]] constexpr bool is_valid_frame_kind(FrameKind frame_kind) noexcept {
  switch (frame_kind) {
    case FrameKind::C4:
    case FrameKind::C3:
    case FrameKind::C2:
    case FrameKind::C1:
      return true;
  }
  return false;
}

/// \brief Request/response payload encoding.
enum class CodeMode : std::uint8_t {
  /// Text-encoded command data and response data.
  Ascii,
  /// Compact binary command data and response data.
  Binary
};

/// \brief Returns whether `code_mode` is a defined public payload-encoding value.
[[nodiscard]] constexpr bool is_valid_code_mode(CodeMode code_mode) noexcept {
  switch (code_mode) {
    case CodeMode::Ascii:
    case CodeMode::Binary:
      return true;
  }
  return false;
}

/// \brief ASCII formatting variant for `C4` / `C3` / `C2` serial frames.
enum class AsciiFormat : std::uint8_t {
  /// ENQ/STX/ETX layout without CR/LF.
  Format1,
  /// Format1 plus a 1-byte block number used for request/response pairing on `2C/3C/4C`.
  Format2,
  /// STX-only layout commonly used on serial MC links.
  Format3,
  /// CR/LF terminated layout often used by host-facing bring-up tools.
  Format4
};

/// \brief Explicit sum-check policy for frame families that support configuration.
enum class SumCheckMode : std::uint8_t {
  Disabled,
  Enabled,
};

/// \brief Returns whether `mode` is a defined public sum-check value.
[[nodiscard]] constexpr bool is_valid_sum_check_mode(SumCheckMode mode) noexcept {
  switch (mode) {
    case SumCheckMode::Disabled:
    case SumCheckMode::Enabled:
      return true;
  }
  return false;
}

/// \brief Returns whether `format` is a defined public ASCII framing value.
[[nodiscard]] constexpr bool is_valid_ascii_format(AsciiFormat format) noexcept {
  switch (format) {
    case AsciiFormat::Format1:
    case AsciiFormat::Format2:
    case AsciiFormat::Format3:
    case AsciiFormat::Format4:
      return true;
  }
  return false;
}

/// \brief PLC family selection used for subcommand and device-layout differences.
enum class PlcSeries : std::uint8_t {
  IQ_R,
  IQ_L,
  Q_L,
  QnA,
  /// AnA/AnUCPU common-command family. Kept value-compatible with the legacy QnA selector.
  AnA_AnU = QnA,
  A,
  IQ_F,
  Unspecified = 0xFFU
};

/// \brief Public PLC profile selector.
///
/// Use `PlcProfile` as the public configuration surface. The lower-level `PlcSeries` enum is kept
/// as an internal command-layout family derived from this profile.
enum class PlcProfile : std::uint8_t {
  Unspecified = 0,
  MelsecIqR = 1,
  MelsecIqL = 2,
  MelsecQnA = 4,
  MelsecAnAAnU = 5,
  MelsecA = 6,
  MelsecIqF = 7,
  MelsecQ = 8,
  MelsecL = 9
};

/// \brief Returns the canonical saved string for a PLC profile.
[[nodiscard]] constexpr const char* plc_profile_name(PlcProfile profile) noexcept {
  switch (profile) {
    case PlcProfile::Unspecified:
      return "";
    case PlcProfile::MelsecIqR:
      return "melsec:iq-r";
    case PlcProfile::MelsecIqL:
      return "melsec:iq-l";
    case PlcProfile::MelsecIqF:
      return "melsec:iq-f";
    case PlcProfile::MelsecQ:
      return "melsec:qcpu";
    case PlcProfile::MelsecL:
      return "melsec:lcpu";
    case PlcProfile::MelsecQnA:
      return "melsec:qna";
    case PlcProfile::MelsecAnAAnU:
      return "melsec:ana-anu";
    case PlcProfile::MelsecA:
      return "melsec:a";
  }
  return "";
}

/// \brief Returns the human-readable display name for a PLC profile.
///
/// Use this for UI labels. Store and parse the canonical value from `plc_profile_name()`,
/// not this display text.
[[nodiscard]] constexpr const char* plc_profile_display_name(PlcProfile profile) noexcept {
  switch (profile) {
    case PlcProfile::Unspecified:
      return "";
    case PlcProfile::MelsecIqR:
      return "MELSEC iQ-R";
    case PlcProfile::MelsecIqL:
      return "MELSEC iQ-L";
    case PlcProfile::MelsecIqF:
      return "MELSEC iQ-F";
    case PlcProfile::MelsecQ:
      return "MELSEC-Q";
    case PlcProfile::MelsecL:
      return "MELSEC-L";
    case PlcProfile::MelsecQnA:
      return "MELSEC QnA";
    case PlcProfile::MelsecAnAAnU:
      return "MELSEC AnA/AnU";
    case PlcProfile::MelsecA:
      return "MELSEC-A";
  }
  return "";
}

/// \brief Compares a bounded text buffer with a canonical PLC profile string.
[[nodiscard]] constexpr bool plc_profile_text_equals(
    const char* text,
    std::size_t text_size,
    const char* expected,
    std::size_t expected_size) noexcept {
  if (text == nullptr || text_size != expected_size) {
    return false;
  }
  for (std::size_t index = 0U; index < expected_size; ++index) {
    if (text[index] != expected[index]) {
      return false;
    }
  }
  return true;
}

/// \brief Parses canonical PLC profile strings.
///
/// Short labels such as `iqr`, `iq-r`, `ql`, or `qna` are intentionally rejected so saved
/// configuration, CLI arguments, and documentation use one stable cross-library spelling.
[[nodiscard]] constexpr bool parse_plc_profile(
    const char* text,
    std::size_t text_size,
    PlcProfile& out_profile) noexcept {
  if (plc_profile_text_equals(text, text_size, "melsec:iq-r", sizeof("melsec:iq-r") - 1U)) {
    out_profile = PlcProfile::MelsecIqR;
    return true;
  }
  if (plc_profile_text_equals(text, text_size, "melsec:iq-l", sizeof("melsec:iq-l") - 1U)) {
    out_profile = PlcProfile::MelsecIqL;
    return true;
  }
  if (plc_profile_text_equals(text, text_size, "melsec:iq-f", sizeof("melsec:iq-f") - 1U)) {
    out_profile = PlcProfile::MelsecIqF;
    return true;
  }
  if (plc_profile_text_equals(text, text_size, "melsec:qcpu", sizeof("melsec:qcpu") - 1U)) {
    out_profile = PlcProfile::MelsecQ;
    return true;
  }
  if (plc_profile_text_equals(text, text_size, "melsec:lcpu", sizeof("melsec:lcpu") - 1U)) {
    out_profile = PlcProfile::MelsecL;
    return true;
  }
  if (plc_profile_text_equals(text, text_size, "melsec:qna", sizeof("melsec:qna") - 1U)) {
    out_profile = PlcProfile::MelsecQnA;
    return true;
  }
  if (plc_profile_text_equals(
          text,
          text_size,
          "melsec:ana-anu",
          sizeof("melsec:ana-anu") - 1U)) {
    out_profile = PlcProfile::MelsecAnAAnU;
    return true;
  }
  if (plc_profile_text_equals(text, text_size, "melsec:a", sizeof("melsec:a") - 1U)) {
    out_profile = PlcProfile::MelsecA;
    return true;
  }
  return false;
}

/// \brief Derives the internal device-layout / command-family selector from a public profile.
[[nodiscard]] constexpr PlcSeries plc_series_from_profile(PlcProfile profile) noexcept {
  switch (profile) {
    case PlcProfile::Unspecified:
      return PlcSeries::Unspecified;
    case PlcProfile::MelsecIqR:
      return PlcSeries::IQ_R;
    case PlcProfile::MelsecIqL:
      return PlcSeries::IQ_L;
    case PlcProfile::MelsecIqF:
      return PlcSeries::IQ_F;
    case PlcProfile::MelsecQ:
    case PlcProfile::MelsecL:
      return PlcSeries::Q_L;
    case PlcProfile::MelsecQnA:
      return PlcSeries::QnA;
    case PlcProfile::MelsecAnAAnU:
      return PlcSeries::AnA_AnU;
    case PlcProfile::MelsecA:
      return PlcSeries::A;
  }
  return PlcSeries::Unspecified;
}

[[nodiscard]] constexpr bool is_plc_profile_specified(PlcProfile profile) noexcept {
  switch (profile) {
    case PlcProfile::MelsecIqR:
    case PlcProfile::MelsecIqL:
    case PlcProfile::MelsecIqF:
    case PlcProfile::MelsecQ:
    case PlcProfile::MelsecL:
    case PlcProfile::MelsecQnA:
    case PlcProfile::MelsecAnAAnU:
    case PlcProfile::MelsecA:
      return true;
    case PlcProfile::Unspecified:
      return false;
  }
  return false;
}

/// \brief Route layout inside the request header.
enum class RouteKind : std::uint8_t {
  /// No route was selected. This value is observable but cannot encode a request.
  Unspecified,
  /// Host-station route with fixed `station=0`, `network=0`, `pc=FF`, and local module fields.
  HostStation,
  /// Multidrop/routed route. `1C/2C` use the station fields; `3C/4C` also carry network/PC fields.
  MultidropStation
};

/// \brief Device-family identifier used by the request codecs.
enum class DeviceCode : std::uint8_t {
  X,
  Y,
  M,
  L,
  SM,
  F,
  V,
  B,
  D,
  SD,
  W,
  TS,
  TC,
  TN,
  STS,
  STC,
  STN,
  CS,
  CC,
  CN,
  SB,
  SW,
  S,
  DX,
  DY,
  LTS,
  LTC,
  LTN,
  LSTS,
  LSTC,
  LSTN,
  LCS,
  LCC,
  LCN,
  LZ,
  Z,
  R,
  RD,
  ZR,
  G,
  HG
};

/// \brief Native Boolean value used by every individual bit read/write API.
///
/// Packed block words remain `std::uint16_t`; they are not individual bit-value inputs.
using BitValue = bool;

/// \brief Conflict-handling mode for remote RUN / PAUSE.
enum class RemoteOperationMode : std::uint16_t {
  DoNotExecuteForcibly = 0x0001,
  ExecuteForcibly = 0x0003
};

/// \brief Clear scope applied during remote RUN initialization.
enum class RemoteRunClearMode : std::uint8_t {
  DoNotClear = 0x00,
  ClearOutsideLatchRange = 0x01,
  AllClear = 0x02
};

/// \brief C24 global-signal selector used by command `1618`.
enum class GlobalSignalTarget : std::uint8_t {
  ReceivedSide = 0x00,
  X1A = 0x01,
  X1B = 0x02
};

/// \brief Target interface selector used by C24 mode switching (`1612`).
enum class SerialModuleChannel : std::uint8_t {
  Ch1 = 0x01,
  Ch2 = 0x02
};

/// \brief Operation mode number used by C24 mode switching (`1612`).
enum class SerialModuleModeNo : std::uint8_t {
  McProtocolFormat1 = 0x01,
  McProtocolFormat2 = 0x02,
  McProtocolFormat3 = 0x03,
  McProtocolFormat4 = 0x04,
  McProtocolFormat5 = 0x05,
  Nonprocedural = 0x06,
  Bidirectional = 0x07,
  Predefined = 0x09,
  ModbusRtu = 0x0A,
  ModbusAscii = 0x0B,
  MelsoftConnection = 0xFF
};

/// \brief Communication speed selector used by C24 mode switching (`1612`).
enum class SerialModuleCommunicationSpeed : std::uint8_t {
  Bps300 = 0x00,
  Bps600 = 0x01,
  Bps1200 = 0x02,
  Bps2400 = 0x03,
  Bps4800 = 0x04,
  Bps9600 = 0x05,
  Bps14400 = 0x06,
  Bps19200 = 0x07,
  Bps28800 = 0x08,
  Bps38400 = 0x09,
  Bps57600 = 0x0A,
  Bps115200 = 0x0B,
  Bps230400 = 0x0C,
  Bps50 = 0x0F
};

/// \brief Decoded PLC response class before command-specific parsing.
enum class ResponseType : std::uint8_t {
  /// Successful response carrying payload bytes.
  SuccessData,
  /// Successful response carrying no payload bytes.
  SuccessNoData,
  /// PLC-side error response with an end code / error code.
  PlcError
};

/// \brief Absolute transaction and retained-frame inactivity timeouts.
struct TimeoutConfig {
  /// Maximum complete transaction duration. Partial progress never restarts this limit.
  std::uint32_t response_timeout_ms = 3000;
  /// Maximum inactivity after a possible response frame has been retained.
  std::uint32_t inter_byte_timeout_ms = 250;
};

/// \brief Connected host-station route.
///
/// The connected-station header values are protocol constants and therefore are intentionally not
/// exposed as mutable inputs.
struct HostStationRoute {};

/// \brief Meaning of a 3C/4C routed PC target.
enum class C34PcTargetKind : std::uint8_t {
  Number,
  ControlSystem,
  StandbySystem,
  SpecialFe,
  ConnectedStation,
};

/// \brief Mandatory typed PC target for 3C/4C multidrop routes.
class C34PcTarget {
 public:
  [[nodiscard]] static constexpr C34PcTarget number(std::uint32_t value) noexcept {
    return C34PcTarget(C34PcTargetKind::Number, value);
  }
  [[nodiscard]] static constexpr C34PcTarget control_system() noexcept {
    return C34PcTarget(C34PcTargetKind::ControlSystem, 0x7DU);
  }
  [[nodiscard]] static constexpr C34PcTarget standby_system() noexcept {
    return C34PcTarget(C34PcTargetKind::StandbySystem, 0x7EU);
  }
  [[nodiscard]] static constexpr C34PcTarget special_fe() noexcept {
    return C34PcTarget(C34PcTargetKind::SpecialFe, 0xFEU);
  }
  [[nodiscard]] static constexpr C34PcTarget connected_station() noexcept {
    return C34PcTarget(C34PcTargetKind::ConnectedStation, 0xFFU);
  }

  [[nodiscard]] constexpr C34PcTargetKind kind() const noexcept { return kind_; }
  [[nodiscard]] constexpr std::uint32_t value() const noexcept { return value_; }
  [[nodiscard]] constexpr bool is_valid() const noexcept {
    switch (kind_) {
      case C34PcTargetKind::Number:
        return value_ >= 0x01U && value_ <= 0x78U;
      case C34PcTargetKind::ControlSystem:
        return value_ == 0x7DU;
      case C34PcTargetKind::StandbySystem:
        return value_ == 0x7EU;
      case C34PcTargetKind::SpecialFe:
        return value_ == 0xFEU;
      case C34PcTargetKind::ConnectedStation:
        return value_ == 0xFFU;
    }
    return false;
  }

 private:
  constexpr C34PcTarget(C34PcTargetKind kind, std::uint32_t value) noexcept
      : kind_(kind), value_(value) {}

  C34PcTargetKind kind_;
  std::uint32_t value_;
};

/// \brief Meaning of a mandatory 4C request-destination module target.
enum class C4DestinationModuleKind : std::uint8_t {
  OwnStation,
  MultipleCpu,
  RedundantControlSystemCpu,
  RedundantStandbySystemCpu,
  RedundantSystemACpu,
  RedundantSystemBCpu,
  Explicit,
};

/// \brief Mandatory typed request-destination module for a 4C multidrop route.
class C4DestinationModule {
 public:
  [[nodiscard]] static constexpr C4DestinationModule own_station() noexcept {
    return C4DestinationModule(
        C4DestinationModuleKind::OwnStation, module_io::OwnStation, 0x00U, 0U);
  }
  [[nodiscard]] static constexpr C4DestinationModule multiple_cpu(
      std::uint32_t cpu_number) noexcept {
    const bool valid = cpu_number >= 1U && cpu_number <= 4U;
    return C4DestinationModule(
        C4DestinationModuleKind::MultipleCpu,
        valid ? (module_io::MultipleCpu1 + cpu_number - 1U) : 0U,
        0x00U,
        cpu_number);
  }
  [[nodiscard]] static constexpr C4DestinationModule redundant_control_system_cpu() noexcept {
    return C4DestinationModule(
        C4DestinationModuleKind::RedundantControlSystemCpu,
        module_io::ControlSystemCpu,
        0x00U,
        0U);
  }
  [[nodiscard]] static constexpr C4DestinationModule redundant_standby_system_cpu() noexcept {
    return C4DestinationModule(
        C4DestinationModuleKind::RedundantStandbySystemCpu,
        module_io::StandbySystemCpu,
        0x00U,
        0U);
  }
  [[nodiscard]] static constexpr C4DestinationModule redundant_system_a_cpu() noexcept {
    return C4DestinationModule(
        C4DestinationModuleKind::RedundantSystemACpu,
        module_io::SystemACpu,
        0x00U,
        0U);
  }
  [[nodiscard]] static constexpr C4DestinationModule redundant_system_b_cpu() noexcept {
    return C4DestinationModule(
        C4DestinationModuleKind::RedundantSystemBCpu,
        module_io::SystemBCpu,
        0x00U,
        0U);
  }
  [[nodiscard]] static constexpr C4DestinationModule explicit_target(
      std::uint32_t io_number,
      std::uint32_t station_number) noexcept {
    return C4DestinationModule(
        C4DestinationModuleKind::Explicit, io_number, station_number, 0U);
  }

  [[nodiscard]] constexpr C4DestinationModuleKind kind() const noexcept { return kind_; }
  [[nodiscard]] constexpr std::uint32_t io_number() const noexcept { return io_number_; }
  [[nodiscard]] constexpr std::uint32_t station_number() const noexcept {
    return station_number_;
  }
  [[nodiscard]] constexpr bool is_own_station_selector() const noexcept {
    return kind_ == C4DestinationModuleKind::OwnStation && is_valid();
  }
  [[nodiscard]] constexpr bool is_valid() const noexcept {
    switch (kind_) {
      case C4DestinationModuleKind::OwnStation:
        return io_number_ == module_io::OwnStation && station_number_ == 0U;
      case C4DestinationModuleKind::MultipleCpu:
        return selector_number_ >= 1U && selector_number_ <= 4U &&
               io_number_ == (module_io::MultipleCpu1 + selector_number_ - 1U) &&
               station_number_ == 0U;
      case C4DestinationModuleKind::RedundantControlSystemCpu:
        return io_number_ == module_io::ControlSystemCpu && station_number_ == 0U;
      case C4DestinationModuleKind::RedundantStandbySystemCpu:
        return io_number_ == module_io::StandbySystemCpu && station_number_ == 0U;
      case C4DestinationModuleKind::RedundantSystemACpu:
        return io_number_ == module_io::SystemACpu && station_number_ == 0U;
      case C4DestinationModuleKind::RedundantSystemBCpu:
        return io_number_ == module_io::SystemBCpu && station_number_ == 0U;
      case C4DestinationModuleKind::Explicit:
        return io_number_ <= 0xFFFFU && station_number_ <= 0xFFU;
    }
    return false;
  }

 private:
  constexpr C4DestinationModule(
      C4DestinationModuleKind kind,
      std::uint32_t io_number,
      std::uint32_t station_number,
      std::uint32_t selector_number) noexcept
      : kind_(kind),
        io_number_(io_number),
        station_number_(station_number),
        selector_number_(selector_number) {}

  C4DestinationModuleKind kind_;
  std::uint32_t io_number_;
  std::uint32_t station_number_;
  std::uint32_t selector_number_;
};

/// \brief Mandatory request-source station number for an m:n multidrop route.
class SelfStationNo {
 public:
  [[nodiscard]] static constexpr SelfStationNo number(std::uint32_t value) noexcept {
    return SelfStationNo(value);
  }

  [[nodiscard]] constexpr std::uint32_t value() const noexcept { return value_; }
  [[nodiscard]] constexpr bool is_valid() const noexcept { return value_ <= 0x1FU; }

 private:
  constexpr explicit SelfStationNo(std::uint32_t value) noexcept : value_(value) {}

  std::uint32_t value_;
};

/// \brief Explicit 1C multidrop route. Network and self-station fields do not exist on this type.
class C1MultidropRoute {
 public:
  constexpr explicit C1MultidropRoute(std::uint32_t station_no) noexcept
      : station_no_(station_no) {}
  [[nodiscard]] constexpr std::uint32_t station_no() const noexcept { return station_no_; }

 private:
  std::uint32_t station_no_;
};

/// \brief Explicit 2C normal/1:n multidrop route. Self-station is fixed to zero.
class C2StandardMultidropRoute {
 public:
  constexpr explicit C2StandardMultidropRoute(std::uint32_t station_no) noexcept
      : station_no_(station_no) {}
  [[nodiscard]] constexpr std::uint32_t station_no() const noexcept { return station_no_; }

 private:
  std::uint32_t station_no_;
};

/// \brief Explicit 2C m:n multidrop route with a mandatory self-station number.
class C2MnMultidropRoute {
 public:
  constexpr C2MnMultidropRoute(
      std::uint32_t station_no,
      SelfStationNo self_station_no) noexcept
      : station_no_(station_no), self_station_no_(self_station_no) {}
  [[nodiscard]] constexpr std::uint32_t station_no() const noexcept { return station_no_; }
  [[nodiscard]] constexpr SelfStationNo self_station_no() const noexcept {
    return self_station_no_;
  }

 private:
  std::uint32_t station_no_;
  SelfStationNo self_station_no_;
};

/// \brief Explicit 3C normal/1:n multidrop route. Self-station is fixed to zero.
class C3StandardMultidropRoute {
 public:
  constexpr C3StandardMultidropRoute(
      std::uint32_t station_no,
      std::uint32_t network_no,
      C34PcTarget pc_target) noexcept
      : station_no_(station_no),
        network_no_(network_no),
        pc_target_(pc_target) {}
  [[nodiscard]] constexpr std::uint32_t station_no() const noexcept { return station_no_; }
  [[nodiscard]] constexpr std::uint32_t network_no() const noexcept { return network_no_; }
  [[nodiscard]] constexpr C34PcTarget pc_target() const noexcept { return pc_target_; }

 private:
  std::uint32_t station_no_;
  std::uint32_t network_no_;
  C34PcTarget pc_target_;
};

/// \brief Explicit 3C m:n multidrop route with a mandatory self-station number.
class C3MnMultidropRoute {
 public:
  constexpr C3MnMultidropRoute(
      std::uint32_t station_no,
      std::uint32_t network_no,
      C34PcTarget pc_target,
      SelfStationNo self_station_no) noexcept
      : station_no_(station_no),
        network_no_(network_no),
        pc_target_(pc_target),
        self_station_no_(self_station_no) {}
  [[nodiscard]] constexpr std::uint32_t station_no() const noexcept { return station_no_; }
  [[nodiscard]] constexpr std::uint32_t network_no() const noexcept { return network_no_; }
  [[nodiscard]] constexpr C34PcTarget pc_target() const noexcept { return pc_target_; }
  [[nodiscard]] constexpr SelfStationNo self_station_no() const noexcept {
    return self_station_no_;
  }

 private:
  std::uint32_t station_no_;
  std::uint32_t network_no_;
  C34PcTarget pc_target_;
  SelfStationNo self_station_no_;
};

/// \brief Explicit 4C normal/1:n multidrop route. Self-station is fixed to zero.
class C4StandardMultidropRoute {
 public:
  constexpr C4StandardMultidropRoute(
      std::uint32_t station_no,
      std::uint32_t network_no,
      C34PcTarget pc_target,
      C4DestinationModule destination_module) noexcept
      : station_no_(station_no),
        network_no_(network_no),
        pc_target_(pc_target),
        destination_module_(destination_module) {}
  [[nodiscard]] constexpr std::uint32_t station_no() const noexcept { return station_no_; }
  [[nodiscard]] constexpr std::uint32_t network_no() const noexcept { return network_no_; }
  [[nodiscard]] constexpr C34PcTarget pc_target() const noexcept { return pc_target_; }
  [[nodiscard]] constexpr C4DestinationModule destination_module() const noexcept {
    return destination_module_;
  }

 private:
  std::uint32_t station_no_;
  std::uint32_t network_no_;
  C34PcTarget pc_target_;
  C4DestinationModule destination_module_;
};

/// \brief Explicit 4C m:n multidrop route with a mandatory self-station number.
class C4MnMultidropRoute {
 public:
  constexpr C4MnMultidropRoute(
      std::uint32_t station_no,
      std::uint32_t network_no,
      C34PcTarget pc_target,
      C4DestinationModule destination_module,
      SelfStationNo self_station_no) noexcept
      : station_no_(station_no),
        network_no_(network_no),
        pc_target_(pc_target),
        destination_module_(destination_module),
        self_station_no_(self_station_no) {}
  [[nodiscard]] constexpr std::uint32_t station_no() const noexcept { return station_no_; }
  [[nodiscard]] constexpr std::uint32_t network_no() const noexcept { return network_no_; }
  [[nodiscard]] constexpr C34PcTarget pc_target() const noexcept { return pc_target_; }
  [[nodiscard]] constexpr C4DestinationModule destination_module() const noexcept {
    return destination_module_;
  }
  [[nodiscard]] constexpr SelfStationNo self_station_no() const noexcept {
    return self_station_no_;
  }

 private:
  std::uint32_t station_no_;
  std::uint32_t network_no_;
  C34PcTarget pc_target_;
  C4DestinationModule destination_module_;
  SelfStationNo self_station_no_;
};

namespace detail {
struct RouteConfigAccess;
}

/// \brief Explicit route selection for a protocol session.
///
/// Default construction represents an omitted route and is rejected before request encoding. Use
/// `RouteConfig {HostStationRoute {}}` or a frame-specific route type explicitly.
class RouteConfig {
 public:
  constexpr RouteConfig() noexcept = default;
  constexpr explicit RouteConfig(HostStationRoute) noexcept : kind_(RouteKind::HostStation) {}
  constexpr explicit RouteConfig(C1MultidropRoute route) noexcept
      : kind_(RouteKind::MultidropStation), route_frame_(FrameKind::C1),
        station_no_(route.station_no()) {}
  constexpr explicit RouteConfig(C2StandardMultidropRoute route) noexcept
      : kind_(RouteKind::MultidropStation), route_frame_(FrameKind::C2),
        station_no_(route.station_no()) {}
  constexpr explicit RouteConfig(C2MnMultidropRoute route) noexcept
      : kind_(RouteKind::MultidropStation), route_frame_(FrameKind::C2),
        station_no_(route.station_no()), mn_multidrop_(true),
        self_station_no_(route.self_station_no().value()),
        self_station_valid_(route.self_station_no().is_valid()) {}
  constexpr explicit RouteConfig(C3StandardMultidropRoute route) noexcept
      : kind_(RouteKind::MultidropStation), route_frame_(FrameKind::C3),
        station_no_(route.station_no()), network_no_(route.network_no()),
        pc_no_(route.pc_target().value()), pc_target_valid_(route.pc_target().is_valid()) {}
  constexpr explicit RouteConfig(C3MnMultidropRoute route) noexcept
      : kind_(RouteKind::MultidropStation), route_frame_(FrameKind::C3),
        station_no_(route.station_no()), network_no_(route.network_no()),
        pc_no_(route.pc_target().value()), pc_target_valid_(route.pc_target().is_valid()),
        mn_multidrop_(true), self_station_no_(route.self_station_no().value()),
        self_station_valid_(route.self_station_no().is_valid()) {}
  constexpr explicit RouteConfig(C4StandardMultidropRoute route) noexcept
      : kind_(RouteKind::MultidropStation), route_frame_(FrameKind::C4),
        station_no_(route.station_no()), network_no_(route.network_no()),
        pc_no_(route.pc_target().value()), pc_target_valid_(route.pc_target().is_valid()),
        module_io_no_(route.destination_module().io_number()),
        module_station_no_(route.destination_module().station_number()),
        destination_module_valid_(route.destination_module().is_valid()),
        destination_module_own_selector_(route.destination_module().is_own_station_selector()) {}
  constexpr explicit RouteConfig(C4MnMultidropRoute route) noexcept
      : kind_(RouteKind::MultidropStation), route_frame_(FrameKind::C4),
        station_no_(route.station_no()), network_no_(route.network_no()),
        pc_no_(route.pc_target().value()), pc_target_valid_(route.pc_target().is_valid()),
        module_io_no_(route.destination_module().io_number()),
        module_station_no_(route.destination_module().station_number()),
        destination_module_valid_(route.destination_module().is_valid()),
        destination_module_own_selector_(route.destination_module().is_own_station_selector()),
        mn_multidrop_(true), self_station_no_(route.self_station_no().value()),
        self_station_valid_(route.self_station_no().is_valid()) {}
  [[nodiscard]] constexpr RouteKind kind() const noexcept { return kind_; }
  [[nodiscard]] constexpr bool is_specified() const noexcept {
    return kind_ != RouteKind::Unspecified;
  }
  [[nodiscard]] constexpr bool is_host_station() const noexcept {
    return kind_ == RouteKind::HostStation;
  }
  [[nodiscard]] constexpr bool is_multidrop() const noexcept {
    return kind_ == RouteKind::MultidropStation;
  }
  [[nodiscard]] constexpr bool supports_frame(FrameKind frame_kind) const noexcept {
    return is_host_station() || (is_multidrop() && route_frame_ == frame_kind);
  }

  [[nodiscard]] constexpr std::uint32_t station_no() const noexcept {
    return is_multidrop() ? station_no_ : 0x00U;
  }
  [[nodiscard]] constexpr std::uint32_t network_no() const noexcept {
    return is_multidrop() ? network_no_ : 0x00U;
  }
  [[nodiscard]] constexpr std::uint32_t pc_no() const noexcept {
    return is_multidrop() ? pc_no_ : 0xFFU;
  }
  [[nodiscard]] constexpr bool pc_target_valid() const noexcept {
    return is_host_station() || pc_target_valid_;
  }
  [[nodiscard]] constexpr std::uint32_t request_destination_module_io_no() const noexcept {
    return is_multidrop() ? module_io_no_ : module_io::OwnStation;
  }
  [[nodiscard]] constexpr std::uint32_t request_destination_module_station_no() const noexcept {
    return is_multidrop() ? module_station_no_ : 0x00U;
  }
  [[nodiscard]] constexpr bool destination_module_valid() const noexcept {
    return is_host_station() || destination_module_valid_;
  }
  [[nodiscard]] constexpr bool destination_module_is_own_station() const noexcept {
    return is_host_station() || destination_module_own_selector_;
  }
  [[nodiscard]] constexpr bool is_mn_multidrop() const noexcept {
    return is_multidrop() && mn_multidrop_;
  }
  [[nodiscard]] constexpr std::uint32_t self_station_no() const noexcept {
    return is_mn_multidrop() ? self_station_no_ : 0x00U;
  }
  [[nodiscard]] constexpr bool self_station_valid() const noexcept {
    return !is_mn_multidrop() || self_station_valid_;
  }

 private:
  friend class FrameCodec;
  friend struct detail::RouteConfigAccess;

  [[nodiscard]] static constexpr RouteConfig c1_wire_route(
      std::uint8_t station_no,
      std::uint8_t pc_no) noexcept {
    RouteConfig route {C1MultidropRoute {station_no}};
    route.pc_no_ = pc_no;
    route.pc_target_valid_ = true;
    return route;
  }

  RouteKind kind_ = RouteKind::Unspecified;
  FrameKind route_frame_ = static_cast<FrameKind>(0xFF);
  std::uint32_t station_no_ = 0x00U;
  std::uint32_t network_no_ = 0x00U;
  std::uint32_t pc_no_ = 0xFFU;
  bool pc_target_valid_ = false;
  std::uint32_t module_io_no_ = module_io::OwnStation;
  std::uint32_t module_station_no_ = 0x00U;
  bool destination_module_valid_ = false;
  bool destination_module_own_selector_ = false;
  bool mn_multidrop_ = false;
  std::uint32_t self_station_no_ = 0x00U;
  bool self_station_valid_ = true;
};

class MelsecSerialClient;
class HostSyncClient;

/// \brief Immutable tagged protocol configuration shared by codecs and client requests.
///
/// Treat this as the immutable session configuration for one serial link. The same object is used
/// by:
///
/// - `FrameCodec` for frame wrapping and response decoding
/// - `CommandCodec` for command subcommand/device-layout differences
/// - `MelsecSerialClient` and `HostSyncClient` for runtime request execution
class ProtocolConfig {
 public:
  ProtocolConfig() = delete;

  /// \brief Constructs an explicit C4 Binary/Format5 session.
  [[nodiscard]] static constexpr ProtocolConfig c4_binary(
      PlcProfile plc_profile,
      SumCheckMode sum_check_mode,
      RouteConfig route,
      TimeoutConfig timeout = {}) noexcept {
    return ProtocolConfig(
        FrameKind::C4,
        CodeMode::Binary,
        static_cast<AsciiFormat>(0xFF),
        plc_profile,
        sum_check_mode,
        route,
        timeout);
  }

  /// \brief Constructs an explicit ASCII session with a mandatory framing format.
  [[nodiscard]] static constexpr ProtocolConfig ascii(
      AsciiFrameKind ascii_frame_kind,
      AsciiFormat ascii_format,
      PlcProfile plc_profile,
      SumCheckMode sum_check_mode,
      RouteConfig route,
      TimeoutConfig timeout = {}) noexcept {
    return ProtocolConfig(
        mcprotocol::serial::frame_kind(ascii_frame_kind),
        CodeMode::Ascii,
        ascii_format,
        plc_profile,
        sum_check_mode,
        route,
        timeout);
  }

  [[nodiscard]] constexpr FrameKind frame_kind() const noexcept { return frame_kind_; }
  [[nodiscard]] constexpr CodeMode code_mode() const noexcept { return code_mode_; }
  [[nodiscard]] constexpr bool has_ascii_format() const noexcept {
    return code_mode_ == CodeMode::Ascii;
  }
  [[nodiscard]] constexpr AsciiFormat ascii_format() const noexcept { return ascii_format_; }
  [[nodiscard]] constexpr PlcProfile plc_profile() const noexcept { return plc_profile_; }
  [[nodiscard]] constexpr SumCheckMode sum_check_mode() const noexcept {
    return sum_check_mode_;
  }
  [[nodiscard]] constexpr const RouteConfig& route() const noexcept { return route_; }
  [[nodiscard]] constexpr const TimeoutConfig& timeout() const noexcept { return timeout_; }

  /// \brief Returns a new immutable session configuration with a different explicit profile.
  [[nodiscard]] constexpr ProtocolConfig with_plc_profile(PlcProfile value) const noexcept {
    return ProtocolConfig(
        frame_kind_, code_mode_, ascii_format_, value, sum_check_mode_, route_, timeout_);
  }

  /// \brief Returns a new immutable session configuration with a different typed route.
  [[nodiscard]] constexpr ProtocolConfig with_route(RouteConfig value) const noexcept {
    return ProtocolConfig(
        frame_kind_, code_mode_, ascii_format_, plc_profile_, sum_check_mode_, value, timeout_);
  }

  /// \brief Returns a new immutable session configuration with different host timeout settings.
  [[nodiscard]] constexpr ProtocolConfig with_timeout(TimeoutConfig value) const noexcept {
    return ProtocolConfig(
        frame_kind_, code_mode_, ascii_format_, plc_profile_, sum_check_mode_, route_, value);
  }

  [[nodiscard]] constexpr ProtocolConfig with_response_timeout_ms(
      std::uint32_t value) const noexcept {
    TimeoutConfig next = timeout_;
    next.response_timeout_ms = value;
    return with_timeout(next);
  }

  /// \brief Returns a new configuration with a different retained-frame inactivity timeout.
  [[nodiscard]] constexpr ProtocolConfig with_inter_byte_timeout_ms(
      std::uint32_t value) const noexcept {
    TimeoutConfig next = timeout_;
    next.inter_byte_timeout_ms = value;
    return with_timeout(next);
  }

 private:
  struct UnconfiguredTag {};

  constexpr ProtocolConfig(
      FrameKind frame_kind,
      CodeMode code_mode,
      AsciiFormat ascii_format,
      PlcProfile plc_profile,
      SumCheckMode sum_check_mode,
      RouteConfig route,
      TimeoutConfig timeout) noexcept
      : frame_kind_(frame_kind),
        code_mode_(code_mode),
        ascii_format_(ascii_format),
        plc_profile_(plc_profile),
        sum_check_mode_(sum_check_mode),
        route_(route),
        timeout_(timeout) {}

  constexpr explicit ProtocolConfig(UnconfiguredTag) noexcept {}
  [[nodiscard]] static constexpr ProtocolConfig unconfigured_for_storage() noexcept {
    return ProtocolConfig(UnconfiguredTag {});
  }

  friend class MelsecSerialClient;
  friend class HostSyncClient;

  FrameKind frame_kind_ = static_cast<FrameKind>(0xFF);
  CodeMode code_mode_ = static_cast<CodeMode>(0xFF);
  AsciiFormat ascii_format_ = static_cast<AsciiFormat>(0xFF);
  PlcProfile plc_profile_ = PlcProfile::Unspecified;
  SumCheckMode sum_check_mode_ = static_cast<SumCheckMode>(0xFF);
  RouteConfig route_ {};
  TimeoutConfig timeout_ {};
};

/// \brief Device code plus numeric address.
///
/// This is the normalized address form used throughout the library after string-address parsing.
struct DeviceAddress {
  DeviceAddress() = delete;
  constexpr DeviceAddress(DeviceCode device_code, std::uint32_t device_number) noexcept
      : code(device_code), number(device_number) {}

  /// Device family such as `D`, `M`, `X`, `LTN`, or `LZ`.
  DeviceCode code;
  /// Numeric index inside the selected device family.
  std::uint32_t number;
};

/// \brief Extended file-register address using block number plus `R` word number.
///
/// This is the block-addressed form used by `1C ACPU-common`.
struct ExtendedFileRegisterAddress {
  ExtendedFileRegisterAddress() = delete;
  constexpr ExtendedFileRegisterAddress(
      std::uint16_t target_block_number,
      std::uint16_t target_word_number) noexcept
      : block_number(target_block_number), word_number(target_word_number) {}

  /// Extended file-register block number.
  std::uint16_t block_number;
  /// Word number inside the selected block.
  std::uint16_t word_number;
};

/// \name Device-Memory Contiguous Requests
/// @{
/// \brief Contiguous word-read request (`0401`).
struct BatchReadWordsRequest {
  BatchReadWordsRequest() = delete;
  constexpr BatchReadWordsRequest(DeviceAddress first_device, std::uint16_t point_count) noexcept
      : head_device(first_device), points(point_count) {}

  /// First device in the contiguous range.
  DeviceAddress head_device;
  /// Number of points to read starting at `head_device`.
  std::uint16_t points;
};

/// \brief Contiguous bit-read request (`0401` bit path).
struct BatchReadBitsRequest {
  BatchReadBitsRequest() = delete;
  constexpr BatchReadBitsRequest(DeviceAddress first_device, std::uint16_t point_count) noexcept
      : head_device(first_device), points(point_count) {}

  /// First bit device in the contiguous range.
  DeviceAddress head_device;
  /// Number of bit points to read starting at `head_device`.
  std::uint16_t points;
};

/// \brief Contiguous word-write request (`1401`).
struct BatchWriteWordsRequest {
  BatchWriteWordsRequest() = delete;
  constexpr BatchWriteWordsRequest(
      DeviceAddress first_device,
      mcprotocol::serial::Span<const std::uint16_t> write_words) noexcept
      : head_device(first_device), words(write_words) {}

  /// First device in the contiguous write range.
  DeviceAddress head_device;
  /// Caller-owned word data to write starting at `head_device`.
  mcprotocol::serial::Span<const std::uint16_t> words;
};

/// \brief Contiguous bit-write request (`1401` bit path).
struct BatchWriteBitsRequest {
  BatchWriteBitsRequest() = delete;
  constexpr BatchWriteBitsRequest(
      DeviceAddress first_device,
      mcprotocol::serial::Span<const BitValue> write_bits) noexcept
      : head_device(first_device), bits(write_bits) {}

  /// First bit device in the contiguous write range.
  DeviceAddress head_device;
  /// Caller-owned bit data to write starting at `head_device`.
  mcprotocol::serial::Span<const BitValue> bits;
};
/// @}

/// \name Extended File-Register Requests
/// @{
/// \brief Extended file-register batch read (`ER` on 1C ACPU-common).
struct ExtendedFileRegisterBatchReadWordsRequest {
  ExtendedFileRegisterBatchReadWordsRequest() = delete;
  constexpr ExtendedFileRegisterBatchReadWordsRequest(
      ExtendedFileRegisterAddress first_device,
      std::uint16_t point_count) noexcept
      : head_device(first_device), points(point_count) {}

  /// First block-addressed file-register word to read.
  ExtendedFileRegisterAddress head_device;
  /// Number of words to read from the file-register range.
  std::uint16_t points;
};

/// \brief Direct extended file-register batch read (`NR` on 1C AnA/AnUCPU common).
struct ExtendedFileRegisterDirectBatchReadWordsRequest {
  ExtendedFileRegisterDirectBatchReadWordsRequest() = delete;
  constexpr ExtendedFileRegisterDirectBatchReadWordsRequest(
      std::uint32_t first_device_number,
      std::uint16_t point_count) noexcept
      : head_device_number(first_device_number), points(point_count) {}

  /// \brief `NR/NW` direct address on 1C.
  std::uint32_t head_device_number;
  /// Number of words to read from the direct file-register range.
  std::uint16_t points;
};

/// \brief Extended file-register batch write (`EW` on 1C ACPU-common).
struct ExtendedFileRegisterBatchWriteWordsRequest {
  ExtendedFileRegisterBatchWriteWordsRequest() = delete;
  constexpr ExtendedFileRegisterBatchWriteWordsRequest(
      ExtendedFileRegisterAddress first_device,
      mcprotocol::serial::Span<const std::uint16_t> write_words) noexcept
      : head_device(first_device), words(write_words) {}

  /// First block-addressed file-register word to write.
  ExtendedFileRegisterAddress head_device;
  /// Caller-owned word data to write starting at `head_device`.
  mcprotocol::serial::Span<const std::uint16_t> words;
};

/// \brief Direct extended file-register batch write (`NW` on 1C AnA/AnUCPU common).
struct ExtendedFileRegisterDirectBatchWriteWordsRequest {
  ExtendedFileRegisterDirectBatchWriteWordsRequest() = delete;
  constexpr ExtendedFileRegisterDirectBatchWriteWordsRequest(
      std::uint32_t first_device_number,
      mcprotocol::serial::Span<const std::uint16_t> write_words) noexcept
      : head_device_number(first_device_number), words(write_words) {}

  /// \brief `NR/NW` direct address on 1C.
  std::uint32_t head_device_number;
  /// Caller-owned word data to write starting at `head_device_number`.
  mcprotocol::serial::Span<const std::uint16_t> words;
};

/// \brief One item inside extended file-register random write (`ET` on 1C).
struct ExtendedFileRegisterRandomWriteWordItem {
  ExtendedFileRegisterRandomWriteWordItem() = delete;
  constexpr ExtendedFileRegisterRandomWriteWordItem(
      ExtendedFileRegisterAddress target_device,
      std::uint16_t write_value) noexcept
      : device(target_device), value(write_value) {}

  /// Target extended file-register address.
  ExtendedFileRegisterAddress device;
  /// One word written to `device`.
  std::uint16_t value;
};

/// \brief Extended file-register monitor registration (`EM` on 1C).
struct ExtendedFileRegisterMonitorRegistration {
  ExtendedFileRegisterMonitorRegistration() = delete;
  constexpr explicit ExtendedFileRegisterMonitorRegistration(
      mcprotocol::serial::Span<const ExtendedFileRegisterAddress> monitor_items) noexcept
      : items(monitor_items) {}

  /// Sparse list of block-addressed file-register items to register for monitoring.
  mcprotocol::serial::Span<const ExtendedFileRegisterAddress> items;
};
/// @}

/// \name Device-Memory Random And Multi-Block Requests
/// @{
/// \brief One explicitly word-width item in a native random-read or monitor request.
struct RandomReadWordItem {
  /// Target device address read as one 16-bit word (bit devices return a 16-point mask word).
  DeviceAddress device;
};

/// \brief One explicitly double-word-width item in a native random-read or monitor request.
struct RandomReadDWordItem {
  /// Target device address read as one 32-bit double word.
  DeviceAddress device;
};

/// \brief Native random-read request with separate word and double-word domains.
struct RandomReadRequest {
  RandomReadRequest() = delete;
  constexpr RandomReadRequest(
      mcprotocol::serial::Span<const RandomReadWordItem> words,
      mcprotocol::serial::Span<const RandomReadDWordItem> dwords) noexcept
      : word_items(words), dword_items(dwords) {}

  /// Sparse 16-bit items, encoded first and returned through the word output span.
  mcprotocol::serial::Span<const RandomReadWordItem> word_items;
  /// Sparse 32-bit items, encoded second and returned through the dword output span.
  mcprotocol::serial::Span<const RandomReadDWordItem> dword_items;
};

/// \brief One explicitly word-width item inside native random write (`1402` word path).
///
/// Device and value are a single construction boundary. Explicit zero is valid; omission is not.
struct RandomWriteWordItem {
  RandomWriteWordItem() = delete;
  constexpr RandomWriteWordItem(DeviceAddress target_device, std::uint16_t write_value) noexcept
      : device(target_device), value(write_value) {}

  /// Target device address for the sparse write.
  DeviceAddress device;
  /// One 16-bit word value to write.
  std::uint16_t value;
};

/// \brief One explicitly double-word-width item inside native random write (`1402` word path).
///
/// Device and value are a single construction boundary. Explicit zero is valid; omission is not.
struct RandomWriteDWordItem {
  RandomWriteDWordItem() = delete;
  constexpr RandomWriteDWordItem(DeviceAddress target_device, std::uint32_t write_value) noexcept
      : device(target_device), value(write_value) {}

  /// Target device address for the sparse write.
  DeviceAddress device;
  /// One 32-bit double-word value to write.
  std::uint32_t value;
};

/// \brief One bit item inside native random write (`1402` bit path).
///
/// Device and value are a single construction boundary. Explicit `Off` is valid; omission and
/// unknown enum values are rejected.
struct RandomWriteBitItem {
  RandomWriteBitItem() = delete;
  constexpr RandomWriteBitItem(DeviceAddress target_device, BitValue write_value) noexcept
      : device(target_device), value(write_value) {}

  /// Target bit device address for the sparse write.
  DeviceAddress device;
  /// Bit value written to `device`.
  BitValue value;
};

/// \brief One block inside native multi-block read (`0406`).
struct MultiBlockReadBlock {
  MultiBlockReadBlock() = delete;
  constexpr MultiBlockReadBlock(
      DeviceAddress first_device,
      std::uint16_t point_count,
      bool use_bit_block) noexcept
      : head_device(first_device), points(point_count), bit_block(use_bit_block) {}

  /// First device in this contiguous block.
  DeviceAddress head_device;
  /// Number of points in this block.
  std::uint16_t points;
  /// `true` for bit blocks, `false` for word blocks.
  bool bit_block;
};

/// \brief Native multi-block read request composed of multiple contiguous blocks.
struct MultiBlockReadRequest {
  MultiBlockReadRequest() = delete;
  constexpr explicit MultiBlockReadRequest(
      mcprotocol::serial::Span<const MultiBlockReadBlock> request_blocks) noexcept
      : blocks(request_blocks) {}

  /// Ordered block list encoded into the native multi-block read request.
  mcprotocol::serial::Span<const MultiBlockReadBlock> blocks;
};

/// \brief One block inside native multi-block write (`1406`).
struct MultiBlockWriteBlock {
  MultiBlockWriteBlock() = delete;
  constexpr MultiBlockWriteBlock(
      DeviceAddress first_device,
      std::uint16_t point_count,
      bool use_bit_block,
      mcprotocol::serial::Span<const std::uint16_t> write_words,
      mcprotocol::serial::Span<const BitValue> write_bits) noexcept
      : head_device(first_device),
        points(point_count),
        bit_block(use_bit_block),
        words(write_words),
        bits(write_bits) {}
  constexpr MultiBlockWriteBlock(
      DeviceAddress first_device,
      std::uint16_t point_count,
      mcprotocol::serial::Span<const std::uint16_t> write_words) noexcept
      : MultiBlockWriteBlock(first_device, point_count, false, write_words, {}) {}
  constexpr MultiBlockWriteBlock(
      DeviceAddress first_device,
      std::uint16_t point_count,
      mcprotocol::serial::Span<const BitValue> write_bits) noexcept
      : MultiBlockWriteBlock(first_device, point_count, true, {}, write_bits) {}

  /// First device in this contiguous block.
  DeviceAddress head_device;
  /// Point count for this block.
  std::uint16_t points;
  /// `true` when `bits` is used, `false` when `words` is used.
  bool bit_block;
  /// Caller-owned word data for word blocks.
  mcprotocol::serial::Span<const std::uint16_t> words;
  /// Caller-owned bit data for bit blocks.
  mcprotocol::serial::Span<const BitValue> bits;
};

/// \brief Native multi-block write request composed of multiple contiguous blocks.
struct MultiBlockWriteRequest {
  MultiBlockWriteRequest() = delete;
  constexpr explicit MultiBlockWriteRequest(
      mcprotocol::serial::Span<const MultiBlockWriteBlock> request_blocks) noexcept
      : blocks(request_blocks) {}

  /// Ordered block list encoded into the native multi-block write request.
  mcprotocol::serial::Span<const MultiBlockWriteBlock> blocks;
};

/// \brief Parsed layout description for one block returned by `parse_multi_block_read_response()`.
struct MultiBlockReadBlockResult {
  /// Block kind copied from the original request.
  bool bit_block = false;
  /// Block head device copied from the original request.
  DeviceAddress head_device {DeviceCode::D, 0U};
  /// Point count copied from the original request.
  std::uint16_t points = 0;
  /// Offset into the aggregate output storage returned by the parser.
  std::uint16_t data_offset = 0;
  /// Number of entries contributed by this block to the aggregate output storage.
  std::uint16_t data_count = 0;
};
/// @}

/// \name Monitor Requests
/// @{
/// \brief Monitor registration payload used by `0801`.
struct MonitorRegistration {
  MonitorRegistration() = delete;
  constexpr MonitorRegistration(
      mcprotocol::serial::Span<const RandomReadWordItem> words,
      mcprotocol::serial::Span<const RandomReadDWordItem> dwords) noexcept
      : word_items(words), dword_items(dwords) {}

  /// Sparse 16-bit items registered first.
  mcprotocol::serial::Span<const RandomReadWordItem> word_items;
  /// Sparse 32-bit items registered second. Unsupported for 1C monitor commands.
  mcprotocol::serial::Span<const RandomReadDWordItem> dword_items;
};
/// @}

/// \name Serial-Module Dedicated Requests
/// @{
/// \brief User-frame registration-data read request (`0610`).
struct UserFrameRegistrationReadRequest {
  UserFrameRegistrationReadRequest() = delete;
  constexpr explicit UserFrameRegistrationReadRequest(std::uint16_t target_frame_no) noexcept
      : frame_no(target_frame_no) {}

  /// User-frame number to read, typically in the documented `0x0000..0x03FF` or `0x8001..0x801F` ranges.
  std::uint16_t frame_no;
};

/// \brief User-frame registration-data payload returned by `0610`.
struct UserFrameRegistrationData {
  /// Number of valid bytes in `registration_data`.
  std::uint16_t registration_data_bytes = 0;
  /// Optional frame-byte count returned by the target for the registered frame data.
  std::uint16_t frame_bytes = 0;
  /// Raw user-frame registration bytes as returned by the target.
  std::array<mcprotocol::serial::Byte, kMaxUserFrameRegistrationBytes> registration_data {};
};

/// \brief User-frame registration-data write request (`1610`, subcommand `0000`).
struct UserFrameRegistrationWriteRequest {
  UserFrameRegistrationWriteRequest() = delete;
  constexpr UserFrameRegistrationWriteRequest(
      std::uint16_t target_frame_no,
      std::uint16_t target_frame_bytes,
      mcprotocol::serial::Span<const mcprotocol::serial::Byte> target_registration_data) noexcept
      : frame_no(target_frame_no),
        frame_bytes(target_frame_bytes),
        registration_data(target_registration_data) {}

  /// User-frame number to overwrite.
  std::uint16_t frame_no;
  /// Frame-byte count encoded into the `1610` payload.
  std::uint16_t frame_bytes;
  /// Raw user-frame registration bytes to store.
  mcprotocol::serial::Span<const mcprotocol::serial::Byte> registration_data;
};

/// \brief User-frame registration-data delete request (`1610`, subcommand `0001`).
struct UserFrameRegistrationDeleteRequest {
  UserFrameRegistrationDeleteRequest() = delete;
  constexpr explicit UserFrameRegistrationDeleteRequest(std::uint16_t target_frame_no) noexcept
      : frame_no(target_frame_no) {}

  /// User-frame number to clear.
  std::uint16_t frame_no;
};

/// \brief One-release compatibility alias for `UserFrameRegistrationReadRequest`.
using UserFrameReadRequest [[deprecated("use UserFrameRegistrationReadRequest")]] =
    UserFrameRegistrationReadRequest;
/// \brief One-release compatibility alias for `UserFrameRegistrationWriteRequest`.
using UserFrameWriteRequest [[deprecated("use UserFrameRegistrationWriteRequest")]] =
    UserFrameRegistrationWriteRequest;
/// \brief One-release compatibility alias for `UserFrameRegistrationDeleteRequest`.
using UserFrameDeleteRequest [[deprecated("use UserFrameRegistrationDeleteRequest")]] =
    UserFrameRegistrationDeleteRequest;

/// \brief C24 global-signal ON/OFF request (`1618`).
struct GlobalSignalControlRequest {
  GlobalSignalControlRequest() = delete;
  constexpr GlobalSignalControlRequest(
      GlobalSignalTarget signal_target,
      BitValue signal_value) noexcept
      : target(signal_target), value(signal_value) {}

  /// Which global signal destination should be controlled.
  GlobalSignalTarget target;
  /// Explicit ON/OFF state.
  BitValue value;
};

/// \brief C24 mode switching request (`1612`).
///
/// The three `switch_*` flags form the documented switching instruction byte:
/// bit0 = mode number, bit1 = transmission setting, bit2 = communication speed.
/// When a flag is false, the C24 uses the Engineering tool setting for that field.
struct SerialModuleModeSwitchRequest {
  SerialModuleModeSwitchRequest() = delete;
  constexpr SerialModuleModeSwitchRequest(
      SerialModuleChannel target_channel,
      bool use_mode_no,
      bool use_transmission_setting,
      bool use_communication_speed,
      SerialModuleModeNo target_mode_no,
      std::uint8_t target_transmission_setting,
      SerialModuleCommunicationSpeed target_communication_speed) noexcept
      : channel(target_channel),
        switch_mode_no(use_mode_no),
        switch_transmission_setting(use_transmission_setting),
        switch_communication_speed(use_communication_speed),
        mode_no(target_mode_no),
        transmission_setting(target_transmission_setting),
        communication_speed(target_communication_speed) {}

  /// Target interface.
  SerialModuleChannel channel;
  /// `true` to use `mode_no` from this command.
  bool switch_mode_no;
  /// `true` to use `transmission_setting` from this command.
  bool switch_transmission_setting;
  /// `true` to use `communication_speed` from this command.
  bool switch_communication_speed;
  /// Operation mode number. The manual requires a valid non-zero value even when `switch_mode_no` is false.
  SerialModuleModeNo mode_no;
  /// Raw transmission-setting bit field used when `switch_transmission_setting` is true.
  std::uint8_t transmission_setting;
  /// Communication speed used when `switch_communication_speed` is true.
  SerialModuleCommunicationSpeed communication_speed;
};
/// @}

/// \name Buffer-Memory Requests
/// @{
/// \brief Host-buffer read request (`0613`).
struct HostBufferReadRequest {
  HostBufferReadRequest() = delete;
  constexpr HostBufferReadRequest(
      std::uint32_t first_address,
      std::uint16_t length_words) noexcept
      : start_address(first_address), word_length(length_words) {}

  /// Starting host-buffer word address.
  std::uint32_t start_address;
  /// Number of words to read.
  std::uint16_t word_length;
};

/// \brief Host-buffer write request (`1613`).
struct HostBufferWriteRequest {
  HostBufferWriteRequest() = delete;
  constexpr HostBufferWriteRequest(
      std::uint32_t first_address,
      mcprotocol::serial::Span<const std::uint16_t> write_words) noexcept
      : start_address(first_address), words(write_words) {}

  /// Starting host-buffer word address.
  std::uint32_t start_address;
  /// Caller-owned words written sequentially from `start_address`.
  mcprotocol::serial::Span<const std::uint16_t> words;
};

/// \brief Module-buffer byte read request (`0601` helper path).
struct ModuleBufferReadRequest {
  ModuleBufferReadRequest() = delete;
  constexpr ModuleBufferReadRequest(
      std::uint32_t first_address,
      std::uint16_t byte_count,
      std::uint16_t target_module_number) noexcept
      : start_address(first_address), bytes(byte_count), module_number(target_module_number) {}

  /// Starting module-buffer byte address.
  std::uint32_t start_address;
  /// Number of bytes to read.
  std::uint16_t bytes;
  /// Module number used by the addressed special-function module.
  std::uint16_t module_number;
};

/// \brief Module-buffer byte write request (`1601` helper path).
struct ModuleBufferWriteRequest {
  ModuleBufferWriteRequest() = delete;
  constexpr ModuleBufferWriteRequest(
      std::uint32_t first_address,
      std::uint16_t target_module_number,
      mcprotocol::serial::Span<const mcprotocol::serial::Byte> write_bytes) noexcept
      : start_address(first_address), module_number(target_module_number), bytes(write_bytes) {}

  /// Starting module-buffer byte address.
  std::uint32_t start_address;
  /// Module number used by the addressed special-function module.
  std::uint16_t module_number;
  /// Caller-owned raw bytes written starting at `start_address`.
  mcprotocol::serial::Span<const mcprotocol::serial::Byte> bytes;
};
/// @}

/// \name Diagnostic And Transport Helper Types
/// @{
/// \brief CPU-model response payload returned by `cpu-model`.
struct CpuModelInfo {
  /// Null-terminated CPU model name with trailing spaces already trimmed by the parser.
  std::array<char, kCpuModelNameLength + 1> model_name {};
  /// Raw model code returned by the target.
  std::uint16_t model_code = 0;
};

/// \brief Optional RS-485 callbacks used by the async client around TX start/end.
///
/// `on_tx_begin` and `on_tx_end` are installed as a pair. Leaving both null disables library-side
/// direction control; `user` may remain null even when the callback pair is installed.
struct Rs485Hooks {
  /// Callback fired immediately before the client expects TX to start.
  void (*on_tx_begin)(void* user) = nullptr;
  /// Matching callback fired after physical TX completion or abort is reported.
  void (*on_tx_end)(void* user) = nullptr;
  /// Opaque user pointer passed back to both callbacks.
  void* user = nullptr;
};

/// \brief Completion callback used by the async client.
///
/// The callback receives the original `user` pointer and the final request status.
using CompletionHandler = void (*)(void* user, Status status);
/// @}

}  // namespace mcprotocol::serial
