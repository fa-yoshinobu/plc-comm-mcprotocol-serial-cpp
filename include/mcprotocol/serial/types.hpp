#pragma once

#include "mcprotocol/serial/compat/array.hpp"
#include "mcprotocol/serial/compat/cstddef.hpp"
#include "mcprotocol/serial/compat/cstdint.hpp"

#include "mcprotocol/serial/span_compat.hpp"

#include "mcprotocol/serial/status.hpp"

/// \file types.hpp
/// \brief Public request, response, configuration, and callback types for the serial MC protocol library.
///
/// This header is the main data-model reference for the public API. It defines:
///
/// - protocol-selection and routing enums
/// - static buffer and feature-limit constants
/// - request/response payload structs used by `MelsecSerialClient`, `PosixSyncClient`, and `CommandCodec`
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

#ifndef MCPROTOCOL_SERIAL_ENABLE_FRAME_E1
#define MCPROTOCOL_SERIAL_ENABLE_FRAME_E1 1
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
/// \brief Named request-destination module I/O numbers used by `3C` / `4C` serial routing.
///
/// `RouteConfig::request_destination_module_io_no` defaults to `OwnStation`. The CPU constants
/// are useful when a `3C` / `4C` request is intentionally routed to a multi-CPU or redundant-CPU
/// target. The serial request header accepts the documented request-destination module I/O number
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
  /// Smallest ASCII serial frame supported by this repository.
  C2,
  /// Legacy ASCII serial frame with its own command naming and routing rules.
  C1,
  /// Legacy frame family used by chapter-18 style command layouts.
  E1
};

/// \brief Returns whether `frame_kind` is a defined public frame-family value.
[[nodiscard]] constexpr bool is_valid_frame_kind(FrameKind frame_kind) noexcept {
  switch (frame_kind) {
    case FrameKind::C4:
    case FrameKind::C3:
    case FrameKind::C2:
    case FrameKind::C1:
    case FrameKind::E1:
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

/// \brief Logical single-bit value used by bit read/write APIs.
enum class BitValue : std::uint8_t {
  Off = 0,
  On = 1
};

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

/// \brief Timeout settings used by the frame decoder and async client.
///
/// These values are transport-facing rather than command-facing:
///
/// - `response_timeout_ms` is the total request timeout once TX finishes
/// - `inter_byte_timeout_ms` is the gap timeout while RX is already in progress
struct TimeoutConfig {
  /// Maximum wait after TX completion before the request is treated as timed out.
  std::uint32_t response_timeout_ms = 5000;
  /// Maximum allowed idle gap between RX bytes once a response has started.
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

/// \brief Meaning of a 1E PC target.
enum class E1PcTargetKind : std::uint8_t {
  Number,
  ConnectedStation,
};

/// \brief Mandatory typed PC target for an explicit 1E route.
class E1PcTarget {
 public:
  [[nodiscard]] static constexpr E1PcTarget number(std::uint32_t value) noexcept {
    return E1PcTarget(E1PcTargetKind::Number, value);
  }
  [[nodiscard]] static constexpr E1PcTarget connected_station() noexcept {
    return E1PcTarget(E1PcTargetKind::ConnectedStation, 0xFFU);
  }

  [[nodiscard]] constexpr E1PcTargetKind kind() const noexcept { return kind_; }
  [[nodiscard]] constexpr std::uint32_t value() const noexcept { return value_; }
  [[nodiscard]] constexpr bool is_valid() const noexcept {
    switch (kind_) {
      case E1PcTargetKind::Number:
        return value_ >= 0x01U && value_ <= 0x40U;
      case E1PcTargetKind::ConnectedStation:
        return value_ == 0xFFU;
    }
    return false;
  }

 private:
  constexpr E1PcTarget(E1PcTargetKind kind, std::uint32_t value) noexcept
      : kind_(kind), value_(value) {}

  E1PcTargetKind kind_;
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

/// \brief Explicit 2C multidrop route with a mandatory station number.
///
/// The temporary enabled/number pair remains until D-103 replaces it with topology-specific types.
class C2MultidropRoute {
 public:
  constexpr explicit C2MultidropRoute(
      std::uint32_t station_no,
      bool self_station_enabled = false,
      std::uint8_t self_station_no = 0x00U) noexcept
      : station_no_(station_no),
        self_station_enabled_(self_station_enabled),
        self_station_no_(self_station_no) {}
  [[nodiscard]] constexpr std::uint32_t station_no() const noexcept { return station_no_; }
  [[nodiscard]] constexpr bool self_station_enabled() const noexcept {
    return self_station_enabled_;
  }
  [[nodiscard]] constexpr std::uint8_t self_station_no() const noexcept {
    return self_station_no_;
  }

 private:
  std::uint32_t station_no_;
  bool self_station_enabled_;
  std::uint8_t self_station_no_;
};

/// \brief Explicit 3C routed multidrop route with mandatory station and network numbers.
class C3MultidropRoute {
 public:
  constexpr C3MultidropRoute(
      std::uint32_t station_no,
      std::uint32_t network_no,
      C34PcTarget pc_target,
      bool self_station_enabled = false,
      std::uint8_t self_station_no = 0x00U) noexcept
      : station_no_(station_no),
        network_no_(network_no),
        pc_target_(pc_target),
        self_station_enabled_(self_station_enabled),
        self_station_no_(self_station_no) {}
  [[nodiscard]] constexpr std::uint32_t station_no() const noexcept { return station_no_; }
  [[nodiscard]] constexpr std::uint32_t network_no() const noexcept { return network_no_; }
  [[nodiscard]] constexpr C34PcTarget pc_target() const noexcept { return pc_target_; }
  [[nodiscard]] constexpr bool self_station_enabled() const noexcept {
    return self_station_enabled_;
  }
  [[nodiscard]] constexpr std::uint8_t self_station_no() const noexcept {
    return self_station_no_;
  }

 private:
  std::uint32_t station_no_;
  std::uint32_t network_no_;
  C34PcTarget pc_target_;
  bool self_station_enabled_;
  std::uint8_t self_station_no_;
};

/// \brief Explicit 4C routed multidrop route with mandatory station and network numbers.
class C4MultidropRoute {
 public:
  constexpr C4MultidropRoute(
      std::uint32_t station_no,
      std::uint32_t network_no,
      C34PcTarget pc_target,
      std::uint16_t module_io_no = module_io::OwnStation,
      std::uint8_t module_station_no = 0x00U,
      bool self_station_enabled = false,
      std::uint8_t self_station_no = 0x00U) noexcept
      : station_no_(station_no),
        network_no_(network_no),
        pc_target_(pc_target),
        module_io_no_(module_io_no),
        module_station_no_(module_station_no),
        self_station_enabled_(self_station_enabled),
        self_station_no_(self_station_no) {}
  [[nodiscard]] constexpr std::uint32_t station_no() const noexcept { return station_no_; }
  [[nodiscard]] constexpr std::uint32_t network_no() const noexcept { return network_no_; }
  [[nodiscard]] constexpr C34PcTarget pc_target() const noexcept { return pc_target_; }
  [[nodiscard]] constexpr std::uint16_t module_io_no() const noexcept { return module_io_no_; }
  [[nodiscard]] constexpr std::uint8_t module_station_no() const noexcept {
    return module_station_no_;
  }
  [[nodiscard]] constexpr bool self_station_enabled() const noexcept {
    return self_station_enabled_;
  }
  [[nodiscard]] constexpr std::uint8_t self_station_no() const noexcept {
    return self_station_no_;
  }

 private:
  std::uint32_t station_no_;
  std::uint32_t network_no_;
  C34PcTarget pc_target_;
  std::uint16_t module_io_no_;
  std::uint8_t module_station_no_;
  bool self_station_enabled_;
  std::uint8_t self_station_no_;
};

/// \brief Explicit non-default 1E route with a mandatory typed PC target.
class E1Route {
 public:
  constexpr explicit E1Route(E1PcTarget pc_target) noexcept : pc_target_(pc_target) {}
  [[nodiscard]] constexpr E1PcTarget pc_target() const noexcept { return pc_target_; }

 private:
  E1PcTarget pc_target_;
};

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
  constexpr explicit RouteConfig(C2MultidropRoute route) noexcept
      : kind_(RouteKind::MultidropStation), route_frame_(FrameKind::C2),
        station_no_(route.station_no()), self_station_enabled_(route.self_station_enabled()),
        self_station_no_(route.self_station_no()) {}
  constexpr explicit RouteConfig(C3MultidropRoute route) noexcept
      : kind_(RouteKind::MultidropStation), route_frame_(FrameKind::C3),
        station_no_(route.station_no()), network_no_(route.network_no()),
        pc_no_(route.pc_target().value()), pc_target_valid_(route.pc_target().is_valid()),
        self_station_enabled_(route.self_station_enabled()), self_station_no_(route.self_station_no()) {}
  constexpr explicit RouteConfig(C4MultidropRoute route) noexcept
      : kind_(RouteKind::MultidropStation), route_frame_(FrameKind::C4),
        station_no_(route.station_no()), network_no_(route.network_no()),
        pc_no_(route.pc_target().value()), pc_target_valid_(route.pc_target().is_valid()),
        module_io_no_(route.module_io_no()), module_station_no_(route.module_station_no()),
        self_station_enabled_(route.self_station_enabled()), self_station_no_(route.self_station_no()) {}
  constexpr explicit RouteConfig(E1Route route) noexcept
      : kind_(RouteKind::MultidropStation), route_frame_(FrameKind::E1),
        pc_no_(route.pc_target().value()), pc_target_valid_(route.pc_target().is_valid()) {}

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
  [[nodiscard]] constexpr std::uint16_t request_destination_module_io_no() const noexcept {
    return is_multidrop() ? module_io_no_ : module_io::OwnStation;
  }
  [[nodiscard]] constexpr std::uint8_t request_destination_module_station_no() const noexcept {
    return is_multidrop() ? module_station_no_ : 0x00U;
  }
  [[nodiscard]] constexpr bool self_station_enabled() const noexcept {
    return is_multidrop() && self_station_enabled_;
  }
  [[nodiscard]] constexpr std::uint8_t self_station_no() const noexcept {
    return self_station_enabled() ? self_station_no_ : 0x00U;
  }

 private:
  friend class FrameCodec;

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
  std::uint16_t module_io_no_ = module_io::OwnStation;
  std::uint8_t module_station_no_ = 0x00U;
  bool self_station_enabled_ = false;
  std::uint8_t self_station_no_ = 0x00U;
};

/// \brief Top-level protocol configuration shared by codecs and client requests.
///
/// Treat this as the immutable session configuration for one serial link. The same object is used
/// by:
///
/// - `FrameCodec` for frame wrapping and response decoding
/// - `CommandCodec` for command subcommand/device-layout differences
/// - `MelsecSerialClient` and `PosixSyncClient` for runtime request execution
struct ProtocolConfig {
  /// Selected serial frame family. A named preset or caller must assign a defined value.
  FrameKind frame_kind = static_cast<FrameKind>(0xFF);
  /// Selected payload encoding. A named preset or caller must assign a defined value.
  CodeMode code_mode = static_cast<CodeMode>(0xFF);
  /// Selected ASCII framing flavor. ASCII callers must assign a defined value.
  AsciiFormat ascii_format = static_cast<AsciiFormat>(0xFF);
  /// Public PLC profile used to derive frame-family compatibility and device/subcommand layout.
  ///
  /// Applications must set this explicitly before encoding requests or running a client.
  PlcProfile plc_profile = PlcProfile::Unspecified;
  /// Explicit ASCII/binary sum-check policy where the selected frame family supports it.
  ///
  /// The invalid initial value ensures generic aggregate construction cannot silently enable a
  /// checksum. Use a named protocol preset or assign `Enabled`/`Disabled` explicitly.
  SumCheckMode sum_check_mode = static_cast<SumCheckMode>(0xFF);
  /// Route header fields used for every encoded request.
  RouteConfig route {};
  /// Request timeout policy used by the async client and stream decoder.
  TimeoutConfig timeout {};
};

/// \brief Device code plus numeric address.
///
/// This is the normalized address form used throughout the library after string-address parsing.
struct DeviceAddress {
  /// Device family such as `D`, `M`, `X`, `LTN`, or `LZ`.
  DeviceCode code = DeviceCode::D;
  /// Numeric index inside the selected device family.
  std::uint32_t number = 0;
};

/// \brief Extended file-register address using block number plus `R` word number.
///
/// This is the block-addressed form used by `1C ACPU-common` and by the chapter-18 block path on
/// `1E`.
struct ExtendedFileRegisterAddress {
  /// Extended file-register block number.
  std::uint16_t block_number = 1;
  /// Word number inside the selected block.
  std::uint16_t word_number = 0;
};

/// \name Device-Memory Contiguous Requests
/// @{
/// \brief Contiguous word-read request (`0401`).
struct BatchReadWordsRequest {
  /// First device in the contiguous range.
  DeviceAddress head_device {};
  /// Number of points to read starting at `head_device`.
  std::uint16_t points = 0;
};

/// \brief Contiguous bit-read request (`0401` bit path).
struct BatchReadBitsRequest {
  /// First bit device in the contiguous range.
  DeviceAddress head_device {};
  /// Number of bit points to read starting at `head_device`.
  std::uint16_t points = 0;
};

/// \brief Contiguous word-write request (`1401`).
struct BatchWriteWordsRequest {
  /// First device in the contiguous write range.
  DeviceAddress head_device {};
  /// Caller-owned word data to write starting at `head_device`.
  std::span<const std::uint16_t> words {};
};

/// \brief Contiguous bit-write request (`1401` bit path).
struct BatchWriteBitsRequest {
  /// First bit device in the contiguous write range.
  DeviceAddress head_device {};
  /// Caller-owned bit data to write starting at `head_device`.
  std::span<const BitValue> bits {};
};
/// @}

/// \name Extended File-Register Requests
/// @{
/// \brief Extended file-register batch read (`ER` on 1C ACPU-common, chapter-18 block path on 1E).
struct ExtendedFileRegisterBatchReadWordsRequest {
  /// First block-addressed file-register word to read.
  ExtendedFileRegisterAddress head_device {};
  /// Number of words to read from the file-register range.
  std::uint16_t points = 0;
};

/// \brief Direct extended file-register batch read (`NR` on 1C AnA/AnUCPU common, chapter-18 direct path on 1E).
struct ExtendedFileRegisterDirectBatchReadWordsRequest {
  /// \brief `NR/NW` direct address on 1C or the chapter-18 direct `R` address on 1E.
  std::uint32_t head_device_number = 0;
  /// Number of words to read from the direct file-register range.
  std::uint16_t points = 0;
};

/// \brief Extended file-register batch write (`EW` on 1C ACPU-common, chapter-18 block path on 1E).
struct ExtendedFileRegisterBatchWriteWordsRequest {
  /// First block-addressed file-register word to write.
  ExtendedFileRegisterAddress head_device {};
  /// Caller-owned word data to write starting at `head_device`.
  std::span<const std::uint16_t> words {};
};

/// \brief Direct extended file-register batch write (`NW` on 1C AnA/AnUCPU common, chapter-18 direct path on 1E).
struct ExtendedFileRegisterDirectBatchWriteWordsRequest {
  /// \brief `NR/NW` direct address on 1C or the chapter-18 direct `R` address on 1E.
  std::uint32_t head_device_number = 0;
  /// Caller-owned word data to write starting at `head_device_number`.
  std::span<const std::uint16_t> words {};
};

/// \brief One item inside extended file-register random write (`ET` on 1C, chapter-18 on 1E).
struct ExtendedFileRegisterRandomWriteWordItem {
  /// Target extended file-register address.
  ExtendedFileRegisterAddress device {};
  /// One word written to `device`.
  std::uint16_t value = 0;
};

/// \brief Extended file-register monitor registration (`EM` on 1C, chapter-18 on 1E).
struct ExtendedFileRegisterMonitorRegistration {
  /// Sparse list of block-addressed file-register items to register for monitoring.
  std::span<const ExtendedFileRegisterAddress> items {};
};
/// @}

/// \name Device-Memory Random And Multi-Block Requests
/// @{
/// \brief One item inside a native random-read request (`0403` or monitor registration).
struct RandomReadItem {
  /// Target device address for this sparse item.
  DeviceAddress device {};
  /// `true` when the item should be encoded as a double-word device access.
  bool double_word = false;
};

/// \brief Native random-read request made of sparse word/dword items.
struct RandomReadRequest {
  /// Sparse word/dword items encoded in the native random-read request.
  std::span<const RandomReadItem> items {};
};

/// \brief One word or double-word item inside native random write (`1402` word path).
struct RandomWriteWordItem {
  /// Target device address for the sparse write.
  DeviceAddress device {};
  /// One word or double-word value to write.
  std::uint32_t value = 0;
  /// `true` when the target is encoded as a double-word write item.
  bool double_word = false;
};

/// \brief One bit item inside native random write (`1402` bit path).
struct RandomWriteBitItem {
  /// Target bit device address for the sparse write.
  DeviceAddress device {};
  /// Bit value written to `device`.
  BitValue value = BitValue::Off;
};

/// \brief One block inside native multi-block read (`0406`).
struct MultiBlockReadBlock {
  /// First device in this contiguous block.
  DeviceAddress head_device {};
  /// Number of points in this block.
  std::uint16_t points = 0;
  /// `true` for bit blocks, `false` for word blocks.
  bool bit_block = false;
};

/// \brief Native multi-block read request composed of multiple contiguous blocks.
struct MultiBlockReadRequest {
  /// Ordered block list encoded into the native multi-block read request.
  std::span<const MultiBlockReadBlock> blocks {};
};

/// \brief One block inside native multi-block write (`1406`).
struct MultiBlockWriteBlock {
  /// First device in this contiguous block.
  DeviceAddress head_device {};
  /// Point count for this block.
  std::uint16_t points = 0;
  /// `true` when `bits` is used, `false` when `words` is used.
  bool bit_block = false;
  /// Caller-owned word data for word blocks.
  std::span<const std::uint16_t> words {};
  /// Caller-owned bit data for bit blocks.
  std::span<const BitValue> bits {};
};

/// \brief Native multi-block write request composed of multiple contiguous blocks.
struct MultiBlockWriteRequest {
  /// Ordered block list encoded into the native multi-block write request.
  std::span<const MultiBlockWriteBlock> blocks {};
};

/// \brief Parsed layout description for one block returned by `parse_multi_block_read_response()`.
struct MultiBlockReadBlockResult {
  /// Block kind copied from the original request.
  bool bit_block = false;
  /// Block head device copied from the original request.
  DeviceAddress head_device {};
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
  /// Sparse list of word/dword items to register for a later `0802` read.
  std::span<const RandomReadItem> items {};
};
/// @}

/// \name Serial-Module Dedicated Requests
/// @{
/// \brief User-frame registration-data read request (`0610`).
struct UserFrameReadRequest {
  /// User-frame number to read, typically in the documented `0x0000..0x03FF` or `0x8001..0x801F` ranges.
  std::uint16_t frame_no = 0;
};

/// \brief User-frame registration-data payload returned by `0610`.
struct UserFrameRegistrationData {
  /// Number of valid bytes in `registration_data`.
  std::uint16_t registration_data_bytes = 0;
  /// Optional frame-byte count returned by the target for the registered frame data.
  std::uint16_t frame_bytes = 0;
  /// Raw user-frame registration bytes as returned by the target.
  std::array<std::byte, kMaxUserFrameRegistrationBytes> registration_data {};
};

/// \brief User-frame registration-data write request (`1610`, subcommand `0000`).
struct UserFrameWriteRequest {
  /// User-frame number to overwrite.
  std::uint16_t frame_no = 0;
  /// Frame-byte count encoded into the `1610` payload.
  std::uint16_t frame_bytes = 0;
  /// Raw user-frame registration bytes to store.
  std::span<const std::byte> registration_data {};
};

/// \brief User-frame registration-data delete request (`1610`, subcommand `0001`).
struct UserFrameDeleteRequest {
  /// User-frame number to clear.
  std::uint16_t frame_no = 0;
};

/// \brief C24 global-signal ON/OFF request (`1618`).
struct GlobalSignalControlRequest {
  /// Which global signal destination should be controlled.
  GlobalSignalTarget target = GlobalSignalTarget::ReceivedSide;
  /// `true` for ON, `false` for OFF.
  bool turn_on = false;
};

/// \brief C24 mode switching request (`1612`).
///
/// The three `switch_*` flags form the documented switching instruction byte:
/// bit0 = mode number, bit1 = transmission setting, bit2 = communication speed.
/// When a flag is false, the C24 uses the Engineering tool setting for that field.
struct SerialModuleModeSwitchRequest {
  /// Target interface.
  SerialModuleChannel channel = SerialModuleChannel::Ch1;
  /// `true` to use `mode_no` from this command.
  bool switch_mode_no = false;
  /// `true` to use `transmission_setting` from this command.
  bool switch_transmission_setting = false;
  /// `true` to use `communication_speed` from this command.
  bool switch_communication_speed = false;
  /// Operation mode number. The manual requires a valid non-zero value even when `switch_mode_no` is false.
  SerialModuleModeNo mode_no = SerialModuleModeNo::McProtocolFormat1;
  /// Raw transmission-setting bit field used when `switch_transmission_setting` is true.
  std::uint8_t transmission_setting = 0;
  /// Communication speed used when `switch_communication_speed` is true.
  SerialModuleCommunicationSpeed communication_speed = SerialModuleCommunicationSpeed::Bps300;
};
/// @}

/// \name Buffer-Memory Requests
/// @{
/// \brief Host-buffer read request (`0613`).
struct HostBufferReadRequest {
  /// Starting host-buffer word address.
  std::uint32_t start_address = 0;
  /// Number of words to read.
  std::uint16_t word_length = 0;
};

/// \brief Host-buffer write request (`1613`).
struct HostBufferWriteRequest {
  /// Starting host-buffer word address.
  std::uint32_t start_address = 0;
  /// Caller-owned words written sequentially from `start_address`.
  std::span<const std::uint16_t> words {};
};

/// \brief Module-buffer byte read request (`0601` helper path).
struct ModuleBufferReadRequest {
  /// Starting module-buffer byte address.
  std::uint32_t start_address = 0;
  /// Number of bytes to read.
  std::uint16_t bytes = 0;
  /// Module number used by the addressed special-function module.
  std::uint16_t module_number = 0;
};

/// \brief Module-buffer byte write request (`1601` helper path).
struct ModuleBufferWriteRequest {
  /// Starting module-buffer byte address.
  std::uint32_t start_address = 0;
  /// Module number used by the addressed special-function module.
  std::uint16_t module_number = 0;
  /// Caller-owned raw bytes written starting at `start_address`.
  std::span<const std::byte> bytes {};
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
struct Rs485Hooks {
  /// Optional callback fired immediately before the client expects TX to start.
  void (*on_tx_begin)(void* user) = nullptr;
  /// Optional callback fired after TX completion or after cleanup on failure/cancel.
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
