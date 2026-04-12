#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "mcprotocol/serial/span_compat.hpp"

#include "mcprotocol/serial/status.hpp"

namespace mcprotocol::serial {

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
#define MCPROTOCOL_SERIAL_MAX_BATCH_BIT_POINTS_BINARY 3584U
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
constexpr std::size_t kCpuModelNameLength = 16;

/// \brief MC protocol frame family used on the serial link.
enum class FrameKind : std::uint8_t {
  C4,
  C3,
  C2,
  C1
};

/// \brief Request/response payload encoding.
enum class CodeMode : std::uint8_t {
  Ascii,
  Binary
};

/// \brief ASCII formatting variant for `C4` / `C3` serial frames.
enum class AsciiFormat : std::uint8_t {
  Format1,
  Format3,
  Format4
};

/// \brief PLC family selection used for subcommand and device-layout differences.
enum class PlcSeries : std::uint8_t {
  IQ_R,
  IQ_L,
  Q_L,
  QnA,
  A
};

/// \brief Route layout inside the request header.
enum class RouteKind : std::uint8_t {
  HostStation,
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

/// \brief Decoded PLC response class before command-specific parsing.
enum class ResponseType : std::uint8_t {
  SuccessData,
  SuccessNoData,
  PlcError
};

/// \brief Timeout settings used by the frame decoder and async client.
struct TimeoutConfig {
  std::uint32_t response_timeout_ms = 5000;
  std::uint32_t inter_byte_timeout_ms = 250;
};

/// \brief Route header fields for serial MC requests.
struct RouteConfig {
  RouteKind kind = RouteKind::HostStation;
  std::uint8_t station_no = 0x00;
  std::uint8_t network_no = 0x00;
  std::uint8_t pc_no = 0xFF;
  std::uint16_t request_destination_module_io_no = 0x03FF;
  std::uint8_t request_destination_module_station_no = 0x00;
  bool self_station_enabled = false;
  std::uint8_t self_station_no = 0x00;
};

/// \brief Top-level protocol configuration shared by codecs and client requests.
struct ProtocolConfig {
  FrameKind frame_kind = FrameKind::C4;
  CodeMode code_mode = CodeMode::Binary;
  AsciiFormat ascii_format = AsciiFormat::Format3;
  PlcSeries target_series = PlcSeries::Q_L;
  bool sum_check_enabled = true;
  RouteConfig route {};
  TimeoutConfig timeout {};
};

/// \brief Device code plus numeric address.
struct DeviceAddress {
  DeviceCode code = DeviceCode::D;
  std::uint32_t number = 0;
};

/// \brief Contiguous word-read request (`0401`).
struct BatchReadWordsRequest {
  DeviceAddress head_device {};
  std::uint16_t points = 0;
};

/// \brief Contiguous bit-read request (`0401` bit path).
struct BatchReadBitsRequest {
  DeviceAddress head_device {};
  std::uint16_t points = 0;
};

/// \brief Contiguous word-write request (`1401`).
struct BatchWriteWordsRequest {
  DeviceAddress head_device {};
  std::span<const std::uint16_t> words {};
};

/// \brief Contiguous bit-write request (`1401` bit path).
struct BatchWriteBitsRequest {
  DeviceAddress head_device {};
  std::span<const BitValue> bits {};
};

/// \brief One item inside a native random-read request (`0403` or monitor registration).
struct RandomReadItem {
  DeviceAddress device {};
  bool double_word = false;
};

/// \brief Native random-read request made of sparse word/dword items.
struct RandomReadRequest {
  std::span<const RandomReadItem> items {};
};

/// \brief One word or double-word item inside native random write (`1402` word path).
struct RandomWriteWordItem {
  DeviceAddress device {};
  std::uint32_t value = 0;
  bool double_word = false;
};

/// \brief One bit item inside native random write (`1402` bit path).
struct RandomWriteBitItem {
  DeviceAddress device {};
  BitValue value = BitValue::Off;
};

/// \brief One block inside native multi-block read (`0406`).
struct MultiBlockReadBlock {
  DeviceAddress head_device {};
  std::uint16_t points = 0;
  bool bit_block = false;
};

/// \brief Native multi-block read request composed of multiple contiguous blocks.
struct MultiBlockReadRequest {
  std::span<const MultiBlockReadBlock> blocks {};
};

/// \brief One block inside native multi-block write (`1406`).
struct MultiBlockWriteBlock {
  DeviceAddress head_device {};
  std::uint16_t points = 0;
  bool bit_block = false;
  std::span<const std::uint16_t> words {};
  std::span<const BitValue> bits {};
};

/// \brief Native multi-block write request composed of multiple contiguous blocks.
struct MultiBlockWriteRequest {
  std::span<const MultiBlockWriteBlock> blocks {};
};

/// \brief Parsed layout description for one block returned by `parse_multi_block_read_response()`.
struct MultiBlockReadBlockResult {
  bool bit_block = false;
  DeviceAddress head_device {};
  std::uint16_t points = 0;
  std::uint16_t data_offset = 0;
  std::uint16_t data_count = 0;
};

/// \brief Monitor registration payload used by `0801`.
struct MonitorRegistration {
  std::span<const RandomReadItem> items {};
};

/// \brief Host-buffer read request (`0613`).
struct HostBufferReadRequest {
  std::uint32_t start_address = 0;
  std::uint16_t word_length = 0;
};

/// \brief Host-buffer write request (`1613`).
struct HostBufferWriteRequest {
  std::uint32_t start_address = 0;
  std::span<const std::uint16_t> words {};
};

/// \brief Module-buffer byte read request (`0601` helper path).
struct ModuleBufferReadRequest {
  std::uint32_t start_address = 0;
  std::uint16_t bytes = 0;
  std::uint16_t module_number = 0;
};

/// \brief Module-buffer byte write request (`1601` helper path).
struct ModuleBufferWriteRequest {
  std::uint32_t start_address = 0;
  std::uint16_t module_number = 0;
  std::span<const std::byte> bytes {};
};

/// \brief CPU-model response payload returned by `cpu-model`.
struct CpuModelInfo {
  std::array<char, kCpuModelNameLength + 1> model_name {};
  std::uint16_t model_code = 0;
};

/// \brief Optional RS-485 callbacks used by the async client around TX start/end.
struct Rs485Hooks {
  void (*on_tx_begin)(void* user) = nullptr;
  void (*on_tx_end)(void* user) = nullptr;
  void* user = nullptr;
};

/// \brief Completion callback used by the async client.
///
/// The callback receives the original `user` pointer and the final request status.
using CompletionHandler = void (*)(void* user, Status status);

}  // namespace mcprotocol::serial
