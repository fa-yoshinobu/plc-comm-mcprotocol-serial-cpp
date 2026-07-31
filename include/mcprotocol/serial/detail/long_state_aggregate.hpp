#pragma once

#include "mcprotocol/serial/compat/array.hpp"
#include "mcprotocol/serial/compat/cstdint.hpp"
#include "mcprotocol/serial/high_level.hpp"

namespace mcprotocol::serial::detail {

/// Internal execution primitive shared by the host wrapper and deterministic aggregate tests.
/// The caller must validate and snapshot the complete plan before invoking this function.
template <typename ReadOneStatusBlock>
[[nodiscard]] Status execute_long_state_read_aggregate(
    const highlevel::LongStateReadSpec& spec,
    std::uint32_t first_device_number,
    std::uint16_t points,
    mcprotocol::serial::Span<BitValue> out_bits,
    ReadOneStatusBlock& read_one) noexcept {
  std::array<std::uint8_t, (0xFFFFU + 7U) / 8U> staged_bits {};
  std::array<std::uint16_t, 4> status_block {};

  for (std::uint16_t index = 0; index < points; ++index) {
    const DeviceAddress device {
        spec.base_code, first_device_number + static_cast<std::uint32_t>(index)};
    Status status = read_one(
        device,
        mcprotocol::serial::Span<std::uint16_t>(status_block.data(), status_block.size()));
    if (!status.ok()) {
      return status;
    }

    BitValue value = false;
    status = highlevel::decode_long_state_bit(
        spec,
        mcprotocol::serial::Span<const std::uint16_t>(status_block.data(), status_block.size()),
        value);
    if (!status.ok()) {
      return status;
    }
    if (value) {
      staged_bits[index / 8U] = static_cast<std::uint8_t>(
          staged_bits[index / 8U] | static_cast<std::uint8_t>(1U << (index % 8U)));
    }
  }

  for (std::uint16_t index = 0; index < points; ++index) {
    out_bits[index] = (staged_bits[index / 8U] & static_cast<std::uint8_t>(1U << (index % 8U))) != 0U;
  }
  return ok_status();
}

}  // namespace mcprotocol::serial::detail
