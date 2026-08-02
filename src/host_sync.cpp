#include "mcprotocol/serial/host_sync.hpp"

#include "host_now_ms.hpp"
#include "host_sync_runner.hpp"
#include "mcprotocol/serial/detail/fixed_item_array.hpp"
#include "mcprotocol/serial/detail/long_state_aggregate.hpp"
#include "mcprotocol/serial/detail/validated_frame_codec.hpp"

#include <cstdio>
#include <cstdlib>

namespace mcprotocol::serial {
namespace {

[[nodiscard]] Status span_size_to_points(
    std::size_t size,
    std::uint16_t& out_points,
    const char* what) noexcept {
  if (size > 0xFFFFU) {
    return make_status(StatusCode::InvalidArgument, what);
  }

  out_points = static_cast<std::uint16_t>(size);
  return ok_status();
}

[[nodiscard]] bool trace_enabled() noexcept {
#if defined(_MSC_VER)
  char buffer[8] {};
  std::size_t required = 0;
  if (getenv_s(&required, buffer, sizeof(buffer), "MCPROTOCOL_SERIAL_TRACE") != 0 || required == 0U) {
    return false;
  }
  return buffer[0] != '\0' && buffer[0] != '0';
#else
  const char* value = std::getenv("MCPROTOCOL_SERIAL_TRACE");
  return value != nullptr && value[0] != '\0' && value[0] != '0';
#endif
}

void trace_bytes(const char* label, mcprotocol::serial::Span<const mcprotocol::serial::Byte> bytes) noexcept {
  if (!trace_enabled()) {
    return;
  }
  std::fprintf(stderr, "%s len=%zu hex=", label, bytes.size());
  for (const mcprotocol::serial::Byte value : bytes) {
    std::fprintf(stderr, "%02X", static_cast<unsigned int>(value));
  }
  std::fprintf(stderr, "\n");
}

}  // namespace

void PosixSyncClient::on_request_complete(void* user, Status status) noexcept {
  auto* state = static_cast<CompletionState*>(user);
  state->done = true;
  state->status = status;
}

Status PosixSyncClient::open(
    const PosixSerialConfig& serial_config,
    const ProtocolConfig& protocol_config) noexcept {
  Status status = validate_mc_serial_config(serial_config, protocol_config);
  if (!status.ok()) {
    return status;
  }

  status = FrameCodec::validate_config(protocol_config);
  if (!status.ok()) {
    return status;
  }

  close();
  client_.apply_validated_config(protocol_config);

  status = port_.open(serial_config);
  if (!status.ok()) {
    return status;
  }

  protocol_config_ = protocol_config;
  return ok_status();
}

void PosixSyncClient::close() noexcept {
  client_.cancel();
  port_.close();
  if (client_.busy()) {
    (void)client_.notify_tx_complete(
        now_ms(), make_status(StatusCode::Closed, "Serial transport closed locally during TX"));
  }
}

bool PosixSyncClient::is_open() const noexcept {
  return port_.is_open();
}

const ProtocolConfig& PosixSyncClient::protocol_config() const noexcept {
  return protocol_config_;
}

Status PosixSyncClient::run_until_complete() noexcept {
  return detail::run_synchronous_request(
      client_,
      port_,
      protocol_config_,
      rx_buffer_,
      completion_,
      []() noexcept { return now_ms(); },
      [](const char* label,
         mcprotocol::serial::Span<const mcprotocol::serial::Byte> bytes) noexcept {
        trace_bytes(label, bytes);
      });
}

Status PosixSyncClient::read_cpu_model(CpuModelInfo& out_info) noexcept {
  const Status status = client_.async_read_cpu_model(
      now_ms(),
      out_info,
      &PosixSyncClient::on_request_complete,
      &completion_);
  if (!status.ok()) {
    return status;
  }
  return run_until_complete();
}

Status PosixSyncClient::remote_run(
    RemoteOperationMode mode,
    RemoteRunClearMode clear_mode) noexcept {
  const Status status = client_.async_remote_run(
      now_ms(),
      mode,
      clear_mode,
      &PosixSyncClient::on_request_complete,
      &completion_);
  if (!status.ok()) {
    return status;
  }
  return run_until_complete();
}

Status PosixSyncClient::remote_stop() noexcept {
  const Status status = client_.async_remote_stop(
      now_ms(),
      &PosixSyncClient::on_request_complete,
      &completion_);
  if (!status.ok()) {
    return status;
  }
  return run_until_complete();
}

Status PosixSyncClient::remote_pause(RemoteOperationMode mode) noexcept {
  const Status status = client_.async_remote_pause(
      now_ms(),
      mode,
      &PosixSyncClient::on_request_complete,
      &completion_);
  if (!status.ok()) {
    return status;
  }
  return run_until_complete();
}

Status PosixSyncClient::remote_latch_clear() noexcept {
  const Status status = client_.async_remote_latch_clear(
      now_ms(),
      &PosixSyncClient::on_request_complete,
      &completion_);
  if (!status.ok()) {
    return status;
  }
  return run_until_complete();
}

Status PosixSyncClient::unlock_remote_password(std::string_view remote_password) noexcept {
  const Status status = client_.async_unlock_remote_password(
      now_ms(),
      remote_password,
      &PosixSyncClient::on_request_complete,
      &completion_);
  if (!status.ok()) {
    return status;
  }
  return run_until_complete();
}

Status PosixSyncClient::lock_remote_password(std::string_view remote_password) noexcept {
  const Status status = client_.async_lock_remote_password(
      now_ms(),
      remote_password,
      &PosixSyncClient::on_request_complete,
      &completion_);
  if (!status.ok()) {
    return status;
  }
  return run_until_complete();
}

Status PosixSyncClient::clear_error_information() noexcept {
  const Status status = client_.async_clear_error_information(
      now_ms(),
      &PosixSyncClient::on_request_complete,
      &completion_);
  if (!status.ok()) {
    return status;
  }
  return run_until_complete();
}

Status PosixSyncClient::remote_reset() noexcept {
  const Status status = client_.async_remote_reset(
      now_ms(),
      &PosixSyncClient::on_request_complete,
      &completion_);
  if (!status.ok()) {
    return status;
  }
  return run_until_complete();
}

Status PosixSyncClient::read_user_frame(
    const UserFrameReadRequest& request,
    UserFrameRegistrationData& out_data) noexcept {
  const Status status = client_.async_read_user_frame(
      now_ms(),
      request,
      out_data,
      &PosixSyncClient::on_request_complete,
      &completion_);
  if (!status.ok()) {
    return status;
  }
  return run_until_complete();
}

Status PosixSyncClient::write_user_frame(const UserFrameWriteRequest& request) noexcept {
  const Status status = client_.async_write_user_frame(
      now_ms(),
      request,
      &PosixSyncClient::on_request_complete,
      &completion_);
  if (!status.ok()) {
    return status;
  }
  return run_until_complete();
}

Status PosixSyncClient::delete_user_frame(const UserFrameDeleteRequest& request) noexcept {
  const Status status = client_.async_delete_user_frame(
      now_ms(),
      request,
      &PosixSyncClient::on_request_complete,
      &completion_);
  if (!status.ok()) {
    return status;
  }
  return run_until_complete();
}

Status PosixSyncClient::control_global_signal(
    const GlobalSignalControlRequest& request) noexcept {
  const Status status = client_.async_control_global_signal(
      now_ms(),
      request,
      &PosixSyncClient::on_request_complete,
      &completion_);
  if (!status.ok()) {
    return status;
  }
  return run_until_complete();
}

Status PosixSyncClient::switch_serial_module_mode(
    const SerialModuleModeSwitchRequest& request) noexcept {
  const Status status = client_.async_switch_serial_module_mode(
      now_ms(),
      request,
      &PosixSyncClient::on_request_complete,
      &completion_);
  if (!status.ok()) {
    return status;
  }
  return run_until_complete();
}

Status PosixSyncClient::initialize_c24_transmission_sequence() noexcept {
  const Status status = client_.async_initialize_c24_transmission_sequence(
      now_ms(),
      &PosixSyncClient::on_request_complete,
      &completion_);
  if (!status.ok()) {
    return status;
  }
  return run_until_complete();
}

Status PosixSyncClient::read_words(
    std::string_view head_device,
    std::uint16_t points,
    mcprotocol::serial::Span<std::uint16_t> out_words) noexcept {
  BatchReadWordsRequest request(DeviceAddress {DeviceCode::D, 0U}, 0U);
  Status status = highlevel::make_batch_read_words_request(head_device, points, request);
  if (!status.ok()) {
    return status;
  }

  status = client_.async_batch_read_words(
      now_ms(),
      request,
      out_words,
      &PosixSyncClient::on_request_complete,
      &completion_);
  if (!status.ok()) {
    return status;
  }
  return run_until_complete();
}

Status PosixSyncClient::read_words(
    std::string_view head_device,
    mcprotocol::serial::Span<std::uint16_t> out_words) noexcept {
  std::uint16_t points = 0;
  const Status status = span_size_to_points(
      out_words.size(),
      points,
      "Word-read output span exceeds the protocol point limit");
  if (!status.ok()) {
    return status;
  }
  return read_words(head_device, points, out_words);
}

Status PosixSyncClient::read_extended_file_register_words(
    const ExtendedFileRegisterBatchReadWordsRequest& request,
    mcprotocol::serial::Span<std::uint16_t> out_words) noexcept {
  const Status status = client_.async_read_extended_file_register_words(
      now_ms(),
      request,
      out_words,
      &PosixSyncClient::on_request_complete,
      &completion_);
  if (!status.ok()) {
    return status;
  }
  return run_until_complete();
}

Status PosixSyncClient::direct_read_extended_file_register_words(
    const ExtendedFileRegisterDirectBatchReadWordsRequest& request,
    mcprotocol::serial::Span<std::uint16_t> out_words) noexcept {
  const Status status = client_.async_direct_read_extended_file_register_words(
      now_ms(),
      request,
      out_words,
      &PosixSyncClient::on_request_complete,
      &completion_);
  if (!status.ok()) {
    return status;
  }
  return run_until_complete();
}

Status PosixSyncClient::read_bits(
    std::string_view head_device,
    std::uint16_t points,
    mcprotocol::serial::Span<BitValue> out_bits) noexcept {
  DeviceAddress parsed(DeviceCode::D, 0U);
  Status status = highlevel::parse_device_address(head_device, parsed);
  if (!status.ok()) {
    return status;
  }
  highlevel::LongStateReadSpec long_state_spec(
      highlevel::LongStateReadRoute::StatusBlock,
      DeviceCode::LTN,
      highlevel::LongStateReadKind::Contact);
  status = highlevel::get_long_state_read_spec(parsed.code, long_state_spec);
  if (status.ok()) {
    return make_status(
        StatusCode::InvalidArgument,
        "Long timer/counter state devices must be read with read_long_state_bits");
  }

  BatchReadBitsRequest request(DeviceAddress {DeviceCode::M, 0U}, 0U);
  status = highlevel::make_batch_read_bits_request(head_device, points, request);
  if (!status.ok()) {
    return status;
  }

  status = client_.async_batch_read_bits(
      now_ms(),
      request,
      out_bits,
      &PosixSyncClient::on_request_complete,
      &completion_);
  if (!status.ok()) {
    return status;
  }
  return run_until_complete();
}

Status PosixSyncClient::read_bits(
    std::string_view head_device,
    mcprotocol::serial::Span<BitValue> out_bits) noexcept {
  std::uint16_t points = 0;
  const Status status = span_size_to_points(
      out_bits.size(),
      points,
      "Bit-read output span exceeds the protocol point limit");
  if (!status.ok()) {
    return status;
  }
  return read_bits(head_device, points, out_bits);
}

Status PosixSyncClient::read_link_direct_words(
    std::string_view head_device,
    std::uint16_t points,
    mcprotocol::serial::Span<std::uint16_t> out_words) noexcept {
  LinkDirectDevice device(0U, DeviceAddress {DeviceCode::D, 0U});
  Status status = parse_link_direct_device(head_device, device);
  if (!status.ok()) {
    return status;
  }

  status = client_.async_link_direct_batch_read_words(
      now_ms(),
      device,
      points,
      out_words,
      &PosixSyncClient::on_request_complete,
      &completion_);
  if (!status.ok()) {
    return status;
  }
  return run_until_complete();
}

Status PosixSyncClient::read_link_direct_bits(
    std::string_view head_device,
    std::uint16_t points,
    mcprotocol::serial::Span<BitValue> out_bits) noexcept {
  LinkDirectDevice device(0U, DeviceAddress {DeviceCode::D, 0U});
  Status status = parse_link_direct_device(head_device, device);
  if (!status.ok()) {
    return status;
  }

  status = client_.async_link_direct_batch_read_bits(
      now_ms(),
      device,
      points,
      out_bits,
      &PosixSyncClient::on_request_complete,
      &completion_);
  if (!status.ok()) {
    return status;
  }
  return run_until_complete();
}

Status PosixSyncClient::read_native_qualified_words(
    std::string_view head_device,
    std::uint16_t points,
    mcprotocol::serial::Span<std::uint16_t> out_words) noexcept {
  QualifiedBufferWordDevice device(QualifiedBufferDeviceKind::G, 0U, 0U);
  Status status = parse_qualified_buffer_word_device(head_device, device);
  if (!status.ok()) {
    return status;
  }

  status = client_.async_extended_batch_read_words(
      now_ms(),
      device,
      points,
      out_words,
      &PosixSyncClient::on_request_complete,
      &completion_);
  if (!status.ok()) {
    return status;
  }
  return run_until_complete();
}

Status PosixSyncClient::read_long_state_bits(
    std::string_view head_device,
    std::uint16_t points,
    mcprotocol::serial::Span<BitValue> out_bits) noexcept {
  if (points == 0U) {
    return make_status(StatusCode::InvalidArgument, "Long state bit-read points must be in range 1..65535");
  }
  if (out_bits.size() < points) {
    return make_status(StatusCode::BufferTooSmall, "Long state bit-read output buffer is too small");
  }

  DeviceAddress parsed(DeviceCode::D, 0U);
  Status status = highlevel::parse_device_address(head_device, parsed);
  if (!status.ok()) {
    return status;
  }

  highlevel::LongStateReadSpec spec(
      highlevel::LongStateReadRoute::StatusBlock,
      DeviceCode::LTN,
      highlevel::LongStateReadKind::Contact);
  status = highlevel::get_long_state_read_spec(parsed.code, spec);
  if (!status.ok()) {
    return status;
  }

  if (spec.route == highlevel::LongStateReadRoute::DirectBits) {
    const BatchReadBitsRequest request(parsed, points);
    status = client_.async_batch_read_bits(
        now_ms(),
        request,
        out_bits,
        &PosixSyncClient::on_request_complete,
        &completion_);
    if (!status.ok()) {
      return status;
    }
    return run_until_complete();
  }

  if (parsed.number > (0xFFFFFFFFU - static_cast<std::uint32_t>(points - 1U))) {
    return make_status(StatusCode::InvalidArgument, "Long state bit-read address range overflows");
  }

  // This is the only explicitly aggregate public read in this library.  Validate the complete
  // immutable plan before the first send.  In particular, do not discover a profile, wire-field,
  // request-frame, or response-frame limit after an earlier point has already been read.
  // All intermediate device numbers are bounded by the already-checked final number, so one
  // typed validation of that endpoint covers the complete contiguous plan. It produces command
  // data only; completed wire frames are encoded exactly once later, when each request is sent.
  const BatchReadWordsRequest final_request(
      DeviceAddress {
          spec.base_code,
          parsed.number + static_cast<std::uint32_t>(points - 1U)},
      4U);
  std::array<std::uint8_t, 64U> validation_request_data {};
  std::size_t request_data_size = 0U;
  status = CommandCodec::encode_batch_read_words(
      protocol_config_, final_request, validation_request_data, request_data_size);
  if (!status.ok()) {
    return status;
  }
  status = detail::validate_request_capacity_validated(protocol_config_, request_data_size);
  if (!status.ok()) {
    return status;
  }
  status = detail::validate_response_capacity_validated(
      protocol_config_,
      detail::long_state_status_block_response_bytes(protocol_config_.code_mode()));
  if (!status.ok()) {
    return status;
  }

  // Caller storage is committed only after every internal request succeeds. This blocking wrapper
  // owns the client for the duration of the call; same-instance cross-thread use is prohibited.
  auto read_one = [this](
                      DeviceAddress device,
                      mcprotocol::serial::Span<std::uint16_t> status_block) noexcept -> Status {
    const BatchReadWordsRequest request(
        device,
        static_cast<std::uint16_t>(status_block.size()));

    Status read_status = client_.async_batch_read_words(
        now_ms(),
        request,
        status_block,
        &PosixSyncClient::on_request_complete,
        &completion_);
    if (!read_status.ok()) {
      return read_status;
    }
    return run_until_complete();
  };
  return detail::execute_long_state_read_aggregate(
      spec, parsed.number, points, out_bits, read_one);
}

Status PosixSyncClient::read_long_state_bits(
    std::string_view head_device,
    mcprotocol::serial::Span<BitValue> out_bits) noexcept {
  std::uint16_t points = 0;
  const Status status = span_size_to_points(
      out_bits.size(),
      points,
      "Long state bit-read output span exceeds the protocol point limit");
  if (!status.ok()) {
    return status;
  }
  return read_long_state_bits(head_device, points, out_bits);
}

Status PosixSyncClient::write_words(
    std::string_view head_device,
    mcprotocol::serial::Span<const std::uint16_t> words) noexcept {
  BatchWriteWordsRequest request(DeviceAddress {DeviceCode::D, 0U}, {});
  Status status = highlevel::make_batch_write_words_request(head_device, words, request);
  if (!status.ok()) {
    return status;
  }

  status = client_.async_batch_write_words(
      now_ms(),
      request,
      &PosixSyncClient::on_request_complete,
      &completion_);
  if (!status.ok()) {
    return status;
  }
  return run_until_complete();
}

Status PosixSyncClient::write_link_direct_words(
    std::string_view head_device,
    mcprotocol::serial::Span<const std::uint16_t> words) noexcept {
  LinkDirectDevice device(0U, DeviceAddress {DeviceCode::D, 0U});
  Status status = parse_link_direct_device(head_device, device);
  if (!status.ok()) {
    return status;
  }

  status = client_.async_link_direct_batch_write_words(
      now_ms(),
      device,
      words,
      &PosixSyncClient::on_request_complete,
      &completion_);
  if (!status.ok()) {
    return status;
  }
  return run_until_complete();
}

Status PosixSyncClient::write_link_direct_bits(
    std::string_view head_device,
    mcprotocol::serial::Span<const BitValue> bits) noexcept {
  LinkDirectDevice device(0U, DeviceAddress {DeviceCode::D, 0U});
  Status status = parse_link_direct_device(head_device, device);
  if (!status.ok()) {
    return status;
  }

  status = client_.async_link_direct_batch_write_bits(
      now_ms(),
      device,
      bits,
      &PosixSyncClient::on_request_complete,
      &completion_);
  if (!status.ok()) {
    return status;
  }
  return run_until_complete();
}

Status PosixSyncClient::write_native_qualified_words(
    std::string_view head_device,
    mcprotocol::serial::Span<const std::uint16_t> words) noexcept {
  QualifiedBufferWordDevice device(QualifiedBufferDeviceKind::G, 0U, 0U);
  Status status = parse_qualified_buffer_word_device(head_device, device);
  if (!status.ok()) {
    return status;
  }

  status = client_.async_extended_batch_write_words(
      now_ms(),
      device,
      words,
      &PosixSyncClient::on_request_complete,
      &completion_);
  if (!status.ok()) {
    return status;
  }
  return run_until_complete();
}

Status PosixSyncClient::write_extended_file_register_words(
    const ExtendedFileRegisterBatchWriteWordsRequest& request) noexcept {
  const Status status = client_.async_write_extended_file_register_words(
      now_ms(),
      request,
      &PosixSyncClient::on_request_complete,
      &completion_);
  if (!status.ok()) {
    return status;
  }
  return run_until_complete();
}

Status PosixSyncClient::direct_write_extended_file_register_words(
    const ExtendedFileRegisterDirectBatchWriteWordsRequest& request) noexcept {
  const Status status = client_.async_direct_write_extended_file_register_words(
      now_ms(),
      request,
      &PosixSyncClient::on_request_complete,
      &completion_);
  if (!status.ok()) {
    return status;
  }
  return run_until_complete();
}

Status PosixSyncClient::random_read(
    mcprotocol::serial::Span<const highlevel::RandomReadWordSpec> word_items,
    mcprotocol::serial::Span<const highlevel::RandomReadDWordSpec> dword_items,
    mcprotocol::serial::Span<std::uint16_t> out_words,
    mcprotocol::serial::Span<std::uint32_t> out_dwords) noexcept {
  auto parsed_word_items = detail::make_filled_array<RandomReadWordItem, kMaxRandomAccessItems>(
      RandomReadWordItem {DeviceAddress {DeviceCode::D, 0U}});
  auto parsed_dword_items = detail::make_filled_array<RandomReadDWordItem, kMaxRandomAccessItems>(
      RandomReadDWordItem {DeviceAddress {DeviceCode::D, 0U}});
  RandomReadRequest request({}, {});
  Status status = highlevel::make_random_read_request(
      word_items, dword_items, parsed_word_items, parsed_dword_items, request);
  if (!status.ok()) {
    return status;
  }

  status = client_.async_random_read(
      now_ms(),
      request,
      out_words,
      out_dwords,
      &PosixSyncClient::on_request_complete,
      &completion_);
  if (!status.ok()) {
    return status;
  }
  return run_until_complete();
}

Status PosixSyncClient::random_read_word(
    std::string_view device,
    std::uint16_t& out_value) noexcept {
  const std::array<highlevel::RandomReadWordSpec, 1> items {{
      highlevel::RandomReadWordSpec(device),
  }};
  return random_read(items, {}, mcprotocol::serial::Span<std::uint16_t>(&out_value, 1U), {});
}

Status PosixSyncClient::random_read_dword(
    std::string_view device,
    std::uint32_t& out_value) noexcept {
  const std::array<highlevel::RandomReadDWordSpec, 1> items {{
      highlevel::RandomReadDWordSpec(device),
  }};
  return random_read({}, items, {}, mcprotocol::serial::Span<std::uint32_t>(&out_value, 1U));
}

Status PosixSyncClient::random_write_words(
    mcprotocol::serial::Span<const highlevel::RandomWriteWordSpec> items) noexcept {
  auto parsed_items = detail::make_filled_array<RandomWriteWordItem, kMaxRandomAccessItems>(
      RandomWriteWordItem(DeviceAddress {DeviceCode::D, 0U}, 0U));
  mcprotocol::serial::Span<const RandomWriteWordItem> request_items {};
  Status status = highlevel::make_random_write_word_items(items, parsed_items, request_items);
  if (!status.ok()) {
    return status;
  }

  status = client_.async_random_write_words(
      now_ms(),
      request_items,
      {},
      &PosixSyncClient::on_request_complete,
      &completion_);
  if (!status.ok()) {
    return status;
  }
  return run_until_complete();
}

Status PosixSyncClient::random_write_dwords(
    mcprotocol::serial::Span<const highlevel::RandomWriteDWordSpec> items) noexcept {
  auto parsed_items = detail::make_filled_array<RandomWriteDWordItem, kMaxRandomAccessItems>(
      RandomWriteDWordItem(DeviceAddress {DeviceCode::D, 0U}, 0U));
  mcprotocol::serial::Span<const RandomWriteDWordItem> request_items {};
  Status status = highlevel::make_random_write_dword_items(items, parsed_items, request_items);
  if (!status.ok()) {
    return status;
  }
  status = client_.async_random_write_words(
      now_ms(), {}, request_items, &PosixSyncClient::on_request_complete, &completion_);
  if (!status.ok()) {
    return status;
  }
  return run_until_complete();
}

Status PosixSyncClient::random_write_extended_file_register_words(
    mcprotocol::serial::Span<const ExtendedFileRegisterRandomWriteWordItem> items) noexcept {
  const Status status = client_.async_random_write_extended_file_register_words(
      now_ms(),
      items,
      &PosixSyncClient::on_request_complete,
      &completion_);
  if (!status.ok()) {
    return status;
  }
  return run_until_complete();
}

Status PosixSyncClient::random_write_word(
    std::string_view device,
    std::uint16_t value) noexcept {
  const std::array<highlevel::RandomWriteWordSpec, 1> items {{
      highlevel::RandomWriteWordSpec(device, value),
  }};
  return random_write_words(items);
}

Status PosixSyncClient::random_write_dword(
    std::string_view device,
    std::uint32_t value) noexcept {
  const std::array<highlevel::RandomWriteDWordSpec, 1> items {{
      highlevel::RandomWriteDWordSpec(device, value),
  }};
  return random_write_dwords(items);
}

Status PosixSyncClient::random_write_bits(
    mcprotocol::serial::Span<const highlevel::RandomWriteBitSpec> items) noexcept {
  auto parsed_items = detail::make_filled_array<RandomWriteBitItem, kMaxRandomAccessItems>(
      RandomWriteBitItem(DeviceAddress {DeviceCode::M, 0U}, false));
  mcprotocol::serial::Span<const RandomWriteBitItem> request_items {};
  Status status = highlevel::make_random_write_bit_items(items, parsed_items, request_items);
  if (!status.ok()) {
    return status;
  }

  status = client_.async_random_write_bits(
      now_ms(),
      request_items,
      &PosixSyncClient::on_request_complete,
      &completion_);
  if (!status.ok()) {
    return status;
  }
  return run_until_complete();
}

Status PosixSyncClient::random_write_bit(
    std::string_view device,
    BitValue value) noexcept {
  const std::array<highlevel::RandomWriteBitSpec, 1> items {{
      highlevel::RandomWriteBitSpec(device, value),
  }};
  return random_write_bits(items);
}

Status PosixSyncClient::register_monitor(
    mcprotocol::serial::Span<const highlevel::RandomReadWordSpec> word_items,
    mcprotocol::serial::Span<const highlevel::RandomReadDWordSpec> dword_items) noexcept {
  auto parsed_word_items = detail::make_filled_array<RandomReadWordItem, kMaxMonitorItems>(
      RandomReadWordItem {DeviceAddress {DeviceCode::D, 0U}});
  auto parsed_dword_items = detail::make_filled_array<RandomReadDWordItem, kMaxMonitorItems>(
      RandomReadDWordItem {DeviceAddress {DeviceCode::D, 0U}});
  MonitorRegistration request({}, {});
  Status status = highlevel::make_monitor_registration(
      word_items, dword_items, parsed_word_items, parsed_dword_items, request);
  if (!status.ok()) {
    return status;
  }

  status = client_.async_register_monitor(
      now_ms(),
      request,
      &PosixSyncClient::on_request_complete,
      &completion_);
  if (!status.ok()) {
    return status;
  }
  return run_until_complete();
}

Status PosixSyncClient::register_monitor_word(std::string_view device) noexcept {
  const std::array<highlevel::RandomReadWordSpec, 1> items {{
      highlevel::RandomReadWordSpec(device),
  }};
  return register_monitor(items, {});
}

Status PosixSyncClient::register_monitor_dword(std::string_view device) noexcept {
  const std::array<highlevel::RandomReadDWordSpec, 1> items {{
      highlevel::RandomReadDWordSpec(device),
  }};
  return register_monitor({}, items);
}

Status PosixSyncClient::register_extended_file_register_monitor(
    const ExtendedFileRegisterMonitorRegistration& request) noexcept {
  const Status status = client_.async_register_extended_file_register_monitor(
      now_ms(),
      request,
      &PosixSyncClient::on_request_complete,
      &completion_);
  if (!status.ok()) {
    return status;
  }
  return run_until_complete();
}

Status PosixSyncClient::read_monitor(
    mcprotocol::serial::Span<std::uint16_t> out_words,
    mcprotocol::serial::Span<std::uint32_t> out_dwords) noexcept {
  const Status status = client_.async_read_monitor(
      now_ms(),
      out_words,
      out_dwords,
      &PosixSyncClient::on_request_complete,
      &completion_);
  if (!status.ok()) {
    return status;
  }
  return run_until_complete();
}

Status PosixSyncClient::read_extended_file_register_monitor(
    mcprotocol::serial::Span<std::uint16_t> out_words) noexcept {
  const Status status = client_.async_read_extended_file_register_monitor(
      now_ms(),
      out_words,
      &PosixSyncClient::on_request_complete,
      &completion_);
  if (!status.ok()) {
    return status;
  }
  return run_until_complete();
}

Status PosixSyncClient::write_bits(
    std::string_view head_device,
    mcprotocol::serial::Span<const BitValue> bits) noexcept {
  BatchWriteBitsRequest request(DeviceAddress {DeviceCode::M, 0U}, {});
  Status status = highlevel::make_batch_write_bits_request(head_device, bits, request);
  if (!status.ok()) {
    return status;
  }

  status = client_.async_batch_write_bits(
      now_ms(),
      request,
      &PosixSyncClient::on_request_complete,
      &completion_);
  if (!status.ok()) {
    return status;
  }
  return run_until_complete();
}

}  // namespace mcprotocol::serial
