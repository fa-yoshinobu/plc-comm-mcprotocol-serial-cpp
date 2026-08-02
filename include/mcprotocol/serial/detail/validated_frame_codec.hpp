#pragma once

#include "mcprotocol/serial/codec.hpp"

namespace mcprotocol::serial::detail {

/// Internal frame helpers for a ProtocolConfig already accepted by a session boundary.
/// Public FrameCodec entry points continue to validate arbitrary caller-supplied configurations.
[[nodiscard]] Status validate_request_capacity_validated(
    const ProtocolConfig& config,
    std::size_t request_data_size) noexcept;

[[nodiscard]] Status validate_response_capacity_validated(
    const ProtocolConfig& config,
    std::size_t response_data_size) noexcept;

[[nodiscard]] Status encode_request_validated(
    const ProtocolConfig& config,
    FrameCodecContext context,
    mcprotocol::serial::Span<const std::uint8_t> request_data,
    mcprotocol::serial::Span<std::uint8_t> out_frame,
    std::size_t& out_size) noexcept;

[[nodiscard]] DecodeResult decode_response_validated(
    const ProtocolConfig& config,
    FrameCodecContext context,
    mcprotocol::serial::Span<const std::uint8_t> bytes) noexcept;

}  // namespace mcprotocol::serial::detail
