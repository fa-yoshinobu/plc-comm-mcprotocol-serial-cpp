#pragma once

#include "mcprotocol/serial/compat/array.hpp"
#include "mcprotocol/serial/compat/cstddef.hpp"
#include "mcprotocol/serial/compat/cstdint.hpp"

#include "mcprotocol/serial/client.hpp"
#include "mcprotocol/serial/detail/parse_helpers.hpp"
#include "mcprotocol/serial/span.hpp"
#include "mcprotocol/serial/status.hpp"
#include "mcprotocol/serial/string_view_compat.hpp"
#include "mcprotocol/serial/types.hpp"

namespace mcprotocol::serial::highlevel {

/// \file high_level.hpp
/// \brief Optional string-address helpers and protocol presets for common library entry paths.
///
/// This layer does not replace `MelsecSerialClient`. It only reduces setup and request-building
/// friction for the most common cases:
///
/// - choose a baseline `ProtocolConfig`
/// - parse `D100`, `M100`, `X10`, `B20` style addresses
/// - build contiguous and sparse request items without hand-filling `DeviceAddress`
///
/// The current parser accepts plain device strings only. If this layer grows typed named-address
/// views later, `:` is reserved for data types such as `D100:D`, while `.` is reserved for
/// bit-in-word access such as `D100.D` meaning bit `0xD` / bit 13.

namespace detail {

struct DeviceParseSpec {
  const char* prefix;
  std::size_t prefix_length;
  DeviceCode code;
  int base;
};

constexpr std::array<DeviceParseSpec, 38> kDeviceParseSpecs {{
    {"STS", 3U, DeviceCode::STS, 10},
    {"STC", 3U, DeviceCode::STC, 10},
    {"STN", 3U, DeviceCode::STN, 10},
    {"TS", 2U, DeviceCode::TS, 10},
    {"TC", 2U, DeviceCode::TC, 10},
    {"TN", 2U, DeviceCode::TN, 10},
    {"CS", 2U, DeviceCode::CS, 10},
    {"CC", 2U, DeviceCode::CC, 10},
    {"CN", 2U, DeviceCode::CN, 10},
    {"SB", 2U, DeviceCode::SB, 16},
    {"SW", 2U, DeviceCode::SW, 16},
    {"SM", 2U, DeviceCode::SM, 10},
    {"SD", 2U, DeviceCode::SD, 10},
    {"DX", 2U, DeviceCode::DX, 16},
    {"DY", 2U, DeviceCode::DY, 16},
    {"LTS", 3U, DeviceCode::LTS, 10},
    {"LTC", 3U, DeviceCode::LTC, 10},
    {"LTN", 3U, DeviceCode::LTN, 10},
    {"LSTS", 4U, DeviceCode::LSTS, 10},
    {"LSTC", 4U, DeviceCode::LSTC, 10},
    {"LSTN", 4U, DeviceCode::LSTN, 10},
    {"LCS", 3U, DeviceCode::LCS, 10},
    {"LCC", 3U, DeviceCode::LCC, 10},
    {"LCN", 3U, DeviceCode::LCN, 10},
    {"LZ", 2U, DeviceCode::LZ, 10},
    {"RD", 2U, DeviceCode::RD, 10},
    {"ZR", 2U, DeviceCode::ZR, 10},
    {"X", 1U, DeviceCode::X, 16},
    {"Y", 1U, DeviceCode::Y, 16},
    {"M", 1U, DeviceCode::M, 10},
    {"L", 1U, DeviceCode::L, 10},
    {"F", 1U, DeviceCode::F, 10},
    {"V", 1U, DeviceCode::V, 10},
    {"B", 1U, DeviceCode::B, 16},
    {"D", 1U, DeviceCode::D, 10},
    {"W", 1U, DeviceCode::W, 16},
    {"Z", 1U, DeviceCode::Z, 10},
    {"R", 1U, DeviceCode::R, 10},
}};

using mcprotocol::serial::detail::ascii_upper;
using mcprotocol::serial::detail::parse_u32;

[[nodiscard]] constexpr bool is_bit_device_code(DeviceCode code) noexcept {
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
    case DeviceCode::LTS:
    case DeviceCode::LTC:
    case DeviceCode::LSTS:
    case DeviceCode::LSTC:
    case DeviceCode::LCS:
    case DeviceCode::LCC:
      return true;
    default:
      return false;
  }
}

[[nodiscard]] constexpr bool is_dword_only_device_code(DeviceCode code) noexcept {
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

}  // namespace detail

/// \brief Returns a practical `Format5 / Binary / C4` configuration for an explicit PLC profile.
[[nodiscard]] constexpr ProtocolConfig make_c4_binary_protocol(
    PlcProfile profile,
    SumCheckMode sum_check_mode,
    RouteConfig route) noexcept {
  return ProtocolConfig::c4_binary(profile, sum_check_mode, route);
}

/// \brief Returns a practical `Format4 / ASCII / C4` configuration for an explicit PLC profile.
[[nodiscard]] constexpr ProtocolConfig make_c4_ascii_format4_protocol(
    PlcProfile profile,
    SumCheckMode sum_check_mode,
    RouteConfig route) noexcept {
  return ProtocolConfig::ascii(
      mcprotocol::serial::AsciiFrameKind::C4, AsciiFormat::Format4, profile, sum_check_mode, route);
}

/// \brief Returns a `Format2 / ASCII / C4` configuration with explicit profile and sum-check mode.
///
/// `Format2` is the `Format1` style `ENQ/ACK/NAK/STX/ETX` link with an extra 1-byte block
/// number inserted before the frame ID. Block-number lifecycle is addressed separately by D-096.
[[nodiscard]] constexpr ProtocolConfig make_c4_ascii_format2_protocol(
    PlcProfile profile,
    SumCheckMode sum_check_mode,
    RouteConfig route) noexcept {
  return ProtocolConfig::ascii(
      mcprotocol::serial::AsciiFrameKind::C4, AsciiFormat::Format2, profile, sum_check_mode, route);
}

/// \brief String-address spec used to build sparse random-read or monitor requests.
struct RandomReadWordSpec {
  RandomReadWordSpec() = delete;
  constexpr explicit RandomReadWordSpec(std::string_view target_device) noexcept
      : device(target_device) {}

  /// Plain device string such as `D100` selected explicitly as 16-bit.
  std::string_view device;
};

/// \brief String-address spec selected explicitly for 32-bit sparse read/monitor access.
struct RandomReadDWordSpec {
  RandomReadDWordSpec() = delete;
  constexpr explicit RandomReadDWordSpec(std::string_view target_device) noexcept
      : device(target_device) {}

  /// Plain device string such as `D100`, `LZ0`, or `LCN10` selected explicitly as 32-bit.
  std::string_view device;
};

/// \brief String-address spec used to build sparse random word-write items.
///
/// Device and value must be supplied together. Explicit zero is valid.
struct RandomWriteWordSpec {
  RandomWriteWordSpec() = delete;
  constexpr RandomWriteWordSpec(std::string_view target_device, std::uint16_t write_value) noexcept
      : device(target_device), value(write_value) {}

  /// Plain device string such as `D100` or `LZ0`.
  std::string_view device;
  /// Explicit 16-bit word value written to `device`.
  std::uint16_t value;
};

/// \brief String-address spec used to build an explicit double-word sparse write item.
///
/// Device and value must be supplied together. Explicit zero is valid.
struct RandomWriteDWordSpec {
  RandomWriteDWordSpec() = delete;
  constexpr RandomWriteDWordSpec(std::string_view target_device, std::uint32_t write_value) noexcept
      : device(target_device), value(write_value) {}

  /// Plain device string such as `D100` or `LZ0`.
  std::string_view device;
  /// Explicit 32-bit double-word value written to `device`.
  std::uint32_t value;
};

/// \brief String-address spec used to build sparse random bit-write items.
///
/// Device and value must be supplied together. Explicit `Off` is valid.
struct RandomWriteBitSpec {
  RandomWriteBitSpec() = delete;
  constexpr RandomWriteBitSpec(std::string_view target_device, BitValue write_value) noexcept
      : device(target_device), value(write_value) {}

  /// Plain bit-device string such as `M100` or `X10`.
  std::string_view device;
  /// Bit value written to `device`.
  BitValue value;
};

/// \brief Logical state selected from a long timer/counter status block.
enum class LongStateReadKind : std::uint8_t {
  Contact,
  Coil
};

enum class LongStateReadRoute : std::uint8_t {
  StatusBlock,
  DirectBits
};

/// \brief Mapping from a long-family state device to the helper's internal read route.
struct LongStateReadSpec {
  LongStateReadSpec() = delete;
  constexpr LongStateReadSpec(
      LongStateReadRoute read_route,
      DeviceCode target_base_code,
      LongStateReadKind state_kind) noexcept
      : route(read_route), base_code(target_base_code), kind(state_kind) {}

  /// Read route used internally by the long-state helper.
  LongStateReadRoute route;
  /// Base current-value device read with `0401` word access, or direct bit device for DirectBits.
  DeviceCode base_code;
  /// Status bit selected from the third word of the block.
  LongStateReadKind kind;
};

/// \brief Parses a plain MC device string such as `D100`, `M100`, `X10`, or `B20`.
///
/// This helper is intentionally limited to plain device syntax. It does not parse `Jn\\...` link-
/// direct addresses, helper-qualified `U...\\G...` addresses, or standalone `G` / `HG`.
[[nodiscard]] inline Status parse_device_address(
    std::string_view text,
    DeviceAddress& out_device) noexcept {
  for (const auto& spec : detail::kDeviceParseSpecs) {
    if (text.size() <= spec.prefix_length) {
      continue;
    }

    bool prefix_match = true;
    for (std::size_t index = 0; index < spec.prefix_length; ++index) {
      const char lhs = detail::ascii_upper(text[index]);
      if (lhs != spec.prefix[index]) {
        prefix_match = false;
        break;
      }
    }
    if (!prefix_match) {
      continue;
    }

    std::uint32_t number = 0;
    if (!detail::parse_u32(text.substr(spec.prefix_length), number, spec.base)) {
      return make_status(StatusCode::InvalidArgument, "Device address number is invalid");
    }

    out_device = DeviceAddress(spec.code, number);
    return ok_status();
  }

  return make_status(StatusCode::InvalidArgument, "Device address prefix is not supported");
}

/// \brief Resolves the dedicated read path for long timer/counter state devices.
///
/// `LTS/LTC/LSTS/LSTC/LCS/LCC` are read through this helper. Timer state devices use the
/// corresponding `LTN/LSTN` 4-word status block; long counter contacts/coils use direct bit access.
[[nodiscard]] inline Status get_long_state_read_spec(
    DeviceCode code,
    LongStateReadSpec& out_spec) noexcept {
  switch (code) {
    case DeviceCode::LTS:
      out_spec = LongStateReadSpec(
          LongStateReadRoute::StatusBlock, DeviceCode::LTN, LongStateReadKind::Contact);
      return ok_status();
    case DeviceCode::LTC:
      out_spec = LongStateReadSpec(
          LongStateReadRoute::StatusBlock, DeviceCode::LTN, LongStateReadKind::Coil);
      return ok_status();
    case DeviceCode::LSTS:
      out_spec = LongStateReadSpec(
          LongStateReadRoute::StatusBlock, DeviceCode::LSTN, LongStateReadKind::Contact);
      return ok_status();
    case DeviceCode::LSTC:
      out_spec = LongStateReadSpec(
          LongStateReadRoute::StatusBlock, DeviceCode::LSTN, LongStateReadKind::Coil);
      return ok_status();
    case DeviceCode::LCS:
      out_spec = LongStateReadSpec(
          LongStateReadRoute::DirectBits, DeviceCode::LCS, LongStateReadKind::Contact);
      return ok_status();
    case DeviceCode::LCC:
      out_spec = LongStateReadSpec(
          LongStateReadRoute::DirectBits, DeviceCode::LCC, LongStateReadKind::Coil);
      return ok_status();
    default:
      return make_status(StatusCode::InvalidArgument, "Device is not a long timer/counter state device");
  }
}

/// \brief Decodes the contact/coil bit from a long-family 4-word status block.
[[nodiscard]] inline Status decode_long_state_bit(
    const LongStateReadSpec& spec,
    mcprotocol::serial::Span<const std::uint16_t> status_block_words,
    BitValue& out_value) noexcept {
  if (status_block_words.size() < 4U) {
    return make_status(StatusCode::BufferTooSmall, "Long state status block requires 4 words");
  }

  if (spec.kind != LongStateReadKind::Contact && spec.kind != LongStateReadKind::Coil) {
    return make_status(StatusCode::InvalidArgument, "Long state read kind is invalid");
  }

  const std::uint16_t status_word = status_block_words[2];
  const std::uint16_t mask =
      spec.kind == LongStateReadKind::Contact ? static_cast<std::uint16_t>(0x0002U)
                                              : static_cast<std::uint16_t>(0x0001U);
  out_value = (status_word & mask) == 0U ? false : true;
  return ok_status();
}

/// \brief Builds a contiguous word-read request from a string address such as `D100`.
[[nodiscard]] inline Status make_batch_read_words_request(
    std::string_view head_device,
    std::uint16_t points,
    BatchReadWordsRequest& out_request) noexcept {
  DeviceAddress parsed(DeviceCode::D, 0U);
  const Status status = parse_device_address(head_device, parsed);
  if (!status.ok()) {
    return status;
  }
  out_request = BatchReadWordsRequest(parsed, points);
  return ok_status();
}

/// \brief Builds a contiguous bit-read request from a string address such as `M100`.
[[nodiscard]] inline Status make_batch_read_bits_request(
    std::string_view head_device,
    std::uint16_t points,
    BatchReadBitsRequest& out_request) noexcept {
  DeviceAddress parsed(DeviceCode::D, 0U);
  const Status status = parse_device_address(head_device, parsed);
  if (!status.ok()) {
    return status;
  }
  out_request = BatchReadBitsRequest(parsed, points);
  return ok_status();
}

/// \brief Builds a contiguous word-write request from a string address such as `D100`.
[[nodiscard]] inline Status make_batch_write_words_request(
    std::string_view head_device,
    mcprotocol::serial::Span<const std::uint16_t> words,
    BatchWriteWordsRequest& out_request) noexcept {
  DeviceAddress parsed(DeviceCode::D, 0U);
  const Status status = parse_device_address(head_device, parsed);
  if (!status.ok()) {
    return status;
  }
  out_request = BatchWriteWordsRequest(parsed, words);
  return ok_status();
}

/// \brief Builds a contiguous bit-write request from a string address such as `M100`.
[[nodiscard]] inline Status make_batch_write_bits_request(
    std::string_view head_device,
    mcprotocol::serial::Span<const BitValue> bits,
    BatchWriteBitsRequest& out_request) noexcept {
  DeviceAddress parsed(DeviceCode::D, 0U);
  const Status status = parse_device_address(head_device, parsed);
  if (!status.ok()) {
    return status;
  }
  out_request = BatchWriteBitsRequest(parsed, bits);
  return ok_status();
}

/// \brief Explicit non-blocking read-modify-write for one bit inside a 16-bit word device.
///
/// `begin()` validates both the read and write before the first request. The two requests occupy
/// the same client continuously and share one absolute deadline. They are not PLC-atomic: PLC
/// logic or another connection can modify the word between them. The write is always sent after a
/// successful read, even when the selected bit already has the requested state. Keep this object
/// alive until its completion callback runs.
class BitInWordWriteOperation {
 public:
  BitInWordWriteOperation() = default;
  BitInWordWriteOperation(const BitInWordWriteOperation&) = delete;
  BitInWordWriteOperation& operator=(const BitInWordWriteOperation&) = delete;

  [[nodiscard]] Status begin(
      MelsecSerialClient& client,
      std::uint32_t now_ms,
      std::string_view word_device,
      int bit_index,
      bool value,
      CompletionHandler callback,
      void* user) noexcept {
    if (busy_) {
      return make_status(StatusCode::Busy, "Bit-in-word operation is already active");
    }
    if (callback == nullptr) {
      return make_status(StatusCode::InvalidArgument, "Completion callback must not be null");
    }
    if (bit_index < 0 || bit_index > 15) {
      return make_status(StatusCode::InvalidArgument, "bit_index must be in range 0..15");
    }
    DeviceAddress parsed(DeviceCode::D, 0U);
    Status status = parse_device_address(word_device, parsed);
    if (!status.ok()) return status;
    if (detail::is_bit_device_code(parsed.code) ||
        detail::is_dword_only_device_code(parsed.code) ||
        parsed.code == DeviceCode::G || parsed.code == DeviceCode::HG) {
      return make_status(
          StatusCode::InvalidArgument,
          "write_bit_in_word requires an ordinary 16-bit word device");
    }
    status = client.validate_bit_in_word_plan(parsed);
    if (!status.ok()) return status;
    status = client.begin_compound_deadline(now_ms);
    if (!status.ok()) return status;

    client_ = &client;
    route_ = Route::Standard;
    device_ = parsed;
    word_ = 0U;
    bit_index_ = bit_index;
    value_ = value;
    callback_ = callback;
    user_ = user;
    busy_ = true;
    const BatchReadWordsRequest request(device_, 1U);
    status = client.async_batch_read_words(
        now_ms,
        request,
        mcprotocol::serial::Span<std::uint16_t>(&word_, 1U),
        &BitInWordWriteOperation::on_read_complete,
        this);
    if (!status.ok()) {
      client.end_compound_deadline();
      clear();
    }
    return status;
  }

  [[nodiscard]] Status begin_extended_file_register(
      MelsecSerialClient& client,
      std::uint32_t now_ms,
      ExtendedFileRegisterAddress word_device,
      int bit_index,
      bool value,
      CompletionHandler callback,
      void* user) noexcept {
    Status status = validate_begin(bit_index, callback);
    if (!status.ok()) return status;
    status = client.validate_bit_in_word_plan(word_device);
    if (!status.ok()) return status;
    status = client.begin_compound_deadline(now_ms);
    if (!status.ok()) return status;

    initialize(client, Route::ExtendedFileRegister, bit_index, value, callback, user);
    extended_file_register_device_ = word_device;
    const ExtendedFileRegisterBatchReadWordsRequest request(word_device, 1U);
    status = client.async_read_extended_file_register_words(
        now_ms,
        request,
        mcprotocol::serial::Span<std::uint16_t>(&word_, 1U),
        &BitInWordWriteOperation::on_read_complete,
        this);
    if (!status.ok()) abort_begin(client);
    return status;
  }

  [[nodiscard]] Status begin_direct_extended_file_register(
      MelsecSerialClient& client,
      std::uint32_t now_ms,
      std::uint32_t word_device_number,
      int bit_index,
      bool value,
      CompletionHandler callback,
      void* user) noexcept {
    Status status = validate_begin(bit_index, callback);
    if (!status.ok()) return status;
    status = client.validate_direct_bit_in_word_plan(word_device_number);
    if (!status.ok()) return status;
    status = client.begin_compound_deadline(now_ms);
    if (!status.ok()) return status;

    initialize(client, Route::DirectExtendedFileRegister, bit_index, value, callback, user);
    direct_extended_file_register_device_ = word_device_number;
    const ExtendedFileRegisterDirectBatchReadWordsRequest request(word_device_number, 1U);
    status = client.async_direct_read_extended_file_register_words(
        now_ms,
        request,
        mcprotocol::serial::Span<std::uint16_t>(&word_, 1U),
        &BitInWordWriteOperation::on_read_complete,
        this);
    if (!status.ok()) abort_begin(client);
    return status;
  }

  [[nodiscard]] Status begin_link_direct(
      MelsecSerialClient& client,
      std::uint32_t now_ms,
      LinkDirectDevice word_device,
      int bit_index,
      bool value,
      CompletionHandler callback,
      void* user) noexcept {
    Status status = validate_begin(bit_index, callback);
    if (!status.ok()) return status;
    if (detail::is_bit_device_code(word_device.device.code) ||
        detail::is_dword_only_device_code(word_device.device.code) ||
        word_device.device.code == DeviceCode::G || word_device.device.code == DeviceCode::HG) {
      return make_status(
          StatusCode::InvalidArgument,
          "begin_link_direct requires an ordinary 16-bit word device");
    }
    status = client.validate_bit_in_word_plan(word_device);
    if (!status.ok()) return status;
    status = client.begin_compound_deadline(now_ms);
    if (!status.ok()) return status;

    initialize(client, Route::LinkDirect, bit_index, value, callback, user);
    link_direct_device_ = word_device;
    status = client.async_link_direct_batch_read_words(
        now_ms,
        word_device,
        1U,
        mcprotocol::serial::Span<std::uint16_t>(&word_, 1U),
        &BitInWordWriteOperation::on_read_complete,
        this);
    if (!status.ok()) abort_begin(client);
    return status;
  }

  [[nodiscard]] Status begin_qualified_buffer(
      MelsecSerialClient& client,
      std::uint32_t now_ms,
      QualifiedBufferWordDevice word_device,
      int bit_index,
      bool value,
      CompletionHandler callback,
      void* user) noexcept {
    Status status = validate_begin(bit_index, callback);
    if (!status.ok()) return status;
    status = client.validate_bit_in_word_plan(word_device);
    if (!status.ok()) return status;
    status = client.begin_compound_deadline(now_ms);
    if (!status.ok()) return status;

    initialize(client, Route::QualifiedBuffer, bit_index, value, callback, user);
    qualified_buffer_device_ = word_device;
    status = client.async_extended_batch_read_words(
        now_ms,
        word_device,
        1U,
        mcprotocol::serial::Span<std::uint16_t>(&word_, 1U),
        &BitInWordWriteOperation::on_read_complete,
        this);
    if (!status.ok()) abort_begin(client);
    return status;
  }

  void cancel() noexcept {
    if (busy_ && client_ != nullptr) client_->cancel();
  }

  [[nodiscard]] bool busy() const noexcept { return busy_; }

 private:
  enum class Route : std::uint8_t {
    Standard,
    ExtendedFileRegister,
    DirectExtendedFileRegister,
    LinkDirect,
    QualifiedBuffer
  };

  [[nodiscard]] Status validate_begin(
      int bit_index,
      CompletionHandler callback) const noexcept {
    if (busy_) return make_status(StatusCode::Busy, "Bit-in-word operation is already active");
    if (callback == nullptr) {
      return make_status(StatusCode::InvalidArgument, "Completion callback must not be null");
    }
    if (bit_index < 0 || bit_index > 15) {
      return make_status(StatusCode::InvalidArgument, "bit_index must be in range 0..15");
    }
    return ok_status();
  }

  void initialize(
      MelsecSerialClient& client,
      Route route,
      int bit_index,
      bool value,
      CompletionHandler callback,
      void* user) noexcept {
    client_ = &client;
    route_ = route;
    word_ = 0U;
    bit_index_ = bit_index;
    value_ = value;
    callback_ = callback;
    user_ = user;
    busy_ = true;
  }

  void abort_begin(MelsecSerialClient& client) noexcept {
    client.end_compound_deadline();
    clear();
  }

  static void on_read_complete(void* context, Status status) noexcept {
    auto& self = *static_cast<BitInWordWriteOperation*>(context);
    if (!status.ok()) {
      self.finish(status);
      return;
    }
    const std::uint16_t mask = static_cast<std::uint16_t>(1U << self.bit_index_);
    self.word_ = self.value_
                     ? static_cast<std::uint16_t>(self.word_ | mask)
                     : static_cast<std::uint16_t>(self.word_ & static_cast<std::uint16_t>(~mask));
    const mcprotocol::serial::Span<const std::uint16_t> words(&self.word_, 1U);
    Status write_status = make_status(StatusCode::InvalidArgument, "Invalid bit-in-word route");
    switch (self.route_) {
      case Route::Standard: {
        const BatchWriteWordsRequest request(self.device_, words);
        write_status = self.client_->async_batch_write_words(
            0U, request, &BitInWordWriteOperation::on_write_complete, &self);
        break;
      }
      case Route::ExtendedFileRegister: {
        const ExtendedFileRegisterBatchWriteWordsRequest request(
            self.extended_file_register_device_, words);
        write_status = self.client_->async_write_extended_file_register_words(
            0U, request, &BitInWordWriteOperation::on_write_complete, &self);
        break;
      }
      case Route::DirectExtendedFileRegister: {
        const ExtendedFileRegisterDirectBatchWriteWordsRequest request(
            self.direct_extended_file_register_device_, words);
        write_status = self.client_->async_direct_write_extended_file_register_words(
            0U, request, &BitInWordWriteOperation::on_write_complete, &self);
        break;
      }
      case Route::LinkDirect:
        write_status = self.client_->async_link_direct_batch_write_words(
            0U,
            self.link_direct_device_,
            words,
            &BitInWordWriteOperation::on_write_complete,
            &self);
        break;
      case Route::QualifiedBuffer:
        write_status = self.client_->async_extended_batch_write_words(
            0U,
            self.qualified_buffer_device_,
            words,
            &BitInWordWriteOperation::on_write_complete,
            &self);
        break;
    }
    if (!write_status.ok()) self.finish(write_status);
  }

  static void on_write_complete(void* context, Status status) noexcept {
    static_cast<BitInWordWriteOperation*>(context)->finish(status);
  }

  void finish(Status status) noexcept {
    MelsecSerialClient* client = client_;
    CompletionHandler callback = callback_;
    void* user = user_;
    clear();
    if (client != nullptr) client->end_compound_deadline();
    if (callback != nullptr) callback(user, status);
  }

  void clear() noexcept {
    client_ = nullptr;
    callback_ = nullptr;
    user_ = nullptr;
    busy_ = false;
  }

  MelsecSerialClient* client_ = nullptr;
  Route route_ = Route::Standard;
  DeviceAddress device_ {DeviceCode::D, 0U};
  ExtendedFileRegisterAddress extended_file_register_device_ {1U, 0U};
  std::uint32_t direct_extended_file_register_device_ = 0U;
  LinkDirectDevice link_direct_device_ {0U, DeviceAddress {DeviceCode::D, 0U}};
  QualifiedBufferWordDevice qualified_buffer_device_ {
      QualifiedBufferDeviceKind::G,
      0U,
      0U};
  std::uint16_t word_ = 0U;
  int bit_index_ = 0;
  bool value_ = false;
  CompletionHandler callback_ = nullptr;
  void* user_ = nullptr;
  bool busy_ = false;
};

/// \brief Builds one explicitly word-width sparse random-read item from a string address.
[[nodiscard]] inline Status make_random_read_word_item(
    std::string_view device,
    RandomReadWordItem& out_item) noexcept {
  DeviceAddress parsed(DeviceCode::D, 0U);
  const Status status = parse_device_address(device, parsed);
  if (!status.ok()) {
    return status;
  }
  out_item = RandomReadWordItem {parsed};
  return ok_status();
}

/// \brief Builds one explicitly double-word-width sparse random-read item.
[[nodiscard]] inline Status make_random_read_dword_item(
    std::string_view device,
    RandomReadDWordItem& out_item) noexcept {
  DeviceAddress parsed(DeviceCode::D, 0U);
  const Status status = parse_device_address(device, parsed);
  if (!status.ok()) {
    return status;
  }
  out_item = RandomReadDWordItem {parsed};
  return ok_status();
}

/// \brief Builds one sparse random word-write item from a string address.
[[nodiscard]] inline Status make_random_write_word_item(
    std::string_view device,
    std::uint16_t value,
    RandomWriteWordItem& out_item) noexcept {
  DeviceAddress parsed(DeviceCode::D, 0U);
  const Status status = parse_device_address(device, parsed);
  if (!status.ok()) {
    return status;
  }
  out_item = RandomWriteWordItem(parsed, value);
  return ok_status();
}

/// \brief Builds one explicitly double-word-width sparse random write item.
[[nodiscard]] inline Status make_random_write_dword_item(
    std::string_view device,
    std::uint32_t value,
    RandomWriteDWordItem& out_item) noexcept {
  DeviceAddress parsed(DeviceCode::D, 0U);
  const Status status = parse_device_address(device, parsed);
  if (!status.ok()) {
    return status;
  }
  out_item = RandomWriteDWordItem(parsed, value);
  return ok_status();
}

/// \brief Builds one sparse random bit-write item from a string address.
[[nodiscard]] inline Status make_random_write_bit_item(
    std::string_view device,
    BitValue value,
    RandomWriteBitItem& out_item) noexcept {
  if (value != false && value != true) {
    return make_status(
        StatusCode::InvalidArgument,
        "Random write bit value must be false or true");
  }
  DeviceAddress parsed(DeviceCode::D, 0U);
  const Status status = parse_device_address(device, parsed);
  if (!status.ok()) {
    return status;
  }
  out_item = RandomWriteBitItem(parsed, value);
  return ok_status();
}

/// \brief Builds a sparse random-read request from string-address specs.
///
/// Use this when you want `0403` style sparse addressing without hand-filling the explicit-width
/// Word and DWord item types.
[[nodiscard]] inline Status make_random_read_request(
    mcprotocol::serial::Span<const RandomReadWordSpec> word_specs,
    mcprotocol::serial::Span<const RandomReadDWordSpec> dword_specs,
    mcprotocol::serial::Span<RandomReadWordItem> out_word_items,
    mcprotocol::serial::Span<RandomReadDWordItem> out_dword_items,
    RandomReadRequest& out_request) noexcept {
  if (out_word_items.size() < word_specs.size() ||
      out_dword_items.size() < dword_specs.size()) {
    return make_status(StatusCode::BufferTooSmall, "Random read output item buffers are too small");
  }

  for (std::size_t index = 0; index < word_specs.size(); ++index) {
    const Status status = make_random_read_word_item(
        word_specs[index].device, out_word_items[index]);
    if (!status.ok()) {
      return status;
    }
  }
  for (std::size_t index = 0; index < dword_specs.size(); ++index) {
    const Status status = make_random_read_dword_item(
        dword_specs[index].device, out_dword_items[index]);
    if (!status.ok()) {
      return status;
    }
  }

  out_request = RandomReadRequest(
      mcprotocol::serial::Span<const RandomReadWordItem>(out_word_items.data(), word_specs.size()),
      mcprotocol::serial::Span<const RandomReadDWordItem>(out_dword_items.data(), dword_specs.size()));
  return ok_status();
}

/// \brief Builds a sparse monitor registration payload from string-address specs.
///
/// The resulting payload is intended for `0801`. Readback still happens through the normal monitor
/// read API.
[[nodiscard]] inline Status make_monitor_registration(
    mcprotocol::serial::Span<const RandomReadWordSpec> word_specs,
    mcprotocol::serial::Span<const RandomReadDWordSpec> dword_specs,
    mcprotocol::serial::Span<RandomReadWordItem> out_word_items,
    mcprotocol::serial::Span<RandomReadDWordItem> out_dword_items,
    MonitorRegistration& out_request) noexcept {
  RandomReadRequest request({}, {});
  const Status status = make_random_read_request(
      word_specs, dword_specs, out_word_items, out_dword_items, request);
  if (!status.ok()) {
    return status;
  }

  out_request = MonitorRegistration(request.word_items, request.dword_items);
  return ok_status();
}

/// \brief Builds sparse random word-write items from string-address specs.
[[nodiscard]] inline Status make_random_write_word_items(
    mcprotocol::serial::Span<const RandomWriteWordSpec> specs,
    mcprotocol::serial::Span<RandomWriteWordItem> out_items,
    mcprotocol::serial::Span<const RandomWriteWordItem>& out_item_view) noexcept {
  if (out_items.size() < specs.size()) {
    return make_status(StatusCode::BufferTooSmall, "Random write word output item buffer is too small");
  }

  for (std::size_t index = 0; index < specs.size(); ++index) {
    const Status status = make_random_write_word_item(
        specs[index].device,
        specs[index].value,
        out_items[index]);
    if (!status.ok()) {
      return status;
    }
  }

  out_item_view = mcprotocol::serial::Span<const RandomWriteWordItem>(out_items.data(), specs.size());
  return ok_status();
}

/// \brief Builds sparse explicit double-word write items from string-address specs.
[[nodiscard]] inline Status make_random_write_dword_items(
    mcprotocol::serial::Span<const RandomWriteDWordSpec> specs,
    mcprotocol::serial::Span<RandomWriteDWordItem> out_items,
    mcprotocol::serial::Span<const RandomWriteDWordItem>& out_item_view) noexcept {
  if (out_items.size() < specs.size()) {
    return make_status(StatusCode::BufferTooSmall, "Random write dword output item buffer is too small");
  }
  for (std::size_t index = 0; index < specs.size(); ++index) {
    const Status status = make_random_write_dword_item(
        specs[index].device, specs[index].value, out_items[index]);
    if (!status.ok()) {
      return status;
    }
  }
  out_item_view = mcprotocol::serial::Span<const RandomWriteDWordItem>(out_items.data(), specs.size());
  return ok_status();
}

/// \brief Builds sparse random bit-write items from string-address specs.
[[nodiscard]] inline Status make_random_write_bit_items(
    mcprotocol::serial::Span<const RandomWriteBitSpec> specs,
    mcprotocol::serial::Span<RandomWriteBitItem> out_items,
    mcprotocol::serial::Span<const RandomWriteBitItem>& out_item_view) noexcept {
  if (out_items.size() < specs.size()) {
    return make_status(StatusCode::BufferTooSmall, "Random write bit output item buffer is too small");
  }

  for (std::size_t index = 0; index < specs.size(); ++index) {
    const Status status =
        make_random_write_bit_item(specs[index].device, specs[index].value, out_items[index]);
    if (!status.ok()) {
      return status;
    }
  }

  out_item_view = mcprotocol::serial::Span<const RandomWriteBitItem>(out_items.data(), specs.size());
  return ok_status();
}

}  // namespace mcprotocol::serial::highlevel
