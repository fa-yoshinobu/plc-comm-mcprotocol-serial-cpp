#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "mcprotocol/serial/status.hpp"

namespace mcprotocol::serial {

constexpr std::size_t kMaxRequestFrameBytes = 4096;
constexpr std::size_t kMaxResponseFrameBytes = 4096;
constexpr std::size_t kMaxRequestDataBytes = 3500;
constexpr std::size_t kMaxBatchWordPoints = 960;
constexpr std::size_t kMaxBatchBitPointsAscii = 7904;
constexpr std::size_t kMaxBatchBitPointsBinary = 3584;
constexpr std::size_t kMaxRandomAccessItems = 192;
constexpr std::size_t kMaxMultiBlockCount = 120;
constexpr std::size_t kMaxMonitorItems = 192;
constexpr std::size_t kMaxLoopbackBytes = 960;
constexpr std::size_t kCpuModelNameLength = 16;

enum class FrameKind : std::uint8_t {
  C4,
  C3,
  C2,
  C1
};

enum class CodeMode : std::uint8_t {
  Ascii,
  Binary
};

enum class AsciiFormat : std::uint8_t {
  Format1,
  Format3,
  Format4
};

enum class PlcSeries : std::uint8_t {
  IQ_R,
  IQ_L,
  Q_L,
  QnA,
  A
};

enum class RouteKind : std::uint8_t {
  HostStation,
  MultidropStation
};

enum class DeviceCode : std::uint8_t {
  X,
  Y,
  M,
  L,
  F,
  V,
  B,
  D,
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
  Z,
  R,
  ZR,
  RD
};

enum class BitValue : std::uint8_t {
  Off = 0,
  On = 1
};

enum class ResponseType : std::uint8_t {
  SuccessData,
  SuccessNoData,
  PlcError
};

struct TimeoutConfig {
  std::uint32_t response_timeout_ms = 5000;
  std::uint32_t inter_byte_timeout_ms = 250;
};

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

struct ProtocolConfig {
  FrameKind frame_kind = FrameKind::C4;
  CodeMode code_mode = CodeMode::Binary;
  AsciiFormat ascii_format = AsciiFormat::Format3;
  PlcSeries target_series = PlcSeries::Q_L;
  bool sum_check_enabled = true;
  RouteConfig route {};
  TimeoutConfig timeout {};
};

struct DeviceAddress {
  DeviceCode code = DeviceCode::D;
  std::uint32_t number = 0;
};

struct BatchReadWordsRequest {
  DeviceAddress head_device {};
  std::uint16_t points = 0;
};

struct BatchReadBitsRequest {
  DeviceAddress head_device {};
  std::uint16_t points = 0;
};

struct BatchWriteWordsRequest {
  DeviceAddress head_device {};
  std::span<const std::uint16_t> words {};
};

struct BatchWriteBitsRequest {
  DeviceAddress head_device {};
  std::span<const BitValue> bits {};
};

struct RandomReadItem {
  DeviceAddress device {};
  bool double_word = false;
};

struct RandomReadRequest {
  std::span<const RandomReadItem> items {};
};

struct RandomWriteWordItem {
  DeviceAddress device {};
  std::uint32_t value = 0;
  bool double_word = false;
};

struct RandomWriteBitItem {
  DeviceAddress device {};
  BitValue value = BitValue::Off;
};

struct MultiBlockReadBlock {
  DeviceAddress head_device {};
  std::uint16_t points = 0;
  bool bit_block = false;
};

struct MultiBlockReadRequest {
  std::span<const MultiBlockReadBlock> blocks {};
};

struct MultiBlockWriteBlock {
  DeviceAddress head_device {};
  std::uint16_t points = 0;
  bool bit_block = false;
  std::span<const std::uint16_t> words {};
  std::span<const BitValue> bits {};
};

struct MultiBlockWriteRequest {
  std::span<const MultiBlockWriteBlock> blocks {};
};

struct MultiBlockReadBlockResult {
  bool bit_block = false;
  DeviceAddress head_device {};
  std::uint16_t points = 0;
  std::uint16_t data_offset = 0;
  std::uint16_t data_count = 0;
};

struct MonitorRegistration {
  std::span<const RandomReadItem> items {};
};

struct HostBufferReadRequest {
  std::uint32_t start_address = 0;
  std::uint16_t word_length = 0;
};

struct HostBufferWriteRequest {
  std::uint32_t start_address = 0;
  std::span<const std::uint16_t> words {};
};

struct ModuleBufferReadRequest {
  std::uint32_t start_address = 0;
  std::uint16_t bytes = 0;
  std::uint16_t module_number = 0;
};

struct ModuleBufferWriteRequest {
  std::uint32_t start_address = 0;
  std::uint16_t module_number = 0;
  std::span<const std::byte> bytes {};
};

struct CpuModelInfo {
  std::array<char, kCpuModelNameLength + 1> model_name {};
  std::uint16_t model_code = 0;
};

struct Rs485Hooks {
  void (*on_tx_begin)(void* user) = nullptr;
  void (*on_tx_end)(void* user) = nullptr;
  void* user = nullptr;
};

using CompletionHandler = void (*)(void* user, Status status);

}  // namespace mcprotocol::serial
