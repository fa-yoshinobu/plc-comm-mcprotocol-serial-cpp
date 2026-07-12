#ifndef MCPROTOCOL_SERIAL_PROTOCOL_PREDICATES_HPP_
#define MCPROTOCOL_SERIAL_PROTOCOL_PREDICATES_HPP_

#include "mcprotocol/serial/types.hpp"

namespace mcprotocol::serial {

constexpr std::uint32_t kMaxWrapSafeTimeoutMs = 0x7FFFFFFFU;

[[nodiscard]] constexpr bool is_wrap_safe_timeout_ms(std::uint32_t timeout_ms) noexcept {
  return timeout_ms > 0U && timeout_ms <= kMaxWrapSafeTimeoutMs;
}

[[nodiscard]] constexpr bool deadline_reached(
    std::uint32_t now_ms,
    std::uint32_t deadline_ms) noexcept {
  return static_cast<std::int32_t>(now_ms - deadline_ms) >= 0;
}

[[nodiscard]] constexpr bool is_iq_r_series(const ProtocolConfig& config) noexcept {
  const PlcSeries series = plc_series_from_profile(config.plc_profile());
  return series == PlcSeries::IQ_R;
}

[[nodiscard]] constexpr bool is_ascii_mode(const ProtocolConfig& config) noexcept {
#if MCPROTOCOL_SERIAL_ENABLE_ASCII_MODE
  return config.code_mode() == CodeMode::Ascii;
#else
  (void)config;
  return false;
#endif
}

[[nodiscard]] constexpr bool is_binary_mode(const ProtocolConfig& config) noexcept {
#if MCPROTOCOL_SERIAL_ENABLE_BINARY_MODE
  return config.code_mode() == CodeMode::Binary;
#else
  (void)config;
  return false;
#endif
}

[[nodiscard]] constexpr bool is_sum_check_enabled(const ProtocolConfig& config) noexcept {
  return config.sum_check_mode() == SumCheckMode::Enabled;
}

}  // namespace mcprotocol::serial

#endif  // MCPROTOCOL_SERIAL_PROTOCOL_PREDICATES_HPP_
