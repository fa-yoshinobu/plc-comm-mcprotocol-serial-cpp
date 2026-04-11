#include "mcprotocol/serial/codec.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace mcprotocol::serial {
namespace {

constexpr std::uint8_t kAsciiEnq = 0x05;
constexpr std::uint8_t kAsciiAck = 0x06;
constexpr std::uint8_t kAsciiNak = 0x15;
constexpr std::uint8_t kAsciiStx = 0x02;
constexpr std::uint8_t kAsciiEtx = 0x03;
constexpr std::uint8_t kAsciiLf = 0x0A;
constexpr std::uint8_t kAsciiCr = 0x0D;
constexpr std::uint8_t kBinaryDle = 0x10;

struct DeviceSpec {
  DeviceCode code;
  const char* ascii_q_l;
  const char* ascii_iq_r;
  std::uint8_t binary_q_l;
  std::uint16_t binary_iq_r;
  bool hexadecimal;
  bool bit_device;
};

constexpr std::array<DeviceSpec, 28> kDeviceSpecs {{
    {DeviceCode::X, "X", "X", 0x9C, 0x009C, true, true},
    {DeviceCode::Y, "Y", "Y", 0x9D, 0x009D, true, true},
    {DeviceCode::M, "M", "M", 0x90, 0x0090, false, true},
    {DeviceCode::L, "L", "L", 0x92, 0x0092, false, true},
    {DeviceCode::F, "F", "F", 0x93, 0x0093, false, true},
    {DeviceCode::V, "V", "V", 0x94, 0x0094, false, true},
    {DeviceCode::B, "B", "B", 0xA0, 0x00A0, true, true},
    {DeviceCode::D, "D", "D", 0xA8, 0x00A8, false, false},
    {DeviceCode::W, "W", "W", 0xB4, 0x00B4, true, false},
    {DeviceCode::TS, "TS", "TS", 0xC1, 0x00C1, false, true},
    {DeviceCode::TC, "TC", "TC", 0xC0, 0x00C0, false, true},
    {DeviceCode::TN, "TN", "TN", 0xC2, 0x00C2, false, false},
    {DeviceCode::STS, "SS", "STS", 0xC7, 0x00C7, false, true},
    {DeviceCode::STC, "SC", "STC", 0xC6, 0x00C6, false, true},
    {DeviceCode::STN, "SN", "STN", 0xC8, 0x00C8, false, false},
    {DeviceCode::CS, "CS", "CS", 0xC4, 0x00C4, false, true},
    {DeviceCode::CC, "CC", "CC", 0xC3, 0x00C3, false, true},
    {DeviceCode::CN, "CN", "CN", 0xC5, 0x00C5, false, false},
    {DeviceCode::SB, "SB", "SB", 0xA1, 0x00A1, true, true},
    {DeviceCode::SW, "SW", "SW", 0xB5, 0x00B5, true, false},
    {DeviceCode::S, "S", "S", 0x98, 0x0098, false, true},
    {DeviceCode::DX, "DX", "DX", 0xA2, 0x00A2, true, true},
    {DeviceCode::DY, "DY", "DY", 0xA3, 0x00A3, true, true},
    {DeviceCode::Z, "Z", "Z", 0xCC, 0x00CC, false, false},
    {DeviceCode::R, "R", "R", 0xAF, 0x00AF, false, false},
    {DeviceCode::ZR, "ZR", "ZR", 0xB0, 0x00B0, true, false},
    {DeviceCode::G, "G", "G", 0xAB, 0x00AB, false, false},
    {DeviceCode::HG, "HG", "HG", 0x2E, 0x002E, false, false},
}};

class ByteWriter {
 public:
  explicit ByteWriter(std::span<std::uint8_t> buffer) : buffer_(buffer) {}

  [[nodiscard]] bool push(std::uint8_t value) noexcept {
    if (size_ >= buffer_.size()) {
      return false;
    }
    buffer_[size_++] = value;
    return true;
  }

  [[nodiscard]] bool append(std::span<const std::uint8_t> bytes) noexcept {
    if ((size_ + bytes.size()) > buffer_.size()) {
      return false;
    }
    std::memcpy(buffer_.data() + size_, bytes.data(), bytes.size());
    size_ += bytes.size();
    return true;
  }

  [[nodiscard]] bool append_le16(std::uint16_t value) noexcept {
    return push(static_cast<std::uint8_t>(value & 0xFFU)) &&
           push(static_cast<std::uint8_t>((value >> 8U) & 0xFFU));
  }

  [[nodiscard]] bool append_le32(std::uint32_t value, std::size_t width) noexcept {
    for (std::size_t index = 0; index < width; ++index) {
      if (!push(static_cast<std::uint8_t>((value >> (index * 8U)) & 0xFFU))) {
        return false;
      }
    }
    return true;
  }

  [[nodiscard]] std::size_t size() const noexcept {
    return size_;
  }

  [[nodiscard]] std::span<const std::uint8_t> written() const noexcept {
    return buffer_.first(size_);
  }

  void clear() noexcept {
    size_ = 0;
  }

 private:
  std::span<std::uint8_t> buffer_;
  std::size_t size_ = 0;
};

[[nodiscard]] constexpr bool is_iq_r_series(const ProtocolConfig& config) noexcept {
  return config.target_series == PlcSeries::IQ_R;
}

[[nodiscard]] constexpr bool is_ascii_format1_family(const ProtocolConfig& config) noexcept {
  return config.ascii_format == AsciiFormat::Format1 || config.ascii_format == AsciiFormat::Format4;
}

[[nodiscard]] constexpr bool uses_ascii_crlf(const ProtocolConfig& config) noexcept {
  return config.ascii_format == AsciiFormat::Format4;
}

[[nodiscard]] constexpr std::size_t ascii_route_length(FrameKind frame_kind) noexcept {
  switch (frame_kind) {
    case FrameKind::C4:
      return 14;
    case FrameKind::C3:
      return 8;
    default:
      return 0;
  }
}

[[nodiscard]] constexpr std::size_t binary_route_length(FrameKind frame_kind) noexcept {
  switch (frame_kind) {
    case FrameKind::C4:
      return 7;
    default:
      return 0;
  }
}

[[nodiscard]] constexpr std::uint8_t frame_id(FrameKind frame_kind) noexcept {
  switch (frame_kind) {
    case FrameKind::C4:
      return 0xF8;
    case FrameKind::C3:
      return 0xF9;
    default:
      return 0x00;
  }
}

[[nodiscard]] constexpr std::size_t ascii_device_code_width(const ProtocolConfig& config) noexcept {
  return is_iq_r_series(config) ? 4U : 2U;
}

[[nodiscard]] constexpr std::size_t ascii_device_number_width(const ProtocolConfig& config) noexcept {
  return is_iq_r_series(config) ? 8U : 6U;
}

[[nodiscard]] constexpr std::size_t ascii_extended_device_number_width(
    const ProtocolConfig& config) noexcept {
  return is_iq_r_series(config) ? 10U : 6U;
}

[[nodiscard]] constexpr std::size_t binary_device_code_width(const ProtocolConfig& config) noexcept {
  return is_iq_r_series(config) ? 2U : 1U;
}

[[nodiscard]] constexpr std::size_t binary_device_number_width(const ProtocolConfig& config) noexcept {
  return is_iq_r_series(config) ? 4U : 3U;
}

[[nodiscard]] constexpr std::size_t request_command_header_size(const ProtocolConfig& config) noexcept {
  return config.code_mode == CodeMode::Ascii ? 8U : 4U;
}

[[nodiscard]] constexpr std::size_t request_word_count_size(const ProtocolConfig& config) noexcept {
  return config.code_mode == CodeMode::Ascii ? 4U : 2U;
}

[[nodiscard]] constexpr std::size_t request_device_reference_size(const ProtocolConfig& config) noexcept {
  return config.code_mode == CodeMode::Ascii ? (ascii_device_code_width(config) + ascii_device_number_width(config))
                                             : (binary_device_code_width(config) + binary_device_number_width(config));
}

[[nodiscard]] constexpr std::size_t batch_write_words_point_limit_for_buffer(
    const ProtocolConfig& config) noexcept {
  const std::size_t overhead =
      request_command_header_size(config) + request_device_reference_size(config) + request_word_count_size(config);
  if (kMaxRequestDataBytes <= overhead) {
    return 0U;
  }
  const std::size_t buffer_limit = (kMaxRequestDataBytes - overhead) / (config.code_mode == CodeMode::Ascii ? 4U : 2U);
  return buffer_limit < kMaxBatchWordPoints ? buffer_limit : kMaxBatchWordPoints;
}

[[nodiscard]] constexpr std::size_t batch_write_bits_point_limit_for_buffer(
    const ProtocolConfig& config) noexcept {
  const std::size_t overhead =
      request_command_header_size(config) + request_device_reference_size(config) + request_word_count_size(config);
  if (kMaxRequestDataBytes <= overhead) {
    return 0U;
  }
  const std::size_t remaining = kMaxRequestDataBytes - overhead;
  const std::size_t buffer_limit = config.code_mode == CodeMode::Ascii ? remaining : (remaining * 2U);
  const std::size_t protocol_limit =
      config.code_mode == CodeMode::Ascii ? kMaxBatchBitPointsAscii : kMaxBatchBitPointsBinary;
  return buffer_limit < protocol_limit ? buffer_limit : protocol_limit;
}

[[nodiscard]] constexpr std::uint16_t word_subcommand(const ProtocolConfig& config) noexcept {
  return is_iq_r_series(config) ? 0x0002U : 0x0000U;
}

[[nodiscard]] constexpr std::uint16_t bit_subcommand(const ProtocolConfig& config) noexcept {
  return is_iq_r_series(config) ? 0x0003U : 0x0001U;
}

[[nodiscard]] constexpr std::uint16_t extended_word_subcommand(const ProtocolConfig& config) noexcept {
  return is_iq_r_series(config) ? 0x0082U : 0x0080U;
}

[[nodiscard]] constexpr std::size_t ascii_device_modification_width(
    const ProtocolConfig& config) noexcept {
  return is_iq_r_series(config) ? 4U : 3U;
}

[[nodiscard]] const DeviceSpec* find_device_spec(DeviceCode code) noexcept {
  for (const auto& spec : kDeviceSpecs) {
    if (spec.code == code) {
      return &spec;
    }
  }
  return nullptr;
}

[[nodiscard]] constexpr std::uint8_t nibble_to_ascii(std::uint8_t value) noexcept {
  value &= 0x0FU;
  return static_cast<std::uint8_t>(value < 10 ? ('0' + value) : ('A' + value - 10));
}

[[nodiscard]] constexpr int ascii_to_nibble(std::uint8_t value) noexcept {
  if (value >= '0' && value <= '9') {
    return value - '0';
  }
  if (value >= 'A' && value <= 'F') {
    return 10 + (value - 'A');
  }
  if (value >= 'a' && value <= 'f') {
    return 10 + (value - 'a');
  }
  return -1;
}

[[nodiscard]] bool append_ascii_hex(
    ByteWriter& writer,
    std::uint64_t value,
    std::size_t digits) noexcept {
  for (std::size_t index = 0; index < digits; ++index) {
    const std::size_t shift = (digits - 1U - index) * 4U;
    if (!writer.push(nibble_to_ascii(static_cast<std::uint8_t>((value >> shift) & 0x0FU)))) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool append_ascii_device_number(
    ByteWriter& writer,
    std::uint32_t value,
    std::size_t digits,
    bool hexadecimal) noexcept {
  if (hexadecimal) {
    return append_ascii_hex(writer, value, digits);
  }

  std::array<std::uint8_t, 10> digits_buffer {};
  std::uint32_t working = value;
  for (std::size_t index = 0; index < digits; ++index) {
    digits_buffer[digits - 1U - index] = static_cast<std::uint8_t>('0' + (working % 10U));
    working /= 10U;
  }
  if (working != 0U) {
    return false;
  }
  return writer.append(std::span<const std::uint8_t>(digits_buffer.data(), digits));
}

[[nodiscard]] bool append_ascii_device_code(
    ByteWriter& writer,
    const ProtocolConfig& config,
    const DeviceSpec& spec) noexcept {
  const char* symbol = is_iq_r_series(config) ? spec.ascii_iq_r : spec.ascii_q_l;
  const std::size_t width = ascii_device_code_width(config);
  std::size_t length = 0;
  while (symbol[length] != '\0') {
    ++length;
  }
  if (length > width) {
    return false;
  }
  for (std::size_t index = 0; index < length; ++index) {
    if (!writer.push(static_cast<std::uint8_t>(symbol[index]))) {
      return false;
    }
  }
  for (std::size_t index = length; index < width; ++index) {
    if (!writer.push(static_cast<std::uint8_t>('*'))) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool append_device_reference_ascii(
    ByteWriter& writer,
    const ProtocolConfig& config,
    const DeviceAddress& device) noexcept {
  const DeviceSpec* spec = find_device_spec(device.code);
  if (spec == nullptr) {
    return false;
  }
  return append_ascii_device_code(writer, config, *spec) &&
         append_ascii_device_number(
             writer,
             device.number,
             ascii_device_number_width(config),
             spec->hexadecimal);
}

[[nodiscard]] bool append_device_reference_binary(
    ByteWriter& writer,
    const ProtocolConfig& config,
    const DeviceAddress& device) noexcept {
  const DeviceSpec* spec = find_device_spec(device.code);
  if (spec == nullptr) {
    return false;
  }
  const std::size_t number_width = binary_device_number_width(config);
  const std::size_t code_width = binary_device_code_width(config);
  const std::uint16_t binary_code = is_iq_r_series(config) ? spec->binary_iq_r : spec->binary_q_l;
  return writer.append_le32(device.number, number_width) &&
         writer.append_le32(binary_code, code_width);
}

[[nodiscard]] bool append_device_reference(
    ByteWriter& writer,
    const ProtocolConfig& config,
    const DeviceAddress& device) noexcept {
  if (config.code_mode == CodeMode::Ascii) {
    return append_device_reference_ascii(writer, config, device);
  }
  return append_device_reference_binary(writer, config, device);
}

[[nodiscard]] Status invalid_argument(const char* message) noexcept;
[[nodiscard]] Status buffer_too_small(const char* message) noexcept;
[[nodiscard]] Status parse_error(const char* message) noexcept;
[[nodiscard]] bool parse_ascii_word(
    std::span<const std::uint8_t> bytes,
    std::uint16_t& value) noexcept;
[[nodiscard]] bool parse_binary_word(
    std::span<const std::uint8_t> bytes,
    std::size_t offset,
    std::uint16_t& value) noexcept;

[[nodiscard]] constexpr DeviceCode qualified_device_code(
    const QualifiedBufferWordDevice& device) noexcept {
  return device.kind == QualifiedBufferDeviceKind::HG ? DeviceCode::HG : DeviceCode::G;
}

[[nodiscard]] constexpr std::uint8_t qualified_binary_direct_memory(
    const ProtocolConfig& config,
    const QualifiedBufferWordDevice& device) noexcept {
  (void)config;
  if (device.kind == QualifiedBufferDeviceKind::HG) {
    return 0xFAU;
  }
  // CPU No.1-4 occupy U3E0..U3E3 in the manual's CPU buffer examples.
  if (device.module_number >= 0x03E0U && device.module_number <= 0x03E3U) {
    return 0xFAU;
  }
  return 0xF8U;
}

[[nodiscard]] Status validate_extended_word_device(
    const ProtocolConfig& config,
    const QualifiedBufferWordDevice& device) noexcept {
  if (device.kind == QualifiedBufferDeviceKind::HG && !is_iq_r_series(config)) {
    return invalid_argument("HG device extension access requires MELSEC iQ-R series");
  }
  if (config.code_mode == CodeMode::Ascii && device.module_number > 0x0FFFU) {
    return invalid_argument("ASCII device extension specification must be in range 0x000..0xFFF");
  }
  return ok_status();
}

[[nodiscard]] bool append_extension_specification_ascii(
    ByteWriter& writer,
    const QualifiedBufferWordDevice& device) noexcept {
  return writer.push(static_cast<std::uint8_t>('U')) &&
         append_ascii_hex(writer, device.module_number & 0x0FFFU, 3U);
}

[[nodiscard]] bool append_extension_modification_ascii(
    ByteWriter& writer) noexcept {
  return writer.push(static_cast<std::uint8_t>('0')) &&
         writer.push(static_cast<std::uint8_t>('0'));
}

[[nodiscard]] bool append_device_modification_ascii(
    ByteWriter& writer,
    const ProtocolConfig& config) noexcept {
  for (std::size_t index = 0; index < ascii_device_modification_width(config); ++index) {
    if (!writer.push(static_cast<std::uint8_t>('0'))) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool append_extended_device_reference_ascii(
    ByteWriter& writer,
    const ProtocolConfig& config,
    const QualifiedBufferWordDevice& device) noexcept {
  const DeviceSpec* spec = find_device_spec(qualified_device_code(device));
  if (spec == nullptr) {
    return false;
  }
  return append_extension_modification_ascii(writer) &&
         append_extension_specification_ascii(writer, device) &&
         append_device_modification_ascii(writer, config) &&
         append_ascii_device_code(writer, config, *spec) &&
         append_ascii_device_number(
             writer,
             device.word_address,
             ascii_extended_device_number_width(config),
             spec->hexadecimal);
}

[[nodiscard]] bool append_extended_device_reference_binary(
    ByteWriter& writer,
    const ProtocolConfig& config,
    const QualifiedBufferWordDevice& device) noexcept {
  const DeviceAddress address {
      .code = qualified_device_code(device),
      .number = device.word_address,
  };
  return writer.push(0x00U) &&
         writer.push(0x00U) &&
         append_device_reference_binary(writer, config, address) &&
         writer.push(0x00U) &&
         writer.push(0x00U) &&
         writer.append_le16(device.module_number) &&
         writer.push(qualified_binary_direct_memory(config, device));
}

[[nodiscard]] bool append_extended_device_reference(
    ByteWriter& writer,
    const ProtocolConfig& config,
    const QualifiedBufferWordDevice& device) noexcept {
  if (config.code_mode == CodeMode::Ascii) {
    return append_extended_device_reference_ascii(writer, config, device);
  }
  return append_extended_device_reference_binary(writer, config, device);
}

[[nodiscard]] Status parse_word_values_response(
    const ProtocolConfig& config,
    std::uint16_t points,
    std::span<const std::uint8_t> response_data,
    std::span<std::uint16_t> out_words,
    const char* ascii_error_prefix,
    const char* binary_error_prefix) noexcept {
  if (out_words.size() < points) {
    return buffer_too_small("Word output buffer is too small");
  }
  if (config.code_mode == CodeMode::Ascii) {
    if (response_data.size() != (static_cast<std::size_t>(points) * 4U)) {
      return parse_error(ascii_error_prefix);
    }
    for (std::size_t index = 0; index < points; ++index) {
      if (!parse_ascii_word(response_data.subspan(index * 4U, 4U), out_words[index])) {
        return parse_error("Failed to parse ASCII word payload");
      }
    }
    return ok_status();
  }

  if (response_data.size() != (static_cast<std::size_t>(points) * 2U)) {
    return parse_error(binary_error_prefix);
  }
  for (std::size_t index = 0; index < points; ++index) {
    if (!parse_binary_word(response_data, index * 2U, out_words[index])) {
      return parse_error("Failed to parse binary word payload");
    }
  }
  return ok_status();
}

[[nodiscard]] std::uint8_t compute_sum_check_byte(std::span<const std::uint8_t> bytes) noexcept {
  std::uint32_t total = 0;
  for (const std::uint8_t byte : bytes) {
    total += byte;
  }
  return static_cast<std::uint8_t>(total & 0xFFU);
}

[[nodiscard]] std::array<std::uint8_t, 2> compute_sum_check_ascii(
    std::span<const std::uint8_t> bytes) noexcept {
  const auto sum = compute_sum_check_byte(bytes);
  return {nibble_to_ascii(static_cast<std::uint8_t>((sum >> 4U) & 0x0FU)), nibble_to_ascii(sum & 0x0FU)};
}

[[nodiscard]] bool verify_ascii_sum(
    std::span<const std::uint8_t> payload,
    std::span<const std::uint8_t> received_sum) noexcept {
  if (received_sum.size() != 2U) {
    return false;
  }
  const auto expected = compute_sum_check_ascii(payload);
  return expected[0] == received_sum[0] && expected[1] == received_sum[1];
}

[[nodiscard]] bool encode_ascii_route(ByteWriter& writer, const ProtocolConfig& config) noexcept {
  if (!append_ascii_hex(writer, config.route.station_no, 2)) {
    return false;
  }
  if (!append_ascii_hex(writer, config.route.network_no, 2)) {
    return false;
  }
  if (!append_ascii_hex(writer, config.route.pc_no, 2)) {
    return false;
  }
  if (config.frame_kind == FrameKind::C4) {
    if (!append_ascii_hex(writer, config.route.request_destination_module_io_no, 4)) {
      return false;
    }
    if (!append_ascii_hex(writer, config.route.request_destination_module_station_no, 2)) {
      return false;
    }
  }
  return append_ascii_hex(writer, config.route.self_station_enabled ? config.route.self_station_no : 0U, 2);
}

[[nodiscard]] bool encode_binary_route(ByteWriter& writer, const ProtocolConfig& config) noexcept {
  return writer.push(config.route.station_no) &&
         writer.push(config.route.network_no) &&
         writer.push(config.route.pc_no) &&
         writer.append_le16(config.route.request_destination_module_io_no) &&
         writer.push(config.route.request_destination_module_station_no) &&
         writer.push(config.route.self_station_enabled ? config.route.self_station_no : 0U);
}

[[nodiscard]] Status unsupported(const char* message) noexcept {
  return make_status(StatusCode::UnsupportedConfiguration, message);
}

[[nodiscard]] Status invalid_argument(const char* message) noexcept {
  return make_status(StatusCode::InvalidArgument, message);
}

[[nodiscard]] Status buffer_too_small(const char* message) noexcept {
  return make_status(StatusCode::BufferTooSmall, message);
}

[[nodiscard]] Status parse_error(const char* message) noexcept {
  return make_status(StatusCode::Parse, message);
}

[[nodiscard]] Status framing_error(const char* message) noexcept {
  return make_status(StatusCode::Framing, message);
}

[[nodiscard]] Status sum_error(const char* message) noexcept {
  return make_status(StatusCode::SumCheckMismatch, message);
}

[[nodiscard]] bool parse_ascii_hex(
    std::span<const std::uint8_t> text,
    std::uint32_t& value) noexcept {
  value = 0;
  for (const std::uint8_t byte : text) {
    const int nibble = ascii_to_nibble(byte);
    if (nibble < 0) {
      return false;
    }
    value = (value << 4U) | static_cast<std::uint32_t>(nibble);
  }
  return true;
}

[[nodiscard]] bool parse_ascii_word(
    std::span<const std::uint8_t> text,
    std::uint16_t& value) noexcept {
  std::uint32_t parsed = 0;
  if (!parse_ascii_hex(text, parsed) || parsed > 0xFFFFU) {
    return false;
  }
  value = static_cast<std::uint16_t>(parsed);
  return true;
}

[[maybe_unused]] [[nodiscard]] bool parse_ascii_dword(
    std::span<const std::uint8_t> text,
    std::uint32_t& value) noexcept {
  return parse_ascii_hex(text, value);
}

[[nodiscard]] bool parse_binary_word(
    std::span<const std::uint8_t> bytes,
    std::size_t offset,
    std::uint16_t& value) noexcept {
  if ((offset + 2U) > bytes.size()) {
    return false;
  }
  value = static_cast<std::uint16_t>(bytes[offset] | (bytes[offset + 1U] << 8U));
  return true;
}

[[maybe_unused]] [[nodiscard]] bool parse_binary_dword(
    std::span<const std::uint8_t> bytes,
    std::size_t offset,
    std::uint32_t& value) noexcept {
  if ((offset + 4U) > bytes.size()) {
    return false;
  }
  value = static_cast<std::uint32_t>(bytes[offset]) |
          (static_cast<std::uint32_t>(bytes[offset + 1U]) << 8U) |
          (static_cast<std::uint32_t>(bytes[offset + 2U]) << 16U) |
          (static_cast<std::uint32_t>(bytes[offset + 3U]) << 24U);
  return true;
}

[[nodiscard]] bool append_command_header(
    ByteWriter& writer,
    const ProtocolConfig& config,
    std::uint16_t command,
    std::uint16_t subcommand) noexcept {
  if (config.code_mode == CodeMode::Ascii) {
    return append_ascii_hex(writer, command, 4) && append_ascii_hex(writer, subcommand, 4);
  }
  return writer.append_le16(command) && writer.append_le16(subcommand);
}

[[nodiscard]] bool append_word_count(
    ByteWriter& writer,
    const ProtocolConfig& config,
    std::uint16_t count) noexcept {
  if (config.code_mode == CodeMode::Ascii) {
    return append_ascii_hex(writer, count, 4);
  }
  return writer.append_le16(count);
}

[[maybe_unused]] [[nodiscard]] bool append_dword_data_ascii(ByteWriter& writer, std::uint32_t value) noexcept {
  return append_ascii_hex(writer, value, 8);
}

[[nodiscard]] bool append_word_data_ascii(ByteWriter& writer, std::uint16_t value) noexcept {
  return append_ascii_hex(writer, value, 4);
}

[[nodiscard]] bool append_bit_units_ascii(
    ByteWriter& writer,
    std::span<const BitValue> bits) noexcept {
  for (const BitValue bit : bits) {
    if (!writer.push(bit == BitValue::On ? static_cast<std::uint8_t>('1')
                                         : static_cast<std::uint8_t>('0'))) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool append_bit_units_binary(
    ByteWriter& writer,
    std::span<const BitValue> bits) noexcept {
  for (std::size_t index = 0; index < bits.size(); index += 2U) {
    const std::uint8_t low = bits[index] == BitValue::On ? 0x01U : 0x00U;
    const std::uint8_t high = ((index + 1U) < bits.size() && bits[index + 1U] == BitValue::On) ? 0x10U : 0x00U;
    if (!writer.push(static_cast<std::uint8_t>(low | high))) {
      return false;
    }
  }
  return true;
}

[[maybe_unused]] [[nodiscard]] bool append_word_units_from_bits_ascii(
    ByteWriter& writer,
    std::span<const BitValue> bits) noexcept {
  if ((bits.size() % 16U) != 0U) {
    return false;
  }
  for (std::size_t offset = 0; offset < bits.size(); offset += 16U) {
    std::uint16_t value = 0;
    for (std::size_t index = 0; index < 16U; ++index) {
      if (bits[offset + index] == BitValue::On) {
        value = static_cast<std::uint16_t>(value | (1U << index));
      }
    }
    if (!append_word_data_ascii(writer, value)) {
      return false;
    }
  }
  return true;
}

[[maybe_unused]] [[nodiscard]] bool append_word_units_from_bits_binary(
    ByteWriter& writer,
    std::span<const BitValue> bits) noexcept {
  if ((bits.size() % 16U) != 0U) {
    return false;
  }
  for (std::size_t offset = 0; offset < bits.size(); offset += 16U) {
    std::uint16_t value = 0;
    for (std::size_t index = 0; index < 16U; ++index) {
      if (bits[offset + index] == BitValue::On) {
        value = static_cast<std::uint16_t>(value | (1U << index));
      }
    }
    if (!writer.append_le16(value)) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool append_word_data(
    ByteWriter& writer,
    const ProtocolConfig& config,
    std::span<const std::uint16_t> words) noexcept {
  for (const std::uint16_t value : words) {
    if (config.code_mode == CodeMode::Ascii) {
      if (!append_word_data_ascii(writer, value)) {
        return false;
      }
    } else if (!writer.append_le16(value)) {
      return false;
    }
  }
  return true;
}

[[maybe_unused]] [[nodiscard]] bool append_byte_data(
    ByteWriter& writer,
    const ProtocolConfig& config,
    std::span<const std::byte> bytes) noexcept {
  for (const std::byte value : bytes) {
    if (config.code_mode == CodeMode::Ascii) {
      if (!append_ascii_hex(writer, static_cast<std::uint8_t>(value), 2)) {
        return false;
      }
    } else if (!writer.push(static_cast<std::uint8_t>(value))) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] Status encode_frame_payload_ascii(
    const ProtocolConfig& config,
    std::span<const std::uint8_t> request_data,
    std::span<std::uint8_t> out_payload,
    std::size_t& out_size) noexcept {
  ByteWriter payload_writer(out_payload);
  if (!append_ascii_hex(payload_writer, frame_id(config.frame_kind), 2) ||
      !encode_ascii_route(payload_writer, config) ||
      !payload_writer.append(request_data)) {
    return buffer_too_small("ASCII frame payload buffer is too small");
  }
  out_size = payload_writer.size();
  return ok_status();
}

[[nodiscard]] Status encode_frame_payload_binary(
    const ProtocolConfig& config,
    std::span<const std::uint8_t> request_data,
    std::span<std::uint8_t> out_payload,
    std::size_t& out_size) noexcept {
  ByteWriter payload_writer(out_payload);
  if (!payload_writer.push(frame_id(config.frame_kind)) ||
      !encode_binary_route(payload_writer, config) ||
      !payload_writer.append(request_data)) {
    return buffer_too_small("Binary frame payload buffer is too small");
  }
  out_size = payload_writer.size();
  return ok_status();
}

[[nodiscard]] bool append_ascii_response_data(
    RawResponseFrame& frame,
    std::span<const std::uint8_t> bytes) noexcept {
  if (bytes.size() > frame.response_data.size()) {
    return false;
  }
  std::memcpy(frame.response_data.data(), bytes.data(), bytes.size());
  frame.response_size = bytes.size();
  return true;
}

[[nodiscard]] Status validate_bit_device(const DeviceAddress& device, const char* message) noexcept {
  const DeviceSpec* spec = find_device_spec(device.code);
  if (spec == nullptr || !spec->bit_device) {
    return invalid_argument(message);
  }
  return ok_status();
}

[[nodiscard]] Status validate_request_data_size(
    const ProtocolConfig& config,
    std::span<const std::uint8_t> request_data) noexcept {
  if (config.code_mode == CodeMode::Ascii) {
    return request_data.size() <= kMaxRequestDataBytes ? ok_status()
                                                       : buffer_too_small("ASCII request data exceeds maximum size");
  }
  return request_data.size() <= kMaxRequestDataBytes ? ok_status()
                                                     : buffer_too_small("Binary request data exceeds maximum size");
}

[[maybe_unused]] [[nodiscard]] Status validate_loopback_chars(std::span<const char> hex_ascii) noexcept {
  if (hex_ascii.empty() || hex_ascii.size() > kMaxLoopbackBytes) {
    return invalid_argument("Loopback data length must be in range 1..960");
  }
  for (const char ch : hex_ascii) {
    if (!std::isxdigit(static_cast<unsigned char>(ch))) {
      return invalid_argument("Loopback data must contain only 0-9 and A-F");
    }
  }
  return ok_status();
}

[[maybe_unused]] void trim_right_spaces(std::array<char, kCpuModelNameLength + 1>& text) noexcept {
  std::size_t end = kCpuModelNameLength;
  while (end > 0U && text[end - 1U] == ' ') {
    --end;
  }
  text[end] = '\0';
}

[[nodiscard]] bool append_crlf(ByteWriter& writer) noexcept {
  return writer.push(kAsciiCr) && writer.push(kAsciiLf);
}

[[nodiscard]] bool has_ascii_crlf(std::span<const std::uint8_t> bytes, std::size_t offset) noexcept {
  return bytes.size() >= (offset + 2U) && bytes[offset] == kAsciiCr && bytes[offset + 1U] == kAsciiLf;
}

}  // namespace

Status FrameCodec::validate_config(const ProtocolConfig& config) noexcept {
  if (config.frame_kind == FrameKind::C2 || config.frame_kind == FrameKind::C1) {
    return unsupported("Only 4C and 3C frames are supported in v1");
  }

  if (config.code_mode == CodeMode::Ascii) {
    if (config.ascii_format != AsciiFormat::Format1 &&
        config.ascii_format != AsciiFormat::Format3 &&
        config.ascii_format != AsciiFormat::Format4) {
      return unsupported("Only ASCII Format1, Format3, and Format4 are supported");
    }
  } else if (config.frame_kind != FrameKind::C4) {
    return unsupported("Binary mode supports only 4C Format5");
  }

  if (config.route.self_station_enabled && config.route.self_station_no > 0x1FU) {
    return invalid_argument("Self-station number must be in range 0x00..0x1F");
  }

  if (config.route.kind == RouteKind::HostStation) {
    if (config.route.station_no != 0x00U || config.route.network_no != 0x00U || config.route.pc_no != 0xFFU) {
      return invalid_argument("Host station route must use station=0, network=0, pc=FF");
    }
  } else if (config.route.station_no > 0x1FU || config.route.network_no != 0x00U || config.route.pc_no != 0xFFU) {
    return invalid_argument("Multidrop route must use station=0x00..0x1F, network=0, pc=FF");
  }

  return ok_status();
}

Status FrameCodec::encode_request(
    const ProtocolConfig& config,
    std::span<const std::uint8_t> request_data,
    std::span<std::uint8_t> out_frame,
    std::size_t& out_size) noexcept {
  const Status config_status = validate_config(config);
  if (!config_status.ok()) {
    return config_status;
  }

  const Status request_status = validate_request_data_size(config, request_data);
  if (!request_status.ok()) {
    return request_status;
  }

  std::array<std::uint8_t, kMaxRequestFrameBytes> payload_storage {};
  std::size_t payload_size = 0;
  Status status = (config.code_mode == CodeMode::Ascii)
                      ? encode_frame_payload_ascii(config, request_data, payload_storage, payload_size)
                      : encode_frame_payload_binary(config, request_data, payload_storage, payload_size);
  if (!status.ok()) {
    return status;
  }

  ByteWriter frame_writer(out_frame);
  if (config.code_mode == CodeMode::Ascii) {
    if (is_ascii_format1_family(config)) {
      if (!frame_writer.push(kAsciiEnq) || !frame_writer.append(std::span<const std::uint8_t>(payload_storage.data(), payload_size))) {
        return buffer_too_small("ASCII request frame buffer is too small");
      }
      if (config.sum_check_enabled) {
        const auto sum = compute_sum_check_ascii(std::span<const std::uint8_t>(payload_storage.data(), payload_size));
        if (!frame_writer.append(sum)) {
          return buffer_too_small("ASCII request sum-check buffer is too small");
        }
      }
    } else {
      if (!frame_writer.push(kAsciiStx) || !frame_writer.append(std::span<const std::uint8_t>(payload_storage.data(), payload_size)) ||
          !frame_writer.push(kAsciiEtx)) {
        return buffer_too_small("ASCII request frame buffer is too small");
      }
      if (config.sum_check_enabled) {
        std::array<std::uint8_t, kMaxRequestFrameBytes + 1U> sum_bytes {};
        std::memcpy(sum_bytes.data(), payload_storage.data(), payload_size);
        sum_bytes[payload_size] = kAsciiEtx;
        const auto sum = compute_sum_check_ascii(std::span<const std::uint8_t>(sum_bytes.data(), payload_size + 1U));
        if (!frame_writer.append(sum)) {
          return buffer_too_small("ASCII request sum-check buffer is too small");
        }
      }
    }
    if (uses_ascii_crlf(config) && !append_crlf(frame_writer)) {
      return buffer_too_small("ASCII request CRLF buffer is too small");
    }
  } else {
    std::array<std::uint8_t, kMaxRequestFrameBytes> binary_payload {};
    ByteWriter payload_writer(binary_payload);
    const std::uint16_t byte_count = static_cast<std::uint16_t>(payload_size);
    if (!payload_writer.append_le16(byte_count) ||
        !payload_writer.append(std::span<const std::uint8_t>(payload_storage.data(), payload_size))) {
      return buffer_too_small("Binary payload buffer is too small");
    }

    if (!frame_writer.push(kBinaryDle) || !frame_writer.push(kAsciiStx)) {
      return buffer_too_small("Binary request frame buffer is too small");
    }

    for (const std::uint8_t byte : payload_writer.written()) {
      if (!frame_writer.push(byte)) {
        return buffer_too_small("Binary request frame buffer is too small");
      }
      if (byte == kBinaryDle && !frame_writer.push(kBinaryDle)) {
        return buffer_too_small("Binary request frame buffer is too small");
      }
    }

    if (!frame_writer.push(kBinaryDle) || !frame_writer.push(kAsciiEtx)) {
      return buffer_too_small("Binary request frame buffer is too small");
    }

    if (config.sum_check_enabled) {
      const auto sum = compute_sum_check_ascii(payload_writer.written());
      if (!frame_writer.append(sum)) {
        return buffer_too_small("Binary request sum-check buffer is too small");
      }
    }
  }

  out_size = frame_writer.size();
  return ok_status();
}

Status FrameCodec::encode_success_response(
    const ProtocolConfig& config,
    std::span<const std::uint8_t> response_data,
    std::span<std::uint8_t> out_frame,
    std::size_t& out_size) noexcept {
  const Status config_status = validate_config(config);
  if (!config_status.ok()) {
    return config_status;
  }

  if (config.code_mode == CodeMode::Ascii) {
    std::array<std::uint8_t, kMaxRequestFrameBytes> payload_storage {};
    std::size_t payload_size = 0;
    Status payload_status = encode_frame_payload_ascii(config, {}, payload_storage, payload_size);
    if (!payload_status.ok()) {
      return payload_status;
    }

    const std::size_t prefix_size = 2U + ascii_route_length(config.frame_kind);
    ByteWriter writer(out_frame);
    if (is_ascii_format1_family(config)) {
      if (response_data.empty()) {
        if (!writer.push(kAsciiAck) ||
            !writer.append(std::span<const std::uint8_t>(payload_storage.data(), prefix_size))) {
          return buffer_too_small("ASCII response frame buffer is too small");
        }
      } else {
        if (!writer.push(kAsciiStx) ||
            !writer.append(std::span<const std::uint8_t>(payload_storage.data(), prefix_size)) ||
            !writer.append(response_data) ||
            !writer.push(kAsciiEtx)) {
          return buffer_too_small("ASCII response frame buffer is too small");
        }
        if (config.sum_check_enabled) {
          std::array<std::uint8_t, kMaxRequestFrameBytes + 1U> sum_bytes {};
          std::memcpy(sum_bytes.data(), payload_storage.data(), prefix_size);
          std::memcpy(sum_bytes.data() + prefix_size, response_data.data(), response_data.size());
          sum_bytes[prefix_size + response_data.size()] = kAsciiEtx;
          const auto sum = compute_sum_check_ascii(std::span<const std::uint8_t>(
              sum_bytes.data(),
              prefix_size + response_data.size() + 1U));
          if (!writer.append(sum)) {
            return buffer_too_small("ASCII response sum-check buffer is too small");
          }
        }
      }
    } else {
      if (!writer.push(kAsciiStx) ||
          !writer.append(std::span<const std::uint8_t>(payload_storage.data(), prefix_size)) ||
          !writer.append(std::span<const std::uint8_t>(
              reinterpret_cast<const std::uint8_t*>("QACK"),
              4U)) ||
          !writer.append(response_data) ||
          !writer.push(kAsciiEtx)) {
        return buffer_too_small("ASCII response frame buffer is too small");
      }
      if (config.sum_check_enabled && !response_data.empty()) {
        const auto written = writer.written();
        const auto sum = compute_sum_check_ascii(written.subspan(1));
        if (!writer.append(sum)) {
          return buffer_too_small("ASCII response sum-check buffer is too small");
        }
      }
    }
    if (uses_ascii_crlf(config) && !append_crlf(writer)) {
      return buffer_too_small("ASCII response CRLF buffer is too small");
    }

    out_size = writer.size();
    return ok_status();
  }

  std::array<std::uint8_t, kMaxRequestFrameBytes> body {};
  ByteWriter body_writer(body);
  if (!body_writer.push(frame_id(config.frame_kind)) ||
      !encode_binary_route(body_writer, config) ||
      !body_writer.append_le16(0xFFFFU) ||
      !body_writer.append_le16(0x0000U) ||
      !body_writer.append(response_data)) {
    return buffer_too_small("Binary response body buffer is too small");
  }

  std::array<std::uint8_t, kMaxRequestFrameBytes> payload {};
  ByteWriter payload_writer(payload);
  if (!payload_writer.append_le16(static_cast<std::uint16_t>(body_writer.size())) ||
      !payload_writer.append(body_writer.written())) {
    return buffer_too_small("Binary response payload buffer is too small");
  }

  ByteWriter frame_writer(out_frame);
  if (!frame_writer.push(kBinaryDle) || !frame_writer.push(kAsciiStx)) {
    return buffer_too_small("Binary response frame buffer is too small");
  }
  for (const std::uint8_t byte : payload_writer.written()) {
    if (!frame_writer.push(byte)) {
      return buffer_too_small("Binary response frame buffer is too small");
    }
    if (byte == kBinaryDle && !frame_writer.push(kBinaryDle)) {
      return buffer_too_small("Binary response frame buffer is too small");
    }
  }
  if (!frame_writer.push(kBinaryDle) || !frame_writer.push(kAsciiEtx)) {
    return buffer_too_small("Binary response frame buffer is too small");
  }
  if (config.sum_check_enabled) {
    const auto sum = compute_sum_check_ascii(payload_writer.written());
    if (!frame_writer.append(sum)) {
      return buffer_too_small("Binary response sum-check buffer is too small");
    }
  }
  out_size = frame_writer.size();
  return ok_status();
}

Status FrameCodec::encode_error_response(
    const ProtocolConfig& config,
    std::uint16_t error_code,
    std::span<std::uint8_t> out_frame,
    std::size_t& out_size) noexcept {
  const Status config_status = validate_config(config);
  if (!config_status.ok()) {
    return config_status;
  }

  if (config.code_mode == CodeMode::Ascii) {
    std::array<std::uint8_t, kMaxRequestFrameBytes> payload_storage {};
    std::size_t payload_size = 0;
    Status payload_status = encode_frame_payload_ascii(config, {}, payload_storage, payload_size);
    if (!payload_status.ok()) {
      return payload_status;
    }

    const std::size_t prefix_size = 2U + ascii_route_length(config.frame_kind);
    ByteWriter writer(out_frame);
    if (is_ascii_format1_family(config)) {
      if (!writer.push(kAsciiNak) ||
          !writer.append(std::span<const std::uint8_t>(payload_storage.data(), prefix_size)) ||
          !append_ascii_hex(writer, error_code, 4)) {
        return buffer_too_small("ASCII error response frame buffer is too small");
      }
    } else {
      if (!writer.push(kAsciiStx) ||
          !writer.append(std::span<const std::uint8_t>(payload_storage.data(), prefix_size)) ||
          !writer.append(std::span<const std::uint8_t>(
              reinterpret_cast<const std::uint8_t*>("QNAK"),
              4U)) ||
          !append_ascii_hex(writer, error_code, 4) ||
          !writer.push(kAsciiEtx)) {
        return buffer_too_small("ASCII error response frame buffer is too small");
      }
    }
    if (uses_ascii_crlf(config) && !append_crlf(writer)) {
      return buffer_too_small("ASCII error response CRLF buffer is too small");
    }
    out_size = writer.size();
    return ok_status();
  }

  std::array<std::uint8_t, kMaxRequestFrameBytes> body {};
  ByteWriter body_writer(body);
  if (!body_writer.push(frame_id(config.frame_kind)) ||
      !encode_binary_route(body_writer, config) ||
      !body_writer.append_le16(0xFFFFU) ||
      !body_writer.append_le16(error_code)) {
    return buffer_too_small("Binary error response body buffer is too small");
  }

  std::array<std::uint8_t, kMaxRequestFrameBytes> payload {};
  ByteWriter payload_writer(payload);
  if (!payload_writer.append_le16(static_cast<std::uint16_t>(body_writer.size())) ||
      !payload_writer.append(body_writer.written())) {
    return buffer_too_small("Binary error response payload buffer is too small");
  }

  ByteWriter frame_writer(out_frame);
  if (!frame_writer.push(kBinaryDle) || !frame_writer.push(kAsciiStx)) {
    return buffer_too_small("Binary error response frame buffer is too small");
  }
  for (const std::uint8_t byte : payload_writer.written()) {
    if (!frame_writer.push(byte)) {
      return buffer_too_small("Binary error response frame buffer is too small");
    }
    if (byte == kBinaryDle && !frame_writer.push(kBinaryDle)) {
      return buffer_too_small("Binary error response frame buffer is too small");
    }
  }
  if (!frame_writer.push(kBinaryDle) || !frame_writer.push(kAsciiEtx)) {
    return buffer_too_small("Binary error response frame buffer is too small");
  }
  if (config.sum_check_enabled) {
    const auto sum = compute_sum_check_ascii(payload_writer.written());
    if (!frame_writer.append(sum)) {
      return buffer_too_small("Binary error response sum-check buffer is too small");
    }
  }
  out_size = frame_writer.size();
  return ok_status();
}

DecodeResult FrameCodec::decode_response(
    const ProtocolConfig& config,
    std::span<const std::uint8_t> bytes) noexcept {
  const Status config_status = validate_config(config);
  if (!config_status.ok()) {
    return DecodeResult {
        .status = DecodeStatus::Error,
        .frame = RawResponseFrame {},
        .error = config_status,
        .bytes_consumed = 0,
    };
  }

  if (bytes.empty()) {
    return DecodeResult {.status = DecodeStatus::Incomplete, .frame = RawResponseFrame {}, .error = ok_status(), .bytes_consumed = 0};
  }

  if (config.code_mode == CodeMode::Ascii) {
    const std::size_t prefix_size = 1U + 2U + ascii_route_length(config.frame_kind);
    const std::size_t terminator_size = uses_ascii_crlf(config) ? 2U : 0U;
    if (is_ascii_format1_family(config)) {
      if (bytes[0] == kAsciiAck) {
        if (bytes.size() < (prefix_size + terminator_size)) {
          return DecodeResult {.status = DecodeStatus::Incomplete, .frame = RawResponseFrame {}, .error = ok_status(), .bytes_consumed = 0};
        }
        if (terminator_size != 0U && !has_ascii_crlf(bytes, prefix_size)) {
          return DecodeResult {.status = DecodeStatus::Incomplete, .frame = RawResponseFrame {}, .error = ok_status(), .bytes_consumed = 0};
        }
        return DecodeResult {
            .status = DecodeStatus::Complete,
            .frame = RawResponseFrame {.type = ResponseType::SuccessNoData, .response_size = 0, .error_code = 0},
            .error = ok_status(),
            .bytes_consumed = prefix_size + terminator_size,
        };
      }

      if (bytes[0] == kAsciiNak) {
        if (bytes.size() < (prefix_size + 4U + terminator_size)) {
          return DecodeResult {.status = DecodeStatus::Incomplete, .frame = RawResponseFrame {}, .error = ok_status(), .bytes_consumed = 0};
        }
        if (terminator_size != 0U && !has_ascii_crlf(bytes, prefix_size + 4U)) {
          return DecodeResult {.status = DecodeStatus::Incomplete, .frame = RawResponseFrame {}, .error = ok_status(), .bytes_consumed = 0};
        }
        std::uint16_t error_code = 0;
        if (!parse_ascii_word(bytes.subspan(prefix_size, 4), error_code)) {
          return DecodeResult {
              .status = DecodeStatus::Error,
              .frame = RawResponseFrame {},
              .error = parse_error("Failed to parse ASCII Format1/4 error code"),
              .bytes_consumed = prefix_size + 4U + terminator_size,
          };
        }
        return DecodeResult {
            .status = DecodeStatus::Complete,
            .frame = RawResponseFrame {.type = ResponseType::PlcError, .response_size = 0, .error_code = error_code},
            .error = ok_status(),
            .bytes_consumed = prefix_size + 4U + terminator_size,
        };
      }

      if (bytes[0] != kAsciiStx) {
        return DecodeResult {
            .status = DecodeStatus::Error,
            .frame = RawResponseFrame {},
            .error = framing_error("ASCII Format1/4 response must begin with ACK, NAK, or STX"),
            .bytes_consumed = 1,
        };
      }

      const auto etx_it = std::find(bytes.begin() + static_cast<std::ptrdiff_t>(prefix_size), bytes.end(), kAsciiEtx);
      if (etx_it == bytes.end()) {
        return DecodeResult {.status = DecodeStatus::Incomplete, .frame = RawResponseFrame {}, .error = ok_status(), .bytes_consumed = 0};
      }

      const std::size_t etx_index = static_cast<std::size_t>(std::distance(bytes.begin(), etx_it));
      const std::size_t checksum_size = config.sum_check_enabled ? 2U : 0U;
      const std::size_t total_size = etx_index + 1U + checksum_size + terminator_size;
      if (bytes.size() < total_size) {
        return DecodeResult {.status = DecodeStatus::Incomplete, .frame = RawResponseFrame {}, .error = ok_status(), .bytes_consumed = 0};
      }
      if (terminator_size != 0U && !has_ascii_crlf(bytes, etx_index + 1U + checksum_size)) {
        return DecodeResult {.status = DecodeStatus::Incomplete, .frame = RawResponseFrame {}, .error = ok_status(), .bytes_consumed = 0};
      }

      if (config.sum_check_enabled &&
          !verify_ascii_sum(bytes.subspan(1, etx_index), bytes.subspan(etx_index + 1U, 2U))) {
        return DecodeResult {
            .status = DecodeStatus::Error,
            .frame = RawResponseFrame {},
            .error = sum_error("ASCII Format1/4 checksum mismatch"),
            .bytes_consumed = total_size,
        };
      }

      RawResponseFrame frame;
      frame.type = ResponseType::SuccessData;
      if (!append_ascii_response_data(frame, bytes.subspan(prefix_size, etx_index - prefix_size))) {
        return DecodeResult {
            .status = DecodeStatus::Error,
            .frame = RawResponseFrame {},
            .error = buffer_too_small("ASCII Format1/4 response payload is too large"),
            .bytes_consumed = total_size,
        };
      }
      return DecodeResult {
          .status = DecodeStatus::Complete,
          .frame = frame,
          .error = ok_status(),
          .bytes_consumed = total_size,
      };
    }

    if (bytes[0] != kAsciiStx) {
      return DecodeResult {
          .status = DecodeStatus::Error,
          .frame = RawResponseFrame {},
          .error = framing_error("ASCII Format3 response must begin with STX"),
          .bytes_consumed = 1,
      };
    }

    if (bytes.size() < (prefix_size + 4U)) {
      return DecodeResult {.status = DecodeStatus::Incomplete, .frame = RawResponseFrame {}, .error = ok_status(), .bytes_consumed = 0};
    }

    const std::size_t content_offset = prefix_size + 4U;
    const auto etx_it = std::find(bytes.begin() + static_cast<std::ptrdiff_t>(content_offset), bytes.end(), kAsciiEtx);
    if (etx_it == bytes.end()) {
      return DecodeResult {.status = DecodeStatus::Incomplete, .frame = RawResponseFrame {}, .error = ok_status(), .bytes_consumed = 0};
    }

    const std::size_t etx_index = static_cast<std::size_t>(std::distance(bytes.begin(), etx_it));
    const auto end_code = bytes.subspan(prefix_size, 4);

    if (std::memcmp(end_code.data(), "QACK", 4U) == 0) {
      const bool has_data = etx_index > content_offset;
      const std::size_t checksum_size = (config.sum_check_enabled && has_data) ? 2U : 0U;
      if (bytes.size() < (etx_index + 1U + checksum_size)) {
        return DecodeResult {.status = DecodeStatus::Incomplete, .frame = RawResponseFrame {}, .error = ok_status(), .bytes_consumed = 0};
      }
      if (checksum_size != 0U &&
          !verify_ascii_sum(bytes.subspan(1, etx_index), bytes.subspan(etx_index + 1U, 2U))) {
        return DecodeResult {
            .status = DecodeStatus::Error,
            .frame = RawResponseFrame {},
            .error = sum_error("ASCII Format3 checksum mismatch"),
            .bytes_consumed = etx_index + 1U + checksum_size,
        };
      }
      RawResponseFrame frame;
      frame.type = has_data ? ResponseType::SuccessData : ResponseType::SuccessNoData;
      if (has_data &&
          !append_ascii_response_data(frame, bytes.subspan(content_offset, etx_index - content_offset))) {
        return DecodeResult {
            .status = DecodeStatus::Error,
            .frame = RawResponseFrame {},
            .error = buffer_too_small("ASCII Format3 response payload is too large"),
            .bytes_consumed = etx_index + 1U + checksum_size,
        };
      }
      return DecodeResult {
          .status = DecodeStatus::Complete,
          .frame = frame,
          .error = ok_status(),
          .bytes_consumed = etx_index + 1U + checksum_size,
      };
    }

    if (std::memcmp(end_code.data(), "QNAK", 4U) != 0) {
      return DecodeResult {
          .status = DecodeStatus::Error,
          .frame = RawResponseFrame {},
          .error = parse_error("ASCII Format3 response must contain QACK or QNAK"),
          .bytes_consumed = etx_index + 1U,
      };
    }

    std::uint16_t error_code = 0;
    if (!parse_ascii_word(bytes.subspan(content_offset, etx_index - content_offset), error_code)) {
      return DecodeResult {
          .status = DecodeStatus::Error,
          .frame = RawResponseFrame {},
          .error = parse_error("Failed to parse ASCII Format3 error code"),
          .bytes_consumed = etx_index + 1U,
      };
    }
    return DecodeResult {
        .status = DecodeStatus::Complete,
        .frame = RawResponseFrame {.type = ResponseType::PlcError, .response_size = 0, .error_code = error_code},
        .error = ok_status(),
        .bytes_consumed = etx_index + 1U,
    };
  }

  if (bytes.size() < 4U) {
    return DecodeResult {.status = DecodeStatus::Incomplete, .frame = RawResponseFrame {}, .error = ok_status(), .bytes_consumed = 0};
  }

  if (bytes[0] != kBinaryDle || bytes[1] != kAsciiStx) {
    return DecodeResult {
        .status = DecodeStatus::Error,
        .frame = RawResponseFrame {},
        .error = framing_error("Binary Format5 response must begin with DLE STX"),
        .bytes_consumed = 1,
    };
  }

  std::array<std::uint8_t, kMaxResponseFrameBytes> payload {};
  std::size_t payload_size = 0;
  std::size_t index = 2U;
  bool found_tail = false;
  while (index < bytes.size()) {
    const std::uint8_t byte = bytes[index];
    if (byte != kBinaryDle) {
      if (payload_size >= payload.size()) {
        return DecodeResult {
            .status = DecodeStatus::Error,
            .frame = RawResponseFrame {},
            .error = buffer_too_small("Binary response payload is too large"),
            .bytes_consumed = index,
        };
      }
      payload[payload_size++] = byte;
      ++index;
      continue;
    }

    if ((index + 1U) >= bytes.size()) {
      return DecodeResult {.status = DecodeStatus::Incomplete, .frame = RawResponseFrame {}, .error = ok_status(), .bytes_consumed = 0};
    }

    if (bytes[index + 1U] == kBinaryDle) {
      if (payload_size >= payload.size()) {
        return DecodeResult {
            .status = DecodeStatus::Error,
            .frame = RawResponseFrame {},
            .error = buffer_too_small("Binary response payload is too large"),
            .bytes_consumed = index,
        };
      }
      payload[payload_size++] = kBinaryDle;
      index += 2U;
      continue;
    }

    if (bytes[index + 1U] == kAsciiEtx) {
      index += 2U;
      found_tail = true;
      break;
    }

    return DecodeResult {
        .status = DecodeStatus::Error,
        .frame = RawResponseFrame {},
        .error = framing_error("Unexpected DLE sequence in binary response"),
        .bytes_consumed = index + 1U,
    };
  }

  if (!found_tail) {
    return DecodeResult {.status = DecodeStatus::Incomplete, .frame = RawResponseFrame {}, .error = ok_status(), .bytes_consumed = 0};
  }

  const std::size_t checksum_size = config.sum_check_enabled ? 2U : 0U;
  if (bytes.size() < (index + checksum_size)) {
    return DecodeResult {.status = DecodeStatus::Incomplete, .frame = RawResponseFrame {}, .error = ok_status(), .bytes_consumed = 0};
  }

  if (config.sum_check_enabled &&
      !verify_ascii_sum(std::span<const std::uint8_t>(payload.data(), payload_size), bytes.subspan(index, 2U))) {
    return DecodeResult {
        .status = DecodeStatus::Error,
        .frame = RawResponseFrame {},
        .error = sum_error("Binary response checksum mismatch"),
        .bytes_consumed = index + checksum_size,
    };
  }

  if (payload_size < 2U) {
    return DecodeResult {
        .status = DecodeStatus::Error,
        .frame = RawResponseFrame {},
        .error = parse_error("Binary response payload is shorter than byte count"),
        .bytes_consumed = index + checksum_size,
    };
  }

  const std::uint16_t declared_count = static_cast<std::uint16_t>(payload[0] | (payload[1] << 8U));
  if (declared_count != static_cast<std::uint16_t>(payload_size - 2U)) {
    return DecodeResult {
        .status = DecodeStatus::Error,
        .frame = RawResponseFrame {},
        .error = framing_error("Binary response byte count does not match payload size"),
        .bytes_consumed = index + checksum_size,
    };
  }

  const std::size_t route_size = binary_route_length(config.frame_kind);
  const std::size_t minimum_body_size = 2U + 1U + route_size + 2U + 2U;
  if (payload_size < minimum_body_size) {
    return DecodeResult {
        .status = DecodeStatus::Error,
        .frame = RawResponseFrame {},
        .error = parse_error("Binary response payload is shorter than a valid frame"),
        .bytes_consumed = index + checksum_size,
    };
  }

  if (payload[2] != frame_id(config.frame_kind)) {
    return DecodeResult {
        .status = DecodeStatus::Error,
        .frame = RawResponseFrame {},
        .error = parse_error("Binary response frame ID does not match protocol"),
        .bytes_consumed = index + checksum_size,
    };
  }

  const std::size_t response_id_offset = 2U + 1U + route_size;
  const std::uint16_t response_id =
      static_cast<std::uint16_t>(payload[response_id_offset] | (payload[response_id_offset + 1U] << 8U));
  if (response_id != 0xFFFFU) {
    return DecodeResult {
        .status = DecodeStatus::Error,
        .frame = RawResponseFrame {},
        .error = parse_error("Binary response ID must be 0xFFFF"),
        .bytes_consumed = index + checksum_size,
    };
  }

  const std::size_t completion_offset = response_id_offset + 2U;
  const std::uint16_t completion_code =
      static_cast<std::uint16_t>(payload[completion_offset] | (payload[completion_offset + 1U] << 8U));

  RawResponseFrame frame;
  if (completion_code == 0x0000U) {
    frame.type = (payload_size == (completion_offset + 2U)) ? ResponseType::SuccessNoData : ResponseType::SuccessData;
    frame.response_size = payload_size - (completion_offset + 2U);
    std::memcpy(
        frame.response_data.data(),
        payload.data() + completion_offset + 2U,
        frame.response_size);
  } else {
    frame.type = ResponseType::PlcError;
    frame.error_code = completion_code;
  }

  return DecodeResult {
      .status = DecodeStatus::Complete,
      .frame = frame,
      .error = ok_status(),
      .bytes_consumed = index + checksum_size,
  };
}

namespace CommandCodec {

Status encode_batch_read_words(
    const ProtocolConfig& config,
    const BatchReadWordsRequest& request,
    std::span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept {
  if (request.points == 0U || request.points > kMaxBatchWordPoints) {
    return invalid_argument("Batch read words points must be in range 1..960");
  }
  ByteWriter writer(out_request_data);
  if (!append_command_header(writer, config, 0x0401U, word_subcommand(config)) ||
      !append_device_reference(writer, config, request.head_device) ||
      !append_word_count(writer, config, request.points)) {
    return buffer_too_small("Batch read words request buffer is too small");
  }
  out_size = writer.size();
  return ok_status();
}

Status encode_extended_batch_read_words(
    const ProtocolConfig& config,
    const QualifiedBufferWordDevice& device,
    std::uint16_t points,
    std::span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept {
  if (points == 0U || points > kMaxBatchWordPoints) {
    return invalid_argument("Extended batch read words points must be in range 1..960");
  }
  const Status device_status = validate_extended_word_device(config, device);
  if (!device_status.ok()) {
    return device_status;
  }
  ByteWriter writer(out_request_data);
  if (!append_command_header(writer, config, 0x0401U, extended_word_subcommand(config)) ||
      !append_extended_device_reference(writer, config, device) ||
      !append_word_count(writer, config, points)) {
    return buffer_too_small("Extended batch read words request buffer is too small");
  }
  out_size = writer.size();
  return ok_status();
}

Status parse_batch_read_words_response(
    const ProtocolConfig& config,
    const BatchReadWordsRequest& request,
    std::span<const std::uint8_t> response_data,
    std::span<std::uint16_t> out_words) noexcept {
  return parse_word_values_response(
      config,
      request.points,
      response_data,
      out_words,
      "Batch read words ASCII response length mismatch",
      "Batch read words binary response length mismatch");
}

Status parse_extended_batch_read_words_response(
    const ProtocolConfig& config,
    std::uint16_t points,
    std::span<const std::uint8_t> response_data,
    std::span<std::uint16_t> out_words) noexcept {
  return parse_word_values_response(
      config,
      points,
      response_data,
      out_words,
      "Extended batch read words ASCII response length mismatch",
      "Extended batch read words binary response length mismatch");
}

Status encode_batch_read_bits(
    const ProtocolConfig& config,
    const BatchReadBitsRequest& request,
    std::span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept {
  const Status bit_status = validate_bit_device(request.head_device, "Batch read bits requires a bit device");
  if (!bit_status.ok()) {
    return bit_status;
  }
  const std::uint16_t max_points =
      config.code_mode == CodeMode::Ascii ? kMaxBatchBitPointsAscii : kMaxBatchBitPointsBinary;
  if (request.points == 0U || request.points > max_points) {
    return invalid_argument("Batch read bits points are out of supported range");
  }
  ByteWriter writer(out_request_data);
  if (!append_command_header(writer, config, 0x0401U, bit_subcommand(config)) ||
      !append_device_reference(writer, config, request.head_device) ||
      !append_word_count(writer, config, request.points)) {
    return buffer_too_small("Batch read bits request buffer is too small");
  }
  out_size = writer.size();
  return ok_status();
}

Status parse_batch_read_bits_response(
    const ProtocolConfig& config,
    const BatchReadBitsRequest& request,
    std::span<const std::uint8_t> response_data,
    std::span<BitValue> out_bits) noexcept {
  if (out_bits.size() < request.points) {
    return buffer_too_small("Batch read bits output buffer is too small");
  }
  if (config.code_mode == CodeMode::Ascii) {
    if (response_data.size() != request.points) {
      return parse_error("Batch read bits ASCII response length mismatch");
    }
    for (std::size_t index = 0; index < request.points; ++index) {
      if (response_data[index] == '0') {
        out_bits[index] = BitValue::Off;
      } else if (response_data[index] == '1') {
        out_bits[index] = BitValue::On;
      } else {
        return parse_error("Batch read bits ASCII payload contains an invalid bit");
      }
    }
    return ok_status();
  }

  const std::size_t expected_size = (request.points + 1U) / 2U;
  if (response_data.size() != expected_size) {
    return parse_error("Batch read bits binary response length mismatch");
  }
  for (std::size_t index = 0; index < request.points; ++index) {
    const std::uint8_t packed = response_data[index / 2U];
    const std::uint8_t nibble = (index % 2U) == 0U ? static_cast<std::uint8_t>(packed & 0x0FU)
                                                   : static_cast<std::uint8_t>((packed >> 4U) & 0x0FU);
    if (nibble > 1U) {
      return parse_error("Batch read bits binary payload contains an invalid bit nibble");
    }
    out_bits[index] = nibble == 0U ? BitValue::Off : BitValue::On;
  }
  return ok_status();
}

Status encode_batch_write_words(
    const ProtocolConfig& config,
    const BatchWriteWordsRequest& request,
    std::span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept {
  const std::size_t max_points = batch_write_words_point_limit_for_buffer(config);
  if (request.words.empty() || request.words.size() > max_points) {
    return invalid_argument("Batch write words count exceeds supported range for the current buffer/configuration");
  }
  ByteWriter writer(out_request_data);
  if (!append_command_header(writer, config, 0x1401U, word_subcommand(config)) ||
      !append_device_reference(writer, config, request.head_device) ||
      !append_word_count(writer, config, static_cast<std::uint16_t>(request.words.size())) ||
      !append_word_data(writer, config, request.words)) {
    return buffer_too_small("Batch write words request buffer is too small");
  }
  out_size = writer.size();
  return ok_status();
}

Status encode_extended_batch_write_words(
    const ProtocolConfig& config,
    const QualifiedBufferWordDevice& device,
    std::span<const std::uint16_t> words,
    std::span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept {
  const std::size_t max_points = batch_write_words_point_limit_for_buffer(config);
  if (words.empty() || words.size() > max_points) {
    return invalid_argument("Extended batch write words count exceeds supported range for the current buffer/configuration");
  }
  const Status device_status = validate_extended_word_device(config, device);
  if (!device_status.ok()) {
    return device_status;
  }
  ByteWriter writer(out_request_data);
  if (!append_command_header(writer, config, 0x1401U, extended_word_subcommand(config)) ||
      !append_extended_device_reference(writer, config, device) ||
      !append_word_count(writer, config, static_cast<std::uint16_t>(words.size())) ||
      !append_word_data(writer, config, words)) {
    return buffer_too_small("Extended batch write words request buffer is too small");
  }
  out_size = writer.size();
  return ok_status();
}

Status encode_batch_write_bits(
    const ProtocolConfig& config,
    const BatchWriteBitsRequest& request,
    std::span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept {
  const Status bit_status = validate_bit_device(request.head_device, "Batch write bits requires a bit device");
  if (!bit_status.ok()) {
    return bit_status;
  }
  const std::size_t max_points = batch_write_bits_point_limit_for_buffer(config);
  if (request.bits.empty() || request.bits.size() > max_points) {
    return invalid_argument("Batch write bits count exceeds supported range for the current buffer/configuration");
  }
  ByteWriter writer(out_request_data);
  if (!append_command_header(writer, config, 0x1401U, bit_subcommand(config)) ||
      !append_device_reference(writer, config, request.head_device) ||
      !append_word_count(writer, config, static_cast<std::uint16_t>(request.bits.size()))) {
    return buffer_too_small("Batch write bits request buffer is too small");
  }

  const bool ok = (config.code_mode == CodeMode::Ascii)
                      ? append_bit_units_ascii(writer, request.bits)
                      : append_bit_units_binary(writer, request.bits);
  if (!ok) {
    return buffer_too_small("Batch write bits request buffer is too small");
  }
  out_size = writer.size();
  return ok_status();
}

#if MCPROTOCOL_SERIAL_ENABLE_RANDOM_COMMANDS || MCPROTOCOL_SERIAL_ENABLE_MONITOR_COMMANDS
Status encode_random_read(
    const ProtocolConfig& config,
    const RandomReadRequest& request,
    std::span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept {
  if (request.items.empty()) {
    return invalid_argument("Random read requires at least one item");
  }
  std::uint16_t word_count = 0;
  std::uint16_t dword_count = 0;
  for (const RandomReadItem& item : request.items) {
    item.double_word ? ++dword_count : ++word_count;
  }

  const std::uint16_t limit = word_subcommand(config) == 0x0000U ? 192U : 96U;
  if (static_cast<std::uint16_t>(word_count + dword_count) > limit) {
    return invalid_argument("Random read access-point count exceeds supported range");
  }

  ByteWriter writer(out_request_data);
  if (!append_command_header(writer, config, 0x0403U, word_subcommand(config)) ||
      !append_word_count(writer, config, word_count) ||
      !append_word_count(writer, config, dword_count)) {
    return buffer_too_small("Random read request buffer is too small");
  }

  for (const RandomReadItem& item : request.items) {
    if (!item.double_word && !append_device_reference(writer, config, item.device)) {
      return buffer_too_small("Random read request buffer is too small");
    }
  }
  for (const RandomReadItem& item : request.items) {
    if (item.double_word && !append_device_reference(writer, config, item.device)) {
      return buffer_too_small("Random read request buffer is too small");
    }
  }

  out_size = writer.size();
  return ok_status();
}
#else
Status encode_random_read(
    const ProtocolConfig& config,
    const RandomReadRequest& request,
    std::span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept {
  (void)config;
  (void)request;
  (void)out_request_data;
  (void)out_size;
  return unsupported("Random commands are disabled at build time");
}
#endif

#if MCPROTOCOL_SERIAL_ENABLE_RANDOM_COMMANDS || MCPROTOCOL_SERIAL_ENABLE_MONITOR_COMMANDS
Status parse_random_read_response(
    const ProtocolConfig& config,
    std::span<const RandomReadItem> items,
    std::span<const std::uint8_t> response_data,
    std::span<std::uint32_t> out_values) noexcept {
  if (out_values.size() < items.size()) {
    return buffer_too_small("Random read output buffer is too small");
  }

  std::size_t expected_size = 0;
  for (const RandomReadItem& item : items) {
    expected_size += item.double_word ? (config.code_mode == CodeMode::Ascii ? 8U : 4U)
                                      : (config.code_mode == CodeMode::Ascii ? 4U : 2U);
  }
  if (response_data.size() != expected_size) {
    return parse_error("Random read response length mismatch");
  }

  std::size_t cursor = 0;
  for (std::size_t index = 0; index < items.size(); ++index) {
    if (items[index].double_word) {
      continue;
    }
    if (config.code_mode == CodeMode::Ascii) {
      if (!parse_ascii_dword(response_data.subspan(cursor, 4U), out_values[index])) {
        return parse_error("Failed to parse random read ASCII word value");
      }
      out_values[index] &= 0xFFFFU;
      cursor += 4U;
    } else {
      std::uint16_t word_value = 0;
      if (!parse_binary_word(response_data, cursor, word_value)) {
        return parse_error("Failed to parse random read binary word value");
      }
      out_values[index] = word_value;
      cursor += 2U;
    }
  }
  for (std::size_t index = 0; index < items.size(); ++index) {
    if (!items[index].double_word) {
      continue;
    }
    if (config.code_mode == CodeMode::Ascii) {
      if (!parse_ascii_dword(response_data.subspan(cursor, 8U), out_values[index])) {
        return parse_error("Failed to parse random read ASCII double-word value");
      }
      cursor += 8U;
    } else {
      if (!parse_binary_dword(response_data, cursor, out_values[index])) {
        return parse_error("Failed to parse random read binary double-word value");
      }
      cursor += 4U;
    }
  }
  return ok_status();
}
#else
Status parse_random_read_response(
    const ProtocolConfig& config,
    std::span<const RandomReadItem> items,
    std::span<const std::uint8_t> response_data,
    std::span<std::uint32_t> out_values) noexcept {
  (void)config;
  (void)items;
  (void)response_data;
  (void)out_values;
  return unsupported("Random commands are disabled at build time");
}
#endif

#if MCPROTOCOL_SERIAL_ENABLE_RANDOM_COMMANDS
Status encode_random_write_words(
    const ProtocolConfig& config,
    std::span<const RandomWriteWordItem> items,
    std::span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept {
  if (items.empty()) {
    return invalid_argument("Random write words requires at least one item");
  }

  std::uint16_t word_count = 0;
  std::uint16_t dword_count = 0;
  for (const RandomWriteWordItem& item : items) {
    item.double_word ? ++dword_count : ++word_count;
  }
  const std::uint16_t weighted_limit = word_subcommand(config) == 0x0000U ? 1920U : 960U;
  const std::uint16_t weighted_size = static_cast<std::uint16_t>((word_count * 12U) + (dword_count * 14U));
  if (weighted_size == 0U || weighted_size > weighted_limit) {
    return invalid_argument("Random write words exceeds the supported request size");
  }

  ByteWriter writer(out_request_data);
  if (!append_command_header(writer, config, 0x1402U, word_subcommand(config)) ||
      !append_word_count(writer, config, word_count) ||
      !append_word_count(writer, config, dword_count)) {
    return buffer_too_small("Random write words request buffer is too small");
  }

  for (const RandomWriteWordItem& item : items) {
    if (!item.double_word &&
        (!append_device_reference(writer, config, item.device) ||
         (config.code_mode == CodeMode::Ascii ? !append_word_data_ascii(writer, static_cast<std::uint16_t>(item.value & 0xFFFFU))
                                              : !writer.append_le16(static_cast<std::uint16_t>(item.value & 0xFFFFU))))) {
      return buffer_too_small("Random write words request buffer is too small");
    }
  }
  for (const RandomWriteWordItem& item : items) {
    if (!item.double_word) {
      continue;
    }
    const bool ok =
        append_device_reference(writer, config, item.device) &&
        (config.code_mode == CodeMode::Ascii ? append_dword_data_ascii(writer, item.value)
                                             : writer.append_le32(item.value, 4U));
    if (!ok) {
      return buffer_too_small("Random write words request buffer is too small");
    }
  }

  out_size = writer.size();
  return ok_status();
}
#else
Status encode_random_write_words(
    const ProtocolConfig& config,
    std::span<const RandomWriteWordItem> items,
    std::span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept {
  (void)config;
  (void)items;
  (void)out_request_data;
  (void)out_size;
  return unsupported("Random commands are disabled at build time");
}
#endif

#if MCPROTOCOL_SERIAL_ENABLE_RANDOM_COMMANDS
Status encode_random_write_bits(
    const ProtocolConfig& config,
    std::span<const RandomWriteBitItem> items,
    std::span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept {
  if (items.empty()) {
    return invalid_argument("Random write bits requires at least one item");
  }
  const std::uint16_t limit = bit_subcommand(config) == 0x0001U ? 188U : 94U;
  if (items.size() > limit) {
    return invalid_argument("Random write bits count exceeds supported range");
  }
  for (const RandomWriteBitItem& item : items) {
    const Status bit_status = validate_bit_device(item.device, "Random write bits requires bit devices");
    if (!bit_status.ok()) {
      return bit_status;
    }
  }

  ByteWriter writer(out_request_data);
  if (!append_command_header(writer, config, 0x1402U, bit_subcommand(config)) ||
      !append_word_count(writer, config, static_cast<std::uint16_t>(items.size()))) {
    return buffer_too_small("Random write bits request buffer is too small");
  }
  for (const RandomWriteBitItem& item : items) {
    if (!append_device_reference(writer, config, item.device)) {
      return buffer_too_small("Random write bits request buffer is too small");
    }
    const std::uint16_t bit_value = item.value == BitValue::On ? 0x0001U : 0x0000U;
    const bool ok = config.code_mode == CodeMode::Ascii
                        ? (is_iq_r_series(config) ? append_word_data_ascii(writer, bit_value)
                                                  : append_ascii_hex(writer, bit_value, 2U))
                        : (is_iq_r_series(config) ? writer.append_le16(bit_value)
                                                  : writer.push(static_cast<std::uint8_t>(bit_value)));
    if (!ok) {
      return buffer_too_small("Random write bits request buffer is too small");
    }
  }
  out_size = writer.size();
  return ok_status();
}
#else
Status encode_random_write_bits(
    const ProtocolConfig& config,
    std::span<const RandomWriteBitItem> items,
    std::span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept {
  (void)config;
  (void)items;
  (void)out_request_data;
  (void)out_size;
  return unsupported("Random commands are disabled at build time");
}
#endif

#if MCPROTOCOL_SERIAL_ENABLE_MULTI_BLOCK_COMMANDS
Status encode_multi_block_read(
    const ProtocolConfig& config,
    const MultiBlockReadRequest& request,
    std::span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept {
  if (request.blocks.empty()) {
    return invalid_argument("Multi-block read requires at least one block");
  }

  std::uint16_t word_blocks = 0;
  std::uint16_t bit_blocks = 0;
  for (const MultiBlockReadBlock& block : request.blocks) {
    if (block.points == 0U || block.points > 960U) {
      return invalid_argument("Each multi-block read block must be in range 1..960 points");
    }
    if (block.bit_block) {
      ++bit_blocks;
    } else {
      ++word_blocks;
    }
  }
  const std::uint16_t limit = word_subcommand(config) == 0x0000U ? 120U : 60U;
  if (static_cast<std::uint16_t>(word_blocks + bit_blocks) > limit) {
    return invalid_argument("Multi-block read block count exceeds supported range");
  }

  ByteWriter writer(out_request_data);
  if (!append_command_header(writer, config, 0x0406U, word_subcommand(config)) ||
      !append_word_count(writer, config, word_blocks) ||
      !append_word_count(writer, config, bit_blocks)) {
    return buffer_too_small("Multi-block read request buffer is too small");
  }
  for (const MultiBlockReadBlock& block : request.blocks) {
    if (!block.bit_block &&
        (!append_device_reference(writer, config, block.head_device) ||
         !append_word_count(writer, config, block.points))) {
      return buffer_too_small("Multi-block read request buffer is too small");
    }
  }
  for (const MultiBlockReadBlock& block : request.blocks) {
    if (block.bit_block &&
        (!append_device_reference(writer, config, block.head_device) ||
         !append_word_count(writer, config, block.points))) {
      return buffer_too_small("Multi-block read request buffer is too small");
    }
  }
  out_size = writer.size();
  return ok_status();
}

Status parse_multi_block_read_response(
    const ProtocolConfig& config,
    std::span<const MultiBlockReadBlock> blocks,
    std::span<const std::uint8_t> response_data,
    std::span<std::uint16_t> out_words,
    std::span<BitValue> out_bits,
    std::span<MultiBlockReadBlockResult> out_results) noexcept {
  if (out_results.size() < blocks.size()) {
    return buffer_too_small("Multi-block read result metadata buffer is too small");
  }

  std::size_t expected_response_size = 0;
  std::size_t total_words = 0;
  std::size_t total_bits = 0;
  for (const MultiBlockReadBlock& block : blocks) {
    if (block.bit_block) {
      expected_response_size += static_cast<std::size_t>(block.points) * (config.code_mode == CodeMode::Ascii ? 4U : 2U);
      total_bits += static_cast<std::size_t>(block.points) * 16U;
    } else {
      expected_response_size += static_cast<std::size_t>(block.points) * (config.code_mode == CodeMode::Ascii ? 4U : 2U);
      total_words += block.points;
    }
  }
  if (response_data.size() != expected_response_size) {
    return parse_error("Multi-block read response length mismatch");
  }
  if (out_words.size() < total_words) {
    return buffer_too_small("Multi-block read word buffer is too small");
  }
  if (out_bits.size() < total_bits) {
    return buffer_too_small("Multi-block read bit buffer is too small");
  }

  std::size_t cursor = 0;
  std::size_t word_offset = 0;
  std::size_t bit_offset = 0;
  for (std::size_t index = 0; index < blocks.size(); ++index) {
    if (blocks[index].bit_block) {
      continue;
    }
    out_results[index] = MultiBlockReadBlockResult {
        .bit_block = false,
        .head_device = blocks[index].head_device,
        .points = blocks[index].points,
        .data_offset = static_cast<std::uint16_t>(word_offset),
        .data_count = blocks[index].points,
    };
    for (std::size_t point = 0; point < blocks[index].points; ++point) {
      if (config.code_mode == CodeMode::Ascii) {
        if (!parse_ascii_word(response_data.subspan(cursor, 4U), out_words[word_offset + point])) {
          return parse_error("Failed to parse multi-block read ASCII word data");
        }
        cursor += 4U;
      } else {
        if (!parse_binary_word(response_data, cursor, out_words[word_offset + point])) {
          return parse_error("Failed to parse multi-block read binary word data");
        }
        cursor += 2U;
      }
    }
    word_offset += blocks[index].points;
  }

  for (std::size_t index = 0; index < blocks.size(); ++index) {
    if (!blocks[index].bit_block) {
      continue;
    }
    out_results[index] = MultiBlockReadBlockResult {
        .bit_block = true,
        .head_device = blocks[index].head_device,
        .points = blocks[index].points,
        .data_offset = static_cast<std::uint16_t>(bit_offset),
        .data_count = static_cast<std::uint16_t>(blocks[index].points * 16U),
    };
    for (std::size_t point = 0; point < blocks[index].points; ++point) {
      std::uint16_t packed_word = 0;
      if (config.code_mode == CodeMode::Ascii) {
        if (!parse_ascii_word(response_data.subspan(cursor, 4U), packed_word)) {
          return parse_error("Failed to parse multi-block read ASCII bit data");
        }
        cursor += 4U;
      } else {
        if (!parse_binary_word(response_data, cursor, packed_word)) {
          return parse_error("Failed to parse multi-block read binary bit data");
        }
        cursor += 2U;
      }
      for (std::size_t bit = 0; bit < 16U; ++bit) {
        out_bits[bit_offset + (point * 16U) + bit] =
            ((packed_word >> bit) & 0x01U) != 0U ? BitValue::On : BitValue::Off;
      }
    }
    bit_offset += blocks[index].points * 16U;
  }
  return ok_status();
}

Status encode_multi_block_write(
    const ProtocolConfig& config,
    const MultiBlockWriteRequest& request,
    std::span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept {
  if (request.blocks.empty()) {
    return invalid_argument("Multi-block write requires at least one block");
  }

  std::uint16_t word_blocks = 0;
  std::uint16_t bit_blocks = 0;
  std::uint32_t weighted_points = 0;
  for (const MultiBlockWriteBlock& block : request.blocks) {
    if (block.points == 0U || block.points > 960U) {
      return invalid_argument("Each multi-block write block must be in range 1..960 points");
    }
    if (block.bit_block) {
      if (block.bits.size() != static_cast<std::size_t>(block.points) * 16U) {
        return invalid_argument("Bit block write data must contain points * 16 entries");
      }
      ++bit_blocks;
    } else {
      if (block.words.size() != block.points) {
        return invalid_argument("Word block write data must contain one word per point");
      }
      ++word_blocks;
    }
    weighted_points += 4U + block.points;
  }
  const std::uint16_t block_limit = word_subcommand(config) == 0x0000U ? 120U : 60U;
  if (static_cast<std::uint16_t>(word_blocks + bit_blocks) > block_limit || weighted_points > 960U) {
    return invalid_argument("Multi-block write request exceeds supported range");
  }

  ByteWriter writer(out_request_data);
  if (!append_command_header(writer, config, 0x1406U, word_subcommand(config)) ||
      !append_word_count(writer, config, word_blocks) ||
      !append_word_count(writer, config, bit_blocks)) {
    return buffer_too_small("Multi-block write request buffer is too small");
  }

  for (const MultiBlockWriteBlock& block : request.blocks) {
    if (block.bit_block) {
      continue;
    }
    if (!append_device_reference(writer, config, block.head_device) ||
        !append_word_count(writer, config, block.points) ||
        !append_word_data(writer, config, block.words)) {
      return buffer_too_small("Multi-block write request buffer is too small");
    }
  }
  for (const MultiBlockWriteBlock& block : request.blocks) {
    if (!block.bit_block) {
      continue;
    }
    const bool ok =
        append_device_reference(writer, config, block.head_device) &&
        append_word_count(writer, config, block.points) &&
        (config.code_mode == CodeMode::Ascii ? append_word_units_from_bits_ascii(writer, block.bits)
                                             : append_word_units_from_bits_binary(writer, block.bits));
    if (!ok) {
      return buffer_too_small("Multi-block write request buffer is too small");
    }
  }

  out_size = writer.size();
  return ok_status();
}
#else
Status encode_multi_block_read(
    const ProtocolConfig& config,
    const MultiBlockReadRequest& request,
    std::span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept {
  (void)config;
  (void)request;
  (void)out_request_data;
  (void)out_size;
  return unsupported("Multi-block commands are disabled at build time");
}

Status parse_multi_block_read_response(
    const ProtocolConfig& config,
    std::span<const MultiBlockReadBlock> blocks,
    std::span<const std::uint8_t> response_data,
    std::span<std::uint16_t> out_words,
    std::span<BitValue> out_bits,
    std::span<MultiBlockReadBlockResult> out_results) noexcept {
  (void)config;
  (void)blocks;
  (void)response_data;
  (void)out_words;
  (void)out_bits;
  (void)out_results;
  return unsupported("Multi-block commands are disabled at build time");
}

Status encode_multi_block_write(
    const ProtocolConfig& config,
    const MultiBlockWriteRequest& request,
    std::span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept {
  (void)config;
  (void)request;
  (void)out_request_data;
  (void)out_size;
  return unsupported("Multi-block commands are disabled at build time");
}
#endif

#if MCPROTOCOL_SERIAL_ENABLE_MONITOR_COMMANDS
Status encode_register_monitor(
    const ProtocolConfig& config,
    const MonitorRegistration& request,
    std::span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept {
  if (request.items.empty()) {
    return invalid_argument("Monitor registration requires at least one item");
  }
  RandomReadRequest random_request {.items = request.items};
  std::array<std::uint8_t, kMaxRequestDataBytes> random_request_data {};
  std::size_t inner_size = 0;
  Status status = encode_random_read(config, random_request, random_request_data, inner_size);
  if (!status.ok()) {
    return status;
  }

  ByteWriter patched(out_request_data);
  patched.clear();
  if (!append_command_header(patched, config, 0x0801U, word_subcommand(config))) {
    return buffer_too_small("Monitor registration request buffer is too small");
  }
  if (!patched.append(std::span<const std::uint8_t>(
          random_request_data.data() + (config.code_mode == CodeMode::Ascii ? 8U : 4U),
          inner_size - (config.code_mode == CodeMode::Ascii ? 8U : 4U)))) {
    return buffer_too_small("Monitor registration request buffer is too small");
  }
  out_size = patched.size();
  return ok_status();
}

Status encode_read_monitor(
    const ProtocolConfig& config,
    std::span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept {
  ByteWriter writer(out_request_data);
  if (!append_command_header(writer, config, 0x0802U, 0x0000U)) {
    return buffer_too_small("Monitor read request buffer is too small");
  }
  out_size = writer.size();
  return ok_status();
}

Status parse_read_monitor_response(
    const ProtocolConfig& config,
    std::span<const RandomReadItem> items,
    std::span<const std::uint8_t> response_data,
    std::span<std::uint32_t> out_values) noexcept {
  return parse_random_read_response(config, items, response_data, out_values);
}
#else
Status encode_register_monitor(
    const ProtocolConfig& config,
    const MonitorRegistration& request,
    std::span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept {
  (void)config;
  (void)request;
  (void)out_request_data;
  (void)out_size;
  return unsupported("Monitor commands are disabled at build time");
}

Status encode_read_monitor(
    const ProtocolConfig& config,
    std::span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept {
  (void)config;
  (void)out_request_data;
  (void)out_size;
  return unsupported("Monitor commands are disabled at build time");
}

Status parse_read_monitor_response(
    const ProtocolConfig& config,
    std::span<const RandomReadItem> items,
    std::span<const std::uint8_t> response_data,
    std::span<std::uint32_t> out_values) noexcept {
  (void)config;
  (void)items;
  (void)response_data;
  (void)out_values;
  return unsupported("Monitor commands are disabled at build time");
}
#endif

#if MCPROTOCOL_SERIAL_ENABLE_HOST_BUFFER_COMMANDS
Status encode_read_host_buffer(
    const ProtocolConfig& config,
    const HostBufferReadRequest& request,
    std::span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept {
  if (request.word_length == 0U || request.word_length > 480U) {
    return invalid_argument("Host buffer read length must be in range 1..480");
  }
  ByteWriter writer(out_request_data);
  const bool ok = append_command_header(writer, config, 0x0613U, 0x0000U) &&
                  (config.code_mode == CodeMode::Ascii ? append_ascii_hex(writer, request.start_address, 8)
                                                       : writer.append_le32(request.start_address, 4U)) &&
                  append_word_count(writer, config, request.word_length);
  if (!ok) {
    return buffer_too_small("Host buffer read request buffer is too small");
  }
  out_size = writer.size();
  return ok_status();
}

Status parse_read_host_buffer_response(
    const ProtocolConfig& config,
    const HostBufferReadRequest& request,
    std::span<const std::uint8_t> response_data,
    std::span<std::uint16_t> out_words) noexcept {
  return parse_batch_read_words_response(
      ProtocolConfig {
          .frame_kind = config.frame_kind,
          .code_mode = config.code_mode,
          .ascii_format = config.ascii_format,
          .target_series = config.target_series,
          .sum_check_enabled = config.sum_check_enabled,
          .route = config.route,
          .timeout = config.timeout,
      },
      BatchReadWordsRequest {.head_device = DeviceAddress {}, .points = request.word_length},
      response_data,
      out_words);
}

Status encode_write_host_buffer(
    const ProtocolConfig& config,
    const HostBufferWriteRequest& request,
    std::span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept {
  if (request.words.empty() || request.words.size() > 480U) {
    return invalid_argument("Host buffer write length must be in range 1..480");
  }
  ByteWriter writer(out_request_data);
  const bool ok = append_command_header(writer, config, 0x1613U, 0x0000U) &&
                  (config.code_mode == CodeMode::Ascii ? append_ascii_hex(writer, request.start_address, 8)
                                                       : writer.append_le32(request.start_address, 4U)) &&
                  append_word_count(writer, config, static_cast<std::uint16_t>(request.words.size())) &&
                  append_word_data(writer, config, request.words);
  if (!ok) {
    return buffer_too_small("Host buffer write request buffer is too small");
  }
  out_size = writer.size();
  return ok_status();
}
#else
Status encode_read_host_buffer(
    const ProtocolConfig& config,
    const HostBufferReadRequest& request,
    std::span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept {
  (void)config;
  (void)request;
  (void)out_request_data;
  (void)out_size;
  return unsupported("Host-buffer commands are disabled at build time");
}

Status parse_read_host_buffer_response(
    const ProtocolConfig& config,
    const HostBufferReadRequest& request,
    std::span<const std::uint8_t> response_data,
    std::span<std::uint16_t> out_words) noexcept {
  (void)config;
  (void)request;
  (void)response_data;
  (void)out_words;
  return unsupported("Host-buffer commands are disabled at build time");
}

Status encode_write_host_buffer(
    const ProtocolConfig& config,
    const HostBufferWriteRequest& request,
    std::span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept {
  (void)config;
  (void)request;
  (void)out_request_data;
  (void)out_size;
  return unsupported("Host-buffer commands are disabled at build time");
}
#endif

#if MCPROTOCOL_SERIAL_ENABLE_MODULE_BUFFER_COMMANDS
Status encode_read_module_buffer(
    const ProtocolConfig& config,
    const ModuleBufferReadRequest& request,
    std::span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept {
  if (request.bytes < 2U || request.bytes > 1920U) {
    return invalid_argument("Module buffer read byte count must be in range 2..1920");
  }
  ByteWriter writer(out_request_data);
  const bool ok = append_command_header(writer, config, 0x0601U, 0x0000U) &&
                  (config.code_mode == CodeMode::Ascii ? append_ascii_hex(writer, request.start_address, 8)
                                                       : writer.append_le32(request.start_address, 4U)) &&
                  append_word_count(writer, config, request.bytes) &&
                  append_word_count(writer, config, request.module_number);
  if (!ok) {
    return buffer_too_small("Module buffer read request buffer is too small");
  }
  out_size = writer.size();
  return ok_status();
}

Status parse_read_module_buffer_response(
    const ProtocolConfig& config,
    const ModuleBufferReadRequest& request,
    std::span<const std::uint8_t> response_data,
    std::span<std::byte> out_bytes) noexcept {
  if (out_bytes.size() < request.bytes) {
    return buffer_too_small("Module buffer output buffer is too small");
  }
  if (config.code_mode == CodeMode::Ascii) {
    if (response_data.size() != static_cast<std::size_t>(request.bytes) * 2U) {
      return parse_error("Module buffer ASCII response length mismatch");
    }
    for (std::size_t index = 0; index < request.bytes; ++index) {
      std::uint32_t byte_value = 0;
      if (!parse_ascii_hex(response_data.subspan(index * 2U, 2U), byte_value) || byte_value > 0xFFU) {
        return parse_error("Failed to parse module buffer ASCII response");
      }
      out_bytes[index] = static_cast<std::byte>(byte_value);
    }
    return ok_status();
  }
  if (response_data.size() != request.bytes) {
    return parse_error("Module buffer binary response length mismatch");
  }
  std::memcpy(out_bytes.data(), response_data.data(), request.bytes);
  return ok_status();
}

Status encode_write_module_buffer(
    const ProtocolConfig& config,
    const ModuleBufferWriteRequest& request,
    std::span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept {
  if (request.bytes.size() < 2U || request.bytes.size() > 1920U) {
    return invalid_argument("Module buffer write byte count must be in range 2..1920");
  }
  ByteWriter writer(out_request_data);
  const bool ok = append_command_header(writer, config, 0x1601U, 0x0000U) &&
                  (config.code_mode == CodeMode::Ascii ? append_ascii_hex(writer, request.start_address, 8)
                                                       : writer.append_le32(request.start_address, 4U)) &&
                  append_word_count(writer, config, static_cast<std::uint16_t>(request.bytes.size())) &&
                  append_word_count(writer, config, request.module_number) &&
                  append_byte_data(writer, config, request.bytes);
  if (!ok) {
    return buffer_too_small("Module buffer write request buffer is too small");
  }
  out_size = writer.size();
  return ok_status();
}
#else
Status encode_read_module_buffer(
    const ProtocolConfig& config,
    const ModuleBufferReadRequest& request,
    std::span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept {
  (void)config;
  (void)request;
  (void)out_request_data;
  (void)out_size;
  return unsupported("Module-buffer commands are disabled at build time");
}

Status parse_read_module_buffer_response(
    const ProtocolConfig& config,
    const ModuleBufferReadRequest& request,
    std::span<const std::uint8_t> response_data,
    std::span<std::byte> out_bytes) noexcept {
  (void)config;
  (void)request;
  (void)response_data;
  (void)out_bytes;
  return unsupported("Module-buffer commands are disabled at build time");
}

Status encode_write_module_buffer(
    const ProtocolConfig& config,
    const ModuleBufferWriteRequest& request,
    std::span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept {
  (void)config;
  (void)request;
  (void)out_request_data;
  (void)out_size;
  return unsupported("Module-buffer commands are disabled at build time");
}
#endif

#if MCPROTOCOL_SERIAL_ENABLE_CPU_MODEL_COMMANDS
Status encode_read_cpu_model(
    const ProtocolConfig& config,
    std::span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept {
  ByteWriter writer(out_request_data);
  if (!append_command_header(writer, config, 0x0101U, 0x0000U)) {
    return buffer_too_small("Read CPU model request buffer is too small");
  }
  out_size = writer.size();
  return ok_status();
}

Status parse_read_cpu_model_response(
    const ProtocolConfig& config,
    std::span<const std::uint8_t> response_data,
    CpuModelInfo& out_info) noexcept {
  if (config.code_mode == CodeMode::Ascii) {
    if (response_data.size() < 20U) {
      return parse_error("ASCII CPU model response must be at least 20 bytes");
    }
    std::memset(out_info.model_name.data(), 0, out_info.model_name.size());
    std::memcpy(out_info.model_name.data(), response_data.data(), kCpuModelNameLength);
    trim_right_spaces(out_info.model_name);
    if (!parse_ascii_word(response_data.subspan(16U, 4U), out_info.model_code)) {
      return parse_error("Failed to parse ASCII CPU model code");
    }
    return ok_status();
  }
  if (response_data.size() < 18U) {
    return parse_error("Binary CPU model response must be at least 18 bytes");
  }
  std::memset(out_info.model_name.data(), 0, out_info.model_name.size());
  std::memcpy(out_info.model_name.data(), response_data.data(), kCpuModelNameLength);
  trim_right_spaces(out_info.model_name);
  out_info.model_code = static_cast<std::uint16_t>(response_data[16] | (response_data[17] << 8U));
  return ok_status();
}
#else
Status encode_read_cpu_model(
    const ProtocolConfig& config,
    std::span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept {
  (void)config;
  (void)out_request_data;
  (void)out_size;
  return unsupported("CPU-model commands are disabled at build time");
}

Status parse_read_cpu_model_response(
    const ProtocolConfig& config,
    std::span<const std::uint8_t> response_data,
    CpuModelInfo& out_info) noexcept {
  (void)config;
  (void)response_data;
  (void)out_info;
  return unsupported("CPU-model commands are disabled at build time");
}
#endif

#if MCPROTOCOL_SERIAL_ENABLE_LOOPBACK_COMMANDS
Status encode_loopback(
    const ProtocolConfig& config,
    std::span<const char> hex_ascii,
    std::span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept {
  const Status validate = validate_loopback_chars(hex_ascii);
  if (!validate.ok()) {
    return validate;
  }
  ByteWriter writer(out_request_data);
  const bool header_ok = append_command_header(writer, config, 0x0619U, 0x0000U) &&
                         append_word_count(writer, config, static_cast<std::uint16_t>(hex_ascii.size()));
  if (!header_ok) {
    return buffer_too_small("Loopback request buffer is too small");
  }
  for (const char ch : hex_ascii) {
    const std::uint8_t upper = static_cast<std::uint8_t>(std::toupper(static_cast<unsigned char>(ch)));
    if (!writer.push(upper)) {
      return buffer_too_small("Loopback request buffer is too small");
    }
  }
  out_size = writer.size();
  return ok_status();
}

Status parse_loopback_response(
    const ProtocolConfig& config,
    std::span<const std::uint8_t> response_data,
    std::span<char> out_echoed) noexcept {
  std::size_t payload_length = 0;
  std::size_t payload_offset = 0;
  if (config.code_mode == CodeMode::Ascii) {
    std::uint32_t ascii_length = 0;
    if (response_data.size() < 4U || !parse_ascii_hex(response_data.first(4U), ascii_length)) {
      return parse_error("Failed to parse ASCII loopback response length");
    }
    payload_length = ascii_length;
    payload_offset = 4U;
  } else {
    if (response_data.size() < 2U) {
      return parse_error("Binary loopback response is shorter than the length field");
    }
    payload_length = static_cast<std::size_t>(response_data[0] | (response_data[1] << 8U));
    payload_offset = 2U;
  }

  if (response_data.size() != (payload_offset + payload_length)) {
    return parse_error("Loopback response length mismatch");
  }
  if (out_echoed.size() < payload_length) {
    return buffer_too_small("Loopback output buffer is too small");
  }
  for (std::size_t index = 0; index < payload_length; ++index) {
    out_echoed[index] = static_cast<char>(response_data[payload_offset + index]);
  }
  if (payload_length < out_echoed.size()) {
    out_echoed[payload_length] = '\0';
  }
  return ok_status();
}
#else
Status encode_loopback(
    const ProtocolConfig& config,
    std::span<const char> hex_ascii,
    std::span<std::uint8_t> out_request_data,
    std::size_t& out_size) noexcept {
  (void)config;
  (void)hex_ascii;
  (void)out_request_data;
  (void)out_size;
  return unsupported("Loopback commands are disabled at build time");
}

Status parse_loopback_response(
    const ProtocolConfig& config,
    std::span<const std::uint8_t> response_data,
    std::span<char> out_echoed) noexcept {
  (void)config;
  (void)response_data;
  (void)out_echoed;
  return unsupported("Loopback commands are disabled at build time");
}
#endif

}  // namespace CommandCodec

}  // namespace mcprotocol::serial
