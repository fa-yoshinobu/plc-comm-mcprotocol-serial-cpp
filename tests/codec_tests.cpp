#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "test_assert.hpp"

#if defined(__unix__) || defined(__APPLE__)
#include <fcntl.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#endif

#include "mcprotocol/serial/client.hpp"
#include "mcprotocol/serial/high_level.hpp"
#include "mcprotocol/serial/host_sync.hpp"
#include "mcprotocol/serial/link_direct.hpp"
#include "mcprotocol/serial/posix_serial.hpp"
#include "mcprotocol/serial/qualified_buffer.hpp"
#include "mcprotocol/serial/span.hpp"
#include "mcprotocol/serial/string_view_compat.hpp"
#include "mcprotocol/serial/detail/fixed_item_array.hpp"
#include "mcprotocol/serial/detail/long_state_aggregate.hpp"
#include "mcprotocol/serial/detail/yield_first_wait.hpp"
#if defined(_WIN32)
#include "mcprotocol/serial/detail/win32_serial_settings.hpp"
#endif
#if defined(__unix__) || defined(__APPLE__)
#include "mcprotocol/serial/detail/posix_serial_settings.hpp"
#endif

namespace {

template <typename T, typename = void>
constexpr bool HasAsciiBlockNumberMember = false;
template <typename T>
constexpr bool HasAsciiBlockNumberMember<
    T,
    std::void_t<decltype(std::declval<T>().ascii_block_number)>> = true;

template <typename T, typename = void>
constexpr bool HasStationNumberMember = false;
template <typename T>
constexpr bool HasStationNumberMember<
    T,
    std::void_t<decltype(std::declval<T>().station_no)>> = true;

template <typename T, typename = void>
constexpr bool HasSelfStationNumberMethod = false;
template <typename T>
constexpr bool HasSelfStationNumberMethod<
    T,
    std::void_t<decltype(std::declval<T>().self_station_no())>> = true;

template <typename T, typename = void>
constexpr bool HasDoubleWordMember = false;
template <typename T>
constexpr bool HasDoubleWordMember<
    T,
    std::void_t<decltype(std::declval<T>().double_word)>> = true;

template <typename T, typename = void>
constexpr bool CanRemoteRunWithoutPolicies = false;
template <typename T>
constexpr bool CanRemoteRunWithoutPolicies<
    T,
    std::void_t<decltype(std::declval<T&>().remote_run())>> = true;

template <typename T, typename = void>
constexpr bool CanRemoteRunWithOnlyConflictPolicy = false;
template <typename T>
constexpr bool CanRemoteRunWithOnlyConflictPolicy<
    T,
    std::void_t<decltype(std::declval<T&>().remote_run(
        mcprotocol::serial::RemoteOperationMode::DoNotExecuteForcibly))>> = true;

template <typename T, typename = void>
constexpr bool CanRemoteRunWithBothPolicies = false;
template <typename T>
constexpr bool CanRemoteRunWithBothPolicies<
    T,
    std::void_t<decltype(std::declval<T&>().remote_run(
        mcprotocol::serial::RemoteOperationMode::DoNotExecuteForcibly,
        mcprotocol::serial::RemoteRunClearMode::DoNotClear))>> = true;

template <typename T, typename = void>
constexpr bool CanRemotePauseWithoutPolicy = false;
template <typename T>
constexpr bool CanRemotePauseWithoutPolicy<
    T,
    std::void_t<decltype(std::declval<T&>().remote_pause())>> = true;

template <typename T, typename = void>
constexpr bool CanRemotePauseWithPolicy = false;
template <typename T>
constexpr bool CanRemotePauseWithPolicy<
    T,
    std::void_t<decltype(std::declval<T&>().remote_pause(
        mcprotocol::serial::RemoteOperationMode::DoNotExecuteForcibly))>> = true;

template <typename T, typename = void>
constexpr bool CanNotifyTxCompleteWithoutStatus = false;
template <typename T>
constexpr bool CanNotifyTxCompleteWithoutStatus<
    T,
    std::void_t<decltype(std::declval<T&>().notify_tx_complete(0U))>> = true;

using mcprotocol::serial::AsciiFormat;
using mcprotocol::serial::BatchReadBitsRequest;
using mcprotocol::serial::BatchReadWordsRequest;
using mcprotocol::serial::BatchWriteBitsRequest;
using mcprotocol::serial::BatchWriteWordsRequest;
using mcprotocol::serial::BitValue;
using mcprotocol::serial::CodeMode;
using mcprotocol::serial::C1MultidropRoute;
using mcprotocol::serial::C2MnMultidropRoute;
using mcprotocol::serial::C2StandardMultidropRoute;
using mcprotocol::serial::C3MnMultidropRoute;
using mcprotocol::serial::C3StandardMultidropRoute;
using mcprotocol::serial::C4MnMultidropRoute;
using mcprotocol::serial::C4StandardMultidropRoute;
using mcprotocol::serial::C4DestinationModule;
using mcprotocol::serial::C34PcTarget;
using mcprotocol::serial::CompletionHandler;
using mcprotocol::serial::CpuModelInfo;
using mcprotocol::serial::DeviceAddress;
using mcprotocol::serial::DecodeStatus;
using mcprotocol::serial::ExtendedFileRegisterAddress;
using mcprotocol::serial::ExtendedFileRegisterBatchReadWordsRequest;
using mcprotocol::serial::ExtendedFileRegisterBatchWriteWordsRequest;
using mcprotocol::serial::ExtendedFileRegisterDirectBatchReadWordsRequest;
using mcprotocol::serial::ExtendedFileRegisterDirectBatchWriteWordsRequest;
using mcprotocol::serial::ExtendedFileRegisterMonitorRegistration;
using mcprotocol::serial::ExtendedFileRegisterRandomWriteWordItem;
using mcprotocol::serial::FrameCodec;
using mcprotocol::serial::FrameCodecContext;
using mcprotocol::serial::FrameKind;
using mcprotocol::serial::GlobalSignalControlRequest;
using mcprotocol::serial::GlobalSignalTarget;
using mcprotocol::serial::HardwareFlowControl;
using mcprotocol::serial::HostBufferReadRequest;
using mcprotocol::serial::HostBufferWriteRequest;
using mcprotocol::serial::HostStationRoute;
using mcprotocol::serial::LinkDirectDevice;
using mcprotocol::serial::LinkDirectMonitorRegistration;
using mcprotocol::serial::LinkDirectMultiBlockReadBlock;
using mcprotocol::serial::LinkDirectMultiBlockReadRequest;
using mcprotocol::serial::LinkDirectMultiBlockWriteBlock;
using mcprotocol::serial::LinkDirectMultiBlockWriteRequest;
using mcprotocol::serial::LinkDirectRandomReadWordItem;
using mcprotocol::serial::LinkDirectRandomWriteBitItem;
using mcprotocol::serial::LinkDirectRandomWriteWordItem;
using mcprotocol::serial::MelsecSerialClient;
using mcprotocol::serial::MonitorRegistration;
using mcprotocol::serial::ModuleBufferReadRequest;
using mcprotocol::serial::ModuleBufferWriteRequest;
using mcprotocol::serial::MultiBlockReadBlock;
using mcprotocol::serial::MultiBlockReadBlockResult;
using mcprotocol::serial::MultiBlockReadRequest;
using mcprotocol::serial::MultiBlockWriteBlock;
using mcprotocol::serial::MultiBlockWriteRequest;
using mcprotocol::serial::PlcProfile;
using mcprotocol::serial::PlcSeries;
using mcprotocol::serial::ProtocolConfig;
using mcprotocol::serial::HostSerialConfig;
using mcprotocol::serial::HostSyncClient;
using mcprotocol::serial::QualifiedBufferDeviceKind;
using mcprotocol::serial::QualifiedBufferWordDevice;
using mcprotocol::serial::RandomReadDWordItem;
using mcprotocol::serial::RandomReadWordItem;
using mcprotocol::serial::RandomReadRequest;
using mcprotocol::serial::RandomWriteBitItem;
using mcprotocol::serial::RandomWriteDWordItem;
using mcprotocol::serial::RandomWriteWordItem;
using mcprotocol::serial::RemoteOperationMode;
using mcprotocol::serial::RemoteRunClearMode;
using mcprotocol::serial::RouteConfig;
using mcprotocol::serial::Rs485Hooks;
using mcprotocol::serial::SelfStationNo;
using mcprotocol::serial::SerialModuleChannel;
using mcprotocol::serial::SerialModuleCommunicationSpeed;
using mcprotocol::serial::SerialModuleModeNo;
using mcprotocol::serial::SerialModuleModeSwitchRequest;
using mcprotocol::serial::SerialParity;
using mcprotocol::serial::parse_plc_profile;
using mcprotocol::serial::is_plc_profile_specified;
using mcprotocol::serial::plc_profile_display_name;
using mcprotocol::serial::plc_profile_name;
using mcprotocol::serial::plc_series_from_profile;
using mcprotocol::serial::sparse_native_mask_word;
using mcprotocol::serial::sparse_native_requested_bit_value;
using mcprotocol::serial::Status;
using mcprotocol::serial::StatusCode;
using mcprotocol::serial::SumCheckMode;
using mcprotocol::serial::UserFrameRegistrationDeleteRequest;
using mcprotocol::serial::UserFrameRegistrationData;
using mcprotocol::serial::UserFrameRegistrationReadRequest;
using mcprotocol::serial::UserFrameRegistrationWriteRequest;
using mcprotocol::serial::decode_qualified_buffer_word_values;
using mcprotocol::serial::highlevel::make_batch_read_words_request;
using mcprotocol::serial::highlevel::make_batch_write_bits_request;
using mcprotocol::serial::highlevel::make_c4_binary_protocol;
using mcprotocol::serial::highlevel::make_c4_ascii_format4_protocol;
using mcprotocol::serial::highlevel::make_monitor_registration;
using mcprotocol::serial::highlevel::make_random_read_dword_item;
using mcprotocol::serial::highlevel::make_random_read_word_item;
using mcprotocol::serial::highlevel::make_random_read_request;
using mcprotocol::serial::highlevel::make_random_write_bit_item;
using mcprotocol::serial::highlevel::make_random_write_bit_items;
using mcprotocol::serial::highlevel::make_random_write_dword_item;
using mcprotocol::serial::highlevel::make_random_write_dword_items;
using mcprotocol::serial::highlevel::make_random_write_word_item;
using mcprotocol::serial::highlevel::make_random_write_word_items;
using mcprotocol::serial::highlevel::parse_device_address;
using mcprotocol::serial::highlevel::decode_long_state_bit;
using mcprotocol::serial::highlevel::get_long_state_read_spec;
using mcprotocol::serial::highlevel::LongStateReadKind;
using mcprotocol::serial::highlevel::LongStateReadRoute;
using mcprotocol::serial::highlevel::LongStateReadSpec;
using mcprotocol::serial::highlevel::RandomReadDWordSpec;
using mcprotocol::serial::highlevel::RandomReadWordSpec;
using mcprotocol::serial::highlevel::RandomWriteBitSpec;
using mcprotocol::serial::highlevel::RandomWriteDWordSpec;
using mcprotocol::serial::highlevel::RandomWriteWordSpec;
using mcprotocol::serial::make_qualified_buffer_read_words_request;
using mcprotocol::serial::make_qualified_buffer_write_words_request;
using mcprotocol::serial::parse_link_direct_device;
using mcprotocol::serial::parse_qualified_buffer_word_device;
using mcprotocol::serial::validate_qualified_buffer_helper_route;

[[nodiscard]] bool parse_plc_profile_text(std::string_view text, PlcProfile& profile) {
  return parse_plc_profile(text.data(), text.size(), profile);
}

namespace CommandCodec = mcprotocol::serial::CommandCodec;
namespace module_io = mcprotocol::serial::module_io;

[[nodiscard]] Status start_and_notify_tx_complete(
    MelsecSerialClient& client,
    std::uint32_t now_ms,
    Status transport_status) noexcept {
  // Most codec tests model an instantaneous transport. Starting again is intentionally harmless
  // here when a deadline-specific test already called notify_tx_started with an earlier timestamp.
  (void)client.notify_tx_started(now_ms);
  return client.notify_tx_complete(now_ms, transport_status);
}

[[nodiscard]] constexpr RouteConfig host_station_route() noexcept {
  return RouteConfig {HostStationRoute {}};
}

[[nodiscard]] constexpr C34PcTarget c34_pc_target(std::uint32_t value) noexcept {
  switch (value) {
    case 0x7DU:
      return C34PcTarget::control_system();
    case 0x7EU:
      return C34PcTarget::standby_system();
    case 0xFEU:
      return C34PcTarget::special_fe();
    case 0xFFU:
      return C34PcTarget::connected_station();
    default:
      return C34PcTarget::number(value);
  }
}

[[nodiscard]] constexpr C4DestinationModule c4_destination_module(
    std::uint32_t io_number,
    std::uint32_t station_number) noexcept {
  if (station_number == 0U) {
    switch (io_number) {
      case module_io::OwnStation:
        return C4DestinationModule::own_station();
      case module_io::MultipleCpu1:
        return C4DestinationModule::multiple_cpu(1U);
      case module_io::MultipleCpu2:
        return C4DestinationModule::multiple_cpu(2U);
      case module_io::MultipleCpu3:
        return C4DestinationModule::multiple_cpu(3U);
      case module_io::MultipleCpu4:
        return C4DestinationModule::multiple_cpu(4U);
      case module_io::ControlSystemCpu:
        return C4DestinationModule::redundant_control_system_cpu();
      case module_io::StandbySystemCpu:
        return C4DestinationModule::redundant_standby_system_cpu();
      case module_io::SystemACpu:
        return C4DestinationModule::redundant_system_a_cpu();
      case module_io::SystemBCpu:
        return C4DestinationModule::redundant_system_b_cpu();
      default:
        break;
    }
  }
  return C4DestinationModule::explicit_target(io_number, station_number);
}

[[nodiscard]] constexpr RouteConfig multidrop_route(
    FrameKind frame_kind,
    std::uint8_t station_no,
    std::uint8_t network_no = 0x00U,
    std::uint8_t pc_no = 0xFFU,
    std::uint16_t module_io_no = module_io::OwnStation,
    std::uint8_t module_station_no = 0x00U) noexcept {
  switch (frame_kind) {
    case FrameKind::C1:
      return RouteConfig {C1MultidropRoute {station_no}};
    case FrameKind::C2:
      return RouteConfig {C2StandardMultidropRoute {station_no}};
    case FrameKind::C3:
      return RouteConfig {C3StandardMultidropRoute {
          station_no, network_no, c34_pc_target(pc_no)}};
    case FrameKind::C4:
      return RouteConfig {C4StandardMultidropRoute {
          station_no,
          network_no,
          c34_pc_target(pc_no),
          c4_destination_module(module_io_no, module_station_no)}};
  }
  return RouteConfig {};
}

[[nodiscard]] constexpr RouteConfig mn_multidrop_route(
    FrameKind frame_kind,
    std::uint8_t station_no,
    std::uint8_t network_no,
    std::uint8_t pc_no,
    std::uint16_t module_io_no,
    std::uint8_t module_station_no,
    std::uint32_t self_station_no) noexcept {
  switch (frame_kind) {
    case FrameKind::C2:
      return RouteConfig {C2MnMultidropRoute {
          station_no, SelfStationNo::number(self_station_no)}};
    case FrameKind::C3:
      return RouteConfig {C3MnMultidropRoute {
          station_no,
          network_no,
          c34_pc_target(pc_no),
          SelfStationNo::number(self_station_no)}};
    case FrameKind::C4:
      return RouteConfig {C4MnMultidropRoute {
          station_no,
          network_no,
          c34_pc_target(pc_no),
          c4_destination_module(module_io_no, module_station_no),
          SelfStationNo::number(self_station_no)}};
    case FrameKind::C1:
      return RouteConfig {};
  }
  return RouteConfig {};
}

void test_module_io_constants() {
  static_assert(std::is_empty_v<HostStationRoute>);
  static_assert(!HasStationNumberMember<HostStationRoute>);
  static_assert(std::is_constructible_v<RouteConfig, HostStationRoute>);
  static_assert(std::is_constructible_v<RouteConfig, C1MultidropRoute>);
  static_assert(std::is_constructible_v<RouteConfig, C2StandardMultidropRoute>);
  static_assert(std::is_constructible_v<RouteConfig, C3StandardMultidropRoute>);
  static_assert(std::is_constructible_v<RouteConfig, C4StandardMultidropRoute>);
  static_assert(!std::is_constructible_v<RouteConfig, mcprotocol::serial::RouteKind>);
  assert(module_io::ControlSystemCpu == 0x03D0U);
  assert(module_io::StandbySystemCpu == 0x03D1U);
  assert(module_io::SystemACpu == 0x03D2U);
  assert(module_io::SystemBCpu == 0x03D3U);
  assert(module_io::MultipleCpu1 == 0x03E0U);
  assert(module_io::MultipleCpu2 == 0x03E1U);
  assert(module_io::MultipleCpu3 == 0x03E2U);
  assert(module_io::MultipleCpu4 == 0x03E3U);
  assert(module_io::RemoteHead1 == module_io::MultipleCpu1);
  assert(module_io::RemoteHead2 == module_io::MultipleCpu2);
  assert(module_io::ControlSystemRemoteHead == module_io::ControlSystemCpu);
  assert(module_io::StandbySystemRemoteHead == module_io::StandbySystemCpu);
  assert(module_io::OwnStation == 0x03FFU);
  assert(!RouteConfig {}.is_specified());
  const RouteConfig host_route = host_station_route();
  assert(host_route.is_host_station());
  assert(host_route.station_no() == 0x00U);
  assert(host_route.network_no() == 0x00U);
  assert(host_route.pc_no() == 0xFFU);
  assert(host_route.request_destination_module_io_no() == module_io::OwnStation);
  assert(host_route.request_destination_module_station_no() == 0x00U);
  assert(!host_route.is_mn_multidrop());
  assert(host_route.self_station_no() == 0x00U);
}

ProtocolConfig make_binary_c4_config() {
  return ProtocolConfig::c4_binary(PlcProfile::MelsecQ, SumCheckMode::Enabled, host_station_route());
}

ProtocolConfig make_binary_c4_iqr_config() {
  return make_binary_c4_config().with_plc_profile(PlcProfile::MelsecIqR);
}

ProtocolConfig make_binary_c4_iql_config() {
  return make_binary_c4_config().with_plc_profile(PlcProfile::MelsecIqL);
}

ProtocolConfig make_binary_c4_iqf_config() {
  return make_binary_c4_config().with_plc_profile(PlcProfile::MelsecIqF);
}

ProtocolConfig make_binary_c4_l_config() {
  return make_binary_c4_config().with_plc_profile(PlcProfile::MelsecL);
}

ProtocolConfig make_ascii_c3_format3_config() {
  return ProtocolConfig::ascii(
      mcprotocol::serial::AsciiFrameKind::C3,
      AsciiFormat::Format3,
      PlcProfile::MelsecQ,
      SumCheckMode::Enabled,
      host_station_route());
}

ProtocolConfig make_ascii_c2_format3_config() {
  return ProtocolConfig::ascii(
      mcprotocol::serial::AsciiFrameKind::C2,
      AsciiFormat::Format3,
      PlcProfile::MelsecQ,
      SumCheckMode::Enabled,
      host_station_route());
}

ProtocolConfig make_ascii_c4_format2_config() {
  return ProtocolConfig::ascii(
      mcprotocol::serial::AsciiFrameKind::C4,
      AsciiFormat::Format2,
      PlcProfile::MelsecQ,
      SumCheckMode::Enabled,
      host_station_route());
}

ProtocolConfig make_ascii_c2_format2_config() {
  return ProtocolConfig::ascii(
      mcprotocol::serial::AsciiFrameKind::C2,
      AsciiFormat::Format2,
      PlcProfile::MelsecQ,
      SumCheckMode::Enabled,
      host_station_route());
}

ProtocolConfig make_ascii_c4_format4_config() {
  return ProtocolConfig::ascii(
      mcprotocol::serial::AsciiFrameKind::C4,
      AsciiFormat::Format4,
      PlcProfile::MelsecQ,
      SumCheckMode::Disabled,
      multidrop_route(FrameKind::C4, 0x01U));
}

ProtocolConfig make_ascii_c2_format4_config() {
  return ProtocolConfig::ascii(
      mcprotocol::serial::AsciiFrameKind::C2,
      AsciiFormat::Format4,
      PlcProfile::MelsecQ,
      SumCheckMode::Disabled,
      RouteConfig {C2StandardMultidropRoute {0x01U}});
}

ProtocolConfig make_ascii_c1_format4_qna_config() {
  return ProtocolConfig::ascii(
      mcprotocol::serial::AsciiFrameKind::C1,
      AsciiFormat::Format4,
      PlcProfile::MelsecQnA,
      SumCheckMode::Disabled,
      host_station_route());
}

ProtocolConfig make_ascii_c1_format4_a_config() {
  return make_ascii_c1_format4_qna_config().with_plc_profile(PlcProfile::MelsecA);
}

ProtocolConfig make_ascii_c4_format4_iqr_config() {
  return make_ascii_c4_format4_config().with_plc_profile(PlcProfile::MelsecIqR);
}

ProtocolConfig test_config_with_sum_check(
    const ProtocolConfig& config,
    SumCheckMode sum_check_mode) {
  if (config.code_mode() == CodeMode::Binary) {
    return ProtocolConfig::c4_binary(
        config.plc_profile(),
        sum_check_mode,
        config.route(),
        config.timeout());
  }
  return ProtocolConfig::ascii(
      static_cast<mcprotocol::serial::AsciiFrameKind>(config.frame_kind()),
      config.ascii_format(),
      config.plc_profile(),
      sum_check_mode,
      config.route(),
      config.timeout());
}

void test_format5_batch_read_request_matches_manual() {
  const auto config = make_binary_c4_config();
  const BatchReadWordsRequest request({mcprotocol::serial::DeviceCode::M, 100}, 2);

  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_data_size = 0;
  Status status = CommandCodec::encode_batch_read_words(config, request, request_data, request_data_size);
  assert(status.ok());

  std::array<std::uint8_t, 128> frame {};
  std::size_t frame_size = 0;
  status = FrameCodec::encode_request(
      config,
      mcprotocol::serial::Span<const std::uint8_t>(request_data.data(), request_data_size),
      frame,
      frame_size);
  assert(status.ok());

  const std::array<std::uint8_t, 18> expected {
      0x10, 0x02, 0x12, 0x00, 0xF8, 0x00, 0x00, 0xFF, 0xFF, 0x03, 0x00, 0x00,
      0x01, 0x04, 0x00, 0x00, 0x64, 0x00,
  };

  assert(frame_size > expected.size());
  assert(std::equal(expected.begin(), expected.end(), frame.begin()));
  assert(frame[18] == 0x00);
  assert(frame[19] == 0x90);
  assert(frame[20] == 0x02);
  assert(frame[21] == 0x00);
  assert(frame[22] == 0x10);
  assert(frame[23] == 0x03);
  assert(frame[24] == '0');
  assert(frame[25] == '6');
}

void test_iq_l_uses_q_l_binary_request_shape() {
  const auto config = make_binary_c4_iql_config();
  const BatchReadWordsRequest request({mcprotocol::serial::DeviceCode::M, 100}, 2);

  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_data_size = 0;
  const Status status = CommandCodec::encode_batch_read_words(config, request, request_data, request_data_size);
  assert(status.ok());

  const std::array<std::uint8_t, 10> expected {
      0x01, 0x04, 0x00, 0x00, 0x64, 0x00, 0x00, 0x90, 0x02, 0x00};
  assert(request_data_size == expected.size());
  assert(std::equal(expected.begin(), expected.end(), request_data.begin()));
}

void test_decode_binary_cpu_model_response() {
  const auto config = make_binary_c4_config();
  const std::array<std::uint8_t, 18> response_data {
      'Q', '0', '2', 'U', 'C', 'P', 'U', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
      0x63, 0x02,
  };

  std::array<std::uint8_t, 128> frame {};
  std::size_t frame_size = 0;
  Status status = FrameCodec::encode_success_response(config, response_data, frame, frame_size);
  assert(status.ok());

  const auto decode = FrameCodec::decode_response(
      config,
      mcprotocol::serial::Span<const std::uint8_t>(frame.data(), frame_size));
  assert(decode.status == DecodeStatus::Complete);
  assert(decode.frame.response_size == response_data.size());

  CpuModelInfo info;
  status = CommandCodec::parse_read_cpu_model_response(
      config,
      mcprotocol::serial::Span<const std::uint8_t>(decode.frame.response_data.data(), decode.frame.response_size),
      info);
  assert(status.ok());
  assert(std::string_view(info.model_name.data()) == "Q02UCPU");
  assert(info.model_code == 0x0263);
}

void test_encode_remote_reset_binary_request() {
  const auto config = make_binary_c4_config();
  std::array<std::uint8_t, 16> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_remote_reset(config, request_data, request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 6> expected {0x06, 0x10, 0x00, 0x00, 0x01, 0x00};
  assert(request_size == expected.size());
  assert(std::equal(expected.begin(), expected.end(), request_data.begin()));
}

void test_encode_remote_run_binary_request() {
  static_assert(!CanRemoteRunWithoutPolicies<HostSyncClient>);
  static_assert(!CanRemoteRunWithOnlyConflictPolicy<HostSyncClient>);
  static_assert(CanRemoteRunWithBothPolicies<HostSyncClient>);

  const auto config = make_binary_c4_config();
  std::array<std::uint8_t, 16> request_data {};
  const std::array<RemoteOperationMode, 2> modes {
      RemoteOperationMode::DoNotExecuteForcibly,
      RemoteOperationMode::ExecuteForcibly,
  };
  const std::array<RemoteRunClearMode, 3> clear_modes {
      RemoteRunClearMode::DoNotClear,
      RemoteRunClearMode::ClearOutsideLatchRange,
      RemoteRunClearMode::AllClear,
  };

  for (const RemoteOperationMode mode : modes) {
    for (const RemoteRunClearMode clear_mode : clear_modes) {
      std::size_t request_size = 99U;
      const Status status = CommandCodec::encode_remote_run(
          config, mode, clear_mode, request_data, request_size);
      assert(status.ok());

      const std::array<std::uint8_t, 8> expected {
          0x01,
          0x10,
          0x00,
          0x00,
          static_cast<std::uint8_t>(static_cast<std::uint16_t>(mode) & 0x00FFU),
          0x00,
          static_cast<std::uint8_t>(clear_mode),
          0x00,
      };
      assert(request_size == expected.size());
      assert(std::equal(expected.begin(), expected.end(), request_data.begin()));
    }
  }

  const auto ascii_config = ProtocolConfig::ascii(
      mcprotocol::serial::AsciiFrameKind::C4,
      AsciiFormat::Format3,
      config.plc_profile(),
      config.sum_check_mode(),
      config.route());
  for (const RemoteOperationMode mode : modes) {
    for (const RemoteRunClearMode clear_mode : clear_modes) {
      std::size_t request_size = 0U;
      const Status status = CommandCodec::encode_remote_run(
          ascii_config, mode, clear_mode, request_data, request_size);
      assert(status.ok());
      const std::string expected =
          std::string("10010000000") +
          (mode == RemoteOperationMode::DoNotExecuteForcibly ? "1" : "3") +
          "0" +
          static_cast<char>('0' + static_cast<std::uint8_t>(clear_mode)) +
          "00";
      assert(request_size == expected.size());
      assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
    }
  }

  std::size_t request_size = 99U;
  Status status = CommandCodec::encode_remote_run(
      config,
      static_cast<RemoteOperationMode>(0xFFFFU),
      RemoteRunClearMode::DoNotClear,
      request_data,
      request_size);
  assert(!status.ok());
  assert(status.code == StatusCode::InvalidArgument);
  assert(request_size == 0U);

  request_size = 99U;
  status = CommandCodec::encode_remote_run(
      config,
      RemoteOperationMode::DoNotExecuteForcibly,
      static_cast<RemoteRunClearMode>(0xFFU),
      request_data,
      request_size);
  assert(!status.ok());
  assert(status.code == StatusCode::InvalidArgument);
  assert(request_size == 0U);

}

void test_encode_remote_stop_binary_request() {
  const auto config = make_binary_c4_config();
  std::array<std::uint8_t, 16> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_remote_stop(config, request_data, request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 6> expected {0x02, 0x10, 0x00, 0x00, 0x01, 0x00};
  assert(request_size == expected.size());
  assert(std::equal(expected.begin(), expected.end(), request_data.begin()));
}

void test_encode_remote_pause_binary_request() {
  static_assert(!CanRemotePauseWithoutPolicy<HostSyncClient>);
  static_assert(CanRemotePauseWithPolicy<HostSyncClient>);

  const auto config = make_binary_c4_config();
  std::array<std::uint8_t, 16> request_data {};
  const std::array<RemoteOperationMode, 2> modes {
      RemoteOperationMode::DoNotExecuteForcibly,
      RemoteOperationMode::ExecuteForcibly,
  };
  for (const RemoteOperationMode mode : modes) {
    std::size_t request_size = 99U;
    const Status status = CommandCodec::encode_remote_pause(
        config, mode, request_data, request_size);
    assert(status.ok());
    const std::array<std::uint8_t, 6> expected {
        0x03,
        0x10,
        0x00,
        0x00,
        static_cast<std::uint8_t>(static_cast<std::uint16_t>(mode) & 0x00FFU),
        0x00,
    };
    assert(request_size == expected.size());
    assert(std::equal(expected.begin(), expected.end(), request_data.begin()));
  }

  const auto ascii_config = ProtocolConfig::ascii(
      mcprotocol::serial::AsciiFrameKind::C4,
      AsciiFormat::Format3,
      config.plc_profile(),
      config.sum_check_mode(),
      config.route());
  for (const RemoteOperationMode mode : modes) {
    std::size_t request_size = 0U;
    const Status status = CommandCodec::encode_remote_pause(
        ascii_config, mode, request_data, request_size);
    assert(status.ok());
    const std::string expected =
        std::string("10030000000") +
        (mode == RemoteOperationMode::DoNotExecuteForcibly ? "1" : "3");
    assert(request_size == expected.size());
    assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
  }

  std::size_t request_size = 99U;
  Status status = CommandCodec::encode_remote_pause(
      config,
      static_cast<RemoteOperationMode>(0xFFFFU),
      request_data,
      request_size);
  assert(!status.ok());
  assert(status.code == StatusCode::InvalidArgument);
  assert(request_size == 0U);

}

void test_encode_remote_latch_clear_binary_request() {
  const auto config = make_binary_c4_config();
  std::array<std::uint8_t, 16> request_data {};
  std::size_t request_size = 0;
  const Status status =
      CommandCodec::encode_remote_latch_clear(config, request_data, request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 6> expected {0x05, 0x10, 0x00, 0x00, 0x01, 0x00};
  assert(request_size == expected.size());
  assert(std::equal(expected.begin(), expected.end(), request_data.begin()));
}

void test_encode_unlock_remote_password_binary_q_l_request() {
  const auto config = make_binary_c4_config();
  std::array<std::uint8_t, 32> request_data {};
  std::size_t request_size = 0;
  const Status status =
      CommandCodec::encode_unlock_remote_password(config, "1234", request_data, request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 10> expected {
      0x30, 0x16, 0x00, 0x00, 0x04, 0x00, 0x31, 0x32, 0x33, 0x34};
  assert(request_size == expected.size());
  assert(std::equal(expected.begin(), expected.end(), request_data.begin()));
}

void test_encode_lock_remote_password_binary_q_l_request() {
  const auto config = make_binary_c4_config();
  std::array<std::uint8_t, 32> request_data {};
  std::size_t request_size = 0;
  const Status status =
      CommandCodec::encode_lock_remote_password(config, "1234", request_data, request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 10> expected {
      0x31, 0x16, 0x00, 0x00, 0x04, 0x00, 0x31, 0x32, 0x33, 0x34};
  assert(request_size == expected.size());
  assert(std::equal(expected.begin(), expected.end(), request_data.begin()));
}

void test_encode_unlock_remote_password_rejects_invalid_lengths() {
  {
    const auto config = make_binary_c4_config();
    std::array<std::uint8_t, 32> request_data {};
    std::size_t request_size = 0;
    const Status status =
        CommandCodec::encode_unlock_remote_password(config, "12345", request_data, request_size);
    assert(!status.ok());
    assert(status.code == StatusCode::InvalidArgument);
  }
  {
    const auto config = make_binary_c4_iqr_config();
    std::array<std::uint8_t, 32> request_data {};
    std::size_t request_size = 0;
    const Status status =
        CommandCodec::encode_unlock_remote_password(config, "1234", request_data, request_size);
    assert(!status.ok());
    assert(status.code == StatusCode::InvalidArgument);
  }
}

void test_encode_clear_error_information_binary_q_l_request() {
  const auto config = make_binary_c4_config();
  std::array<std::uint8_t, 16> request_data {};
  std::size_t request_size = 0;
  const Status status =
      CommandCodec::encode_clear_error_information(config, request_data, request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 8> expected {0x17, 0x16, 0x0F, 0x00, 0xFF, 0x00, 0xFF, 0x00};
  assert(request_size == expected.size());
  assert(std::equal(expected.begin(), expected.end(), request_data.begin()));
}

void test_encode_clear_error_information_binary_iqr_request() {
  const auto config = make_binary_c4_iqr_config();
  std::array<std::uint8_t, 16> request_data {};
  std::size_t request_size = 0;
  const Status status =
      CommandCodec::encode_clear_error_information(config, request_data, request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 8> expected {0x17, 0x16, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00};
  assert(request_size == expected.size());
  assert(std::equal(expected.begin(), expected.end(), request_data.begin()));
}

void test_encode_initialize_transmission_sequence_binary_request() {
  const auto config = make_binary_c4_config();
  std::array<std::uint8_t, 16> request_data {};
  std::size_t request_size = 0;
  const Status status =
      CommandCodec::encode_initialize_transmission_sequence(config, request_data, request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 4> expected {0x15, 0x16, 0x00, 0x00};
  assert(request_size == expected.size());
  assert(std::equal(expected.begin(), expected.end(), request_data.begin()));
}

void test_encode_initialize_transmission_sequence_rejects_ascii() {
  const auto config = make_ascii_c3_format3_config();
  std::array<std::uint8_t, 16> request_data {};
  std::size_t request_size = 0;
  const Status status =
      CommandCodec::encode_initialize_transmission_sequence(config, request_data, request_size);
  assert(!status.ok());
  assert(status.code == StatusCode::UnsupportedConfiguration);
}

void test_encode_control_global_signal_binary_request() {
  const auto config = make_binary_c4_config();
  std::array<std::uint8_t, 16> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_control_global_signal(
      config,
      GlobalSignalControlRequest(GlobalSignalTarget::X1A, (true ? true : false)),
      request_data,
      request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 6> expected {0x18, 0x16, 0x01, 0x00, 0x01, 0x00};
  assert(request_size == expected.size());
  assert(std::equal(expected.begin(), expected.end(), request_data.begin()));
}

void test_encode_control_global_signal_uses_route_station_not_specification_word() {
  auto config = make_binary_c4_config();
  config = config.with_route(multidrop_route(FrameKind::C4, 0x1FU));

  std::array<std::uint8_t, 16> request_data {};
  std::size_t request_size = 0;
  Status status = CommandCodec::encode_control_global_signal(
      config,
      GlobalSignalControlRequest(GlobalSignalTarget::X1B, (false ? true : false)),
      request_data,
      request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 6> expected_request {0x18, 0x16, 0x00, 0x00, 0x02, 0x00};
  assert(request_size == expected_request.size());
  assert(std::equal(expected_request.begin(), expected_request.end(), request_data.begin()));

  std::array<std::uint8_t, 32> frame {};
  std::size_t frame_size = 0;
  status = FrameCodec::encode_request(
      config,
      mcprotocol::serial::Span<const std::uint8_t>(request_data.data(), request_size),
      frame,
      frame_size);
  assert(status.ok());
  assert(frame_size >= 6U);
  assert(frame[5] == 0x1FU);
}

void test_control_requests_reject_unknown_or_empty_changes_without_tx() {
  const auto config = make_binary_c4_config();
  MelsecSerialClient client;
  Status status = client.configure(config);
  assert(status.ok());

  status = client.async_control_global_signal(
      0U,
      GlobalSignalControlRequest(
          static_cast<GlobalSignalTarget>(0xFFU), false),
      nullptr,
      nullptr);
  assert(status.code == StatusCode::InvalidArgument);
  assert(!client.busy());
  assert(client.pending_tx_frame().empty());

  status = client.async_switch_serial_module_mode(
      0U,
      SerialModuleModeSwitchRequest(
          SerialModuleChannel::Ch1,
          false,
          false,
          false,
          SerialModuleModeNo::McProtocolFormat1,
          0U,
          SerialModuleCommunicationSpeed::Bps300),
      nullptr,
      nullptr);
  assert(status.code == StatusCode::InvalidArgument);
  assert(!client.busy());
  assert(client.pending_tx_frame().empty());
}

void test_empty_request_containers_are_rejected_without_tx() {
  const auto config = make_binary_c4_iqr_config();
  MelsecSerialClient client;
  Status status = client.configure(config);
  assert(status.ok());
  const auto assert_idle = [&client]() {
    assert(!client.busy());
    assert(client.pending_tx_frame().empty());
  };

  status = client.async_batch_read_words(
      0U,
      BatchReadWordsRequest(DeviceAddress {mcprotocol::serial::DeviceCode::D, 0U}, 0U),
      {},
      nullptr,
      nullptr);
  assert(status.code == StatusCode::InvalidArgument);
  assert_idle();

  status = client.async_batch_write_words(
      0U,
      BatchWriteWordsRequest(DeviceAddress {mcprotocol::serial::DeviceCode::D, 0U}, {}),
      nullptr,
      nullptr);
  assert(status.code == StatusCode::InvalidArgument);
  assert_idle();

  status = client.async_random_read(
      0U, RandomReadRequest({}, {}), {}, {}, nullptr, nullptr);
  assert(status.code == StatusCode::InvalidArgument);
  assert_idle();

  status = client.async_random_write_words(0U, {}, {}, nullptr, nullptr);
  assert(status.code == StatusCode::InvalidArgument);
  assert_idle();

  status = client.async_random_write_bits(0U, {}, nullptr, nullptr);
  assert(status.code == StatusCode::InvalidArgument);
  assert_idle();

  status = client.async_multi_block_read(
      0U,
      MultiBlockReadRequest(mcprotocol::serial::Span<const MultiBlockReadBlock> {}),
      {},
      {},
      {},
      nullptr,
      nullptr);
  assert(status.code == StatusCode::InvalidArgument);
  assert_idle();

  status = client.async_multi_block_write(
      0U,
      MultiBlockWriteRequest(mcprotocol::serial::Span<const MultiBlockWriteBlock> {}),
      nullptr,
      nullptr);
  assert(status.code == StatusCode::InvalidArgument);
  assert_idle();

  status = client.async_register_monitor_devices(
      0U, MonitorRegistration({}, {}), nullptr, nullptr);
  assert(status.code == StatusCode::InvalidArgument);
  assert_idle();

  status = client.configure(make_ascii_c1_format4_a_config());
  assert(status.ok());
  status = client.async_register_extended_file_register_monitor(
      0U,
      ExtendedFileRegisterMonitorRegistration(
          mcprotocol::serial::Span<const ExtendedFileRegisterAddress> {}),
      nullptr,
      nullptr);
  assert(status.code == StatusCode::InvalidArgument);
  assert_idle();

  status = client.configure(config);
  assert(status.ok());
  status = client.async_link_direct_register_monitor(
      0U,
      LinkDirectMonitorRegistration(
          mcprotocol::serial::Span<const LinkDirectRandomReadWordItem> {}),
      nullptr,
      nullptr);
  assert(status.code == StatusCode::InvalidArgument);
  assert_idle();
}

void test_encode_switch_serial_module_mode_binary_request_matches_manual() {
  const auto config = make_binary_c4_config();
  std::array<std::uint8_t, 16> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_switch_serial_module_mode(
      config,
      SerialModuleModeSwitchRequest(SerialModuleChannel::Ch1, true, true, true, SerialModuleModeNo::McProtocolFormat1, 0xB0U, SerialModuleCommunicationSpeed::Bps9600),
      request_data,
      request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 9> expected {
      0x12,
      0x16,
      0x00,
      0x00,
      0x01,
      0x07,
      0x01,
      0xB0,
      0x05,
  };
  assert(request_size == expected.size());
  assert(std::equal(expected.begin(), expected.end(), request_data.begin()));
}

void test_encode_switch_serial_module_mode_ascii_request_matches_manual() {
  const auto config = make_ascii_c3_format3_config();
  std::array<std::uint8_t, 32> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_switch_serial_module_mode(
      config,
      SerialModuleModeSwitchRequest(SerialModuleChannel::Ch1, true, true, true, SerialModuleModeNo::McProtocolFormat1, 0xB0U, SerialModuleCommunicationSpeed::Bps9600),
      request_data,
      request_size);
  assert(status.ok());

  constexpr std::string_view expected = "16120000010701B005";
  assert(request_size == expected.size());
  assert(std::equal(expected.begin(), expected.end(), request_data.begin()));
}

void test_encode_switch_serial_module_mode_rejects_invalid_request() {
  const auto config = make_binary_c4_config();
  std::array<std::uint8_t, 16> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_switch_serial_module_mode(
      config,
      SerialModuleModeSwitchRequest(
          SerialModuleChannel::Ch1,
          false,
          false,
          false,
          SerialModuleModeNo::McProtocolFormat1,
          0U,
          SerialModuleCommunicationSpeed::Bps300),
      request_data,
      request_size);
  assert(!status.ok());
  assert(status.code == StatusCode::InvalidArgument);
  assert(request_size == 0U);
}

void test_decode_ascii_loopback_response() {
  const auto config = make_ascii_c3_format3_config();
  const std::array<std::uint8_t, 9> response_data {'0', '0', '0', '5', 'A', 'B', 'C', 'D', 'E'};

  std::array<std::uint8_t, 128> frame {};
  std::size_t frame_size = 0;
  Status status = FrameCodec::encode_success_response(config, response_data, frame, frame_size);
  assert(status.ok());

  const auto decode = FrameCodec::decode_response(
      config,
      mcprotocol::serial::Span<const std::uint8_t>(frame.data(), frame_size));
  assert(decode.status == DecodeStatus::Complete);

  std::array<char, 8> echoed {};
  status = CommandCodec::parse_loopback_response(
      config,
      mcprotocol::serial::Span<const std::uint8_t>(decode.frame.response_data.data(), decode.frame.response_size),
      echoed);
  assert(status.ok());
  assert(std::string_view(echoed.data(), 5) == "ABCDE");
}

void test_encode_ascii_read_user_frame_request_shape() {
  const auto config = make_ascii_c3_format3_config();
  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_read_user_frame(
      config,
      UserFrameRegistrationReadRequest(0x03E8U),
      request_data,
      request_size);
  assert(status.ok());

  constexpr std::string_view expected = "0610000003E8";
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_parse_ascii_read_user_frame_response() {
  const auto config = make_ascii_c3_format3_config();
  const std::array<std::uint8_t, 18> response_data {
      '0', '0', '0', '5', '0', '0', '0', '4', '0', '3', 'F', 'F', 'F', '1', '0', 'D', '0', 'A',
  };

  std::array<std::uint8_t, 128> frame {};
  std::size_t frame_size = 0;
  Status status = FrameCodec::encode_success_response(config, response_data, frame, frame_size);
  assert(status.ok());

  const auto decode = FrameCodec::decode_response(
      config,
      mcprotocol::serial::Span<const std::uint8_t>(frame.data(), frame_size));
  assert(decode.status == DecodeStatus::Complete);

  UserFrameRegistrationData out_data {};
  status = CommandCodec::parse_read_user_frame_response(
      config,
      mcprotocol::serial::Span<const std::uint8_t>(decode.frame.response_data.data(), decode.frame.response_size),
      out_data);
  assert(status.ok());
  assert(out_data.registration_data_bytes == 5U);
  assert(out_data.frame_bytes == 4U);
  const std::array<mcprotocol::serial::Byte, 5> expected {
      mcprotocol::serial::Byte {0x03}, mcprotocol::serial::Byte {0xFF}, mcprotocol::serial::Byte {0xF1}, mcprotocol::serial::Byte {0x0D}, mcprotocol::serial::Byte {0x0A},
  };
  assert(std::memcmp(out_data.registration_data.data(), expected.data(), expected.size()) == 0);
}

void test_parse_binary_read_user_frame_response_accepts_zero_frame_bytes() {
  const auto config = make_binary_c4_iqr_config();
  const std::array<std::uint8_t, 9> response_data {
      0x05, 0x00, 0x00, 0x00, 0x03, 0xFF, 0xF1, 0x0D, 0x0A,
  };

  UserFrameRegistrationData out_data {};
  const Status status = CommandCodec::parse_read_user_frame_response(
      config,
      response_data,
      out_data);
  assert(status.ok());
  assert(out_data.registration_data_bytes == 5U);
  assert(out_data.frame_bytes == 0U);
  const std::array<mcprotocol::serial::Byte, 5> expected {
      mcprotocol::serial::Byte {0x03}, mcprotocol::serial::Byte {0xFF}, mcprotocol::serial::Byte {0xF1}, mcprotocol::serial::Byte {0x0D}, mcprotocol::serial::Byte {0x0A},
  };
  assert(std::memcmp(out_data.registration_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_binary_write_user_frame_request_shape() {
  const auto config = make_binary_c4_config();
  const std::array<mcprotocol::serial::Byte, 5> registration_data {
      mcprotocol::serial::Byte {0x03}, mcprotocol::serial::Byte {0xFF}, mcprotocol::serial::Byte {0xF1}, mcprotocol::serial::Byte {0x0D}, mcprotocol::serial::Byte {0x0A},
  };
  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_write_user_frame(
      config,
      UserFrameRegistrationWriteRequest(0x03E8U, 4U, registration_data),
      request_data,
      request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 15> expected {
      0x10U, 0x16U, 0x00U, 0x00U, 0xE8U, 0x03U, 0x05U, 0x00U, 0x04U, 0x00U,
      0x03U, 0xFFU, 0xF1U, 0x0DU, 0x0AU,
  };
  assert(request_size == expected.size());
  assert(std::equal(expected.begin(), expected.end(), request_data.begin()));
}

void test_encode_binary_delete_user_frame_request_shape() {
  const auto config = make_binary_c4_config();
  std::array<std::uint8_t, 32> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_delete_user_frame(
      config,
      UserFrameRegistrationDeleteRequest(0x03E8U),
      request_data,
      request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 10> expected {
      0x10U, 0x16U, 0x01U, 0x00U, 0xE8U, 0x03U, 0x00U, 0x00U, 0x00U, 0x00U,
  };
  assert(request_size == expected.size());
  assert(std::equal(expected.begin(), expected.end(), request_data.begin()));
}

void test_validate_ascii_c2_config_and_reject_binary() {
  ProtocolConfig config = make_ascii_c2_format3_config();
  Status status = FrameCodec::validate_config(config);
  assert(status.ok());

  config = ProtocolConfig::ascii(
      mcprotocol::serial::AsciiFrameKind::C2,
      AsciiFormat::Format2,
      config.plc_profile(),
      config.sum_check_mode(),
      config.route());
  status = FrameCodec::validate_config(config);
  assert(status.ok());

  const auto binary = ProtocolConfig::c4_binary(
      config.plc_profile(), config.sum_check_mode(), config.route());
  assert(binary.frame_kind() == FrameKind::C4);
}

void test_validate_ascii_c1_config_and_reject_binary() {
  ProtocolConfig config = make_ascii_c1_format4_qna_config();
  Status status = FrameCodec::validate_config(config);
  assert(status.ok());

  config = ProtocolConfig::ascii(
      mcprotocol::serial::AsciiFrameKind::C1,
      AsciiFormat::Format2,
      config.plc_profile(),
      config.sum_check_mode(),
      config.route());
  status = FrameCodec::validate_config(config);
  assert(!status.ok());
  assert(status.code == StatusCode::UnsupportedConfiguration);

  const auto binary = ProtocolConfig::c4_binary(
      config.plc_profile(), config.sum_check_mode(), config.route());
  assert(binary.frame_kind() == FrameKind::C4);
}

void test_validate_c4_routed_access_and_connected_station_only_commands() {
  auto config = make_binary_c4_config();
  config = config.with_route(multidrop_route(FrameKind::C4, 0x02U, 0x01U, 0x7DU, 0x0100U, 0x03U));

  Status status = FrameCodec::validate_config(config);
  assert(status.ok());

  std::array<std::uint8_t, 16> request_data {};
  std::size_t request_size = 0;
  status = CommandCodec::encode_batch_read_words(
      config,
      BatchReadWordsRequest(DeviceAddress {mcprotocol::serial::DeviceCode::D, 100}, 1),
      request_data,
      request_size);
  assert(status.ok());

  status = CommandCodec::encode_read_host_buffer(
      config,
      HostBufferReadRequest(0, 1),
      request_data,
      request_size);
  assert(!status.ok());
  assert(status.code == StatusCode::InvalidArgument);
}

void test_route_types_require_frame_specific_station_and_network() {
  static_assert(!std::is_default_constructible_v<C1MultidropRoute>);
  static_assert(!std::is_default_constructible_v<C2StandardMultidropRoute>);
  static_assert(!std::is_default_constructible_v<C3StandardMultidropRoute>);
  static_assert(!std::is_default_constructible_v<C4StandardMultidropRoute>);
  static_assert(std::is_constructible_v<C1MultidropRoute, std::uint8_t>);
  static_assert(std::is_constructible_v<C2StandardMultidropRoute, std::uint8_t>);
  static_assert(!std::is_constructible_v<C3StandardMultidropRoute, std::uint8_t>);
  static_assert(!std::is_constructible_v<C3StandardMultidropRoute, std::uint8_t, std::uint8_t>);
  static_assert(std::is_constructible_v<
                C3StandardMultidropRoute,
                std::uint8_t,
                std::uint8_t,
                C34PcTarget>);
  static_assert(!std::is_constructible_v<C4StandardMultidropRoute, std::uint8_t>);
  static_assert(!std::is_constructible_v<C4StandardMultidropRoute, std::uint8_t, std::uint8_t>);
  static_assert(std::is_constructible_v<
                C4StandardMultidropRoute,
                std::uint8_t,
                std::uint8_t,
                C34PcTarget,
                C4DestinationModule>);

  ProtocolConfig config = make_binary_c4_config();
  config = config.with_route(RouteConfig {C2StandardMultidropRoute {0x00U}});
  Status status = FrameCodec::validate_config(config);
  assert(!status.ok());
  assert(status.code == StatusCode::InvalidArgument);

  config = config.with_route(RouteConfig {C4StandardMultidropRoute {
      0x00U, 0x00U, C34PcTarget::connected_station(), C4DestinationModule::own_station()}});
  status = FrameCodec::validate_config(config);
  assert(status.ok());

  config = config.with_route(RouteConfig {C4StandardMultidropRoute {
      0x1FU, 0xEFU, C34PcTarget::connected_station(), C4DestinationModule::own_station()}});
  assert(FrameCodec::validate_config(config).ok());
  for (const std::uint32_t invalid_station : {0x20U, 0xFFU, 0x100U}) {
    config = config.with_route(RouteConfig {C4StandardMultidropRoute {
        invalid_station, 0x00U, C34PcTarget::connected_station(), C4DestinationModule::own_station()}});
    status = FrameCodec::validate_config(config);
    assert(!status.ok());
    assert(status.code == StatusCode::InvalidArgument);
  }
  for (const std::uint32_t invalid_network : {0xF0U, 0xFEU, 0x100U}) {
    config = config.with_route(RouteConfig {C4StandardMultidropRoute {
        0x00U, invalid_network, C34PcTarget::connected_station(), C4DestinationModule::own_station()}});
    status = FrameCodec::validate_config(config);
    assert(!status.ok());
    assert(status.code == StatusCode::InvalidArgument);
  }
}

void test_self_station_topology_is_typed_required_and_strict() {
  static_assert(!std::is_default_constructible_v<SelfStationNo>);
  static_assert(!std::is_default_constructible_v<C2MnMultidropRoute>);
  static_assert(!std::is_default_constructible_v<C3MnMultidropRoute>);
  static_assert(!std::is_default_constructible_v<C4MnMultidropRoute>);
  static_assert(!std::is_constructible_v<C2MnMultidropRoute, std::uint32_t>);
  static_assert(std::is_constructible_v<
                C2MnMultidropRoute,
                std::uint32_t,
                SelfStationNo>);
  static_assert(!HasSelfStationNumberMethod<C1MultidropRoute>);
  static_assert(!HasSelfStationNumberMethod<C2StandardMultidropRoute>);
  static_assert(HasSelfStationNumberMethod<C2MnMultidropRoute>);

  const RouteConfig standard {C2StandardMultidropRoute {0x01U}};
  assert(!standard.is_mn_multidrop());
  assert(standard.self_station_no() == 0U);
  assert(standard.self_station_valid());

  const RouteConfig explicit_zero {C2MnMultidropRoute {
      0x01U, SelfStationNo::number(0U)}};
  assert(explicit_zero.is_mn_multidrop());
  assert(explicit_zero.self_station_no() == 0U);
  assert(explicit_zero.self_station_valid());

  const RouteConfig upper_bound {C2MnMultidropRoute {
      0x01U, SelfStationNo::number(0x1FU)}};
  assert(upper_bound.is_mn_multidrop());
  assert(upper_bound.self_station_no() == 0x1FU);
  assert(upper_bound.self_station_valid());

  auto invalid_config = make_ascii_c2_format3_config();
  invalid_config = invalid_config.with_route(RouteConfig {C2MnMultidropRoute {
      0x01U, SelfStationNo::number(0x20U)}});
  Status status = FrameCodec::validate_config(invalid_config);
  assert(!status.ok());
  assert(status.code == StatusCode::InvalidArgument);
  assert(invalid_config.route().self_station_no() == 0x20U);

  const BatchReadWordsRequest request({mcprotocol::serial::DeviceCode::D, 100U}, 1U);
  const auto encode_request = [&](
                                  const ProtocolConfig& config,
                                  std::array<std::uint8_t, 128>& frame,
                                  std::size_t& frame_size) {
    std::array<std::uint8_t, 64> command {};
    std::size_t command_size = 0U;
    Status encode_status = CommandCodec::encode_batch_read_words(
        config, request, command, command_size);
    assert(encode_status.ok());
    frame_size = 0U;
    encode_status = FrameCodec::encode_request(
        config,
        mcprotocol::serial::Span<const std::uint8_t>(command.data(), command_size),
        frame,
        frame_size);
    assert(encode_status.ok());
  };

  auto c2_standard = make_ascii_c2_format3_config();
  c2_standard = c2_standard.with_route(RouteConfig {C2StandardMultidropRoute {0x11U}});
  auto c2_zero = c2_standard;
  c2_zero = c2_zero.with_route(RouteConfig {C2MnMultidropRoute {
      0x11U, SelfStationNo::number(0U)}});
  auto c2_upper = c2_standard;
  c2_upper = c2_upper.with_route(RouteConfig {C2MnMultidropRoute {
      0x11U, SelfStationNo::number(0x1FU)}});
  std::array<std::uint8_t, 128> standard_frame {};
  std::array<std::uint8_t, 128> zero_frame {};
  std::array<std::uint8_t, 128> upper_frame {};
  std::size_t standard_size = 0U;
  std::size_t zero_size = 0U;
  std::size_t upper_size = 0U;
  encode_request(c2_standard, standard_frame, standard_size);
  encode_request(c2_zero, zero_frame, zero_size);
  encode_request(c2_upper, upper_frame, upper_size);
  assert(standard_size == zero_size);
  assert(std::equal(
      standard_frame.begin(), standard_frame.begin() + standard_size, zero_frame.begin()));
  assert(upper_frame[5] == '1');
  assert(upper_frame[6] == 'F');

  auto c3_upper = make_ascii_c3_format3_config();
  c3_upper = c3_upper.with_route(RouteConfig {C3MnMultidropRoute {
      0x11U,
      0x01U,
      C34PcTarget::connected_station(),
      SelfStationNo::number(0x1FU)}});
  encode_request(c3_upper, upper_frame, upper_size);
  assert(upper_frame[9] == '1');
  assert(upper_frame[10] == 'F');

  auto c4_standard = make_binary_c4_config();
  c4_standard = c4_standard.with_route(RouteConfig {C4StandardMultidropRoute {
      0x11U,
      0x01U,
      C34PcTarget::connected_station(),
      C4DestinationModule::own_station()}});
  auto c4_zero = c4_standard;
  c4_zero = c4_zero.with_route(RouteConfig {C4MnMultidropRoute {
      0x11U,
      0x01U,
      C34PcTarget::connected_station(),
      C4DestinationModule::own_station(),
      SelfStationNo::number(0U)}});
  auto c4_upper = c4_standard;
  c4_upper = c4_upper.with_route(RouteConfig {C4MnMultidropRoute {
      0x11U,
      0x01U,
      C34PcTarget::connected_station(),
      C4DestinationModule::own_station(),
      SelfStationNo::number(0x1FU)}});
  encode_request(c4_standard, standard_frame, standard_size);
  encode_request(c4_zero, zero_frame, zero_size);
  encode_request(c4_upper, upper_frame, upper_size);
  assert(standard_size == zero_size);
  assert(std::equal(
      standard_frame.begin(), standard_frame.begin() + standard_size, zero_frame.begin()));
  assert(upper_frame[11] == 0x1FU);

  auto foreign_response = c4_upper;
  foreign_response = foreign_response.with_route(RouteConfig {C4MnMultidropRoute {
      0x11U,
      0x01U,
      C34PcTarget::connected_station(),
      C4DestinationModule::own_station(),
      SelfStationNo::number(0x1EU)}});
  std::size_t response_size = 0U;
  status = FrameCodec::encode_success_response(
      foreign_response,
      mcprotocol::serial::Span<const std::uint8_t> {},
      upper_frame,
      response_size);
  assert(status.ok());
  const auto decode = FrameCodec::decode_response(
      c4_upper,
      mcprotocol::serial::Span<const std::uint8_t>(upper_frame.data(), response_size));
  assert(decode.status == DecodeStatus::Complete);
  assert(decode.response_identity_mismatch);
  assert(decode.bytes_consumed == response_size);
}

void test_response_route_identity_is_strict() {
  const std::array<std::uint8_t, 4> response_data {'1', '2', '3', '4'};

  auto expected_ascii = make_ascii_c4_format4_config();
  auto foreign_ascii = expected_ascii;
  foreign_ascii = foreign_ascii.with_route(RouteConfig {C4StandardMultidropRoute {
      0x02U, 0x01U, C34PcTarget::connected_station(), C4DestinationModule::own_station()}});
  std::array<std::uint8_t, 128> frame {};
  std::size_t frame_size = 0U;
  Status status = FrameCodec::encode_success_response(
      foreign_ascii,
      response_data,
      frame,
      frame_size);
  assert(status.ok());

  auto decode = FrameCodec::decode_response(
      expected_ascii,
      mcprotocol::serial::Span<const std::uint8_t>(frame.data(), frame_size));
  assert(decode.status == DecodeStatus::Complete);
  assert(decode.response_identity_mismatch);
  assert(decode.bytes_consumed == frame_size);

  auto foreign_module_ascii = expected_ascii;
  foreign_module_ascii = foreign_module_ascii.with_route(RouteConfig {C4StandardMultidropRoute {
      expected_ascii.route().station_no(),
      expected_ascii.route().network_no(),
      C34PcTarget::connected_station(),
      C4DestinationModule::multiple_cpu(1U)}});
  frame_size = 0U;
  status = FrameCodec::encode_success_response(
      foreign_module_ascii,
      response_data,
      frame,
      frame_size);
  assert(status.ok());
  decode = FrameCodec::decode_response(
      expected_ascii,
      mcprotocol::serial::Span<const std::uint8_t>(frame.data(), frame_size));
  assert(decode.status == DecodeStatus::Complete);
  assert(decode.response_identity_mismatch);
  assert(decode.bytes_consumed == frame_size);

  frame[9] = static_cast<std::uint8_t>('Z');
  decode = FrameCodec::decode_response(
      foreign_module_ascii,
      mcprotocol::serial::Span<const std::uint8_t>(frame.data(), frame_size));
  assert(decode.status == DecodeStatus::Error);
  assert(decode.error.code == StatusCode::Parse);
  assert(!decode.response_identity_mismatch);

  auto foreign_pc_ascii = expected_ascii;
  foreign_pc_ascii = foreign_pc_ascii.with_route(RouteConfig {C4StandardMultidropRoute {
      expected_ascii.route().station_no(),
      expected_ascii.route().network_no(),
      C34PcTarget::control_system(),
      C4DestinationModule::own_station()}});
  frame_size = 0U;
  status = FrameCodec::encode_success_response(
      foreign_pc_ascii,
      response_data,
      frame,
      frame_size);
  assert(status.ok());
  decode = FrameCodec::decode_response(
      expected_ascii,
      mcprotocol::serial::Span<const std::uint8_t>(frame.data(), frame_size));
  assert(decode.status == DecodeStatus::Complete);
  assert(decode.response_identity_mismatch);
  assert(decode.bytes_consumed == frame_size);

  frame[3] = static_cast<std::uint8_t>('Z');
  decode = FrameCodec::decode_response(
      foreign_ascii,
      mcprotocol::serial::Span<const std::uint8_t>(frame.data(), frame_size));
  assert(decode.status == DecodeStatus::Error);
  assert(decode.error.code == StatusCode::Parse);
  assert(!decode.response_identity_mismatch);

  const auto expected_binary = make_binary_c4_config();
  auto foreign_binary = expected_binary;
  foreign_binary = foreign_binary.with_route(RouteConfig {C4StandardMultidropRoute {
      0x02U, 0x01U, C34PcTarget::connected_station(), C4DestinationModule::own_station()}});
  frame_size = 0U;
  status = FrameCodec::encode_success_response(
      foreign_binary,
      mcprotocol::serial::Span<const std::uint8_t> {},
      frame,
      frame_size);
  assert(status.ok());
  decode = FrameCodec::decode_response(
      expected_binary,
      mcprotocol::serial::Span<const std::uint8_t>(frame.data(), frame_size));
  assert(decode.status == DecodeStatus::Complete);
  assert(decode.response_identity_mismatch);
  assert(decode.bytes_consumed == frame_size);

  auto foreign_pc_binary = expected_binary;
  foreign_pc_binary = foreign_pc_binary.with_route(RouteConfig {C4StandardMultidropRoute {
      expected_binary.route().station_no(),
      expected_binary.route().network_no(),
      C34PcTarget::control_system(),
      C4DestinationModule::own_station()}});
  frame_size = 0U;
  status = FrameCodec::encode_success_response(
      foreign_pc_binary,
      mcprotocol::serial::Span<const std::uint8_t> {},
      frame,
      frame_size);
  assert(status.ok());
  decode = FrameCodec::decode_response(
      expected_binary,
      mcprotocol::serial::Span<const std::uint8_t>(frame.data(), frame_size));
  assert(decode.status == DecodeStatus::Complete);
  assert(decode.response_identity_mismatch);
  assert(decode.bytes_consumed == frame_size);

  auto foreign_module_binary = expected_binary;
  foreign_module_binary = foreign_module_binary.with_route(RouteConfig {C4StandardMultidropRoute {
      expected_binary.route().station_no(),
      expected_binary.route().network_no(),
      C34PcTarget::connected_station(),
      C4DestinationModule::multiple_cpu(1U)}});
  frame_size = 0U;
  status = FrameCodec::encode_success_response(
      foreign_module_binary,
      mcprotocol::serial::Span<const std::uint8_t> {},
      frame,
      frame_size);
  assert(status.ok());
  decode = FrameCodec::decode_response(
      expected_binary,
      mcprotocol::serial::Span<const std::uint8_t>(frame.data(), frame_size));
  assert(decode.status == DecodeStatus::Complete);
  assert(decode.response_identity_mismatch);
  assert(decode.bytes_consumed == frame_size);
}

void test_pc_targets_are_required_typed_and_frame_specific() {
  static_assert(!std::is_default_constructible_v<C34PcTarget>);
  static_assert(!std::is_constructible_v<
                C3StandardMultidropRoute,
                std::uint32_t,
                std::uint32_t>);
  static_assert(!std::is_constructible_v<
                C4StandardMultidropRoute,
                std::uint32_t,
                std::uint32_t>);

  assert(C34PcTarget::number(0x01U).is_valid());
  assert(C34PcTarget::number(0x78U).is_valid());
  assert(C34PcTarget::control_system().is_valid());
  assert(C34PcTarget::standby_system().is_valid());
  assert(C34PcTarget::special_fe().is_valid());
  assert(C34PcTarget::connected_station().is_valid());
  for (const std::uint32_t invalid : {0x00U, 0x79U, 0x7DU, 0x7EU, 0xFEU, 0xFFU, 0x100U}) {
    assert(!C34PcTarget::number(invalid).is_valid());
  }

  ProtocolConfig config = make_binary_c4_config();
  config = config.with_route(RouteConfig {C4StandardMultidropRoute {
      0x00U, 0x00U, C34PcTarget::number(0x00U), C4DestinationModule::own_station()}});
  Status status = FrameCodec::validate_config(config);
  assert(!status.ok());
  assert(status.code == StatusCode::InvalidArgument);

  std::array<std::uint8_t, 32> frame {};
  std::size_t frame_size = 99U;
  const std::array<std::uint8_t, 1> request_data {0x00U};
  status = FrameCodec::encode_request(config, request_data, frame, frame_size);
  assert(!status.ok());
  assert(frame_size == 0U);

  config = config.with_route(RouteConfig {C4StandardMultidropRoute {
      0x00U, 0x00U, C34PcTarget::control_system(), C4DestinationModule::own_station()}});
  assert(FrameCodec::validate_config(config).ok());
  assert(config.route().pc_no() == 0x7DU);

  std::array<std::uint8_t, 16> state_change_data {};
  std::size_t state_change_size = 99U;
  status = CommandCodec::encode_remote_stop(config, state_change_data, state_change_size);
  assert(status.ok());
  assert(state_change_size != 0U);

  ProtocolConfig invalid_state_change_config = config;
  invalid_state_change_config = invalid_state_change_config.with_route(RouteConfig {C4StandardMultidropRoute {
      0x00U, 0x00U, C34PcTarget::number(0x00U), C4DestinationModule::own_station()}});
  frame_size = 99U;
  status = FrameCodec::encode_request(
      invalid_state_change_config,
      mcprotocol::serial::Span<const std::uint8_t>(state_change_data.data(), state_change_size),
      frame,
      frame_size);
  assert(!status.ok());
  assert(status.code == StatusCode::InvalidArgument);
  assert(frame_size == 0U);

  ProtocolConfig invalid_client_config = make_binary_c4_config();
  invalid_client_config = invalid_client_config.with_route(RouteConfig {C4StandardMultidropRoute {
      0x00U, 0x00U, C34PcTarget::number(0x100U), C4DestinationModule::own_station()}});
  MelsecSerialClient client;
  status = client.configure(invalid_client_config);
  assert(!status.ok());
  assert(client.pending_tx_frame().empty());

  const RouteConfig c1_route {C1MultidropRoute {0x00U}};
  const RouteConfig c2_route {C2StandardMultidropRoute {0x00U}};
  assert(c1_route.pc_no() == 0xFFU);
  assert(c2_route.pc_no() == 0xFFU);
}

void test_c4_destination_module_is_required_typed_and_validated() {
  static_assert(!std::is_default_constructible_v<C4DestinationModule>);
  static_assert(!std::is_constructible_v<
                C4StandardMultidropRoute,
                std::uint32_t,
                std::uint32_t,
                C34PcTarget>);
  static_assert(std::is_constructible_v<
                C4StandardMultidropRoute,
                std::uint32_t,
                std::uint32_t,
                C34PcTarget,
                C4DestinationModule>);

  const auto own = C4DestinationModule::own_station();
  assert(own.is_valid());
  assert(own.is_own_station_selector());
  assert(own.io_number() == module_io::OwnStation);
  assert(own.station_number() == 0U);

  for (std::uint32_t cpu = 1U; cpu <= 4U; ++cpu) {
    const auto target = C4DestinationModule::multiple_cpu(cpu);
    assert(target.is_valid());
    assert(!target.is_own_station_selector());
    assert(target.io_number() == module_io::MultipleCpu1 + cpu - 1U);
    assert(target.station_number() == 0U);
  }
  assert(!C4DestinationModule::multiple_cpu(0U).is_valid());
  assert(!C4DestinationModule::multiple_cpu(5U).is_valid());

  assert(C4DestinationModule::redundant_control_system_cpu().io_number() ==
         module_io::ControlSystemCpu);
  assert(C4DestinationModule::redundant_standby_system_cpu().io_number() ==
         module_io::StandbySystemCpu);
  assert(C4DestinationModule::redundant_system_a_cpu().io_number() == module_io::SystemACpu);
  assert(C4DestinationModule::redundant_system_b_cpu().io_number() == module_io::SystemBCpu);
  assert(C4DestinationModule::redundant_control_system_cpu().is_valid());
  assert(C4DestinationModule::redundant_standby_system_cpu().is_valid());
  assert(C4DestinationModule::redundant_system_a_cpu().is_valid());
  assert(C4DestinationModule::redundant_system_b_cpu().is_valid());

  assert(C4DestinationModule::explicit_target(0x0000U, 0x00U).is_valid());
  assert(C4DestinationModule::explicit_target(0xFFFFU, 0xFFU).is_valid());
  assert(!C4DestinationModule::explicit_target(0x10000U, 0x00U).is_valid());
  assert(!C4DestinationModule::explicit_target(0x0000U, 0x100U).is_valid());

  ProtocolConfig invalid_config = make_binary_c4_config();
  invalid_config = invalid_config.with_route(RouteConfig {C4StandardMultidropRoute {
      0x00U,
      0x00U,
      C34PcTarget::connected_station(),
      C4DestinationModule::explicit_target(0x10000U, 0x00U)}});
  Status status = FrameCodec::validate_config(invalid_config);
  assert(!status.ok());
  assert(status.code == StatusCode::InvalidArgument);

  std::array<std::uint8_t, 32> frame {};
  const std::array<std::uint8_t, 1> request_data {0x00U};
  std::size_t frame_size = 99U;
  status = FrameCodec::encode_request(invalid_config, request_data, frame, frame_size);
  assert(!status.ok());
  assert(frame_size == 0U);

  MelsecSerialClient invalid_client;
  status = invalid_client.configure(invalid_config);
  assert(!status.ok());
  assert(invalid_client.pending_tx_frame().empty());

  ProtocolConfig own_config = make_binary_c4_config();
  own_config = own_config.with_route(RouteConfig {C4StandardMultidropRoute {
      0x00U,
      0x00U,
      C34PcTarget::connected_station(),
      C4DestinationModule::own_station()}});
  std::array<std::uint8_t, 64> command_data {};
  std::size_t command_size = 0U;
  const std::array<std::uint16_t, 1> host_words {0x1234U};
  status = CommandCodec::encode_read_host_buffer(
      own_config,
      HostBufferReadRequest(0U, 1U),
      command_data,
      command_size);
  assert(status.ok());
  command_size = 0U;
  status = CommandCodec::encode_write_host_buffer(
      own_config,
      HostBufferWriteRequest(0U, host_words),
      command_data,
      command_size);
  assert(status.ok());

  ProtocolConfig cpu_config = own_config;
  cpu_config = cpu_config.with_route(RouteConfig {C4StandardMultidropRoute {
      0x00U,
      0x00U,
      C34PcTarget::connected_station(),
      C4DestinationModule::multiple_cpu(1U)}});
  command_size = 99U;
  status = CommandCodec::encode_read_host_buffer(
      cpu_config,
      HostBufferReadRequest(0U, 1U),
      command_data,
      command_size);
  assert(!status.ok());
  assert(status.code == StatusCode::InvalidArgument);
  command_size = 99U;
  status = CommandCodec::encode_write_host_buffer(
      cpu_config,
      HostBufferWriteRequest(0U, host_words),
      command_data,
      command_size);
  assert(!status.ok());
  assert(status.code == StatusCode::InvalidArgument);

  ProtocolConfig explicit_own_config = own_config;
  explicit_own_config = explicit_own_config.with_route(RouteConfig {C4StandardMultidropRoute {
      0x00U,
      0x00U,
      C34PcTarget::connected_station(),
      C4DestinationModule::explicit_target(module_io::OwnStation, 0x00U)}});
  command_size = 99U;
  status = CommandCodec::encode_read_host_buffer(
      explicit_own_config,
      HostBufferReadRequest(0U, 1U),
      command_data,
      command_size);
  assert(!status.ok());
  assert(status.code == StatusCode::InvalidArgument);

  command_size = 0U;
  status = CommandCodec::encode_batch_read_words(
      cpu_config,
      BatchReadWordsRequest(DeviceAddress {mcprotocol::serial::DeviceCode::D, 100U}, 1U),
      command_data,
      command_size);
  assert(status.ok());
  frame_size = 0U;
  status = FrameCodec::encode_request(
      cpu_config,
      mcprotocol::serial::Span<const std::uint8_t>(command_data.data(), command_size),
      frame,
      frame_size);
  assert(status.ok());
  assert(frame_size != 0U);

  const std::array<std::uint16_t, 2> words {0x1234U, 0x5678U};
  command_size = 0U;
  status = CommandCodec::encode_batch_write_words(
      cpu_config,
      BatchWriteWordsRequest(DeviceAddress {mcprotocol::serial::DeviceCode::D, 100U}, words),
      command_data,
      command_size);
  assert(status.ok());
  frame_size = 0U;
  status = FrameCodec::encode_request(
      cpu_config,
      mcprotocol::serial::Span<const std::uint8_t>(command_data.data(), command_size),
      frame,
      frame_size);
  assert(status.ok());
  assert(frame_size != 0U);
}

void test_encode_ascii_c2_format3_request_uses_fb_frame_id_and_short_command() {
  auto config = make_ascii_c2_format3_config();
  config = config.with_route(mn_multidrop_route(FrameKind::C2, 0x11U, 0x00U, 0xFFU, module_io::OwnStation, 0x00U, 0x05U));

  const BatchReadWordsRequest request({mcprotocol::serial::DeviceCode::D, 100}, 1);

  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_data_size = 0;
  Status status = CommandCodec::encode_batch_read_words(config, request, request_data, request_data_size);
  assert(status.ok());

  std::array<std::uint8_t, 128> frame {};
  std::size_t frame_size = 0;
  status = FrameCodec::encode_request(
      config,
      mcprotocol::serial::Span<const std::uint8_t>(request_data.data(), request_data_size),
      frame,
      frame_size);
  assert(status.ok());

  constexpr std::string_view expected_body = "FB11052D*0001000001";
  assert(frame_size == (1U + expected_body.size() + 1U + 2U));
  assert(frame[0] == 0x02);
  assert(std::memcmp(frame.data() + 1, expected_body.data(), expected_body.size()) == 0);
  assert(frame[1U + expected_body.size()] == 0x03);
}

void test_decode_ascii_c2_format3_data_response() {
  auto config = make_ascii_c2_format3_config();
  config = config.with_route(mn_multidrop_route(FrameKind::C2, 0x11U, 0x00U, 0xFFU, module_io::OwnStation, 0x00U, 0x05U));
  const std::array<std::uint8_t, 4> response_data {'1', '2', '3', '4'};

  std::array<std::uint8_t, 64> frame {};
  std::size_t frame_size = 0;
  Status status = FrameCodec::encode_success_response(config, response_data, frame, frame_size);
  assert(status.ok());

  constexpr std::string_view expected_prefix = "FB1105QACK";
  assert(frame[0] == 0x02);
  assert(std::memcmp(frame.data() + 1, expected_prefix.data(), expected_prefix.size()) == 0);

  const auto decode = FrameCodec::decode_response(
      config,
      mcprotocol::serial::Span<const std::uint8_t>(frame.data(), frame_size));
  assert(decode.status == DecodeStatus::Complete);
  assert(decode.frame.type == mcprotocol::serial::ResponseType::SuccessData);
  assert(decode.frame.response_size == response_data.size());
  assert(std::memcmp(decode.frame.response_data.data(), response_data.data(), response_data.size()) == 0);
}

void test_decode_ascii_c2_format3_four_digit_error_response() {
  auto config = make_ascii_c2_format3_config();
  config = config.with_route(mn_multidrop_route(FrameKind::C2, 0x11U, 0x00U, 0xFFU, module_io::OwnStation, 0x00U, 0x05U));
  config = test_config_with_sum_check(config, SumCheckMode::Disabled);

  constexpr std::string_view frame = "\x02""FB1105QNAK0006\x03";
  const auto decode = FrameCodec::decode_response(
      config,
      mcprotocol::serial::Span<const std::uint8_t>(
          reinterpret_cast<const std::uint8_t*>(frame.data()),
          frame.size()));
  assert(decode.status == DecodeStatus::Complete);
  assert(decode.frame.type == mcprotocol::serial::ResponseType::PlcError);
  assert(decode.frame.error_code == 0x0006U);
  assert(decode.bytes_consumed == frame.size());
}

void test_encode_ascii_c2_format3_error_preserves_four_digit_code() {
  auto config = make_ascii_c2_format3_config();
  config = config.with_route(mn_multidrop_route(FrameKind::C2, 0x11U, 0x00U, 0xFFU, module_io::OwnStation, 0x00U, 0x05U));
  config = test_config_with_sum_check(config, SumCheckMode::Disabled);

  std::array<std::uint8_t, 32> frame {};
  std::size_t frame_size = 0;
  Status status = FrameCodec::encode_error_response(config, 0x7151U, frame, frame_size);
  assert(status.ok());

  constexpr std::string_view expected = "\x02""FB1105QNAK7151\x03";
  assert(frame_size == expected.size());
  assert(std::memcmp(frame.data(), expected.data(), frame_size) == 0);

  const auto decode = FrameCodec::decode_response(
      config,
      mcprotocol::serial::Span<const std::uint8_t>(frame.data(), frame_size));
  assert(decode.status == DecodeStatus::Complete);
  assert(decode.frame.type == mcprotocol::serial::ResponseType::PlcError);
  assert(decode.frame.error_code == 0x7151U);
}

void test_encode_ascii_format2_request_inserts_block_number() {
  auto config = make_ascii_c4_format2_config();
  config = test_config_with_sum_check(config, SumCheckMode::Disabled);

  const BatchReadWordsRequest request({mcprotocol::serial::DeviceCode::D, 100}, 1);

  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_data_size = 0;
  Status status = CommandCodec::encode_batch_read_words(config, request, request_data, request_data_size);
  assert(status.ok());

  std::array<std::uint8_t, 128> frame {};
  std::size_t frame_size = 0;
  status = FrameCodec::encode_request(
      config,
      FrameCodecContext::format2(0x7AU),
      mcprotocol::serial::Span<const std::uint8_t>(request_data.data(), request_data_size),
      frame,
      frame_size);
  assert(status.ok());

  constexpr std::string_view expected = "\x05""7AF80000FF03FF000004010000D*0001000001";
  assert(frame_size == expected.size());
  assert(std::memcmp(frame.data(), expected.data(), expected.size()) == 0);
}

void test_decode_ascii_format2_partial_header_returns_incomplete() {
  auto config = make_ascii_c4_format2_config();
  config = test_config_with_sum_check(config, SumCheckMode::Disabled);

  // Field-observed partial link-direct responses (FORMAT2 investigation packet):
  // the first serial chunk can be shorter than the STX + block-no + header
  // prefix. The decoder must report Incomplete instead of reading past the
  // buffer.
  constexpr std::string_view partials[] = {
      "\x02",
      "\x02""0",
      "\x02""00F8000",
      "\x02""00F80000",
      "\x02""00F80000FF0",
  };
  for (const std::string_view partial : partials) {
    const auto decode = FrameCodec::decode_response(
        config,
        FrameCodecContext::format2(0x00U),
        mcprotocol::serial::Span<const std::uint8_t>(
            reinterpret_cast<const std::uint8_t*>(partial.data()),
            partial.size()));
    assert(decode.status == DecodeStatus::Incomplete);
    assert(decode.bytes_consumed == 0U);
  }
}

void test_decode_ascii_format1_partial_header_returns_incomplete() {
  auto config = make_ascii_c4_format2_config();
  config = ProtocolConfig::ascii(
      mcprotocol::serial::AsciiFrameKind::C4,
      AsciiFormat::Format1,
      config.plc_profile(),
      SumCheckMode::Disabled,
      config.route());

  // Field-observed partial link-direct responses (FORMAT1 investigation packet).
  constexpr std::string_view partials[] = {
      "\x02""F80000FF0",
      "\x02""F80000FF03",
      "\x02""F80000FF03F",
      "\x02""F80000FF03FF00",
  };
  for (const std::string_view partial : partials) {
    const auto decode = FrameCodec::decode_response(
        config,
        mcprotocol::serial::Span<const std::uint8_t>(
            reinterpret_cast<const std::uint8_t*>(partial.data()),
            partial.size()));
    assert(decode.status == DecodeStatus::Incomplete);
    assert(decode.bytes_consumed == 0U);
  }
}

void test_decode_response_prefix_sweep_reports_incomplete() {
  // Every strict prefix of a valid success or error response must decode as
  // Incomplete for the C-frame families: never Complete, never Error, and no
  // out-of-bounds access (regression net for the Format1/2 partial-header
  // crash).
  ProtocolConfig configs[] = {
      make_ascii_c4_format2_config(),
      test_config_with_sum_check(make_ascii_c4_format2_config(), SumCheckMode::Disabled),
      ProtocolConfig::ascii(
          mcprotocol::serial::AsciiFrameKind::C4,
          AsciiFormat::Format1,
          PlcProfile::MelsecQ,
          SumCheckMode::Enabled,
          host_station_route()),
      make_ascii_c2_format2_config(),
      make_ascii_c3_format3_config(),
      make_ascii_c2_format3_config(),
      make_ascii_c4_format4_config(),
      make_ascii_c2_format4_config(),
      make_ascii_c4_format4_iqr_config(),
      make_ascii_c1_format4_qna_config(),
      make_ascii_c1_format4_a_config(),
      make_binary_c4_config(),
      test_config_with_sum_check(make_binary_c4_config(), SumCheckMode::Disabled),
  };

  const std::array<std::uint8_t, 4> response_data {'1', '2', '3', '4'};
  for (const ProtocolConfig& config : configs) {
    const FrameCodecContext frame_context =
        config.code_mode() == CodeMode::Ascii &&
                config.ascii_format() == AsciiFormat::Format2 &&
                config.frame_kind() != FrameKind::C1
            ? FrameCodecContext::format2(0x00U)
            : FrameCodecContext::none();
    std::array<std::uint8_t, 128> frame {};
    std::size_t frame_size = 0;

    Status status = FrameCodec::encode_success_response(
        config,
        frame_context,
        response_data,
        frame,
        frame_size);
    assert(status.ok());
    for (std::size_t length = 1; length < frame_size; ++length) {
      const auto decode = FrameCodec::decode_response(
          config,
          frame_context,
          mcprotocol::serial::Span<const std::uint8_t>(frame.data(), length));
      assert(decode.status == DecodeStatus::Incomplete);
      assert(decode.bytes_consumed == 0U);
    }

    status = FrameCodec::encode_error_response(
        config,
        frame_context,
        0x7151U,
        frame,
        frame_size);
    assert(status.ok());
    for (std::size_t length = 1; length < frame_size; ++length) {
      const auto decode = FrameCodec::decode_response(
          config,
          frame_context,
          mcprotocol::serial::Span<const std::uint8_t>(frame.data(), length));
      assert(decode.status == DecodeStatus::Incomplete);
      assert(decode.bytes_consumed == 0U);
    }
  }
}

void test_encode_ascii_c2_format2_request_uses_fb_frame_id_and_short_command() {
  auto config = make_ascii_c2_format2_config();
  config = test_config_with_sum_check(config, SumCheckMode::Disabled);
  config = config.with_route(mn_multidrop_route(FrameKind::C2, 0x11U, 0x00U, 0xFFU, module_io::OwnStation, 0x00U, 0x05U));

  const BatchReadWordsRequest request({mcprotocol::serial::DeviceCode::D, 100}, 1);

  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_data_size = 0;
  Status status = CommandCodec::encode_batch_read_words(config, request, request_data, request_data_size);
  assert(status.ok());

  std::array<std::uint8_t, 128> frame {};
  std::size_t frame_size = 0;
  status = FrameCodec::encode_request(
      config,
      FrameCodecContext::format2(0x7AU),
      mcprotocol::serial::Span<const std::uint8_t>(request_data.data(), request_data_size),
      frame,
      frame_size);
  assert(status.ok());

  constexpr std::string_view expected = "\x05""7AFB11052D*0001000001";
  assert(frame_size == expected.size());
  assert(std::memcmp(frame.data(), expected.data(), expected.size()) == 0);
}

void test_decode_ascii_format2_ack_response() {
  auto config = make_ascii_c4_format2_config();
  config = test_config_with_sum_check(config, SumCheckMode::Disabled);
  std::array<std::uint8_t, 32> frame {};
  std::size_t frame_size = 0;
  Status status = FrameCodec::encode_success_response(
      config,
      FrameCodecContext::format2(0x7AU),
      {},
      frame,
      frame_size);
  assert(status.ok());

  constexpr std::string_view expected = "\x06""7AF80000FF03FF0000";
  assert(frame_size == expected.size());
  assert(std::memcmp(frame.data(), expected.data(), expected.size()) == 0);

  const auto decode = FrameCodec::decode_response(
      config,
      FrameCodecContext::format2(0x7AU),
      mcprotocol::serial::Span<const std::uint8_t>(frame.data(), frame_size));
  assert(decode.status == DecodeStatus::Complete);
  assert(decode.frame.type == mcprotocol::serial::ResponseType::SuccessNoData);
  assert(decode.bytes_consumed == frame_size);
}

void test_decode_ascii_c2_format2_four_digit_error_response() {
  auto config = make_ascii_c2_format2_config();
  config = test_config_with_sum_check(config, SumCheckMode::Disabled);

  constexpr std::string_view frame = "\x15""7AFB0000QNAK0006";
  const auto decode = FrameCodec::decode_response(
      config,
      FrameCodecContext::format2(0x7AU),
      mcprotocol::serial::Span<const std::uint8_t>(
          reinterpret_cast<const std::uint8_t*>(frame.data()),
          frame.size()));
  assert(decode.status == DecodeStatus::Complete);
  assert(decode.frame.type == mcprotocol::serial::ResponseType::PlcError);
  assert(decode.frame.error_code == 0x0006U);
  assert(decode.bytes_consumed == frame.size());
}

void test_encode_ascii_c2_format2_error_preserves_four_digit_code() {
  auto config = make_ascii_c2_format2_config();
  config = test_config_with_sum_check(config, SumCheckMode::Disabled);

  std::array<std::uint8_t, 32> frame {};
  std::size_t frame_size = 0;
  Status status = FrameCodec::encode_error_response(
      config,
      FrameCodecContext::format2(0x7AU),
      0x7151U,
      frame,
      frame_size);
  assert(status.ok());

  constexpr std::string_view expected = "\x15""7AFB0000QNAK7151";
  assert(frame_size == expected.size());
  assert(std::memcmp(frame.data(), expected.data(), frame_size) == 0);

  const auto decode = FrameCodec::decode_response(
      config,
      FrameCodecContext::format2(0x7AU),
      mcprotocol::serial::Span<const std::uint8_t>(frame.data(), frame_size));
  assert(decode.status == DecodeStatus::Complete);
  assert(decode.frame.type == mcprotocol::serial::ResponseType::PlcError);
  assert(decode.frame.error_code == 0x7151U);
}

void test_sum_check_modes_are_strict_and_corruption_is_rejected() {
  const auto verify = [](
                          const char* label,
                          const ProtocolConfig& enabled,
                          FrameCodecContext context,
                          std::size_t checksum_offset_from_end) {
    const std::array<std::uint8_t, 4> response_data {'1', '2', '3', '4'};
    std::array<std::uint8_t, 128> enabled_frame {};
    std::size_t enabled_size = 0U;
    Status status = FrameCodec::encode_success_response(
        enabled, context, response_data, enabled_frame, enabled_size);
    if (!status.ok()) {
      std::fprintf(stderr, "sum-check vector %s encode failed: %s\n", label, status.message);
    }
    assert(status.ok());
    assert(enabled.sum_check_mode() == SumCheckMode::Enabled);
    assert(enabled_size > checksum_offset_from_end);

    const ProtocolConfig disabled = test_config_with_sum_check(enabled, SumCheckMode::Disabled);
    std::array<std::uint8_t, 128> disabled_frame {};
    std::size_t disabled_size = 0U;
    status = FrameCodec::encode_success_response(
        disabled, context, response_data, disabled_frame, disabled_size);
    assert(status.ok());
    assert(disabled.sum_check_mode() == SumCheckMode::Disabled);
    assert(enabled_size == disabled_size + 2U);

    const auto disabled_decode = FrameCodec::decode_response(
        disabled,
        context,
        mcprotocol::serial::Span<const std::uint8_t>(disabled_frame.data(), disabled_size));
    assert(disabled_decode.status == DecodeStatus::Complete);
    assert(disabled_decode.bytes_consumed == disabled_size);

    const auto missing_checksum = FrameCodec::decode_response(
        enabled,
        context,
        mcprotocol::serial::Span<const std::uint8_t>(disabled_frame.data(), disabled_size));
    assert(missing_checksum.status == DecodeStatus::Incomplete);
    assert(missing_checksum.bytes_consumed == 0U);

    const auto disabled_with_extra_checksum = FrameCodec::decode_response(
        disabled,
        context,
        mcprotocol::serial::Span<const std::uint8_t>(enabled_frame.data(), enabled_size));
    if (checksum_offset_from_end == 4U) {
      assert(disabled_with_extra_checksum.status == DecodeStatus::Error);
      assert(disabled_with_extra_checksum.error.code == StatusCode::Framing);
      assert(disabled_with_extra_checksum.bytes_consumed == disabled_size);
    } else {
      assert(disabled_with_extra_checksum.status == DecodeStatus::Complete);
      assert(disabled_with_extra_checksum.bytes_consumed == disabled_size);
      assert(disabled_with_extra_checksum.bytes_consumed + 2U == enabled_size);
    }

    const std::size_t checksum_index = enabled_size - checksum_offset_from_end;
    enabled_frame[checksum_index] = enabled_frame[checksum_index] == '0' ? '1' : '0';
    const auto corrupted = FrameCodec::decode_response(
        enabled,
        context,
        mcprotocol::serial::Span<const std::uint8_t>(enabled_frame.data(), enabled_size));
    assert(corrupted.status == DecodeStatus::Error);
    assert(corrupted.error.code == StatusCode::SumCheckMismatch);
    assert(corrupted.bytes_consumed == enabled_size);
    assert(enabled.sum_check_mode() == SumCheckMode::Enabled);
  };

  verify(
      "c4-ascii-f1",
      ProtocolConfig::ascii(
          mcprotocol::serial::AsciiFrameKind::C4,
          AsciiFormat::Format1,
          PlcProfile::MelsecQ,
          SumCheckMode::Enabled,
          host_station_route()),
      FrameCodecContext::none(),
      2U);
  verify("c4-ascii-f2", make_ascii_c4_format2_config(), FrameCodecContext::format2(0xA5U), 2U);
  verify("c3-ascii-f3", make_ascii_c3_format3_config(), FrameCodecContext::none(), 2U);
  verify(
      "c4-ascii-f4",
      test_config_with_sum_check(make_ascii_c4_format4_config(), SumCheckMode::Enabled),
      FrameCodecContext::none(),
      4U);
  verify(
      "c1-ascii-f4",
      test_config_with_sum_check(make_ascii_c1_format4_qna_config(), SumCheckMode::Enabled),
      FrameCodecContext::none(),
      4U);
  verify("c4-binary", make_binary_c4_config(), FrameCodecContext::none(), 2U);
}

void test_format2_raw_context_is_explicit_and_strict() {
  auto config = make_ascii_c4_format2_config();
  config = test_config_with_sum_check(config, SumCheckMode::Disabled);
  const std::array<std::uint8_t, 1> request_data {0x00U};
  std::array<std::uint8_t, 128> frame {};
  std::size_t frame_size = 123U;

  Status status = FrameCodec::encode_request(config, request_data, frame, frame_size);
  assert(status.code == StatusCode::InvalidArgument);
  assert(frame_size == 0U);

  status = FrameCodec::encode_success_response(
      config,
      FrameCodecContext::format2(0x2AU),
      {},
      frame,
      frame_size);
  assert(status.ok());

  const auto mismatch = FrameCodec::decode_response(
      config,
      FrameCodecContext::format2(0x2BU),
      mcprotocol::serial::Span<const std::uint8_t>(frame.data(), frame_size));
  assert(mismatch.status == DecodeStatus::Complete);
  assert(mismatch.response_identity_mismatch);
  assert(mismatch.bytes_consumed == frame_size);

  frame[1] = 'G';
  const auto malformed = FrameCodec::decode_response(
      config,
      FrameCodecContext::format2(0x2AU),
      mcprotocol::serial::Span<const std::uint8_t>(frame.data(), frame_size));
  assert(malformed.status == DecodeStatus::Error);
  assert(malformed.error.code == StatusCode::Parse);
  assert(!malformed.response_identity_mismatch);

  const auto non_format2 = make_binary_c4_config();
  frame_size = 123U;
  status = FrameCodec::encode_request(
      non_format2,
      FrameCodecContext::format2(0x00U),
      request_data,
      frame,
      frame_size);
  assert(status.code == StatusCode::InvalidArgument);
  assert(frame_size == 0U);
}

void test_encode_ascii_c1_batch_read_words_qna_request_shape() {
  const auto config = make_ascii_c1_format4_qna_config();
  const BatchReadWordsRequest request({mcprotocol::serial::DeviceCode::D, 100}, 1);

  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_data_size = 0;
  Status status = CommandCodec::encode_batch_read_words(config, request, request_data, request_data_size);
  assert(status.ok());
  constexpr std::string_view expected_request_data = "QR0D00010001";
  assert(request_data_size == expected_request_data.size());
  assert(std::memcmp(request_data.data(), expected_request_data.data(), expected_request_data.size()) == 0);

  std::array<std::uint8_t, 128> frame {};
  std::size_t frame_size = 0;
  status = FrameCodec::encode_request(
      config,
      mcprotocol::serial::Span<const std::uint8_t>(request_data.data(), request_data_size),
      frame,
      frame_size);
  assert(status.ok());
  constexpr std::string_view expected_frame = "\x05""00FFQR0D00010001\r\n";
  assert(frame_size == expected_frame.size());
  assert(std::memcmp(frame.data(), expected_frame.data(), frame_size) == 0);
}

void test_encode_ascii_c1_batch_read_bits_a_request_shape() {
  const auto config = make_ascii_c1_format4_a_config();
  const BatchReadBitsRequest request({mcprotocol::serial::DeviceCode::X, 0x40}, 5);

  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_data_size = 0;
  Status status = CommandCodec::encode_batch_read_bits(config, request, request_data, request_data_size);
  assert(status.ok());
  constexpr std::string_view expected_request_data = "BR0X004005";
  assert(request_data_size == expected_request_data.size());
  assert(std::memcmp(request_data.data(), expected_request_data.data(), expected_request_data.size()) == 0);
}

void test_encode_ascii_c1_batch_write_words_qna_request_shape() {
  const auto config = make_ascii_c1_format4_qna_config();
  const std::array<std::uint16_t, 2> values {0x1234U, 0x5678U};
  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_batch_write_words(
      config,
      BatchWriteWordsRequest(DeviceAddress {mcprotocol::serial::DeviceCode::D, 100}, values),
      request_data,
      request_size);
  assert(status.ok());

  constexpr std::string_view expected = "QW0D0001000212345678";
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_decode_ascii_c1_ack_response() {
  const auto config = make_ascii_c1_format4_qna_config();
  std::array<std::uint8_t, 32> frame {};
  std::size_t frame_size = 0;
  Status status = FrameCodec::encode_success_response(config, {}, frame, frame_size);
  assert(status.ok());

  constexpr std::string_view expected = "\x06""00FF\r\n";
  assert(frame_size == expected.size());
  assert(std::memcmp(frame.data(), expected.data(), frame_size) == 0);

  const auto decode = FrameCodec::decode_response(
      config,
      mcprotocol::serial::Span<const std::uint8_t>(frame.data(), frame_size));
  assert(decode.status == DecodeStatus::Complete);
  assert(decode.frame.type == mcprotocol::serial::ResponseType::SuccessNoData);
  assert(decode.bytes_consumed == frame_size);
}

void test_encode_ascii_c1_format3_uses_gg_end_code() {
  const auto config = ProtocolConfig::ascii(
      mcprotocol::serial::AsciiFrameKind::C1,
      AsciiFormat::Format3,
      PlcProfile::MelsecQnA,
      SumCheckMode::Disabled,
      host_station_route());
  const std::array<std::uint8_t, 4> response_data {'1', '2', '3', '4'};
  std::array<std::uint8_t, 32> frame {};
  std::size_t frame_size = 0;
  Status status = FrameCodec::encode_success_response(config, response_data, frame, frame_size);
  assert(status.ok());

  constexpr std::string_view expected = "\x02""00FFGG1234\x03";
  assert(frame_size == expected.size());
  assert(std::memcmp(frame.data(), expected.data(), frame_size) == 0);

  const auto decode = FrameCodec::decode_response(
      config,
      mcprotocol::serial::Span<const std::uint8_t>(frame.data(), frame_size));
  assert(decode.status == DecodeStatus::Complete);
  assert(decode.frame.type == mcprotocol::serial::ResponseType::SuccessData);
  assert(decode.frame.response_size == response_data.size());
}

void test_decode_ascii_c1_error_uses_two_digit_code() {
  const auto config = make_ascii_c1_format4_qna_config();
  std::array<std::uint8_t, 32> frame {};
  std::size_t frame_size = 0;
  Status status = FrameCodec::encode_error_response(config, 0x05U, frame, frame_size);
  assert(status.ok());

  constexpr std::string_view expected = "\x15""00FF05\r\n";
  assert(frame_size == expected.size());
  assert(std::memcmp(frame.data(), expected.data(), frame_size) == 0);

  const auto decode = FrameCodec::decode_response(
      config,
      mcprotocol::serial::Span<const std::uint8_t>(frame.data(), frame_size));
  assert(decode.status == DecodeStatus::Complete);
  assert(decode.frame.type == mcprotocol::serial::ResponseType::PlcError);
  assert(decode.frame.error_code == 0x05U);
}

void test_encode_ascii_c1_rejects_unsupported_series() {
  ProtocolConfig config = make_ascii_c1_format4_qna_config();
  config = config.with_plc_profile(PlcProfile::MelsecQ);
  const BatchReadWordsRequest request({mcprotocol::serial::DeviceCode::D, 100}, 1);
  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_batch_read_words(config, request, request_data, request_size);
  assert(!status.ok());
  assert(status.code == StatusCode::UnsupportedConfiguration);
}

void test_encode_ascii_c1_random_write_bits_qna_request_shape() {
  const auto config = make_ascii_c1_format4_qna_config();
  const std::array<RandomWriteBitItem, 3> items {{
      RandomWriteBitItem(DeviceAddress {mcprotocol::serial::DeviceCode::M, 50}, true),
      RandomWriteBitItem(DeviceAddress {mcprotocol::serial::DeviceCode::B, 0x31AU}, false),
      RandomWriteBitItem(DeviceAddress {mcprotocol::serial::DeviceCode::Y, 0x2FU}, true),
  }};
  std::array<std::uint8_t, 96> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_random_write_bits(
      config,
      mcprotocol::serial::Span<const RandomWriteBitItem>(items.data(), items.size()),
      request_data,
      request_size);
  assert(status.ok());

  constexpr std::string_view expected = "JT003M0000501B00031A0Y00002F1";
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_ascii_c1_random_write_words_qna_request_shape() {
  const auto config = make_ascii_c1_format4_qna_config();
  const std::array<RandomWriteWordItem, 3> items {{
      RandomWriteWordItem(DeviceAddress {mcprotocol::serial::DeviceCode::D, 500}, 0x1234U),
      RandomWriteWordItem(DeviceAddress {mcprotocol::serial::DeviceCode::Y, 0x100U}, 0xBCA9U),
      RandomWriteWordItem(DeviceAddress {mcprotocol::serial::DeviceCode::CN, 100}, 0x0064U),
  }};
  std::array<std::uint8_t, 128> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_random_write_words(
      config,
      mcprotocol::serial::Span<const RandomWriteWordItem>(items.data(), items.size()),
      {},
      request_data,
      request_size);
  assert(status.ok());

  constexpr std::string_view expected = "QT003D0005001234Y000100BCA9CN001000064";
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_ascii_c1_register_monitor_bits_and_read_request_shape() {
  const auto config = make_ascii_c1_format4_qna_config();
  const std::array<RandomReadWordItem, 3> items {{
      {{mcprotocol::serial::DeviceCode::X, 0x40U}},
      {{mcprotocol::serial::DeviceCode::Y, 0x60U}},
      {{mcprotocol::serial::DeviceCode::TS, 123U}},
  }};
  std::array<std::uint8_t, 96> request_data {};
  std::size_t request_size = 0;
  Status status = CommandCodec::encode_register_monitor(
      config,
      MonitorRegistration(
          mcprotocol::serial::Span<const RandomReadWordItem>(items.data(), items.size()), {}),
      request_data,
      request_size);
  assert(status.ok());

  constexpr std::string_view expected_register = "JM003X000040Y000060TS00123";
  assert(request_size == expected_register.size());
  assert(std::memcmp(request_data.data(), expected_register.data(), expected_register.size()) == 0);

  request_size = 0;
  status = CommandCodec::encode_read_monitor(
      config,
      MonitorRegistration(items, {}),
      request_data,
      request_size);
  assert(status.ok());
  constexpr std::string_view expected_read = "MJ0";
  assert(request_size == expected_read.size());
  assert(std::memcmp(request_data.data(), expected_read.data(), expected_read.size()) == 0);

  std::array<std::uint16_t, 3> values {};
  status = CommandCodec::parse_read_monitor_response(
      config,
      MonitorRegistration(items, {}),
      mcprotocol::serial::Span<const std::uint8_t>(
          reinterpret_cast<const std::uint8_t*>("101"),
          3U),
      values,
      {});
  assert(status.ok());
  assert(values[0] == 1U);
  assert(values[1] == 0U);
  assert(values[2] == 1U);
}

void test_encode_ascii_c1_register_monitor_words_and_read_request_shape() {
  const auto config = make_ascii_c1_format4_qna_config();
  const std::array<RandomReadWordItem, 4> items {{
      {{mcprotocol::serial::DeviceCode::D, 15U}},
      {{mcprotocol::serial::DeviceCode::W, 0x11EU}},
      {{mcprotocol::serial::DeviceCode::TN, 123U}},
      {{mcprotocol::serial::DeviceCode::Y, 0x60U}},
  }};
  std::array<std::uint8_t, 128> request_data {};
  std::size_t request_size = 0;
  Status status = CommandCodec::encode_register_monitor(
      config,
      MonitorRegistration(
          mcprotocol::serial::Span<const RandomReadWordItem>(items.data(), items.size()), {}),
      request_data,
      request_size);
  assert(status.ok());

  constexpr std::string_view expected_register = "QM004D000015W00011ETN00123Y000060";
  assert(request_size == expected_register.size());
  assert(std::memcmp(request_data.data(), expected_register.data(), expected_register.size()) == 0);

  request_size = 0;
  status = CommandCodec::encode_read_monitor(
      config,
      MonitorRegistration(items, {}),
      request_data,
      request_size);
  assert(status.ok());
  constexpr std::string_view expected_read = "MQ0";
  assert(request_size == expected_read.size());
  assert(std::memcmp(request_data.data(), expected_read.data(), expected_read.size()) == 0);
}

void test_client_ascii_c1_register_monitor_roundtrip() {
  struct LocalCapture {
    bool called = false;
    Status status {};
  };
  const auto local_callback = +[](void* user, Status status) {
    auto* capture = static_cast<LocalCapture*>(user);
    capture->called = true;
    capture->status = status;
  };

  const auto config = make_ascii_c1_format4_qna_config();
  MelsecSerialClient client;
  Status status = client.configure(config);
  assert(status.ok());

  const std::array<RandomReadWordItem, 3> items {{
      {{mcprotocol::serial::DeviceCode::X, 0x40U}},
      {{mcprotocol::serial::DeviceCode::Y, 0x60U}},
      {{mcprotocol::serial::DeviceCode::TS, 123U}},
  }};
  LocalCapture register_capture;
  status = client.async_register_monitor_devices(
      0,
      MonitorRegistration(
          mcprotocol::serial::Span<const RandomReadWordItem>(items.data(), items.size()), {}),
      local_callback,
      &register_capture);
  assert(status.ok());
  status = start_and_notify_tx_complete(client, 1, mcprotocol::serial::ok_status());
  assert(status.ok());

  std::array<std::uint8_t, 32> register_frame {};
  std::size_t register_frame_size = 0;
  status = FrameCodec::encode_success_response(config, {}, register_frame, register_frame_size);
  assert(status.ok());
  client.on_rx_bytes(
      2,
      mcprotocol::serial::Span<const mcprotocol::serial::Byte>(
          reinterpret_cast<const mcprotocol::serial::Byte*>(register_frame.data()),
          register_frame_size));
  assert(register_capture.called);
  assert(register_capture.status.ok());

  std::array<std::uint16_t, 3> values {};
  LocalCapture read_capture;
  status = client.async_run_monitor_cycle(10, values, {}, local_callback, &read_capture);
  assert(status.ok());
  status = start_and_notify_tx_complete(client, 11, mcprotocol::serial::ok_status());
  assert(status.ok());

  const std::array<std::uint8_t, 3> response_data {'1', '0', '1'};
  std::array<std::uint8_t, 64> response_frame {};
  std::size_t response_frame_size = 0;
  status = FrameCodec::encode_success_response(config, response_data, response_frame, response_frame_size);
  assert(status.ok());
  client.on_rx_bytes(
      12,
      mcprotocol::serial::Span<const mcprotocol::serial::Byte>(
          reinterpret_cast<const mcprotocol::serial::Byte*>(response_frame.data()),
          response_frame_size));
  assert(read_capture.called);
  assert(read_capture.status.ok());
  assert(values[0] == 1U);
  assert(values[1] == 0U);
  assert(values[2] == 1U);
}

void test_encode_ascii_c1_read_module_buffer_request_shape() {
  const auto config = make_ascii_c1_format4_a_config();
  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_read_module_buffer(
      config,
      ModuleBufferReadRequest(0x07F0U, 0x04U, 0x13U),
      request_data,
      request_size);
  assert(status.ok());

  constexpr std::string_view expected = "TR0007F00413";
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);

  const auto ana_config = config.with_plc_profile(PlcProfile::MelsecAnAAnU);
  request_size = 0U;
  Status ana_status = CommandCodec::encode_read_module_buffer(
      ana_config,
      ModuleBufferReadRequest(0x07F0U, 0x04U, 0x13U),
      request_data,
      request_size);
  assert(ana_status.ok());
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);

  const auto qna_config = config.with_plc_profile(PlcProfile::MelsecQnA);
  request_size = 99U;
  Status qna_status = CommandCodec::encode_read_module_buffer(
      qna_config,
      ModuleBufferReadRequest(0x07F0U, 0x04U, 0x13U),
      request_data,
      request_size);
  assert(qna_status.code == StatusCode::UnsupportedConfiguration);
  assert(request_size == 0U);
}

void test_encode_ascii_c1_write_module_buffer_request_shape() {
  const auto config = make_ascii_c1_format4_a_config();
  const std::array<mcprotocol::serial::Byte, 4> bytes {
      mcprotocol::serial::Byte {0xCD},
      mcprotocol::serial::Byte {0x01},
      mcprotocol::serial::Byte {0xEF},
      mcprotocol::serial::Byte {0xAB},
  };
  std::array<std::uint8_t, 96> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_write_module_buffer(
      config,
      ModuleBufferWriteRequest(0x27FAU, 0x13U, mcprotocol::serial::Span<const mcprotocol::serial::Byte>(bytes.data(), bytes.size())),
      request_data,
      request_size);
  assert(status.ok());

  constexpr std::string_view expected = "TW0027FA0413CD01EFAB";
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);

  const auto ana_config = config.with_plc_profile(PlcProfile::MelsecAnAAnU);
  request_size = 0U;
  Status ana_status = CommandCodec::encode_write_module_buffer(
      ana_config,
      ModuleBufferWriteRequest(0x27FAU, 0x13U, mcprotocol::serial::Span<const mcprotocol::serial::Byte>(bytes.data(), bytes.size())),
      request_data,
      request_size);
  assert(ana_status.ok());
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);

  const auto qna_config = config.with_plc_profile(PlcProfile::MelsecQnA);
  request_size = 99U;
  Status qna_status = CommandCodec::encode_write_module_buffer(
      qna_config,
      ModuleBufferWriteRequest(0x27FAU, 0x13U, mcprotocol::serial::Span<const mcprotocol::serial::Byte>(bytes.data(), bytes.size())),
      request_data,
      request_size);
  assert(qna_status.code == StatusCode::UnsupportedConfiguration);
  assert(request_size == 0U);
}

void test_encode_ascii_c1_loopback_request_shape() {
  const auto config = make_ascii_c1_format4_qna_config();
  constexpr std::string_view loopback = "aBcDe";
  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;
  Status status = CommandCodec::encode_loopback(config, mcprotocol::serial::Span<const char>(loopback.data(), loopback.size()), request_data, request_size);
  assert(status.ok());

  constexpr std::string_view expected_request_data = "TT005ABCDE";
  assert(request_size == expected_request_data.size());
  assert(std::memcmp(request_data.data(), expected_request_data.data(), expected_request_data.size()) == 0);

  std::array<std::uint8_t, 128> frame {};
  std::size_t frame_size = 0;
  status = FrameCodec::encode_request(
      config,
      mcprotocol::serial::Span<const std::uint8_t>(request_data.data(), request_size),
      frame,
      frame_size);
  assert(status.ok());

  constexpr std::string_view expected_frame = "\x05""00FFTT005ABCDE\r\n";
  assert(frame_size == expected_frame.size());
  assert(std::memcmp(frame.data(), expected_frame.data(), frame_size) == 0);
}

void test_decode_ascii_c1_loopback_response() {
  const auto config = make_ascii_c1_format4_qna_config();
  const std::array<std::uint8_t, 7> response_data {'0', '5', 'A', 'B', 'C', 'D', 'E'};

  std::array<std::uint8_t, 128> frame {};
  std::size_t frame_size = 0;
  Status status = FrameCodec::encode_success_response(config, response_data, frame, frame_size);
  assert(status.ok());

  const auto decode = FrameCodec::decode_response(
      config,
      mcprotocol::serial::Span<const std::uint8_t>(frame.data(), frame_size));
  assert(decode.status == DecodeStatus::Complete);

  std::array<char, 8> echoed {};
  status = CommandCodec::parse_loopback_response(
      config,
      mcprotocol::serial::Span<const std::uint8_t>(decode.frame.response_data.data(), decode.frame.response_size),
      echoed);
  assert(status.ok());
  assert(std::string_view(echoed.data(), 5) == "ABCDE");
}

void test_encode_ascii_c1_loopback_uses_internal_ff_pc_no() {
  auto config = make_ascii_c1_format4_qna_config();
  config = config.with_route(RouteConfig {C1MultidropRoute {0x00U}});
  constexpr std::string_view loopback = "ABCDE";
  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_loopback(
      config,
      mcprotocol::serial::Span<const char>(loopback.data(), loopback.size()),
      request_data,
      request_size);
  assert(status.ok());
}

void test_encode_ascii_c1_extended_file_register_read_a_request_shape() {
  const auto config = make_ascii_c1_format4_a_config();
  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_read_extended_file_register_words(
      config,
      ExtendedFileRegisterBatchReadWordsRequest(ExtendedFileRegisterAddress {12U, 8190U}, 2U),
      request_data,
      request_size);
  assert(status.ok());

  constexpr std::string_view expected = "ER012R819002";
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_ascii_c1_direct_extended_file_register_read_ana_request_shape() {
  const auto config = make_ascii_c1_format4_qna_config().with_plc_profile(PlcProfile::MelsecAnAAnU);
  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_direct_read_extended_file_register_words(
      config,
      ExtendedFileRegisterDirectBatchReadWordsRequest(16382U, 2U),
      request_data,
      request_size);
  assert(status.ok());

  constexpr std::string_view expected = "NR0001638202";
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);

  request_size = 99U;
  const Status qna_status = CommandCodec::encode_direct_read_extended_file_register_words(
      config.with_plc_profile(PlcProfile::MelsecQnA),
      ExtendedFileRegisterDirectBatchReadWordsRequest(16382U, 2U),
      request_data,
      request_size);
  assert(qna_status.code == StatusCode::UnsupportedConfiguration);
  assert(request_size == 0U);
}

void test_encode_ascii_c1_extended_file_register_write_a_request_shape() {
  const auto config = make_ascii_c1_format4_a_config();
  const std::array<std::uint16_t, 3> values {0x0123U, 0xABC7U, 0x3322U};
  std::array<std::uint8_t, 96> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_write_extended_file_register_words(
      config,
      ExtendedFileRegisterBatchWriteWordsRequest(ExtendedFileRegisterAddress {5U, 7010U}, values),
      request_data,
      request_size);
  assert(status.ok());

  constexpr std::string_view expected = "EW005R7010030123ABC73322";
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_ascii_c1_direct_extended_file_register_write_ana_request_shape() {
  const auto config = make_ascii_c1_format4_qna_config().with_plc_profile(PlcProfile::MelsecAnAAnU);
  const std::array<std::uint16_t, 3> values {0x0123U, 0xABC7U, 0x3322U};
  std::array<std::uint8_t, 96> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_direct_write_extended_file_register_words(
      config,
      ExtendedFileRegisterDirectBatchWriteWordsRequest(90110U, values),
      request_data,
      request_size);
  assert(status.ok());

  constexpr std::string_view expected = "NW00090110030123ABC73322";
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);

  request_size = 99U;
  const Status qna_status = CommandCodec::encode_direct_write_extended_file_register_words(
      config.with_plc_profile(PlcProfile::MelsecQnA),
      ExtendedFileRegisterDirectBatchWriteWordsRequest(90110U, values),
      request_data,
      request_size);
  assert(qna_status.code == StatusCode::UnsupportedConfiguration);
  assert(request_size == 0U);
}

void test_encode_ascii_c1_extended_file_register_random_write_a_request_shape() {
  const auto config = make_ascii_c1_format4_a_config();
  const std::array<ExtendedFileRegisterRandomWriteWordItem, 3> items {{
      ExtendedFileRegisterRandomWriteWordItem(ExtendedFileRegisterAddress {5U, 1050U}, 0x1234U),
      ExtendedFileRegisterRandomWriteWordItem(ExtendedFileRegisterAddress {7U, 2121U}, 0x1A1BU),
      ExtendedFileRegisterRandomWriteWordItem(ExtendedFileRegisterAddress {10U, 3210U}, 0x0506U),
  }};
  std::array<std::uint8_t, 128> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_random_write_extended_file_register_words(
      config,
      mcprotocol::serial::Span<const ExtendedFileRegisterRandomWriteWordItem>(items.data(), items.size()),
      request_data,
      request_size);
  assert(status.ok());

  constexpr std::string_view expected = "ET00305R1050123407R21211A1B10R32100506";
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_ascii_c1_extended_file_register_monitor_a_request_shape() {
  const auto config = make_ascii_c1_format4_a_config();
  const std::array<ExtendedFileRegisterAddress, 4> items {{
      ExtendedFileRegisterAddress {5U, 1234U},
      ExtendedFileRegisterAddress {6U, 2345U},
      ExtendedFileRegisterAddress {15U, 3055U},
      ExtendedFileRegisterAddress {17U, 8000U},
  }};
  std::array<std::uint8_t, 128> request_data {};
  std::size_t request_size = 0;
  Status status = CommandCodec::encode_register_extended_file_register_monitor(
      config,
      ExtendedFileRegisterMonitorRegistration(mcprotocol::serial::Span<const ExtendedFileRegisterAddress>(items.data(), items.size())),
      request_data,
      request_size);
  assert(status.ok());

  constexpr std::string_view expected_register = "EM00405R123406R234515R305517R8000";
  assert(request_size == expected_register.size());
  assert(std::memcmp(request_data.data(), expected_register.data(), expected_register.size()) == 0);

  request_size = 0;
  status = CommandCodec::encode_read_extended_file_register_monitor(
      config,
      mcprotocol::serial::Span<const ExtendedFileRegisterAddress>(items.data(), items.size()),
      request_data,
      request_size);
  assert(status.ok());
  constexpr std::string_view expected_read = "ME0";
  assert(request_size == expected_read.size());
  assert(std::memcmp(request_data.data(), expected_read.data(), expected_read.size()) == 0);

  std::array<std::uint16_t, 4> values {};
  constexpr std::array<std::uint8_t, 16> response_data {
      '3','5','0','1','4','F','5','B','0','1','6','E','0','1','6','E'
  };
  status = CommandCodec::parse_read_extended_file_register_monitor_response(
      config,
      mcprotocol::serial::Span<const ExtendedFileRegisterAddress>(items.data(), items.size()),
      mcprotocol::serial::Span<const std::uint8_t>(response_data.data(), response_data.size()),
      values);
  assert(status.ok());
  assert(values[0] == 0x3501U);
  assert(values[1] == 0x4F5BU);
  assert(values[2] == 0x016EU);
  assert(values[3] == 0x016EU);
}

void test_client_ascii_c1_extended_file_register_monitor_roundtrip() {
  struct LocalCapture {
    bool called = false;
    Status status {};
  };
  const auto local_callback = +[](void* user, Status status) {
    auto* capture = static_cast<LocalCapture*>(user);
    capture->called = true;
    capture->status = status;
  };

  const auto config = make_ascii_c1_format4_a_config();
  MelsecSerialClient client;
  Status status = client.configure(config);
  assert(status.ok());

  const std::array<ExtendedFileRegisterAddress, 2> items {{
      ExtendedFileRegisterAddress {5U, 1234U},
      ExtendedFileRegisterAddress {6U, 2345U},
  }};
  LocalCapture register_capture;
  status = client.async_register_extended_file_register_monitor(
      0,
      ExtendedFileRegisterMonitorRegistration(mcprotocol::serial::Span<const ExtendedFileRegisterAddress>(items.data(), items.size())),
      local_callback,
      &register_capture);
  assert(status.ok());
  status = start_and_notify_tx_complete(client, 1, mcprotocol::serial::ok_status());
  assert(status.ok());

  std::array<std::uint8_t, 32> register_frame {};
  std::size_t register_frame_size = 0;
  status = FrameCodec::encode_success_response(config, {}, register_frame, register_frame_size);
  assert(status.ok());
  client.on_rx_bytes(
      2,
      mcprotocol::serial::Span<const mcprotocol::serial::Byte>(
          reinterpret_cast<const mcprotocol::serial::Byte*>(register_frame.data()),
          register_frame_size));
  assert(register_capture.called);
  assert(register_capture.status.ok());

  std::array<std::uint16_t, 2> values {};
  LocalCapture read_capture;
  status = client.async_read_extended_file_register_monitor(10, values, local_callback, &read_capture);
  assert(status.ok());
  status = start_and_notify_tx_complete(client, 11, mcprotocol::serial::ok_status());
  assert(status.ok());

  const std::array<std::uint8_t, 8> response_data {'1','2','3','4','A','B','C','D'};
  std::array<std::uint8_t, 64> response_frame {};
  std::size_t response_frame_size = 0;
  status = FrameCodec::encode_success_response(config, response_data, response_frame, response_frame_size);
  assert(status.ok());
  client.on_rx_bytes(
      12,
      mcprotocol::serial::Span<const mcprotocol::serial::Byte>(
          reinterpret_cast<const mcprotocol::serial::Byte*>(response_frame.data()),
          response_frame_size));
  assert(read_capture.called);
  assert(read_capture.status.ok());
  assert(values[0] == 0x1234U);
  assert(values[1] == 0xABCDU);
}

void test_encode_ascii_format4_request_appends_crlf() {
  const auto config = make_ascii_c4_format4_config();
  const BatchReadWordsRequest request({mcprotocol::serial::DeviceCode::D, 100}, 1);

  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_data_size = 0;
  Status status = CommandCodec::encode_batch_read_words(config, request, request_data, request_data_size);
  assert(status.ok());

  std::array<std::uint8_t, 128> frame {};
  std::size_t frame_size = 0;
  status = FrameCodec::encode_request(
      config,
      mcprotocol::serial::Span<const std::uint8_t>(request_data.data(), request_data_size),
      frame,
      frame_size);
  assert(status.ok());
  assert(frame_size >= 2);
  assert(frame[0] == 0x05);
  assert(frame[frame_size - 2] == 0x0D);
  assert(frame[frame_size - 1] == 0x0A);
}

void test_decode_ascii_c2_format4_ack_response() {
  auto config = make_ascii_c2_format4_config();
  config = config.with_route(mn_multidrop_route(FrameKind::C2, 0x01U, 0x00U, 0xFFU, module_io::OwnStation, 0x00U, 0x02U));
  std::array<std::uint8_t, 32> frame {};
  std::size_t frame_size = 0;
  Status status = FrameCodec::encode_success_response(config, {}, frame, frame_size);
  assert(status.ok());

  assert(frame_size == 9U);
  assert(frame[0] == 0x06);
  assert(std::memcmp(frame.data() + 1, "FB0102", 6U) == 0);
  assert(frame[7] == 0x0D);
  assert(frame[8] == 0x0A);

  const auto decode = FrameCodec::decode_response(
      config,
      mcprotocol::serial::Span<const std::uint8_t>(frame.data(), frame_size));
  assert(decode.status == DecodeStatus::Complete);
  assert(decode.frame.type == mcprotocol::serial::ResponseType::SuccessNoData);
  assert(decode.bytes_consumed == frame_size);
}

void test_decode_ascii_c2_format4_four_digit_error_response() {
  auto config = make_ascii_c2_format4_config();
  config = test_config_with_sum_check(config, SumCheckMode::Enabled);

  constexpr std::string_view frame = "\x15""FB0100QNAK0006\r\n";
  const auto decode = FrameCodec::decode_response(
      config,
      mcprotocol::serial::Span<const std::uint8_t>(
          reinterpret_cast<const std::uint8_t*>(frame.data()),
          frame.size()));
  assert(decode.status == DecodeStatus::Complete);
  assert(decode.frame.type == mcprotocol::serial::ResponseType::PlcError);
  assert(decode.frame.error_code == 0x0006U);
  assert(decode.bytes_consumed == frame.size());
}

void test_decode_ascii_format4_ack_response() {
  const auto config = make_ascii_c4_format4_config();
  std::array<std::uint8_t, 32> frame {};
  std::size_t frame_size = 0;
  Status status = FrameCodec::encode_success_response(config, {}, frame, frame_size);
  assert(status.ok());

  const auto decode = FrameCodec::decode_response(
      config,
      mcprotocol::serial::Span<const std::uint8_t>(frame.data(), frame_size));
  assert(decode.status == DecodeStatus::Complete);
  assert(decode.frame.type == mcprotocol::serial::ResponseType::SuccessNoData);
  assert(decode.bytes_consumed == frame_size);
}

void test_high_level_parse_device_address() {
  mcprotocol::serial::DeviceAddress address(mcprotocol::serial::DeviceCode::D, 0U);
  Status status = parse_device_address("D100", address);
  assert(status.ok());
  assert(address.code == mcprotocol::serial::DeviceCode::D);
  assert(address.number == 100U);

  status = parse_device_address("X1A", address);
  assert(status.ok());
  assert(address.code == mcprotocol::serial::DeviceCode::X);
  assert(address.number == 0x1AU);

  status = parse_device_address("XFF", address);
  assert(status.ok());
  assert(address.code == mcprotocol::serial::DeviceCode::X);
  assert(address.number == 0xFFU);

  status = parse_device_address("SM100", address);
  assert(status.ok());
  assert(address.code == mcprotocol::serial::DeviceCode::SM);
  assert(address.number == 100U);

  status = parse_device_address("SWFF", address);
  assert(status.ok());
  assert(address.code == mcprotocol::serial::DeviceCode::SW);
  assert(address.number == 0xFFU);

  status = parse_device_address("SD200", address);
  assert(status.ok());
  assert(address.code == mcprotocol::serial::DeviceCode::SD);
  assert(address.number == 200U);

  status = parse_device_address("LZ10", address);
  assert(status.ok());
  assert(address.code == mcprotocol::serial::DeviceCode::LZ);
  assert(address.number == 10U);

  status = parse_device_address("RD20", address);
  assert(status.ok());
  assert(address.code == mcprotocol::serial::DeviceCode::RD);
  assert(address.number == 20U);

  status = parse_device_address("LTN0", address);
  assert(status.ok());
  assert(address.code == mcprotocol::serial::DeviceCode::LTN);
  assert(address.number == 0U);

  status = parse_device_address("LSTN1", address);
  assert(status.ok());
  assert(address.code == mcprotocol::serial::DeviceCode::LSTN);
  assert(address.number == 1U);

  status = parse_device_address("LCN2", address);
  assert(status.ok());
  assert(address.code == mcprotocol::serial::DeviceCode::LCN);
  assert(address.number == 2U);

  status = parse_device_address("LTS3", address);
  assert(status.ok());
  assert(address.code == mcprotocol::serial::DeviceCode::LTS);
  assert(address.number == 3U);

  status = parse_device_address("LTC4", address);
  assert(status.ok());
  assert(address.code == mcprotocol::serial::DeviceCode::LTC);
  assert(address.number == 4U);

  status = parse_device_address("LSTS5", address);
  assert(status.ok());
  assert(address.code == mcprotocol::serial::DeviceCode::LSTS);
  assert(address.number == 5U);

  status = parse_device_address("LSTC6", address);
  assert(status.ok());
  assert(address.code == mcprotocol::serial::DeviceCode::LSTC);
  assert(address.number == 6U);

  status = parse_device_address("LCS7", address);
  assert(status.ok());
  assert(address.code == mcprotocol::serial::DeviceCode::LCS);
  assert(address.number == 7U);

  status = parse_device_address("LCC8", address);
  assert(status.ok());
  assert(address.code == mcprotocol::serial::DeviceCode::LCC);
  assert(address.number == 8U);

  status = parse_device_address("ZR10", address);
  assert(status.ok());
  assert(address.code == mcprotocol::serial::DeviceCode::ZR);
  assert(address.number == 10U);

  status = parse_device_address("G100", address);
  assert(!status.ok());
  assert(status.code == StatusCode::InvalidArgument);

  status = parse_device_address("HG100", address);
  assert(!status.ok());
  assert(status.code == StatusCode::InvalidArgument);

  status = parse_device_address("", address);
  assert(!status.ok());
  assert(status.code == StatusCode::InvalidArgument);

  status = parse_device_address("DFFFF", address);
  assert(!status.ok());
  assert(status.code == StatusCode::InvalidArgument);
  assert(std::strcmp(status.message, "Device address number is invalid") == 0);

  status = parse_device_address("D4294967296", address);
  assert(!status.ok());
  assert(status.code == StatusCode::InvalidArgument);

  status = parse_device_address("SW100000000", address);
  assert(!status.ok());
  assert(status.code == StatusCode::InvalidArgument);
}

void test_parse_link_direct_device_surface() {
  LinkDirectDevice device(0U, DeviceAddress {mcprotocol::serial::DeviceCode::D, 0U});
  Status status = parse_link_direct_device("J1\\W100", device);
  assert(status.ok());
  assert(device.network_number == 0x0001U);
  assert(device.device.code == mcprotocol::serial::DeviceCode::W);
  assert(device.device.number == 0x0100U);

  status = parse_link_direct_device("J00A\\X10", device);
  assert(status.ok());
  assert(device.network_number == 0x000AU);
  assert(device.device.code == mcprotocol::serial::DeviceCode::X);
  assert(device.device.number == 0x0010U);

  status = parse_link_direct_device("J1\\D100", device);
  assert(!status.ok());

  status = parse_link_direct_device("J100000000\\W10", device);
  assert(!status.ok());
}

void test_parse_qualified_buffer_word_device_rejects_overflow() {
  QualifiedBufferWordDevice device(QualifiedBufferDeviceKind::G, 0U, 0U);
  Status status = parse_qualified_buffer_word_device("U100000000\\G0", device);
  assert(!status.ok());
  assert(status.code == StatusCode::InvalidArgument);

  status = parse_qualified_buffer_word_device("U3E0\\G4294967296", device);
  assert(!status.ok());
  assert(status.code == StatusCode::InvalidArgument);
}

void test_high_level_make_contiguous_requests() {
  BatchReadWordsRequest read_request(
      DeviceAddress {mcprotocol::serial::DeviceCode::D, 0U}, 0U);
  Status status = make_batch_read_words_request("D100", 2U, read_request);
  assert(status.ok());
  assert(read_request.head_device.code == mcprotocol::serial::DeviceCode::D);
  assert(read_request.head_device.number == 100U);
  assert(read_request.points == 2U);

  const std::array<BitValue, 2> bits {true, false};
  BatchWriteBitsRequest write_request(
      DeviceAddress {mcprotocol::serial::DeviceCode::M, 0U}, {});
  status = make_batch_write_bits_request("M100", bits, write_request);
  assert(status.ok());
  assert(write_request.head_device.code == mcprotocol::serial::DeviceCode::M);
  assert(write_request.head_device.number == 100U);
  assert(write_request.bits.size() == 2U);
}

void test_high_level_protocol_presets() {
  static_assert(!HasAsciiBlockNumberMember<ProtocolConfig>);
  static_assert(!std::is_default_constructible_v<ProtocolConfig>);

  const ProtocolConfig config =
      make_c4_binary_protocol(
          PlcProfile::MelsecQ,
          SumCheckMode::Enabled,
          host_station_route());
  assert(config.frame_kind() == FrameKind::C4);
  assert(config.code_mode() == CodeMode::Binary);
  assert(!config.has_ascii_format());
  assert(config.plc_profile() == PlcProfile::MelsecQ);
  assert(config.sum_check_mode() == SumCheckMode::Enabled);
  assert(config.route().is_host_station());
  assert(config.route().station_no() == 0x00);

  const ProtocolConfig ascii_config =
      mcprotocol::serial::highlevel::make_c4_ascii_format2_protocol(
          PlcProfile::MelsecQ,
          SumCheckMode::Enabled,
          host_station_route());
  assert(ascii_config.frame_kind() == FrameKind::C4);
  assert(ascii_config.code_mode() == CodeMode::Ascii);
  assert(ascii_config.has_ascii_format());
  assert(ascii_config.ascii_format() == AsciiFormat::Format2);

  assert(ascii_config.sum_check_mode() == SumCheckMode::Enabled);
}

void test_response_timeout_contract() {
  static_assert(mcprotocol::serial::TimeoutConfig {}.response_timeout_ms == 3000U);
  static_assert(
      make_c4_binary_protocol(
          PlcProfile::MelsecQ,
          SumCheckMode::Enabled,
          RouteConfig {HostStationRoute {}})
          .timeout().response_timeout_ms == 3000U);
  static_assert(
      make_c4_ascii_format4_protocol(
          PlcProfile::MelsecQ,
          SumCheckMode::Disabled,
          RouteConfig {HostStationRoute {}})
          .timeout().response_timeout_ms == 3000U);

  auto config = make_binary_c4_config();
  for (const std::uint32_t valid_timeout : {1U, 3000U, 0x7FFFFFFFU}) {
    config = config.with_response_timeout_ms(valid_timeout);
    assert(FrameCodec::validate_config(config).ok());
  }
  for (const std::uint32_t invalid_timeout : {0U, 0x80000000U, 0xFFFFFFFFU}) {
    config = config.with_response_timeout_ms(invalid_timeout);
    const Status status = FrameCodec::validate_config(config);
    assert(!status.ok());
    assert(status.code == StatusCode::InvalidArgument);
    std::array<std::uint8_t, 32> frame {};
    std::size_t frame_size = 99U;
    const Status encode_status = FrameCodec::encode_request(
        config,
        mcprotocol::serial::Span<const std::uint8_t> {},
        frame,
        frame_size);
    assert(!encode_status.ok());
    assert(frame_size == 0U);
  }

}

void test_inter_byte_timeout_contract_and_candidate_progress() {
  struct LocalCapture {
    bool called = false;
    Status status {};
  };
  const auto completion = [](void* user, Status status) noexcept {
    auto* capture = static_cast<LocalCapture*>(user);
    capture->called = true;
    capture->status = status;
  };

  static_assert(mcprotocol::serial::TimeoutConfig {}.inter_byte_timeout_ms == 250U);
  static_assert(
      make_c4_binary_protocol(
          PlcProfile::MelsecQ,
          SumCheckMode::Enabled,
          RouteConfig {HostStationRoute {}})
          .timeout().inter_byte_timeout_ms == 250U);

  auto config = make_binary_c4_config();
  for (const std::uint32_t value : {1U, 250U, 0x7FFFFFFFU}) {
    assert(FrameCodec::validate_config(config.with_inter_byte_timeout_ms(value)).ok());
  }
  for (const std::uint32_t value : {0U, 0x80000000U, 0xFFFFFFFFU}) {
    const Status invalid = FrameCodec::validate_config(config.with_inter_byte_timeout_ms(value));
    assert(invalid.code == StatusCode::InvalidArgument);
  }

  config = config.with_response_timeout_ms(1000U).with_inter_byte_timeout_ms(250U);
  const BatchReadWordsRequest request(
      DeviceAddress {mcprotocol::serial::DeviceCode::D, 100U}, 1U);
  const std::array<std::uint8_t, 2> response_data {0x34U, 0x12U};
  std::array<std::uint8_t, 64> response_frame {};
  std::size_t response_frame_size = 0U;
  assert(FrameCodec::encode_success_response(
             config, response_data, response_frame, response_frame_size)
             .ok());
  assert(response_frame_size > 2U);

  const auto begin_read = [&](MelsecSerialClient& client,
                              std::array<std::uint16_t, 1>& words,
                              LocalCapture& capture,
                              std::uint32_t now_ms) {
    assert(client.configure(config).ok());
    assert(client.async_batch_read_words(
                     now_ms, request, words, completion, &capture)
               .ok());
    assert(start_and_notify_tx_complete(client, now_ms, mcprotocol::serial::ok_status()).ok());
  };

  // No candidate has arrived: the inter-byte timeout is not an alternate response-start timer.
  {
    MelsecSerialClient client;
    std::array<std::uint16_t, 1> words {};
    LocalCapture capture;
    begin_read(client, words, capture, 0U);
    client.poll(250U);
    assert(!capture.called);

    const std::array<mcprotocol::serial::Byte, 1> noise {
        static_cast<mcprotocol::serial::Byte>(0x55U)};
    client.on_rx_bytes(300U, noise);
    client.poll(550U);
    assert(!capture.called);
    client.on_rx_bytes(
        600U,
        mcprotocol::serial::Span<const mcprotocol::serial::Byte>(
            reinterpret_cast<const mcprotocol::serial::Byte*>(response_frame.data()),
            response_frame_size));
    assert(capture.called);
    assert(capture.status.ok());
    assert(words[0] == 0x1234U);
  }

  // A retained partial candidate starts the inactivity timer, and progress restarts only it.
  {
    MelsecSerialClient client;
    std::array<std::uint16_t, 1> words {};
    LocalCapture capture;
    begin_read(client, words, capture, 0U);
    client.on_rx_bytes(
        100U,
        mcprotocol::serial::Span<const mcprotocol::serial::Byte>(
            reinterpret_cast<const mcprotocol::serial::Byte*>(response_frame.data()), 1U));
    client.poll(349U);
    assert(!capture.called);
    client.on_rx_bytes(
        349U,
        mcprotocol::serial::Span<const mcprotocol::serial::Byte>(
            reinterpret_cast<const mcprotocol::serial::Byte*>(response_frame.data() + 1U), 1U));
    client.poll(598U);
    assert(!capture.called);
    client.on_rx_bytes(
        599U,
        mcprotocol::serial::Span<const mcprotocol::serial::Byte>(
            reinterpret_cast<const mcprotocol::serial::Byte*>(response_frame.data() + 2U),
            response_frame_size - 2U));
    assert(capture.called);
    assert(capture.status.code == StatusCode::Timeout);
    assert(client.requires_transport_reset());
  }

  // The absolute deadline remains earlier when it expires before the restarted inactivity timer.
  {
    const ProtocolConfig short_total =
        config.with_response_timeout_ms(100U).with_inter_byte_timeout_ms(250U);
    MelsecSerialClient client;
    std::array<std::uint16_t, 1> words {};
    LocalCapture capture;
    assert(client.configure(short_total).ok());
    assert(client.async_batch_read_words(0U, request, words, completion, &capture).ok());
    assert(start_and_notify_tx_complete(client, 0U, mcprotocol::serial::ok_status()).ok());
    client.on_rx_bytes(
        10U,
        mcprotocol::serial::Span<const mcprotocol::serial::Byte>(
            reinterpret_cast<const mcprotocol::serial::Byte*>(response_frame.data()), 1U));
    client.poll(100U);
    assert(capture.called);
    assert(capture.status.code == StatusCode::Timeout);
  }

  // Both deadline calculations remain correct across uint32_t wrap.
  {
    const ProtocolConfig wrap_config =
        config.with_response_timeout_ms(1000U).with_inter_byte_timeout_ms(10U);
    MelsecSerialClient client;
    std::array<std::uint16_t, 1> words {};
    LocalCapture capture;
    assert(client.configure(wrap_config).ok());
    assert(client.async_batch_read_words(
                     0xFFFFFF00U, request, words, completion, &capture)
               .ok());
    assert(start_and_notify_tx_complete(
               client, 0xFFFFFF00U, mcprotocol::serial::ok_status())
               .ok());
    client.on_rx_bytes(
        0xFFFFFFFAU,
        mcprotocol::serial::Span<const mcprotocol::serial::Byte>(
            reinterpret_cast<const mcprotocol::serial::Byte*>(response_frame.data()), 1U));
    client.poll(3U);
    assert(!capture.called);
    client.poll(4U);
    assert(capture.called);
    assert(capture.status.code == StatusCode::Timeout);
  }

  // A partial response to an already-transmitted state change keeps the structured ambiguity.
  {
    MelsecSerialClient client;
    LocalCapture capture;
    const std::array<std::uint16_t, 1> values {0x1234U};
    const BatchWriteWordsRequest write_request(
        DeviceAddress {mcprotocol::serial::DeviceCode::D, 100U}, values);
    assert(client.configure(config).ok());
    assert(client.async_batch_write_words(
                     0U, write_request, completion, &capture)
               .ok());
    assert(start_and_notify_tx_complete(client, 0U, mcprotocol::serial::ok_status()).ok());

    std::array<std::uint8_t, 64> write_response {};
    std::size_t write_response_size = 0U;
    assert(FrameCodec::encode_success_response(
               config,
               mcprotocol::serial::Span<const std::uint8_t> {},
               write_response,
               write_response_size)
               .ok());
    client.on_rx_bytes(
        100U,
        mcprotocol::serial::Span<const mcprotocol::serial::Byte>(
            reinterpret_cast<const mcprotocol::serial::Byte*>(write_response.data()), 1U));
    client.poll(350U);
    assert(capture.called);
    assert(capture.status.code == StatusCode::OperationOutcomeUnknown);
    assert(capture.status.cause == StatusCode::Timeout);
    assert(client.requires_transport_reset());
  }
}

void test_invalid_reconfigure_preserves_previous_validated_config() {
  ProtocolConfig valid = make_binary_c4_config()
                             .with_response_timeout_ms(100U)
                             .with_inter_byte_timeout_ms(250U);
  MelsecSerialClient client;
  assert(client.configure(valid).ok());

  const ProtocolConfig invalid = valid.with_response_timeout_ms(200U)
                                     .with_inter_byte_timeout_ms(0U);
  const Status configure_status = client.configure(invalid);
  assert(configure_status.code == StatusCode::InvalidArgument);

  std::array<std::uint16_t, 1> words {};
  const BatchReadWordsRequest request(
      DeviceAddress {mcprotocol::serial::DeviceCode::D, 100U}, 1U);
  assert(client.async_batch_read_words(
                   0U,
                   request,
                   words,
                   [](void*, Status) {},
                   nullptr)
             .ok());
  assert(client.notify_tx_started(10U).ok());
  assert(client.transaction_deadline_ms() == 110U);
  client.cancel();
}

void test_tx_drain_wait_policy_yields_before_each_bounded_sleep() {
  using mcprotocol::serial::detail::YieldFirstWaitAction;
  using mcprotocol::serial::detail::YieldFirstWaitPolicy;

  YieldFirstWaitPolicy policy;
  assert(policy.observe(8U) == YieldFirstWaitAction::Yield);
  assert(policy.observe(8U) == YieldFirstWaitAction::SleepOneMillisecond);
  assert(policy.observe(8U) == YieldFirstWaitAction::Yield);
  assert(policy.observe(8U) == YieldFirstWaitAction::SleepOneMillisecond);

  // Queue progress resets the phase even when the preceding observation slept.
  assert(policy.observe(4U) == YieldFirstWaitAction::Yield);
  assert(policy.observe(4U) == YieldFirstWaitAction::SleepOneMillisecond);
  assert(policy.observe(1U) == YieldFirstWaitAction::Yield);
}

struct DrainSimulation {
  std::vector<std::uint32_t> remaining_values;
  std::vector<std::uint64_t> pending_values;
  Status query_status = mcprotocol::serial::ok_status();
  std::size_t remaining_calls = 0U;
  std::size_t query_calls = 0U;
  std::size_t yield_calls = 0U;
  std::vector<std::uint32_t> sleep_durations;

  Status run() {
    auto remaining = [this](std::uint32_t) noexcept -> std::uint32_t {
      const std::size_t selected = std::min(remaining_calls, remaining_values.size() - 1U);
      ++remaining_calls;
      return remaining_values[selected];
    };
    auto query = [this](std::uint64_t& out_pending) noexcept -> Status {
      const std::size_t selected = std::min(query_calls, pending_values.size() - 1U);
      ++query_calls;
      out_pending = pending_values[selected];
      return query_status;
    };
    auto yield_now = [this]() noexcept { ++yield_calls; };
    auto sleep_one = [this](std::uint32_t duration) noexcept {
      sleep_durations.push_back(duration);
    };
    return mcprotocol::serial::detail::drain_tx_with_yield_first(
        123U, "simulated drain timeout", remaining, query, yield_now, sleep_one);
  }
};

void test_tx_drain_loop_boundaries_failures_and_simulated_delay() {
  {
    DrainSimulation immediate {{5U, 5U, 5U, 5U}, {8U, 0U}};
    assert(immediate.run().ok());
    assert(immediate.query_calls == 2U);
    assert(immediate.yield_calls == 1U);
    assert(immediate.sleep_durations.empty());
    // The replaced fixed-sleep loop deliberately waited 1 ms after the first non-empty query.
    // For this same immediate-completion sequence the approved loop adds 0 ms deliberate sleep.
    constexpr std::uint32_t old_deliberate_sleep_ms = 1U;
    const std::uint32_t new_deliberate_sleep_ms = 0U;
    assert(new_deliberate_sleep_ms < old_deliberate_sleep_ms);
  }
  {
    DrainSimulation stalled {{5U, 5U, 5U, 5U, 5U, 5U}, {8U, 8U, 0U}};
    assert(stalled.run().ok());
    assert(stalled.query_calls == 3U);
    assert(stalled.yield_calls == 1U);
    assert((stalled.sleep_durations == std::vector<std::uint32_t> {1U}));
  }
  {
    DrainSimulation progress {{5U, 5U, 5U, 5U, 5U, 5U, 5U, 5U}, {8U, 4U, 4U, 0U}};
    assert(progress.run().ok());
    assert(progress.yield_calls == 2U);
    assert((progress.sleep_durations == std::vector<std::uint32_t> {1U}));
  }
  {
    DrainSimulation expired_before_query {{0U}, {0U}};
    const Status status = expired_before_query.run();
    assert(status.code == StatusCode::Timeout);
    assert(expired_before_query.query_calls == 0U);
  }
  {
    DrainSimulation expired_during_query {{1U, 0U}, {0U}};
    const Status status = expired_during_query.run();
    assert(status.code == StatusCode::Timeout);
    assert(expired_during_query.query_calls == 1U);
  }
  {
    DrainSimulation query_failure {{5U}, {8U}};
    query_failure.query_status =
        mcprotocol::serial::make_status(StatusCode::Transport, "simulated queue query failure");
    const Status status = query_failure.run();
    assert(status.code == StatusCode::Transport);
    assert(query_failure.yield_calls == 0U);
    assert(query_failure.sleep_durations.empty());
  }
  {
    DrainSimulation bounded_sleep {{2U, 2U, 1U, 1U, 0U}, {8U, 8U}};
    const Status status = bounded_sleep.run();
    assert(status.code == StatusCode::Timeout);
    assert((bounded_sleep.sleep_durations == std::vector<std::uint32_t> {1U}));
  }
}

void test_plc_profile_names_and_internal_grouping() {
  PlcProfile profile = PlcProfile::MelsecQ;
  assert(std::string_view(plc_profile_name(PlcProfile::Unspecified)).empty());
  assert(std::string_view(plc_profile_display_name(PlcProfile::Unspecified)).empty());
  assert(!is_plc_profile_specified(PlcProfile::Unspecified));
  assert(!is_plc_profile_specified(static_cast<PlcProfile>(0xFE)));
  assert(plc_series_from_profile(PlcProfile::Unspecified) == PlcSeries::Unspecified);
  assert(is_plc_profile_specified(PlcProfile::MelsecQ));

  assert(parse_plc_profile_text("melsec:iq-r", profile));
  assert(profile == PlcProfile::MelsecIqR);
  assert(plc_series_from_profile(profile) == PlcSeries::IQ_R);
  assert(std::string_view(plc_profile_name(profile)) == "melsec:iq-r");
  assert(std::string_view(plc_profile_display_name(profile)) == "MELSEC iQ-R");
  assert(!parse_plc_profile_text("MELSEC:IQ-R", profile));

  assert(parse_plc_profile_text("melsec:iq-l", profile));
  assert(profile == PlcProfile::MelsecIqL);
  assert(plc_series_from_profile(profile) == PlcSeries::IQ_L);
  assert(std::string_view(plc_profile_name(profile)) == "melsec:iq-l");
  assert(std::string_view(plc_profile_display_name(profile)) == "MELSEC iQ-L");

  assert(parse_plc_profile_text("melsec:iq-f", profile));
  assert(profile == PlcProfile::MelsecIqF);
  assert(plc_series_from_profile(profile) == PlcSeries::IQ_F);
  assert(std::string_view(plc_profile_name(profile)) == "melsec:iq-f");
  assert(std::string_view(plc_profile_display_name(profile)) == "MELSEC iQ-F");

  assert(parse_plc_profile_text("melsec:qcpu", profile));
  assert(profile == PlcProfile::MelsecQ);
  assert(plc_series_from_profile(profile) == PlcSeries::Q_L);
  assert(std::string_view(plc_profile_name(profile)) == "melsec:qcpu");
  assert(std::string_view(plc_profile_display_name(profile)) == "MELSEC-Q");

  assert(!parse_plc_profile_text("melsec:q", profile));
  assert(parse_plc_profile_text("melsec:lcpu", profile));
  assert(profile == PlcProfile::MelsecL);
  assert(plc_series_from_profile(profile) == PlcSeries::Q_L);
  assert(std::string_view(plc_profile_name(profile)) == "melsec:lcpu");
  assert(std::string_view(plc_profile_display_name(profile)) == "MELSEC-L");

  assert(!parse_plc_profile_text("melsec:l", profile));
  assert(!parse_plc_profile_text("melsec:q-l", profile));

  assert(parse_plc_profile_text("melsec:qna", profile));
  assert(profile == PlcProfile::MelsecQnA);
  assert(plc_series_from_profile(profile) == PlcSeries::QnA);
  assert(std::string_view(plc_profile_name(profile)) == "melsec:qna");
  assert(std::string_view(plc_profile_display_name(profile)) == "MELSEC QnA");

  assert(parse_plc_profile_text("melsec:ana-anu", profile));
  assert(profile == PlcProfile::MelsecAnAAnU);
  assert(plc_series_from_profile(profile) == PlcSeries::QnA);
  assert(std::string_view(plc_profile_name(profile)) == "melsec:ana-anu");
  assert(std::string_view(plc_profile_display_name(profile)) == "MELSEC AnA/AnU");

  assert(parse_plc_profile_text("melsec:a", profile));
  assert(profile == PlcProfile::MelsecA);
  assert(plc_series_from_profile(profile) == PlcSeries::A);
  assert(std::string_view(plc_profile_name(profile)) == "melsec:a");
  assert(std::string_view(plc_profile_display_name(profile)) == "MELSEC-A");

  assert(!parse_plc_profile_text("iqr", profile));
  assert(!parse_plc_profile_text("iq-r", profile));
  assert(!parse_plc_profile_text("iq-f", profile));
  assert(!parse_plc_profile_text("q", profile));
  assert(!parse_plc_profile_text("l", profile));
  assert(!parse_plc_profile_text("lcpu", profile));
  assert(!parse_plc_profile_text("ql", profile));
  assert(!parse_plc_profile_text("qna", profile));
}

void test_plc_profile_is_required_for_encoding() {
  const ProtocolConfig config = ProtocolConfig::c4_binary(PlcProfile::Unspecified,
      SumCheckMode::Enabled,
      host_station_route());
  BatchReadWordsRequest request(
      mcprotocol::serial::DeviceAddress {mcprotocol::serial::DeviceCode::D, 100U}, 1U);

  std::array<std::uint8_t, mcprotocol::serial::kMaxRequestDataBytes> request_data {};
  std::size_t request_size = 0;
  Status status = CommandCodec::encode_batch_read_words(config, request, request_data, request_size);
  assert(status.code == StatusCode::InvalidArgument);
  assert(std::strcmp(
             status.message,
             "PLC profile is required. Set ProtocolConfig::plc_profile to an explicit canonical profile.") == 0);

  std::array<std::uint8_t, mcprotocol::serial::kMaxRequestFrameBytes> frame {};
  std::size_t frame_size = 0;
  status = FrameCodec::encode_request(config, {}, frame, frame_size);
  assert(status.code == StatusCode::InvalidArgument);
  assert(std::strcmp(
             status.message,
             "PLC profile is required. Set ProtocolConfig::plc_profile to an explicit canonical profile.") == 0);
}

void test_high_level_make_random_bit_item() {
  RandomWriteBitItem item(
      DeviceAddress {mcprotocol::serial::DeviceCode::M, 0U}, false);
  Status status = make_random_write_bit_item("M105", true, item);
  assert(status.ok());
  assert(item.device.code == mcprotocol::serial::DeviceCode::M);
  assert(item.device.number == 105U);
  assert(item.value == true);
}

void test_high_level_make_random_dword_item_defaults() {
  RandomReadDWordItem read_item {DeviceAddress {mcprotocol::serial::DeviceCode::D, 0U}};
  Status status = make_random_read_dword_item("LZ1", read_item);
  assert(status.ok());
  assert(read_item.device.code == mcprotocol::serial::DeviceCode::LZ);
  assert(read_item.device.number == 1U);

  RandomWriteDWordItem write_item(
      DeviceAddress {mcprotocol::serial::DeviceCode::D, 0U}, 0U);
  status = make_random_write_dword_item("LZ0", 0x12345678U, write_item);
  assert(status.ok());
  assert(write_item.device.code == mcprotocol::serial::DeviceCode::LZ);
  assert(write_item.device.number == 0U);
  assert(write_item.value == 0x12345678U);

  status = make_random_read_dword_item("LTN1", read_item);
  assert(status.ok());
  assert(read_item.device.code == mcprotocol::serial::DeviceCode::LTN);
  assert(read_item.device.number == 1U);

  status = make_random_read_dword_item("LSTN2", read_item);
  assert(status.ok());
  assert(read_item.device.code == mcprotocol::serial::DeviceCode::LSTN);
  assert(read_item.device.number == 2U);

  status = make_random_write_dword_item("LCN3", 0x12345678U, write_item);
  assert(status.ok());
  assert(write_item.device.code == mcprotocol::serial::DeviceCode::LCN);
  assert(write_item.device.number == 3U);
}

void test_high_level_make_random_request_from_specs() {
  const std::array<RandomReadWordSpec, 1> word_specs {{RandomReadWordSpec("D100")}};
  const std::array<RandomReadDWordSpec, 2> dword_specs {{
      RandomReadDWordSpec("LZ0"),
      RandomReadDWordSpec("LTN1"),
  }};
  auto word_items = mcprotocol::serial::detail::make_filled_array<RandomReadWordItem, 1>(
      RandomReadWordItem {DeviceAddress {mcprotocol::serial::DeviceCode::D, 0U}});
  auto dword_items = mcprotocol::serial::detail::make_filled_array<RandomReadDWordItem, 2>(
      RandomReadDWordItem {DeviceAddress {mcprotocol::serial::DeviceCode::D, 0U}});
  RandomReadRequest request({}, {});
  Status status = make_random_read_request(
      word_specs, dword_specs, word_items, dword_items, request);
  assert(status.ok());
  assert(request.word_items.size() == 1U);
  assert(request.dword_items.size() == 2U);
  assert(request.word_items[0].device.code == mcprotocol::serial::DeviceCode::D);
  assert(request.dword_items[0].device.code == mcprotocol::serial::DeviceCode::LZ);
  assert(request.dword_items[1].device.code == mcprotocol::serial::DeviceCode::LTN);

  auto too_small = mcprotocol::serial::detail::make_filled_array<RandomReadDWordItem, 1>(
      RandomReadDWordItem {DeviceAddress {mcprotocol::serial::DeviceCode::D, 0U}});
  status = make_random_read_request(word_specs, dword_specs, word_items, too_small, request);
  assert(!status.ok());
  assert(status.code == StatusCode::BufferTooSmall);
}

void test_high_level_make_random_write_items_from_specs() {
  const std::array<RandomWriteWordSpec, 1> word_specs {{
      RandomWriteWordSpec("D100", 0x1234U),
  }};
  auto word_items = mcprotocol::serial::detail::make_filled_array<RandomWriteWordItem, 1>(
      RandomWriteWordItem(DeviceAddress {mcprotocol::serial::DeviceCode::D, 0U}, 0U));
  mcprotocol::serial::Span<const RandomWriteWordItem> word_view {};
  Status status = make_random_write_word_items(word_specs, word_items, word_view);
  assert(status.ok());
  assert(word_view.size() == 1U);
  assert(word_view[0].device.code == mcprotocol::serial::DeviceCode::D);
  assert(word_view[0].value == 0x1234U);

  const std::array<RandomWriteDWordSpec, 1> dword_specs {{
      RandomWriteDWordSpec("LZ0", 0x12345678U),
  }};
  auto dword_items = mcprotocol::serial::detail::make_filled_array<RandomWriteDWordItem, 1>(
      RandomWriteDWordItem(DeviceAddress {mcprotocol::serial::DeviceCode::D, 0U}, 0U));
  mcprotocol::serial::Span<const RandomWriteDWordItem> dword_view {};
  status = make_random_write_dword_items(dword_specs, dword_items, dword_view);
  assert(status.ok());
  assert(dword_view[0].device.code == mcprotocol::serial::DeviceCode::LZ);
  assert(dword_view[0].value == 0x12345678U);

  const std::array<RandomWriteBitSpec, 2> bit_specs {{
      RandomWriteBitSpec("M100", true),
      RandomWriteBitSpec("Y2F", false),
  }};
  auto bit_items = mcprotocol::serial::detail::make_filled_array<RandomWriteBitItem, 2>(
      RandomWriteBitItem(DeviceAddress {mcprotocol::serial::DeviceCode::M, 0U}, false));
  mcprotocol::serial::Span<const RandomWriteBitItem> bit_view {};
  status = make_random_write_bit_items(bit_specs, bit_items, bit_view);
  assert(status.ok());
  assert(bit_view.size() == bit_specs.size());
  assert(bit_view[0].device.code == mcprotocol::serial::DeviceCode::M);
  assert(bit_view[0].value == true);
  assert(bit_view[1].device.code == mcprotocol::serial::DeviceCode::Y);
  assert(bit_view[1].device.number == 0x2FU);
  assert(bit_view[1].value == false);
}

void test_high_level_make_monitor_registration_from_specs() {
  const std::array<RandomReadWordSpec, 1> word_specs {{RandomReadWordSpec("D100")}};
  const std::array<RandomReadDWordSpec, 1> dword_specs {{RandomReadDWordSpec("LZ0")}};
  auto word_items = mcprotocol::serial::detail::make_filled_array<RandomReadWordItem, 1>(
      RandomReadWordItem {DeviceAddress {mcprotocol::serial::DeviceCode::D, 0U}});
  auto dword_items = mcprotocol::serial::detail::make_filled_array<RandomReadDWordItem, 1>(
      RandomReadDWordItem {DeviceAddress {mcprotocol::serial::DeviceCode::D, 0U}});
  MonitorRegistration request({}, {});
  Status status = make_monitor_registration(
      word_specs, dword_specs, word_items, dword_items, request);
  assert(status.ok());
  assert(request.word_items[0].device.code == mcprotocol::serial::DeviceCode::D);
  assert(request.dword_items[0].device.code == mcprotocol::serial::DeviceCode::LZ);
}

void test_high_level_long_state_read_spec_and_decode() {
  LongStateReadSpec spec(
      LongStateReadRoute::StatusBlock,
      mcprotocol::serial::DeviceCode::LTN,
      LongStateReadKind::Contact);
  Status status = get_long_state_read_spec(mcprotocol::serial::DeviceCode::LTS, spec);
  assert(status.ok());
  assert(spec.route == LongStateReadRoute::StatusBlock);
  assert(spec.base_code == mcprotocol::serial::DeviceCode::LTN);
  assert(spec.kind == LongStateReadKind::Contact);

  std::array<std::uint16_t, 4> words {{0x1234U, 0x0000U, 0x0002U, 0x0000U}};
  BitValue value = false;
  status = decode_long_state_bit(spec, mcprotocol::serial::Span<const std::uint16_t>(words.data(), words.size()), value);
  assert(status.ok());
  assert(value == true);

  status = get_long_state_read_spec(mcprotocol::serial::DeviceCode::LTC, spec);
  assert(status.ok());
  assert(spec.route == LongStateReadRoute::StatusBlock);
  assert(spec.base_code == mcprotocol::serial::DeviceCode::LTN);
  assert(spec.kind == LongStateReadKind::Coil);
  status = decode_long_state_bit(spec, mcprotocol::serial::Span<const std::uint16_t>(words.data(), words.size()), value);
  assert(status.ok());
  assert(value == false);

  words[2] = 0x0001U;
  status = decode_long_state_bit(spec, mcprotocol::serial::Span<const std::uint16_t>(words.data(), words.size()), value);
  assert(status.ok());
  assert(value == true);

  status = get_long_state_read_spec(mcprotocol::serial::DeviceCode::LCS, spec);
  assert(status.ok());
  assert(spec.route == LongStateReadRoute::DirectBits);
  assert(spec.base_code == mcprotocol::serial::DeviceCode::LCS);
  assert(spec.kind == LongStateReadKind::Contact);

  status = get_long_state_read_spec(mcprotocol::serial::DeviceCode::LCC, spec);
  assert(status.ok());
  assert(spec.route == LongStateReadRoute::DirectBits);
  assert(spec.base_code == mcprotocol::serial::DeviceCode::LCC);
  assert(spec.kind == LongStateReadKind::Coil);

  status = get_long_state_read_spec(mcprotocol::serial::DeviceCode::M, spec);
  assert(!status.ok());
  assert(status.code == StatusCode::InvalidArgument);

  const LongStateReadSpec invalid_spec(
      LongStateReadRoute::StatusBlock,
      mcprotocol::serial::DeviceCode::LTN,
      static_cast<LongStateReadKind>(0xFF));
  status = decode_long_state_bit(
      invalid_spec,
      mcprotocol::serial::Span<const std::uint16_t>(words.data(), words.size()),
      value);
  assert(status.code == StatusCode::InvalidArgument);
}

void test_long_state_read_aggregate_order_boundary_and_no_partial_output() {
  static_assert(mcprotocol::serial::detail::long_state_stage_bytes(1U) == 1U);
  static_assert(mcprotocol::serial::detail::long_state_stage_bytes(8U) == 1U);
  static_assert(mcprotocol::serial::detail::long_state_stage_bytes(9U) == 2U);
  static_assert(mcprotocol::serial::detail::long_state_stage_bytes(0xFFFFU) == 8192U);
  static_assert(
      mcprotocol::serial::detail::long_state_status_block_response_bytes(CodeMode::Binary) == 8U);
  static_assert(
      mcprotocol::serial::detail::long_state_status_block_response_bytes(CodeMode::Ascii) == 16U);
  static_assert(
      static_cast<std::uint8_t>(StatusCode::OutOfMemory) ==
      static_cast<std::uint8_t>(StatusCode::OperationOutcomeUnknown) + 1U);

  const LongStateReadSpec spec(
      LongStateReadRoute::StatusBlock,
      mcprotocol::serial::DeviceCode::LTN,
      LongStateReadKind::Contact);

  std::array<BitValue, 3> failed_output {{true, false, true}};
  std::array<std::uint32_t, 3> observed_addresses {};
  std::size_t calls = 0U;
  bool callback_active = false;
  auto fail_second = [&](mcprotocol::serial::DeviceAddress device,
                         mcprotocol::serial::Span<std::uint16_t> words) noexcept -> Status {
    assert(!callback_active);
    callback_active = true;
    observed_addresses[calls] = device.number;
    ++calls;
    if (calls == 2U) {
      callback_active = false;
      return mcprotocol::serial::make_status(StatusCode::Timeout, "injected aggregate failure");
    }
    words[2] = 0x0002U;
    callback_active = false;
    return mcprotocol::serial::ok_status();
  };

  Status status = mcprotocol::serial::detail::execute_long_state_read_aggregate(
      spec,
      100U,
      static_cast<std::uint16_t>(failed_output.size()),
      mcprotocol::serial::Span<BitValue>(failed_output.data(), failed_output.size()),
      fail_second);
  assert(status.code == StatusCode::Timeout);
  assert(calls == 2U);
  assert(observed_addresses[0] == 100U);
  assert(observed_addresses[1] == 101U);
  assert(failed_output[0] == true);
  assert(failed_output[1] == false);
  assert(failed_output[2] == true);

  calls = 0U;
  const auto fail_allocation = [](std::size_t) noexcept {
    return std::unique_ptr<std::uint8_t[]> {};
  };
  status = mcprotocol::serial::detail::execute_long_state_read_aggregate(
      spec,
      100U,
      static_cast<std::uint16_t>(failed_output.size()),
      mcprotocol::serial::Span<BitValue>(failed_output.data(), failed_output.size()),
      fail_second,
      fail_allocation);
  assert(status.code == StatusCode::OutOfMemory);
  assert(calls == 0U);
  assert(failed_output[0] == true);
  assert(failed_output[1] == false);
  assert(failed_output[2] == true);

  std::array<BitValue, 0xFFFFU> boundary_output {};
  calls = 0U;
  auto succeed_in_order = [&](mcprotocol::serial::DeviceAddress device,
                              mcprotocol::serial::Span<std::uint16_t> words) noexcept -> Status {
    assert(device.number == 500U + calls);
    words[2] = (calls % 2U) == 0U ? 0x0000U : 0x0002U;
    ++calls;
    return mcprotocol::serial::ok_status();
  };
  status = mcprotocol::serial::detail::execute_long_state_read_aggregate(
      spec,
      500U,
      0xFFFFU,
      mcprotocol::serial::Span<BitValue>(boundary_output.data(), boundary_output.size()),
      succeed_in_order);
  assert(status.ok());
  assert(calls == 0xFFFFU);
  assert(boundary_output[0] == false);
  assert(boundary_output[1] == true);
  assert(boundary_output[0xFFFEU] == false);
}

void test_encode_sm_sd_and_lz_device_codes() {
  const auto config = make_binary_c4_config();

  {
    const BatchReadBitsRequest request({mcprotocol::serial::DeviceCode::SM, 100}, 1);
    std::array<std::uint8_t, 32> request_data {};
    std::size_t request_data_size = 0;
    Status status = CommandCodec::encode_batch_read_bits(config, request, request_data, request_data_size);
    assert(status.ok());
    const std::array<std::uint8_t, 10> expected {0x01, 0x04, 0x01, 0x00, 0x64, 0x00, 0x00, 0x91, 0x01, 0x00};
    assert(request_data_size == expected.size());
    assert(std::equal(expected.begin(), expected.end(), request_data.begin()));
  }

  {
    const BatchReadBitsRequest request({mcprotocol::serial::DeviceCode::LTS, 3}, 1);
    std::array<std::uint8_t, 32> request_data {};
    std::size_t request_data_size = 0;
    Status status = CommandCodec::encode_batch_read_bits(config, request, request_data, request_data_size);
    assert(!status.ok());
    assert(status.code == StatusCode::InvalidArgument);
  }

  {
    const BatchReadWordsRequest request({mcprotocol::serial::DeviceCode::SD, 100}, 1);
    std::array<std::uint8_t, 32> request_data {};
    std::size_t request_data_size = 0;
    Status status = CommandCodec::encode_batch_read_words(config, request, request_data, request_data_size);
    assert(status.ok());
    const std::array<std::uint8_t, 10> expected {0x01, 0x04, 0x00, 0x00, 0x64, 0x00, 0x00, 0xA9, 0x01, 0x00};
    assert(request_data_size == expected.size());
    assert(std::equal(expected.begin(), expected.end(), request_data.begin()));
  }

  {
    const auto iqr_config = make_binary_c4_iqr_config();
    const BatchReadWordsRequest request({mcprotocol::serial::DeviceCode::RD, 20}, 1);
    std::array<std::uint8_t, 32> request_data {};
    std::size_t request_data_size = 0;
    Status status = CommandCodec::encode_batch_read_words(iqr_config, request, request_data, request_data_size);
    assert(status.ok());
    const std::array<std::uint8_t, 12> expected {
        0x01, 0x04, 0x02, 0x00, 0x14, 0x00, 0x00, 0x00, 0x2C, 0x00, 0x01, 0x00};
    assert(request_data_size == expected.size());
    assert(std::equal(expected.begin(), expected.end(), request_data.begin()));
  }

  {
    const BatchReadWordsRequest request({mcprotocol::serial::DeviceCode::ZR, 10}, 1);
    std::array<std::uint8_t, 32> request_data {};
    std::size_t request_data_size = 0;
    Status status = CommandCodec::encode_batch_read_words(config, request, request_data, request_data_size);
    assert(status.ok());
    const std::array<std::uint8_t, 10> expected {0x01, 0x04, 0x00, 0x00, 0x0A, 0x00, 0x00, 0xB0, 0x01, 0x00};
    assert(request_data_size == expected.size());
    assert(std::equal(expected.begin(), expected.end(), request_data.begin()));
  }

  {
    const std::array<RandomReadDWordItem, 1> items {{
        {{mcprotocol::serial::DeviceCode::LZ, 10}},
    }};
    const RandomReadRequest request({}, items);
    std::array<std::uint8_t, 32> request_data {};
    std::size_t request_data_size = 0;
    Status status = CommandCodec::encode_random_read(make_binary_c4_iqr_config(), request, request_data, request_data_size);
    assert(status.ok());
    const std::array<std::uint8_t, 12> expected {
        0x03, 0x04, 0x02, 0x00, 0x00, 0x01, 0x0A, 0x00, 0x00, 0x00, 0x62, 0x00};
    assert(request_data_size == expected.size());
    assert(std::equal(expected.begin(), expected.end(), request_data.begin()));

    request_data_size = 0;
    status = CommandCodec::encode_random_read(config, request, request_data, request_data_size);
    assert(!status.ok());
    assert(status.code == StatusCode::InvalidArgument);

    request_data_size = 0;
    status = CommandCodec::encode_random_read(make_binary_c4_iqf_config(), request, request_data, request_data_size);
    assert(status.ok());
    const std::array<std::uint8_t, 10> expected_iqf {
        0x03, 0x04, 0x00, 0x00, 0x00, 0x01, 0x0A, 0x00, 0x00, 0x62};
    assert(request_data_size == expected_iqf.size());
    assert(std::equal(expected_iqf.begin(), expected_iqf.end(), request_data.begin()));
  }

  {
    const std::array<RandomReadDWordItem, 1> items {{
        {{mcprotocol::serial::DeviceCode::LTN, 0}},
    }};
    const RandomReadRequest request({}, items);
    std::array<std::uint8_t, 32> request_data {};
    std::size_t request_data_size = 0;
    Status status = CommandCodec::encode_random_read(make_binary_c4_iqr_config(), request, request_data, request_data_size);
    assert(status.ok());
    const std::array<std::uint8_t, 12> expected {
        0x03, 0x04, 0x02, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x52, 0x00};
    assert(request_data_size == expected.size());
    assert(std::equal(expected.begin(), expected.end(), request_data.begin()));
  }

  {
    const std::array<RandomReadDWordItem, 1> items {{
        {{mcprotocol::serial::DeviceCode::LCN, 3}},
    }};
    const RandomReadRequest request({}, items);
    std::array<std::uint8_t, 32> request_data {};
    std::size_t request_data_size = 0;
    Status status = CommandCodec::encode_random_read(make_binary_c4_iqr_config(), request, request_data, request_data_size);
    assert(status.ok());
    const std::array<std::uint8_t, 12> expected {
        0x03, 0x04, 0x02, 0x00, 0x00, 0x01, 0x03, 0x00, 0x00, 0x00, 0x56, 0x00};
    assert(request_data_size == expected.size());
    assert(std::equal(expected.begin(), expected.end(), request_data.begin()));
  }
}

void test_encode_batch_write_words_ascii_order() {
  const auto config = make_ascii_c4_format4_config();
  const std::array<std::uint16_t, 2> values {0x007BU, 0x01C8U};
  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;

  const Status status = CommandCodec::encode_batch_write_words(
      config,
      BatchWriteWordsRequest(DeviceAddress {mcprotocol::serial::DeviceCode::D, 100}, values),
      request_data,
      request_size);
  assert(status.ok());

  constexpr std::string_view expected = "14010000D*0001000002007B01C8";
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_batch_word_access_rejects_standalone_qualified_only_devices() {
  const auto config = make_binary_c4_iqr_config();
  const std::array<std::uint16_t, 1> values {0x1234U};
  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;

  const mcprotocol::serial::DeviceCode rejected[] = {
      mcprotocol::serial::DeviceCode::G,
      mcprotocol::serial::DeviceCode::HG,
  };
  for (const auto code : rejected) {
    const Status read_status = CommandCodec::encode_batch_read_words(
        config,
        BatchReadWordsRequest(DeviceAddress {code, 10}, 1),
        request_data,
        request_size);
    assert(!read_status.ok());
    assert(read_status.code == StatusCode::InvalidArgument);

    const Status write_status = CommandCodec::encode_batch_write_words(
        config,
        BatchWriteWordsRequest(DeviceAddress {code, 10}, values),
        request_data,
        request_size);
    assert(!write_status.ok());
    assert(write_status.code == StatusCode::InvalidArgument);
  }
}

void test_all_profiles_reject_standalone_g_hg_plain_access() {
  const std::array<PlcProfile, 8> profiles {{
      PlcProfile::MelsecIqR,
      PlcProfile::MelsecIqL,
      PlcProfile::MelsecIqF,
      PlcProfile::MelsecQ,
      PlcProfile::MelsecL,
      PlcProfile::MelsecQnA,
      PlcProfile::MelsecAnAAnU,
      PlcProfile::MelsecA,
  }};
  const std::array<mcprotocol::serial::DeviceCode, 2> rejected {{
      mcprotocol::serial::DeviceCode::G,
      mcprotocol::serial::DeviceCode::HG,
  }};
  const std::array<std::uint16_t, 1> words {0x1234U};
  const std::array<BitValue, 1> bits {true};
  std::array<std::uint8_t, 128> request_data {};
  std::size_t request_size = 0;

  for (const auto profile : profiles) {
    auto config = make_binary_c4_config();
    config = config.with_plc_profile(profile);
    for (const auto code : rejected) {
      Status status = CommandCodec::encode_batch_read_words(
          config,
          BatchReadWordsRequest(DeviceAddress {code, 10}, 1),
          request_data,
          request_size);
      assert(!status.ok());
      assert(status.code == StatusCode::InvalidArgument);

      status = CommandCodec::encode_batch_write_words(
          config,
          BatchWriteWordsRequest(DeviceAddress {code, 10}, mcprotocol::serial::Span<const std::uint16_t>(words.data(), words.size())),
          request_data,
          request_size);
      assert(!status.ok());
      assert(status.code == StatusCode::InvalidArgument);

      status = CommandCodec::encode_batch_read_bits(
          config,
          BatchReadBitsRequest(DeviceAddress {code, 10}, 1),
          request_data,
          request_size);
      assert(!status.ok());
      assert(status.code == StatusCode::InvalidArgument);

      status = CommandCodec::encode_batch_write_bits(
          config,
          BatchWriteBitsRequest(DeviceAddress {code, 10}, mcprotocol::serial::Span<const BitValue>(bits.data(), bits.size())),
          request_data,
          request_size);
      assert(!status.ok());
      assert(status.code == StatusCode::InvalidArgument);

      const RandomReadWordItem read_word {{code, 10}};
      const RandomReadDWordItem read_dword {{code, 10}};
      status = CommandCodec::encode_random_read(
          config,
          RandomReadRequest(mcprotocol::serial::Span<const RandomReadWordItem>(&read_word, 1), mcprotocol::serial::Span<const RandomReadDWordItem>(&read_dword, 1)),
          request_data,
          request_size);
      assert(!status.ok());
      assert(status.code == StatusCode::InvalidArgument);

      const RandomWriteWordItem write_word({code, 10}, 0x1234U);
      const RandomWriteDWordItem write_dword({code, 10}, 0x12345678U);
      status = CommandCodec::encode_random_write_words(
          config,
          mcprotocol::serial::Span<const RandomWriteWordItem>(&write_word, 1),
          mcprotocol::serial::Span<const RandomWriteDWordItem>(&write_dword, 1),
          request_data,
          request_size);
      assert(!status.ok());
      assert(status.code == StatusCode::InvalidArgument);

      const RandomWriteBitItem write_bit({code, 10}, true);
      status = CommandCodec::encode_random_write_bits(
          config,
          mcprotocol::serial::Span<const RandomWriteBitItem>(&write_bit, 1),
          request_data,
          request_size);
      assert(!status.ok());
      assert(status.code == StatusCode::InvalidArgument);
    }
  }
}

void test_encode_extended_batch_read_words_ascii_matches_manual_shape() {
  const auto config = make_ascii_c4_format4_iqr_config();
  const QualifiedBufferWordDevice device(QualifiedBufferDeviceKind::G, 0x03E0U, 1U);
  std::array<std::uint8_t, 96> request_data {};
  std::size_t request_size = 0;

  const Status status = CommandCodec::encode_extended_batch_read_words(
      config,
      device,
      1U,
      request_data,
      request_size);
  assert(status.ok());

  constexpr std::string_view expected_prefix = "0401008200U3E00000G***00000000010000";
  assert(request_size == expected_prefix.size() + 4U);
  assert(std::memcmp(request_data.data(), expected_prefix.data(), expected_prefix.size()) == 0);
  assert(std::memcmp(request_data.data() + expected_prefix.size(), "0001", 4U) == 0);
}

void test_encode_extended_batch_read_words_binary_matches_capture_shape() {
  const auto config = make_binary_c4_iqr_config();
  const QualifiedBufferWordDevice device(QualifiedBufferDeviceKind::G, 0x03E0U, 10U);
  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;

  const Status status = CommandCodec::encode_extended_batch_read_words(
      config,
      device,
      1U,
      request_data,
      request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 19> expected {
      0x01, 0x04, 0x82, 0x00,
      0x00, 0x00,
      0x0A, 0x00, 0x00, 0x00,
      0xAB, 0x00,
      0x00, 0x00,
      0xE0, 0x03,
      0xFA,
      0x01, 0x00,
  };
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_extended_batch_read_words_iq_l_g_uses_q_l_wire_shape() {
  const auto config = make_binary_c4_iql_config();
  const QualifiedBufferWordDevice device(QualifiedBufferDeviceKind::G, 0x0001U, 10U);
  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;

  const Status status = CommandCodec::encode_extended_batch_read_words(
      config,
      device,
      1U,
      request_data,
      request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 17> expected {
      0x01, 0x04, 0x80, 0x00,
      0x00, 0x00,
      0x0A, 0x00, 0x00,
      0xAB,
      0x00, 0x00,
      0x01, 0x00,
      0xF8,
      0x01, 0x00,
  };
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_extended_batch_read_words_binary_hg_matches_capture_shape() {
  const auto config = make_binary_c4_iqr_config();
  const QualifiedBufferWordDevice device(QualifiedBufferDeviceKind::HG, 0x03E0U, 20U);
  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;

  const Status status = CommandCodec::encode_extended_batch_read_words(
      config,
      device,
      1U,
      request_data,
      request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 19> expected {
      0x01, 0x04, 0x82, 0x00,
      0x00, 0x00,
      0x14, 0x00, 0x00, 0x00,
      0x2E, 0x00,
      0x00, 0x00,
      0xE0, 0x03,
      0xFA,
      0x01, 0x00,
  };
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);

  request_size = 0U;
  const QualifiedBufferWordDevice last_cpu(
      QualifiedBufferDeviceKind::HG, 0x03E3U, 20U);
  const Status last_cpu_status = CommandCodec::encode_extended_batch_read_words(
      config, last_cpu, 1U, request_data, request_size);
  assert(last_cpu_status.ok());
  assert(request_size == expected.size());
  assert(request_data[14] == 0xE3U);
  assert(request_data[15] == 0x03U);
}

void test_encode_extended_batch_read_words_rejects_iq_l_hg() {
  const auto config = make_binary_c4_iql_config();
  const QualifiedBufferWordDevice device(QualifiedBufferDeviceKind::HG, 0x0001U, 10U);
  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;

  const Status status = CommandCodec::encode_extended_batch_read_words(
      config,
      device,
      1U,
      request_data,
      request_size);
  assert(!status.ok());
  assert(status.code == StatusCode::InvalidArgument);
  assert(std::strcmp(status.message, "HG device extension access is not available for MELSEC iQ-L") == 0);
}

void test_encode_extended_batch_read_words_binary_module_access_ql_shape() {
  const auto config = make_binary_c4_config();
  const QualifiedBufferWordDevice device(QualifiedBufferDeviceKind::G, 0x0003U, 1U);
  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;

  const Status status = CommandCodec::encode_extended_batch_read_words(
      config,
      device,
      1U,
      request_data,
      request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 15> expected {
      0x01, 0x04, 0x80, 0x00,
      0x00, 0x00,
      0x01, 0x00, 0x00,
      0xAB,
      0x00, 0x00,
      0x03, 0x00,
      0xF8,
  };
  assert(request_size == expected.size() + 2U);
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
  assert(request_data[expected.size()] == 0x01);
  assert(request_data[expected.size() + 1U] == 0x00);
}

void test_qualified_hg_rejects_non_cpu_modules_before_send() {
  struct LocalCapture {
    bool called = false;
  };
  const auto local_callback = [](void* user, Status) noexcept {
    static_cast<LocalCapture*>(user)->called = true;
  };
  const auto config = make_binary_c4_iqr_config();
  const std::array<QualifiedBufferWordDevice, 3> invalid_devices {{
      QualifiedBufferWordDevice(QualifiedBufferDeviceKind::HG, 0x0000U, 10U),
      QualifiedBufferWordDevice(QualifiedBufferDeviceKind::HG, 0x03DFU, 10U),
      QualifiedBufferWordDevice(QualifiedBufferDeviceKind::HG, 0x03E4U, 10U),
  }};
  const std::array<std::uint16_t, 1> write_words {0x1234U};

  for (const QualifiedBufferWordDevice& device : invalid_devices) {
    std::array<std::uint8_t, 64> request_data {};
    std::size_t request_size = 0U;
    Status status = CommandCodec::encode_extended_batch_read_words(
        config, device, 1U, request_data, request_size);
    assert(status.code == StatusCode::InvalidArgument);
    assert(request_size == 0U);

    status = CommandCodec::encode_extended_batch_write_words(
        config, device, write_words, request_data, request_size);
    assert(status.code == StatusCode::InvalidArgument);
    assert(request_size == 0U);

    MelsecSerialClient client;
    assert(client.configure(config).ok());
    std::array<std::uint16_t, 1> read_words {};
    LocalCapture capture {};
    status = client.async_qualified_buffer_batch_read_words(
        0U, device, 1U, read_words, local_callback, &capture);
    assert(status.code == StatusCode::InvalidArgument);
    assert(client.pending_tx_frame().empty());
    assert(!client.busy());

    status = client.async_qualified_buffer_batch_write_words(
        0U, device, write_words, local_callback, &capture);
    assert(status.code == StatusCode::InvalidArgument);
    assert(client.pending_tx_frame().empty());
    assert(!client.busy());

    mcprotocol::serial::highlevel::BitInWordWriteOperation operation;
    status = operation.begin_qualified_buffer(
        client, 0U, device, 0, true, local_callback, &capture);
    assert(status.code == StatusCode::InvalidArgument);
    assert(client.pending_tx_frame().empty());
    assert(!client.busy());
    assert(!operation.busy());
  }
}

void test_encode_link_direct_batch_read_words_binary_iqr_matches_manual_shape() {
  const auto config = make_binary_c4_iqr_config();
  const LinkDirectDevice device(0x0001U, DeviceAddress {mcprotocol::serial::DeviceCode::W, 0x0100U});
  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;

  const Status status = CommandCodec::encode_link_direct_batch_read_words(
      config,
      device,
      1U,
      request_data,
      request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 19> expected {
      0x01, 0x04, 0x82, 0x00,
      0x00, 0x00,
      0x00, 0x01, 0x00, 0x00,
      0xB4, 0x00,
      0x00, 0x00,
      0x01, 0x00,
      0xF9,
      0x01, 0x00,
  };
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_link_direct_batch_read_bits_binary_iqr_matches_manual_shape() {
  const auto config = make_binary_c4_iqr_config();
  const LinkDirectDevice device(0x0001U, DeviceAddress {mcprotocol::serial::DeviceCode::X, 0x0010U});
  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;

  const Status status = CommandCodec::encode_link_direct_batch_read_bits(
      config,
      device,
      4U,
      request_data,
      request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 19> expected {
      0x01, 0x04, 0x83, 0x00,
      0x00, 0x00,
      0x10, 0x00, 0x00, 0x00,
      0x9C, 0x00,
      0x00, 0x00,
      0x01, 0x00,
      0xF9,
      0x04, 0x00,
  };
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_link_direct_batch_read_bits_binary_single_uses_addressed_point() {
  const auto config = make_binary_c4_iqr_config();
  const LinkDirectDevice device(0x0001U, DeviceAddress {mcprotocol::serial::DeviceCode::B, 0x0010U});
  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;

  const Status status = CommandCodec::encode_link_direct_batch_read_bits(
      config,
      device,
      1U,
      request_data,
      request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 19> expected {
      0x01, 0x04, 0x83, 0x00,
      0x00, 0x00,
      0x10, 0x00, 0x00, 0x00,
      0xA0, 0x00,
      0x00, 0x00,
      0x01, 0x00,
      0xF9,
      0x01, 0x00,
  };
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_link_direct_batch_read_words_ascii_iqr_shape() {
  const auto config = make_ascii_c4_format4_iqr_config();
  const LinkDirectDevice device(0x0001U, DeviceAddress {mcprotocol::serial::DeviceCode::SW, 0x0011U});
  std::array<std::uint8_t, 96> request_data {};
  std::size_t request_size = 0;

  const Status status = CommandCodec::encode_link_direct_batch_read_words(
      config,
      device,
      1U,
      request_data,
      request_size);
  assert(status.ok());

  constexpr std::string_view expected = "0401008200J0010000SW**000000001100000001";
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_link_direct_batch_read_words_ascii_q_l_matches_manual_shape() {
  const auto config = make_ascii_c4_format4_config();
  const LinkDirectDevice device(0x0001U, DeviceAddress {mcprotocol::serial::DeviceCode::W, 0x0100U});
  std::array<std::uint8_t, 96> request_data {};
  std::size_t request_size = 0;

  const Status status = CommandCodec::encode_link_direct_batch_read_words(
      config,
      device,
      1U,
      request_data,
      request_size);
  assert(status.ok());

  constexpr std::string_view expected = "0401008000J001000W*0001000000001";
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_link_direct_batch_read_bits_ascii_iqr_shape() {
  const auto config = make_ascii_c4_format4_iqr_config();
  const LinkDirectDevice device(0x0001U, DeviceAddress {mcprotocol::serial::DeviceCode::SB, 0x0010U});
  std::array<std::uint8_t, 96> request_data {};
  std::size_t request_size = 0;

  const Status status = CommandCodec::encode_link_direct_batch_read_bits(
      config,
      device,
      4U,
      request_data,
      request_size);
  assert(status.ok());

  constexpr std::string_view expected = "0401008300J0010000SB**000000001000000004";
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_batch_read_bits_binary_single_uses_addressed_point() {
  const auto config = make_binary_c4_iqr_config();
  std::array<std::uint8_t, 32> request_data {};
  std::size_t request_size = 0;

  const Status status = CommandCodec::encode_batch_read_bits(
      config,
      BatchReadBitsRequest(DeviceAddress {mcprotocol::serial::DeviceCode::B, 0x0010U}, 1U),
      request_data,
      request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 12> expected {
      0x01, 0x04, 0x03, 0x00,
      0x10, 0x00, 0x00, 0x00, 0xA0, 0x00,
      0x01, 0x00,
  };
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_link_direct_batch_write_words_binary_iqr_shape() {
  const auto config = make_binary_c4_iqr_config();
  const LinkDirectDevice device(0x0001U, DeviceAddress {mcprotocol::serial::DeviceCode::W, 0x0100U});
  const std::array<std::uint16_t, 2> words {0x1234U, 0xABCDU};
  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;

  const Status status = CommandCodec::encode_link_direct_batch_write_words(
      config,
      device,
      words,
      request_data,
      request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 23> expected {
      0x01, 0x14, 0x82, 0x00,
      0x00, 0x00,
      0x00, 0x01, 0x00, 0x00,
      0xB4, 0x00,
      0x00, 0x00,
      0x01, 0x00,
      0xF9,
      0x02, 0x00,
      0x34, 0x12,
      0xCD, 0xAB,
  };
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_link_direct_batch_write_bits_binary_iqr_shape() {
  const auto config = make_binary_c4_iqr_config();
  const LinkDirectDevice device(0x0001U, DeviceAddress {mcprotocol::serial::DeviceCode::B, 0x0000U});
  const std::array<BitValue, 4> bits {true, false, true, false};
  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;

  const Status status = CommandCodec::encode_link_direct_batch_write_bits(
      config,
      device,
      bits,
      request_data,
      request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 21> expected {
      0x01, 0x14, 0x83, 0x00,
      0x00, 0x00,
      0x00, 0x00, 0x00, 0x00,
      0xA0, 0x00,
      0x00, 0x00,
      0x01, 0x00,
      0xF9,
      0x04, 0x00,
      0x10, 0x10,
  };
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_link_direct_batch_write_words_ascii_iqr_shape() {
  const auto config = make_ascii_c4_format4_iqr_config();
  const LinkDirectDevice device(0x0001U, DeviceAddress {mcprotocol::serial::DeviceCode::W, 0x0040U});
  const std::array<std::uint16_t, 2> words {0x1234U, 0xABCDU};
  std::array<std::uint8_t, 96> request_data {};
  std::size_t request_size = 0;

  const Status status = CommandCodec::encode_link_direct_batch_write_words(
      config,
      device,
      words,
      request_data,
      request_size);
  assert(status.ok());

  constexpr std::string_view expected = "1401008200J0010000W***0000000040000000021234ABCD";
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_link_direct_batch_write_bits_ascii_iqr_shape() {
  const auto config = make_ascii_c4_format4_iqr_config();
  const LinkDirectDevice device(0x0001U, DeviceAddress {mcprotocol::serial::DeviceCode::B, 0x0030U});
  const std::array<BitValue, 4> bits {true, false, true, false};
  std::array<std::uint8_t, 96> request_data {};
  std::size_t request_size = 0;

  const Status status = CommandCodec::encode_link_direct_batch_write_bits(
      config,
      device,
      bits,
      request_data,
      request_size);
  assert(status.ok());

  constexpr std::string_view expected = "1401008300J0010000B***0000000030000000041010";
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_link_direct_random_read_binary_iqr_shape() {
  const auto config = make_binary_c4_iqr_config();
  const std::array<LinkDirectRandomReadWordItem, 2> items {{
      {LinkDirectDevice(0x0001U, DeviceAddress {mcprotocol::serial::DeviceCode::W, 0x0100U})},
      {LinkDirectDevice(0x0001U, DeviceAddress {mcprotocol::serial::DeviceCode::SW, 0x0000U})},
  }};

  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_link_direct_random_read(
      config,
      mcprotocol::serial::Span<const LinkDirectRandomReadWordItem>(items.data(), items.size()),
      request_data,
      request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 28> expected {
      0x03, 0x04, 0x80, 0x00,
      0x02, 0x00,
      0x00, 0x00, 0x00, 0x01, 0x00, 0xB4, 0x00, 0x00, 0x01, 0x00, 0xF9,
      0x00, 0x00, 0x00, 0x00, 0x00, 0xB5, 0x00, 0x00, 0x01, 0x00, 0xF9,
  };
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_link_direct_random_write_words_binary_iqr_shape() {
  const auto config = make_binary_c4_iqr_config();
  const std::array<LinkDirectRandomWriteWordItem, 2> items {{
      LinkDirectRandomWriteWordItem(LinkDirectDevice(0x0001U, DeviceAddress {mcprotocol::serial::DeviceCode::W, 0x0100U}), 0x1234U),
      LinkDirectRandomWriteWordItem(LinkDirectDevice(0x0001U, DeviceAddress {mcprotocol::serial::DeviceCode::SW, 0x0000U}), 0xABCDU),
  }};

  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_link_direct_random_write_words(
      config,
      mcprotocol::serial::Span<const LinkDirectRandomWriteWordItem>(items.data(), items.size()),
      request_data,
      request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 32> expected {
      0x02, 0x14, 0x80, 0x00,
      0x02, 0x00,
      0x00, 0x00, 0x00, 0x01, 0x00, 0xB4, 0x00, 0x00, 0x01, 0x00, 0xF9, 0x34, 0x12,
      0x00, 0x00, 0x00, 0x00, 0x00, 0xB5, 0x00, 0x00, 0x01, 0x00, 0xF9, 0xCD, 0xAB,
  };
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_link_direct_random_write_words_rejects_wrapped_sizes() {
  const LinkDirectRandomWriteWordItem item(
      LinkDirectDevice(0x0001U, DeviceAddress {mcprotocol::serial::DeviceCode::W, 0x0100U}),
      0x1234U);
  auto encode_count = [&](const ProtocolConfig& config, std::size_t count) {
    const std::vector<LinkDirectRandomWriteWordItem> items(count, item);
    std::vector<std::uint8_t> request_data(4096U, 0U);
    std::size_t request_size = 0;
    return CommandCodec::encode_link_direct_random_write_words(
        config,
        mcprotocol::serial::Span<const LinkDirectRandomWriteWordItem>(items.data(), items.size()),
        mcprotocol::serial::Span<std::uint8_t>(request_data.data(), request_data.size()),
        request_size);
  };

  Status status = encode_count(make_ascii_c4_format4_iqr_config(), 80U);
  assert(status.ok());
  status = encode_count(make_ascii_c4_format4_iqr_config(), 81U);
  assert(status.code == StatusCode::InvalidArgument);
  status = encode_count(make_binary_c4_config(), 160U);
  assert(status.ok());
  status = encode_count(make_binary_c4_config(), 161U);
  assert(status.code == StatusCode::InvalidArgument);
  status = encode_count(make_binary_c4_config(), 5462U);
  assert(status.code == StatusCode::InvalidArgument);
  status = encode_count(make_binary_c4_config(), 65537U);
  assert(status.code == StatusCode::InvalidArgument);
}

void test_ql_normal_device_number_rejects_wire_overflow_without_truncation() {
  const auto config = make_binary_c4_config();
  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;
  Status status = CommandCodec::encode_batch_read_words(
      config,
      BatchReadWordsRequest(DeviceAddress {mcprotocol::serial::DeviceCode::D, 0x00FFFFFFU}, 1U),
      request_data,
      request_size);
  assert(status.ok());
  assert(request_data[4] == 0xFFU && request_data[5] == 0xFFU && request_data[6] == 0xFFU);

  request_size = 123U;
  status = CommandCodec::encode_batch_read_words(
      config,
      BatchReadWordsRequest(DeviceAddress {mcprotocol::serial::DeviceCode::D, 0x01000000U}, 1U),
      request_data,
      request_size);
  assert(status.code == StatusCode::InvalidArgument);
}

void test_encode_link_direct_random_write_bits_binary_iqr_shape() {
  const auto config = make_binary_c4_iqr_config();
  const std::array<LinkDirectRandomWriteBitItem, 2> items {{
      LinkDirectRandomWriteBitItem(LinkDirectDevice(0x0001U, DeviceAddress {mcprotocol::serial::DeviceCode::B, 0x0010U}), true),
      LinkDirectRandomWriteBitItem(LinkDirectDevice(0x0001U, DeviceAddress {mcprotocol::serial::DeviceCode::SB, 0x0010U}), false),
  }};

  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_link_direct_random_write_bits(
      config,
      mcprotocol::serial::Span<const LinkDirectRandomWriteBitItem>(items.data(), items.size()),
      request_data,
      request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 29> expected {
      0x02, 0x14, 0x81, 0x00,
      0x02,
      0x00, 0x00, 0x10, 0x00, 0x00, 0xA0, 0x00, 0x00, 0x01, 0x00, 0xF9, 0x01,
      0x00, 0x00, 0x10, 0x00, 0x00, 0xA1, 0x00, 0x00, 0x01, 0x00, 0xF9, 0x00,
  };
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_link_direct_multi_block_read_binary_iqr_shape() {
  const auto config = make_binary_c4_iqr_config();
  const std::array<LinkDirectMultiBlockReadBlock, 2> blocks {{
      LinkDirectMultiBlockReadBlock(
          LinkDirectDevice(0x0001U, DeviceAddress {mcprotocol::serial::DeviceCode::W, 0x0100U}),
          2U,
          false),
      LinkDirectMultiBlockReadBlock(
          LinkDirectDevice(0x0001U, DeviceAddress {mcprotocol::serial::DeviceCode::B, 0x0010U}),
          1U,
          true),
  }};

  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_link_direct_multi_block_read(
      config,
      LinkDirectMultiBlockReadRequest(mcprotocol::serial::Span<const LinkDirectMultiBlockReadBlock>(blocks.data(), blocks.size())),
      request_data,
      request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 36> expected {
      0x06, 0x04, 0x82, 0x00,
      0x01, 0x01,
      0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0xB4, 0x00, 0x00, 0x00, 0x01, 0x00, 0xF9, 0x02, 0x00,
      0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0xA0, 0x00, 0x00, 0x00, 0x01, 0x00, 0xF9, 0x01, 0x00,
  };
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_link_direct_multi_block_write_binary_iqr_shape() {
  const auto config = make_binary_c4_iqr_config();
  const std::array<std::uint16_t, 2> word_values {0x1234U, 0xABCDU};
  const std::array<BitValue, 16> bit_values {{
      true, false, true, false,
      true, false, true, false,
      false, true, false, true,
      false, true, false, true,
  }};
  const std::array<LinkDirectMultiBlockWriteBlock, 2> blocks {{
      LinkDirectMultiBlockWriteBlock(
          LinkDirectDevice(0x0001U, DeviceAddress {mcprotocol::serial::DeviceCode::W, 0x0100U}),
          2U,
          mcprotocol::serial::Span<const std::uint16_t>(word_values.data(), word_values.size())),
      LinkDirectMultiBlockWriteBlock(
          LinkDirectDevice(0x0001U, DeviceAddress {mcprotocol::serial::DeviceCode::B, 0x0010U}),
          1U,
          mcprotocol::serial::Span<const BitValue>(bit_values.data(), bit_values.size())),
  }};

  std::array<std::uint8_t, 96> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_link_direct_multi_block_write(
      config,
      LinkDirectMultiBlockWriteRequest(mcprotocol::serial::Span<const LinkDirectMultiBlockWriteBlock>(blocks.data(), blocks.size())),
      request_data,
      request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 42> expected {
      0x06, 0x14, 0x82, 0x00,
      0x01, 0x01,
      0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0xB4, 0x00, 0x00, 0x00, 0x01, 0x00, 0xF9, 0x02, 0x00, 0x34, 0x12, 0xCD, 0xAB,
      0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0xA0, 0x00, 0x00, 0x00, 0x01, 0x00, 0xF9, 0x01, 0x00, 0x55, 0xAA,
  };
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_link_direct_multi_block_write_binary_bit_order() {
  const auto config = make_binary_c4_iqr_config();
  const std::array<BitValue, 16> bit_values {{
      true, false, false, false,
      false, false, false, false,
      false, false, false, false,
      false, false, false, false,
  }};
  const std::array<LinkDirectMultiBlockWriteBlock, 1> blocks {{
      LinkDirectMultiBlockWriteBlock(
          LinkDirectDevice(0x0001U, DeviceAddress {mcprotocol::serial::DeviceCode::B, 0x0010U}),
          1U,
          mcprotocol::serial::Span<const BitValue>(bit_values.data(), bit_values.size())),
  }};

  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_link_direct_multi_block_write(
      config,
      LinkDirectMultiBlockWriteRequest(mcprotocol::serial::Span<const LinkDirectMultiBlockWriteBlock>(blocks.data(), blocks.size())),
      request_data,
      request_size);
  assert(status.ok());
  assert(request_size >= 2U);
  assert(request_data[request_size - 2U] == 0x01U);
  assert(request_data[request_size - 1U] == 0x00U);
}

void test_encode_link_direct_register_monitor_binary_iqr_shape() {
  const auto config = make_binary_c4_iqr_config();
  const std::array<LinkDirectRandomReadWordItem, 2> items {{
      {LinkDirectDevice(0x0001U, DeviceAddress {mcprotocol::serial::DeviceCode::W, 0x0100U})},
      {LinkDirectDevice(0x0001U, DeviceAddress {mcprotocol::serial::DeviceCode::SW, 0x0000U})},
  }};

  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_link_direct_register_monitor(
      config,
      LinkDirectMonitorRegistration(mcprotocol::serial::Span<const LinkDirectRandomReadWordItem>(items.data(), items.size())),
      request_data,
      request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 28> expected {
      0x01, 0x08, 0x80, 0x00,
      0x02, 0x00,
      0x00, 0x00, 0x00, 0x01, 0x00, 0xB4, 0x00, 0x00, 0x01, 0x00, 0xF9,
      0x00, 0x00, 0x00, 0x00, 0x00, 0xB5, 0x00, 0x00, 0x01, 0x00, 0xF9,
  };
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_batch_write_bits_binary_single_even_uses_addressed_point_and_high_nibble() {
  const auto config = make_binary_c4_iqr_config();
  const std::array<BitValue, 1> values {true};
  std::array<std::uint8_t, 32> request_data {};
  std::size_t request_size = 0;

  const Status status = CommandCodec::encode_batch_write_bits(
      config,
      BatchWriteBitsRequest(
          DeviceAddress {mcprotocol::serial::DeviceCode::B, 0x0010U},
          values),
      request_data,
      request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 13> expected {
      0x01, 0x14, 0x03, 0x00,
      0x10, 0x00, 0x00, 0x00, 0xA0, 0x00,
      0x01, 0x00,
      0x10,
  };
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_batch_write_bits_binary_single_odd_uses_addressed_point_and_high_nibble() {
  const auto config = make_binary_c4_iqr_config();
  const std::array<BitValue, 1> values {true};
  std::array<std::uint8_t, 32> request_data {};
  std::size_t request_size = 0;

  const Status status = CommandCodec::encode_batch_write_bits(
      config,
      BatchWriteBitsRequest(
          DeviceAddress {mcprotocol::serial::DeviceCode::B, 0x0011U},
          values),
      request_data,
      request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 13> expected {
      0x01, 0x14, 0x03, 0x00,
      0x11, 0x00, 0x00, 0x00, 0xA0, 0x00,
      0x01, 0x00,
      0x10,
  };
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_link_direct_batch_write_bits_binary_single_even_uses_addressed_point_and_high_nibble() {
  const auto config = make_binary_c4_iqr_config();
  const LinkDirectDevice device(0x0001U, DeviceAddress {mcprotocol::serial::DeviceCode::B, 0x0010U});
  const std::array<BitValue, 1> values {true};
  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;

  const Status status = CommandCodec::encode_link_direct_batch_write_bits(
      config,
      device,
      values,
      request_data,
      request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 20> expected {
      0x01, 0x14, 0x83, 0x00,
      0x00, 0x00,
      0x10, 0x00, 0x00, 0x00,
      0xA0, 0x00,
      0x00, 0x00,
      0x01, 0x00,
      0xF9,
      0x01, 0x00,
      0x10,
  };
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_parse_batch_read_bits_binary_single_uses_high_nibble() {
  const auto config = make_binary_c4_iqr_config();
  const BatchReadBitsRequest request({mcprotocol::serial::DeviceCode::B, 0x0010U}, 1U);
  const std::array<std::uint8_t, 1> response {0x10U};
  std::array<BitValue, 1> bits {};
  const Status status = CommandCodec::parse_batch_read_bits_response(config, request, response, bits);
  assert(status.ok());
  assert(bits[0] == true);
}

void test_encode_batch_write_bits_binary_two_points_use_high_then_low_nibbles() {
  const auto config = make_binary_c4_iqr_config();
  const std::array<BitValue, 2> values {true, false};
  std::array<std::uint8_t, 32> request_data {};
  std::size_t request_size = 0;

  const Status status = CommandCodec::encode_batch_write_bits(
      config,
      BatchWriteBitsRequest(
          DeviceAddress {mcprotocol::serial::DeviceCode::B, 0x0010U},
          values),
      request_data,
      request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 13> expected {
      0x01, 0x14, 0x03, 0x00,
      0x10, 0x00, 0x00, 0x00, 0xA0, 0x00,
      0x02, 0x00,
      0x10,
  };
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_parse_batch_read_bits_binary_two_points_use_high_then_low_nibbles() {
  const auto config = make_binary_c4_iqr_config();
  const BatchReadBitsRequest request({mcprotocol::serial::DeviceCode::B, 0x0010U}, 2U);
  const std::array<std::uint8_t, 1> response {0x10U};
  std::array<BitValue, 2> bits {};
  const Status status = CommandCodec::parse_batch_read_bits_response(config, request, response, bits);
  assert(status.ok());
  assert(bits[0] == true);
  assert(bits[1] == false);
}

void test_encode_batch_write_words_ascii_limit_matches_buffer() {
  const auto config = make_ascii_c4_format4_config();
  std::array<std::uint16_t, 870> ok_values {};
  std::array<std::uint16_t, 871> too_many_values {};
  std::array<std::uint8_t, mcprotocol::serial::kMaxRequestDataBytes> request_data {};
  std::size_t request_size = 0;

  Status status = CommandCodec::encode_batch_write_words(
      config,
      BatchWriteWordsRequest(DeviceAddress {mcprotocol::serial::DeviceCode::D, 100}, ok_values),
      request_data,
      request_size);
  assert(status.ok());

  status = CommandCodec::encode_batch_write_words(
      config,
      BatchWriteWordsRequest(DeviceAddress {mcprotocol::serial::DeviceCode::D, 100}, too_many_values),
      request_data,
      request_size);
  assert(status.code == StatusCode::InvalidArgument);
}

void test_encode_batch_write_bits_ascii_limit_matches_buffer() {
  const auto config = make_ascii_c4_format4_config();
  std::array<BitValue, 3480> ok_values {};
  std::array<BitValue, 3481> too_many_values {};
  std::array<std::uint8_t, mcprotocol::serial::kMaxRequestDataBytes> request_data {};
  std::size_t request_size = 0;

  Status status = CommandCodec::encode_batch_write_bits(
      config,
      BatchWriteBitsRequest(DeviceAddress {mcprotocol::serial::DeviceCode::M, 100}, ok_values),
      request_data,
      request_size);
  assert(status.ok());

  status = CommandCodec::encode_batch_write_bits(
      config,
      BatchWriteBitsRequest(DeviceAddress {mcprotocol::serial::DeviceCode::M, 100}, too_many_values),
      request_data,
      request_size);
  assert(status.code == StatusCode::InvalidArgument);
}

void test_encode_batch_read_bits_binary_c24_limit_is_7904_points() {
  const auto config = make_binary_c4_config();
  std::array<std::uint8_t, 32> request_data {};
  std::size_t request_size = 0;

  Status status = CommandCodec::encode_batch_read_bits(
      config,
      BatchReadBitsRequest(DeviceAddress {mcprotocol::serial::DeviceCode::M, 0}, 7904),
      request_data,
      request_size);
  assert(status.ok());

  status = CommandCodec::encode_batch_read_bits(
      config,
      BatchReadBitsRequest(DeviceAddress {mcprotocol::serial::DeviceCode::M, 0}, 7905),
      request_data,
      request_size);
  assert(status.code == StatusCode::InvalidArgument);
}

void test_c1_word_unit_bit_device_limits_and_alignment() {
  std::array<std::uint8_t, 256> request_data {};
  std::size_t request_size = 0;

  const auto c1_config = make_ascii_c1_format4_qna_config();
  Status status = CommandCodec::encode_batch_read_words(
      c1_config,
      BatchReadWordsRequest(DeviceAddress {mcprotocol::serial::DeviceCode::M, 0}, 32),
      request_data,
      request_size);
  assert(status.ok());

  status = CommandCodec::encode_batch_read_words(
      c1_config,
      BatchReadWordsRequest(DeviceAddress {mcprotocol::serial::DeviceCode::M, 0}, 33),
      request_data,
      request_size);
  assert(status.code == StatusCode::InvalidArgument);

  status = CommandCodec::encode_batch_read_words(
      c1_config,
      BatchReadWordsRequest(DeviceAddress {mcprotocol::serial::DeviceCode::M, 1}, 1),
      request_data,
      request_size);
  assert(status.code == StatusCode::InvalidArgument);

  std::array<std::uint16_t, 10> ten_words {};
  std::array<std::uint16_t, 11> eleven_words {};
  status = CommandCodec::encode_batch_write_words(
      c1_config,
      BatchWriteWordsRequest(DeviceAddress {mcprotocol::serial::DeviceCode::M, 0}, ten_words),
      request_data,
      request_size);
  assert(status.ok());

  status = CommandCodec::encode_batch_write_words(
      c1_config,
      BatchWriteWordsRequest(DeviceAddress {mcprotocol::serial::DeviceCode::M, 0}, eleven_words),
      request_data,
      request_size);
  assert(status.code == StatusCode::InvalidArgument);

}

void test_encode_random_write_words_ascii_matches_manual() {
  const auto config = make_ascii_c4_format4_config();
  const std::array<RandomWriteWordItem, 4> word_items {{
      RandomWriteWordItem(DeviceAddress {mcprotocol::serial::DeviceCode::D, 0}, 0x0550U),
      RandomWriteWordItem(DeviceAddress {mcprotocol::serial::DeviceCode::D, 1}, 0x0575U),
      RandomWriteWordItem(DeviceAddress {mcprotocol::serial::DeviceCode::M, 100}, 0x0540U),
      RandomWriteWordItem(DeviceAddress {mcprotocol::serial::DeviceCode::X, 0x20}, 0x0583U),
  }};
  const std::array<RandomWriteDWordItem, 3> dword_items {{
      RandomWriteDWordItem(DeviceAddress {mcprotocol::serial::DeviceCode::D, 1500}, 0x12024391U),
      RandomWriteDWordItem(DeviceAddress {mcprotocol::serial::DeviceCode::Y, 0x160}, 0x23752607U),
      RandomWriteDWordItem(DeviceAddress {mcprotocol::serial::DeviceCode::M, 1111}, 0x04250475U),
  }};

  std::array<std::uint8_t, 256> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_random_write_words(
      config,
      word_items,
      dword_items,
      request_data,
      request_size);
  assert(status.ok());

  constexpr std::string_view expected =
      "140200000403"
      "D*0000000550"
      "D*0000010575"
      "M*0001000540"
      "X*0000200583"
      "D*00150012024391"
      "Y*00016023752607"
      "M*00111104250475";
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_required_input_types_are_not_default_constructible() {
  static_assert(!std::is_default_constructible_v<DeviceAddress>);
  static_assert(!std::is_default_constructible_v<ExtendedFileRegisterAddress>);
  static_assert(!std::is_default_constructible_v<BatchReadWordsRequest>);
  static_assert(!std::is_default_constructible_v<BatchReadBitsRequest>);
  static_assert(!std::is_default_constructible_v<BatchWriteWordsRequest>);
  static_assert(!std::is_default_constructible_v<BatchWriteBitsRequest>);
  static_assert(!std::is_default_constructible_v<ExtendedFileRegisterBatchReadWordsRequest>);
  static_assert(!std::is_default_constructible_v<ExtendedFileRegisterDirectBatchReadWordsRequest>);
  static_assert(!std::is_default_constructible_v<ExtendedFileRegisterBatchWriteWordsRequest>);
  static_assert(!std::is_default_constructible_v<ExtendedFileRegisterDirectBatchWriteWordsRequest>);
  static_assert(!std::is_default_constructible_v<ExtendedFileRegisterRandomWriteWordItem>);
  static_assert(!std::is_default_constructible_v<ExtendedFileRegisterMonitorRegistration>);
  static_assert(!std::is_default_constructible_v<RandomReadWordItem>);
  static_assert(!std::is_default_constructible_v<RandomReadDWordItem>);
  static_assert(!std::is_default_constructible_v<RandomReadRequest>);
  static_assert(!std::is_default_constructible_v<RandomWriteWordItem>);
  static_assert(!std::is_default_constructible_v<RandomWriteDWordItem>);
  static_assert(!std::is_default_constructible_v<RandomWriteBitItem>);
  static_assert(!std::is_default_constructible_v<MultiBlockReadBlock>);
  static_assert(!std::is_default_constructible_v<MultiBlockReadRequest>);
  static_assert(!std::is_default_constructible_v<MultiBlockWriteBlock>);
  static_assert(!std::is_default_constructible_v<MultiBlockWriteRequest>);
  static_assert(!std::is_default_constructible_v<MonitorRegistration>);
  static_assert(!std::is_default_constructible_v<UserFrameRegistrationReadRequest>);
  static_assert(!std::is_default_constructible_v<UserFrameRegistrationWriteRequest>);
  static_assert(!std::is_default_constructible_v<UserFrameRegistrationDeleteRequest>);
  static_assert(!std::is_default_constructible_v<GlobalSignalControlRequest>);
  static_assert(!std::is_default_constructible_v<SerialModuleModeSwitchRequest>);
  static_assert(!std::is_default_constructible_v<HostBufferReadRequest>);
  static_assert(!std::is_default_constructible_v<HostBufferWriteRequest>);
  static_assert(!std::is_default_constructible_v<ModuleBufferReadRequest>);
  static_assert(!std::is_default_constructible_v<ModuleBufferWriteRequest>);
  static_assert(!std::is_default_constructible_v<LinkDirectDevice>);
  static_assert(!std::is_default_constructible_v<LinkDirectRandomReadWordItem>);
  static_assert(!std::is_default_constructible_v<LinkDirectRandomWriteWordItem>);
  static_assert(!std::is_default_constructible_v<LinkDirectRandomWriteBitItem>);
  static_assert(!std::is_default_constructible_v<LinkDirectMultiBlockReadBlock>);
  static_assert(!std::is_default_constructible_v<LinkDirectMultiBlockReadRequest>);
  static_assert(!std::is_default_constructible_v<LinkDirectMultiBlockWriteBlock>);
  static_assert(!std::is_default_constructible_v<LinkDirectMultiBlockWriteRequest>);
  static_assert(!std::is_default_constructible_v<LinkDirectMonitorRegistration>);
  static_assert(!std::is_default_constructible_v<QualifiedBufferWordDevice>);
  static_assert(!std::is_default_constructible_v<RandomReadWordSpec>);
  static_assert(!std::is_default_constructible_v<RandomReadDWordSpec>);
  static_assert(!std::is_default_constructible_v<RandomWriteWordSpec>);
  static_assert(!std::is_default_constructible_v<RandomWriteDWordSpec>);
  static_assert(!std::is_default_constructible_v<RandomWriteBitSpec>);
  static_assert(!std::is_default_constructible_v<LongStateReadSpec>);

  static_assert(std::is_default_constructible_v<CpuModelInfo>);
  static_assert(std::is_default_constructible_v<UserFrameRegistrationData>);
  static_assert(std::is_default_constructible_v<MultiBlockReadBlockResult>);
}

void test_explicit_random_width_contract_and_boundaries() {
  static_assert(!std::is_default_constructible_v<RandomWriteWordItem>);
  static_assert(!std::is_default_constructible_v<RandomWriteDWordItem>);
  static_assert(!std::is_default_constructible_v<RandomWriteBitItem>);
  static_assert(!std::is_default_constructible_v<RandomWriteWordSpec>);
  static_assert(!std::is_default_constructible_v<RandomWriteDWordSpec>);
  static_assert(!std::is_default_constructible_v<RandomWriteBitSpec>);
  static_assert(!std::is_default_constructible_v<LinkDirectRandomWriteWordItem>);
  static_assert(!std::is_default_constructible_v<LinkDirectRandomWriteBitItem>);
  static_assert(std::is_constructible_v<RandomWriteWordItem, DeviceAddress, std::uint16_t>);
  static_assert(std::is_constructible_v<RandomWriteDWordItem, DeviceAddress, std::uint32_t>);
  static_assert(std::is_constructible_v<RandomWriteBitItem, DeviceAddress, BitValue>);
  static_assert(!HasDoubleWordMember<RandomReadWordItem>);
  static_assert(!HasDoubleWordMember<RandomReadDWordItem>);
  static_assert(!HasDoubleWordMember<RandomWriteWordItem>);
  static_assert(!HasDoubleWordMember<RandomWriteDWordItem>);
  static_assert(!HasDoubleWordMember<LinkDirectRandomReadWordItem>);
  static_assert(!HasDoubleWordMember<LinkDirectRandomWriteWordItem>);
  static_assert(std::is_same_v<decltype(std::declval<RandomWriteWordItem>().value), std::uint16_t>);
  static_assert(std::is_same_v<decltype(std::declval<RandomWriteDWordItem>().value), std::uint32_t>);
  static_assert(std::is_same_v<
                decltype(std::declval<LinkDirectRandomWriteWordItem>().value),
                std::uint16_t>);

  const auto config = make_binary_c4_iqr_config();
  const std::array<RandomWriteWordItem, 2> word_writes {{
      RandomWriteWordItem(DeviceAddress {mcprotocol::serial::DeviceCode::D, 0}, 0x0000U),
      RandomWriteWordItem(DeviceAddress {mcprotocol::serial::DeviceCode::D, 1}, 0xFFFFU),
  }};
  const std::array<RandomWriteDWordItem, 2> dword_writes {{
      RandomWriteDWordItem(DeviceAddress {mcprotocol::serial::DeviceCode::D, 2}, 0x00010000U),
      RandomWriteDWordItem(DeviceAddress {mcprotocol::serial::DeviceCode::D, 4}, 0xFFFFFFFFU),
  }};
  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;
  Status status = CommandCodec::encode_random_write_words(
      config, word_writes, dword_writes, request_data, request_size);
  assert(status.ok());
  assert(request_size == 42U);
  assert(request_data[4] == 2U && request_data[5] == 2U);
  assert(request_data[12] == 0x00U && request_data[13] == 0x00U);
  assert(request_data[20] == 0xFFU && request_data[21] == 0xFFU);
  assert(request_data[28] == 0x00U && request_data[29] == 0x00U &&
         request_data[30] == 0x01U && request_data[31] == 0x00U);
  assert(request_data[38] == 0xFFU && request_data[39] == 0xFFU &&
         request_data[40] == 0xFFU && request_data[41] == 0xFFU);

  auto overflow_weight_items =
      mcprotocol::serial::detail::make_filled_array<RandomWriteWordItem, 5462>(
          RandomWriteWordItem(DeviceAddress {mcprotocol::serial::DeviceCode::D, 0U}, 0U));
  for (RandomWriteWordItem& item : overflow_weight_items) {
    item = RandomWriteWordItem(
        DeviceAddress {mcprotocol::serial::DeviceCode::D, 0}, 0U);
  }
  status = CommandCodec::encode_random_write_words(
      config, overflow_weight_items, {}, request_data, request_size);
  assert(status.code == StatusCode::InvalidArgument);

  const std::array<RandomReadWordItem, 2> word_reads {{
      {{mcprotocol::serial::DeviceCode::D, 0}},
      {{mcprotocol::serial::DeviceCode::D, 1}},
  }};
  const std::array<RandomReadDWordItem, 2> dword_reads {{
      {{mcprotocol::serial::DeviceCode::D, 2}},
      {{mcprotocol::serial::DeviceCode::D, 4}},
  }};
  const RandomReadRequest mixed_request(word_reads, dword_reads);
  const std::array<std::uint8_t, 12> response_data {
      0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x01, 0x00, 0xFF, 0xFF, 0xFF, 0xFF};
  std::array<std::uint16_t, 2> out_words {};
  std::array<std::uint32_t, 2> out_dwords {};
  status = CommandCodec::parse_random_read_response(
      config, mixed_request, response_data, out_words, out_dwords);
  assert(status.ok());
  assert(out_words[0] == 0x0000U && out_words[1] == 0xFFFFU);
  assert(out_dwords[0] == 0x00010000U && out_dwords[1] == 0xFFFFFFFFU);

  const MonitorRegistration mixed_monitor(word_reads, dword_reads);
  status = CommandCodec::encode_register_monitor(
      config, mixed_monitor, request_data, request_size);
  assert(status.ok());
  out_words.fill(0U);
  out_dwords.fill(0U);
  status = CommandCodec::parse_read_monitor_response(
      config, mixed_monitor, response_data, out_words, out_dwords);
  assert(status.ok());
  assert(out_words[0] == 0x0000U && out_words[1] == 0xFFFFU);
  assert(out_dwords[0] == 0x00010000U && out_dwords[1] == 0xFFFFFFFFU);

  const RandomReadWordItem lz_word {
      {mcprotocol::serial::DeviceCode::LZ, 0}};
  status = CommandCodec::encode_random_read(
      config,
      RandomReadRequest(mcprotocol::serial::Span<const RandomReadWordItem>(&lz_word, 1), {}),
      request_data, request_size);
  assert(status.code == StatusCode::InvalidArgument);
  const RandomReadDWordItem lz_dword {
      {mcprotocol::serial::DeviceCode::LZ, 0}};
  status = CommandCodec::encode_random_read(
      config,
      RandomReadRequest({}, mcprotocol::serial::Span<const RandomReadDWordItem>(&lz_dword, 1)),
      request_data, request_size);
  assert(status.ok());

  MelsecSerialClient output_guard_client;
  status = output_guard_client.configure(config);
  assert(status.ok());
  std::array<std::uint16_t, 1> too_few_words {};
  status = output_guard_client.async_random_read(
      0U, mixed_request, too_few_words, out_dwords, nullptr, nullptr);
  assert(status.code == StatusCode::BufferTooSmall);
  assert(!output_guard_client.busy());
  assert(output_guard_client.pending_tx_frame().empty());

  const std::array<LinkDirectRandomReadWordItem, 2> link_items {{
      {LinkDirectDevice(1U, DeviceAddress {mcprotocol::serial::DeviceCode::W, 0U})},
      {LinkDirectDevice(1U, DeviceAddress {mcprotocol::serial::DeviceCode::W, 1U})},
  }};
  status = output_guard_client.async_link_direct_random_read(
      0U, link_items, too_few_words, nullptr, nullptr);
  assert(status.code == StatusCode::BufferTooSmall);
  assert(!output_guard_client.busy());
  assert(output_guard_client.pending_tx_frame().empty());

  const RandomWriteDWordItem dword_item({mcprotocol::serial::DeviceCode::D, 0}, 0x00010000U);
  for (const ProtocolConfig restricted_config : {
           make_ascii_c1_format4_qna_config()}) {
    MelsecSerialClient client;
    status = client.configure(restricted_config);
    assert(status.ok());
    status = client.async_random_write_words(
        0U, {}, mcprotocol::serial::Span<const RandomWriteDWordItem>(&dword_item, 1), nullptr, nullptr);
    assert(!status.ok());
    assert(!client.busy());
    assert(client.pending_tx_frame().empty());

    status = client.async_register_monitor_devices(
        0U,
        MonitorRegistration(
            {}, mcprotocol::serial::Span<const RandomReadDWordItem>(&dword_reads[0], 1)),
        nullptr,
        nullptr);
    assert(!status.ok());
    assert(!client.busy());
    assert(client.pending_tx_frame().empty());
  }

  struct WriteCapture {
    bool called = false;
    Status status {};
  };
  const auto write_callback = +[](void* user, Status completion_status) {
    auto* capture = static_cast<WriteCapture*>(user);
    capture->called = true;
    capture->status = completion_status;
  };

  const RandomWriteWordItem explicit_zero(
      {mcprotocol::serial::DeviceCode::D, 100U}, 0U);
  MelsecSerialClient write_client;
  status = write_client.configure(config);
  assert(status.ok());
  WriteCapture timeout_capture;
  status = write_client.async_random_write_words(
      0U,
      mcprotocol::serial::Span<const RandomWriteWordItem>(&explicit_zero, 1),
      {},
      write_callback,
      &timeout_capture);
  assert(status.ok());
  assert(!write_client.pending_tx_frame().empty());
  status = start_and_notify_tx_complete(write_client, 1U, mcprotocol::serial::ok_status());
  assert(status.ok());
  write_client.poll(1U + config.timeout().response_timeout_ms);
  assert(timeout_capture.called);
  assert(timeout_capture.status.code == StatusCode::OperationOutcomeUnknown);
  assert(!write_client.busy());
  assert(write_client.pending_tx_frame().empty());
  assert(write_client.requires_transport_reset());

  MelsecSerialClient pre_tx_cancel_client;
  status = pre_tx_cancel_client.configure(config);
  assert(status.ok());
  WriteCapture pre_tx_cancel_capture;
  status = pre_tx_cancel_client.async_random_write_words(
      0U,
      mcprotocol::serial::Span<const RandomWriteWordItem>(&explicit_zero, 1),
      {},
      write_callback,
      &pre_tx_cancel_capture);
  assert(status.ok());
  pre_tx_cancel_client.cancel();
  assert(!pre_tx_cancel_client.busy());
  assert(pre_tx_cancel_capture.called);
  assert(pre_tx_cancel_capture.status.code == StatusCode::Cancelled);
  assert(!pre_tx_cancel_client.requires_transport_reset());
  status = pre_tx_cancel_client.notify_tx_complete(1U, mcprotocol::serial::ok_status());
  assert(status.code == StatusCode::InvalidArgument);

  const RandomWriteDWordItem explicit_dword_zero(
      {mcprotocol::serial::DeviceCode::D, 102U}, 0U);
  MelsecSerialClient transport_failure_client;
  status = transport_failure_client.configure(config);
  assert(status.ok());
  WriteCapture transport_failure_capture;
  status = transport_failure_client.async_random_write_words(
      10U,
      {},
      mcprotocol::serial::Span<const RandomWriteDWordItem>(&explicit_dword_zero, 1),
      write_callback,
      &transport_failure_capture);
  assert(status.ok());
  status = start_and_notify_tx_complete(transport_failure_client,
      11U,
      mcprotocol::serial::make_status(StatusCode::Transport, "simulated TX failure"));
  assert(status.ok());
  assert(transport_failure_capture.called);
  assert(transport_failure_capture.status.code == StatusCode::OperationOutcomeUnknown);
  assert(transport_failure_client.requires_transport_reset());
  assert(transport_failure_client.pending_tx_frame().empty());

  const RandomWriteBitItem explicit_off(
      {mcprotocol::serial::DeviceCode::M, 100U}, false);
  MelsecSerialClient post_tx_cancel_client;
  status = post_tx_cancel_client.configure(config);
  assert(status.ok());
  WriteCapture post_tx_cancel_capture;
  status = post_tx_cancel_client.async_random_write_bits(
      20U,
      mcprotocol::serial::Span<const RandomWriteBitItem>(&explicit_off, 1),
      write_callback,
      &post_tx_cancel_capture);
  assert(status.ok());
  assert(start_and_notify_tx_complete(post_tx_cancel_client, 21U, mcprotocol::serial::ok_status()).ok());
  post_tx_cancel_client.cancel();
  assert(post_tx_cancel_capture.called);
  assert(post_tx_cancel_capture.status.code == StatusCode::OperationOutcomeUnknown);
  assert(post_tx_cancel_client.requires_transport_reset());

  const LinkDirectRandomWriteWordItem link_explicit_zero(
      LinkDirectDevice(1U, DeviceAddress {mcprotocol::serial::DeviceCode::W, 100U}),
      0U);
  MelsecSerialClient link_write_client;
  status = link_write_client.configure(config);
  assert(status.ok());
  WriteCapture link_timeout_capture;
  status = link_write_client.async_link_direct_random_write_words(
      30U,
      mcprotocol::serial::Span<const LinkDirectRandomWriteWordItem>(&link_explicit_zero, 1),
      write_callback,
      &link_timeout_capture);
  assert(status.ok());
  assert(start_and_notify_tx_complete(link_write_client, 31U, mcprotocol::serial::ok_status()).ok());
  link_write_client.poll(31U + config.timeout().response_timeout_ms);
  assert(link_timeout_capture.called);
  assert(link_timeout_capture.status.code == StatusCode::OperationOutcomeUnknown);
  assert(link_write_client.requires_transport_reset());

  MelsecSerialClient plc_error_client;
  status = plc_error_client.configure(config);
  assert(status.ok());
  WriteCapture plc_error_capture;
  status = plc_error_client.async_random_write_words(
      40U,
      mcprotocol::serial::Span<const RandomWriteWordItem>(&explicit_zero, 1),
      {},
      write_callback,
      &plc_error_capture);
  assert(status.ok());
  assert(start_and_notify_tx_complete(plc_error_client, 41U, mcprotocol::serial::ok_status()).ok());
  std::array<std::uint8_t, 64> error_frame {};
  std::size_t error_frame_size = 0U;
  status = FrameCodec::encode_error_response(config, 0x7151U, error_frame, error_frame_size);
  assert(status.ok());
  plc_error_client.on_rx_bytes(
      42U,
      mcprotocol::serial::Span<const mcprotocol::serial::Byte>(
          reinterpret_cast<const mcprotocol::serial::Byte*>(error_frame.data()), error_frame_size));
  assert(plc_error_capture.called);
  assert(plc_error_capture.status.code == StatusCode::PlcError);
  assert(plc_error_capture.status.plc_error_code == 0x7151U);
  assert(!plc_error_client.requires_transport_reset());

}

void test_encode_random_read_binary_iqr_layout() {
  const auto config = make_binary_c4_iqr_config();
  const std::array<mcprotocol::serial::RandomReadWordItem, 2> items {{
      {{mcprotocol::serial::DeviceCode::D, 100}},
      {{mcprotocol::serial::DeviceCode::M, 100}},
  }};

  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_random_read(
      config,
      mcprotocol::serial::RandomReadRequest(
          mcprotocol::serial::Span<const mcprotocol::serial::RandomReadWordItem>(items.data(), items.size()),
          {}),
      request_data,
      request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 18> expected {
      0x03, 0x04, 0x02, 0x00,
      0x02, 0x00,
      0x64, 0x00, 0x00, 0x00, 0xA8, 0x00,
      0x64, 0x00, 0x00, 0x00, 0x90, 0x00,
  };
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_random_read_binary_ql_layout() {
  const auto config = make_binary_c4_config();
  const std::array<mcprotocol::serial::RandomReadWordItem, 2> items {{
      {{mcprotocol::serial::DeviceCode::D, 100}},
      {{mcprotocol::serial::DeviceCode::M, 100}},
  }};

  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_random_read(
      config,
      mcprotocol::serial::RandomReadRequest(
          mcprotocol::serial::Span<const mcprotocol::serial::RandomReadWordItem>(items.data(), items.size()),
          {}),
      request_data,
      request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 14> expected {
      0x03, 0x04, 0x00, 0x00,
      0x02, 0x00,
      0x64, 0x00, 0x00, 0xA8,
      0x64, 0x00, 0x00, 0x90,
  };
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_random_write_words_binary_ql_layout() {
  const auto config = make_binary_c4_config();
  const std::array<RandomWriteWordItem, 2> items {{
      RandomWriteWordItem(DeviceAddress {mcprotocol::serial::DeviceCode::D, 100}, 0x0001U),
      RandomWriteWordItem(DeviceAddress {mcprotocol::serial::DeviceCode::D, 101}, 0x0002U),
  }};

  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_random_write_words(
      config,
      mcprotocol::serial::Span<const RandomWriteWordItem>(items.data(), items.size()),
      {},
      request_data,
      request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 18> expected {
      0x02, 0x14, 0x00, 0x00,
      0x02, 0x00,
      0x64, 0x00, 0x00, 0xA8, 0x01, 0x00,
      0x65, 0x00, 0x00, 0xA8, 0x02, 0x00,
  };
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_random_write_words_binary_iqr_layout() {
  const auto config = make_binary_c4_iqr_config();
  const std::array<RandomWriteWordItem, 2> items {{
      RandomWriteWordItem(DeviceAddress {mcprotocol::serial::DeviceCode::D, 100}, 0x0001U),
      RandomWriteWordItem(DeviceAddress {mcprotocol::serial::DeviceCode::D, 101}, 0x0002U),
  }};

  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_random_write_words(
      config,
      mcprotocol::serial::Span<const RandomWriteWordItem>(items.data(), items.size()),
      {},
      request_data,
      request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 22> expected {
      0x02, 0x14, 0x02, 0x00,
      0x02, 0x00,
      0x64, 0x00, 0x00, 0x00, 0xA8, 0x00, 0x01, 0x00,
      0x65, 0x00, 0x00, 0x00, 0xA8, 0x00, 0x02, 0x00,
  };
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_random_write_bits_ascii_matches_manual() {
  const auto config = make_ascii_c4_format4_config();
  const std::array<RandomWriteBitItem, 2> items {{
      RandomWriteBitItem(DeviceAddress {mcprotocol::serial::DeviceCode::M, 50}, false),
      RandomWriteBitItem(DeviceAddress {mcprotocol::serial::DeviceCode::Y, 0x2F}, true),
  }};

  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_random_write_bits(
      config,
      mcprotocol::serial::Span<const RandomWriteBitItem>(items.data(), items.size()),
      request_data,
      request_size);
  assert(status.ok());

  constexpr std::string_view expected = "1402000102M*00005000Y*00002F01";
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_random_write_bits_ascii_iqr_shape() {
  const auto config = make_ascii_c4_format4_iqr_config();
  const std::array<RandomWriteBitItem, 2> items {{
      RandomWriteBitItem(DeviceAddress {mcprotocol::serial::DeviceCode::M, 50}, false),
      RandomWriteBitItem(DeviceAddress {mcprotocol::serial::DeviceCode::Y, 0x2F}, true),
  }};

  std::array<std::uint8_t, 96> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_random_write_bits(
      config,
      mcprotocol::serial::Span<const RandomWriteBitItem>(items.data(), items.size()),
      request_data,
      request_size);
  assert(status.ok());

  constexpr std::string_view expected = "1402000302M***000000500000Y***0000002F0001";
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_random_write_bits_binary_iqr_layout() {
  const auto config = make_binary_c4_iqr_config();
  const std::array<RandomWriteBitItem, 2> items {{
      RandomWriteBitItem(DeviceAddress {mcprotocol::serial::DeviceCode::M, 50}, false),
      RandomWriteBitItem(DeviceAddress {mcprotocol::serial::DeviceCode::Y, 0x2F}, true),
  }};

  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_random_write_bits(
      config,
      mcprotocol::serial::Span<const RandomWriteBitItem>(items.data(), items.size()),
      request_data,
      request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 21> expected {
      0x02, 0x14, 0x03, 0x00, 0x02,
      0x32, 0x00, 0x00, 0x00, 0x90, 0x00, 0x00, 0x00,
      0x2F, 0x00, 0x00, 0x00, 0x9D, 0x00, 0x01, 0x00,
  };
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_random_write_bits_binary_ql_keeps_device_numbers() {
  const auto config = make_binary_c4_config();
  const std::array<RandomWriteBitItem, 2> items {{
      RandomWriteBitItem(DeviceAddress {mcprotocol::serial::DeviceCode::M, 50}, false),
      RandomWriteBitItem(DeviceAddress {mcprotocol::serial::DeviceCode::Y, 0x2F}, true),
  }};

  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_random_write_bits(
      config,
      mcprotocol::serial::Span<const RandomWriteBitItem>(items.data(), items.size()),
      request_data,
      request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 15> expected {
      0x02, 0x14, 0x01, 0x00, 0x02, 0x32, 0x00,
      0x00, 0x90, 0x00, 0x2F, 0x00, 0x00, 0x9D, 0x01,
  };
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_qna_family_random_access_uses_smaller_limits() {
  auto config = make_binary_c4_config();
  config = config.with_plc_profile(PlcProfile::MelsecAnAAnU);
  static_assert(static_cast<std::uint8_t>(PlcSeries::AnA_AnU) == static_cast<std::uint8_t>(PlcSeries::QnA));

  std::array<std::uint8_t, 2048> request_data {};
  std::size_t request_size = 0;

  auto read_items = mcprotocol::serial::detail::make_filled_array<RandomReadWordItem, 97>(
      RandomReadWordItem {DeviceAddress {mcprotocol::serial::DeviceCode::D, 0U}});
  for (std::size_t index = 0; index < read_items.size(); ++index) {
    read_items[index] = RandomReadWordItem {
        DeviceAddress {mcprotocol::serial::DeviceCode::D, static_cast<std::uint32_t>(index)}};
  }
  Status status = CommandCodec::encode_random_read(
      config,
      RandomReadRequest(mcprotocol::serial::Span<const RandomReadWordItem>(read_items.data(), 96U), {}),
      request_data,
      request_size);
  assert(status.ok());

  status = CommandCodec::encode_random_read(
      config,
      RandomReadRequest(
          mcprotocol::serial::Span<const RandomReadWordItem>(read_items.data(), read_items.size()), {}),
      request_data,
      request_size);
  assert(status.code == StatusCode::InvalidArgument);

  auto bit_items = mcprotocol::serial::detail::make_filled_array<RandomWriteBitItem, 95>(
      RandomWriteBitItem(DeviceAddress {mcprotocol::serial::DeviceCode::M, 0U}, false));
  for (std::size_t index = 0; index < bit_items.size(); ++index) {
    bit_items[index] = RandomWriteBitItem(
        DeviceAddress {mcprotocol::serial::DeviceCode::M, static_cast<std::uint32_t>(index)},
        true);
  }
  status = CommandCodec::encode_random_write_bits(
      config,
      mcprotocol::serial::Span<const RandomWriteBitItem>(bit_items.data(), 94U),
      request_data,
      request_size);
  assert(status.ok());

  status = CommandCodec::encode_random_write_bits(
      config,
      mcprotocol::serial::Span<const RandomWriteBitItem>(bit_items.data(), bit_items.size()),
      request_data,
      request_size);
  assert(status.code == StatusCode::InvalidArgument);

  auto word_items = mcprotocol::serial::detail::make_filled_array<RandomWriteWordItem, 81>(
      RandomWriteWordItem(DeviceAddress {mcprotocol::serial::DeviceCode::D, 0U}, 0U));
  for (std::size_t index = 0; index < word_items.size(); ++index) {
    word_items[index] = RandomWriteWordItem(
        DeviceAddress {mcprotocol::serial::DeviceCode::D, static_cast<std::uint32_t>(index)},
        static_cast<std::uint16_t>(index));
  }
  status = CommandCodec::encode_random_write_words(
      config,
      mcprotocol::serial::Span<const RandomWriteWordItem>(word_items.data(), 80U),
      {},
      request_data,
      request_size);
  assert(status.ok());

  status = CommandCodec::encode_random_write_words(
      config,
      mcprotocol::serial::Span<const RandomWriteWordItem>(word_items.data(), word_items.size()),
      {},
      request_data,
      request_size);
  assert(status.code == StatusCode::InvalidArgument);
}

void test_encode_multi_block_read_ascii_matches_manual() {
  const auto config = make_ascii_c4_format4_config();
  const std::array<MultiBlockReadBlock, 5> blocks {{
      MultiBlockReadBlock(DeviceAddress {mcprotocol::serial::DeviceCode::D, 0}, 4, false),
      MultiBlockReadBlock(DeviceAddress {mcprotocol::serial::DeviceCode::W, 0x100}, 8, false),
      MultiBlockReadBlock(DeviceAddress {mcprotocol::serial::DeviceCode::M, 0}, 2, true),
      MultiBlockReadBlock(DeviceAddress {mcprotocol::serial::DeviceCode::M, 128}, 2, true),
      MultiBlockReadBlock(DeviceAddress {mcprotocol::serial::DeviceCode::B, 0x100}, 3, true),
  }};

  std::array<std::uint8_t, 128> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_multi_block_read(
      config,
      MultiBlockReadRequest(mcprotocol::serial::Span<const MultiBlockReadBlock>(blocks.data(), blocks.size())),
      request_data,
      request_size);
  assert(status.ok());

  constexpr std::string_view expected =
      "040600000203"
      "D*0000000004"
      "W*0001000008"
      "M*0000000002"
      "M*0001280002"
      "B*0001000003";
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_multi_block_read_binary_matches_capture_counts() {
  const auto config = make_binary_c4_config();

  const std::array<MultiBlockReadBlock, 3> bit_blocks {{
      MultiBlockReadBlock(DeviceAddress {mcprotocol::serial::DeviceCode::X, 0}, 1, true),
      MultiBlockReadBlock(DeviceAddress {mcprotocol::serial::DeviceCode::Y, 0}, 1, true),
      MultiBlockReadBlock(DeviceAddress {mcprotocol::serial::DeviceCode::B, 0}, 1, true),
  }};

  std::array<std::uint8_t, 128> request_data {};
  std::size_t request_size = 0;
  Status status = CommandCodec::encode_multi_block_read(
      config,
      MultiBlockReadRequest(mcprotocol::serial::Span<const MultiBlockReadBlock>(bit_blocks.data(), bit_blocks.size())),
      request_data,
      request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 24> expected_bit_request {
      0x06, 0x04, 0x00, 0x00,
      0x00, 0x03,
      0x00, 0x00, 0x00, 0x9C, 0x01, 0x00,
      0x00, 0x00, 0x00, 0x9D, 0x01, 0x00,
      0x00, 0x00, 0x00, 0xA0, 0x01, 0x00,
  };
  assert(request_size == expected_bit_request.size());
  assert(std::memcmp(request_data.data(), expected_bit_request.data(), expected_bit_request.size()) == 0);

  const std::array<MultiBlockReadBlock, 3> word_blocks {{
      MultiBlockReadBlock(DeviceAddress {mcprotocol::serial::DeviceCode::D, 0}, 1, false),
      MultiBlockReadBlock(DeviceAddress {mcprotocol::serial::DeviceCode::D, 10}, 1, false),
      MultiBlockReadBlock(DeviceAddress {mcprotocol::serial::DeviceCode::D, 100}, 1, false),
  }};

  request_size = 0;
  status = CommandCodec::encode_multi_block_read(
      config,
      MultiBlockReadRequest(mcprotocol::serial::Span<const MultiBlockReadBlock>(word_blocks.data(), word_blocks.size())),
      request_data,
      request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 24> expected_word_request {
      0x06, 0x04, 0x00, 0x00,
      0x03, 0x00,
      0x00, 0x00, 0x00, 0xA8, 0x01, 0x00,
      0x0A, 0x00, 0x00, 0xA8, 0x01, 0x00,
      0x64, 0x00, 0x00, 0xA8, 0x01, 0x00,
  };
  assert(request_size == expected_word_request.size());
  assert(std::memcmp(request_data.data(), expected_word_request.data(), expected_word_request.size()) == 0);
}

void test_encode_multi_block_read_rejects_total_points_over_960() {
  const auto config = make_binary_c4_config();
  const std::array<MultiBlockReadBlock, 2> blocks {{
      MultiBlockReadBlock(DeviceAddress {mcprotocol::serial::DeviceCode::D, 0}, 600, false),
      MultiBlockReadBlock(DeviceAddress {mcprotocol::serial::DeviceCode::D, 1000}, 361, false),
  }};
  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_multi_block_read(
      config,
      MultiBlockReadRequest(mcprotocol::serial::Span<const MultiBlockReadBlock>(blocks.data(), blocks.size())),
      request_data,
      request_size);
  assert(status.code == StatusCode::InvalidArgument);
}

void test_encode_multi_block_write_binary_uses_single_byte_block_counts() {
  const auto config = make_binary_c4_config();

  const std::array<std::uint16_t, 2> word_values {0x1234U, 0x5678U};
  const std::array<BitValue, 16> bit_values {{
      true, false, true, false,
      true, false, true, false,
      false, true, false, true,
      false, true, false, true,
  }};
  const std::array<MultiBlockWriteBlock, 2> blocks {{
      MultiBlockWriteBlock(
          DeviceAddress {mcprotocol::serial::DeviceCode::D, 0},
          2,
          mcprotocol::serial::Span<const std::uint16_t>(word_values.data(), word_values.size())),
      MultiBlockWriteBlock(
          DeviceAddress {mcprotocol::serial::DeviceCode::M, 100},
          1,
          mcprotocol::serial::Span<const BitValue>(bit_values.data(), bit_values.size())),
  }};

  std::array<std::uint8_t, 128> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_multi_block_write(
      config,
      MultiBlockWriteRequest(mcprotocol::serial::Span<const MultiBlockWriteBlock>(blocks.data(), blocks.size())),
      request_data,
      request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 24> expected {
      0x06, 0x14, 0x00, 0x00,
      0x01, 0x01,
      0x00, 0x00, 0x00, 0xA8, 0x02, 0x00, 0x34, 0x12, 0x78, 0x56,
      0x64, 0x00, 0x00, 0x90, 0x01, 0x00, 0x55, 0xAA,
  };
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_multi_block_write_iqr_uses_coefficient_nine() {
  const auto config = make_binary_c4_iqr_config();
  std::array<std::uint16_t, 472> first_words {};
  std::array<std::uint16_t, 471> second_words {};
  const std::array<MultiBlockWriteBlock, 2> blocks {{
      MultiBlockWriteBlock(
          DeviceAddress {mcprotocol::serial::DeviceCode::D, 0},
          472,
          mcprotocol::serial::Span<const std::uint16_t>(first_words.data(), first_words.size())),
      MultiBlockWriteBlock(
          DeviceAddress {mcprotocol::serial::DeviceCode::D, 1000},
          471,
          mcprotocol::serial::Span<const std::uint16_t>(second_words.data(), second_words.size())),
  }};

  std::array<std::uint8_t, mcprotocol::serial::kMaxRequestDataBytes> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_multi_block_write(
      config,
      MultiBlockWriteRequest(mcprotocol::serial::Span<const MultiBlockWriteBlock>(blocks.data(), blocks.size())),
      request_data,
      request_size);
  assert(status.code == StatusCode::InvalidArgument);
}

void test_encode_multi_block_write_binary_bit_blocks_use_lsb_first_word_packing() {
  const auto config = make_binary_c4_config();
  const std::array<BitValue, 32> bit_values {{
      false, false, false, false,
      false, false, false, false,
      false, false, false, false,
      false, false, false, false,
      false, false, false, true,
      false, false, true, false,
      false, false, true, true,
      false, true, false, false,
  }};
  const std::array<MultiBlockWriteBlock, 1> blocks {{
      MultiBlockWriteBlock(
          DeviceAddress {mcprotocol::serial::DeviceCode::M, 200},
          2,
          mcprotocol::serial::Span<const BitValue>(bit_values.data(), bit_values.size())),
  }};

  std::array<std::uint8_t, 128> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_multi_block_write(
      config,
      MultiBlockWriteRequest(mcprotocol::serial::Span<const MultiBlockWriteBlock>(blocks.data(), blocks.size())),
      request_data,
      request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 12> expected {
      0x06, 0x14, 0x00, 0x00,
      0x00, 0x01,
      0xC8, 0x00, 0x00, 0x90, 0x02, 0x00,
  };
  const std::array<std::uint8_t, 4> expected_bit_words {
      0x00, 0x00, 0x48, 0x2C,
  };
  assert(request_size == expected.size() + expected_bit_words.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
  assert(std::memcmp(request_data.data() + expected.size(), expected_bit_words.data(), expected_bit_words.size()) == 0);
}

void test_encode_multi_block_write_ascii_bit_blocks_use_lsb_first_word_packing() {
  const auto config = make_ascii_c4_format4_config();
  const std::array<BitValue, 16> bit_values {{
      true, false, true, false,
      true, false, true, false,
      false, true, false, true,
      false, true, false, true,
  }};
  const std::array<MultiBlockWriteBlock, 1> blocks {{
      MultiBlockWriteBlock(
          DeviceAddress {mcprotocol::serial::DeviceCode::M, 100},
          1,
          mcprotocol::serial::Span<const BitValue>(bit_values.data(), bit_values.size())),
  }};

  std::array<std::uint8_t, 128> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_multi_block_write(
      config,
      MultiBlockWriteRequest(mcprotocol::serial::Span<const MultiBlockWriteBlock>(blocks.data(), blocks.size())),
      request_data,
      request_size);
  assert(status.ok());

  constexpr std::string_view expected = "140600000001M*0001000001AA55";
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_register_monitor_ascii_reuses_random_read_layout() {
  const auto config = make_ascii_c4_format4_config();
  const std::array<mcprotocol::serial::RandomReadWordItem, 4> items {{
      {{mcprotocol::serial::DeviceCode::D, 100}},
      {{mcprotocol::serial::DeviceCode::D, 105}},
      {{mcprotocol::serial::DeviceCode::M, 100}},
      {{mcprotocol::serial::DeviceCode::M, 105}},
  }};

  std::array<std::uint8_t, 128> random_read_request {};
  std::size_t random_read_size = 0;
  Status status = CommandCodec::encode_random_read(
      config,
      mcprotocol::serial::RandomReadRequest(
          mcprotocol::serial::Span<const mcprotocol::serial::RandomReadWordItem>(items.data(), items.size()),
          {}),
      random_read_request,
      random_read_size);
  assert(status.ok());

  std::array<std::uint8_t, 128> monitor_request {};
  std::size_t monitor_size = 0;
  status = CommandCodec::encode_register_monitor(
      config,
      mcprotocol::serial::MonitorRegistration(mcprotocol::serial::Span<const mcprotocol::serial::RandomReadWordItem>(items.data(), items.size()), {}),
      monitor_request,
      monitor_size);
  assert(status.ok());

  assert(monitor_size == random_read_size);
  constexpr std::string_view expected_prefix = "08010000";
  assert(std::memcmp(monitor_request.data(), expected_prefix.data(), expected_prefix.size()) == 0);
  assert(std::memcmp(
             monitor_request.data() + expected_prefix.size(),
             random_read_request.data() + expected_prefix.size(),
             random_read_size - expected_prefix.size()) == 0);
}

void test_encode_register_monitor_ascii_c2_reuses_compact_command_header() {
  const auto config = make_ascii_c2_format4_config();
  const std::array<RandomReadWordItem, 2> items {{
      {{mcprotocol::serial::DeviceCode::D, 100U}},
      {{mcprotocol::serial::DeviceCode::M, 105U}},
  }};
  std::array<std::uint8_t, 128> random_request {};
  std::size_t random_size = 0;
  Status status = CommandCodec::encode_random_read(
      config,
      RandomReadRequest(items, {}),
      random_request,
      random_size);
  assert(status.ok());
  assert(random_request[0] == static_cast<std::uint8_t>('5'));

  std::array<std::uint8_t, 128> monitor_request {};
  std::size_t monitor_size = 0;
  status = CommandCodec::encode_register_monitor(
      config,
      MonitorRegistration(items, {}),
      monitor_request,
      monitor_size);
  assert(status.ok());
  assert(monitor_size == random_size);
  assert(monitor_request[0] == static_cast<std::uint8_t>('8'));
  assert(std::memcmp(monitor_request.data() + 1U, random_request.data() + 1U, random_size - 1U) == 0);

  std::array<std::uint8_t, 16> read_monitor_request {};
  std::size_t read_monitor_size = 0U;
  status = CommandCodec::encode_read_monitor(
      config, read_monitor_request, read_monitor_size);
  assert(status.ok());
  assert(read_monitor_size == 1U);
  assert(read_monitor_request[0] == static_cast<std::uint8_t>('9'));
}

void test_ascii_c2_exact_command_allowlist_vectors() {
  const auto config = make_ascii_c2_format3_config();
  std::array<std::uint8_t, 128> request_data {};
  std::size_t request_size = 0U;
  const auto assert_vector = [&](std::string_view expected) {
    assert(request_size == expected.size());
    assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
  };

  Status status = CommandCodec::encode_batch_read_bits(
      config,
      BatchReadBitsRequest(
          DeviceAddress {mcprotocol::serial::DeviceCode::M, 100U}, 2U),
      request_data,
      request_size);
  assert(status.ok());
  assert_vector("1M*0001000002");

  request_size = 0U;
  status = CommandCodec::encode_batch_read_words(
      config,
      BatchReadWordsRequest(
          DeviceAddress {mcprotocol::serial::DeviceCode::D, 100U}, 1U),
      request_data,
      request_size);
  assert(status.ok());
  assert_vector("2D*0001000001");

  const std::array<BitValue, 2> bits {true, false};
  request_size = 0U;
  status = CommandCodec::encode_batch_write_bits(
      config,
      BatchWriteBitsRequest(
          DeviceAddress {mcprotocol::serial::DeviceCode::M, 100U}, bits),
      request_data,
      request_size);
  assert(status.ok());
  assert_vector("3M*000100000210");

  const std::array<std::uint16_t, 1> words {0x1234U};
  request_size = 0U;
  status = CommandCodec::encode_batch_write_words(
      config,
      BatchWriteWordsRequest(
          DeviceAddress {mcprotocol::serial::DeviceCode::D, 100U}, words),
      request_data,
      request_size);
  assert(status.ok());
  assert_vector("4D*00010000011234");

  const std::array<RandomReadWordItem, 1> read_items {{
      RandomReadWordItem {DeviceAddress {mcprotocol::serial::DeviceCode::D, 100U}},
  }};
  request_size = 0U;
  status = CommandCodec::encode_random_read(
      config, RandomReadRequest(read_items, {}), request_data, request_size);
  assert(status.ok());
  assert_vector("50100D*000100");

  const std::array<RandomWriteBitItem, 1> write_bit_items {{
      RandomWriteBitItem(
          DeviceAddress {mcprotocol::serial::DeviceCode::M, 100U}, true),
  }};
  request_size = 0U;
  status = CommandCodec::encode_random_write_bits(
      config, write_bit_items, request_data, request_size);
  assert(status.ok());
  assert_vector("601M*00010001");

  const std::array<RandomWriteWordItem, 1> write_word_items {{
      RandomWriteWordItem(
          DeviceAddress {mcprotocol::serial::DeviceCode::D, 100U}, 0x1234U),
  }};
  request_size = 0U;
  status = CommandCodec::encode_random_write_words(
      config, write_word_items, {}, request_data, request_size);
  assert(status.ok());
  assert_vector("70100D*0001001234");

  request_size = 0U;
  status = CommandCodec::encode_register_monitor(
      config, MonitorRegistration(read_items, {}), request_data, request_size);
  assert(status.ok());
  assert_vector("80100D*000100");

  request_size = 0U;
  status = CommandCodec::encode_read_monitor(config, request_data, request_size);
  assert(status.ok());
  assert_vector("9");

  constexpr std::string_view loopback = "aBcD";
  request_size = 0U;
  status = CommandCodec::encode_loopback(
      config,
      mcprotocol::serial::Span<const char>(loopback.data(), loopback.size()),
      request_data,
      request_size);
  assert(status.ok());
  assert_vector("061900000004ABCD");
}

void test_ascii_c2_allowlist_rejects_unsupported_before_output_or_client_tx() {
  struct LocalCapture {
    bool called = false;
  };
  const auto local_callback = [](void* user, Status) noexcept {
    static_cast<LocalCapture*>(user)->called = true;
  };
  const auto base_config = make_ascii_c2_format3_config();
  const auto iq_r_config = base_config.with_plc_profile(PlcProfile::MelsecIqR);
  std::array<std::uint8_t, 128> request_data {};
  request_data.fill(0xA5U);
  std::size_t request_size = 0U;

  Status status = CommandCodec::encode_batch_read_words(
      iq_r_config,
      BatchReadWordsRequest(
          DeviceAddress {mcprotocol::serial::DeviceCode::D, 100U}, 1U),
      request_data,
      request_size);
  assert(status.code == StatusCode::UnsupportedConfiguration);
  assert(request_size == 0U);
  assert(request_data[0] == 0xA5U);

  status = CommandCodec::encode_batch_read_bits(
      iq_r_config,
      BatchReadBitsRequest(
          DeviceAddress {mcprotocol::serial::DeviceCode::M, 100U}, 1U),
      request_data,
      request_size);
  assert(status.code == StatusCode::UnsupportedConfiguration);
  assert(request_size == 0U);
  assert(request_data[0] == 0xA5U);

  const std::array<std::uint16_t, 1> batch_words {0x1234U};
  status = CommandCodec::encode_batch_write_words(
      iq_r_config,
      BatchWriteWordsRequest(
          DeviceAddress {mcprotocol::serial::DeviceCode::D, 100U}, batch_words),
      request_data,
      request_size);
  assert(status.code == StatusCode::UnsupportedConfiguration);
  assert(request_size == 0U);
  assert(request_data[0] == 0xA5U);

  const std::array<BitValue, 1> batch_bits {true};
  status = CommandCodec::encode_batch_write_bits(
      iq_r_config,
      BatchWriteBitsRequest(
          DeviceAddress {mcprotocol::serial::DeviceCode::M, 100U}, batch_bits),
      request_data,
      request_size);
  assert(status.code == StatusCode::UnsupportedConfiguration);
  assert(request_size == 0U);
  assert(request_data[0] == 0xA5U);

  const std::array<RandomReadWordItem, 1> items {{
      RandomReadWordItem {DeviceAddress {mcprotocol::serial::DeviceCode::D, 100U}},
  }};
  status = CommandCodec::encode_random_read(
      iq_r_config, RandomReadRequest(items, {}), request_data, request_size);
  assert(status.code == StatusCode::UnsupportedConfiguration);
  assert(request_size == 0U);
  assert(request_data[0] == 0xA5U);

  status = CommandCodec::encode_register_monitor(
      iq_r_config, MonitorRegistration(items, {}), request_data, request_size);
  assert(status.code == StatusCode::UnsupportedConfiguration);
  assert(request_size == 0U);
  assert(request_data[0] == 0xA5U);

  const std::array<RandomWriteWordItem, 1> write_words {{
      RandomWriteWordItem(
          DeviceAddress {mcprotocol::serial::DeviceCode::D, 100U}, 0x1234U),
  }};
  status = CommandCodec::encode_random_write_words(
      iq_r_config, write_words, {}, request_data, request_size);
  assert(status.code == StatusCode::UnsupportedConfiguration);
  assert(request_size == 0U);
  assert(request_data[0] == 0xA5U);

  const std::array<RandomWriteBitItem, 1> write_bits {{
      RandomWriteBitItem(
          DeviceAddress {mcprotocol::serial::DeviceCode::M, 100U}, true),
  }};
  status = CommandCodec::encode_random_write_bits(
      iq_r_config, write_bits, request_data, request_size);
  assert(status.code == StatusCode::UnsupportedConfiguration);
  assert(request_size == 0U);
  assert(request_data[0] == 0xA5U);

  const LinkDirectDevice link_word(
      1U, DeviceAddress {mcprotocol::serial::DeviceCode::W, 0x0100U});
  const LinkDirectDevice link_bit(
      1U, DeviceAddress {mcprotocol::serial::DeviceCode::B, 0x0100U});
  for (const ProtocolConfig& config :
       std::array<ProtocolConfig, 2> {{base_config, iq_r_config}}) {
    status = CommandCodec::encode_link_direct_batch_read_words(
        config, link_word, 1U, request_data, request_size);
    assert(status.code == StatusCode::UnsupportedConfiguration);
    assert(request_size == 0U);
    assert(request_data[0] == 0xA5U);

    status = CommandCodec::encode_link_direct_batch_read_bits(
        config, link_bit, 1U, request_data, request_size);
    assert(status.code == StatusCode::UnsupportedConfiguration);
    assert(request_size == 0U);
    assert(request_data[0] == 0xA5U);
  }

  const std::array<MultiBlockReadBlock, 1> read_blocks {{
      MultiBlockReadBlock(
          DeviceAddress {mcprotocol::serial::DeviceCode::D, 100U}, 1U, false),
  }};
  status = CommandCodec::encode_multi_block_read(
      base_config, MultiBlockReadRequest(read_blocks), request_data, request_size);
  assert(status.code == StatusCode::UnsupportedConfiguration);
  assert(request_size == 0U);
  assert(request_data[0] == 0xA5U);

  const std::array<std::uint16_t, 1> block_words {0x1234U};
  const std::array<MultiBlockWriteBlock, 1> write_blocks {{
      MultiBlockWriteBlock(
          DeviceAddress {mcprotocol::serial::DeviceCode::D, 100U},
          1U,
          block_words),
  }};
  status = CommandCodec::encode_multi_block_write(
      base_config, MultiBlockWriteRequest(write_blocks), request_data, request_size);
  assert(status.code == StatusCode::UnsupportedConfiguration);
  assert(request_size == 0U);
  assert(request_data[0] == 0xA5U);

  status = CommandCodec::encode_read_cpu_model(
      base_config, request_data, request_size);
  assert(status.code == StatusCode::UnsupportedConfiguration);
  assert(request_size == 0U);
  assert(request_data[0] == 0xA5U);

  status = CommandCodec::encode_read_module_buffer(
      base_config,
      ModuleBufferReadRequest(0U, 2U, 1U),
      request_data,
      request_size);
  assert(status.code == StatusCode::UnsupportedConfiguration);
  assert(request_size == 0U);
  assert(request_data[0] == 0xA5U);

  status = CommandCodec::encode_remote_run(
      base_config,
      RemoteOperationMode::DoNotExecuteForcibly,
      RemoteRunClearMode::DoNotClear,
      request_data,
      request_size);
  assert(status.code == StatusCode::UnsupportedConfiguration);
  assert(request_size == 0U);
  assert(request_data[0] == 0xA5U);

  status = CommandCodec::encode_read_user_frame(
      base_config,
      UserFrameRegistrationReadRequest(0x03E8U),
      request_data,
      request_size);
  assert(status.code == StatusCode::UnsupportedConfiguration);
  assert(request_size == 0U);
  assert(request_data[0] == 0xA5U);

  MelsecSerialClient client;
  assert(client.configure(iq_r_config).ok());
  std::array<std::uint16_t, 1> values {};
  LocalCapture capture {};
  status = client.async_batch_read_words(
      0U,
      BatchReadWordsRequest(
          DeviceAddress {mcprotocol::serial::DeviceCode::D, 100U}, 1U),
      values,
      local_callback,
      &capture);
  assert(status.code == StatusCode::UnsupportedConfiguration);
  assert(client.pending_tx_frame().empty());
  assert(!client.busy());
  assert(!capture.called);

  status = client.async_random_read(
      0U, RandomReadRequest(items, {}), values, {}, local_callback, &capture);
  assert(status.code == StatusCode::UnsupportedConfiguration);
  assert(client.pending_tx_frame().empty());
  assert(!client.busy());
  assert(!capture.called);

  status = client.async_register_monitor_devices(
      0U, MonitorRegistration(items, {}), local_callback, &capture);
  assert(status.code == StatusCode::UnsupportedConfiguration);
  assert(client.pending_tx_frame().empty());
  assert(!client.busy());
  assert(!capture.called);

  assert(client.configure(base_config).ok());
  CpuModelInfo model_info {};
  status = client.async_read_cpu_model(
      0U, model_info, local_callback, &capture);
  assert(status.code == StatusCode::UnsupportedConfiguration);
  assert(client.pending_tx_frame().empty());
  assert(!client.busy());
  assert(!capture.called);
}

void test_command_codec_rejects_removed_e1_ordinal_without_output() {
  constexpr auto removed_e1_ordinal = static_cast<mcprotocol::serial::AsciiFrameKind>(
      static_cast<std::uint8_t>(mcprotocol::serial::AsciiFrameKind::C1) + 1U);
  const ProtocolConfig config = ProtocolConfig::ascii(
      removed_e1_ordinal,
      AsciiFormat::Format3,
      PlcProfile::MelsecQ,
      SumCheckMode::Disabled,
      RouteConfig {HostStationRoute {}});
  std::array<std::uint8_t, 32> request_data {};
  request_data.fill(0xA5U);
  std::size_t request_size = 0U;

  const Status status = CommandCodec::encode_batch_read_words(
      config,
      BatchReadWordsRequest(
          DeviceAddress {mcprotocol::serial::DeviceCode::D, 100U}, 1U),
      request_data,
      request_size);

  assert(status.code == StatusCode::UnsupportedConfiguration);
  assert(request_size == 0U);
  assert(request_data[0] == 0xA5U);
}

void test_ascii_c2_loopback_uses_exact_full_header_capacity() {
  const auto config = make_ascii_c2_format3_config();
  constexpr std::string_view loopback = "aBcD";
  std::array<std::uint8_t, 16> exact {};
  std::size_t request_size = 0U;
  Status status = CommandCodec::encode_loopback(
      config,
      mcprotocol::serial::Span<const char>(loopback.data(), loopback.size()),
      exact,
      request_size);
  assert(status.ok());
  assert(request_size == exact.size());
  constexpr std::string_view expected = "061900000004ABCD";
  assert(std::memcmp(exact.data(), expected.data(), expected.size()) == 0);

  std::array<std::uint8_t, 15> one_byte_short {};
  request_size = 0U;
  status = CommandCodec::encode_loopback(
      config,
      mcprotocol::serial::Span<const char>(loopback.data(), loopback.size()),
      one_byte_short,
      request_size);
  assert(status.code == StatusCode::BufferTooSmall);
  assert(request_size == 0U);
}

void test_ascii_c2_compact_header_capacity_uses_one_byte() {
  const auto config = make_ascii_c2_format3_config();
  std::array<std::uint8_t, mcprotocol::serial::kMaxRequestDataBytes> request_data {};
  std::size_t request_size = 0U;

  const std::array<std::uint16_t, 871> fitting_words {};
  Status status = CommandCodec::encode_batch_write_words(
      config,
      BatchWriteWordsRequest(
          DeviceAddress {mcprotocol::serial::DeviceCode::D, 100U}, fitting_words),
      request_data,
      request_size);
  assert(status.ok());
  assert(request_size == 3497U);

  const std::array<std::uint16_t, 872> oversized_words {};
  request_size = 0U;
  status = CommandCodec::encode_batch_write_words(
      config,
      BatchWriteWordsRequest(
          DeviceAddress {mcprotocol::serial::DeviceCode::D, 100U}, oversized_words),
      request_data,
      request_size);
  assert(status.code == StatusCode::InvalidArgument);
  assert(request_size == 0U);

  const std::array<BitValue, 3487> fitting_bits {};
  request_size = 0U;
  status = CommandCodec::encode_batch_write_bits(
      config,
      BatchWriteBitsRequest(
          DeviceAddress {mcprotocol::serial::DeviceCode::M, 100U}, fitting_bits),
      request_data,
      request_size);
  assert(status.ok());
  assert(request_size == request_data.size());

  const std::array<BitValue, 3488> oversized_bits {};
  request_size = 0U;
  status = CommandCodec::encode_batch_write_bits(
      config,
      BatchWriteBitsRequest(
          DeviceAddress {mcprotocol::serial::DeviceCode::M, 100U}, oversized_bits),
      request_data,
      request_size);
  assert(status.code == StatusCode::InvalidArgument);
  assert(request_size == 0U);
}

void test_link_direct_extended_random_and_monitor_reject_ascii_c2() {
  struct LocalCapture {
    bool called = false;
  };
  const auto local_callback = [](void* user, Status) noexcept {
    static_cast<LocalCapture*>(user)->called = true;
  };
  const auto ql_config = make_ascii_c2_format4_config();
  const std::array<ProtocolConfig, 2> configs {{
      ql_config,
      ql_config.with_plc_profile(PlcProfile::MelsecIqR),
  }};
  const std::array<LinkDirectRandomReadWordItem, 1> items {{
      {LinkDirectDevice(0x0001U, DeviceAddress {mcprotocol::serial::DeviceCode::W, 0x0100U})},
  }};
  for (const ProtocolConfig& config : configs) {
    std::array<std::uint8_t, 128> random_request {};
    std::size_t random_size = 0;
    Status status = CommandCodec::encode_link_direct_random_read(
        config, items, random_request, random_size);
    assert(status.code == StatusCode::UnsupportedConfiguration);
    assert(random_size == 0U);

    std::array<std::uint8_t, 128> monitor_request {};
    std::size_t monitor_size = 0;
    status = CommandCodec::encode_link_direct_register_monitor(
        config,
        LinkDirectMonitorRegistration(items),
        monitor_request,
        monitor_size);
    assert(status.code == StatusCode::UnsupportedConfiguration);
    assert(monitor_size == 0U);

    MelsecSerialClient client;
    assert(client.configure(config).ok());
    std::array<std::uint16_t, 1> values {};
    LocalCapture capture {};
    status = client.async_link_direct_random_read(
        0U, items, values, local_callback, &capture);
    assert(status.code == StatusCode::UnsupportedConfiguration);
    assert(client.pending_tx_frame().empty());
    assert(!client.busy());

    status = client.async_link_direct_register_monitor(
        0U, LinkDirectMonitorRegistration(items), local_callback, &capture);
    assert(status.code == StatusCode::UnsupportedConfiguration);
    assert(client.pending_tx_frame().empty());
    assert(!client.busy());
    assert(!capture.called);
  }
}

void test_encode_success_response_large_sum_check_has_no_fixed_scratch_limit() {
  const auto config = test_config_with_sum_check(
      make_ascii_c4_format4_config(),
      SumCheckMode::Enabled);
  const std::vector<std::uint8_t> response_data(
      mcprotocol::serial::kMaxRequestFrameBytes + 64U,
      static_cast<std::uint8_t>('A'));
  std::vector<std::uint8_t> frame(response_data.size() + 128U, 0U);
  std::size_t frame_size = 0;
  const Status status = FrameCodec::encode_success_response(
      config,
      mcprotocol::serial::Span<const std::uint8_t>(response_data.data(), response_data.size()),
      mcprotocol::serial::Span<std::uint8_t>(frame.data(), frame.size()),
      frame_size);
  assert(status.ok());
  assert(frame_size > response_data.size());
}

void test_encode_register_monitor_binary_iqr_layout() {
  const auto config = make_binary_c4_iqr_config();
  const std::array<mcprotocol::serial::RandomReadWordItem, 2> items {{
      {{mcprotocol::serial::DeviceCode::D, 100}},
      {{mcprotocol::serial::DeviceCode::M, 100}},
  }};

  std::array<std::uint8_t, 64> random_read_request {};
  std::size_t random_read_size = 0;
  Status status = CommandCodec::encode_random_read(
      config,
      mcprotocol::serial::RandomReadRequest(
          mcprotocol::serial::Span<const mcprotocol::serial::RandomReadWordItem>(items.data(), items.size()),
          {}),
      random_read_request,
      random_read_size);
  assert(status.ok());

  std::array<std::uint8_t, 64> monitor_request {};
  std::size_t monitor_size = 0;
  status = CommandCodec::encode_register_monitor(
      config,
      mcprotocol::serial::MonitorRegistration(mcprotocol::serial::Span<const mcprotocol::serial::RandomReadWordItem>(items.data(), items.size()), {}),
      monitor_request,
      monitor_size);
  assert(status.ok());

  const std::array<std::uint8_t, 4> expected_prefix {0x01, 0x08, 0x02, 0x00};
  assert(monitor_size == random_read_size);
  assert(std::memcmp(monitor_request.data(), expected_prefix.data(), expected_prefix.size()) == 0);
  assert(std::memcmp(
             monitor_request.data() + expected_prefix.size(),
             random_read_request.data() + expected_prefix.size(),
             random_read_size - expected_prefix.size()) == 0);
}

void test_encode_register_monitor_binary_iqr_allows_lz_shape() {
  const auto config = make_binary_c4_iqr_config();
  const std::array<mcprotocol::serial::RandomReadDWordItem, 1> items {{
      {{mcprotocol::serial::DeviceCode::LZ, 1}},
  }};

  std::array<std::uint8_t, 64> monitor_request {};
  std::size_t monitor_size = 0;
  Status status = CommandCodec::encode_register_monitor(
      config,
      mcprotocol::serial::MonitorRegistration({}, items),
      monitor_request,
      monitor_size);
  assert(status.ok());

  const std::array<std::uint8_t, 12> expected {
      0x01, 0x08, 0x02, 0x00,
      0x00, 0x01,
      0x01, 0x00, 0x00, 0x00, 0x62, 0x00,
  };
  assert(monitor_size == expected.size());
  assert(std::memcmp(monitor_request.data(), expected.data(), expected.size()) == 0);
}

void test_encode_read_monitor_ascii_matches_manual() {
  const auto config = make_ascii_c4_format4_config();
  std::array<std::uint8_t, 16> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_read_monitor(config, request_data, request_size);
  assert(status.ok());

  constexpr std::string_view expected = "08020000";
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_sparse_native_bit_helpers_match_batch_random_and_monitor_values() {
  const std::array<BitValue, 16> batch_bits_on_on {{
      true, false, false, false,
      false, true, false, false,
      false, false, false, false,
      false, false, false, false,
  }};
  const std::array<std::uint32_t, 2> random_raw_on_on {{
      0x0021U,
      0x0001U,
  }};
  const std::array<std::uint32_t, 2> monitor_raw_on_on = random_raw_on_on;
  assert(sparse_native_mask_word(random_raw_on_on[0]) == 0x0021U);
  assert(sparse_native_mask_word(random_raw_on_on[1]) == 0x0001U);
  assert(sparse_native_requested_bit_value(random_raw_on_on[0]) == batch_bits_on_on[0]);
  assert(sparse_native_requested_bit_value(random_raw_on_on[1]) == batch_bits_on_on[5]);
  assert(sparse_native_requested_bit_value(monitor_raw_on_on[0]) == batch_bits_on_on[0]);
  assert(sparse_native_requested_bit_value(monitor_raw_on_on[1]) == batch_bits_on_on[5]);

  const std::array<BitValue, 16> batch_bits_off_on {{
      false, false, false, false,
      false, true, false, false,
      false, false, false, false,
      false, false, false, false,
  }};
  const std::array<std::uint32_t, 2> random_raw_off_on {{
      0x0020U,
      0x0001U,
  }};
  const std::array<std::uint32_t, 2> monitor_raw_off_on = random_raw_off_on;
  assert(sparse_native_mask_word(random_raw_off_on[0]) == 0x0020U);
  assert(sparse_native_mask_word(random_raw_off_on[1]) == 0x0001U);
  assert(sparse_native_requested_bit_value(random_raw_off_on[0]) == batch_bits_off_on[0]);
  assert(sparse_native_requested_bit_value(random_raw_off_on[1]) == batch_bits_off_on[5]);
  assert(sparse_native_requested_bit_value(monitor_raw_off_on[0]) == batch_bits_off_on[0]);
  assert(sparse_native_requested_bit_value(monitor_raw_off_on[1]) == batch_bits_off_on[5]);
}

void test_parse_multi_block_read_response_ascii_mixed_blocks() {
  const auto config = make_ascii_c4_format4_config();
  const std::array<MultiBlockReadBlock, 3> blocks {{
      MultiBlockReadBlock(DeviceAddress {mcprotocol::serial::DeviceCode::D, 100}, 2, false),
      MultiBlockReadBlock(DeviceAddress {mcprotocol::serial::DeviceCode::ZR, 200}, 1, false),
      MultiBlockReadBlock(DeviceAddress {mcprotocol::serial::DeviceCode::M, 300}, 1, true),
  }};
  const std::array<std::uint8_t, 16> response_data {
      '1', '2', '3', '4',
      'A', 'B', 'C', 'D',
      '0', 'F', '0', 'F',
      'A', '5', '5', 'A',
  };

  std::array<std::uint16_t, 3> words {};
  std::array<BitValue, 16> bits {};
  std::array<MultiBlockReadBlockResult, 3> results {};

  Status status = CommandCodec::parse_multi_block_read_response(
      config,
      mcprotocol::serial::Span<const MultiBlockReadBlock>(blocks.data(), blocks.size()),
      mcprotocol::serial::Span<const std::uint8_t>(response_data.data(), response_data.size()),
      mcprotocol::serial::Span<std::uint16_t>(words.data(), words.size()),
      mcprotocol::serial::Span<BitValue>(bits.data(), bits.size()),
      mcprotocol::serial::Span<MultiBlockReadBlockResult>(results.data(), results.size()));
  assert(status.ok());

  assert(words[0] == 0x1234U);
  assert(words[1] == 0xABCDU);
  assert(words[2] == 0x0F0FU);

  assert(!results[0].bit_block);
  assert(results[0].data_offset == 0U);
  assert(results[0].data_count == 2U);
  assert(!results[1].bit_block);
  assert(results[1].data_offset == 2U);
  assert(results[1].data_count == 1U);
  assert(results[2].bit_block);
  assert(results[2].data_offset == 0U);
  assert(results[2].data_count == 16U);

  const std::array<BitValue, 16> expected_bits {{
      false, true, false, true,
      true, false, true, false,
      true, false, true, false,
      false, true, false, true,
  }};
  for (std::size_t index = 0; index < bits.size(); ++index) {
    assert(bits[index] == expected_bits[index]);
  }
}

void test_parse_qualified_buffer_word_device_accepts_g_and_hg() {
  QualifiedBufferWordDevice g_device(QualifiedBufferDeviceKind::G, 0U, 0U);
  Status status = parse_qualified_buffer_word_device("U3E0\\G10", g_device);
  assert(status.ok());
  assert(g_device.kind == QualifiedBufferDeviceKind::G);
  assert(g_device.module_number == 0x03E0U);
  assert(g_device.word_address == 10U);

  QualifiedBufferWordDevice hg_device(QualifiedBufferDeviceKind::G, 0U, 0U);
  status = parse_qualified_buffer_word_device("u3e0/hg20", hg_device);
  assert(status.ok());
  assert(hg_device.kind == QualifiedBufferDeviceKind::HG);
  assert(hg_device.module_number == 0x03E0U);
  assert(hg_device.word_address == 20U);

  status = parse_qualified_buffer_word_device("U3E3\\HG21", hg_device);
  assert(status.ok());
  assert(hg_device.module_number == 0x03E3U);

  for (const std::string_view invalid : {
           std::string_view {"U0\\HG0"},
           std::string_view {"U3DF\\HG0"},
           std::string_view {"U3E4\\HG0"}}) {
    status = parse_qualified_buffer_word_device(invalid, hg_device);
    assert(status.code == StatusCode::InvalidArgument);
  }
}

void test_make_qualified_buffer_read_words_request_maps_to_module_buffer() {
  const QualifiedBufferWordDevice device(QualifiedBufferDeviceKind::G, 0x0002U, 10U);

  ModuleBufferReadRequest request(0U, 0U, 0U);
  const Status status = make_qualified_buffer_read_words_request(device, 4U, request);
  assert(status.ok());
  assert(request.start_address == 20U);
  assert(request.bytes == 8U);
  assert(request.module_number == 0x0002U);

  for (const QualifiedBufferWordDevice invalid : {
           QualifiedBufferWordDevice(QualifiedBufferDeviceKind::G, 0x03E0U, 10U),
           QualifiedBufferWordDevice(QualifiedBufferDeviceKind::HG, 0x03E0U, 10U)}) {
    const Status invalid_status =
        make_qualified_buffer_read_words_request(invalid, 4U, request);
    assert(invalid_status.code == StatusCode::UnsupportedConfiguration);
  }
}

void test_validate_qualified_buffer_helper_route_rejects_q_l_equivalent_profiles() {
  const QualifiedBufferWordDevice g_device(QualifiedBufferDeviceKind::G, 0x0002U, 1000U);
  Status status = validate_qualified_buffer_helper_route(PlcProfile::MelsecQ, g_device);
  assert(status.code == StatusCode::UnsupportedConfiguration);
  assert(std::strcmp(
             status.message,
             "melsec:qcpu qualified buffer helper route is disabled; use native-qualified Un\\G access") == 0);

  status = validate_qualified_buffer_helper_route(PlcProfile::MelsecL, g_device);
  assert(status.code == StatusCode::UnsupportedConfiguration);
  assert(std::strcmp(
             status.message,
             "melsec:lcpu qualified buffer helper route is disabled; use native-qualified Un\\G access") == 0);

  status = validate_qualified_buffer_helper_route(PlcProfile::MelsecIqR, g_device);
  assert(status.code == StatusCode::UnsupportedConfiguration);
  assert(std::strcmp(
             status.message,
             "melsec:iq-r qualified buffer helper route is disabled; use the native-qualified API") == 0);

  status = validate_qualified_buffer_helper_route(
      PlcProfile::MelsecA,
      QualifiedBufferWordDevice(QualifiedBufferDeviceKind::G, 0x03E0U, 10U));
  assert(status.code == StatusCode::UnsupportedConfiguration);
  status = validate_qualified_buffer_helper_route(
      PlcProfile::MelsecAnAAnU,
      QualifiedBufferWordDevice(QualifiedBufferDeviceKind::HG, 0x03E0U, 10U));
  assert(status.code == StatusCode::UnsupportedConfiguration);

  status = validate_qualified_buffer_helper_route(PlcProfile::MelsecIqL, g_device);
  assert(status.code == StatusCode::UnsupportedConfiguration);
  assert(std::strcmp(
             status.message,
             "melsec:iq-l qualified buffer helper route is disabled; use native-qualified Un\\G access") == 0);

  const QualifiedBufferWordDevice hg_device(QualifiedBufferDeviceKind::HG, 0x03E0U, 10U);
  status = validate_qualified_buffer_helper_route(PlcProfile::MelsecIqL, hg_device);
  assert(status.code == StatusCode::UnsupportedConfiguration);
  assert(std::strcmp(status.message, "melsec:iq-l does not support Un\\HG; use Un\\G only") == 0);

  status = validate_qualified_buffer_helper_route(PlcProfile::MelsecIqF, g_device);
  assert(status.code == StatusCode::UnsupportedConfiguration);
  assert(std::strcmp(
             status.message,
             "melsec:iq-f qualified buffer helper route is disabled; use native-qualified Un\\G access") == 0);

  status = validate_qualified_buffer_helper_route(PlcProfile::MelsecIqF, hg_device);
  assert(status.code == StatusCode::UnsupportedConfiguration);
  assert(std::strcmp(status.message, "melsec:iq-f does not support Un\\HG; use Un\\G only") == 0);

  const QualifiedBufferWordDevice invalid_device(
      static_cast<QualifiedBufferDeviceKind>(0xFF), 0x0001U, 10U);
  status = validate_qualified_buffer_helper_route(PlcProfile::MelsecIqR, invalid_device);
  assert(status.code == StatusCode::InvalidArgument);
}

void test_single_request_capacity_uses_complete_worst_case_wire_size() {
  struct CapacityCapture {
    bool called = false;
    Status status {};
  };
  const auto capacity_callback = [](void* user, Status completion_status) noexcept {
    auto* capture = static_cast<CapacityCapture*>(user);
    capture->called = true;
    capture->status = completion_status;
  };
  const auto binary = make_binary_c4_config();

  std::size_t maximum_request_payload = 0U;
  while (maximum_request_payload < mcprotocol::serial::kMaxRequestDataBytes &&
         FrameCodec::validate_request_capacity(binary, maximum_request_payload + 1U).ok()) {
    ++maximum_request_payload;
  }
  assert(maximum_request_payload > 0U);
  assert(FrameCodec::validate_request_capacity(binary, maximum_request_payload).ok());
  assert(!FrameCodec::validate_request_capacity(binary, maximum_request_payload + 1U).ok());

  std::vector<std::uint8_t> request_data(maximum_request_payload, 0x10U);
  std::array<std::uint8_t, mcprotocol::serial::kMaxRequestFrameBytes> request_frame {};
  std::size_t request_frame_size = 0U;
  Status status = FrameCodec::encode_request(
      binary,
      mcprotocol::serial::Span<const std::uint8_t>(request_data.data(), request_data.size()),
      request_frame,
      request_frame_size);
  assert(status.ok());
  assert(request_frame_size <= request_frame.size());

  const std::array<ProtocolConfig, 8> configs {{
      make_binary_c4_config(),
      make_ascii_c4_format2_config(),
      make_ascii_c4_format4_config(),
      make_ascii_c3_format3_config(),
      make_ascii_c2_format2_config(),
      make_ascii_c2_format3_config(),
      make_ascii_c2_format4_config(),
      make_ascii_c1_format4_qna_config(),
  }};

  for (const ProtocolConfig& config : configs) {
    std::uint16_t maximum_points = 0U;
    for (std::uint16_t points = 1U; points <= 7904U; ++points) {
      std::array<std::uint8_t, mcprotocol::serial::kMaxRequestDataBytes> command_data {};
      std::size_t command_size = 0U;
      const Status command_status = mcprotocol::serial::CommandCodec::encode_batch_read_bits(
          config,
          BatchReadBitsRequest(
              DeviceAddress {mcprotocol::serial::DeviceCode::M, 0U}, points),
          command_data,
          command_size);
      if (!command_status.ok() ||
          !FrameCodec::validate_request_capacity(config, command_size).ok()) {
        break;
      }
      const std::size_t response_data_size =
          config.code_mode() == CodeMode::Ascii
              ? static_cast<std::size_t>(points)
              : static_cast<std::size_t>((points + 1U) / 2U);
      if (!FrameCodec::validate_response_capacity(config, response_data_size).ok()) {
        break;
      }
      maximum_points = points;
    }
    assert(maximum_points > 0U);
    assert(maximum_points < 7904U);

    MelsecSerialClient client;
    status = client.configure(config);
    assert(status.ok());
    std::array<BitValue, 7904U> accepted_output {};
    CapacityCapture accepted_capture;
    status = client.async_batch_read_bits(
        10U,
        BatchReadBitsRequest(
            DeviceAddress {mcprotocol::serial::DeviceCode::M, 0U}, maximum_points),
        mcprotocol::serial::Span<BitValue>(accepted_output.data(), maximum_points),
        capacity_callback,
        &accepted_capture);
    assert(status.ok());
    assert(client.busy());
    assert(!client.pending_tx_frame().empty());
    client.cancel();
    assert(accepted_capture.called);
    assert(accepted_capture.status.code == StatusCode::Cancelled);
    assert(!client.busy());

    status = client.configure(config);
    assert(status.ok());
    const std::uint16_t rejected_points = static_cast<std::uint16_t>(maximum_points + 1U);
    std::array<BitValue, 7904U> rejected_output {};
    CapacityCapture rejected_capture;
    status = client.async_batch_read_bits(
        20U,
        BatchReadBitsRequest(
            DeviceAddress {mcprotocol::serial::DeviceCode::M, 0U}, rejected_points),
        mcprotocol::serial::Span<BitValue>(rejected_output.data(), rejected_points),
        capacity_callback,
        &rejected_capture);
    assert(!status.ok());
    assert(status.code == StatusCode::InvalidArgument);
    assert(!client.busy());
    assert(client.pending_tx_frame().empty());
    assert(!rejected_capture.called);
  }
}

void test_make_qualified_buffer_write_words_request_encodes_little_endian_bytes() {
  const QualifiedBufferWordDevice device(QualifiedBufferDeviceKind::G, 0x0002U, 20U);
  const std::array<std::uint16_t, 2> words {0x1234U, 0xABCDU};
  std::array<mcprotocol::serial::Byte, 8> byte_storage {};
  ModuleBufferWriteRequest request(0U, 0U, {});
  std::size_t byte_count = 0U;

  const Status status = make_qualified_buffer_write_words_request(
      device,
      words,
      byte_storage,
      request,
      byte_count);
  assert(status.ok());
  assert(byte_count == 4U);
  assert(request.start_address == 40U);
  assert(request.module_number == 0x0002U);
  assert(request.bytes.size() == 4U);
  assert(mcprotocol::serial::byte_to_integer<std::uint8_t>(request.bytes[0]) == 0x34U);
  assert(mcprotocol::serial::byte_to_integer<std::uint8_t>(request.bytes[1]) == 0x12U);
  assert(mcprotocol::serial::byte_to_integer<std::uint8_t>(request.bytes[2]) == 0xCDU);
  assert(mcprotocol::serial::byte_to_integer<std::uint8_t>(request.bytes[3]) == 0xABU);

  for (const QualifiedBufferWordDevice invalid : {
           QualifiedBufferWordDevice(QualifiedBufferDeviceKind::G, 0x03E0U, 20U),
           QualifiedBufferWordDevice(QualifiedBufferDeviceKind::HG, 0x03E0U, 20U)}) {
    byte_count = 99U;
    const Status invalid_status = make_qualified_buffer_write_words_request(
        invalid, words, byte_storage, request, byte_count);
    assert(invalid_status.code == StatusCode::UnsupportedConfiguration);
    assert(byte_count == 0U);
  }
}

void test_decode_qualified_buffer_word_values_decodes_little_endian_bytes() {
  const std::array<mcprotocol::serial::Byte, 4> bytes {
      mcprotocol::serial::Byte {0x34},
      mcprotocol::serial::Byte {0x12},
      mcprotocol::serial::Byte {0xCD},
      mcprotocol::serial::Byte {0xAB},
  };
  std::array<std::uint16_t, 2> words {};

  const Status status = decode_qualified_buffer_word_values(bytes, words);
  assert(status.ok());
  assert(words[0] == 0x1234U);
  assert(words[1] == 0xABCDU);
}

struct CallbackCapture {
  bool called = false;
  Status status {};
};

void completion_callback(void* user, Status status) {
  auto* capture = static_cast<CallbackCapture*>(user);
  capture->called = true;
  capture->status = status;
}

void test_c1_physical_profile_rejections_do_not_start_client_requests() {
  const auto config = make_ascii_c1_format4_qna_config();
  MelsecSerialClient client;
  assert(client.configure(config).ok());
  CallbackCapture capture {};
  std::array<std::uint16_t, 1> words {};

  Status status = client.async_direct_read_extended_file_register_words(
      0U,
      ExtendedFileRegisterDirectBatchReadWordsRequest(100U, 1U),
      words,
      completion_callback,
      &capture);
  assert(status.code == StatusCode::UnsupportedConfiguration);
  assert(client.pending_tx_frame().empty());
  assert(!client.busy());

  const std::array<std::uint16_t, 1> write_words {0x1234U};
  status = client.async_direct_write_extended_file_register_words(
      0U,
      ExtendedFileRegisterDirectBatchWriteWordsRequest(100U, write_words),
      completion_callback,
      &capture);
  assert(status.code == StatusCode::UnsupportedConfiguration);
  assert(client.pending_tx_frame().empty());
  assert(!client.busy());

  mcprotocol::serial::highlevel::BitInWordWriteOperation operation;
  status = operation.begin_direct_extended_file_register(
      client, 0U, 100U, 0, true, completion_callback, &capture);
  assert(status.code == StatusCode::UnsupportedConfiguration);
  assert(client.pending_tx_frame().empty());
  assert(!client.busy());
  assert(!operation.busy());

  std::array<mcprotocol::serial::Byte, 4> bytes {};
  status = client.async_read_module_buffer(
      0U,
      ModuleBufferReadRequest(0x100U, 4U, 1U),
      bytes,
      completion_callback,
      &capture);
  assert(status.code == StatusCode::UnsupportedConfiguration);
  assert(client.pending_tx_frame().empty());
  assert(!client.busy());

  status = client.async_write_module_buffer(
      0U,
      ModuleBufferWriteRequest(0x100U, 1U, bytes),
      completion_callback,
      &capture);
  assert(status.code == StatusCode::UnsupportedConfiguration);
  assert(client.pending_tx_frame().empty());
  assert(!client.busy());
  assert(!capture.called);
}

void configure_sync_client_without_open_transport(
    HostSyncClient& client,
    const ProtocolConfig& config) {
  const HostSerialConfig serial(
      ".", 19200U, 7U, 1U, SerialParity::Even, HardwareFlowControl::None);
  const Status open_status = client.open(serial, config);
  assert(!open_status.ok());
  assert(!client.is_open());
}

void test_c1_direct_extended_file_register_sync_profile_paths() {
  const auto qna_config = make_ascii_c1_format4_qna_config();
  const ExtendedFileRegisterDirectBatchReadWordsRequest read_request(100U, 1U);
  const std::array<std::uint16_t, 1> write_words {0x1234U};
  const ExtendedFileRegisterDirectBatchWriteWordsRequest write_request(100U, write_words);
  std::array<std::uint16_t, 1> read_words {};

  HostSyncClient qna_client;
  configure_sync_client_without_open_transport(qna_client, qna_config);
  Status status = qna_client.read_direct_extended_file_register_words(
      read_request, read_words);
  assert(status.code == StatusCode::UnsupportedConfiguration);
  assert(!qna_client.is_open());
  status = qna_client.write_direct_extended_file_register_words(write_request);
  assert(status.code == StatusCode::UnsupportedConfiguration);
  assert(!qna_client.is_open());
  status = qna_client.write_direct_extended_file_register_bit_in_word(
      100U, 0, true);
  assert(status.code == StatusCode::UnsupportedConfiguration);
  assert(!qna_client.is_open());

  HostSyncClient ana_client;
  configure_sync_client_without_open_transport(
      ana_client, qna_config.with_plc_profile(PlcProfile::MelsecAnAAnU));
  status = ana_client.read_direct_extended_file_register_words(
      read_request, read_words);
  assert(status.code == StatusCode::NotConnected);
  status = ana_client.write_direct_extended_file_register_words(write_request);
  assert(status.code == StatusCode::NotConnected);
  status = ana_client.write_direct_extended_file_register_bit_in_word(
      100U, 0, true);
  assert(status.code == StatusCode::NotConnected);
}

void test_native_qualified_sync_hg_validation_precedes_transport() {
  const auto config = make_binary_c4_iqr_config();
  const std::array<std::uint16_t, 1> write_words {0x1234U};
  std::array<std::uint16_t, 1> read_words {};

  HostSyncClient client;
  configure_sync_client_without_open_transport(client, config);
  for (const std::string_view invalid : {
           std::string_view {"U0\\HG0"},
           std::string_view {"U3DF\\HG0"},
           std::string_view {"U3E4\\HG0"}}) {
    Status status = client.read_qualified_buffer_words(invalid, 1U, read_words);
    assert(status.code == StatusCode::InvalidArgument);
    assert(!client.is_open());
    status = client.write_qualified_buffer_words(invalid, write_words);
    assert(status.code == StatusCode::InvalidArgument);
    assert(!client.is_open());
    status = client.write_qualified_buffer_bit_in_word(invalid, 0, true);
    assert(status.code == StatusCode::InvalidArgument);
    assert(!client.is_open());
  }

  Status status = client.read_qualified_buffer_words("U3E0\\HG0", 1U, read_words);
  assert(status.code == StatusCode::NotConnected);
  status = client.write_qualified_buffer_words("U3E3\\HG0", write_words);
  assert(status.code == StatusCode::NotConnected);
  status = client.write_qualified_buffer_bit_in_word("U3E0\\HG0", 0, true);
  assert(status.code == StatusCode::NotConnected);
}

void test_bit_in_word_operation_contract() {
  auto config = make_binary_c4_config();
  config = config.with_response_timeout_ms(10U);

  const auto deliver_success = [&config](
                                   MelsecSerialClient& client,
                                   std::uint32_t now_ms,
                                   mcprotocol::serial::Span<const std::uint8_t> data) {
    std::array<std::uint8_t, 64U> frame {};
    std::size_t frame_size = 0U;
    const Status status = FrameCodec::encode_success_response(
        config, data, frame, frame_size);
    assert(status.ok());
    client.on_rx_bytes(
        now_ms,
        mcprotocol::serial::Span<const mcprotocol::serial::Byte>(
            reinterpret_cast<const mcprotocol::serial::Byte*>(frame.data()),
            frame_size));
  };

  {
    MelsecSerialClient client;
    assert(client.configure(config).ok());
    mcprotocol::serial::highlevel::BitInWordWriteOperation operation;
    CallbackCapture capture {};

    Status status = operation.begin(
        client, 100U, "D100", 3, true, completion_callback, &capture);
    assert(status.ok());
    assert(operation.busy());
    assert(client.busy());
    assert(client.notify_tx_started(100U).ok());
    assert(client.notify_tx_complete(100U, mcprotocol::serial::ok_status()).ok());

    const std::array<std::uint8_t, 2U> read_data {0x34U, 0x12U};
    deliver_success(client, 105U, read_data);
    assert(!capture.called);
    assert(operation.busy());
    assert(client.busy());

    const std::uint16_t expected_word = 0x123CU;
    std::array<std::uint8_t, 64U> expected_data {};
    std::size_t expected_data_size = 0U;
    status = CommandCodec::encode_batch_write_words(
        config,
        BatchWriteWordsRequest(
            DeviceAddress {mcprotocol::serial::DeviceCode::D, 100U},
            mcprotocol::serial::Span<const std::uint16_t>(&expected_word, 1U)),
        expected_data,
        expected_data_size);
    assert(status.ok());
    std::array<std::uint8_t, 128U> expected_frame {};
    std::size_t expected_frame_size = 0U;
    status = FrameCodec::encode_request(
        config,
        mcprotocol::serial::Span<const std::uint8_t>(
            expected_data.data(), expected_data_size),
        expected_frame,
        expected_frame_size);
    assert(status.ok());
    const auto pending = client.pending_tx_frame();
    assert(pending.size() == expected_frame_size);
    assert(std::memcmp(pending.data(), expected_frame.data(), expected_frame_size) == 0);

    assert(client.notify_tx_started(106U).ok());
    assert(client.notify_tx_complete(106U, mcprotocol::serial::ok_status()).ok());
    deliver_success(client, 107U, {});
    assert(capture.called);
    assert(capture.status.ok());
    assert(!operation.busy());
    assert(!client.busy());
  }

  {
    MelsecSerialClient client;
    assert(client.configure(config).ok());
    mcprotocol::serial::highlevel::BitInWordWriteOperation operation;
    CallbackCapture capture {};

    Status status = operation.begin(
        client, 100U, "M100", 3, true, completion_callback, &capture);
    assert(status.code == StatusCode::InvalidArgument);
    assert(!operation.busy());
    assert(!client.busy());
    assert(client.pending_tx_frame().empty());
    assert(!capture.called);

    status = operation.begin(
        client, 100U, "D100", 16, true, completion_callback, &capture);
    assert(status.code == StatusCode::InvalidArgument);
    assert(!operation.busy());
    assert(!client.busy());
    assert(client.pending_tx_frame().empty());
    assert(!capture.called);

    for (const std::string_view device : {"LTN0", "LSTN0", "LCN0", "LZ0"}) {
      status = operation.begin(
          client, 100U, device, 0, true, completion_callback, &capture);
      assert(status.code == StatusCode::InvalidArgument);
      assert(!operation.busy());
      assert(!client.busy());
      assert(client.pending_tx_frame().empty());
      assert(!capture.called);
    }
  }

  {
    MelsecSerialClient client;
    assert(client.configure(config).ok());
    mcprotocol::serial::highlevel::BitInWordWriteOperation operation;
    CallbackCapture capture {};
    Status status = operation.begin(
        client, 100U, "D100", 0, false, completion_callback, &capture);
    assert(status.ok());
    assert(client.notify_tx_started(100U).ok());
    assert(client.notify_tx_complete(100U, mcprotocol::serial::ok_status()).ok());
    const std::array<std::uint8_t, 2U> read_data {0x01U, 0x00U};
    deliver_success(client, 109U, read_data);
    assert(client.busy());
    status = client.notify_tx_started(110U);
    assert(status.code == StatusCode::Timeout);
    assert(capture.called);
    assert(capture.status.code == StatusCode::Timeout);
    assert(!operation.busy());
    assert(!client.busy());
    assert(!client.requires_transport_reset());
  }

  {
    MelsecSerialClient client;
    assert(client.configure(config).ok());
    mcprotocol::serial::highlevel::BitInWordWriteOperation operation;
    CallbackCapture capture {};
    Status status = operation.begin(
        client, 100U, "D100", 0, true, completion_callback, &capture);
    assert(status.ok());
    assert(client.notify_tx_started(100U).ok());
    assert(client.notify_tx_complete(100U, mcprotocol::serial::ok_status()).ok());
    const std::array<std::uint8_t, 2U> read_data {0x00U, 0x00U};
    deliver_success(client, 101U, read_data);
    operation.cancel();
    assert(capture.called);
    assert(capture.status.code == StatusCode::Cancelled);
    assert(!operation.busy());
    assert(!client.busy());
    assert(!client.requires_transport_reset());
  }
}

void test_bit_in_word_operation_covers_every_complete_word_route() {
  const auto exercise = [](
                            const ProtocolConfig& config,
                            auto begin_operation,
                            auto encode_expected_read,
                            auto encode_expected_write) {
    MelsecSerialClient client;
    assert(client.configure(config).ok());
    mcprotocol::serial::highlevel::BitInWordWriteOperation operation;
    CallbackCapture capture {};
    Status status = begin_operation(operation, client, capture);
    assert(status.ok());

    std::array<std::uint8_t, 96U> request_data {};
    std::size_t request_data_size = 0U;
    status = encode_expected_read(config, request_data, request_data_size);
    assert(status.ok());
    std::array<std::uint8_t, 160U> expected_frame {};
    std::size_t expected_frame_size = 0U;
    status = FrameCodec::encode_request(
        config,
        mcprotocol::serial::Span<const std::uint8_t>(
            request_data.data(), request_data_size),
        expected_frame,
        expected_frame_size);
    assert(status.ok());
    auto pending = client.pending_tx_frame();
    assert(pending.size() == expected_frame_size);
    assert(std::memcmp(pending.data(), expected_frame.data(), expected_frame_size) == 0);

    assert(client.notify_tx_started(100U).ok());
    assert(client.notify_tx_complete(100U, mcprotocol::serial::ok_status()).ok());

    std::array<std::uint8_t, 64U> response {};
    std::size_t response_size = 0U;
    const std::array<std::uint8_t, 4U> ascii_read_data {'1', '2', '0', '0'};
    const std::array<std::uint8_t, 2U> binary_read_data {0x00U, 0x12U};
    const auto read_data = config.code_mode() == CodeMode::Ascii
                               ? mcprotocol::serial::Span<const std::uint8_t>(ascii_read_data)
                               : mcprotocol::serial::Span<const std::uint8_t>(binary_read_data);
    status = FrameCodec::encode_success_response(config, read_data, response, response_size);
    assert(status.ok());
    client.on_rx_bytes(
        101U,
        mcprotocol::serial::Span<const mcprotocol::serial::Byte>(
            reinterpret_cast<const mcprotocol::serial::Byte*>(response.data()),
            response_size));
    assert(!capture.called);
    assert(client.busy());

    const std::uint16_t expected_word = 0x1208U;
    request_data_size = 0U;
    status = encode_expected_write(config, expected_word, request_data, request_data_size);
    assert(status.ok());
    expected_frame_size = 0U;
    status = FrameCodec::encode_request(
        config,
        mcprotocol::serial::Span<const std::uint8_t>(
            request_data.data(), request_data_size),
        expected_frame,
        expected_frame_size);
    assert(status.ok());
    pending = client.pending_tx_frame();
    assert(pending.size() == expected_frame_size);
    assert(std::memcmp(pending.data(), expected_frame.data(), expected_frame_size) == 0);

    assert(client.notify_tx_started(102U).ok());
    assert(client.notify_tx_complete(102U, mcprotocol::serial::ok_status()).ok());
    response_size = 0U;
    assert(FrameCodec::encode_success_response(config, {}, response, response_size).ok());
    client.on_rx_bytes(
        103U,
        mcprotocol::serial::Span<const mcprotocol::serial::Byte>(
            reinterpret_cast<const mcprotocol::serial::Byte*>(response.data()),
            response_size));
    assert(capture.called);
    assert(capture.status.ok());
    assert(!operation.busy());
    assert(!client.busy());
  };

  const ExtendedFileRegisterAddress extended_device {2U, 70U};
  exercise(
      make_ascii_c1_format4_a_config(),
      [extended_device](auto& operation, auto& client, auto& capture) {
        return operation.begin_extended_file_register(
            client, 100U, extended_device, 3, true, completion_callback, &capture);
      },
      [extended_device](const auto& config, auto& out, auto& out_size) {
        return CommandCodec::encode_read_extended_file_register_words(
            config,
            ExtendedFileRegisterBatchReadWordsRequest(extended_device, 1U),
            out,
            out_size);
      },
      [extended_device](const auto& config, std::uint16_t word, auto& out, auto& out_size) {
        const ExtendedFileRegisterBatchWriteWordsRequest request(
            extended_device,
            mcprotocol::serial::Span<const std::uint16_t>(&word, 1U));
        return CommandCodec::encode_write_extended_file_register_words(
            config, request, out, out_size);
      });

  constexpr std::uint32_t direct_device = 1234U;
  exercise(
      make_ascii_c1_format4_qna_config().with_plc_profile(PlcProfile::MelsecAnAAnU),
      [](auto& operation, auto& client, auto& capture) {
        return operation.begin_direct_extended_file_register(
            client, 100U, direct_device, 3, true, completion_callback, &capture);
      },
      [](const auto& config, auto& out, auto& out_size) {
        return CommandCodec::encode_direct_read_extended_file_register_words(
            config,
            ExtendedFileRegisterDirectBatchReadWordsRequest(direct_device, 1U),
            out,
            out_size);
      },
      [](const auto& config, std::uint16_t word, auto& out, auto& out_size) {
        const ExtendedFileRegisterDirectBatchWriteWordsRequest request(
            direct_device,
            mcprotocol::serial::Span<const std::uint16_t>(&word, 1U));
        return CommandCodec::encode_direct_write_extended_file_register_words(
            config, request, out, out_size);
      });

  const LinkDirectDevice link_device(
      1U,
      DeviceAddress {mcprotocol::serial::DeviceCode::W, 0x100U});
  exercise(
      make_binary_c4_iqr_config(),
      [link_device](auto& operation, auto& client, auto& capture) {
        return operation.begin_link_direct(
            client, 100U, link_device, 3, true, completion_callback, &capture);
      },
      [link_device](const auto& config, auto& out, auto& out_size) {
        return CommandCodec::encode_link_direct_batch_read_words(
            config, link_device, 1U, out, out_size);
      },
      [link_device](const auto& config, std::uint16_t word, auto& out, auto& out_size) {
        return CommandCodec::encode_link_direct_batch_write_words(
            config,
            link_device,
            mcprotocol::serial::Span<const std::uint16_t>(&word, 1U),
            out,
            out_size);
      });

  const QualifiedBufferWordDevice qualified_device(
      QualifiedBufferDeviceKind::G,
      0x03E0U,
      10U);
  exercise(
      make_binary_c4_iqr_config(),
      [qualified_device](auto& operation, auto& client, auto& capture) {
        return operation.begin_qualified_buffer(
            client, 100U, qualified_device, 3, true, completion_callback, &capture);
      },
      [qualified_device](const auto& config, auto& out, auto& out_size) {
        return CommandCodec::encode_extended_batch_read_words(
            config, qualified_device, 1U, out, out_size);
      },
      [qualified_device](const auto& config, std::uint16_t word, auto& out, auto& out_size) {
        return CommandCodec::encode_extended_batch_write_words(
            config,
            qualified_device,
            mcprotocol::serial::Span<const std::uint16_t>(&word, 1U),
            out,
            out_size);
      });
}

void test_bit_in_word_sync_surface_covers_every_complete_word_route() {
  using Client = mcprotocol::serial::HostSyncClient;
  using TextSignature = Status (Client::*)(std::string_view, int, bool) noexcept;
  using ExtendedSignature = Status (Client::*)(ExtendedFileRegisterAddress, int, bool) noexcept;
  using DirectExtendedSignature = Status (Client::*)(std::uint32_t, int, bool) noexcept;

  const TextSignature ordinary = &Client::write_bit_in_word;
  const ExtendedSignature extended = &Client::write_extended_file_register_bit_in_word;
  const DirectExtendedSignature direct_extended =
      &Client::write_direct_extended_file_register_bit_in_word;
  const TextSignature link_direct = &Client::write_link_direct_bit_in_word;
  const TextSignature qualified = &Client::write_qualified_buffer_bit_in_word;
  assert(ordinary != nullptr);
  assert(extended != nullptr);
  assert(direct_extended != nullptr);
  assert(link_direct != nullptr);
  assert(qualified != nullptr);
}

void test_host_single_request_surface_and_deprecated_delegates() {
  using Client = mcprotocol::serial::HostSyncClient;
  using ReadWordsSignature = Status (Client::*)(
      std::string_view,
      std::uint16_t,
      mcprotocol::serial::Span<std::uint16_t>) noexcept;
  using ReadBitsSignature = Status (Client::*)(
      std::string_view,
      std::uint16_t,
      mcprotocol::serial::Span<BitValue>) noexcept;
  using WriteWordsSignature = Status (Client::*)(
      std::string_view,
      mcprotocol::serial::Span<const std::uint16_t>) noexcept;
  using WriteBitsSignature = Status (Client::*)(
      std::string_view,
      mcprotocol::serial::Span<const BitValue>) noexcept;

  const ReadWordsSignature read_words = &Client::read_words_single_request;
  const ReadBitsSignature read_bits = &Client::read_bits_single_request;
  const WriteWordsSignature write_words = &Client::write_words_single_request;
  const WriteBitsSignature write_bits = &Client::write_bits_single_request;
  assert(read_words != nullptr);
  assert(read_bits != nullptr);
  assert(write_words != nullptr);
  assert(write_bits != nullptr);

  Client client;
  std::array<std::uint16_t, 1> words {};
  std::array<BitValue, 1> bits {};
  const Status canonical_word_read = client.read_words_single_request("invalid", 1U, words);
  const Status canonical_bit_read = client.read_bits_single_request("invalid", 1U, bits);
  const Status canonical_word_write = client.write_words_single_request("invalid", words);
  const Status canonical_bit_write = client.write_bits_single_request("invalid", bits);
  assert(canonical_word_read.code == StatusCode::InvalidArgument);
  assert(canonical_bit_read.code == StatusCode::InvalidArgument);
  assert(canonical_word_write.code == StatusCode::InvalidArgument);
  assert(canonical_bit_write.code == StatusCode::InvalidArgument);

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#elif defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4996)
#endif
  const Status legacy_word_read = client.read_words("invalid", 1U, words);
  const Status legacy_bit_read = client.read_bits("invalid", 1U, bits);
  const Status legacy_word_write = client.write_words("invalid", words);
  const Status legacy_bit_write = client.write_bits("invalid", bits);
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#pragma warning(pop)
#endif
  assert(legacy_word_read.code == canonical_word_read.code);
  assert(legacy_bit_read.code == canonical_bit_read.code);
  assert(legacy_word_write.code == canonical_word_write.code);
  assert(legacy_bit_write.code == canonical_bit_write.code);
}

void test_client_receive_failure_preserves_transport_status() {
  const auto config = make_binary_c4_config();
  MelsecSerialClient client;
  Status status = client.configure(config);
  assert(status.ok());

  status = client.notify_rx_failure(
      mcprotocol::serial::make_status(StatusCode::Transport, "no active response wait"));
  assert(status.code == StatusCode::InvalidArgument);

  const std::array<std::uint16_t, 1> write_values {0x1234U};
  CallbackCapture write_capture {};
  status = client.async_batch_write_words(
      0U,
      BatchWriteWordsRequest(
          DeviceAddress {mcprotocol::serial::DeviceCode::D, 100U}, write_values),
      completion_callback,
      &write_capture);
  assert(status.ok());
  status = client.notify_rx_failure(
      mcprotocol::serial::make_status(StatusCode::Transport, "TX is not complete"));
  assert(status.code == StatusCode::InvalidArgument);
  assert(client.busy());
  assert(!write_capture.called);

  assert(start_and_notify_tx_complete(client, 1U, mcprotocol::serial::ok_status()).ok());
  status = client.notify_rx_failure(mcprotocol::serial::ok_status());
  assert(status.code == StatusCode::InvalidArgument);
  assert(client.busy());
  assert(!write_capture.called);

  status = client.notify_rx_failure(
      mcprotocol::serial::make_status(StatusCode::Transport, "simulated receive failure"));
  assert(status.ok());
  assert(write_capture.called);
  assert(write_capture.status.code == StatusCode::OperationOutcomeUnknown);
  assert(write_capture.status.cause == StatusCode::Transport);
  assert(!client.busy());
  assert(client.requires_transport_reset());

  assert(client.configure(config).ok());
  std::array<std::uint16_t, 1> read_values {};
  CallbackCapture read_capture {};
  status = client.async_batch_read_words(
      10U,
      BatchReadWordsRequest(DeviceAddress {mcprotocol::serial::DeviceCode::D, 100U}, 1U),
      read_values,
      completion_callback,
      &read_capture);
  assert(status.ok());
  assert(start_and_notify_tx_complete(client, 11U, mcprotocol::serial::ok_status()).ok());
  status = client.notify_rx_failure(
      mcprotocol::serial::make_status(StatusCode::Timeout, "simulated receive timeout"));
  assert(status.ok());
  assert(read_capture.called);
  assert(read_capture.status.code == StatusCode::Timeout);
  assert(read_capture.status.cause == StatusCode::Ok);
  assert(!client.busy());
  assert(client.requires_transport_reset());
}

void test_client_busy_rejection_preserves_active_request_state() {
  const auto config = make_binary_c4_config();
  MelsecSerialClient client;
  Status status = client.configure(config);
  assert(status.ok());

  const BatchReadWordsRequest active_request(
      DeviceAddress {mcprotocol::serial::DeviceCode::D, 100U}, 1U);
  const BatchReadWordsRequest rejected_request(
      DeviceAddress {mcprotocol::serial::DeviceCode::D, 200U}, 1U);
  std::array<std::uint16_t, 1> active_words {0xAAAAU};
  std::array<std::uint16_t, 1> rejected_words {0xBBBBU};
  CallbackCapture active_capture {};
  CallbackCapture rejected_capture {};

  status = client.async_batch_read_words(
      0U, active_request, active_words, completion_callback, &active_capture);
  assert(status.ok());
  const std::vector<mcprotocol::serial::Byte> active_tx(
      client.pending_tx_frame().begin(), client.pending_tx_frame().end());

  status = client.async_batch_read_words(
      1U, rejected_request, rejected_words, completion_callback, &rejected_capture);
  assert(status.code == StatusCode::Busy);
  assert(!rejected_capture.called);
  assert(std::equal(
      active_tx.begin(), active_tx.end(), client.pending_tx_frame().begin(), client.pending_tx_frame().end()));

  CpuModelInfo rejected_model {};
  status = client.async_read_cpu_model(
      2U, rejected_model, completion_callback, &rejected_capture);
  assert(status.code == StatusCode::Busy);
  assert(!rejected_capture.called);

  const std::array<std::uint16_t, 1> rejected_write_values {0xCAFEU};
  status = client.async_batch_write_words(
      2U,
      BatchWriteWordsRequest(
          DeviceAddress {mcprotocol::serial::DeviceCode::D, 300U},
          rejected_write_values),
      completion_callback,
      &rejected_capture);
  assert(status.code == StatusCode::Busy);
  assert(!rejected_capture.called);
  status = client.async_remote_stop(2U, completion_callback, &rejected_capture);
  assert(status.code == StatusCode::Busy);
  assert(!rejected_capture.called);
  assert(std::equal(
      active_tx.begin(), active_tx.end(), client.pending_tx_frame().begin(), client.pending_tx_frame().end()));

  status = start_and_notify_tx_complete(client, 3U, mcprotocol::serial::ok_status());
  assert(status.ok());
  const std::array<std::uint8_t, 2> response_data {0x34U, 0x12U};
  std::array<std::uint8_t, 64> response_frame {};
  std::size_t response_size = 0U;
  status = FrameCodec::encode_success_response(
      config, response_data, response_frame, response_size);
  assert(status.ok());
  client.on_rx_bytes(
      4U,
      mcprotocol::serial::Span<const mcprotocol::serial::Byte>(
          reinterpret_cast<const mcprotocol::serial::Byte*>(response_frame.data()), response_size));
  assert(active_capture.called);
  assert(active_capture.status.ok());
  assert(active_words[0] == 0x1234U);
  assert(rejected_words[0] == 0xBBBBU);
  assert(!rejected_capture.called);
}

void test_client_instances_have_independent_in_flight_state() {
  const auto config = make_binary_c4_config();
  MelsecSerialClient first;
  MelsecSerialClient second;
  assert(first.configure(config).ok());
  assert(second.configure(config).ok());

  const BatchReadWordsRequest first_request(
      DeviceAddress {mcprotocol::serial::DeviceCode::D, 100U}, 1U);
  const BatchReadWordsRequest second_request(
      DeviceAddress {mcprotocol::serial::DeviceCode::D, 200U}, 1U);
  std::array<std::uint16_t, 1> first_words {};
  std::array<std::uint16_t, 1> second_words {};
  CallbackCapture first_capture {};
  CallbackCapture second_capture {};
  assert(first.async_batch_read_words(
      0U, first_request, first_words, completion_callback, &first_capture).ok());
  assert(second.async_batch_read_words(
      0U, second_request, second_words, completion_callback, &second_capture).ok());
  assert(first.busy());
  assert(second.busy());
  assert(first.notify_tx_started(1U).ok());
  assert(second.notify_tx_started(1U).ok());
  assert(start_and_notify_tx_complete(first, 2U, mcprotocol::serial::ok_status()).ok());
  assert(start_and_notify_tx_complete(second, 2U, mcprotocol::serial::ok_status()).ok());

  const std::array<std::uint8_t, 2> first_data {0x34U, 0x12U};
  const std::array<std::uint8_t, 2> second_data {0x78U, 0x56U};
  std::array<std::uint8_t, 64> first_frame {};
  std::array<std::uint8_t, 64> second_frame {};
  std::size_t first_size = 0U;
  std::size_t second_size = 0U;
  assert(FrameCodec::encode_success_response(config, first_data, first_frame, first_size).ok());
  assert(FrameCodec::encode_success_response(config, second_data, second_frame, second_size).ok());
  first.on_rx_bytes(
      3U,
      mcprotocol::serial::Span<const mcprotocol::serial::Byte>(
          reinterpret_cast<const mcprotocol::serial::Byte*>(first_frame.data()), first_size));
  assert(first_capture.called);
  assert(!second_capture.called);
  second.on_rx_bytes(
      3U,
      mcprotocol::serial::Span<const mcprotocol::serial::Byte>(
          reinterpret_cast<const mcprotocol::serial::Byte*>(second_frame.data()), second_size));
  assert(second_capture.called);
  assert(first_words[0] == 0x1234U);
  assert(second_words[0] == 0x5678U);
}

void test_client_all_state_changes_report_ambiguous_outcomes() {
  auto config = make_binary_c4_config();
  config = config.with_response_timeout_ms(10U);
  MelsecSerialClient client;
  Status status = client.configure(config);
  assert(status.ok());

  const std::array<std::uint16_t, 1> values {0x1234U};
  CallbackCapture write_capture {};
  status = client.async_batch_write_words(
      0U,
      BatchWriteWordsRequest(
          DeviceAddress {mcprotocol::serial::DeviceCode::D, 100U}, values),
      completion_callback,
      &write_capture);
  assert(status.ok());
  assert(start_and_notify_tx_complete(client, 1U, mcprotocol::serial::ok_status()).ok());
  client.poll(11U);
  assert(write_capture.called);
  assert(write_capture.status.code == StatusCode::OperationOutcomeUnknown);
  assert(write_capture.status.cause == StatusCode::Timeout);
  assert(client.requires_transport_reset());

  status = client.configure(config);
  assert(status.ok());
  CallbackCapture stop_capture {};
  status = client.async_remote_stop(20U, completion_callback, &stop_capture);
  assert(status.ok());
  status = start_and_notify_tx_complete(client,
      21U,
      mcprotocol::serial::make_status(StatusCode::Transport, "simulated TX failure"));
  assert(status.ok());
  assert(stop_capture.called);
  assert(stop_capture.status.code == StatusCode::OperationOutcomeUnknown);
  assert(stop_capture.status.cause == StatusCode::Transport);

  status = client.configure(config);
  assert(status.ok());
  CallbackCapture closed_capture {};
  status = client.async_remote_stop(25U, completion_callback, &closed_capture);
  assert(status.ok());
  status = start_and_notify_tx_complete(
      client,
      26U,
      mcprotocol::serial::make_status(StatusCode::Closed, "simulated local close during TX"));
  assert(status.ok());
  assert(closed_capture.called);
  assert(closed_capture.status.code == StatusCode::OperationOutcomeUnknown);
  assert(closed_capture.status.cause == StatusCode::Closed);

  status = client.configure(config);
  assert(status.ok());
  CallbackCapture reset_capture {};
  status = client.async_remote_reset(30U, completion_callback, &reset_capture);
  assert(status.ok());
  assert(client.notify_tx_started(30U).ok());
  client.cancel();
  assert(!reset_capture.called);
  assert(start_and_notify_tx_complete(client, 31U, mcprotocol::serial::ok_status()).ok());
  assert(reset_capture.called);
  assert(reset_capture.status.ok());
  assert(!client.requires_transport_reset());
}

void test_not_connected_is_distinct_from_transport_failure() {
  MelsecSerialClient client;
  CpuModelInfo info {};
  CallbackCapture capture {};
  Status status = client.async_read_cpu_model(0U, info, completion_callback, &capture);
  assert(status.code == StatusCode::NotConnected);
  assert(!client.busy());
  assert(!capture.called);

  mcprotocol::serial::HostSerialPort port;
  std::array<mcprotocol::serial::Byte, 1> bytes {};
  std::size_t received = 99U;
  status = port.write_all_until(bytes, 1U);
  assert(status.code == StatusCode::NotConnected);
  status = port.read_some_until(bytes, 1U, received);
  assert(status.code == StatusCode::NotConnected);
  assert(received == 0U);
  status = port.drain_tx_until(1U);
  assert(status.code == StatusCode::NotConnected);
}

void test_client_unsequenced_decode_failures_require_transport_reset() {
  auto config = test_config_with_sum_check(
      make_ascii_c4_format4_config(), SumCheckMode::Enabled);
  MelsecSerialClient client;
  Status status = client.configure(config);
  assert(status.ok());

  const BatchReadWordsRequest request(
      DeviceAddress {mcprotocol::serial::DeviceCode::D, 100U}, 1U);
  std::array<std::uint16_t, 1> words {};
  CallbackCapture decode_capture {};
  status = client.async_batch_read_words(
      0U, request, words, completion_callback, &decode_capture);
  assert(status.ok());
  assert(start_and_notify_tx_complete(client, 1U, mcprotocol::serial::ok_status()).ok());

  const std::array<std::uint8_t, 4> response_data {'1', '2', '3', '4'};
  std::array<std::uint8_t, 64> response_frame {};
  std::size_t response_size = 0U;
  status = FrameCodec::encode_success_response(
      config, response_data, response_frame, response_size);
  assert(status.ok());
  assert(response_size > 4U);
  response_frame[response_size - 4U] =
      response_frame[response_size - 4U] == '0' ? '1' : '0';
  client.on_rx_bytes(
      2U,
      mcprotocol::serial::Span<const mcprotocol::serial::Byte>(
          reinterpret_cast<const mcprotocol::serial::Byte*>(response_frame.data()), response_size));
  assert(decode_capture.called);
  assert(!decode_capture.status.ok());
  assert(client.requires_transport_reset());

  status = client.configure(config);
  assert(status.ok());
  CallbackCapture overflow_capture {};
  status = client.async_batch_read_words(
      10U, request, words, completion_callback, &overflow_capture);
  assert(status.ok());
  assert(start_and_notify_tx_complete(client, 11U, mcprotocol::serial::ok_status()).ok());
  std::array<mcprotocol::serial::Byte, mcprotocol::serial::kMaxResponseFrameBytes + 1U> overflow {};
  client.on_rx_bytes(12U, overflow);
  assert(overflow_capture.called);
  assert(overflow_capture.status.code == StatusCode::BufferTooSmall);
  assert(client.requires_transport_reset());
}

struct Rs485HookCapture {
  std::size_t begin_count = 0U;
  std::size_t end_count = 0U;
};

struct CountingCallbackCapture {
  std::size_t call_count = 0U;
  Status status {};
};

void counting_completion_callback(void* user, Status status) {
  auto* capture = static_cast<CountingCallbackCapture*>(user);
  ++capture->call_count;
  capture->status = status;
}

void rs485_tx_begin(void* user) {
  if (user != nullptr) {
    ++static_cast<Rs485HookCapture*>(user)->begin_count;
  }
}

void rs485_tx_end(void* user) {
  if (user != nullptr) {
    ++static_cast<Rs485HookCapture*>(user)->end_count;
  }
}

void test_client_rs485_hooks_and_tx_completion_lifecycle() {
  static_assert(!CanNotifyTxCompleteWithoutStatus<MelsecSerialClient>);

  MelsecSerialClient client;
  Status status = client.configure(make_binary_c4_config());
  assert(status.ok());

  status = client.set_rs485_hooks(Rs485Hooks {rs485_tx_begin});
  assert(status.code == StatusCode::InvalidArgument);
  status = client.set_rs485_hooks(Rs485Hooks {nullptr, rs485_tx_end, nullptr});
  assert(status.code == StatusCode::InvalidArgument);
  status = client.set_rs485_hooks(Rs485Hooks {
      rs485_tx_begin,
      rs485_tx_end,
      nullptr,
  });
  assert(status.ok());
  status = client.set_rs485_hooks(Rs485Hooks {});
  assert(status.ok());

  Rs485HookCapture active_hooks {};
  status = client.set_rs485_hooks(Rs485Hooks {
      rs485_tx_begin,
      rs485_tx_end,
      &active_hooks,
  });
  assert(status.ok());

  const BatchReadWordsRequest request({mcprotocol::serial::DeviceCode::D, 100U}, 1U);
  std::array<std::uint16_t, 1> words {};
  CallbackCapture cancelled_capture {};
  status = client.async_batch_read_words(
      0U, request, words, completion_callback, &cancelled_capture);
  assert(status.ok());
  assert(client.notify_tx_started(0U).ok());
  assert(active_hooks.begin_count == 1U);
  assert(active_hooks.end_count == 0U);

  Rs485HookCapture replacement_hooks {};
  status = client.set_rs485_hooks(Rs485Hooks {
      rs485_tx_begin,
      rs485_tx_end,
      &replacement_hooks,
  });
  assert(status.code == StatusCode::Busy);

  client.cancel();
  assert(client.busy());
  assert(!cancelled_capture.called);
  assert(active_hooks.end_count == 0U);
  status = start_and_notify_tx_complete(client, 1U, mcprotocol::serial::ok_status());
  assert(status.ok());
  assert(!client.busy());
  assert(cancelled_capture.called);
  assert(cancelled_capture.status.code == StatusCode::Cancelled);
  assert(active_hooks.begin_count == 1U);
  assert(active_hooks.end_count == 1U);
  assert(replacement_hooks.begin_count == 0U);
  assert(replacement_hooks.end_count == 0U);
  assert(client.requires_transport_reset());

  status = start_and_notify_tx_complete(client, 2U, mcprotocol::serial::ok_status());
  assert(status.code == StatusCode::InvalidArgument);
  assert(active_hooks.end_count == 1U);

  status = client.configure(make_binary_c4_config());
  assert(status.ok());
  status = client.set_rs485_hooks(Rs485Hooks {});
  assert(status.ok());
  CallbackCapture hookless_capture {};
  status = client.async_batch_read_words(
      3U, request, words, completion_callback, &hookless_capture);
  assert(status.ok());
  status = start_and_notify_tx_complete(client,
      4U,
      mcprotocol::serial::make_status(StatusCode::Transport, "simulated TX failure"));
  assert(status.ok());
  assert(hookless_capture.called);
  assert(hookless_capture.status.code == StatusCode::Transport);
  assert(client.requires_transport_reset());
}

void test_tx_deadline_latches_until_physical_completion_or_abort() {
  const auto config = make_binary_c4_config().with_response_timeout_ms(10U);
  const BatchReadWordsRequest read_request({mcprotocol::serial::DeviceCode::D, 100U}, 1U);
  const std::array<std::uint16_t, 1> write_values {0x1234U};

  {
    MelsecSerialClient client;
    assert(client.configure(config).ok());
    Rs485HookCapture hooks {};
    assert(client.set_rs485_hooks(Rs485Hooks {rs485_tx_begin, rs485_tx_end, &hooks}).ok());
    std::array<std::uint16_t, 1> words {};
    CountingCallbackCapture completion {};
    assert(client.async_batch_read_words(
        0U, read_request, words, counting_completion_callback, &completion).ok());
    assert(client.notify_tx_started(100U).ok());
    assert(client.transaction_deadline_ms() == 110U);
    assert(hooks.begin_count == 1U);

    client.poll(109U);
    assert(client.busy());
    assert(!client.requires_transport_reset());
    assert(hooks.end_count == 0U);
    assert(completion.call_count == 0U);

    client.poll(110U);
    client.poll(111U);
    assert(client.busy());
    assert(client.requires_transport_reset());
    assert(hooks.end_count == 0U);
    assert(completion.call_count == 0U);

    std::array<std::uint16_t, 1> rejected_words {};
    CountingCallbackCapture rejected_completion {};
    const Status busy_status = client.async_batch_read_words(
        111U,
        read_request,
        rejected_words,
        counting_completion_callback,
        &rejected_completion);
    assert(busy_status.code == StatusCode::Busy);
    assert(rejected_completion.call_count == 0U);

    const Status notification = client.notify_tx_complete(
        112U,
        mcprotocol::serial::make_status(
            StatusCode::Transport, "physical UART abort reported after the deadline"));
    assert(notification.ok());
    assert(!client.busy());
    assert(hooks.end_count == 1U);
    assert(completion.call_count == 1U);
    assert(completion.status.code == StatusCode::Timeout);
    assert(completion.status.cause == StatusCode::Ok);
    assert(client.requires_transport_reset());

    const Status duplicate = client.notify_tx_complete(113U, mcprotocol::serial::ok_status());
    assert(duplicate.code == StatusCode::InvalidArgument);
    client.poll(114U);
    assert(hooks.end_count == 1U);
    assert(completion.call_count == 1U);

    const Status blocked = client.async_batch_read_words(
        115U,
        read_request,
        rejected_words,
        counting_completion_callback,
        &rejected_completion);
    assert(blocked.code == StatusCode::Transport);
  }

  {
    MelsecSerialClient client;
    assert(client.configure(config).ok());
    Rs485HookCapture hooks {};
    assert(client.set_rs485_hooks(Rs485Hooks {rs485_tx_begin, rs485_tx_end, &hooks}).ok());
    CountingCallbackCapture completion {};
    assert(client.async_batch_write_words(
        0U,
        BatchWriteWordsRequest(
            DeviceAddress {mcprotocol::serial::DeviceCode::D, 100U}, write_values),
        counting_completion_callback,
        &completion).ok());
    assert(client.notify_tx_started(0U).ok());
    client.poll(10U);
    client.cancel();
    assert(client.busy());
    assert(completion.call_count == 0U);
    assert(hooks.end_count == 0U);

    assert(client.notify_tx_complete(
        11U,
        mcprotocol::serial::make_status(StatusCode::Cancelled, "UART abort completed")).ok());
    assert(!client.busy());
    assert(hooks.end_count == 1U);
    assert(completion.call_count == 1U);
    assert(completion.status.code == StatusCode::OperationOutcomeUnknown);
    assert(completion.status.cause == StatusCode::Timeout);
    assert(client.requires_transport_reset());
  }

  {
    MelsecSerialClient client;
    assert(client.configure(config).ok());
    CountingCallbackCapture completion {};
    assert(client.async_batch_write_words(
        0U,
        BatchWriteWordsRequest(
            DeviceAddress {mcprotocol::serial::DeviceCode::D, 100U}, write_values),
        counting_completion_callback,
        &completion).ok());
    assert(client.notify_tx_started(0U).ok());
    assert(client.notify_tx_complete(
        9U,
        mcprotocol::serial::make_status(StatusCode::Transport, "TX failed before deadline")).ok());
    assert(completion.call_count == 1U);
    assert(completion.status.code == StatusCode::OperationOutcomeUnknown);
    assert(completion.status.cause == StatusCode::Transport);
  }
}

[[nodiscard]] std::uint8_t ascii_hex_digit(std::uint8_t value) {
  return static_cast<std::uint8_t>(value < 10U ? ('0' + value) : ('A' + value - 10U));
}

void test_client_discards_foreign_route_then_accepts_current_route() {
  auto config = make_ascii_c4_format4_config();
  config = config.with_route(RouteConfig {C4MnMultidropRoute {
      0x01U,
      0x00U,
      C34PcTarget::connected_station(),
      C4DestinationModule::own_station(),
      SelfStationNo::number(0x01U)}});
  config = config.with_response_timeout_ms(10U);
  MelsecSerialClient client;
  Status status = client.configure(config);
  assert(status.ok());

  const BatchReadWordsRequest request({mcprotocol::serial::DeviceCode::D, 100U}, 1U);
  std::array<std::uint16_t, 1> words {};
  CallbackCapture capture {};
  status = client.async_batch_read_words(0U, request, words, completion_callback, &capture);
  assert(status.ok());
  status = start_and_notify_tx_complete(client, 1U, mcprotocol::serial::ok_status());
  assert(status.ok());

  auto foreign_config = config;
  foreign_config = foreign_config.with_route(RouteConfig {C4MnMultidropRoute {
      0x01U,
      0x00U,
      C34PcTarget::connected_station(),
      C4DestinationModule::own_station(),
      SelfStationNo::number(0x02U)}});
  const std::array<std::uint8_t, 4> response_data {'1', '2', '3', '4'};
  std::array<std::uint8_t, 128> frame {};
  std::size_t frame_size = 0U;
  status = FrameCodec::encode_success_response(
      foreign_config,
      response_data,
      frame,
      frame_size);
  assert(status.ok());
  client.on_rx_bytes(
      2U,
      mcprotocol::serial::Span<const mcprotocol::serial::Byte>(
          reinterpret_cast<const mcprotocol::serial::Byte*>(frame.data()),
          frame_size));
  assert(!capture.called);
  assert(client.busy());

  frame_size = 0U;
  status = FrameCodec::encode_success_response(config, response_data, frame, frame_size);
  assert(status.ok());
  client.on_rx_bytes(
      3U,
      mcprotocol::serial::Span<const mcprotocol::serial::Byte>(
          reinterpret_cast<const mcprotocol::serial::Byte*>(frame.data()),
          frame_size));
  assert(capture.called);
  assert(capture.status.ok());
  assert(words[0] == 0x1234U);

  MelsecSerialClient reconfigured_client;
  status = reconfigured_client.configure(config);
  assert(status.ok());
  std::array<std::uint16_t, 1> timed_out_words {};
  CallbackCapture timed_out_capture {};
  status = reconfigured_client.async_batch_read_words(
      10U,
      request,
      timed_out_words,
      completion_callback,
      &timed_out_capture);
  assert(status.ok());
  assert(start_and_notify_tx_complete(reconfigured_client, 11U, mcprotocol::serial::ok_status()).ok());
  reconfigured_client.poll(21U);
  assert(timed_out_capture.called);
  assert(timed_out_capture.status.code == StatusCode::Timeout);

  auto next_config = config;
  next_config = next_config.with_route(RouteConfig {C4StandardMultidropRoute {
      0x02U, 0x01U, C34PcTarget::connected_station(), C4DestinationModule::own_station()}});
  status = reconfigured_client.configure(next_config);
  assert(status.ok());
  std::array<std::uint16_t, 1> next_words {};
  CallbackCapture next_capture {};
  status = reconfigured_client.async_batch_read_words(
      30U,
      request,
      next_words,
      completion_callback,
      &next_capture);
  assert(status.ok());
  assert(start_and_notify_tx_complete(reconfigured_client, 31U, mcprotocol::serial::ok_status()).ok());

  frame_size = 0U;
  status = FrameCodec::encode_success_response(config, response_data, frame, frame_size);
  assert(status.ok());
  reconfigured_client.on_rx_bytes(
      32U,
      mcprotocol::serial::Span<const mcprotocol::serial::Byte>(
          reinterpret_cast<const mcprotocol::serial::Byte*>(frame.data()),
          frame_size));
  assert(!next_capture.called);
  assert(reconfigured_client.busy());

  frame_size = 0U;
  status = FrameCodec::encode_success_response(next_config, response_data, frame, frame_size);
  assert(status.ok());
  reconfigured_client.on_rx_bytes(
      33U,
      mcprotocol::serial::Span<const mcprotocol::serial::Byte>(
          reinterpret_cast<const mcprotocol::serial::Byte*>(frame.data()),
          frame_size));
  assert(next_capture.called);
  assert(next_capture.status.ok());
  assert(next_words[0] == 0x1234U);
}

void test_client_format2_auto_sequence_wrap_and_stale_response_isolation() {
  auto config = make_ascii_c4_format2_config();
  config = test_config_with_sum_check(config, SumCheckMode::Disabled);
  MelsecSerialClient client;
  Status status = client.configure(config);
  assert(status.ok());

  const BatchReadWordsRequest request({mcprotocol::serial::DeviceCode::D, 100U}, 1U);
  const std::array<std::uint8_t, 4> response_data {'1', '2', '3', '4'};

  for (std::uint32_t request_index = 0U; request_index < 258U; ++request_index) {
    const std::uint8_t expected_block = static_cast<std::uint8_t>(request_index);
    std::array<std::uint16_t, 1> words {};
    CallbackCapture capture {};
    status = client.async_batch_read_words(
        request_index * 10U,
        request,
        words,
        completion_callback,
        &capture);
    assert(status.ok());

    const auto tx = client.pending_tx_frame();
    assert(tx.size() > 3U);
    assert(mcprotocol::serial::byte_to_integer<std::uint8_t>(tx[0]) == 0x05U);
    assert(mcprotocol::serial::byte_to_integer<std::uint8_t>(tx[1]) == ascii_hex_digit(expected_block >> 4U));
    assert(mcprotocol::serial::byte_to_integer<std::uint8_t>(tx[2]) == ascii_hex_digit(expected_block & 0x0FU));

    status = start_and_notify_tx_complete(client, request_index * 10U + 1U, mcprotocol::serial::ok_status());
    assert(status.ok());

    if (request_index == 1U) {
      std::array<std::uint8_t, 128> stale_frame {};
      std::size_t stale_size = 0U;
      status = FrameCodec::encode_success_response(
          config,
          FrameCodecContext::format2(0x00U),
          response_data,
          stale_frame,
          stale_size);
      assert(status.ok());
      client.on_rx_bytes(
          request_index * 10U + 2U,
          mcprotocol::serial::Span<const mcprotocol::serial::Byte>(
              reinterpret_cast<const mcprotocol::serial::Byte*>(stale_frame.data()),
              stale_size));
      assert(!capture.called);
      assert(client.busy());
    }

    std::array<std::uint8_t, 128> response_frame {};
    std::size_t response_size = 0U;
    status = FrameCodec::encode_success_response(
        config,
        FrameCodecContext::format2(expected_block),
        response_data,
        response_frame,
        response_size);
    assert(status.ok());
    client.on_rx_bytes(
        request_index * 10U + 3U,
        mcprotocol::serial::Span<const mcprotocol::serial::Byte>(
            reinterpret_cast<const mcprotocol::serial::Byte*>(response_frame.data()),
            response_size));
    assert(capture.called);
    assert(capture.status.ok());
    assert(words[0] == 0x1234U);
  }

  std::array<std::uint16_t, 1> cancelled_words {};
  CallbackCapture cancelled_capture {};
  status = client.async_batch_read_words(
      3000U,
      request,
      cancelled_words,
      completion_callback,
      &cancelled_capture);
  assert(status.ok());
  const auto cancelled_tx = client.pending_tx_frame();
  assert(mcprotocol::serial::byte_to_integer<std::uint8_t>(cancelled_tx[1]) == '0');
  assert(mcprotocol::serial::byte_to_integer<std::uint8_t>(cancelled_tx[2]) == '2');
  assert(client.notify_tx_started(3000U).ok());
  client.cancel();
  assert(client.busy());
  assert(!cancelled_capture.called);
  status = start_and_notify_tx_complete(client, 3001U, mcprotocol::serial::ok_status());
  assert(status.ok());
  assert(cancelled_capture.called);
  assert(cancelled_capture.status.code == StatusCode::Cancelled);

  std::array<std::uint16_t, 1> next_words {};
  CallbackCapture next_capture {};
  status = client.async_batch_read_words(
      3010U,
      request,
      next_words,
      completion_callback,
      &next_capture);
  assert(status.ok());
  const auto next_tx = client.pending_tx_frame();
  assert(mcprotocol::serial::byte_to_integer<std::uint8_t>(next_tx[1]) == '0');
  assert(mcprotocol::serial::byte_to_integer<std::uint8_t>(next_tx[2]) == '3');
  assert(client.notify_tx_started(3010U).ok());
  client.cancel();
  assert(start_and_notify_tx_complete(client, 3011U, mcprotocol::serial::ok_status()).ok());

  std::array<std::uint16_t, 1> timeout_words {};
  CallbackCapture timeout_capture {};
  status = client.async_batch_read_words(
      4000U,
      request,
      timeout_words,
      completion_callback,
      &timeout_capture);
  assert(status.ok());
  status = start_and_notify_tx_complete(client, 4001U, mcprotocol::serial::ok_status());
  assert(status.ok());
  client.poll(4001U + config.timeout().response_timeout_ms);
  assert(timeout_capture.called);
  assert(timeout_capture.status.code == StatusCode::Timeout);
  assert(client.requires_transport_reset());
  status = client.configure(config);
  assert(status.ok());

  std::array<std::uint16_t, 1> after_timeout_words {};
  CallbackCapture after_timeout_capture {};
  status = client.async_batch_read_words(
      10000U,
      request,
      after_timeout_words,
      completion_callback,
      &after_timeout_capture);
  assert(status.ok());
  const auto after_timeout_tx = client.pending_tx_frame();
  assert(mcprotocol::serial::byte_to_integer<std::uint8_t>(after_timeout_tx[1]) == '0');
  assert(mcprotocol::serial::byte_to_integer<std::uint8_t>(after_timeout_tx[2]) == '5');
  status = start_and_notify_tx_complete(client, 10001U, mcprotocol::serial::ok_status());
  assert(status.ok());

  std::array<std::uint8_t, 128> late_frame {};
  std::size_t late_size = 0U;
  status = FrameCodec::encode_success_response(
      config,
      FrameCodecContext::format2(0x04U),
      response_data,
      late_frame,
      late_size);
  assert(status.ok());
  client.on_rx_bytes(
      10002U,
      mcprotocol::serial::Span<const mcprotocol::serial::Byte>(
          reinterpret_cast<const mcprotocol::serial::Byte*>(late_frame.data()),
          late_size));
  assert(!after_timeout_capture.called);

  std::array<std::uint8_t, 128> current_frame {};
  std::size_t current_size = 0U;
  status = FrameCodec::encode_success_response(
      config,
      FrameCodecContext::format2(0x05U),
      response_data,
      current_frame,
      current_size);
  assert(status.ok());
  client.on_rx_bytes(
      10003U,
      mcprotocol::serial::Span<const mcprotocol::serial::Byte>(
          reinterpret_cast<const mcprotocol::serial::Byte*>(current_frame.data()),
          current_size));
  assert(after_timeout_capture.called);
  assert(after_timeout_capture.status.ok());
  assert(after_timeout_words[0] == 0x1234U);
}

void test_client_binary_cpu_model_roundtrip() {
  const auto config = make_binary_c4_config();
  MelsecSerialClient client;
  Status status = client.configure(config);
  assert(status.ok());

  CpuModelInfo info;
  CallbackCapture capture;
  status = client.async_read_cpu_model(0, info, completion_callback, &capture);
  assert(status.ok());
  assert(client.busy());
  assert(!client.pending_tx_frame().empty());

  status = start_and_notify_tx_complete(client, 10, mcprotocol::serial::ok_status());
  assert(status.ok());

  const std::array<std::uint8_t, 18> response_data {
      'Q', '0', '2', 'H', 'C', 'P', 'U', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
      0x41, 0x00,
  };
  std::array<std::uint8_t, 128> response_frame {};
  std::size_t response_frame_size = 0;
  status = FrameCodec::encode_success_response(config, response_data, response_frame, response_frame_size);
  assert(status.ok());

  client.on_rx_bytes(
      20,
      mcprotocol::serial::Span<const mcprotocol::serial::Byte>(
          reinterpret_cast<const mcprotocol::serial::Byte*>(response_frame.data()),
          7));
  assert(!capture.called);

  client.on_rx_bytes(
      25,
      mcprotocol::serial::Span<const mcprotocol::serial::Byte>(
          reinterpret_cast<const mcprotocol::serial::Byte*>(response_frame.data() + 7),
          response_frame_size - 7));
  assert(capture.called);
  assert(capture.status.ok());
  assert(std::string_view(info.model_name.data()) == "Q02HCPU");
  assert(info.model_code == 0x0041);
  assert(!client.busy());
}

void test_client_binary_read_user_frame_roundtrip() {
  const auto config = make_binary_c4_config();
  MelsecSerialClient client;
  Status status = client.configure(config);
  assert(status.ok());

  UserFrameRegistrationData out_data {};
  CallbackCapture capture;
  status = client.async_read_user_frame_registration(
      0,
      UserFrameRegistrationReadRequest(0x03E8U),
      out_data,
      completion_callback,
      &capture);
  assert(status.ok());
  status = start_and_notify_tx_complete(client, 1, mcprotocol::serial::ok_status());
  assert(status.ok());

  const std::array<std::uint8_t, 9> response_data {
      0x05U, 0x00U, 0x04U, 0x00U, 0x03U, 0xFFU, 0xF1U, 0x0DU, 0x0AU,
  };
  std::array<std::uint8_t, 128> response_frame {};
  std::size_t response_frame_size = 0;
  status = FrameCodec::encode_success_response(config, response_data, response_frame, response_frame_size);
  assert(status.ok());

  client.on_rx_bytes(
      2,
      mcprotocol::serial::Span<const mcprotocol::serial::Byte>(
          reinterpret_cast<const mcprotocol::serial::Byte*>(response_frame.data()),
          8U));
  assert(!capture.called);
  client.on_rx_bytes(
      3,
      mcprotocol::serial::Span<const mcprotocol::serial::Byte>(
          reinterpret_cast<const mcprotocol::serial::Byte*>(response_frame.data() + 8U),
          response_frame_size - 8U));
  assert(capture.called);
  assert(capture.status.ok());
  assert(out_data.registration_data_bytes == 5U);
  assert(out_data.frame_bytes == 4U);
  const std::array<mcprotocol::serial::Byte, 5> expected {
      mcprotocol::serial::Byte {0x03}, mcprotocol::serial::Byte {0xFF}, mcprotocol::serial::Byte {0xF1}, mcprotocol::serial::Byte {0x0D}, mcprotocol::serial::Byte {0x0A},
  };
  assert(std::memcmp(out_data.registration_data.data(), expected.data(), expected.size()) == 0);
  assert(!client.busy());
}

void test_client_binary_write_user_frame_roundtrip() {
  const auto config = make_binary_c4_config();
  MelsecSerialClient client;
  Status status = client.configure(config);
  assert(status.ok());

  const std::array<mcprotocol::serial::Byte, 5> registration_data {
      mcprotocol::serial::Byte {0x03}, mcprotocol::serial::Byte {0xFF}, mcprotocol::serial::Byte {0xF1}, mcprotocol::serial::Byte {0x0D}, mcprotocol::serial::Byte {0x0A},
  };
  CallbackCapture capture;
  status = client.async_write_user_frame_registration(
      0,
      UserFrameRegistrationWriteRequest(0x03E8U, 4U, registration_data),
      completion_callback,
      &capture);
  assert(status.ok());
  status = start_and_notify_tx_complete(client, 1, mcprotocol::serial::ok_status());
  assert(status.ok());

  std::array<std::uint8_t, 64> response_frame {};
  std::size_t response_frame_size = 0;
  status = FrameCodec::encode_success_response(config, {}, response_frame, response_frame_size);
  assert(status.ok());

  client.on_rx_bytes(
      2,
      mcprotocol::serial::Span<const mcprotocol::serial::Byte>(
          reinterpret_cast<const mcprotocol::serial::Byte*>(response_frame.data()),
          response_frame_size));
  assert(capture.called);
  assert(capture.status.ok());
  assert(!client.busy());
}

void test_client_ascii_c1_loopback_roundtrip() {
  const auto config = make_ascii_c1_format4_qna_config();
  MelsecSerialClient client;
  Status status = client.configure(config);
  assert(status.ok());

  std::array<char, 8> echoed {};
  CallbackCapture capture;
  constexpr std::string_view loopback = "aBcDe";
  status = client.async_loopback(
      0,
      mcprotocol::serial::Span<const char>(loopback.data(), loopback.size()),
      echoed,
      completion_callback,
      &capture);
  assert(status.ok());
  status = start_and_notify_tx_complete(client, 1, mcprotocol::serial::ok_status());
  assert(status.ok());

  const std::array<std::uint8_t, 7> response_data {'0', '5', 'A', 'B', 'C', 'D', 'E'};
  std::array<std::uint8_t, 64> response_frame {};
  std::size_t response_frame_size = 0;
  status = FrameCodec::encode_success_response(config, response_data, response_frame, response_frame_size);
  assert(status.ok());
  client.on_rx_bytes(
      2,
      mcprotocol::serial::Span<const mcprotocol::serial::Byte>(
          reinterpret_cast<const mcprotocol::serial::Byte*>(response_frame.data()),
          6U));
  assert(!capture.called);
  client.on_rx_bytes(
      3,
      mcprotocol::serial::Span<const mcprotocol::serial::Byte>(
          reinterpret_cast<const mcprotocol::serial::Byte*>(response_frame.data() + 6U),
          response_frame_size - 6U));
  assert(capture.called);
  assert(capture.status.ok());
  assert(std::string_view(echoed.data(), 5U) == "ABCDE");
  assert(!client.busy());
}

void test_client_monitor_registration_unconfirmed_results_are_outcome_unknown() {
  const auto monitor_config = make_binary_c4_iqr_config().with_response_timeout_ms(10U);
  MelsecSerialClient monitor_client;
  Status status = monitor_client.configure(monitor_config);
  assert(status.ok());

  const RandomReadWordItem first_monitor_item {
      {mcprotocol::serial::DeviceCode::D, 100U}};
  CallbackCapture first_monitor_capture;
  status = monitor_client.async_register_monitor_devices(
      0U,
      MonitorRegistration(mcprotocol::serial::Span<const RandomReadWordItem>(&first_monitor_item, 1), {}),
      completion_callback,
      &first_monitor_capture);
  assert(status.ok());
  assert(start_and_notify_tx_complete(monitor_client, 1U, mcprotocol::serial::ok_status()).ok());
  std::array<std::uint8_t, 32> register_response {};
  std::size_t register_response_size = 0U;
  status = FrameCodec::encode_success_response(
      monitor_config, {}, register_response, register_response_size);
  assert(status.ok());
  monitor_client.on_rx_bytes(
      2U,
      mcprotocol::serial::Span<const mcprotocol::serial::Byte>(
          reinterpret_cast<const mcprotocol::serial::Byte*>(register_response.data()),
          register_response_size));
  assert(first_monitor_capture.called);
  assert(first_monitor_capture.status.ok());

  const RandomReadWordItem replacement_monitor_item {
      {mcprotocol::serial::DeviceCode::D, 200U}};
  CallbackCapture monitor_capture;
  status = monitor_client.async_register_monitor_devices(
      3U,
      MonitorRegistration(
          mcprotocol::serial::Span<const RandomReadWordItem>(&replacement_monitor_item, 1), {}),
      completion_callback,
      &monitor_capture);
  assert(status.ok());
  assert(start_and_notify_tx_complete(monitor_client, 4U, mcprotocol::serial::ok_status()).ok());
  monitor_client.poll(4U + monitor_config.timeout().response_timeout_ms);
  assert(monitor_capture.called);
  assert(monitor_capture.status.code == StatusCode::OperationOutcomeUnknown);

  status = monitor_client.configure(monitor_config);
  assert(status.ok());
  std::array<std::uint16_t, 1> monitor_values {};
  status = monitor_client.async_run_monitor_cycle(
      20U, monitor_values, {}, completion_callback, nullptr);
  assert(status.code == StatusCode::InvalidArgument);

  const auto extended_config = make_ascii_c1_format4_a_config().with_response_timeout_ms(10U);
  MelsecSerialClient extended_client;
  status = extended_client.configure(extended_config);
  assert(status.ok());

  const ExtendedFileRegisterAddress extended_item {2U, 70U};
  CallbackCapture extended_capture;
  status = extended_client.async_register_extended_file_register_monitor(
      20U,
      ExtendedFileRegisterMonitorRegistration(
          mcprotocol::serial::Span<const ExtendedFileRegisterAddress>(&extended_item, 1)),
      completion_callback,
      &extended_capture);
  assert(status.ok());
  assert(start_and_notify_tx_complete(extended_client, 21U, mcprotocol::serial::ok_status()).ok());
  extended_client.poll(21U + extended_config.timeout().response_timeout_ms);
  assert(extended_capture.called);
  assert(extended_capture.status.code == StatusCode::OperationOutcomeUnknown);
}

void test_client_remote_reset_completes_when_transmission_completes() {
  const auto config = make_binary_c4_config();
  MelsecSerialClient client;
  Status status = client.configure(config);
  assert(status.ok());

  CallbackCapture capture;
  status = client.async_remote_reset(0, completion_callback, &capture);
  assert(status.ok());
  assert(client.busy());

  status = start_and_notify_tx_complete(client, 10, mcprotocol::serial::ok_status());
  assert(status.ok());
  assert(capture.called);
  assert(capture.status.ok());
  assert(
      std::string_view(capture.status.message) ==
      "Remote RESET request transmission completed; PLC reset state is not confirmed");
  assert(!client.busy());

  std::array<std::uint8_t, 64> response_frame {};
  std::size_t response_frame_size = 0;
  status = FrameCodec::encode_success_response(config, {}, response_frame, response_frame_size);
  assert(status.ok());

  client.on_rx_bytes(
      20,
      mcprotocol::serial::Span<const mcprotocol::serial::Byte>(
          reinterpret_cast<const mcprotocol::serial::Byte*>(response_frame.data()),
          response_frame_size));
  assert(capture.called);
  assert(capture.status.ok());
  assert(
      std::string_view(capture.status.message) ==
      "Remote RESET request transmission completed; PLC reset state is not confirmed");
  assert(!client.busy());
}

void test_client_remote_run_validation_and_unknown_outcome() {
  auto config = make_binary_c4_iqr_config();
  config = config.with_response_timeout_ms(10U);
  MelsecSerialClient client;
  Status status = client.configure(config);
  assert(status.ok());

  CallbackCapture invalid_capture;
  status = client.async_remote_run(
      0U,
      static_cast<RemoteOperationMode>(0xFFFFU),
      RemoteRunClearMode::DoNotClear,
      completion_callback,
      &invalid_capture);
  assert(!status.ok());
  assert(status.code == StatusCode::InvalidArgument);
  assert(!invalid_capture.called);
  assert(!client.busy());
  assert(client.pending_tx_frame().empty());

  status = client.async_remote_run(
      0U,
      RemoteOperationMode::DoNotExecuteForcibly,
      static_cast<RemoteRunClearMode>(0xFFU),
      completion_callback,
      &invalid_capture);
  assert(!status.ok());
  assert(status.code == StatusCode::InvalidArgument);
  assert(!invalid_capture.called);
  assert(!client.busy());
  assert(client.pending_tx_frame().empty());

  CallbackCapture transport_failure_capture;
  status = client.async_remote_run(
      0U,
      RemoteOperationMode::ExecuteForcibly,
      RemoteRunClearMode::AllClear,
      completion_callback,
      &transport_failure_capture);
  assert(status.ok());
  assert(client.busy());
  assert(!client.pending_tx_frame().empty());
  status = start_and_notify_tx_complete(client,
      1U,
      mcprotocol::serial::make_status(StatusCode::Transport, "simulated TX failure"));
  assert(status.ok());
  assert(transport_failure_capture.called);
  assert(transport_failure_capture.status.code == StatusCode::OperationOutcomeUnknown);
  assert(!client.busy());
  assert(client.requires_transport_reset());
  assert(client.pending_tx_frame().empty());

  status = client.configure(config);
  assert(status.ok());
  CallbackCapture timeout_capture;
  status = client.async_remote_run(
      20U,
      RemoteOperationMode::DoNotExecuteForcibly,
      RemoteRunClearMode::ClearOutsideLatchRange,
      completion_callback,
      &timeout_capture);
  assert(status.ok());
  const std::size_t transmitted_frame_size = client.pending_tx_frame().size();
  assert(transmitted_frame_size != 0U);
  status = start_and_notify_tx_complete(client, 21U, mcprotocol::serial::ok_status());
  assert(status.ok());
  client.poll(31U);
  assert(timeout_capture.called);
  assert(timeout_capture.status.code == StatusCode::OperationOutcomeUnknown);
  assert(!client.busy());
  assert(client.requires_transport_reset());
  assert(client.pending_tx_frame().empty());

  CallbackCapture retry_capture;
  status = client.async_remote_run(
      32U,
      RemoteOperationMode::DoNotExecuteForcibly,
      RemoteRunClearMode::DoNotClear,
      completion_callback,
      &retry_capture);
  assert(!status.ok());
  assert(status.code == StatusCode::Transport);
  assert(!retry_capture.called);
  assert(!client.busy());
  assert(client.pending_tx_frame().empty());

  status = client.configure(config);
  assert(status.ok());
  CallbackCapture cancelled_after_send_capture;
  status = client.async_remote_run(
      40U,
      RemoteOperationMode::DoNotExecuteForcibly,
      RemoteRunClearMode::DoNotClear,
      completion_callback,
      &cancelled_after_send_capture);
  assert(status.ok());
  status = start_and_notify_tx_complete(client, 41U, mcprotocol::serial::ok_status());
  assert(status.ok());
  client.cancel();
  assert(cancelled_after_send_capture.called);
  assert(
      cancelled_after_send_capture.status.code == StatusCode::OperationOutcomeUnknown);
  assert(!client.busy());
  assert(client.requires_transport_reset());
}

void test_client_remote_pause_validation_and_unknown_outcome() {
  auto config = make_binary_c4_iqr_config();
  config = config.with_response_timeout_ms(10U);
  MelsecSerialClient client;
  Status status = client.configure(config);
  assert(status.ok());

  CallbackCapture invalid_capture;
  status = client.async_remote_pause(
      0U,
      static_cast<RemoteOperationMode>(0xFFFFU),
      completion_callback,
      &invalid_capture);
  assert(!status.ok());
  assert(status.code == StatusCode::InvalidArgument);
  assert(!invalid_capture.called);
  assert(!client.busy());
  assert(client.pending_tx_frame().empty());

  CallbackCapture transport_failure_capture;
  status = client.async_remote_pause(
      0U,
      RemoteOperationMode::ExecuteForcibly,
      completion_callback,
      &transport_failure_capture);
  assert(status.ok());
  assert(!client.pending_tx_frame().empty());
  status = start_and_notify_tx_complete(client,
      1U,
      mcprotocol::serial::make_status(StatusCode::Transport, "simulated TX failure"));
  assert(status.ok());
  assert(transport_failure_capture.called);
  assert(transport_failure_capture.status.code == StatusCode::OperationOutcomeUnknown);
  assert(!client.busy());
  assert(client.requires_transport_reset());

  status = client.configure(config);
  assert(status.ok());
  CallbackCapture timeout_capture;
  status = client.async_remote_pause(
      20U,
      RemoteOperationMode::DoNotExecuteForcibly,
      completion_callback,
      &timeout_capture);
  assert(status.ok());
  assert(start_and_notify_tx_complete(client, 21U, mcprotocol::serial::ok_status()).ok());
  client.poll(31U);
  assert(timeout_capture.called);
  assert(timeout_capture.status.code == StatusCode::OperationOutcomeUnknown);
  assert(!client.busy());
  assert(client.requires_transport_reset());
  assert(client.pending_tx_frame().empty());

  CallbackCapture retry_capture;
  status = client.async_remote_pause(
      32U,
      RemoteOperationMode::ExecuteForcibly,
      completion_callback,
      &retry_capture);
  assert(!status.ok());
  assert(status.code == StatusCode::Transport);
  assert(!retry_capture.called);
  assert(!client.busy());
  assert(client.pending_tx_frame().empty());

  status = client.configure(config);
  assert(status.ok());
  CallbackCapture plc_error_capture;
  status = client.async_remote_pause(
      40U,
      RemoteOperationMode::DoNotExecuteForcibly,
      completion_callback,
      &plc_error_capture);
  assert(status.ok());
  assert(start_and_notify_tx_complete(client, 41U, mcprotocol::serial::ok_status()).ok());
  std::array<std::uint8_t, 64> error_frame {};
  std::size_t error_frame_size = 0U;
  status = FrameCodec::encode_error_response(
      config, 0x7151U, error_frame, error_frame_size);
  assert(status.ok());
  client.on_rx_bytes(
      42U,
      mcprotocol::serial::Span<const mcprotocol::serial::Byte>(
          reinterpret_cast<const mcprotocol::serial::Byte*>(error_frame.data()),
          error_frame_size));
  assert(plc_error_capture.called);
  assert(plc_error_capture.status.code == StatusCode::PlcError);
  assert(plc_error_capture.status.plc_error_code == 0x7151U);
  assert(!client.busy());
  assert(client.pending_tx_frame().empty());

  CallbackCapture cancel_capture;
  status = client.async_remote_pause(
      50U,
      RemoteOperationMode::DoNotExecuteForcibly,
      completion_callback,
      &cancel_capture);
  assert(status.ok());
  assert(start_and_notify_tx_complete(client, 51U, mcprotocol::serial::ok_status()).ok());
  client.cancel();
  assert(cancel_capture.called);
  assert(cancel_capture.status.code == StatusCode::OperationOutcomeUnknown);
  assert(!client.busy());
  assert(client.requires_transport_reset());
}

void test_client_remote_control_and_password_roundtrips() {
  const auto config = make_binary_c4_iqr_config();
  std::array<std::uint8_t, 64> response_frame {};
  std::size_t response_frame_size = 0;
  Status status = FrameCodec::encode_success_response(config, {}, response_frame, response_frame_size);
  assert(status.ok());

  {
    MelsecSerialClient client;
    status = client.configure(config);
    assert(status.ok());
    CallbackCapture capture;
    status = client.async_remote_run(
        0,
        mcprotocol::serial::RemoteOperationMode::DoNotExecuteForcibly,
        mcprotocol::serial::RemoteRunClearMode::DoNotClear,
        completion_callback,
        &capture);
    assert(status.ok());
    status = start_and_notify_tx_complete(client, 1, mcprotocol::serial::ok_status());
    assert(status.ok());
    client.on_rx_bytes(
        2,
        mcprotocol::serial::Span<const mcprotocol::serial::Byte>(
            reinterpret_cast<const mcprotocol::serial::Byte*>(response_frame.data()),
            response_frame_size));
    assert(capture.called);
    assert(capture.status.ok());
  }
  {
    MelsecSerialClient client;
    status = client.configure(config);
    assert(status.ok());
    CallbackCapture capture;
    status = client.async_switch_serial_module_mode(
        0,
        SerialModuleModeSwitchRequest(
            SerialModuleChannel::Ch1,
            true,
            false,
            false,
            SerialModuleModeNo::McProtocolFormat1,
            0U,
            SerialModuleCommunicationSpeed::Bps300),
        completion_callback,
        &capture);
    assert(status.ok());
    status = start_and_notify_tx_complete(client, 1, mcprotocol::serial::ok_status());
    assert(status.ok());
    client.on_rx_bytes(
        2,
        mcprotocol::serial::Span<const mcprotocol::serial::Byte>(
            reinterpret_cast<const mcprotocol::serial::Byte*>(response_frame.data()),
            response_frame_size));
    assert(capture.called);
    assert(capture.status.ok());
  }
  {
    MelsecSerialClient client;
    status = client.configure(config);
    assert(status.ok());
    CallbackCapture capture;
    status = client.async_remote_stop(0, completion_callback, &capture);
    assert(status.ok());
    status = start_and_notify_tx_complete(client, 1, mcprotocol::serial::ok_status());
    assert(status.ok());
    client.on_rx_bytes(
        2,
        mcprotocol::serial::Span<const mcprotocol::serial::Byte>(
            reinterpret_cast<const mcprotocol::serial::Byte*>(response_frame.data()),
            response_frame_size));
    assert(capture.called);
    assert(capture.status.ok());
  }
  {
    MelsecSerialClient client;
    status = client.configure(config);
    assert(status.ok());
    CallbackCapture capture;
    status = client.async_remote_pause(
        0,
        mcprotocol::serial::RemoteOperationMode::ExecuteForcibly,
        completion_callback,
        &capture);
    assert(status.ok());
    status = start_and_notify_tx_complete(client, 1, mcprotocol::serial::ok_status());
    assert(status.ok());
    client.on_rx_bytes(
        2,
        mcprotocol::serial::Span<const mcprotocol::serial::Byte>(
            reinterpret_cast<const mcprotocol::serial::Byte*>(response_frame.data()),
            response_frame_size));
    assert(capture.called);
    assert(capture.status.ok());
  }
  {
    MelsecSerialClient client;
    status = client.configure(config);
    assert(status.ok());
    CallbackCapture capture;
    status = client.async_remote_latch_clear(0, completion_callback, &capture);
    assert(status.ok());
    status = start_and_notify_tx_complete(client, 1, mcprotocol::serial::ok_status());
    assert(status.ok());
    client.on_rx_bytes(
        2,
        mcprotocol::serial::Span<const mcprotocol::serial::Byte>(
            reinterpret_cast<const mcprotocol::serial::Byte*>(response_frame.data()),
            response_frame_size));
    assert(capture.called);
    assert(capture.status.ok());
  }
  {
    MelsecSerialClient client;
    status = client.configure(config);
    assert(status.ok());
    CallbackCapture capture;
    status = client.async_unlock_remote_password(0, "abcdef", completion_callback, &capture);
    assert(status.ok());
    status = start_and_notify_tx_complete(client, 1, mcprotocol::serial::ok_status());
    assert(status.ok());
    client.on_rx_bytes(
        2,
        mcprotocol::serial::Span<const mcprotocol::serial::Byte>(
            reinterpret_cast<const mcprotocol::serial::Byte*>(response_frame.data()),
            response_frame_size));
    assert(capture.called);
    assert(capture.status.ok());
  }
  {
    MelsecSerialClient client;
    status = client.configure(config);
    assert(status.ok());
    CallbackCapture capture;
    status = client.async_lock_remote_password(0, "abcdef", completion_callback, &capture);
    assert(status.ok());
    status = start_and_notify_tx_complete(client, 1, mcprotocol::serial::ok_status());
    assert(status.ok());
    client.on_rx_bytes(
        2,
        mcprotocol::serial::Span<const mcprotocol::serial::Byte>(
            reinterpret_cast<const mcprotocol::serial::Byte*>(response_frame.data()),
            response_frame_size));
    assert(capture.called);
    assert(capture.status.ok());
  }
}

void test_client_clear_error_information_roundtrip() {
  const auto config = make_binary_c4_iqr_config();
  MelsecSerialClient client;
  Status status = client.configure(config);
  assert(status.ok());

  CallbackCapture capture;
  status = client.async_clear_error_information(0, completion_callback, &capture);
  assert(status.ok());
  assert(client.busy());

  status = start_and_notify_tx_complete(client, 10, mcprotocol::serial::ok_status());
  assert(status.ok());

  std::array<std::uint8_t, 64> response_frame {};
  std::size_t response_frame_size = 0;
  status = FrameCodec::encode_success_response(config, {}, response_frame, response_frame_size);
  assert(status.ok());

  client.on_rx_bytes(
      20,
      mcprotocol::serial::Span<const mcprotocol::serial::Byte>(
          reinterpret_cast<const mcprotocol::serial::Byte*>(response_frame.data()),
          response_frame_size));
  assert(capture.called);
  assert(capture.status.ok());
  assert(std::string_view(capture.status.message) == "ok");
  assert(!client.busy());
}

void test_client_c24_small_command_roundtrips() {
  const auto config = make_binary_c4_config();
  std::array<std::uint8_t, 64> response_frame {};
  std::size_t response_frame_size = 0;
  Status status = FrameCodec::encode_success_response(config, {}, response_frame, response_frame_size);
  assert(status.ok());

  {
    MelsecSerialClient client;
    status = client.configure(config);
    assert(status.ok());
    CallbackCapture capture;
    status = client.async_control_global_signal(
        0,
        GlobalSignalControlRequest(GlobalSignalTarget::X1B, (false ? true : false)),
        completion_callback,
        &capture);
    assert(status.ok());
    status = start_and_notify_tx_complete(client, 1, mcprotocol::serial::ok_status());
    assert(status.ok());
    client.on_rx_bytes(
        2,
        mcprotocol::serial::Span<const mcprotocol::serial::Byte>(
            reinterpret_cast<const mcprotocol::serial::Byte*>(response_frame.data()),
            response_frame_size));
    assert(capture.called);
    assert(capture.status.ok());
  }
  {
    MelsecSerialClient client;
    status = client.configure(config);
    assert(status.ok());
    CallbackCapture capture;
    status = client.async_initialize_c24_transmission_sequence(0, completion_callback, &capture);
    assert(status.ok());
    status = start_and_notify_tx_complete(client, 1, mcprotocol::serial::ok_status());
    assert(status.ok());
    client.on_rx_bytes(
        2,
        mcprotocol::serial::Span<const mcprotocol::serial::Byte>(
            reinterpret_cast<const mcprotocol::serial::Byte*>(response_frame.data()),
            response_frame_size));
    assert(capture.called);
    assert(capture.status.ok());
  }
}

void test_client_remote_reset_does_not_wait_for_response_timeout() {
  auto config = make_binary_c4_config();
  config = config.with_response_timeout_ms(5);

  MelsecSerialClient client;
  Status status = client.configure(config);
  assert(status.ok());

  CallbackCapture capture;
  status = client.async_remote_reset(0, completion_callback, &capture);
  assert(status.ok());

  status = start_and_notify_tx_complete(client, 0, mcprotocol::serial::ok_status());
  assert(status.ok());
  assert(capture.called);
  assert(capture.status.ok());
  assert(
      std::string_view(capture.status.message) ==
      "Remote RESET request transmission completed; PLC reset state is not confirmed");
  assert(!client.busy());

  MelsecSerialClient failed_client;
  status = failed_client.configure(config);
  assert(status.ok());
  CallbackCapture failed_capture;
  status = failed_client.async_remote_reset(0U, completion_callback, &failed_capture);
  assert(status.ok());
  status = start_and_notify_tx_complete(failed_client,
      0U,
      mcprotocol::serial::make_status(StatusCode::Transport, "test transport failure"));
  assert(status.ok());
  assert(failed_capture.called);
  assert(failed_capture.status.code == StatusCode::OperationOutcomeUnknown);
}

void test_client_init_sequence_completes_when_transmission_completes() {
  auto config = make_binary_c4_config();
  config = config.with_response_timeout_ms(5);

  MelsecSerialClient client;
  Status status = client.configure(config);
  assert(status.ok());

  CallbackCapture capture;
  status = client.async_initialize_c24_transmission_sequence(0, completion_callback, &capture);
  assert(status.ok());
  assert(client.busy());

  status = start_and_notify_tx_complete(client, 1, mcprotocol::serial::ok_status());
  assert(status.ok());
  assert(capture.called);
  assert(capture.status.ok());
  assert(
      std::string_view(capture.status.message) ==
      "Transmission-sequence initialization request transmission completed; PLC state is not confirmed");
  assert(!client.requires_transport_reset());
  assert(!client.busy());

  MelsecSerialClient failed_client;
  status = failed_client.configure(config);
  assert(status.ok());
  CallbackCapture failed_capture;
  status = failed_client.async_initialize_c24_transmission_sequence(
      0, completion_callback, &failed_capture);
  assert(status.ok());
  status = start_and_notify_tx_complete(
      failed_client,
      1,
      mcprotocol::serial::make_status(StatusCode::Transport, "test transport failure"));
  assert(status.ok());
  assert(failed_capture.called);
  assert(failed_capture.status.code == StatusCode::OperationOutcomeUnknown);
}

void test_client_global_signal_completes_when_transmission_completes() {
  auto config = make_binary_c4_config();
  config = config.with_response_timeout_ms(5);

  MelsecSerialClient client;
  Status status = client.configure(config);
  assert(status.ok());

  CallbackCapture capture;
  status = client.async_control_global_signal(
      0,
      GlobalSignalControlRequest(GlobalSignalTarget::ReceivedSide, (true ? true : false)),
      completion_callback,
      &capture);
  assert(status.ok());
  assert(client.busy());

  status = start_and_notify_tx_complete(client, 1, mcprotocol::serial::ok_status());
  assert(status.ok());
  assert(capture.called);
  assert(capture.status.ok());
  assert(
      std::string_view(capture.status.message) ==
      "Global-signal request transmission completed; PLC signal state is not confirmed");
  assert(!client.requires_transport_reset());
  assert(!client.busy());

  MelsecSerialClient failed_client;
  status = failed_client.configure(config);
  assert(status.ok());
  CallbackCapture failed_capture;
  status = failed_client.async_control_global_signal(
      0,
      GlobalSignalControlRequest(GlobalSignalTarget::ReceivedSide, true),
      completion_callback,
      &failed_capture);
  assert(status.ok());
  status = start_and_notify_tx_complete(
      failed_client,
      1,
      mcprotocol::serial::make_status(StatusCode::Transport, "test transport failure"));
  assert(status.ok());
  assert(failed_capture.called);
  assert(failed_capture.status.code == StatusCode::OperationOutcomeUnknown);
}

void test_client_timeout() {
  const auto config = make_binary_c4_config();
  MelsecSerialClient client;
  Status status = client.configure(config);
  assert(status.ok());

  CpuModelInfo info;
  CallbackCapture capture;
  status = client.async_read_cpu_model(0, info, completion_callback, &capture);
  assert(status.ok());

  status = start_and_notify_tx_complete(client, 0, mcprotocol::serial::ok_status());
  assert(status.ok());

  client.poll(config.timeout().response_timeout_ms + 1);
  assert(capture.called);
  assert(capture.status.code == StatusCode::Timeout);
  assert(client.requires_transport_reset());

  CallbackCapture blocked_capture;
  status = client.async_read_cpu_model(4000U, info, completion_callback, &blocked_capture);
  assert(!status.ok());
  assert(status.code == StatusCode::Transport);
  assert(!blocked_capture.called);

  status = client.configure(config);
  assert(status.ok());
  assert(!client.requires_transport_reset());
}

void test_client_response_timeout_is_wrap_safe_and_not_extended_by_rx() {
  auto config = make_binary_c4_config();
  config = config.with_response_timeout_ms(10U);

  {
    MelsecSerialClient client;
    Status status = client.configure(config);
    assert(status.ok());
    CpuModelInfo info;
    CallbackCapture capture;
    status = client.async_read_cpu_model(0U, info, completion_callback, &capture);
    assert(status.ok());
    status = client.notify_tx_started(0xFFFFFFFAU);
    assert(status.ok());
    status = start_and_notify_tx_complete(client, 0xFFFFFFFAU, mcprotocol::serial::ok_status());
    assert(status.ok());
    client.poll(3U);
    assert(!capture.called);
    client.poll(4U);
    assert(capture.called);
    assert(capture.status.code == StatusCode::Timeout);
    assert(client.requires_transport_reset());
  }

  {
    MelsecSerialClient client;
    Status status = client.configure(config);
    assert(status.ok());
    CpuModelInfo info;
    CallbackCapture capture;
    status = client.async_read_cpu_model(100U, info, completion_callback, &capture);
    assert(status.ok());
    status = start_and_notify_tx_complete(client, 100U, mcprotocol::serial::ok_status());
    assert(status.ok());

    const std::array<mcprotocol::serial::Byte, 1> incomplete_response {mcprotocol::serial::Byte {0x10U}};
    client.on_rx_bytes(109U, incomplete_response);
    assert(!capture.called);
    client.poll(110U);
    assert(capture.called);
    assert(capture.status.code == StatusCode::Timeout);
    assert(client.requires_transport_reset());
  }
}

void test_absolute_transaction_deadline_covers_tx_and_is_not_extended_by_chunks() {
  auto config = make_binary_c4_config();
  // Keep the independent inactivity deadline longer than every test chunk gap so this test
  // isolates the fixed absolute transaction deadline.
  config = config.with_response_timeout_ms(1000U).with_inter_byte_timeout_ms(900U);
  const BatchReadWordsRequest request({mcprotocol::serial::DeviceCode::D, 100U}, 1U);
  const std::array<std::uint8_t, 2> response_data {0x34U, 0x12U};
  std::array<std::uint8_t, 64> response_frame {};
  std::size_t response_frame_size = 0U;
  Status status = FrameCodec::encode_success_response(
      config, response_data, response_frame, response_frame_size);
  assert(status.ok());
  assert(response_frame_size > 2U);

  {
    MelsecSerialClient client;
    status = client.configure(config);
    assert(status.ok());
    std::array<std::uint16_t, 1> words {};
    CallbackCapture capture;
    status = client.async_batch_read_words(
        0U, request, words, completion_callback, &capture);
    assert(status.ok());
    assert(client.notify_tx_started(0U).ok());
    assert(start_and_notify_tx_complete(client, 100U, mcprotocol::serial::ok_status()).ok());
    client.on_rx_bytes(
        100U,
        mcprotocol::serial::Span<const mcprotocol::serial::Byte>(
            reinterpret_cast<const mcprotocol::serial::Byte*>(response_frame.data()), 1U));
    client.on_rx_bytes(
        500U,
        mcprotocol::serial::Span<const mcprotocol::serial::Byte>(
            reinterpret_cast<const mcprotocol::serial::Byte*>(response_frame.data() + 1U), 1U));
    client.poll(999U);
    assert(!capture.called);
    client.on_rx_bytes(
        999U,
        mcprotocol::serial::Span<const mcprotocol::serial::Byte>(
            reinterpret_cast<const mcprotocol::serial::Byte*>(response_frame.data() + 2U),
            response_frame_size - 2U));
    assert(capture.called);
    assert(capture.status.ok());
    assert(words[0] == 0x1234U);
    assert(!client.requires_transport_reset());
  }

  {
    MelsecSerialClient client;
    status = client.configure(config);
    assert(status.ok());
    std::array<std::uint16_t, 1> words {};
    CallbackCapture capture;
    status = client.async_batch_read_words(
        0U, request, words, completion_callback, &capture);
    assert(status.ok());
    assert(client.notify_tx_started(0U).ok());
    assert(start_and_notify_tx_complete(client, 100U, mcprotocol::serial::ok_status()).ok());
    client.on_rx_bytes(
        999U,
        mcprotocol::serial::Span<const mcprotocol::serial::Byte>(
            reinterpret_cast<const mcprotocol::serial::Byte*>(response_frame.data()), 1U));
    client.poll(1000U);
    assert(capture.called);
    assert(capture.status.code == StatusCode::Timeout);
    assert(client.requires_transport_reset());
  }

  {
    // A response chunk delivered exactly at the deadline resets even sequenced Format 2.  A late
    // frame must not be retained for reuse merely because the block number is available.
    const auto format2_config = make_ascii_c4_format2_config().with_response_timeout_ms(10U);
    MelsecSerialClient client;
    status = client.configure(format2_config);
    assert(status.ok());
    std::array<std::uint16_t, 1> words {};
    CallbackCapture capture;
    status = client.async_batch_read_words(
        0U, request, words, completion_callback, &capture);
    assert(status.ok());
    assert(client.notify_tx_started(100U).ok());
    assert(start_and_notify_tx_complete(client, 101U, mcprotocol::serial::ok_status()).ok());
    const std::array<mcprotocol::serial::Byte, 1> late_byte {mcprotocol::serial::Byte {0x06U}};
    client.on_rx_bytes(110U, late_byte);
    assert(capture.called);
    assert(capture.status.code == StatusCode::Timeout);
    assert(client.requires_transport_reset());
  }

  {
    MelsecSerialClient client;
    const auto wrap_config = config.with_response_timeout_ms(10U);
    status = client.configure(wrap_config);
    assert(status.ok());
    std::array<std::uint16_t, 1> words {};
    CallbackCapture capture;
    status = client.async_batch_read_words(
        0U, request, words, completion_callback, &capture);
    assert(status.ok());
    assert(client.notify_tx_started(0xFFFFFFFAU).ok());
    assert(start_and_notify_tx_complete(client, 0xFFFFFFFBU, mcprotocol::serial::ok_status()).ok());
    client.poll(3U);
    assert(!capture.called);
    client.on_rx_bytes(
        4U,
        mcprotocol::serial::Span<const mcprotocol::serial::Byte>(
            reinterpret_cast<const mcprotocol::serial::Byte*>(response_frame.data()),
            response_frame_size));
    assert(capture.called);
    assert(capture.status.code == StatusCode::Timeout);
  }

  {
    MelsecSerialClient client;
    status = client.configure(config);
    assert(status.ok());
    std::array<std::uint16_t, 1> words {};
    CallbackCapture capture;
    status = client.async_batch_read_words(
        0U, request, words, completion_callback, &capture);
    assert(status.ok());
    assert(client.notify_tx_started(100U).ok());
    assert(start_and_notify_tx_complete(client, 1100U, mcprotocol::serial::ok_status()).ok());
    assert(capture.called);
    assert(capture.status.code == StatusCode::Timeout);
    assert(client.requires_transport_reset());
  }
}

void test_client_ascii_format4_resynchronizes_on_stale_ack() {
  const auto config = make_ascii_c4_format4_config();
  MelsecSerialClient client;
  Status status = client.configure(config);
  assert(status.ok());

  const std::array<std::uint16_t, 1> values {0x1234U};
  CallbackCapture capture;
  status = client.async_batch_write_words(
      0,
      BatchWriteWordsRequest(DeviceAddress {mcprotocol::serial::DeviceCode::D, 100}, values),
      completion_callback,
      &capture);
  assert(status.ok());

  status = start_and_notify_tx_complete(client, 1, mcprotocol::serial::ok_status());
  assert(status.ok());

  std::array<std::uint8_t, 32> ack_frame {};
  std::size_t ack_size = 0;
  status = FrameCodec::encode_success_response(config, {}, ack_frame, ack_size);
  assert(status.ok());

  std::array<std::uint8_t, 64> noisy_frame {};
  noisy_frame[0] = 0x06U;
  std::memcpy(noisy_frame.data() + 1U, ack_frame.data(), ack_size);
  client.on_rx_bytes(
      2,
      mcprotocol::serial::Span<const mcprotocol::serial::Byte>(
          reinterpret_cast<const mcprotocol::serial::Byte*>(noisy_frame.data()),
          ack_size + 1U));

  assert(capture.called);
  assert(capture.status.ok());
  assert(!client.busy());
}

void test_client_write_rejects_unexpected_success_data() {
  const auto config = make_ascii_c4_format4_config();
  MelsecSerialClient client;
  Status status = client.configure(config);
  assert(status.ok());

  const std::array<std::uint16_t, 1> values {0x1234U};
  CallbackCapture capture;
  status = client.async_batch_write_words(
      0,
      BatchWriteWordsRequest(DeviceAddress {mcprotocol::serial::DeviceCode::D, 100}, values),
      completion_callback,
      &capture);
  assert(status.ok());

  status = start_and_notify_tx_complete(client, 1, mcprotocol::serial::ok_status());
  assert(status.ok());

  const std::array<std::uint8_t, 4> unexpected_data {'0', '0', '0', '0'};
  std::array<std::uint8_t, 64> response_frame {};
  std::size_t response_size = 0;
  status = FrameCodec::encode_success_response(config, unexpected_data, response_frame, response_size);
  assert(status.ok());

  client.on_rx_bytes(
      2,
      mcprotocol::serial::Span<const mcprotocol::serial::Byte>(
          reinterpret_cast<const mcprotocol::serial::Byte*>(response_frame.data()),
          response_size));

  assert(capture.called);
  assert(capture.status.code == StatusCode::OperationOutcomeUnknown);
  assert(client.requires_transport_reset());
  assert(!client.busy());
}

void test_client_link_direct_random_read_roundtrip() {
  const auto config = make_binary_c4_iqr_config();
  MelsecSerialClient client;
  Status status = client.configure(config);
  assert(status.ok());

  const std::array<LinkDirectRandomReadWordItem, 2> items {{
      {LinkDirectDevice(0x0001U, DeviceAddress {mcprotocol::serial::DeviceCode::W, 0x0100U})},
      {LinkDirectDevice(0x0001U, DeviceAddress {mcprotocol::serial::DeviceCode::B, 0x0010U})},
  }};
  std::array<std::uint16_t, 2> values {};
  CallbackCapture capture;

  status = client.async_link_direct_random_read(
      0,
      mcprotocol::serial::Span<const LinkDirectRandomReadWordItem>(items.data(), items.size()),
      values,
      completion_callback,
      &capture);
  assert(status.ok());
  status = start_and_notify_tx_complete(client, 1, mcprotocol::serial::ok_status());
  assert(status.ok());

  const std::array<std::uint8_t, 4> response_data {0x34, 0x12, 0x01, 0x00};
  std::array<std::uint8_t, 64> response_frame {};
  std::size_t response_frame_size = 0;
  status = FrameCodec::encode_success_response(config, response_data, response_frame, response_frame_size);
  assert(status.ok());

  client.on_rx_bytes(
      2,
      mcprotocol::serial::Span<const mcprotocol::serial::Byte>(
          reinterpret_cast<const mcprotocol::serial::Byte*>(response_frame.data()),
          response_frame_size));
  assert(capture.called);
  assert(capture.status.ok());
  assert(values[0] == 0x1234U);
  assert(values[1] == 0x0001U);
  assert(!client.busy());
}

void test_client_link_direct_register_monitor_roundtrip() {
  const auto config = make_binary_c4_iqr_config();
  MelsecSerialClient client;
  Status status = client.configure(config);
  assert(status.ok());

  const std::array<LinkDirectRandomReadWordItem, 2> items {{
      {LinkDirectDevice(0x0001U, DeviceAddress {mcprotocol::serial::DeviceCode::W, 0x0100U})},
      {LinkDirectDevice(0x0001U, DeviceAddress {mcprotocol::serial::DeviceCode::B, 0x0010U})},
  }};
  CallbackCapture register_capture;
  status = client.async_link_direct_register_monitor(
      0,
      LinkDirectMonitorRegistration(mcprotocol::serial::Span<const LinkDirectRandomReadWordItem>(items.data(), items.size())),
      completion_callback,
      &register_capture);
  assert(status.ok());
  status = start_and_notify_tx_complete(client, 1, mcprotocol::serial::ok_status());
  assert(status.ok());

  std::array<std::uint8_t, 32> register_frame {};
  std::size_t register_frame_size = 0;
  status = FrameCodec::encode_success_response(config, {}, register_frame, register_frame_size);
  assert(status.ok());
  client.on_rx_bytes(
      2,
      mcprotocol::serial::Span<const mcprotocol::serial::Byte>(
          reinterpret_cast<const mcprotocol::serial::Byte*>(register_frame.data()),
          register_frame_size));
  assert(register_capture.called);
  assert(register_capture.status.ok());

  std::array<std::uint16_t, 2> values {};
  CallbackCapture read_capture;
  status = client.async_run_monitor_cycle(10, values, {}, completion_callback, &read_capture);
  assert(status.ok());
  status = start_and_notify_tx_complete(client, 11, mcprotocol::serial::ok_status());
  assert(status.ok());

  const std::array<std::uint8_t, 4> response_data {0x78, 0x56, 0x01, 0x00};
  std::array<std::uint8_t, 64> response_frame {};
  std::size_t response_frame_size = 0;
  status = FrameCodec::encode_success_response(config, response_data, response_frame, response_frame_size);
  assert(status.ok());
  client.on_rx_bytes(
      12,
      mcprotocol::serial::Span<const mcprotocol::serial::Byte>(
          reinterpret_cast<const mcprotocol::serial::Byte*>(response_frame.data()),
          response_frame_size));
  assert(read_capture.called);
  assert(read_capture.status.ok());
  assert(values[0] == 0x5678U);
  assert(values[1] == 0x0001U);
  assert(!client.busy());
}

// Validate that 0401 batch bit read rejects long timer state devices but allows
// long counter contact/coil devices. The high-level sync API still routes all
// long timer/counter states through read_long_timer_counter_state_bits.
void test_encode_batch_read_bits_long_state_device_rules() {
  const auto config = make_binary_c4_iqr_config();
  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;

  const mcprotocol::serial::DeviceCode rejected[] = {
      mcprotocol::serial::DeviceCode::LTS,
      mcprotocol::serial::DeviceCode::LTC,
      mcprotocol::serial::DeviceCode::LSTS,
      mcprotocol::serial::DeviceCode::LSTC,
  };
  for (const auto code : rejected) {
    const BatchReadBitsRequest request({code, 10}, 1);
    const Status status = CommandCodec::encode_batch_read_bits(
        config,
        request,
        request_data,
        request_size);
    assert(!status.ok());
    assert(status.code == StatusCode::InvalidArgument);
  }

  const mcprotocol::serial::DeviceCode allowed[] = {
      mcprotocol::serial::DeviceCode::LCS,
      mcprotocol::serial::DeviceCode::LCC,
  };
  for (const auto code : allowed) {
    const BatchReadBitsRequest request({code, 10}, 1);
    const Status status = CommandCodec::encode_batch_read_bits(
        config,
        request,
        request_data,
        request_size);
    assert(status.ok());
  }
}

// Validate that 0403 random read rejects long timer/counter contact/coil devices
// (LTS, LTC, LSTS, LSTC, LCS, LCC). These are not allowed per the serial manual restriction.
void test_encode_random_read_rejects_long_contact_coil_devices() {
  const auto config = make_binary_c4_config();
  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;

  const mcprotocol::serial::DeviceCode excluded[] = {
      mcprotocol::serial::DeviceCode::LTS,
      mcprotocol::serial::DeviceCode::LTC,
      mcprotocol::serial::DeviceCode::LSTS,
      mcprotocol::serial::DeviceCode::LSTC,
      mcprotocol::serial::DeviceCode::LCS,
      mcprotocol::serial::DeviceCode::LCC,
  };
  for (const auto code : excluded) {
    const RandomReadWordItem item {{code, 0}};
    const Status status = CommandCodec::encode_random_read(
        config,
        mcprotocol::serial::RandomReadRequest(
            mcprotocol::serial::Span<const mcprotocol::serial::RandomReadWordItem>(&item, 1), {}),
        request_data,
        request_size);
    assert(!status.ok());
    assert(status.code == StatusCode::InvalidArgument);
  }
}

void test_encode_random_read_rejects_standalone_qualified_only_devices() {
  const auto config = make_binary_c4_iqr_config();
  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;

  const mcprotocol::serial::DeviceCode rejected[] = {
      mcprotocol::serial::DeviceCode::G,
      mcprotocol::serial::DeviceCode::HG,
  };
  for (const auto code : rejected) {
    const RandomReadWordItem word_item {{code, 10}};
    const RandomReadDWordItem dword_item {{code, 10}};
    const Status status = CommandCodec::encode_random_read(
        config,
        mcprotocol::serial::RandomReadRequest(
            mcprotocol::serial::Span<const RandomReadWordItem>(&word_item, 1),
            mcprotocol::serial::Span<const RandomReadDWordItem>(&dword_item, 1)),
        request_data,
        request_size);
    assert(!status.ok());
    assert(status.code == StatusCode::InvalidArgument);
  }
}

// Validate that 1402 random write words supports LTN/LSTN as double-word devices on iQ-R.
void test_encode_random_write_words_allows_ltn_and_lstn() {
  const auto config = make_binary_c4_iqr_config();
  std::array<std::uint8_t, 32> request_data {};
  std::size_t request_size = 0;

  struct ExpectedCase {
    mcprotocol::serial::DeviceCode code;
    std::array<std::uint8_t, 16> expected;
  };
  const std::array<ExpectedCase, 2> cases {{
      {mcprotocol::serial::DeviceCode::LTN,
       {0x02, 0x14, 0x02, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x52, 0x00, 0x40, 0xE2, 0x01, 0x00}},
      {mcprotocol::serial::DeviceCode::LSTN,
       {0x02, 0x14, 0x02, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x5A, 0x00, 0x47, 0x94, 0x03, 0x00}},
  }};
  for (const auto& entry : cases) {
    const RandomWriteDWordItem item(
        {entry.code, 0},
        entry.code == mcprotocol::serial::DeviceCode::LTN ? 123456U : 234567U);
    const Status status = CommandCodec::encode_random_write_words(
        config,
        {},
        mcprotocol::serial::Span<const RandomWriteDWordItem>(&item, 1),
        request_data,
        request_size);
    assert(status.ok());
    assert(request_size == entry.expected.size());
    assert(std::memcmp(request_data.data(), entry.expected.data(), entry.expected.size()) == 0);
  }
}

void test_encode_random_write_words_allows_lz_on_iq_f() {
  const auto config = make_binary_c4_iqf_config();
  std::array<std::uint8_t, 32> request_data {};
  std::size_t request_size = 0;

  const RandomWriteDWordItem item(
      {mcprotocol::serial::DeviceCode::LZ, 0}, 0x12345678U);
  const Status status = CommandCodec::encode_random_write_words(
      config,
      {},
      mcprotocol::serial::Span<const RandomWriteDWordItem>(&item, 1),
      request_data,
      request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 14> expected {
      0x02, 0x14, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x62, 0x78, 0x56, 0x34, 0x12};
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_encode_random_write_words_rejects_standalone_qualified_only_devices() {
  const auto config = make_binary_c4_iqr_config();
  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;

  const mcprotocol::serial::DeviceCode rejected[] = {
      mcprotocol::serial::DeviceCode::G,
      mcprotocol::serial::DeviceCode::HG,
  };
  for (const auto code : rejected) {
    const RandomWriteWordItem word_item({code, 10}, 0x1234U);
    const RandomWriteDWordItem dword_item({code, 10}, 0x12345678U);
    const Status status = CommandCodec::encode_random_write_words(
        config,
        mcprotocol::serial::Span<const RandomWriteWordItem>(&word_item, 1),
        mcprotocol::serial::Span<const RandomWriteDWordItem>(&dword_item, 1),
        request_data,
        request_size);
    assert(!status.ok());
    assert(status.code == StatusCode::InvalidArgument);
  }
}

// Validate that 1402 random write words rejects long timer/counter contact/coil devices
// (LTS, LTC, LSTS, LSTC, LCS, LCC).
void test_encode_random_write_words_rejects_long_contact_coil_devices() {
  const auto config = make_binary_c4_config();
  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;

  const mcprotocol::serial::DeviceCode excluded[] = {
      mcprotocol::serial::DeviceCode::LTS,
      mcprotocol::serial::DeviceCode::LTC,
      mcprotocol::serial::DeviceCode::LSTS,
      mcprotocol::serial::DeviceCode::LSTC,
      mcprotocol::serial::DeviceCode::LCS,
      mcprotocol::serial::DeviceCode::LCC,
  };
  for (const auto code : excluded) {
    const RandomWriteWordItem item({code, 0}, 0);
    const Status status = CommandCodec::encode_random_write_words(
        config,
        mcprotocol::serial::Span<const RandomWriteWordItem>(&item, 1),
        {},
        request_data,
        request_size);
    assert(!status.ok());
    assert(status.code == StatusCode::InvalidArgument);
  }
}

// Validate that 1402 random write bits supports the long timer/counter contact/coil devices
// exposed by the selected profile.
void test_encode_random_write_bits_long_device_rules() {
  const auto config = make_binary_c4_iqr_config();
  std::array<std::uint8_t, 64> request_data {};
  std::size_t request_size = 0;

  const mcprotocol::serial::DeviceCode allowed[] = {
      mcprotocol::serial::DeviceCode::LTS,
      mcprotocol::serial::DeviceCode::LTC,
      mcprotocol::serial::DeviceCode::LSTS,
      mcprotocol::serial::DeviceCode::LSTC,
      mcprotocol::serial::DeviceCode::LCS,
      mcprotocol::serial::DeviceCode::LCC,
  };
  for (const auto code : allowed) {
    const RandomWriteBitItem item({code, 0}, false);
    const Status status = CommandCodec::encode_random_write_bits(
        config,
        mcprotocol::serial::Span<const RandomWriteBitItem>(&item, 1),
        request_data,
        request_size);
    assert(status.ok());
  }
}

void test_encode_random_write_bits_binary_iqr_long_counter_layout() {
  const auto config = make_binary_c4_iqr_config();
  const std::array<RandomWriteBitItem, 2> items {{
      RandomWriteBitItem(DeviceAddress {mcprotocol::serial::DeviceCode::LCS, 10}, true),
      RandomWriteBitItem(DeviceAddress {mcprotocol::serial::DeviceCode::LCC, 11}, false),
  }};

  std::array<std::uint8_t, 32> request_data {};
  std::size_t request_size = 0;
  const Status status = CommandCodec::encode_random_write_bits(
      config,
      mcprotocol::serial::Span<const RandomWriteBitItem>(items.data(), items.size()),
      request_data,
      request_size);
  assert(status.ok());

  const std::array<std::uint8_t, 21> expected {
      0x02, 0x14, 0x03, 0x00, 0x02,
      0x0A, 0x00, 0x00, 0x00, 0x55, 0x00, 0x01, 0x00,
      0x0B, 0x00, 0x00, 0x00, 0x54, 0x00, 0x00, 0x00,
  };
  assert(request_size == expected.size());
  assert(std::memcmp(request_data.data(), expected.data(), expected.size()) == 0);
}

void test_iq_l_rejects_unsupported_plain_device_families() {
  const auto config = make_binary_c4_iql_config();
  std::array<std::uint8_t, 128> request_data {};
  std::size_t request_size = 0;

  const mcprotocol::serial::DeviceCode word_devices[] = {
      mcprotocol::serial::DeviceCode::LTN,
      mcprotocol::serial::DeviceCode::LSTN,
      mcprotocol::serial::DeviceCode::LCN,
      mcprotocol::serial::DeviceCode::RD,
  };
  const std::array<std::uint16_t, 1> words {0x1234U};
  for (const auto code : word_devices) {
    const BatchReadWordsRequest read_request({code, 0}, 1);
    Status status = CommandCodec::encode_batch_read_words(config, read_request, request_data, request_size);
    assert(!status.ok());
    assert(status.code == StatusCode::InvalidArgument);

    const BatchWriteWordsRequest write_request({code, 0}, mcprotocol::serial::Span<const std::uint16_t>(words.data(), words.size()));
    status = CommandCodec::encode_batch_write_words(config, write_request, request_data, request_size);
    assert(!status.ok());
    assert(status.code == StatusCode::InvalidArgument);
  }

  const mcprotocol::serial::DeviceCode random_word_devices[] = {
      mcprotocol::serial::DeviceCode::LTN,
      mcprotocol::serial::DeviceCode::LSTN,
      mcprotocol::serial::DeviceCode::LCN,
      mcprotocol::serial::DeviceCode::LZ,
      mcprotocol::serial::DeviceCode::RD,
  };
  for (const auto code : random_word_devices) {
    const RandomReadWordItem read_word {{code, 0}};
    const RandomReadDWordItem read_dword {{code, 0}};
    Status status = CommandCodec::encode_random_read(
        config,
        mcprotocol::serial::RandomReadRequest(mcprotocol::serial::Span<const RandomReadWordItem>(&read_word, 1), mcprotocol::serial::Span<const RandomReadDWordItem>(&read_dword, 1)),
        request_data,
        request_size);
    assert(!status.ok());
    assert(status.code == StatusCode::InvalidArgument);

    const RandomWriteWordItem write_word({code, 0}, 0x1234U);
    const RandomWriteDWordItem write_dword({code, 0}, 0x12345678U);
    status = CommandCodec::encode_random_write_words(
        config,
        mcprotocol::serial::Span<const RandomWriteWordItem>(&write_word, 1),
        mcprotocol::serial::Span<const RandomWriteDWordItem>(&write_dword, 1),
        request_data,
        request_size);
    assert(!status.ok());
    assert(status.code == StatusCode::InvalidArgument);
  }

  const mcprotocol::serial::DeviceCode bit_devices[] = {
      mcprotocol::serial::DeviceCode::LTS,
      mcprotocol::serial::DeviceCode::LTC,
      mcprotocol::serial::DeviceCode::LSTS,
      mcprotocol::serial::DeviceCode::LSTC,
      mcprotocol::serial::DeviceCode::LCS,
      mcprotocol::serial::DeviceCode::LCC,
  };
  const std::array<BitValue, 1> bits {true};
  for (const auto code : bit_devices) {
    const BatchReadBitsRequest read_request({code, 0}, 1);
    Status status = CommandCodec::encode_batch_read_bits(config, read_request, request_data, request_size);
    assert(!status.ok());
    assert(status.code == StatusCode::InvalidArgument);

    const BatchWriteBitsRequest write_request({code, 0}, mcprotocol::serial::Span<const BitValue>(bits.data(), bits.size()));
    status = CommandCodec::encode_batch_write_bits(config, write_request, request_data, request_size);
    assert(!status.ok());
    assert(status.code == StatusCode::InvalidArgument);

    const RandomWriteBitItem write_bit({code, 0}, true);
    status = CommandCodec::encode_random_write_bits(
        config,
        mcprotocol::serial::Span<const RandomWriteBitItem>(&write_bit, 1),
        request_data,
        request_size);
    assert(!status.ok());
    assert(status.code == StatusCode::InvalidArgument);
  }

  const BatchReadBitsRequest s_read_request({mcprotocol::serial::DeviceCode::S, 10}, 1);
  Status status = CommandCodec::encode_batch_read_bits(config, s_read_request, request_data, request_size);
  assert(!status.ok());
  assert(status.code == StatusCode::InvalidArgument);

  const BatchWriteBitsRequest s_write_request({mcprotocol::serial::DeviceCode::S, 10}, mcprotocol::serial::Span<const BitValue>(bits.data(), bits.size()));
  status = CommandCodec::encode_batch_write_bits(config, s_write_request, request_data, request_size);
  assert(!status.ok());
  assert(status.code == StatusCode::InvalidArgument);

  const RandomWriteBitItem s_random_write({mcprotocol::serial::DeviceCode::S, 10}, true);
  status = CommandCodec::encode_random_write_bits(
      config,
      mcprotocol::serial::Span<const RandomWriteBitItem>(&s_random_write, 1),
      request_data,
      request_size);
  assert(!status.ok());
  assert(status.code == StatusCode::InvalidArgument);

  const std::array<BitValue, 16> bit_block {};
  const MultiBlockWriteBlock s_multi_block(
      DeviceAddress {mcprotocol::serial::DeviceCode::S, 0},
      1,
      mcprotocol::serial::Span<const BitValue>(bit_block.data(), bit_block.size()));
  status = CommandCodec::encode_multi_block_write(
      config,
      mcprotocol::serial::MultiBlockWriteRequest(mcprotocol::serial::Span<const MultiBlockWriteBlock>(&s_multi_block, 1)),
      request_data,
      request_size);
  assert(!status.ok());
  assert(status.code == StatusCode::InvalidArgument);
}

void test_melsec_l_rejects_s_device_access() {
  const auto config = make_binary_c4_l_config();
  std::array<std::uint8_t, 128> request_data {};
  std::size_t request_size = 0;
  const std::array<BitValue, 1> bits {true};

  const BatchReadBitsRequest s_read_request({mcprotocol::serial::DeviceCode::S, 10}, 1);
  Status status = CommandCodec::encode_batch_read_bits(config, s_read_request, request_data, request_size);
  assert(!status.ok());
  assert(status.code == StatusCode::InvalidArgument);

  const BatchWriteBitsRequest s_write_request({mcprotocol::serial::DeviceCode::S, 10}, mcprotocol::serial::Span<const BitValue>(bits.data(), bits.size()));
  status = CommandCodec::encode_batch_write_bits(config, s_write_request, request_data, request_size);
  assert(!status.ok());
  assert(status.code == StatusCode::InvalidArgument);

  const RandomWriteBitItem s_random_write(
      {mcprotocol::serial::DeviceCode::S, 10}, true);
  status = CommandCodec::encode_random_write_bits(
      config,
      mcprotocol::serial::Span<const RandomWriteBitItem>(&s_random_write, 1),
      request_data,
      request_size);
  assert(!status.ok());
  assert(status.code == StatusCode::InvalidArgument);

  const std::array<BitValue, 16> bit_block {};
  const MultiBlockWriteBlock s_multi_block(
      DeviceAddress {mcprotocol::serial::DeviceCode::S, 0},
      1,
      mcprotocol::serial::Span<const BitValue>(bit_block.data(), bit_block.size()));
  status = CommandCodec::encode_multi_block_write(
      config,
      mcprotocol::serial::MultiBlockWriteRequest(mcprotocol::serial::Span<const MultiBlockWriteBlock>(&s_multi_block, 1)),
      request_data,
      request_size);
  assert(!status.ok());
  assert(status.code == StatusCode::InvalidArgument);
}

void assert_s_device_rejected_for_profile(const ProtocolConfig& config) {
  std::array<std::uint8_t, 128> request_data {};
  std::size_t request_size = 0;
  const std::array<BitValue, 1> bits {true};
  const std::array<BitValue, 16> bit_block {};

  const BatchReadBitsRequest read_request({mcprotocol::serial::DeviceCode::S, 10}, 1);
  Status status = CommandCodec::encode_batch_read_bits(config, read_request, request_data, request_size);
  assert(!status.ok());
  assert(status.code == StatusCode::InvalidArgument);

  const BatchWriteBitsRequest batch_write_request({mcprotocol::serial::DeviceCode::S, 10}, mcprotocol::serial::Span<const BitValue>(bits.data(), bits.size()));
  status = CommandCodec::encode_batch_write_bits(config, batch_write_request, request_data, request_size);
  assert(!status.ok());
  assert(status.code == StatusCode::InvalidArgument);

  const RandomReadWordItem random_read {
      {mcprotocol::serial::DeviceCode::S, 10},
  };
  status = CommandCodec::encode_random_read(
      config,
      RandomReadRequest(mcprotocol::serial::Span<const RandomReadWordItem>(&random_read, 1), {}),
      request_data,
      request_size);
  assert(!status.ok());
  assert(status.code == StatusCode::InvalidArgument);

  const RandomWriteBitItem random_write(
      {mcprotocol::serial::DeviceCode::S, 10}, true);
  status = CommandCodec::encode_random_write_bits(
      config,
      mcprotocol::serial::Span<const RandomWriteBitItem>(&random_write, 1),
      request_data,
      request_size);
  assert(!status.ok());
  assert(status.code == StatusCode::InvalidArgument);

  const MultiBlockReadBlock read_block(
      DeviceAddress {mcprotocol::serial::DeviceCode::S, 0}, 1, true);
  status = CommandCodec::encode_multi_block_read(
      config,
      mcprotocol::serial::MultiBlockReadRequest(mcprotocol::serial::Span<const MultiBlockReadBlock>(&read_block, 1)),
      request_data,
      request_size);
  assert(!status.ok());
  assert(status.code == StatusCode::InvalidArgument);

  const MultiBlockWriteBlock block(
      DeviceAddress {mcprotocol::serial::DeviceCode::S, 0},
      1,
      mcprotocol::serial::Span<const BitValue>(bit_block.data(), bit_block.size()));
  status = CommandCodec::encode_multi_block_write(
      config,
      mcprotocol::serial::MultiBlockWriteRequest(mcprotocol::serial::Span<const MultiBlockWriteBlock>(&block, 1)),
      request_data,
      request_size);
  assert(!status.ok());
  assert(status.code == StatusCode::InvalidArgument);

  const MonitorRegistration monitor(
      mcprotocol::serial::Span<const RandomReadWordItem>(&random_read, 1), {});
  status = CommandCodec::encode_register_monitor(config, monitor, request_data, request_size);
  assert(!status.ok());
  if (config.plc_profile() != PlcProfile::MelsecIqF) {
    assert(status.code == StatusCode::InvalidArgument);
  }
}

void test_all_c4_profiles_reject_s_device_access() {
  auto q_config = make_binary_c4_config();
  q_config = q_config.with_plc_profile(PlcProfile::MelsecQ);
  assert_s_device_rejected_for_profile(q_config);

  assert_s_device_rejected_for_profile(make_binary_c4_config());
  assert_s_device_rejected_for_profile(make_binary_c4_l_config());
  assert_s_device_rejected_for_profile(make_binary_c4_iqr_config());
  assert_s_device_rejected_for_profile(make_binary_c4_iql_config());
  assert_s_device_rejected_for_profile(make_binary_c4_iqf_config());
}

void test_iq_f_rejects_unsupported_plain_device_families() {
  const auto config = make_binary_c4_iqf_config();
  std::array<std::uint8_t, 256> request_data {};
  std::size_t request_size = 0;
  const std::array<std::uint16_t, 1> words {0x1234U};
  const std::array<BitValue, 1> bits {true};

  const mcprotocol::serial::DeviceCode word_devices[] = {
      mcprotocol::serial::DeviceCode::ZR,
      mcprotocol::serial::DeviceCode::LTN,
      mcprotocol::serial::DeviceCode::LSTN,
  };
  for (const auto code : word_devices) {
    Status status = CommandCodec::encode_batch_read_words(
        config,
        BatchReadWordsRequest(DeviceAddress {code, 0}, 1),
        request_data,
        request_size);
    assert(!status.ok());
    assert(status.code == StatusCode::InvalidArgument);

    status = CommandCodec::encode_batch_write_words(
        config,
        BatchWriteWordsRequest(DeviceAddress {code, 0}, mcprotocol::serial::Span<const std::uint16_t>(words.data(), words.size())),
        request_data,
        request_size);
    assert(!status.ok());
    assert(status.code == StatusCode::InvalidArgument);

    const RandomReadWordItem read_word {{code, 0}};
    const RandomReadDWordItem read_dword {{code, 0}};
    status = CommandCodec::encode_random_read(
        config,
        RandomReadRequest(
            mcprotocol::serial::Span<const RandomReadWordItem>(&read_word, 1),
            mcprotocol::serial::Span<const RandomReadDWordItem>(&read_dword, 1)),
        request_data,
        request_size);
    assert(!status.ok());
    assert(status.code == StatusCode::InvalidArgument);

    const RandomWriteWordItem write_word({code, 0}, 0x1234U);
    const RandomWriteDWordItem write_dword({code, 0}, 0x12345678U);
    status = CommandCodec::encode_random_write_words(
        config,
        mcprotocol::serial::Span<const RandomWriteWordItem>(&write_word, 1),
        mcprotocol::serial::Span<const RandomWriteDWordItem>(&write_dword, 1),
        request_data,
        request_size);
    assert(!status.ok());
    assert(status.code == StatusCode::InvalidArgument);

    const MultiBlockReadBlock read_block(DeviceAddress {code, 0}, 1, false);
    status = CommandCodec::encode_multi_block_read(
        config,
        MultiBlockReadRequest(mcprotocol::serial::Span<const MultiBlockReadBlock>(&read_block, 1)),
        request_data,
        request_size);
    assert(!status.ok());
    assert(status.code == StatusCode::InvalidArgument);
  }

  const mcprotocol::serial::DeviceCode bit_devices[] = {
      mcprotocol::serial::DeviceCode::V,
      mcprotocol::serial::DeviceCode::DX,
      mcprotocol::serial::DeviceCode::DY,
      mcprotocol::serial::DeviceCode::LTS,
      mcprotocol::serial::DeviceCode::LTC,
      mcprotocol::serial::DeviceCode::LSTS,
      mcprotocol::serial::DeviceCode::LSTC,
  };
  for (const auto code : bit_devices) {
    Status status = CommandCodec::encode_batch_read_bits(
        config,
        BatchReadBitsRequest(DeviceAddress {code, 0}, 1),
        request_data,
        request_size);
    assert(!status.ok());
    assert(status.code == StatusCode::InvalidArgument);

    status = CommandCodec::encode_batch_write_bits(
        config,
        BatchWriteBitsRequest(DeviceAddress {code, 0}, mcprotocol::serial::Span<const BitValue>(bits.data(), bits.size())),
        request_data,
        request_size);
    assert(!status.ok());
    assert(status.code == StatusCode::InvalidArgument);

    const RandomWriteBitItem write_bit({code, 0}, true);
    status = CommandCodec::encode_random_write_bits(
        config,
        mcprotocol::serial::Span<const RandomWriteBitItem>(&write_bit, 1),
        request_data,
        request_size);
    assert(!status.ok());
    assert(status.code == StatusCode::InvalidArgument);

    const MultiBlockReadBlock read_block(DeviceAddress {code, 0}, 1, true);
    status = CommandCodec::encode_multi_block_read(
        config,
        MultiBlockReadRequest(mcprotocol::serial::Span<const MultiBlockReadBlock>(&read_block, 1)),
        request_data,
        request_size);
    assert(!status.ok());
    assert(status.code == StatusCode::InvalidArgument);
  }
}

void test_iq_f_rejects_unsupported_special_routes() {
  const auto config = make_binary_c4_iqf_config();
  std::array<std::uint8_t, 256> request_data {};
  std::size_t request_size = 0;

  const LinkDirectDevice link_word(1, DeviceAddress {mcprotocol::serial::DeviceCode::W, 0});
  Status status = CommandCodec::encode_link_direct_batch_read_words(
      config,
      link_word,
      1U,
      request_data,
      request_size);
  assert(!status.ok());
  assert(status.code == StatusCode::InvalidArgument);

  const LinkDirectDevice link_bit(1, DeviceAddress {mcprotocol::serial::DeviceCode::B, 0});
  status = CommandCodec::encode_link_direct_batch_read_bits(
      config,
      link_bit,
      1U,
      request_data,
      request_size);
  assert(!status.ok());
  assert(status.code == StatusCode::InvalidArgument);

  const QualifiedBufferWordDevice hg_device(QualifiedBufferDeviceKind::HG, 1U, 0U);
  status = CommandCodec::encode_extended_batch_read_words(
      config,
      hg_device,
      1U,
      request_data,
      request_size);
  assert(!status.ok());
  assert(status.code == StatusCode::InvalidArgument);

  status = CommandCodec::encode_read_host_buffer(
      config,
      HostBufferReadRequest(0, 1),
      request_data,
      request_size);
  assert(!status.ok());
  assert(status.code == StatusCode::UnsupportedConfiguration);

  const std::array<std::uint16_t, 1> words {0x1234U};
  status = CommandCodec::encode_write_host_buffer(
      config,
      HostBufferWriteRequest(0, mcprotocol::serial::Span<const std::uint16_t>(words.data(), words.size())),
      request_data,
      request_size);
  assert(!status.ok());
  assert(status.code == StatusCode::UnsupportedConfiguration);

  status = CommandCodec::encode_read_module_buffer(
      config,
      ModuleBufferReadRequest(0, 2, 1),
      request_data,
      request_size);
  assert(!status.ok());
  assert(status.code == StatusCode::UnsupportedConfiguration);

  const std::array<mcprotocol::serial::Byte, 2> bytes {mcprotocol::serial::Byte {0x34}, mcprotocol::serial::Byte {0x12}};
  status = CommandCodec::encode_write_module_buffer(
      config,
      ModuleBufferWriteRequest(0, 1, mcprotocol::serial::Span<const mcprotocol::serial::Byte>(bytes.data(), bytes.size())),
      request_data,
      request_size);
  assert(!status.ok());
  assert(status.code == StatusCode::UnsupportedConfiguration);

  const RandomReadWordItem monitor_item {
      {mcprotocol::serial::DeviceCode::D, 0},
  };
  status = CommandCodec::encode_register_monitor(
      config,
      MonitorRegistration(mcprotocol::serial::Span<const RandomReadWordItem>(&monitor_item, 1), {}),
      request_data,
      request_size);
  assert(!status.ok());
  assert(status.code == StatusCode::UnsupportedConfiguration);

  status = CommandCodec::encode_read_monitor(config, request_data, request_size);
  assert(!status.ok());
  assert(status.code == StatusCode::UnsupportedConfiguration);

  status = CommandCodec::encode_read_monitor(
      config,
      MonitorRegistration(mcprotocol::serial::Span<const RandomReadWordItem>(&monitor_item, 1), {}),
      request_data,
      request_size);
  assert(!status.ok());
  assert(status.code == StatusCode::UnsupportedConfiguration);
}

// Validate that 0406 multi-block read rejects long timer/counter/index devices
// and qualified-only G/HG standalone heads.
void test_encode_multi_block_read_rejects_long_devices_as_head() {
  const auto config = make_binary_c4_config();
  std::array<std::uint8_t, 256> request_data {};
  std::size_t request_size = 0;

  const mcprotocol::serial::DeviceCode excluded[] = {
      mcprotocol::serial::DeviceCode::LTS,
      mcprotocol::serial::DeviceCode::LTC,
      mcprotocol::serial::DeviceCode::LTN,
      mcprotocol::serial::DeviceCode::LSTS,
      mcprotocol::serial::DeviceCode::LSTC,
      mcprotocol::serial::DeviceCode::LSTN,
      mcprotocol::serial::DeviceCode::LCS,
      mcprotocol::serial::DeviceCode::LCC,
      mcprotocol::serial::DeviceCode::LCN,
      mcprotocol::serial::DeviceCode::LZ,
      mcprotocol::serial::DeviceCode::G,
      mcprotocol::serial::DeviceCode::HG,
  };
  for (const auto code : excluded) {
    const MultiBlockReadBlock block(DeviceAddress {code, 0}, 1, false);
    const Status status = CommandCodec::encode_multi_block_read(
        config,
        mcprotocol::serial::MultiBlockReadRequest(mcprotocol::serial::Span<const MultiBlockReadBlock>(&block, 1)),
        request_data,
        request_size);
    assert(!status.ok());
    assert(status.code == StatusCode::InvalidArgument);
  }
}

// Validate that 1406 multi-block write rejects long timer/counter/index devices
// and qualified-only G/HG standalone heads.
void test_encode_multi_block_write_rejects_long_devices_as_head() {
  const auto config = make_binary_c4_config();
  std::array<std::uint8_t, 256> request_data {};
  std::size_t request_size = 0;

  const mcprotocol::serial::DeviceCode excluded[] = {
      mcprotocol::serial::DeviceCode::LTS,
      mcprotocol::serial::DeviceCode::LTC,
      mcprotocol::serial::DeviceCode::LTN,
      mcprotocol::serial::DeviceCode::LSTS,
      mcprotocol::serial::DeviceCode::LSTC,
      mcprotocol::serial::DeviceCode::LSTN,
      mcprotocol::serial::DeviceCode::LCS,
      mcprotocol::serial::DeviceCode::LCC,
      mcprotocol::serial::DeviceCode::LCN,
      mcprotocol::serial::DeviceCode::LZ,
      mcprotocol::serial::DeviceCode::G,
      mcprotocol::serial::DeviceCode::HG,
  };
  const std::array<std::uint16_t, 1> dummy_words {0};
  for (const auto code : excluded) {
    const MultiBlockWriteBlock block(
        DeviceAddress {code, 0},
        1,
        mcprotocol::serial::Span<const std::uint16_t>(dummy_words.data(), 1));
    const Status status = CommandCodec::encode_multi_block_write(
        config,
        mcprotocol::serial::MultiBlockWriteRequest(mcprotocol::serial::Span<const MultiBlockWriteBlock>(&block, 1)),
        request_data,
        request_size);
    assert(!status.ok());
    assert(status.code == StatusCode::InvalidArgument);
  }
}

void test_protocol_config_rejects_unknown_enum_values() {
  static_assert(!mcprotocol::serial::is_valid_frame_kind(static_cast<FrameKind>(4U)));
  static_assert(!std::is_invocable_v<
                decltype(&mcprotocol::serial::highlevel::make_c4_binary_protocol),
                PlcProfile>);
  static_assert(!std::is_invocable_v<
                decltype(&mcprotocol::serial::highlevel::make_c4_binary_protocol),
                PlcProfile,
                SumCheckMode>);
  static_assert(std::is_invocable_v<
                decltype(&mcprotocol::serial::highlevel::make_c4_binary_protocol),
                PlcProfile,
                SumCheckMode,
                RouteConfig>);

  static_assert(!std::is_default_constructible_v<ProtocolConfig>);

  ProtocolConfig config = ProtocolConfig::ascii(
      static_cast<mcprotocol::serial::AsciiFrameKind>(0xFF),
      AsciiFormat::Format1,
      PlcProfile::MelsecQ,
      SumCheckMode::Enabled,
      host_station_route());
  Status status = FrameCodec::validate_config(config);
  assert(!status.ok());
  assert(status.code == StatusCode::InvalidArgument);

  config = ProtocolConfig::ascii(
      mcprotocol::serial::AsciiFrameKind::C4,
      static_cast<AsciiFormat>(0xFF),
      PlcProfile::MelsecQ,
      SumCheckMode::Enabled,
      host_station_route());
  status = FrameCodec::validate_config(config);
  assert(!status.ok());
  assert(status.code == StatusCode::InvalidArgument);

  config = ProtocolConfig::ascii(
      static_cast<mcprotocol::serial::AsciiFrameKind>(4U),
      AsciiFormat::Format1,
      PlcProfile::MelsecA,
      SumCheckMode::Enabled,
      host_station_route());
  status = FrameCodec::validate_config(config);
  assert(!status.ok());
  assert(status.code == StatusCode::InvalidArgument);

  config = make_binary_c4_config();
  config = config.with_plc_profile(static_cast<PlcProfile>(0xFE));
  status = FrameCodec::validate_config(config);
  assert(!status.ok());
  assert(status.code == StatusCode::InvalidArgument);

  config = make_binary_c4_config();
  config = config.with_route(RouteConfig {});
  status = FrameCodec::validate_config(config);
  assert(!status.ok());
  assert(status.code == StatusCode::InvalidArgument);

  config = make_binary_c4_config();
  config = ProtocolConfig::c4_binary(
      config.plc_profile(),
      static_cast<SumCheckMode>(0xFF),
      config.route());
  status = FrameCodec::validate_config(config);
  assert(!status.ok());
  assert(status.code == StatusCode::InvalidArgument);

  config = test_config_with_sum_check(config, SumCheckMode::Enabled);
  assert(FrameCodec::validate_config(config).ok());
  config = test_config_with_sum_check(config, SumCheckMode::Disabled);
  assert(FrameCodec::validate_config(config).ok());

  config = ProtocolConfig::ascii(
      static_cast<mcprotocol::serial::AsciiFrameKind>(0xFF),
      AsciiFormat::Format1,
      PlcProfile::MelsecQ,
      SumCheckMode::Disabled,
      host_station_route());
  std::array<std::uint8_t, 64> frame {};
  std::size_t frame_size = 123U;
  const std::array<std::uint8_t, 1> request_data {0x00};
  status = FrameCodec::encode_request(config, request_data, frame, frame_size);
  assert(!status.ok());
  assert(frame_size == 0U);
}

void test_wire_field_overflow_and_invalid_enum_regressions() {
  std::array<std::uint8_t, 128> request_data {};
  std::size_t request_size = 0U;
  const std::array<mcprotocol::serial::Byte, 2> bytes {mcprotocol::serial::Byte {0x12}, mcprotocol::serial::Byte {0x34}};

  const auto c1_config =
      make_ascii_c1_format4_qna_config().with_plc_profile(PlcProfile::MelsecAnAAnU);
  Status status = CommandCodec::encode_read_module_buffer(
      c1_config,
      ModuleBufferReadRequest(0x100000U, 1U, 1U),
      request_data,
      request_size);
  assert(status.code == StatusCode::InvalidArgument);
  status = CommandCodec::encode_read_module_buffer(
      c1_config,
      ModuleBufferReadRequest(0xFFFFFU, 2U, 1U),
      request_data,
      request_size);
  assert(status.code == StatusCode::InvalidArgument);
  status = CommandCodec::encode_write_module_buffer(
      c1_config,
      ModuleBufferWriteRequest(0U, 0x100U, bytes),
      request_data,
      request_size);
  assert(status.code == StatusCode::InvalidArgument);

  const auto c4_config = make_binary_c4_config();
  status = CommandCodec::encode_read_module_buffer(
      c4_config,
      ModuleBufferReadRequest(0xFFFFFFFFU, 2U, 1U),
      request_data,
      request_size);
  assert(status.code == StatusCode::InvalidArgument);

  std::uint32_t converted_address = 0U;
  status = mcprotocol::serial::qualified_buffer_word_to_byte_address(
      0x7FFFFFFFU, converted_address);
  assert(status.ok());
  assert(converted_address == 0xFFFFFFFEU);
  status = mcprotocol::serial::qualified_buffer_word_to_byte_address(
      0x80000000U, converted_address);
  assert(status.code == StatusCode::InvalidArgument);

  status = CommandCodec::module_buffer_start_address(
      0x7FFFFFFFU, 1U, converted_address);
  assert(status.ok());
  assert(converted_address == 0xFFFFFFFFU);
  status = CommandCodec::module_buffer_start_address(
      0x80000000U, 0U, converted_address);
  assert(status.code == StatusCode::InvalidArgument);

  const auto iq_l_config = make_binary_c4_iql_config();
  const QualifiedBufferWordDevice last_iq_l_g(
      QualifiedBufferDeviceKind::G, 1U, 0xFFFFFFU);
  status = CommandCodec::encode_extended_batch_read_words(
      iq_l_config, last_iq_l_g, 1U, request_data, request_size);
  assert(status.ok());
  status = CommandCodec::encode_extended_batch_read_words(
      iq_l_config, last_iq_l_g, 2U, request_data, request_size);
  assert(status.code == StatusCode::InvalidArgument);
  const QualifiedBufferWordDevice overflowing_iq_l_g(
      QualifiedBufferDeviceKind::G, 1U, 0x1000000U);
  status = CommandCodec::encode_extended_batch_read_words(
      iq_l_config, overflowing_iq_l_g, 1U, request_data, request_size);
  assert(status.code == StatusCode::InvalidArgument);

  const auto invalid_kind = static_cast<QualifiedBufferDeviceKind>(0xFF);
  assert(std::string_view(mcprotocol::serial::qualified_buffer_kind_name(invalid_kind)) == "INVALID");
  status = CommandCodec::encode_extended_batch_read_words(
      make_binary_c4_iqr_config(),
      QualifiedBufferWordDevice(invalid_kind, 1U, 0U),
      1U,
      request_data,
      request_size);
  assert(status.code == StatusCode::InvalidArgument);
}

void test_native_bool_values_are_used_by_all_block_encoders() {
  static_assert(std::is_same<BitValue, bool>::value);
  const std::array<BitValue, 2> bits {
      false, true};
  std::array<std::uint8_t, 128> request_data {};
  std::size_t request_size = 0U;

  Status status = CommandCodec::encode_batch_write_bits(
      make_binary_c4_config(),
      BatchWriteBitsRequest(
          DeviceAddress {mcprotocol::serial::DeviceCode::M, 0U}, bits),
      request_data,
      request_size);
  assert(status.ok());

  const auto iq_r_config = make_binary_c4_iqr_config();
  const LinkDirectDevice link_device(
      1U, DeviceAddress {mcprotocol::serial::DeviceCode::B, 0U});
  status = CommandCodec::encode_link_direct_batch_write_bits(
      iq_r_config, link_device, bits, request_data, request_size);
  assert(status.ok());

  const std::array<BitValue, 16> packed_block_bits {{
      false, true, false, true, false, true, false, true,
      false, true, false, true, false, true, false, true,
  }};
  const MultiBlockWriteBlock block(
      DeviceAddress {mcprotocol::serial::DeviceCode::M, 0U}, 1U, packed_block_bits);
  status = CommandCodec::encode_multi_block_write(
      iq_r_config,
      MultiBlockWriteRequest(mcprotocol::serial::Span<const MultiBlockWriteBlock>(&block, 1U)),
      request_data,
      request_size);
  assert(status.ok());

  const LinkDirectMultiBlockWriteBlock link_block(link_device, 1U, packed_block_bits);
  status = CommandCodec::encode_link_direct_multi_block_write(
      iq_r_config,
      LinkDirectMultiBlockWriteRequest(
          mcprotocol::serial::Span<const LinkDirectMultiBlockWriteBlock>(&link_block, 1U)),
      request_data,
      request_size);
  assert(status.ok());
}

void test_format4_invalid_crlf_is_a_framing_error() {
  const auto config = make_ascii_c4_format4_config();
  std::array<std::uint8_t, 64> frame {};
  std::size_t frame_size = 0U;

  Status status = FrameCodec::encode_success_response(config, {}, frame, frame_size);
  assert(status.ok());
  frame[frame_size - 1U] = static_cast<std::uint8_t>('X');
  auto decode = FrameCodec::decode_response(
      config, mcprotocol::serial::Span<const std::uint8_t>(frame.data(), frame_size));
  assert(decode.status == DecodeStatus::Error);
  assert(decode.error.code == StatusCode::Framing);

  status = FrameCodec::encode_error_response(config, 0x05U, frame, frame_size);
  assert(status.ok());
  frame[frame_size - 2U] = static_cast<std::uint8_t>('X');
  decode = FrameCodec::decode_response(
      config, mcprotocol::serial::Span<const std::uint8_t>(frame.data(), frame_size));
  assert(decode.status == DecodeStatus::Error);
  assert(decode.error.code == StatusCode::Framing);

  const std::array<std::uint8_t, 4> response_data {'1', '2', '3', '4'};
  status = FrameCodec::encode_success_response(
      config, response_data, frame, frame_size);
  assert(status.ok());
  frame[frame_size - 1U] = static_cast<std::uint8_t>('X');
  decode = FrameCodec::decode_response(
      config, mcprotocol::serial::Span<const std::uint8_t>(frame.data(), frame_size));
  assert(decode.status == DecodeStatus::Error);
  assert(decode.error.code == StatusCode::Framing);
}

void test_serial_config_requires_explicit_valid_settings() {
  static_assert(!std::is_constructible_v<
                mcprotocol::serial::Span<const std::uint8_t>,
                const std::vector<std::uint8_t>&>);
  static_assert(!std::is_constructible_v<
                mcprotocol::serial::Span<const std::uint8_t>,
                std::vector<std::uint8_t>&&>);
  static_assert(std::is_constructible_v<
                mcprotocol::serial::Span<const std::uint8_t>,
                const std::array<std::uint8_t, 1>&>);
  static_assert(!std::is_constructible_v<
                mcprotocol::serial::Span<std::uint8_t>,
                const std::array<std::uint8_t, 1>&>);
  static_assert(!std::is_default_constructible_v<HostSerialConfig>);
  static_assert(std::is_constructible_v<
                HostSerialConfig,
                std::string_view,
                std::uint32_t,
                std::uint32_t,
                std::uint32_t,
                SerialParity,
                HardwareFlowControl>);

  const ProtocolConfig binary_protocol = make_binary_c4_config();
  const ProtocolConfig ascii_protocol = ProtocolConfig::ascii(
      mcprotocol::serial::AsciiFrameKind::C4,
      AsciiFormat::Format4,
      PlcProfile::MelsecQ,
      SumCheckMode::Disabled,
      host_station_route());

  const HostSerialConfig binary_serial(
      "COM3", 19200, 8, 1, SerialParity::Even, HardwareFlowControl::None);
  assert(mcprotocol::serial::validate_serial_config(binary_serial).ok());
  assert(mcprotocol::serial::validate_mc_serial_config(binary_serial, binary_protocol).ok());

  const HostSerialConfig ascii_7_serial(
      "/dev/ttyUSB0", 9600, 7, 2, SerialParity::Odd, HardwareFlowControl::RtsCts);
  assert(mcprotocol::serial::validate_mc_serial_config(ascii_7_serial, ascii_protocol).ok());

  const HostSerialConfig ascii_8_serial(
      "/dev/ttyUSB0", 9600, 8, 2, SerialParity::None, HardwareFlowControl::None);
  assert(mcprotocol::serial::validate_mc_serial_config(ascii_8_serial, ascii_protocol).ok());

  assert(!mcprotocol::serial::validate_serial_config(
              HostSerialConfig("", 9600, 8, 1, SerialParity::None, HardwareFlowControl::None))
              .ok());
  constexpr char embedded_nul_path[] = {'C', 'O', 'M', '3', '\0', 'x'};
  assert(!mcprotocol::serial::validate_serial_config(HostSerialConfig(
              std::string_view(embedded_nul_path, sizeof(embedded_nul_path)),
              9600,
              8,
              1,
              SerialParity::None,
              HardwareFlowControl::None))
              .ok());
  assert(!mcprotocol::serial::validate_serial_config(
              HostSerialConfig("COM3", 0, 8, 1, SerialParity::None, HardwareFlowControl::None))
              .ok());

  for (const std::uint32_t data_bits : {5U, 6U, 9U, 263U}) {
    assert(!mcprotocol::serial::validate_serial_config(HostSerialConfig(
                "COM3", 9600, data_bits, 1, SerialParity::None, HardwareFlowControl::None))
                .ok());
  }
  for (const std::uint32_t stop_bits : {0U, 3U, 257U}) {
    assert(!mcprotocol::serial::validate_serial_config(HostSerialConfig(
                "COM3", 9600, 8, stop_bits, SerialParity::None, HardwareFlowControl::None))
                .ok());
  }

  assert(!mcprotocol::serial::validate_serial_config(HostSerialConfig(
              "COM3",
              9600,
              8,
              1,
              static_cast<SerialParity>(0xFF),
              HardwareFlowControl::None))
              .ok());
  assert(!mcprotocol::serial::validate_serial_config(HostSerialConfig(
              "COM3",
              9600,
              8,
              1,
              SerialParity::None,
              static_cast<HardwareFlowControl>(0xFF)))
              .ok());

  const HostSerialConfig binary_7_serial(
      "COM3", 9600, 7, 1, SerialParity::Even, HardwareFlowControl::None);
  assert(!mcprotocol::serial::validate_mc_serial_config(binary_7_serial, binary_protocol).ok());
}

void test_byte_and_span_cxx17_contract() {
  constexpr mcprotocol::serial::Byte byte {0xA5U};
  static_assert(mcprotocol::serial::byte_to_integer<std::uint8_t>(byte) == 0xA5U);
  static_assert(!std::is_convertible_v<mcprotocol::serial::Byte, std::uint8_t>);

  std::array<std::uint8_t, 3> values {1U, 2U, 3U};
  const std::array<std::uint8_t, 2> const_values {4U, 5U};
  mcprotocol::serial::Span<std::uint8_t> mutable_span(values);
  mcprotocol::serial::Span<const std::uint8_t> const_span(mutable_span);
  mcprotocol::serial::Span<const std::uint8_t> const_array_span(const_values);
  assert(const_span.size() == 3U);
  assert(const_array_span.size() == 2U);
  assert(mutable_span.try_at(0U) == values.data());
  assert(mutable_span.try_at(3U) == nullptr);

  mcprotocol::serial::Span<std::uint8_t> slice(values.data(), values.size());
  assert(mutable_span.try_first(2U, slice));
  assert(slice.size() == 2U);
  assert(slice[0] == 1U && slice[1] == 2U);
  assert(!mutable_span.try_first(4U, slice));
  assert(slice.empty());
  assert(slice.data() == nullptr);

  assert(mutable_span.try_subspan(1U, 2U, slice));
  assert(slice.size() == 2U);
  assert(slice[0] == 2U && slice[1] == 3U);
  assert(mutable_span.try_subspan(3U, slice));
  assert(slice.empty());
  assert(!mutable_span.try_subspan(4U, slice));
  assert(slice.empty());
  assert(!mutable_span.try_subspan(2U, 2U, slice));
  assert(slice.empty());
}

#if defined(_WIN32)
void test_win32_serial_dcb_is_fully_owned() {
  DCB dcb;
  std::memset(&dcb, 0xFF, sizeof(dcb));
  dcb.fDummy2 = 0x15555U;
  dcb.wReserved = 0xA55AU;
  dcb.wReserved1 = 0x5AA5U;
  const HostSerialConfig serial(
      "COM3", 19200, 7, 2, SerialParity::Even, HardwareFlowControl::RtsCts);
  const Status status = mcprotocol::serial::detail::build_win32_dcb(dcb, serial);
  assert(status.ok());
  assert(dcb.DCBlength == sizeof(dcb));
  assert(dcb.BaudRate == 19200U);
  assert(dcb.ByteSize == 7U);
  assert(dcb.StopBits == TWOSTOPBITS);
  assert(dcb.Parity == EVENPARITY);
  assert(dcb.fBinary == TRUE);
  assert(dcb.fParity == TRUE);
  assert(dcb.fOutxCtsFlow == TRUE);
  assert(dcb.fOutxDsrFlow == FALSE);
  assert(dcb.fDtrControl == DTR_CONTROL_DISABLE);
  assert(dcb.fDsrSensitivity == FALSE);
  assert(dcb.fTXContinueOnXoff == FALSE);
  assert(dcb.fOutX == FALSE);
  assert(dcb.fInX == FALSE);
  assert(dcb.fErrorChar == FALSE);
  assert(dcb.fNull == FALSE);
  assert(dcb.fRtsControl == RTS_CONTROL_HANDSHAKE);
  assert(dcb.fAbortOnError == FALSE);
  assert(dcb.fDummy2 == 0x15555U);
  assert(dcb.wReserved == 0xA55AU);
  assert(dcb.XonLim == 0U);
  assert(dcb.XoffLim == 0U);
  assert(static_cast<unsigned char>(dcb.XonChar) == 0x11U);
  assert(static_cast<unsigned char>(dcb.XoffChar) == 0x13U);
  assert(dcb.ErrorChar == 0);
  assert(dcb.EofChar == 0);
  assert(dcb.EvtChar == 0);
  assert(dcb.wReserved1 == 0x5AA5U);

  const HostSerialConfig no_flow(
      "COM3", 9600, 8, 1, SerialParity::None, HardwareFlowControl::None);
  std::memset(&dcb, 0xFF, sizeof(dcb));
  dcb.fDummy2 = 0x12222U;
  dcb.wReserved = 0x1357U;
  dcb.wReserved1 = 0x2468U;
  assert(mcprotocol::serial::detail::build_win32_dcb(dcb, no_flow).ok());
  assert(dcb.fBinary == TRUE);
  assert(dcb.fParity == FALSE);
  assert(dcb.Parity == NOPARITY);
  assert(dcb.fOutxCtsFlow == FALSE);
  assert(dcb.fOutxDsrFlow == FALSE);
  assert(dcb.fDtrControl == DTR_CONTROL_DISABLE);
  assert(dcb.fDsrSensitivity == FALSE);
  assert(dcb.fTXContinueOnXoff == FALSE);
  assert(dcb.fOutX == FALSE);
  assert(dcb.fInX == FALSE);
  assert(dcb.fErrorChar == FALSE);
  assert(dcb.fNull == FALSE);
  assert(dcb.fRtsControl == RTS_CONTROL_DISABLE);
  assert(dcb.fAbortOnError == FALSE);
  assert(dcb.fDummy2 == 0x12222U);
  assert(dcb.wReserved == 0x1357U);
  assert(dcb.wReserved1 == 0x2468U);
}

void test_win32_serial_io_size_boundaries() {
  Status status = mcprotocol::serial::detail::validate_win32_io_size(
      static_cast<std::uint64_t>(MAXDWORD),
      "size rejected");
  assert(status.ok());

  status = mcprotocol::serial::detail::validate_win32_io_size(
      static_cast<std::uint64_t>(MAXDWORD) + 1U,
      "size rejected");
  assert(status.code == StatusCode::InvalidArgument);
}

void test_win32_deadline_timeouts_return_buffered_bytes_without_losing_deadline() {
  const COMMTIMEOUTS timeouts =
      mcprotocol::serial::detail::build_win32_deadline_timeouts(750U);
  assert(timeouts.ReadIntervalTimeout == MAXDWORD);
  assert(timeouts.ReadTotalTimeoutMultiplier == MAXDWORD);
  assert(timeouts.ReadTotalTimeoutConstant == 750U);
  assert(timeouts.WriteTotalTimeoutMultiplier == 0U);
  assert(timeouts.WriteTotalTimeoutConstant == 750U);
}
#endif

#if defined(__unix__) || defined(__APPLE__)
void test_posix_serial_termios_is_fully_owned() {
  termios configured {};
  std::memset(&configured, 0xFF, sizeof(configured));
  const HostSerialConfig none_settings(
      "/dev/null", 19200, 8, 1, SerialParity::None, HardwareFlowControl::None);
  assert(mcprotocol::serial::detail::build_posix_termios(
             configured, none_settings, B19200)
             .ok());
  assert(configured.c_iflag == 0U);
  assert(configured.c_oflag == 0U);
  assert(configured.c_lflag == 0U);
  assert((configured.c_cflag & CSIZE) == CS8);
  assert((configured.c_cflag & CLOCAL) != 0U);
  assert((configured.c_cflag & CREAD) != 0U);
  assert((configured.c_cflag & (CSTOPB | PARENB | PARODD)) == 0U);
#if defined(CRTSCTS)
  assert((configured.c_cflag & CRTSCTS) == 0U);
#elif defined(CCTS_OFLOW) && defined(CRTS_IFLOW)
  assert((configured.c_cflag & (CCTS_OFLOW | CRTS_IFLOW)) == 0U);
#endif
  for (const auto control_character : configured.c_cc) {
    assert(control_character == 0U);
  }
  assert(::cfgetispeed(&configured) == B19200);
  assert(::cfgetospeed(&configured) == B19200);

  const HostSerialConfig rts_cts_settings(
      "/dev/null", 9600, 7, 2, SerialParity::Odd, HardwareFlowControl::RtsCts);
  std::memset(&configured, 0xFF, sizeof(configured));
  assert(mcprotocol::serial::detail::build_posix_termios(
             configured, rts_cts_settings, B9600)
             .ok());
  assert(configured.c_iflag == 0U);
  assert(configured.c_oflag == 0U);
  assert(configured.c_lflag == 0U);
  assert((configured.c_cflag & CSIZE) == CS7);
  assert((configured.c_cflag & CSTOPB) != 0U);
  assert((configured.c_cflag & PARENB) != 0U);
  assert((configured.c_cflag & PARODD) != 0U);
#if defined(CRTSCTS)
  assert((configured.c_cflag & CRTSCTS) != 0U);
#elif defined(CCTS_OFLOW) && defined(CRTS_IFLOW)
  assert((configured.c_cflag & (CCTS_OFLOW | CRTS_IFLOW)) ==
         (CCTS_OFLOW | CRTS_IFLOW));
#endif

  const int master_fd = ::posix_openpt(O_RDWR | O_NOCTTY);
  assert(master_fd >= 0);
  assert(::grantpt(master_fd) == 0);
  assert(::unlockpt(master_fd) == 0);
  const char* slave_path = ::ptsname(master_fd);
  assert(slave_path != nullptr);

  mcprotocol::serial::HostSerialPort port;
  const HostSerialConfig serial(
      slave_path, 19200, 8, 1, SerialParity::None, HardwareFlowControl::None);
  const Status status = port.open(serial);
  assert(status.ok());

  termios tty {};
  assert(::tcgetattr(static_cast<int>(port.native_handle()), &tty) == 0);
  assert(tty.c_iflag == 0U);
  assert(tty.c_oflag == 0U);
  assert(tty.c_lflag == 0U);
  assert((tty.c_cflag & CSIZE) == CS8);
  assert((tty.c_cflag & CLOCAL) != 0U);
  assert((tty.c_cflag & CREAD) != 0U);
  assert((tty.c_cflag & CSTOPB) == 0U);
  assert((tty.c_cflag & PARENB) == 0U);
  assert((tty.c_cflag & PARODD) == 0U);
#if defined(CRTSCTS)
  assert((tty.c_cflag & CRTSCTS) == 0U);
#elif defined(CCTS_OFLOW) && defined(CRTS_IFLOW)
  assert((tty.c_cflag & (CCTS_OFLOW | CRTS_IFLOW)) == 0U);
#endif
  assert(tty.c_cc[VMIN] == 0U);
  assert(tty.c_cc[VTIME] == 0U);
  assert(::cfgetispeed(&tty) == B19200);
  assert(::cfgetospeed(&tty) == B19200);

  port.close();
  assert(::close(master_fd) == 0);
}
#endif

}  // namespace

int main() {
  test_wire_field_overflow_and_invalid_enum_regressions();
  test_native_bool_values_are_used_by_all_block_encoders();
  test_format4_invalid_crlf_is_a_framing_error();
  test_serial_config_requires_explicit_valid_settings();
  test_byte_and_span_cxx17_contract();
#if defined(_WIN32)
  test_win32_serial_dcb_is_fully_owned();
  test_win32_serial_io_size_boundaries();
  test_win32_deadline_timeouts_return_buffered_bytes_without_losing_deadline();
#endif
#if defined(__unix__) || defined(__APPLE__)
  test_posix_serial_termios_is_fully_owned();
#endif
  test_module_io_constants();
  test_format5_batch_read_request_matches_manual();
  test_iq_l_uses_q_l_binary_request_shape();
  test_decode_binary_cpu_model_response();
  test_encode_remote_run_binary_request();
  test_encode_remote_stop_binary_request();
  test_encode_remote_pause_binary_request();
  test_encode_remote_latch_clear_binary_request();
  test_encode_remote_reset_binary_request();
  test_encode_unlock_remote_password_binary_q_l_request();
  test_encode_lock_remote_password_binary_q_l_request();
  test_encode_unlock_remote_password_rejects_invalid_lengths();
  test_encode_clear_error_information_binary_q_l_request();
  test_encode_clear_error_information_binary_iqr_request();
  test_encode_initialize_transmission_sequence_binary_request();
  test_encode_initialize_transmission_sequence_rejects_ascii();
  test_encode_control_global_signal_binary_request();
  test_encode_control_global_signal_uses_route_station_not_specification_word();
  test_control_requests_reject_unknown_or_empty_changes_without_tx();
  test_empty_request_containers_are_rejected_without_tx();
  test_encode_switch_serial_module_mode_binary_request_matches_manual();
  test_encode_switch_serial_module_mode_ascii_request_matches_manual();
  test_encode_switch_serial_module_mode_rejects_invalid_request();
  test_decode_ascii_loopback_response();
  test_encode_ascii_read_user_frame_request_shape();
  test_parse_ascii_read_user_frame_response();
  test_parse_binary_read_user_frame_response_accepts_zero_frame_bytes();
  test_encode_binary_write_user_frame_request_shape();
  test_encode_binary_delete_user_frame_request_shape();
  test_validate_ascii_c2_config_and_reject_binary();
  test_validate_ascii_c1_config_and_reject_binary();
  test_validate_c4_routed_access_and_connected_station_only_commands();
  test_route_types_require_frame_specific_station_and_network();
  test_self_station_topology_is_typed_required_and_strict();
  test_response_route_identity_is_strict();
  test_pc_targets_are_required_typed_and_frame_specific();
  test_c4_destination_module_is_required_typed_and_validated();
  test_encode_ascii_format2_request_inserts_block_number();
  test_decode_ascii_format2_partial_header_returns_incomplete();
  test_decode_ascii_format1_partial_header_returns_incomplete();
  test_decode_response_prefix_sweep_reports_incomplete();
  test_sum_check_modes_are_strict_and_corruption_is_rejected();
  test_encode_ascii_c2_format2_request_uses_fb_frame_id_and_short_command();
  test_decode_ascii_format2_ack_response();
  test_decode_ascii_c2_format2_four_digit_error_response();
  test_encode_ascii_c2_format2_error_preserves_four_digit_code();
  test_format2_raw_context_is_explicit_and_strict();
  test_encode_ascii_c2_format3_request_uses_fb_frame_id_and_short_command();
  test_decode_ascii_c2_format3_data_response();
  test_decode_ascii_c2_format3_four_digit_error_response();
  test_encode_ascii_c2_format3_error_preserves_four_digit_code();
  test_encode_ascii_c1_batch_read_words_qna_request_shape();
  test_encode_ascii_c1_batch_read_bits_a_request_shape();
  test_encode_ascii_c1_batch_write_words_qna_request_shape();
  test_decode_ascii_c1_ack_response();
  test_encode_ascii_c1_format3_uses_gg_end_code();
  test_decode_ascii_c1_error_uses_two_digit_code();
  test_encode_ascii_c1_rejects_unsupported_series();
  test_encode_ascii_c1_random_write_bits_qna_request_shape();
  test_encode_ascii_c1_random_write_words_qna_request_shape();
  test_encode_ascii_c1_register_monitor_bits_and_read_request_shape();
  test_encode_ascii_c1_register_monitor_words_and_read_request_shape();
  test_encode_ascii_c1_read_module_buffer_request_shape();
  test_encode_ascii_c1_write_module_buffer_request_shape();
  test_encode_ascii_c1_loopback_request_shape();
  test_decode_ascii_c1_loopback_response();
  test_encode_ascii_c1_loopback_uses_internal_ff_pc_no();
  test_encode_ascii_c1_extended_file_register_read_a_request_shape();
  test_encode_ascii_c1_direct_extended_file_register_read_ana_request_shape();
  test_encode_ascii_c1_extended_file_register_write_a_request_shape();
  test_encode_ascii_c1_direct_extended_file_register_write_ana_request_shape();
  test_encode_ascii_c1_extended_file_register_random_write_a_request_shape();
  test_encode_ascii_c1_extended_file_register_monitor_a_request_shape();
  test_response_timeout_contract();
  test_inter_byte_timeout_contract_and_candidate_progress();
  test_invalid_reconfigure_preserves_previous_validated_config();
  test_tx_drain_wait_policy_yields_before_each_bounded_sleep();
  test_tx_drain_loop_boundaries_failures_and_simulated_delay();
  test_encode_ascii_format4_request_appends_crlf();
  test_decode_ascii_c2_format4_ack_response();
  test_decode_ascii_c2_format4_four_digit_error_response();
  test_decode_ascii_format4_ack_response();
  test_high_level_parse_device_address();
  test_parse_link_direct_device_surface();
  test_parse_qualified_buffer_word_device_rejects_overflow();
  test_high_level_make_contiguous_requests();
  test_high_level_protocol_presets();
  test_plc_profile_names_and_internal_grouping();
  test_protocol_config_rejects_unknown_enum_values();
  test_plc_profile_is_required_for_encoding();
  test_high_level_make_random_bit_item();
  test_high_level_make_random_dword_item_defaults();
  test_high_level_make_random_request_from_specs();
  test_high_level_make_random_write_items_from_specs();
  test_high_level_make_monitor_registration_from_specs();
  test_high_level_long_state_read_spec_and_decode();
  test_long_state_read_aggregate_order_boundary_and_no_partial_output();
  test_encode_sm_sd_and_lz_device_codes();
  test_encode_batch_write_words_ascii_order();
  test_encode_batch_word_access_rejects_standalone_qualified_only_devices();
  test_all_profiles_reject_standalone_g_hg_plain_access();
  test_encode_extended_batch_read_words_ascii_matches_manual_shape();
  test_encode_extended_batch_read_words_binary_matches_capture_shape();
  test_encode_extended_batch_read_words_iq_l_g_uses_q_l_wire_shape();
  test_encode_extended_batch_read_words_binary_hg_matches_capture_shape();
  test_encode_extended_batch_read_words_rejects_iq_l_hg();
  test_encode_extended_batch_read_words_binary_module_access_ql_shape();
  test_qualified_hg_rejects_non_cpu_modules_before_send();
  test_encode_link_direct_batch_read_words_binary_iqr_matches_manual_shape();
  test_encode_link_direct_batch_read_bits_binary_iqr_matches_manual_shape();
  test_encode_batch_read_bits_binary_single_uses_addressed_point();
  test_encode_link_direct_batch_read_bits_binary_single_uses_addressed_point();
  test_encode_link_direct_batch_read_words_ascii_iqr_shape();
  test_encode_link_direct_batch_read_words_ascii_q_l_matches_manual_shape();
  test_encode_link_direct_batch_read_bits_ascii_iqr_shape();
  test_encode_link_direct_batch_write_words_binary_iqr_shape();
  test_encode_link_direct_batch_write_bits_binary_iqr_shape();
  test_encode_link_direct_batch_write_words_ascii_iqr_shape();
  test_encode_link_direct_batch_write_bits_ascii_iqr_shape();
  test_encode_link_direct_random_read_binary_iqr_shape();
  test_encode_link_direct_random_write_words_binary_iqr_shape();
  test_encode_link_direct_random_write_words_rejects_wrapped_sizes();
  test_ql_normal_device_number_rejects_wire_overflow_without_truncation();
  test_encode_link_direct_random_write_bits_binary_iqr_shape();
  test_encode_link_direct_multi_block_read_binary_iqr_shape();
  test_encode_link_direct_multi_block_write_binary_iqr_shape();
  test_encode_link_direct_multi_block_write_binary_bit_order();
  test_encode_link_direct_register_monitor_binary_iqr_shape();
  test_encode_batch_write_bits_binary_single_even_uses_addressed_point_and_high_nibble();
  test_encode_batch_write_bits_binary_single_odd_uses_addressed_point_and_high_nibble();
  test_encode_link_direct_batch_write_bits_binary_single_even_uses_addressed_point_and_high_nibble();
  test_parse_batch_read_bits_binary_single_uses_high_nibble();
  test_encode_batch_write_bits_binary_two_points_use_high_then_low_nibbles();
  test_parse_batch_read_bits_binary_two_points_use_high_then_low_nibbles();
  test_encode_batch_write_words_ascii_limit_matches_buffer();
  test_encode_batch_write_bits_ascii_limit_matches_buffer();
  test_single_request_capacity_uses_complete_worst_case_wire_size();
  test_encode_batch_read_bits_binary_c24_limit_is_7904_points();
  test_c1_word_unit_bit_device_limits_and_alignment();
  test_encode_random_write_words_ascii_matches_manual();
  test_required_input_types_are_not_default_constructible();
  test_explicit_random_width_contract_and_boundaries();
  test_encode_random_read_binary_iqr_layout();
  test_encode_random_read_binary_ql_layout();
  test_encode_random_write_words_binary_iqr_layout();
  test_encode_random_write_words_binary_ql_layout();
  test_encode_random_write_words_allows_lz_on_iq_f();
  test_encode_random_write_bits_ascii_matches_manual();
  test_encode_random_write_bits_ascii_iqr_shape();
  test_encode_random_write_bits_binary_iqr_layout();
  test_encode_random_write_bits_binary_ql_keeps_device_numbers();
  test_encode_batch_read_bits_long_state_device_rules();
  test_qna_family_random_access_uses_smaller_limits();
  test_encode_multi_block_read_ascii_matches_manual();
  test_encode_multi_block_read_binary_matches_capture_counts();
  test_encode_multi_block_read_rejects_total_points_over_960();
  test_encode_multi_block_write_binary_uses_single_byte_block_counts();
  test_encode_multi_block_write_iqr_uses_coefficient_nine();
  test_encode_multi_block_write_binary_bit_blocks_use_lsb_first_word_packing();
  test_encode_multi_block_write_ascii_bit_blocks_use_lsb_first_word_packing();
  test_encode_register_monitor_ascii_reuses_random_read_layout();
  test_encode_register_monitor_ascii_c2_reuses_compact_command_header();
  test_ascii_c2_exact_command_allowlist_vectors();
  test_ascii_c2_allowlist_rejects_unsupported_before_output_or_client_tx();
  test_command_codec_rejects_removed_e1_ordinal_without_output();
  test_ascii_c2_compact_header_capacity_uses_one_byte();
  test_ascii_c2_loopback_uses_exact_full_header_capacity();
  test_link_direct_extended_random_and_monitor_reject_ascii_c2();
  test_encode_success_response_large_sum_check_has_no_fixed_scratch_limit();
  test_encode_register_monitor_binary_iqr_layout();
  test_encode_register_monitor_binary_iqr_allows_lz_shape();
  test_encode_read_monitor_ascii_matches_manual();
  test_sparse_native_bit_helpers_match_batch_random_and_monitor_values();
  test_parse_multi_block_read_response_ascii_mixed_blocks();
  test_parse_qualified_buffer_word_device_accepts_g_and_hg();
  test_make_qualified_buffer_read_words_request_maps_to_module_buffer();
  test_validate_qualified_buffer_helper_route_rejects_q_l_equivalent_profiles();
  test_make_qualified_buffer_write_words_request_encodes_little_endian_bytes();
  test_decode_qualified_buffer_word_values_decodes_little_endian_bytes();
  test_c1_physical_profile_rejections_do_not_start_client_requests();
  test_c1_direct_extended_file_register_sync_profile_paths();
  test_native_qualified_sync_hg_validation_precedes_transport();
  test_bit_in_word_operation_contract();
  test_bit_in_word_operation_covers_every_complete_word_route();
  test_bit_in_word_sync_surface_covers_every_complete_word_route();
  test_host_single_request_surface_and_deprecated_delegates();
  test_client_receive_failure_preserves_transport_status();
  test_client_busy_rejection_preserves_active_request_state();
  test_client_instances_have_independent_in_flight_state();
  test_client_all_state_changes_report_ambiguous_outcomes();
  test_not_connected_is_distinct_from_transport_failure();
  test_client_unsequenced_decode_failures_require_transport_reset();
  test_client_rs485_hooks_and_tx_completion_lifecycle();
  test_tx_deadline_latches_until_physical_completion_or_abort();
  test_client_binary_cpu_model_roundtrip();
  test_client_discards_foreign_route_then_accepts_current_route();
  test_client_format2_auto_sequence_wrap_and_stale_response_isolation();
  test_client_binary_read_user_frame_roundtrip();
  test_client_binary_write_user_frame_roundtrip();
  test_client_ascii_c1_loopback_roundtrip();
  test_client_monitor_registration_unconfirmed_results_are_outcome_unknown();
  test_client_remote_control_and_password_roundtrips();
  test_client_remote_run_validation_and_unknown_outcome();
  test_client_remote_pause_validation_and_unknown_outcome();
  test_client_clear_error_information_roundtrip();
  test_client_c24_small_command_roundtrips();
  test_client_remote_reset_completes_when_transmission_completes();
  test_client_remote_reset_does_not_wait_for_response_timeout();
  test_client_init_sequence_completes_when_transmission_completes();
  test_client_global_signal_completes_when_transmission_completes();
  test_client_link_direct_random_read_roundtrip();
  test_client_link_direct_register_monitor_roundtrip();
  test_client_ascii_c1_register_monitor_roundtrip();
  test_client_ascii_c1_extended_file_register_monitor_roundtrip();
  test_client_timeout();
  test_client_response_timeout_is_wrap_safe_and_not_extended_by_rx();
  test_absolute_transaction_deadline_covers_tx_and_is_not_extended_by_chunks();
  test_client_ascii_format4_resynchronizes_on_stale_ack();
  test_client_write_rejects_unexpected_success_data();
  test_encode_random_read_rejects_long_contact_coil_devices();
  test_encode_random_read_rejects_standalone_qualified_only_devices();
  test_encode_random_write_words_allows_ltn_and_lstn();
  test_encode_random_write_words_rejects_long_contact_coil_devices();
  test_encode_random_write_words_rejects_standalone_qualified_only_devices();
  test_encode_random_write_bits_long_device_rules();
  test_encode_random_write_bits_binary_iqr_long_counter_layout();
  test_iq_l_rejects_unsupported_plain_device_families();
  test_melsec_l_rejects_s_device_access();
  test_all_c4_profiles_reject_s_device_access();
  test_iq_f_rejects_unsupported_plain_device_families();
  test_iq_f_rejects_unsupported_special_routes();
  test_encode_multi_block_read_rejects_long_devices_as_head();
  test_encode_multi_block_write_rejects_long_devices_as_head();

  return 0;
}
