#ifndef MCPROTOCOL_SERIAL_PROTOCOL_PREDICATES_HPP_
#define MCPROTOCOL_SERIAL_PROTOCOL_PREDICATES_HPP_

#include "mcprotocol/serial/types.hpp"

namespace mcprotocol::serial {

[[nodiscard]] constexpr bool is_iq_r_series(const ProtocolConfig& config) noexcept {
  const PlcSeries series = plc_series_from_profile(config.plc_profile);
  return series == PlcSeries::IQ_R;
}

[[nodiscard]] constexpr bool is_ascii_mode(const ProtocolConfig& config) noexcept {
#if MCPROTOCOL_SERIAL_ENABLE_ASCII_MODE
  return config.code_mode == CodeMode::Ascii;
#else
  (void)config;
  return false;
#endif
}

[[nodiscard]] constexpr bool is_binary_mode(const ProtocolConfig& config) noexcept {
#if MCPROTOCOL_SERIAL_ENABLE_BINARY_MODE
  return config.code_mode == CodeMode::Binary;
#else
  (void)config;
  return false;
#endif
}

}  // namespace mcprotocol::serial

#endif  // MCPROTOCOL_SERIAL_PROTOCOL_PREDICATES_HPP_
