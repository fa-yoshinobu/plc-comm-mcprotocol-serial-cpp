#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

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

/// \brief Request/response payload encoding.
enum class CodeMode : std::uint8_t {
  /// Text-encoded command data and response data.
  Ascii,
  /// Compact binary command data and response data.
  Binary
};

/// \brief ASCII formatting variant for `C4` / `C3` / `C2` serial frames.
enum class AsciiFormat : std::uint8_t {
  /// ENQ/STX/ETX layout without CR/LF.
  Format1,
  /// STX-only layout commonly used on serial MC links.
  Format3,
  /// CR/LF terminated layout often used by host-facing bring-up tools.
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
  /// Host-station route with fixed `station=0`, `network=0`, and `pc=FF`.
  HostStation,
  /// Multidrop route where the destination station number is encoded in the frame.
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

/// \brief Route header fields for serial MC requests.
///
/// The same struct is shared across `2C`/`3C`/`4C`, `1C`, and `1E`, but not every field is active
/// on every frame family. `FrameCodec::validate_config()` checks the combinations that are legal for
/// the selected frame.
struct RouteConfig {
  /// Route interpretation used by the selected frame family.
  RouteKind kind = RouteKind::HostStation;
  /// Target station number on multidrop serial links.
  std::uint8_t station_no = 0x00;
  /// Network number used by routed `3C/4C` requests.
  std::uint8_t network_no = 0x00;
  /// PLC number field used by `3C/4C` and legacy frame families.
  std::uint8_t pc_no = 0xFF;
  /// Destination I/O number for the target CPU/module in `3C/4C` routing.
  std::uint16_t request_destination_module_io_no = 0x03FF;
  /// Destination station number for the target CPU/module in `3C/4C` routing.
  std::uint8_t request_destination_module_station_no = 0x00;
  /// Enables self-station routing on frame families that support it.
  bool self_station_enabled = false;
  /// Self-station number used when `self_station_enabled` is true.
  std::uint8_t self_station_no = 0x00;
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
  /// Selected serial frame family.
  FrameKind frame_kind = FrameKind::C4;
  /// Selected payload encoding inside the frame.
  CodeMode code_mode = CodeMode::Binary;
  /// Selected ASCII framing flavor when `code_mode == CodeMode::Ascii`.
  AsciiFormat ascii_format = AsciiFormat::Format3;
  /// PLC family used for device and subcommand differences.
  PlcSeries target_series = PlcSeries::Q_L;
  /// Enables or disables the ASCII/binary sum-check where that frame family supports it.
  bool sum_check_enabled = true;
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

/// \brief Direct extended file-register batch read (`NR` on 1C QnA-common, chapter-18 direct path on 1E).
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

/// \brief Direct extended file-register batch write (`NW` on 1C QnA-common, chapter-18 direct path on 1E).
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
  /// Station number encoded in the `1618` specification word.
  std::uint8_t station_no = 0;
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
