#pragma once

#include "mcprotocol/serial/compat/array.hpp"
#include "mcprotocol/serial/compat/cstdint.hpp"
#include "mcprotocol/serial/high_level.hpp"

#include <memory>
#include <new>

namespace mcprotocol::serial::detail {

[[nodiscard]] constexpr std::size_t long_state_stage_bytes(std::uint16_t points) noexcept {
  return (static_cast<std::size_t>(points) + 7U) / 8U;
}

[[nodiscard]] constexpr std::size_t long_state_status_block_response_bytes(CodeMode mode) noexcept {
  constexpr std::size_t status_words = 4U;
  return status_words * (mode == CodeMode::Ascii ? 4U : sizeof(std::uint16_t));
}

struct LongStateStageAllocator {
  [[nodiscard]] std::unique_ptr<std::uint8_t[]> operator()(std::size_t size) const noexcept {
    return std::unique_ptr<std::uint8_t[]>(new (std::nothrow) std::uint8_t[size] {});
  }
};

/// Internal execution primitive shared by the host wrapper and deterministic aggregate tests.
/// The caller must validate and snapshot the complete plan before invoking this function.
template <typename ReadOneStatusBlock, typename AllocateBytes = LongStateStageAllocator>
[[nodiscard]] Status execute_long_state_read_aggregate(
    const highlevel::LongStateReadSpec& spec,
    std::uint32_t first_device_number,
    std::uint16_t points,
    mcprotocol::serial::Span<BitValue> out_bits,
    ReadOneStatusBlock& read_one,
    AllocateBytes allocate_bytes = {}) noexcept {
  const std::size_t staged_size = long_state_stage_bytes(points);
  auto staged_bits = allocate_bytes(staged_size);
  if (!staged_bits) {
    return make_status(StatusCode::OutOfMemory, "Long state staging allocation failed");
  }
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
