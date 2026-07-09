# MC Protocol Serial C++ API Reference

This file is generated from Doxygen XML for the public C++ headers.
Do not edit it manually; run `scripts/generate_api_reference.py` instead.

## Header Inputs

- `include/mcprotocol_serial.hpp`
- `include/mcprotocol/serial/types.hpp`
- `include/mcprotocol/serial/status.hpp`
- `include/mcprotocol/serial/codec.hpp`
- `include/mcprotocol/serial/client.hpp`
- `include/mcprotocol/serial/high_level.hpp`
- `include/mcprotocol/serial/host_sync.hpp`
- `include/mcprotocol/serial/posix_serial.hpp`
- `include/mcprotocol/serial/link_direct.hpp`
- `include/mcprotocol/serial/qualified_buffer.hpp`
- `include/mcprotocol/serial/version.hpp`

### Public Header Macros In `include/mcprotocol/serial/types.hpp`

#### Configuration Macros

#### `MCPROTOCOL_SERIAL_MAX_REQUEST_FRAME_BYTES`

```cpp
#define MCPROTOCOL_SERIAL_MAX_REQUEST_FRAME_BYTES 4096U
```

#### `MCPROTOCOL_SERIAL_MAX_RESPONSE_FRAME_BYTES`

```cpp
#define MCPROTOCOL_SERIAL_MAX_RESPONSE_FRAME_BYTES 4096U
```

#### `MCPROTOCOL_SERIAL_MAX_REQUEST_DATA_BYTES`

```cpp
#define MCPROTOCOL_SERIAL_MAX_REQUEST_DATA_BYTES 3500U
```

#### `MCPROTOCOL_SERIAL_MAX_BATCH_WORD_POINTS`

```cpp
#define MCPROTOCOL_SERIAL_MAX_BATCH_WORD_POINTS 960U
```

#### `MCPROTOCOL_SERIAL_MAX_BATCH_BIT_POINTS_ASCII`

```cpp
#define MCPROTOCOL_SERIAL_MAX_BATCH_BIT_POINTS_ASCII 7904U
```

#### `MCPROTOCOL_SERIAL_MAX_BATCH_BIT_POINTS_BINARY`

```cpp
#define MCPROTOCOL_SERIAL_MAX_BATCH_BIT_POINTS_BINARY 7904U
```

#### `MCPROTOCOL_SERIAL_MAX_RANDOM_ACCESS_ITEMS`

```cpp
#define MCPROTOCOL_SERIAL_MAX_RANDOM_ACCESS_ITEMS 192U
```

#### `MCPROTOCOL_SERIAL_MAX_MULTI_BLOCK_COUNT`

```cpp
#define MCPROTOCOL_SERIAL_MAX_MULTI_BLOCK_COUNT 120U
```

#### `MCPROTOCOL_SERIAL_MAX_MONITOR_ITEMS`

```cpp
#define MCPROTOCOL_SERIAL_MAX_MONITOR_ITEMS 192U
```

#### `MCPROTOCOL_SERIAL_MAX_LOOPBACK_BYTES`

```cpp
#define MCPROTOCOL_SERIAL_MAX_LOOPBACK_BYTES 960U
```

#### `MCPROTOCOL_SERIAL_ENABLE_RANDOM_COMMANDS`

```cpp
#define MCPROTOCOL_SERIAL_ENABLE_RANDOM_COMMANDS 1
```

#### `MCPROTOCOL_SERIAL_ENABLE_MULTI_BLOCK_COMMANDS`

```cpp
#define MCPROTOCOL_SERIAL_ENABLE_MULTI_BLOCK_COMMANDS 1
```

#### `MCPROTOCOL_SERIAL_ENABLE_MONITOR_COMMANDS`

```cpp
#define MCPROTOCOL_SERIAL_ENABLE_MONITOR_COMMANDS 1
```

#### `MCPROTOCOL_SERIAL_ENABLE_HOST_BUFFER_COMMANDS`

```cpp
#define MCPROTOCOL_SERIAL_ENABLE_HOST_BUFFER_COMMANDS 1
```

#### `MCPROTOCOL_SERIAL_ENABLE_MODULE_BUFFER_COMMANDS`

```cpp
#define MCPROTOCOL_SERIAL_ENABLE_MODULE_BUFFER_COMMANDS 1
```

#### `MCPROTOCOL_SERIAL_ENABLE_CPU_MODEL_COMMANDS`

```cpp
#define MCPROTOCOL_SERIAL_ENABLE_CPU_MODEL_COMMANDS 1
```

#### `MCPROTOCOL_SERIAL_ENABLE_LOOPBACK_COMMANDS`

```cpp
#define MCPROTOCOL_SERIAL_ENABLE_LOOPBACK_COMMANDS 1
```

#### `MCPROTOCOL_SERIAL_ENABLE_ASCII_MODE`

```cpp
#define MCPROTOCOL_SERIAL_ENABLE_ASCII_MODE 1
```

#### `MCPROTOCOL_SERIAL_ENABLE_BINARY_MODE`

```cpp
#define MCPROTOCOL_SERIAL_ENABLE_BINARY_MODE 1
```

#### `MCPROTOCOL_SERIAL_ENABLE_FRAME_C4`

```cpp
#define MCPROTOCOL_SERIAL_ENABLE_FRAME_C4 1
```

#### `MCPROTOCOL_SERIAL_ENABLE_FRAME_C3`

```cpp
#define MCPROTOCOL_SERIAL_ENABLE_FRAME_C3 1
```

#### `MCPROTOCOL_SERIAL_ENABLE_FRAME_C2`

```cpp
#define MCPROTOCOL_SERIAL_ENABLE_FRAME_C2 1
```

#### `MCPROTOCOL_SERIAL_ENABLE_FRAME_C1`

```cpp
#define MCPROTOCOL_SERIAL_ENABLE_FRAME_C1 1
```

#### `MCPROTOCOL_SERIAL_ENABLE_FRAME_E1`

```cpp
#define MCPROTOCOL_SERIAL_ENABLE_FRAME_E1 1
```

### Public Header Macros In `include/mcprotocol/serial/version.hpp`

#### Configuration Macros

#### `MCPROTOCOL_SERIAL_VERSION_MAJOR`

```cpp
#define MCPROTOCOL_SERIAL_VERSION_MAJOR 2
```

#### `MCPROTOCOL_SERIAL_VERSION_MINOR`

```cpp
#define MCPROTOCOL_SERIAL_VERSION_MINOR 0
```

#### `MCPROTOCOL_SERIAL_VERSION_PATCH`

```cpp
#define MCPROTOCOL_SERIAL_VERSION_PATCH 1
```

#### `MCPROTOCOL_SERIAL_VERSION_STRING`

```cpp
#define MCPROTOCOL_SERIAL_VERSION_STRING "2.0.1"
```

## Namespaces

### Namespace `mcprotocol::serial::CommandCodec`

Command-payload codec helpers below the frame layer.

These helpers operate on request/response data only. They do not add or remove the surrounding C1/C2/C3/C4 frame bytes.

#### Functions

#### `encode_batch_read_words`

```cpp
Status mcprotocol::serial::CommandCodec::encode_batch_read_words(const ProtocolConfig &config, const BatchReadWordsRequest &request, std::span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_read_extended_file_register_words`

```cpp
Status mcprotocol::serial::CommandCodec::encode_read_extended_file_register_words(const ProtocolConfig &config, const ExtendedFileRegisterBatchReadWordsRequest &request, std::span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_direct_read_extended_file_register_words`

```cpp
Status mcprotocol::serial::CommandCodec::encode_direct_read_extended_file_register_words(const ProtocolConfig &config, const ExtendedFileRegisterDirectBatchReadWordsRequest &request, std::span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_extended_batch_read_words`

```cpp
Status mcprotocol::serial::CommandCodec::encode_extended_batch_read_words(const ProtocolConfig &config, const QualifiedBufferWordDevice &device, std::uint16_t points, std::span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_link_direct_batch_read_words`

```cpp
Status mcprotocol::serial::CommandCodec::encode_link_direct_batch_read_words(const ProtocolConfig &config, const LinkDirectDevice &device, std::uint16_t points, std::span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `parse_batch_read_words_response`

```cpp
Status mcprotocol::serial::CommandCodec::parse_batch_read_words_response(const ProtocolConfig &config, const BatchReadWordsRequest &request, std::span< const std::uint8_t > response_data, std::span< std::uint16_t > out_words) noexcept
```

#### `parse_read_extended_file_register_words_response`

```cpp
Status mcprotocol::serial::CommandCodec::parse_read_extended_file_register_words_response(const ProtocolConfig &config, std::uint16_t points, std::span< const std::uint8_t > response_data, std::span< std::uint16_t > out_words) noexcept
```

#### `parse_extended_batch_read_words_response`

```cpp
Status mcprotocol::serial::CommandCodec::parse_extended_batch_read_words_response(const ProtocolConfig &config, std::uint16_t points, std::span< const std::uint8_t > response_data, std::span< std::uint16_t > out_words) noexcept
```

#### `encode_batch_read_bits`

```cpp
Status mcprotocol::serial::CommandCodec::encode_batch_read_bits(const ProtocolConfig &config, const BatchReadBitsRequest &request, std::span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_link_direct_batch_read_bits`

```cpp
Status mcprotocol::serial::CommandCodec::encode_link_direct_batch_read_bits(const ProtocolConfig &config, const LinkDirectDevice &device, std::uint16_t points, std::span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `parse_batch_read_bits_response`

```cpp
Status mcprotocol::serial::CommandCodec::parse_batch_read_bits_response(const ProtocolConfig &config, const BatchReadBitsRequest &request, std::span< const std::uint8_t > response_data, std::span< BitValue > out_bits) noexcept
```

#### `encode_batch_write_words`

```cpp
Status mcprotocol::serial::CommandCodec::encode_batch_write_words(const ProtocolConfig &config, const BatchWriteWordsRequest &request, std::span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_write_extended_file_register_words`

```cpp
Status mcprotocol::serial::CommandCodec::encode_write_extended_file_register_words(const ProtocolConfig &config, const ExtendedFileRegisterBatchWriteWordsRequest &request, std::span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_direct_write_extended_file_register_words`

```cpp
Status mcprotocol::serial::CommandCodec::encode_direct_write_extended_file_register_words(const ProtocolConfig &config, const ExtendedFileRegisterDirectBatchWriteWordsRequest &request, std::span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_link_direct_batch_write_words`

```cpp
Status mcprotocol::serial::CommandCodec::encode_link_direct_batch_write_words(const ProtocolConfig &config, const LinkDirectDevice &device, std::span< const std::uint16_t > words, std::span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_extended_batch_write_words`

```cpp
Status mcprotocol::serial::CommandCodec::encode_extended_batch_write_words(const ProtocolConfig &config, const QualifiedBufferWordDevice &device, std::span< const std::uint16_t > words, std::span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_batch_write_bits`

```cpp
Status mcprotocol::serial::CommandCodec::encode_batch_write_bits(const ProtocolConfig &config, const BatchWriteBitsRequest &request, std::span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_link_direct_batch_write_bits`

```cpp
Status mcprotocol::serial::CommandCodec::encode_link_direct_batch_write_bits(const ProtocolConfig &config, const LinkDirectDevice &device, std::span< const BitValue > bits, std::span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_link_direct_random_read`

```cpp
Status mcprotocol::serial::CommandCodec::encode_link_direct_random_read(const ProtocolConfig &config, std::span< const LinkDirectRandomReadItem > items, std::span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_random_read`

```cpp
Status mcprotocol::serial::CommandCodec::encode_random_read(const ProtocolConfig &config, const RandomReadRequest &request, std::span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `parse_random_read_response`

```cpp
Status mcprotocol::serial::CommandCodec::parse_random_read_response(const ProtocolConfig &config, std::span< const RandomReadItem > items, std::span< const std::uint8_t > response_data, std::span< std::uint32_t > out_values) noexcept
```

#### `encode_random_write_words`

```cpp
Status mcprotocol::serial::CommandCodec::encode_random_write_words(const ProtocolConfig &config, std::span< const RandomWriteWordItem > items, std::span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_random_write_extended_file_register_words`

```cpp
Status mcprotocol::serial::CommandCodec::encode_random_write_extended_file_register_words(const ProtocolConfig &config, std::span< const ExtendedFileRegisterRandomWriteWordItem > items, std::span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_link_direct_random_write_words`

```cpp
Status mcprotocol::serial::CommandCodec::encode_link_direct_random_write_words(const ProtocolConfig &config, std::span< const LinkDirectRandomWriteWordItem > items, std::span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_random_write_bits`

```cpp
Status mcprotocol::serial::CommandCodec::encode_random_write_bits(const ProtocolConfig &config, std::span< const RandomWriteBitItem > items, std::span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_link_direct_random_write_bits`

```cpp
Status mcprotocol::serial::CommandCodec::encode_link_direct_random_write_bits(const ProtocolConfig &config, std::span< const LinkDirectRandomWriteBitItem > items, std::span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_link_direct_multi_block_read`

```cpp
Status mcprotocol::serial::CommandCodec::encode_link_direct_multi_block_read(const ProtocolConfig &config, const LinkDirectMultiBlockReadRequest &request, std::span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_multi_block_read`

```cpp
Status mcprotocol::serial::CommandCodec::encode_multi_block_read(const ProtocolConfig &config, const MultiBlockReadRequest &request, std::span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `parse_multi_block_read_response`

```cpp
Status mcprotocol::serial::CommandCodec::parse_multi_block_read_response(const ProtocolConfig &config, std::span< const MultiBlockReadBlock > blocks, std::span< const std::uint8_t > response_data, std::span< std::uint16_t > out_words, std::span< BitValue > out_bits, std::span< MultiBlockReadBlockResult > out_results) noexcept
```

#### `encode_multi_block_write`

```cpp
Status mcprotocol::serial::CommandCodec::encode_multi_block_write(const ProtocolConfig &config, const MultiBlockWriteRequest &request, std::span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_link_direct_multi_block_write`

```cpp
Status mcprotocol::serial::CommandCodec::encode_link_direct_multi_block_write(const ProtocolConfig &config, const LinkDirectMultiBlockWriteRequest &request, std::span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_register_monitor`

```cpp
Status mcprotocol::serial::CommandCodec::encode_register_monitor(const ProtocolConfig &config, const MonitorRegistration &request, std::span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_register_extended_file_register_monitor`

```cpp
Status mcprotocol::serial::CommandCodec::encode_register_extended_file_register_monitor(const ProtocolConfig &config, const ExtendedFileRegisterMonitorRegistration &request, std::span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_link_direct_register_monitor`

```cpp
Status mcprotocol::serial::CommandCodec::encode_link_direct_register_monitor(const ProtocolConfig &config, const LinkDirectMonitorRegistration &request, std::span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_read_monitor`

```cpp
Status mcprotocol::serial::CommandCodec::encode_read_monitor(const ProtocolConfig &config, std::span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_read_monitor`

```cpp
Status mcprotocol::serial::CommandCodec::encode_read_monitor(const ProtocolConfig &config, std::span< const RandomReadItem > items, std::span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_read_extended_file_register_monitor`

```cpp
Status mcprotocol::serial::CommandCodec::encode_read_extended_file_register_monitor(const ProtocolConfig &config, std::span< const ExtendedFileRegisterAddress > items, std::span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `parse_read_monitor_response`

```cpp
Status mcprotocol::serial::CommandCodec::parse_read_monitor_response(const ProtocolConfig &config, std::span< const RandomReadItem > items, std::span< const std::uint8_t > response_data, std::span< std::uint32_t > out_values) noexcept
```

#### `parse_read_extended_file_register_monitor_response`

```cpp
Status mcprotocol::serial::CommandCodec::parse_read_extended_file_register_monitor_response(const ProtocolConfig &config, std::span< const ExtendedFileRegisterAddress > items, std::span< const std::uint8_t > response_data, std::span< std::uint16_t > out_words) noexcept
```

#### `encode_read_user_frame`

```cpp
Status mcprotocol::serial::CommandCodec::encode_read_user_frame(const ProtocolConfig &config, const UserFrameReadRequest &request, std::span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `parse_read_user_frame_response`

```cpp
Status mcprotocol::serial::CommandCodec::parse_read_user_frame_response(const ProtocolConfig &config, std::span< const std::uint8_t > response_data, UserFrameRegistrationData &out_data) noexcept
```

#### `encode_write_user_frame`

```cpp
Status mcprotocol::serial::CommandCodec::encode_write_user_frame(const ProtocolConfig &config, const UserFrameWriteRequest &request, std::span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_delete_user_frame`

```cpp
Status mcprotocol::serial::CommandCodec::encode_delete_user_frame(const ProtocolConfig &config, const UserFrameDeleteRequest &request, std::span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_control_global_signal`

```cpp
Status mcprotocol::serial::CommandCodec::encode_control_global_signal(const ProtocolConfig &config, const GlobalSignalControlRequest &request, std::span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_switch_serial_module_mode`

```cpp
Status mcprotocol::serial::CommandCodec::encode_switch_serial_module_mode(const ProtocolConfig &config, const SerialModuleModeSwitchRequest &request, std::span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_initialize_transmission_sequence`

```cpp
Status mcprotocol::serial::CommandCodec::encode_initialize_transmission_sequence(const ProtocolConfig &config, std::span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_read_host_buffer`

```cpp
Status mcprotocol::serial::CommandCodec::encode_read_host_buffer(const ProtocolConfig &config, const HostBufferReadRequest &request, std::span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `parse_read_host_buffer_response`

```cpp
Status mcprotocol::serial::CommandCodec::parse_read_host_buffer_response(const ProtocolConfig &config, const HostBufferReadRequest &request, std::span< const std::uint8_t > response_data, std::span< std::uint16_t > out_words) noexcept
```

#### `encode_write_host_buffer`

```cpp
Status mcprotocol::serial::CommandCodec::encode_write_host_buffer(const ProtocolConfig &config, const HostBufferWriteRequest &request, std::span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_read_module_buffer`

```cpp
Status mcprotocol::serial::CommandCodec::encode_read_module_buffer(const ProtocolConfig &config, const ModuleBufferReadRequest &request, std::span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `parse_read_module_buffer_response`

```cpp
Status mcprotocol::serial::CommandCodec::parse_read_module_buffer_response(const ProtocolConfig &config, const ModuleBufferReadRequest &request, std::span< const std::uint8_t > response_data, std::span< std::byte > out_bytes) noexcept
```

#### `encode_write_module_buffer`

```cpp
Status mcprotocol::serial::CommandCodec::encode_write_module_buffer(const ProtocolConfig &config, const ModuleBufferWriteRequest &request, std::span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_read_cpu_model`

```cpp
Status mcprotocol::serial::CommandCodec::encode_read_cpu_model(const ProtocolConfig &config, std::span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `parse_read_cpu_model_response`

```cpp
Status mcprotocol::serial::CommandCodec::parse_read_cpu_model_response(const ProtocolConfig &config, std::span< const std::uint8_t > response_data, CpuModelInfo &out_info) noexcept
```

#### `encode_remote_run`

```cpp
Status mcprotocol::serial::CommandCodec::encode_remote_run(const ProtocolConfig &config, RemoteOperationMode mode, RemoteRunClearMode clear_mode, std::span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_remote_stop`

```cpp
Status mcprotocol::serial::CommandCodec::encode_remote_stop(const ProtocolConfig &config, std::span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_remote_pause`

```cpp
Status mcprotocol::serial::CommandCodec::encode_remote_pause(const ProtocolConfig &config, RemoteOperationMode mode, std::span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_remote_latch_clear`

```cpp
Status mcprotocol::serial::CommandCodec::encode_remote_latch_clear(const ProtocolConfig &config, std::span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_remote_reset`

```cpp
Status mcprotocol::serial::CommandCodec::encode_remote_reset(const ProtocolConfig &config, std::span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_unlock_remote_password`

```cpp
Status mcprotocol::serial::CommandCodec::encode_unlock_remote_password(const ProtocolConfig &config, std::string_view remote_password, std::span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_lock_remote_password`

```cpp
Status mcprotocol::serial::CommandCodec::encode_lock_remote_password(const ProtocolConfig &config, std::string_view remote_password, std::span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_clear_error_information`

```cpp
Status mcprotocol::serial::CommandCodec::encode_clear_error_information(const ProtocolConfig &config, std::span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_loopback`

```cpp
Status mcprotocol::serial::CommandCodec::encode_loopback(const ProtocolConfig &config, std::span< const char > hex_ascii, std::span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `parse_loopback_response`

```cpp
Status mcprotocol::serial::CommandCodec::parse_loopback_response(const ProtocolConfig &config, std::span< const std::uint8_t > response_data, std::span< char > out_echoed) noexcept
```

#### `module_buffer_start_address`

```cpp
std::uint32_t mcprotocol::serial::CommandCodec::module_buffer_start_address(std::uint32_t buffer_memory_address, std::uint32_t module_additional_value) noexcept
```

Converts a logical buffer-memory word address plus module offset into a byte start address.

### Namespace `mcprotocol::serial::highlevel`

#### Enums

#### `LongStateReadKind`

Logical state selected from a long timer/counter status block.

| Value | Description |
| --- | --- |
| `Contact` |  |
| `Coil` |  |

#### `LongStateReadRoute`

| Value | Description |
| --- | --- |
| `StatusBlock` |  |
| `DirectBits` |  |

#### Functions

#### `make_c4_binary_protocol`

```cpp
ProtocolConfig mcprotocol::serial::highlevel::make_c4_binary_protocol(PlcProfile profile) noexcept
```

Returns a practical Format5 / Binary / C4 configuration for an explicit PLC profile.

#### `make_c4_ascii_format4_protocol`

```cpp
ProtocolConfig mcprotocol::serial::highlevel::make_c4_ascii_format4_protocol(PlcProfile profile) noexcept
```

Returns a practical Format4 / ASCII / C4 configuration for an explicit PLC profile.

#### `make_c4_ascii_format2_protocol`

```cpp
ProtocolConfig mcprotocol::serial::highlevel::make_c4_ascii_format2_protocol(PlcProfile profile) noexcept
```

Returns a practical default for Format2 / ASCII / C4.

Format2 is the Format1 style ENQ/ACK/NAK/STX/ETX link with an extra 1-byte block number inserted before the frame ID. The default block number is 0x00; change ProtocolConfig::ascii_block_number if the host side needs a different value.

#### `parse_device_address`

```cpp
Status mcprotocol::serial::highlevel::parse_device_address(std::string_view text, DeviceAddress &out_device) noexcept
```

Parses a plain MC device string such as D100, M100, X10, or B20.

This helper is intentionally limited to plain device syntax. It does not parse Jn\\... link- direct addresses, helper-qualified U...\\G... addresses, or standalone G / HG.

#### `get_long_state_read_spec`

```cpp
Status mcprotocol::serial::highlevel::get_long_state_read_spec(DeviceCode code, LongStateReadSpec &out_spec) noexcept
```

Resolves the dedicated read path for long timer/counter state devices.

LTS/LTC/LSTS/LSTC/LCS/LCC are read through this helper. Timer state devices use the corresponding LTN/LSTN 4-word status block; long counter contacts/coils use direct bit access.

#### `decode_long_state_bit`

```cpp
Status mcprotocol::serial::highlevel::decode_long_state_bit(const LongStateReadSpec &spec, std::span< const std::uint16_t > status_block_words, BitValue &out_value) noexcept
```

Decodes the contact/coil bit from a long-family 4-word status block.

#### `make_batch_read_words_request`

```cpp
Status mcprotocol::serial::highlevel::make_batch_read_words_request(std::string_view head_device, std::uint16_t points, BatchReadWordsRequest &out_request) noexcept
```

Builds a contiguous word-read request from a string address such as D100.

#### `make_batch_read_bits_request`

```cpp
Status mcprotocol::serial::highlevel::make_batch_read_bits_request(std::string_view head_device, std::uint16_t points, BatchReadBitsRequest &out_request) noexcept
```

Builds a contiguous bit-read request from a string address such as M100.

#### `make_batch_write_words_request`

```cpp
Status mcprotocol::serial::highlevel::make_batch_write_words_request(std::string_view head_device, std::span< const std::uint16_t > words, BatchWriteWordsRequest &out_request) noexcept
```

Builds a contiguous word-write request from a string address such as D100.

#### `make_batch_write_bits_request`

```cpp
Status mcprotocol::serial::highlevel::make_batch_write_bits_request(std::string_view head_device, std::span< const BitValue > bits, BatchWriteBitsRequest &out_request) noexcept
```

Builds a contiguous bit-write request from a string address such as M100.

#### `make_random_read_item`

```cpp
Status mcprotocol::serial::highlevel::make_random_read_item(std::string_view device, RandomReadItem &out_item, bool double_word=false) noexcept
```

Builds one sparse random-read item from a string address.

#### `make_random_write_word_item`

```cpp
Status mcprotocol::serial::highlevel::make_random_write_word_item(std::string_view device, std::uint32_t value, RandomWriteWordItem &out_item, bool double_word=false) noexcept
```

Builds one sparse random word-write item from a string address.

#### `make_random_write_bit_item`

```cpp
Status mcprotocol::serial::highlevel::make_random_write_bit_item(std::string_view device, BitValue value, RandomWriteBitItem &out_item) noexcept
```

Builds one sparse random bit-write item from a string address.

#### `make_random_read_request`

```cpp
Status mcprotocol::serial::highlevel::make_random_read_request(std::span< const RandomReadSpec > specs, std::span< RandomReadItem > out_items, RandomReadRequest &out_request) noexcept
```

Builds a sparse random-read request from string-address specs.

Use this when you want 0403 style sparse addressing without hand-filling RandomReadItem entries.

#### `make_monitor_registration`

```cpp
Status mcprotocol::serial::highlevel::make_monitor_registration(std::span< const RandomReadSpec > specs, std::span< RandomReadItem > out_items, MonitorRegistration &out_request) noexcept
```

Builds a sparse monitor registration payload from string-address specs.

The resulting payload is intended for 0801. Readback still happens through the normal monitor read API.

#### `make_random_write_word_items`

```cpp
Status mcprotocol::serial::highlevel::make_random_write_word_items(std::span< const RandomWriteWordSpec > specs, std::span< RandomWriteWordItem > out_items, std::span< const RandomWriteWordItem > &out_item_view) noexcept
```

Builds sparse random word-write items from string-address specs.

#### `make_random_write_bit_items`

```cpp
Status mcprotocol::serial::highlevel::make_random_write_bit_items(std::span< const RandomWriteBitSpec > specs, std::span< RandomWriteBitItem > out_items, std::span< const RandomWriteBitItem > &out_item_view) noexcept
```

Builds sparse random bit-write items from string-address specs.

### Namespace `mcprotocol::serial`

#### Aliases

#### `CompletionHandler`

```cpp
using mcprotocol::serial::CompletionHandler = void (*)(void* user, Status status)
```

Completion callback used by the async client.

The callback receives the original user pointer and the final request status.

#### Enums

#### `StatusCode`

Library-level status code returned by encode, decode, transport, and client operations.

| Value | Description |
| --- | --- |
| `Ok = 0` |  |
| `InvalidArgument` |  |
| `Busy` |  |
| `Timeout` |  |
| `Transport` |  |
| `Framing` |  |
| `SumCheckMismatch` |  |
| `Parse` |  |
| `UnsupportedConfiguration` |  |
| `PlcError` |  |
| `BufferTooSmall` |  |
| `Cancelled` |  |

#### `QualifiedBufferDeviceKind`

Qualified buffer-memory family used by helper U... accessors.

| Value | Description |
| --- | --- |
| `G` |  |
| `HG` |  |

#### `DecodeStatus`

High-level result of frame decoding.

| Value | Description |
| --- | --- |
| `Incomplete` | More bytes are required before a full frame can be decoded. |
| `Complete` | One full frame was decoded successfully. |
| `Error` | The byte stream is invalid for the selected frame configuration. |

#### `FrameKind`

MC protocol frame family used on the serial link.

| Value | Description |
| --- | --- |
| `C4` | Chapter-8/10/11/13 oriented serial frame with the fullest feature coverage in this repository. |
| `C3` | Shorter ASCII serial frame that reuses the C4 payload codec. |
| `C2` | Smallest ASCII serial frame supported by this repository. |
| `C1` | Legacy ASCII serial frame with its own command naming and routing rules. |
| `E1` | Legacy frame family used by chapter-18 style command layouts. |

#### `CodeMode`

Request/response payload encoding.

| Value | Description |
| --- | --- |
| `Ascii` | Text-encoded command data and response data. |
| `Binary` | Compact binary command data and response data. |

#### `AsciiFormat`

ASCII formatting variant for C4 / C3 / C2 serial frames.

| Value | Description |
| --- | --- |
| `Format1` | ENQ/STX/ETX layout without CR/LF. |
| `Format2` | Format1 plus a 1-byte block number used for request/response pairing on 2C/3C/4C. |
| `Format3` | STX-only layout commonly used on serial MC links. |
| `Format4` | CR/LF terminated layout often used by host-facing bring-up tools. |

#### `PlcSeries`

PLC family selection used for subcommand and device-layout differences.

| Value | Description |
| --- | --- |
| `IQ_R` |  |
| `IQ_L` |  |
| `Q_L` |  |
| `QnA` |  |
| `AnA_AnU = QnA` | AnA/AnUCPU common-command family. Kept value-compatible with the legacy QnA selector. |
| `A` |  |
| `IQ_F` |  |
| `Unspecified = 0xFFU` |  |

#### `PlcProfile`

Public PLC profile selector.

Use PlcProfile as the public configuration surface. The lower-level PlcSeries enum is kept as an internal command-layout family derived from this profile.

| Value | Description |
| --- | --- |
| `Unspecified = 0` |  |
| `MelsecIqR = 1` |  |
| `MelsecIqL = 2` |  |
| `MelsecQnA = 4` |  |
| `MelsecAnAAnU = 5` |  |
| `MelsecA = 6` |  |
| `MelsecIqF = 7` |  |
| `MelsecQ = 8` |  |
| `MelsecL = 9` |  |

#### `RouteKind`

Route layout inside the request header.

| Value | Description |
| --- | --- |
| `HostStation` | Host-station route with fixed station=0, network=0, pc=FF, and local module fields. |
| `MultidropStation` | Multidrop/routed route. 1C/2C use the station fields; 3C/4C also carry network/PC fields. |

#### `DeviceCode`

Device-family identifier used by the request codecs.

| Value | Description |
| --- | --- |
| `X` |  |
| `Y` |  |
| `M` |  |
| `L` |  |
| `SM` |  |
| `F` |  |
| `V` |  |
| `B` |  |
| `D` |  |
| `SD` |  |
| `W` |  |
| `TS` |  |
| `TC` |  |
| `TN` |  |
| `STS` |  |
| `STC` |  |
| `STN` |  |
| `CS` |  |
| `CC` |  |
| `CN` |  |
| `SB` |  |
| `SW` |  |
| `S` |  |
| `DX` |  |
| `DY` |  |
| `LTS` |  |
| `LTC` |  |
| `LTN` |  |
| `LSTS` |  |
| `LSTC` |  |
| `LSTN` |  |
| `LCS` |  |
| `LCC` |  |
| `LCN` |  |
| `LZ` |  |
| `Z` |  |
| `R` |  |
| `RD` |  |
| `ZR` |  |
| `G` |  |
| `HG` |  |

#### `BitValue`

Logical single-bit value used by bit read/write APIs.

| Value | Description |
| --- | --- |
| `Off = 0` |  |
| `On = 1` |  |

#### `RemoteOperationMode`

Conflict-handling mode for remote RUN / PAUSE.

| Value | Description |
| --- | --- |
| `DoNotExecuteForcibly = 0x0001` |  |
| `ExecuteForcibly = 0x0003` |  |

#### `RemoteRunClearMode`

Clear scope applied during remote RUN initialization.

| Value | Description |
| --- | --- |
| `DoNotClear = 0x00` |  |
| `ClearOutsideLatchRange = 0x01` |  |
| `AllClear = 0x02` |  |

#### `GlobalSignalTarget`

C24 global-signal selector used by command 1618.

| Value | Description |
| --- | --- |
| `ReceivedSide = 0x00` |  |
| `X1A = 0x01` |  |
| `X1B = 0x02` |  |

#### `SerialModuleChannel`

Target interface selector used by C24 mode switching (1612).

| Value | Description |
| --- | --- |
| `Ch1 = 0x01` |  |
| `Ch2 = 0x02` |  |

#### `SerialModuleModeNo`

Operation mode number used by C24 mode switching (1612).

| Value | Description |
| --- | --- |
| `McProtocolFormat1 = 0x01` |  |
| `McProtocolFormat2 = 0x02` |  |
| `McProtocolFormat3 = 0x03` |  |
| `McProtocolFormat4 = 0x04` |  |
| `McProtocolFormat5 = 0x05` |  |
| `Nonprocedural = 0x06` |  |
| `Bidirectional = 0x07` |  |
| `Predefined = 0x09` |  |
| `ModbusRtu = 0x0A` |  |
| `ModbusAscii = 0x0B` |  |
| `MelsoftConnection = 0xFF` |  |

#### `SerialModuleCommunicationSpeed`

Communication speed selector used by C24 mode switching (1612).

| Value | Description |
| --- | --- |
| `Bps300 = 0x00` |  |
| `Bps600 = 0x01` |  |
| `Bps1200 = 0x02` |  |
| `Bps2400 = 0x03` |  |
| `Bps4800 = 0x04` |  |
| `Bps9600 = 0x05` |  |
| `Bps14400 = 0x06` |  |
| `Bps19200 = 0x07` |  |
| `Bps28800 = 0x08` |  |
| `Bps38400 = 0x09` |  |
| `Bps57600 = 0x0A` |  |
| `Bps115200 = 0x0B` |  |
| `Bps230400 = 0x0C` |  |
| `Bps50 = 0x0F` |  |

#### `ResponseType`

Decoded PLC response class before command-specific parsing.

| Value | Description |
| --- | --- |
| `SuccessData` | Successful response carrying payload bytes. |
| `SuccessNoData` | Successful response carrying no payload bytes. |
| `PlcError` | PLC-side error response with an end code / error code. |

#### Variables And Constants

#### `kMaxRequestFrameBytes`

```cpp
std::size_t mcprotocol::serial::kMaxRequestFrameBytes = static_cast<std::size_t>(MCPROTOCOL_SERIAL_MAX_REQUEST_FRAME_BYTES)
```

#### `kMaxResponseFrameBytes`

```cpp
std::size_t mcprotocol::serial::kMaxResponseFrameBytes = static_cast<std::size_t>(MCPROTOCOL_SERIAL_MAX_RESPONSE_FRAME_BYTES)
```

#### `kMaxRequestDataBytes`

```cpp
std::size_t mcprotocol::serial::kMaxRequestDataBytes = static_cast<std::size_t>(MCPROTOCOL_SERIAL_MAX_REQUEST_DATA_BYTES)
```

#### `kMaxBatchWordPoints`

```cpp
std::size_t mcprotocol::serial::kMaxBatchWordPoints = static_cast<std::size_t>(MCPROTOCOL_SERIAL_MAX_BATCH_WORD_POINTS)
```

#### `kMaxBatchBitPointsAscii`

```cpp
std::size_t mcprotocol::serial::kMaxBatchBitPointsAscii = static_cast<std::size_t>(MCPROTOCOL_SERIAL_MAX_BATCH_BIT_POINTS_ASCII)
```

#### `kMaxBatchBitPointsBinary`

```cpp
std::size_t mcprotocol::serial::kMaxBatchBitPointsBinary = static_cast<std::size_t>(MCPROTOCOL_SERIAL_MAX_BATCH_BIT_POINTS_BINARY)
```

#### `kMaxRandomAccessItems`

```cpp
std::size_t mcprotocol::serial::kMaxRandomAccessItems = static_cast<std::size_t>(MCPROTOCOL_SERIAL_MAX_RANDOM_ACCESS_ITEMS)
```

#### `kMaxMultiBlockCount`

```cpp
std::size_t mcprotocol::serial::kMaxMultiBlockCount = static_cast<std::size_t>(MCPROTOCOL_SERIAL_MAX_MULTI_BLOCK_COUNT)
```

#### `kMaxMonitorItems`

```cpp
std::size_t mcprotocol::serial::kMaxMonitorItems = static_cast<std::size_t>(MCPROTOCOL_SERIAL_MAX_MONITOR_ITEMS)
```

#### `kMaxLoopbackBytes`

```cpp
std::size_t mcprotocol::serial::kMaxLoopbackBytes = static_cast<std::size_t>(MCPROTOCOL_SERIAL_MAX_LOOPBACK_BYTES)
```

#### `kMaxUserFrameRegistrationBytes`

```cpp
std::size_t mcprotocol::serial::kMaxUserFrameRegistrationBytes = 80U
```

#### `kCpuModelNameLength`

```cpp
std::size_t mcprotocol::serial::kCpuModelNameLength = 16
```

#### Functions

#### `qualified_buffer_kind_name`

```cpp
const char * mcprotocol::serial::qualified_buffer_kind_name(QualifiedBufferDeviceKind kind) noexcept
```

Returns "G" or "HG" for the helper device kind.

#### `qualified_buffer_word_to_byte_address`

```cpp
std::uint32_t mcprotocol::serial::qualified_buffer_word_to_byte_address(std::uint32_t word_address) noexcept
```

Converts a qualified word address to the corresponding module-buffer byte address.

#### `ok_status`

```cpp
Status mcprotocol::serial::ok_status() noexcept
```

Returns the default success status.

#### `make_status`

```cpp
Status mcprotocol::serial::make_status(StatusCode code, const char *message, std::uint16_t plc_error_code=0) noexcept
```

Builds a status value with an optional PLC end code.

#### `validate_qualified_buffer_helper_route`

```cpp
Status mcprotocol::serial::validate_qualified_buffer_helper_route(PlcProfile profile, const QualifiedBufferWordDevice &device) noexcept
```

Validates whether the helper 0601/1601 route may be used for a profile.

This helper route maps Un\\G-style text onto module-buffer commands. Some profiles, such as MELSEC-Q, MELSEC-L, iQ-L, and iQ-F, require the native device-access route instead.

#### `sparse_native_requested_bit_value`

```cpp
BitValue mcprotocol::serial::sparse_native_requested_bit_value(std::uint32_t raw_value) noexcept
```

Returns the requested-point value from a sparse native bit result word.

On 2C/3C/4C, native sparse bit reads (0403) and monitor reads (0802) return the addressed point inside a 16-point mask word. The requested head device is represented by bit 0 of that returned word.

#### `sparse_native_mask_word`

```cpp
std::uint16_t mcprotocol::serial::sparse_native_mask_word(std::uint32_t raw_value) noexcept
```

Returns the raw 16-point mask word from a sparse native bit result.

Keep this raw word visible for diagnostics when the target-specific offset pattern matters.

#### `parse_qualified_buffer_word_device`

```cpp
Status mcprotocol::serial::parse_qualified_buffer_word_device(std::string_view text, QualifiedBufferWordDevice &out_device) noexcept
```

Parses a helper qualified device string such as U3E0\\G10 or U3E0\\HG20.

#### `parse_link_direct_device`

```cpp
Status mcprotocol::serial::parse_link_direct_device(std::string_view text, LinkDirectDevice &out_device) noexcept
```

Parses a Jn\\... link-direct device string such as J1\\W100 or J1\\X10.

#### `make_qualified_buffer_read_words_request`

```cpp
Status mcprotocol::serial::make_qualified_buffer_read_words_request(const QualifiedBufferWordDevice &device, std::uint16_t word_length, ModuleBufferReadRequest &out_request) noexcept
```

Builds a module-buffer read request for a helper qualified word range.

#### `encode_qualified_buffer_word_values`

```cpp
Status mcprotocol::serial::encode_qualified_buffer_word_values(std::span< const std::uint16_t > words, std::span< std::byte > out_bytes, std::size_t &out_size) noexcept
```

Encodes helper qualified word values into little-endian module-buffer bytes.

#### `make_qualified_buffer_write_words_request`

```cpp
Status mcprotocol::serial::make_qualified_buffer_write_words_request(const QualifiedBufferWordDevice &device, std::span< const std::uint16_t > words, std::span< std::byte > byte_storage, ModuleBufferWriteRequest &out_request, std::size_t &out_byte_count) noexcept
```

Builds a module-buffer write request for helper qualified word access.

#### `plc_profile_name`

```cpp
const char * mcprotocol::serial::plc_profile_name(PlcProfile profile) noexcept
```

Returns the canonical saved string for a PLC profile.

#### `decode_qualified_buffer_word_values`

```cpp
Status mcprotocol::serial::decode_qualified_buffer_word_values(std::span< const std::byte > bytes, std::span< std::uint16_t > out_words) noexcept
```

Decodes little-endian module-buffer bytes into helper qualified word values.

#### `plc_profile_display_name`

```cpp
const char * mcprotocol::serial::plc_profile_display_name(PlcProfile profile) noexcept
```

Returns the human-readable display name for a PLC profile.

Use this for UI labels. Store and parse the canonical value from plc_profile_name(), not this display text.

#### `plc_profile_text_equals`

```cpp
bool mcprotocol::serial::plc_profile_text_equals(const char *text, std::size_t text_size, const char *expected, std::size_t expected_size) noexcept
```

Compares a bounded text buffer with a canonical PLC profile string.

#### `parse_plc_profile`

```cpp
bool mcprotocol::serial::parse_plc_profile(const char *text, std::size_t text_size, PlcProfile &out_profile) noexcept
```

Parses canonical PLC profile strings.

Short labels such as iqr, iq-r, ql, or qna are intentionally rejected so saved configuration, CLI arguments, and documentation use one stable cross-library spelling.

#### `plc_series_from_profile`

```cpp
PlcSeries mcprotocol::serial::plc_series_from_profile(PlcProfile profile) noexcept
```

Derives the internal device-layout / command-family selector from a public profile.

#### `is_plc_profile_specified`

```cpp
bool mcprotocol::serial::is_plc_profile_specified(PlcProfile profile) noexcept
```

### Namespace `mcprotocol::serial::module_io`

Named request-destination module I/O numbers used by 3C / 4C serial routing.

RouteConfig::request_destination_module_io_no defaults to OwnStation. The CPU constants are useful when a 3C / 4C request is intentionally routed to a multi-CPU or redundant-CPU target. The serial request header accepts the documented request-destination module I/O number field; common CPU values are 0x03D0..0x03D3, 0x03E0..0x03E3, and own station 0x03FF. Remote-head names are provided as vocabulary aliases for parity with the other plc-comm implementations; do not assume a remote-head route is valid on serial hardware unless the selected module, PLC family, and configuration define it.

#### Variables And Constants

#### `ControlSystemCpu`

```cpp
std::uint16_t mcprotocol::serial::module_io::ControlSystemCpu = 0x03D0
```

Control system CPU in a redundant CPU system.

#### `StandbySystemCpu`

```cpp
std::uint16_t mcprotocol::serial::module_io::StandbySystemCpu = 0x03D1
```

Standby system CPU in a redundant CPU system.

#### `SystemACpu`

```cpp
std::uint16_t mcprotocol::serial::module_io::SystemACpu = 0x03D2
```

System A CPU in a redundant CPU system.

#### `SystemBCpu`

```cpp
std::uint16_t mcprotocol::serial::module_io::SystemBCpu = 0x03D3
```

System B CPU in a redundant CPU system.

#### `MultipleCpu1`

```cpp
std::uint16_t mcprotocol::serial::module_io::MultipleCpu1 = 0x03E0
```

CPU No. 1 in a multi-CPU system.

#### `MultipleCpu2`

```cpp
std::uint16_t mcprotocol::serial::module_io::MultipleCpu2 = 0x03E1
```

CPU No. 2 in a multi-CPU system.

#### `MultipleCpu3`

```cpp
std::uint16_t mcprotocol::serial::module_io::MultipleCpu3 = 0x03E2
```

CPU No. 3 in a multi-CPU system.

#### `MultipleCpu4`

```cpp
std::uint16_t mcprotocol::serial::module_io::MultipleCpu4 = 0x03E3
```

CPU No. 4 in a multi-CPU system.

#### `RemoteHead1`

```cpp
std::uint16_t mcprotocol::serial::module_io::RemoteHead1 = MultipleCpu1
```

Remote head No. 1 route name.

#### `RemoteHead2`

```cpp
std::uint16_t mcprotocol::serial::module_io::RemoteHead2 = MultipleCpu2
```

Remote head No. 2 route name.

#### `ControlSystemRemoteHead`

```cpp
std::uint16_t mcprotocol::serial::module_io::ControlSystemRemoteHead = ControlSystemCpu
```

Control system remote-head route name.

#### `StandbySystemRemoteHead`

```cpp
std::uint16_t mcprotocol::serial::module_io::StandbySystemRemoteHead = StandbySystemCpu
```

Standby system remote-head route name.

#### `OwnStation`

```cpp
std::uint16_t mcprotocol::serial::module_io::OwnStation = 0x03FF
```

Connected own-station route.

## Classes

### Class `mcprotocol::serial::MelsecSerialClient`

Asynchronous MC protocol client for UART / serial integrations.

The intended MCU-side workflow is: call configure() start an async_* request transmit pending_tx_frame() with the board UART layer call notify_tx_complete() when TX finishes feed received bytes with on_rx_bytes() call poll() from the main loop or scheduler for timeout handling

Output spans passed to async_* requests must remain valid until the completion callback fires or until the request is cancelled.

#### Member Functions

#### `MelsecSerialClient`

```cpp
mcprotocol::serial::MelsecSerialClient::MelsecSerialClient()=default
```

#### `configure`

```cpp
Status mcprotocol::serial::MelsecSerialClient::configure(const ProtocolConfig &config) noexcept
```

Stores protocol settings and validates the static configuration.

#### `set_rs485_hooks`

```cpp
void mcprotocol::serial::MelsecSerialClient::set_rs485_hooks(const Rs485Hooks &hooks) noexcept
```

Installs optional RS-485 TX begin/end hooks used by the async workflow.

#### `busy`

```cpp
bool mcprotocol::serial::MelsecSerialClient::busy() const noexcept
```

Returns whether a request is currently in flight.

#### `pending_tx_frame`

```cpp
std::span< const std::byte > mcprotocol::serial::MelsecSerialClient::pending_tx_frame() const noexcept
```

Returns the encoded frame that should be sent to the UART layer.

#### `notify_tx_complete`

```cpp
Status mcprotocol::serial::MelsecSerialClient::notify_tx_complete(std::uint32_t now_ms, Status transport_status=ok_status()) noexcept
```

Advances the state machine after the transport finished sending the pending frame.

#### `on_rx_bytes`

```cpp
void mcprotocol::serial::MelsecSerialClient::on_rx_bytes(std::uint32_t now_ms, std::span< const std::byte > bytes) noexcept
```

Feeds received bytes into the response decoder.

#### `poll`

```cpp
void mcprotocol::serial::MelsecSerialClient::poll(std::uint32_t now_ms) noexcept
```

Checks timeouts for the current in-flight request.

#### `cancel`

```cpp
void mcprotocol::serial::MelsecSerialClient::cancel() noexcept
```

Cancels the in-flight request and clears transient state.

#### `async_batch_read_words`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_batch_read_words(std::uint32_t now_ms, const BatchReadWordsRequest &request, std::span< std::uint16_t > out_words, CompletionHandler callback, void *user) noexcept
```

Starts contiguous word read (0401).

#### `async_read_extended_file_register_words`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_read_extended_file_register_words(std::uint32_t now_ms, const ExtendedFileRegisterBatchReadWordsRequest &request, std::span< std::uint16_t > out_words, CompletionHandler callback, void *user) noexcept
```

Starts extended file-register word read.

#### `async_direct_read_extended_file_register_words`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_direct_read_extended_file_register_words(std::uint32_t now_ms, const ExtendedFileRegisterDirectBatchReadWordsRequest &request, std::span< std::uint16_t > out_words, CompletionHandler callback, void *user) noexcept
```

Starts direct extended file-register word read.

#### `async_link_direct_batch_read_words`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_link_direct_batch_read_words(std::uint32_t now_ms, const LinkDirectDevice &device, std::uint16_t points, std::span< std::uint16_t > out_words, CompletionHandler callback, void *user) noexcept
```

Starts Jn\\... link-direct word read over device extension specification.

#### `async_batch_read_bits`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_batch_read_bits(std::uint32_t now_ms, const BatchReadBitsRequest &request, std::span< BitValue > out_bits, CompletionHandler callback, void *user) noexcept
```

Starts contiguous bit read (0401 bit path).

#### `async_link_direct_batch_read_bits`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_link_direct_batch_read_bits(std::uint32_t now_ms, const LinkDirectDevice &device, std::uint16_t points, std::span< BitValue > out_bits, CompletionHandler callback, void *user) noexcept
```

Starts Jn\\... link-direct bit read over device extension specification.

#### `async_batch_write_words`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_batch_write_words(std::uint32_t now_ms, const BatchWriteWordsRequest &request, CompletionHandler callback, void *user) noexcept
```

Starts contiguous word write (1401).

#### `async_write_extended_file_register_words`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_write_extended_file_register_words(std::uint32_t now_ms, const ExtendedFileRegisterBatchWriteWordsRequest &request, CompletionHandler callback, void *user) noexcept
```

Starts extended file-register word write.

#### `async_direct_write_extended_file_register_words`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_direct_write_extended_file_register_words(std::uint32_t now_ms, const ExtendedFileRegisterDirectBatchWriteWordsRequest &request, CompletionHandler callback, void *user) noexcept
```

Starts direct extended file-register word write.

#### `async_link_direct_batch_write_words`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_link_direct_batch_write_words(std::uint32_t now_ms, const LinkDirectDevice &device, std::span< const std::uint16_t > words, CompletionHandler callback, void *user) noexcept
```

Starts Jn\\... link-direct contiguous word write over device extension specification.

#### `async_batch_write_bits`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_batch_write_bits(std::uint32_t now_ms, const BatchWriteBitsRequest &request, CompletionHandler callback, void *user) noexcept
```

Starts contiguous bit write (1401 bit path).

#### `async_link_direct_batch_write_bits`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_link_direct_batch_write_bits(std::uint32_t now_ms, const LinkDirectDevice &device, std::span< const BitValue > bits, CompletionHandler callback, void *user) noexcept
```

Starts Jn\\... link-direct contiguous bit write over device extension specification.

#### `async_extended_batch_read_words`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_extended_batch_read_words(std::uint32_t now_ms, const QualifiedBufferWordDevice &device, std::uint16_t points, std::span< std::uint16_t > out_words, CompletionHandler callback, void *user) noexcept
```

Starts helper qualified word read over module-buffer access.

#### `async_extended_batch_write_words`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_extended_batch_write_words(std::uint32_t now_ms, const QualifiedBufferWordDevice &device, std::span< const std::uint16_t > words, CompletionHandler callback, void *user) noexcept
```

Starts helper qualified word write over module-buffer access.

#### `async_random_read`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_random_read(std::uint32_t now_ms, const RandomReadRequest &request, std::span< std::uint32_t > out_values, CompletionHandler callback, void *user) noexcept
```

Starts native random read (0403).

#### `async_link_direct_random_read`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_link_direct_random_read(std::uint32_t now_ms, std::span< const LinkDirectRandomReadItem > items, std::span< std::uint32_t > out_values, CompletionHandler callback, void *user) noexcept
```

Starts native Jn\\... random read (0403 + device extension specification).

#### `async_random_write_words`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_random_write_words(std::uint32_t now_ms, std::span< const RandomWriteWordItem > items, CompletionHandler callback, void *user) noexcept
```

Starts native random word/dword write (1402 word path).

#### `async_random_write_extended_file_register_words`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_random_write_extended_file_register_words(std::uint32_t now_ms, std::span< const ExtendedFileRegisterRandomWriteWordItem > items, CompletionHandler callback, void *user) noexcept
```

Starts extended file-register random word write.

#### `async_link_direct_random_write_words`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_link_direct_random_write_words(std::uint32_t now_ms, std::span< const LinkDirectRandomWriteWordItem > items, CompletionHandler callback, void *user) noexcept
```

Starts native Jn\\... random word write (1402 + device extension specification).

#### `async_random_write_bits`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_random_write_bits(std::uint32_t now_ms, std::span< const RandomWriteBitItem > items, CompletionHandler callback, void *user) noexcept
```

Starts native random bit write (1402 bit path).

#### `async_link_direct_random_write_bits`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_link_direct_random_write_bits(std::uint32_t now_ms, std::span< const LinkDirectRandomWriteBitItem > items, CompletionHandler callback, void *user) noexcept
```

Starts native Jn\\... random bit write (1402 + device extension specification).

#### `async_multi_block_read`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_multi_block_read(std::uint32_t now_ms, const MultiBlockReadRequest &request, std::span< std::uint16_t > out_words, std::span< BitValue > out_bits, std::span< MultiBlockReadBlockResult > out_results, CompletionHandler callback, void *user) noexcept
```

Starts native multi-block read (0406).

#### `async_link_direct_multi_block_read`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_link_direct_multi_block_read(std::uint32_t now_ms, const LinkDirectMultiBlockReadRequest &request, std::span< std::uint16_t > out_words, std::span< BitValue > out_bits, std::span< MultiBlockReadBlockResult > out_results, CompletionHandler callback, void *user) noexcept
```

Starts native Jn\\... multi-block read (0406 + device extension specification).

The returned out_results preserve block order, point counts, and offsets. Their head_device field contains the inner device code/address, while the network number stays in the original request blocks.

#### `async_multi_block_write`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_multi_block_write(std::uint32_t now_ms, const MultiBlockWriteRequest &request, CompletionHandler callback, void *user) noexcept
```

Starts native multi-block write (1406).

#### `async_link_direct_multi_block_write`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_link_direct_multi_block_write(std::uint32_t now_ms, const LinkDirectMultiBlockWriteRequest &request, CompletionHandler callback, void *user) noexcept
```

Starts native Jn\\... multi-block write (1406 + device extension specification).

#### `async_register_monitor`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_register_monitor(std::uint32_t now_ms, const MonitorRegistration &request, CompletionHandler callback, void *user) noexcept
```

Starts monitor registration (0801).

#### `async_register_extended_file_register_monitor`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_register_extended_file_register_monitor(std::uint32_t now_ms, const ExtendedFileRegisterMonitorRegistration &request, CompletionHandler callback, void *user) noexcept
```

Starts extended file-register monitor registration.

#### `async_link_direct_register_monitor`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_link_direct_register_monitor(std::uint32_t now_ms, const LinkDirectMonitorRegistration &request, CompletionHandler callback, void *user) noexcept
```

Starts native Jn\\... monitor registration (0801 + device extension specification).

#### `async_read_monitor`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_read_monitor(std::uint32_t now_ms, std::span< std::uint32_t > out_values, CompletionHandler callback, void *user) noexcept
```

Starts monitor read (0802) using the most recent registration.

#### `async_read_extended_file_register_monitor`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_read_extended_file_register_monitor(std::uint32_t now_ms, std::span< std::uint16_t > out_words, CompletionHandler callback, void *user) noexcept
```

Starts extended file-register monitor read.

#### `async_read_host_buffer`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_read_host_buffer(std::uint32_t now_ms, const HostBufferReadRequest &request, std::span< std::uint16_t > out_words, CompletionHandler callback, void *user) noexcept
```

Starts host-buffer read (0613).

#### `async_write_host_buffer`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_write_host_buffer(std::uint32_t now_ms, const HostBufferWriteRequest &request, CompletionHandler callback, void *user) noexcept
```

Starts host-buffer write (1613).

#### `async_read_module_buffer`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_read_module_buffer(std::uint32_t now_ms, const ModuleBufferReadRequest &request, std::span< std::byte > out_bytes, CompletionHandler callback, void *user) noexcept
```

Starts module-buffer byte read (0601).

#### `async_write_module_buffer`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_write_module_buffer(std::uint32_t now_ms, const ModuleBufferWriteRequest &request, CompletionHandler callback, void *user) noexcept
```

Starts module-buffer byte write (1601).

#### `async_read_cpu_model`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_read_cpu_model(std::uint32_t now_ms, CpuModelInfo &out_info, CompletionHandler callback, void *user) noexcept
```

Starts CPU-model read.

#### `async_remote_run`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_remote_run(std::uint32_t now_ms, RemoteOperationMode mode, RemoteRunClearMode clear_mode, CompletionHandler callback, void *user) noexcept
```

Starts remote RUN (1001).

#### `async_remote_stop`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_remote_stop(std::uint32_t now_ms, CompletionHandler callback, void *user) noexcept
```

Starts remote STOP (1002).

#### `async_remote_pause`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_remote_pause(std::uint32_t now_ms, RemoteOperationMode mode, CompletionHandler callback, void *user) noexcept
```

Starts remote PAUSE (1003).

#### `async_remote_latch_clear`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_remote_latch_clear(std::uint32_t now_ms, CompletionHandler callback, void *user) noexcept
```

Starts remote latch clear (1005).

#### `async_unlock_remote_password`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_unlock_remote_password(std::uint32_t now_ms, std::string_view remote_password, CompletionHandler callback, void *user) noexcept
```

Unlocks remote-password-protected access (1630).

#### `async_lock_remote_password`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_lock_remote_password(std::uint32_t now_ms, std::string_view remote_password, CompletionHandler callback, void *user) noexcept
```

Locks remote-password-protected access (1631).

#### `async_clear_error_information`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_clear_error_information(std::uint32_t now_ms, CompletionHandler callback, void *user) noexcept
```

Starts clear error information (1617) for serial/C24 targets.

#### `async_remote_reset`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_remote_reset(std::uint32_t now_ms, CompletionHandler callback, void *user) noexcept
```

Starts remote RESET (1006).

The manual notes that some targets may reset before returning a response. In that case this client treats a pure response-timeout with no received bytes as success for this operation.

#### `async_read_user_frame`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_read_user_frame(std::uint32_t now_ms, const UserFrameReadRequest &request, UserFrameRegistrationData &out_data, CompletionHandler callback, void *user) noexcept
```

Starts user-frame registration-data read (0610).

#### `async_write_user_frame`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_write_user_frame(std::uint32_t now_ms, const UserFrameWriteRequest &request, CompletionHandler callback, void *user) noexcept
```

Starts user-frame registration-data write (1610, subcommand 0000).

#### `async_delete_user_frame`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_delete_user_frame(std::uint32_t now_ms, const UserFrameDeleteRequest &request, CompletionHandler callback, void *user) noexcept
```

Starts user-frame registration-data delete (1610, subcommand 0001).

#### `async_control_global_signal`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_control_global_signal(std::uint32_t now_ms, const GlobalSignalControlRequest &request, CompletionHandler callback, void *user) noexcept
```

Starts C24 global-signal ON/OFF control (1618).

#### `async_switch_serial_module_mode`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_switch_serial_module_mode(std::uint32_t now_ms, const SerialModuleModeSwitchRequest &request, CompletionHandler callback, void *user) noexcept
```

Starts C24 mode switching (1612).

#### `async_initialize_c24_transmission_sequence`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_initialize_c24_transmission_sequence(std::uint32_t now_ms, CompletionHandler callback, void *user) noexcept
```

Starts C24 transmission-sequence initialization (1615).

#### `async_loopback`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_loopback(std::uint32_t now_ms, std::span< const char > hex_ascii, std::span< char > out_echoed, CompletionHandler callback, void *user) noexcept
```

Starts loopback using hexadecimal ASCII payload bytes.

### Class `mcprotocol::serial::FrameCodec`

Frame-level encode/decode helper for complete serial MC frames.

Use this class when you already have a command payload and only need the outer serial frame layer, or when you need to decode a stream of returned bytes before passing the payload to a command-specific parser.

#### Member Functions

#### `validate_config`

```cpp
static Status mcprotocol::serial::FrameCodec::validate_config(const ProtocolConfig &config) noexcept
```

Validates a static protocol configuration before encoding requests.

This checks frame-family / code-mode compatibility, route constraints, and combinations that are compiled out by feature macros.

#### `encode_request`

```cpp
static Status mcprotocol::serial::FrameCodec::encode_request(const ProtocolConfig &config, std::span< const std::uint8_t > request_data, std::span< std::uint8_t > out_frame, std::size_t &out_size) noexcept
```

Wraps command data in the configured serial frame format.

request_data must already contain the command payload generated by CommandCodec.

#### `encode_success_response`

```cpp
static Status mcprotocol::serial::FrameCodec::encode_success_response(const ProtocolConfig &config, std::span< const std::uint8_t > response_data, std::span< std::uint8_t > out_frame, std::size_t &out_size) noexcept
```

Builds a success response frame for tests and local tools.

This is mainly used by tests and local validation helpers. Library users typically decode real target responses instead of constructing synthetic ones.

#### `encode_error_response`

```cpp
static Status mcprotocol::serial::FrameCodec::encode_error_response(const ProtocolConfig &config, std::uint16_t error_code, std::span< std::uint8_t > out_frame, std::size_t &out_size) noexcept
```

Builds a PLC-error response frame for tests and local tools.

#### `decode_response`

```cpp
static DecodeResult mcprotocol::serial::FrameCodec::decode_response(const ProtocolConfig &config, std::span< const std::uint8_t > bytes) noexcept
```

Decodes one response frame from the front of bytes.

The caller can use bytes_consumed to drop the decoded prefix and continue stream processing.

### Class `mcprotocol::serial::PosixSyncClient`

Host-side synchronous convenience wrapper built on PosixSerialPort and MelsecSerialClient.

This class is intentionally small:

- it keeps the existing low-level client unchanged - it opens a host-side serial port - it runs one request synchronously from TX to completion - it exposes string-address helpers for common contiguous, sparse random, and monitor operations

Use it on Windows or POSIX hosts when you want a simpler bring-up path than manually driving pending_tx_frame(), notify_tx_complete(), on_rx_bytes(), and poll().

#### Member Functions

#### `PosixSyncClient`

```cpp
mcprotocol::serial::PosixSyncClient::PosixSyncClient()=default
```

#### `PosixSyncClient`

```cpp
mcprotocol::serial::PosixSyncClient::PosixSyncClient(const PosixSyncClient &)=delete
```

#### `operator=`

```cpp
PosixSyncClient & mcprotocol::serial::PosixSyncClient::operator=(const PosixSyncClient &)=delete
```

#### `open`

```cpp
Status mcprotocol::serial::PosixSyncClient::open(const PosixSerialConfig &serial_config, const ProtocolConfig &protocol_config) noexcept
```

Opens the serial port and configures the underlying MC protocol client.

#### `close`

```cpp
void mcprotocol::serial::PosixSyncClient::close() noexcept
```

Closes the serial port and clears any in-flight request state.

#### `is_open`

```cpp
bool mcprotocol::serial::PosixSyncClient::is_open() const noexcept
```

Returns whether the underlying serial port is open.

#### `protocol_config`

```cpp
const ProtocolConfig & mcprotocol::serial::PosixSyncClient::protocol_config() const noexcept
```

Returns the currently configured protocol settings.

#### `read_cpu_model`

```cpp
Status mcprotocol::serial::PosixSyncClient::read_cpu_model(CpuModelInfo &out_info) noexcept
```

Reads the remote CPU model synchronously.

#### `remote_run`

```cpp
Status mcprotocol::serial::PosixSyncClient::remote_run(RemoteOperationMode mode=RemoteOperationMode::DoNotExecuteForcibly, RemoteRunClearMode clear_mode=RemoteRunClearMode::DoNotClear) noexcept
```

Issues remote RUN (1001) synchronously.

#### `remote_stop`

```cpp
Status mcprotocol::serial::PosixSyncClient::remote_stop() noexcept
```

Issues remote STOP (1002) synchronously.

#### `remote_pause`

```cpp
Status mcprotocol::serial::PosixSyncClient::remote_pause(RemoteOperationMode mode=RemoteOperationMode::DoNotExecuteForcibly) noexcept
```

Issues remote PAUSE (1003) synchronously.

#### `remote_latch_clear`

```cpp
Status mcprotocol::serial::PosixSyncClient::remote_latch_clear() noexcept
```

Issues remote latch clear (1005) synchronously.

#### `unlock_remote_password`

```cpp
Status mcprotocol::serial::PosixSyncClient::unlock_remote_password(std::string_view remote_password) noexcept
```

Unlocks remote-password-protected access (1630) synchronously.

#### `lock_remote_password`

```cpp
Status mcprotocol::serial::PosixSyncClient::lock_remote_password(std::string_view remote_password) noexcept
```

Locks remote-password-protected access (1631) synchronously.

#### `clear_error_information`

```cpp
Status mcprotocol::serial::PosixSyncClient::clear_error_information() noexcept
```

Clears serial/C24 error information (1617) synchronously.

#### `remote_reset`

```cpp
Status mcprotocol::serial::PosixSyncClient::remote_reset() noexcept
```

Issues remote RESET (1006) synchronously.

Some targets reset before returning a response. In that case the underlying client treats a pure response-timeout with no received bytes as success for this operation.

#### `read_user_frame`

```cpp
Status mcprotocol::serial::PosixSyncClient::read_user_frame(const UserFrameReadRequest &request, UserFrameRegistrationData &out_data) noexcept
```

Reads user-frame registration data synchronously (0610).

#### `write_user_frame`

```cpp
Status mcprotocol::serial::PosixSyncClient::write_user_frame(const UserFrameWriteRequest &request) noexcept
```

Writes user-frame registration data synchronously (1610, subcommand 0000).

#### `delete_user_frame`

```cpp
Status mcprotocol::serial::PosixSyncClient::delete_user_frame(const UserFrameDeleteRequest &request) noexcept
```

Deletes user-frame registration data synchronously (1610, subcommand 0001).

#### `control_global_signal`

```cpp
Status mcprotocol::serial::PosixSyncClient::control_global_signal(const GlobalSignalControlRequest &request) noexcept
```

Controls C24 global signal ON/OFF synchronously (1618).

#### `switch_serial_module_mode`

```cpp
Status mcprotocol::serial::PosixSyncClient::switch_serial_module_mode(const SerialModuleModeSwitchRequest &request) noexcept
```

Switches C24 operation mode / transmission settings synchronously (1612).

#### `initialize_c24_transmission_sequence`

```cpp
Status mcprotocol::serial::PosixSyncClient::initialize_c24_transmission_sequence() noexcept
```

Initializes C24 format-5 transmission sequence synchronously (1615).

#### `read_words`

```cpp
Status mcprotocol::serial::PosixSyncClient::read_words(std::string_view head_device, std::uint16_t points, std::span< std::uint16_t > out_words) noexcept
```

Reads contiguous words synchronously from a string address such as D100.

#### `read_words`

```cpp
Status mcprotocol::serial::PosixSyncClient::read_words(std::string_view head_device, std::span< std::uint16_t > out_words) noexcept
```

Reads contiguous words synchronously using out_words.size() as the point count.

#### `read_extended_file_register_words`

```cpp
Status mcprotocol::serial::PosixSyncClient::read_extended_file_register_words(const ExtendedFileRegisterBatchReadWordsRequest &request, std::span< std::uint16_t > out_words) noexcept
```

Reads extended file-register words synchronously.

#### `direct_read_extended_file_register_words`

```cpp
Status mcprotocol::serial::PosixSyncClient::direct_read_extended_file_register_words(const ExtendedFileRegisterDirectBatchReadWordsRequest &request, std::span< std::uint16_t > out_words) noexcept
```

Reads direct extended file-register words synchronously.

#### `read_bits`

```cpp
Status mcprotocol::serial::PosixSyncClient::read_bits(std::string_view head_device, std::uint16_t points, std::span< BitValue > out_bits) noexcept
```

Reads contiguous bits synchronously from a string address such as M100.

#### `read_bits`

```cpp
Status mcprotocol::serial::PosixSyncClient::read_bits(std::string_view head_device, std::span< BitValue > out_bits) noexcept
```

Reads contiguous bits synchronously using out_bits.size() as the point count.

#### `read_link_direct_words`

```cpp
Status mcprotocol::serial::PosixSyncClient::read_link_direct_words(std::string_view head_device, std::uint16_t points, std::span< std::uint16_t > out_words) noexcept
```

Reads contiguous Jn\\... link-direct words synchronously.

#### `read_link_direct_bits`

```cpp
Status mcprotocol::serial::PosixSyncClient::read_link_direct_bits(std::string_view head_device, std::uint16_t points, std::span< BitValue > out_bits) noexcept
```

Reads contiguous Jn\\... link-direct bits synchronously.

#### `read_native_qualified_words`

```cpp
Status mcprotocol::serial::PosixSyncClient::read_native_qualified_words(std::string_view head_device, std::uint16_t points, std::span< std::uint16_t > out_words) noexcept
```

Reads native-qualified Un\\Gn or Un\\HGn words.

Use this for profiles whose qualified access route is native device access (0401). The 0601 helper route is profile/target-specific and may be rejected.

#### `read_long_state_bits`

```cpp
Status mcprotocol::serial::PosixSyncClient::read_long_state_bits(std::string_view head_device, std::uint16_t points, std::span< BitValue > out_bits) noexcept
```

Reads long timer/counter contact or coil states through the dedicated status-block path.

#### `read_long_state_bits`

```cpp
Status mcprotocol::serial::PosixSyncClient::read_long_state_bits(std::string_view head_device, std::span< BitValue > out_bits) noexcept
```

Reads long timer/counter states using out_bits.size() as the point count.

#### `write_words`

```cpp
Status mcprotocol::serial::PosixSyncClient::write_words(std::string_view head_device, std::span< const std::uint16_t > words) noexcept
```

Writes contiguous words synchronously to a string address such as D100.

#### `write_extended_file_register_words`

```cpp
Status mcprotocol::serial::PosixSyncClient::write_extended_file_register_words(const ExtendedFileRegisterBatchWriteWordsRequest &request) noexcept
```

Writes extended file-register words synchronously.

#### `direct_write_extended_file_register_words`

```cpp
Status mcprotocol::serial::PosixSyncClient::direct_write_extended_file_register_words(const ExtendedFileRegisterDirectBatchWriteWordsRequest &request) noexcept
```

Writes direct extended file-register words synchronously.

#### `write_bits`

```cpp
Status mcprotocol::serial::PosixSyncClient::write_bits(std::string_view head_device, std::span< const BitValue > bits) noexcept
```

Writes contiguous bits synchronously to a string address such as M100.

#### `write_link_direct_words`

```cpp
Status mcprotocol::serial::PosixSyncClient::write_link_direct_words(std::string_view head_device, std::span< const std::uint16_t > words) noexcept
```

Writes contiguous Jn\\... link-direct words synchronously.

#### `write_link_direct_bits`

```cpp
Status mcprotocol::serial::PosixSyncClient::write_link_direct_bits(std::string_view head_device, std::span< const BitValue > bits) noexcept
```

Writes contiguous Jn\\... link-direct bits synchronously.

#### `write_native_qualified_words`

```cpp
Status mcprotocol::serial::PosixSyncClient::write_native_qualified_words(std::string_view head_device, std::span< const std::uint16_t > words) noexcept
```

Writes native-qualified Un\\Gn or Un\\HGn words.

Use this for profiles whose qualified access route is native device access (1401). The 1601 helper route is profile/target-specific and may be rejected.

#### `random_read`

```cpp
Status mcprotocol::serial::PosixSyncClient::random_read(std::span< const highlevel::RandomReadSpec > items, std::span< std::uint32_t > out_values) noexcept
```

Reads sparse word/dword items synchronously from string-address specs.

#### `random_read`

```cpp
Status mcprotocol::serial::PosixSyncClient::random_read(std::string_view device, std::uint32_t &out_value, bool double_word=false) noexcept
```

Reads one sparse word/dword item synchronously from a string address.

#### `random_write_words`

```cpp
Status mcprotocol::serial::PosixSyncClient::random_write_words(std::span< const highlevel::RandomWriteWordSpec > items) noexcept
```

Writes sparse word/dword items synchronously from string-address specs.

#### `random_write_extended_file_register_words`

```cpp
Status mcprotocol::serial::PosixSyncClient::random_write_extended_file_register_words(std::span< const ExtendedFileRegisterRandomWriteWordItem > items) noexcept
```

Writes extended file-register words randomly.

#### `random_write_word`

```cpp
Status mcprotocol::serial::PosixSyncClient::random_write_word(std::string_view device, std::uint32_t value, bool double_word=false) noexcept
```

Writes one sparse word/dword item synchronously from a string address.

#### `random_write_bits`

```cpp
Status mcprotocol::serial::PosixSyncClient::random_write_bits(std::span< const highlevel::RandomWriteBitSpec > items) noexcept
```

Writes sparse bit items synchronously from string-address specs.

#### `random_write_bit`

```cpp
Status mcprotocol::serial::PosixSyncClient::random_write_bit(std::string_view device, BitValue value) noexcept
```

Writes one sparse bit item synchronously from a string address.

#### `register_monitor`

```cpp
Status mcprotocol::serial::PosixSyncClient::register_monitor(std::span< const highlevel::RandomReadSpec > items) noexcept
```

Registers a sparse monitor synchronously from string-address specs.

#### `register_monitor`

```cpp
Status mcprotocol::serial::PosixSyncClient::register_monitor(std::string_view device, bool double_word=false) noexcept
```

Registers one sparse monitor item synchronously from a string address.

#### `register_extended_file_register_monitor`

```cpp
Status mcprotocol::serial::PosixSyncClient::register_extended_file_register_monitor(const ExtendedFileRegisterMonitorRegistration &request) noexcept
```

Registers extended file-register monitor data synchronously.

#### `read_monitor`

```cpp
Status mcprotocol::serial::PosixSyncClient::read_monitor(std::span< std::uint32_t > out_values) noexcept
```

Reads the most recently registered monitor items synchronously.

#### `read_monitor`

```cpp
Status mcprotocol::serial::PosixSyncClient::read_monitor(std::uint32_t &out_value) noexcept
```

Reads one previously registered monitor item synchronously.

#### `read_extended_file_register_monitor`

```cpp
Status mcprotocol::serial::PosixSyncClient::read_extended_file_register_monitor(std::span< std::uint16_t > out_words) noexcept
```

Reads the most recently registered extended file-register monitor items synchronously.

### Class `mcprotocol::serial::PosixSerialPort`

Minimal blocking host-side serial-port wrapper used by the CLI tools.

This class is not required on MCU targets. It exists so the same request/response codec and client logic can be exercised from host-side validation tools.

#### Member Functions

#### `PosixSerialPort`

```cpp
mcprotocol::serial::PosixSerialPort::PosixSerialPort()=default
```

#### `~PosixSerialPort`

```cpp
mcprotocol::serial::PosixSerialPort::~PosixSerialPort()
```

#### `PosixSerialPort`

```cpp
mcprotocol::serial::PosixSerialPort::PosixSerialPort(const PosixSerialPort &)=delete
```

#### `operator=`

```cpp
PosixSerialPort & mcprotocol::serial::PosixSerialPort::operator=(const PosixSerialPort &)=delete
```

#### `open`

```cpp
Status mcprotocol::serial::PosixSerialPort::open(const PosixSerialConfig &config) noexcept
```

Opens and configures the serial port.

#### `close`

```cpp
void mcprotocol::serial::PosixSerialPort::close() noexcept
```

Closes the serial port if it is open.

#### `is_open`

```cpp
bool mcprotocol::serial::PosixSerialPort::is_open() const noexcept
```

Returns whether the serial port is currently open.

#### `native_handle`

```cpp
std::intptr_t mcprotocol::serial::PosixSerialPort::native_handle() const noexcept
```

Returns the native handle value, or -1 when closed.

#### `write_all`

```cpp
Status mcprotocol::serial::PosixSerialPort::write_all(std::span< const std::byte > bytes) noexcept
```

Writes the entire byte range before returning.

#### `read_some`

```cpp
Status mcprotocol::serial::PosixSerialPort::read_some(std::span< std::byte > buffer, int timeout_ms, std::size_t &out_size) noexcept
```

Reads up to buffer.size() bytes with a timeout.

#### `flush_rx`

```cpp
Status mcprotocol::serial::PosixSerialPort::flush_rx() noexcept
```

Drops unread RX data that is already buffered by the driver.

#### `drain_tx`

```cpp
Status mcprotocol::serial::PosixSerialPort::drain_tx() noexcept
```

Waits until queued TX data has physically drained.

#### `set_rts`

```cpp
Status mcprotocol::serial::PosixSerialPort::set_rts(bool enabled) noexcept
```

Sets the RTS line when the underlying driver supports it.

## Structs

### Struct `mcprotocol::serial::RawResponseFrame`

Raw decoded response frame before command-specific parsing.

#### Fields

#### `type`

```cpp
ResponseType mcprotocol::serial::RawResponseFrame::type = ResponseType::SuccessNoData
```

Success-with-data, success-without-data, or PLC-error classification.

#### `response_size`

```cpp
std::size_t mcprotocol::serial::RawResponseFrame::response_size = 0
```

Number of valid bytes in response_data.

#### `error_code`

```cpp
std::uint16_t mcprotocol::serial::RawResponseFrame::error_code = 0
```

PLC error code when type == ResponseType::PlcError.

#### `response_data`

```cpp
std::array<std::uint8_t, kMaxResponseFrameBytes> mcprotocol::serial::RawResponseFrame::response_data {}
```

Raw response payload bytes with the serial frame already removed.

### Struct `mcprotocol::serial::DecodeResult`

Result returned by FrameCodec::decode_response().

#### Fields

#### `status`

```cpp
DecodeStatus mcprotocol::serial::DecodeResult::status = DecodeStatus::Incomplete
```

Stream-level decode status.

#### `frame`

```cpp
RawResponseFrame mcprotocol::serial::DecodeResult::frame {}
```

Raw response frame when status == DecodeStatus::Complete.

#### `error`

```cpp
Status mcprotocol::serial::DecodeResult::error {}
```

Decoder-side error when status == DecodeStatus::Error.

#### `bytes_consumed`

```cpp
std::size_t mcprotocol::serial::DecodeResult::bytes_consumed = 0
```

Number of bytes consumed from the input span.

### Struct `mcprotocol::serial::highlevel::RandomReadSpec`

String-address spec used to build sparse random-read or monitor requests.

#### Fields

#### `device`

```cpp
std::string_view mcprotocol::serial::highlevel::RandomReadSpec::device {}
```

Plain device string such as D100, LZ0, or LCN10.

#### `double_word`

```cpp
bool mcprotocol::serial::highlevel::RandomReadSpec::double_word = false
```

true when the target should be encoded as a double-word sparse item.

### Struct `mcprotocol::serial::highlevel::RandomWriteWordSpec`

String-address spec used to build sparse random word-write items.

#### Fields

#### `device`

```cpp
std::string_view mcprotocol::serial::highlevel::RandomWriteWordSpec::device {}
```

Plain device string such as D100 or LZ0.

#### `value`

```cpp
std::uint32_t mcprotocol::serial::highlevel::RandomWriteWordSpec::value = 0
```

Word or double-word value written to device.

#### `double_word`

```cpp
bool mcprotocol::serial::highlevel::RandomWriteWordSpec::double_word = false
```

true when the item should be encoded as a double-word sparse write.

### Struct `mcprotocol::serial::highlevel::RandomWriteBitSpec`

String-address spec used to build sparse random bit-write items.

#### Fields

#### `device`

```cpp
std::string_view mcprotocol::serial::highlevel::RandomWriteBitSpec::device {}
```

Plain bit-device string such as M100 or X10.

#### `value`

```cpp
BitValue mcprotocol::serial::highlevel::RandomWriteBitSpec::value = BitValue::Off
```

Bit value written to device.

### Struct `mcprotocol::serial::highlevel::LongStateReadSpec`

Mapping from a long-family state device to the helper's internal read route.

#### Fields

#### `route`

```cpp
LongStateReadRoute mcprotocol::serial::highlevel::LongStateReadSpec::route = LongStateReadRoute::StatusBlock
```

Read route used internally by the long-state helper.

#### `base_code`

```cpp
DeviceCode mcprotocol::serial::highlevel::LongStateReadSpec::base_code = DeviceCode::LTN
```

Base current-value device read with 0401 word access, or direct bit device for DirectBits.

#### `kind`

```cpp
LongStateReadKind mcprotocol::serial::highlevel::LongStateReadSpec::kind = LongStateReadKind::Contact
```

Status bit selected from the third word of the block.

### Struct `mcprotocol::serial::PosixSyncClient::CompletionState`

#### Fields

#### `done`

```cpp
bool mcprotocol::serial::PosixSyncClient::CompletionState::done = false
```

#### `status`

```cpp
Status mcprotocol::serial::PosixSyncClient::CompletionState::status {}
```

### Struct `mcprotocol::serial::LinkDirectDevice`

Parsed Jn\\... link-direct device reference such as J1\\W100.

#### Fields

#### `network_number`

```cpp
std::uint16_t mcprotocol::serial::LinkDirectDevice::network_number = 0
```

#### `device`

```cpp
DeviceAddress mcprotocol::serial::LinkDirectDevice::device {}
```

### Struct `mcprotocol::serial::LinkDirectRandomReadItem`

One sparse Jn\\... item used by native random-read and monitor registration.

#### Fields

#### `device`

```cpp
LinkDirectDevice mcprotocol::serial::LinkDirectRandomReadItem::device {}
```

#### `double_word`

```cpp
bool mcprotocol::serial::LinkDirectRandomReadItem::double_word = false
```

### Struct `mcprotocol::serial::LinkDirectRandomWriteWordItem`

One sparse Jn\\... word item used by native random word-write.

#### Fields

#### `device`

```cpp
LinkDirectDevice mcprotocol::serial::LinkDirectRandomWriteWordItem::device {}
```

#### `value`

```cpp
std::uint32_t mcprotocol::serial::LinkDirectRandomWriteWordItem::value = 0
```

#### `double_word`

```cpp
bool mcprotocol::serial::LinkDirectRandomWriteWordItem::double_word = false
```

### Struct `mcprotocol::serial::LinkDirectRandomWriteBitItem`

One sparse Jn\\... bit item used by native random bit-write.

#### Fields

#### `device`

```cpp
LinkDirectDevice mcprotocol::serial::LinkDirectRandomWriteBitItem::device {}
```

#### `value`

```cpp
BitValue mcprotocol::serial::LinkDirectRandomWriteBitItem::value = BitValue::Off
```

### Struct `mcprotocol::serial::LinkDirectMultiBlockReadBlock`

One Jn\\... block used by native multi-block read.

#### Fields

#### `head_device`

```cpp
LinkDirectDevice mcprotocol::serial::LinkDirectMultiBlockReadBlock::head_device {}
```

#### `points`

```cpp
std::uint16_t mcprotocol::serial::LinkDirectMultiBlockReadBlock::points = 0
```

#### `bit_block`

```cpp
bool mcprotocol::serial::LinkDirectMultiBlockReadBlock::bit_block = false
```

### Struct `mcprotocol::serial::LinkDirectMultiBlockReadRequest`

Jn\\... native multi-block read request.

#### Fields

#### `blocks`

```cpp
std::span<const LinkDirectMultiBlockReadBlock> mcprotocol::serial::LinkDirectMultiBlockReadRequest::blocks {}
```

### Struct `mcprotocol::serial::LinkDirectMultiBlockWriteBlock`

One Jn\\... block used by native multi-block write.

#### Fields

#### `head_device`

```cpp
LinkDirectDevice mcprotocol::serial::LinkDirectMultiBlockWriteBlock::head_device {}
```

#### `points`

```cpp
std::uint16_t mcprotocol::serial::LinkDirectMultiBlockWriteBlock::points = 0
```

#### `bit_block`

```cpp
bool mcprotocol::serial::LinkDirectMultiBlockWriteBlock::bit_block = false
```

#### `words`

```cpp
std::span<const std::uint16_t> mcprotocol::serial::LinkDirectMultiBlockWriteBlock::words {}
```

#### `bits`

```cpp
std::span<const BitValue> mcprotocol::serial::LinkDirectMultiBlockWriteBlock::bits {}
```

### Struct `mcprotocol::serial::LinkDirectMultiBlockWriteRequest`

Jn\\... native multi-block write request.

#### Fields

#### `blocks`

```cpp
std::span<const LinkDirectMultiBlockWriteBlock> mcprotocol::serial::LinkDirectMultiBlockWriteRequest::blocks {}
```

### Struct `mcprotocol::serial::LinkDirectMonitorRegistration`

Jn\\... monitor registration payload (0801 + device extension specification).

#### Fields

#### `items`

```cpp
std::span<const LinkDirectRandomReadItem> mcprotocol::serial::LinkDirectMonitorRegistration::items {}
```

### Struct `mcprotocol::serial::PosixSerialConfig`

Host-side serial-port configuration used by PosixSerialPort.

device_path accepts /dev/... style paths on POSIX systems and COM3 or \\.\COM10 style names on Windows.

#### Fields

#### `device_path`

```cpp
std::string_view mcprotocol::serial::PosixSerialConfig::device_path {}
```

#### `baud_rate`

```cpp
std::uint32_t mcprotocol::serial::PosixSerialConfig::baud_rate = 9600
```

#### `data_bits`

```cpp
std::uint8_t mcprotocol::serial::PosixSerialConfig::data_bits = 8
```

#### `stop_bits`

```cpp
std::uint8_t mcprotocol::serial::PosixSerialConfig::stop_bits = 1
```

#### `parity`

```cpp
char mcprotocol::serial::PosixSerialConfig::parity = 'N'
```

#### `rts_cts`

```cpp
bool mcprotocol::serial::PosixSerialConfig::rts_cts = false
```

### Struct `mcprotocol::serial::QualifiedBufferWordDevice`

Parsed U...\\G... or U...\\HG... qualified word device.

#### Fields

#### `kind`

```cpp
QualifiedBufferDeviceKind mcprotocol::serial::QualifiedBufferWordDevice::kind = QualifiedBufferDeviceKind::G
```

#### `module_number`

```cpp
std::uint16_t mcprotocol::serial::QualifiedBufferWordDevice::module_number = 0
```

#### `word_address`

```cpp
std::uint32_t mcprotocol::serial::QualifiedBufferWordDevice::word_address = 0
```

### Struct `mcprotocol::serial::Status`

Result object returned by most public APIs.

plc_error_code is meaningful when code == StatusCode::PlcError.

#### Fields

#### `code`

```cpp
StatusCode mcprotocol::serial::Status::code = StatusCode::Ok
```

#### `plc_error_code`

```cpp
std::uint16_t mcprotocol::serial::Status::plc_error_code = 0
```

#### `message`

```cpp
const char* mcprotocol::serial::Status::message = "ok"
```

#### Member Functions

#### `ok`

```cpp
bool mcprotocol::serial::Status::ok() const noexcept
```

### Struct `mcprotocol::serial::TimeoutConfig`

Timeout settings used by the frame decoder and async client.

These values are transport-facing rather than command-facing:

- response_timeout_ms is the total request timeout once TX finishes - inter_byte_timeout_ms is the gap timeout while RX is already in progress

#### Fields

#### `response_timeout_ms`

```cpp
std::uint32_t mcprotocol::serial::TimeoutConfig::response_timeout_ms = 5000
```

Maximum wait after TX completion before the request is treated as timed out.

#### `inter_byte_timeout_ms`

```cpp
std::uint32_t mcprotocol::serial::TimeoutConfig::inter_byte_timeout_ms = 250
```

Maximum allowed idle gap between RX bytes once a response has started.

### Struct `mcprotocol::serial::RouteConfig`

Route header fields for serial MC requests.

The same struct is shared across 2C/3C/4C, 1C, and 1E, but not every field is active on every frame family. FrameCodec::validate_config() checks the combinations that are legal for the selected frame.

#### Fields

#### `kind`

```cpp
RouteKind mcprotocol::serial::RouteConfig::kind = RouteKind::HostStation
```

Route interpretation used by the selected frame family.

#### `station_no`

```cpp
std::uint8_t mcprotocol::serial::RouteConfig::station_no = 0x00
```

Target station number on multidrop serial links.

#### `network_no`

```cpp
std::uint8_t mcprotocol::serial::RouteConfig::network_no = 0x00
```

Network number used by routed 3C/4C requests.

#### `pc_no`

```cpp
std::uint8_t mcprotocol::serial::RouteConfig::pc_no = 0xFF
```

PLC number field used by 3C/4C and legacy frame families.

#### `request_destination_module_io_no`

```cpp
std::uint16_t mcprotocol::serial::RouteConfig::request_destination_module_io_no = module_io::OwnStation
```

Destination I/O number for the target CPU/module in 3C/4C routing.

#### `request_destination_module_station_no`

```cpp
std::uint8_t mcprotocol::serial::RouteConfig::request_destination_module_station_no = 0x00
```

Destination station number for the target CPU/module in 3C/4C routing.

#### `self_station_enabled`

```cpp
bool mcprotocol::serial::RouteConfig::self_station_enabled = false
```

Enables self-station routing on frame families that support it.

#### `self_station_no`

```cpp
std::uint8_t mcprotocol::serial::RouteConfig::self_station_no = 0x00
```

Self-station number used when self_station_enabled is true.

### Struct `mcprotocol::serial::ProtocolConfig`

Top-level protocol configuration shared by codecs and client requests.

Treat this as the immutable session configuration for one serial link. The same object is used by:

- FrameCodec for frame wrapping and response decoding - CommandCodec for command subcommand/device-layout differences - MelsecSerialClient and PosixSyncClient for runtime request execution

#### Fields

#### `frame_kind`

```cpp
FrameKind mcprotocol::serial::ProtocolConfig::frame_kind = FrameKind::C4
```

Selected serial frame family.

#### `code_mode`

```cpp
CodeMode mcprotocol::serial::ProtocolConfig::code_mode = CodeMode::Binary
```

Selected payload encoding inside the frame.

#### `ascii_format`

```cpp
AsciiFormat mcprotocol::serial::ProtocolConfig::ascii_format = AsciiFormat::Format3
```

Selected ASCII framing flavor when code_mode == CodeMode::Ascii.

#### `ascii_block_number`

```cpp
std::uint8_t mcprotocol::serial::ProtocolConfig::ascii_block_number = 0x00
```

Block number used only by ASCII Format2 on 2C/3C/4C.

The external device chooses this value in the range 0x00..0xFF. It is ignored by Format1, Format3, Format4, binary Format5, 1C, and 1E.

#### `plc_profile`

```cpp
PlcProfile mcprotocol::serial::ProtocolConfig::plc_profile = PlcProfile::Unspecified
```

Public PLC profile used to derive frame-family compatibility and device/subcommand layout.

Applications must set this explicitly before encoding requests or running a client.

#### `sum_check_enabled`

```cpp
bool mcprotocol::serial::ProtocolConfig::sum_check_enabled = true
```

Enables or disables the ASCII/binary sum-check where that frame family supports it.

#### `route`

```cpp
RouteConfig mcprotocol::serial::ProtocolConfig::route {}
```

Route header fields used for every encoded request.

#### `timeout`

```cpp
TimeoutConfig mcprotocol::serial::ProtocolConfig::timeout {}
```

Request timeout policy used by the async client and stream decoder.

### Struct `mcprotocol::serial::DeviceAddress`

Device code plus numeric address.

This is the normalized address form used throughout the library after string-address parsing.

#### Fields

#### `code`

```cpp
DeviceCode mcprotocol::serial::DeviceAddress::code = DeviceCode::D
```

Device family such as D, M, X, LTN, or LZ.

#### `number`

```cpp
std::uint32_t mcprotocol::serial::DeviceAddress::number = 0
```

Numeric index inside the selected device family.

### Struct `mcprotocol::serial::ExtendedFileRegisterAddress`

Extended file-register address using block number plus R word number.

This is the block-addressed form used by 1C ACPU-common and by the chapter-18 block path on 1E.

#### Fields

#### `block_number`

```cpp
std::uint16_t mcprotocol::serial::ExtendedFileRegisterAddress::block_number = 1
```

Extended file-register block number.

#### `word_number`

```cpp
std::uint16_t mcprotocol::serial::ExtendedFileRegisterAddress::word_number = 0
```

Word number inside the selected block.

### Struct `mcprotocol::serial::BatchReadWordsRequest`

#### Fields

#### `head_device`

```cpp
DeviceAddress mcprotocol::serial::BatchReadWordsRequest::head_device {}
```

First device in the contiguous range.

#### `points`

```cpp
std::uint16_t mcprotocol::serial::BatchReadWordsRequest::points = 0
```

Number of points to read starting at head_device.

### Struct `mcprotocol::serial::BatchReadBitsRequest`

Contiguous bit-read request (0401 bit path).

#### Fields

#### `head_device`

```cpp
DeviceAddress mcprotocol::serial::BatchReadBitsRequest::head_device {}
```

First bit device in the contiguous range.

#### `points`

```cpp
std::uint16_t mcprotocol::serial::BatchReadBitsRequest::points = 0
```

Number of bit points to read starting at head_device.

### Struct `mcprotocol::serial::BatchWriteWordsRequest`

Contiguous word-write request (1401).

#### Fields

#### `head_device`

```cpp
DeviceAddress mcprotocol::serial::BatchWriteWordsRequest::head_device {}
```

First device in the contiguous write range.

#### `words`

```cpp
std::span<const std::uint16_t> mcprotocol::serial::BatchWriteWordsRequest::words {}
```

Caller-owned word data to write starting at head_device.

### Struct `mcprotocol::serial::BatchWriteBitsRequest`

Contiguous bit-write request (1401 bit path).

#### Fields

#### `head_device`

```cpp
DeviceAddress mcprotocol::serial::BatchWriteBitsRequest::head_device {}
```

First bit device in the contiguous write range.

#### `bits`

```cpp
std::span<const BitValue> mcprotocol::serial::BatchWriteBitsRequest::bits {}
```

Caller-owned bit data to write starting at head_device.

### Struct `mcprotocol::serial::ExtendedFileRegisterBatchReadWordsRequest`

#### Fields

#### `head_device`

```cpp
ExtendedFileRegisterAddress mcprotocol::serial::ExtendedFileRegisterBatchReadWordsRequest::head_device {}
```

First block-addressed file-register word to read.

#### `points`

```cpp
std::uint16_t mcprotocol::serial::ExtendedFileRegisterBatchReadWordsRequest::points = 0
```

Number of words to read from the file-register range.

### Struct `mcprotocol::serial::ExtendedFileRegisterDirectBatchReadWordsRequest`

Direct extended file-register batch read (NR on 1C AnA/AnUCPU common, chapter-18 direct path on 1E).

#### Fields

#### `head_device_number`

```cpp
std::uint32_t mcprotocol::serial::ExtendedFileRegisterDirectBatchReadWordsRequest::head_device_number = 0
```

NR/NW direct address on 1C or the chapter-18 direct R address on 1E.

#### `points`

```cpp
std::uint16_t mcprotocol::serial::ExtendedFileRegisterDirectBatchReadWordsRequest::points = 0
```

Number of words to read from the direct file-register range.

### Struct `mcprotocol::serial::ExtendedFileRegisterBatchWriteWordsRequest`

Extended file-register batch write (EW on 1C ACPU-common, chapter-18 block path on 1E).

#### Fields

#### `head_device`

```cpp
ExtendedFileRegisterAddress mcprotocol::serial::ExtendedFileRegisterBatchWriteWordsRequest::head_device {}
```

First block-addressed file-register word to write.

#### `words`

```cpp
std::span<const std::uint16_t> mcprotocol::serial::ExtendedFileRegisterBatchWriteWordsRequest::words {}
```

Caller-owned word data to write starting at head_device.

### Struct `mcprotocol::serial::ExtendedFileRegisterDirectBatchWriteWordsRequest`

Direct extended file-register batch write (NW on 1C AnA/AnUCPU common, chapter-18 direct path on 1E).

#### Fields

#### `head_device_number`

```cpp
std::uint32_t mcprotocol::serial::ExtendedFileRegisterDirectBatchWriteWordsRequest::head_device_number = 0
```

NR/NW direct address on 1C or the chapter-18 direct R address on 1E.

#### `words`

```cpp
std::span<const std::uint16_t> mcprotocol::serial::ExtendedFileRegisterDirectBatchWriteWordsRequest::words {}
```

Caller-owned word data to write starting at head_device_number.

### Struct `mcprotocol::serial::ExtendedFileRegisterRandomWriteWordItem`

One item inside extended file-register random write (ET on 1C, chapter-18 on 1E).

#### Fields

#### `device`

```cpp
ExtendedFileRegisterAddress mcprotocol::serial::ExtendedFileRegisterRandomWriteWordItem::device {}
```

Target extended file-register address.

#### `value`

```cpp
std::uint16_t mcprotocol::serial::ExtendedFileRegisterRandomWriteWordItem::value = 0
```

One word written to device.

### Struct `mcprotocol::serial::ExtendedFileRegisterMonitorRegistration`

Extended file-register monitor registration (EM on 1C, chapter-18 on 1E).

#### Fields

#### `items`

```cpp
std::span<const ExtendedFileRegisterAddress> mcprotocol::serial::ExtendedFileRegisterMonitorRegistration::items {}
```

Sparse list of block-addressed file-register items to register for monitoring.

### Struct `mcprotocol::serial::RandomReadItem`

#### Fields

#### `device`

```cpp
DeviceAddress mcprotocol::serial::RandomReadItem::device {}
```

Target device address for this sparse item.

#### `double_word`

```cpp
bool mcprotocol::serial::RandomReadItem::double_word = false
```

true when the item should be encoded as a double-word device access.

### Struct `mcprotocol::serial::RandomReadRequest`

Native random-read request made of sparse word/dword items.

#### Fields

#### `items`

```cpp
std::span<const RandomReadItem> mcprotocol::serial::RandomReadRequest::items {}
```

Sparse word/dword items encoded in the native random-read request.

### Struct `mcprotocol::serial::RandomWriteWordItem`

One word or double-word item inside native random write (1402 word path).

#### Fields

#### `device`

```cpp
DeviceAddress mcprotocol::serial::RandomWriteWordItem::device {}
```

Target device address for the sparse write.

#### `value`

```cpp
std::uint32_t mcprotocol::serial::RandomWriteWordItem::value = 0
```

One word or double-word value to write.

#### `double_word`

```cpp
bool mcprotocol::serial::RandomWriteWordItem::double_word = false
```

true when the target is encoded as a double-word write item.

### Struct `mcprotocol::serial::RandomWriteBitItem`

One bit item inside native random write (1402 bit path).

#### Fields

#### `device`

```cpp
DeviceAddress mcprotocol::serial::RandomWriteBitItem::device {}
```

Target bit device address for the sparse write.

#### `value`

```cpp
BitValue mcprotocol::serial::RandomWriteBitItem::value = BitValue::Off
```

Bit value written to device.

### Struct `mcprotocol::serial::MultiBlockReadBlock`

One block inside native multi-block read (0406).

#### Fields

#### `head_device`

```cpp
DeviceAddress mcprotocol::serial::MultiBlockReadBlock::head_device {}
```

First device in this contiguous block.

#### `points`

```cpp
std::uint16_t mcprotocol::serial::MultiBlockReadBlock::points = 0
```

Number of points in this block.

#### `bit_block`

```cpp
bool mcprotocol::serial::MultiBlockReadBlock::bit_block = false
```

true for bit blocks, false for word blocks.

### Struct `mcprotocol::serial::MultiBlockReadRequest`

Native multi-block read request composed of multiple contiguous blocks.

#### Fields

#### `blocks`

```cpp
std::span<const MultiBlockReadBlock> mcprotocol::serial::MultiBlockReadRequest::blocks {}
```

Ordered block list encoded into the native multi-block read request.

### Struct `mcprotocol::serial::MultiBlockWriteBlock`

One block inside native multi-block write (1406).

#### Fields

#### `head_device`

```cpp
DeviceAddress mcprotocol::serial::MultiBlockWriteBlock::head_device {}
```

First device in this contiguous block.

#### `points`

```cpp
std::uint16_t mcprotocol::serial::MultiBlockWriteBlock::points = 0
```

Point count for this block.

#### `bit_block`

```cpp
bool mcprotocol::serial::MultiBlockWriteBlock::bit_block = false
```

true when bits is used, false when words is used.

#### `words`

```cpp
std::span<const std::uint16_t> mcprotocol::serial::MultiBlockWriteBlock::words {}
```

Caller-owned word data for word blocks.

#### `bits`

```cpp
std::span<const BitValue> mcprotocol::serial::MultiBlockWriteBlock::bits {}
```

Caller-owned bit data for bit blocks.

### Struct `mcprotocol::serial::MultiBlockWriteRequest`

Native multi-block write request composed of multiple contiguous blocks.

#### Fields

#### `blocks`

```cpp
std::span<const MultiBlockWriteBlock> mcprotocol::serial::MultiBlockWriteRequest::blocks {}
```

Ordered block list encoded into the native multi-block write request.

### Struct `mcprotocol::serial::MultiBlockReadBlockResult`

Parsed layout description for one block returned by parse_multi_block_read_response().

#### Fields

#### `bit_block`

```cpp
bool mcprotocol::serial::MultiBlockReadBlockResult::bit_block = false
```

Block kind copied from the original request.

#### `head_device`

```cpp
DeviceAddress mcprotocol::serial::MultiBlockReadBlockResult::head_device {}
```

Block head device copied from the original request.

#### `points`

```cpp
std::uint16_t mcprotocol::serial::MultiBlockReadBlockResult::points = 0
```

Point count copied from the original request.

#### `data_offset`

```cpp
std::uint16_t mcprotocol::serial::MultiBlockReadBlockResult::data_offset = 0
```

Offset into the aggregate output storage returned by the parser.

#### `data_count`

```cpp
std::uint16_t mcprotocol::serial::MultiBlockReadBlockResult::data_count = 0
```

Number of entries contributed by this block to the aggregate output storage.

### Struct `mcprotocol::serial::MonitorRegistration`

#### Fields

#### `items`

```cpp
std::span<const RandomReadItem> mcprotocol::serial::MonitorRegistration::items {}
```

Sparse list of word/dword items to register for a later 0802 read.

### Struct `mcprotocol::serial::UserFrameReadRequest`

#### Fields

#### `frame_no`

```cpp
std::uint16_t mcprotocol::serial::UserFrameReadRequest::frame_no = 0
```

User-frame number to read, typically in the documented 0x0000..0x03FF or 0x8001..0x801F ranges.

### Struct `mcprotocol::serial::UserFrameRegistrationData`

User-frame registration-data payload returned by 0610.

#### Fields

#### `registration_data_bytes`

```cpp
std::uint16_t mcprotocol::serial::UserFrameRegistrationData::registration_data_bytes = 0
```

Number of valid bytes in registration_data.

#### `frame_bytes`

```cpp
std::uint16_t mcprotocol::serial::UserFrameRegistrationData::frame_bytes = 0
```

Optional frame-byte count returned by the target for the registered frame data.

#### `registration_data`

```cpp
std::array<std::byte, kMaxUserFrameRegistrationBytes> mcprotocol::serial::UserFrameRegistrationData::registration_data {}
```

Raw user-frame registration bytes as returned by the target.

### Struct `mcprotocol::serial::UserFrameWriteRequest`

User-frame registration-data write request (1610, subcommand 0000).

#### Fields

#### `frame_no`

```cpp
std::uint16_t mcprotocol::serial::UserFrameWriteRequest::frame_no = 0
```

User-frame number to overwrite.

#### `frame_bytes`

```cpp
std::uint16_t mcprotocol::serial::UserFrameWriteRequest::frame_bytes = 0
```

Frame-byte count encoded into the 1610 payload.

#### `registration_data`

```cpp
std::span<const std::byte> mcprotocol::serial::UserFrameWriteRequest::registration_data {}
```

Raw user-frame registration bytes to store.

### Struct `mcprotocol::serial::UserFrameDeleteRequest`

User-frame registration-data delete request (1610, subcommand 0001).

#### Fields

#### `frame_no`

```cpp
std::uint16_t mcprotocol::serial::UserFrameDeleteRequest::frame_no = 0
```

User-frame number to clear.

### Struct `mcprotocol::serial::GlobalSignalControlRequest`

C24 global-signal ON/OFF request (1618).

#### Fields

#### `target`

```cpp
GlobalSignalTarget mcprotocol::serial::GlobalSignalControlRequest::target = GlobalSignalTarget::ReceivedSide
```

Which global signal destination should be controlled.

#### `turn_on`

```cpp
bool mcprotocol::serial::GlobalSignalControlRequest::turn_on = false
```

true for ON, false for OFF.

### Struct `mcprotocol::serial::SerialModuleModeSwitchRequest`

C24 mode switching request (1612).

The three switch_* flags form the documented switching instruction byte: bit0 = mode number, bit1 = transmission setting, bit2 = communication speed. When a flag is false, the C24 uses the Engineering tool setting for that field.

#### Fields

#### `channel`

```cpp
SerialModuleChannel mcprotocol::serial::SerialModuleModeSwitchRequest::channel = SerialModuleChannel::Ch1
```

Target interface.

#### `switch_mode_no`

```cpp
bool mcprotocol::serial::SerialModuleModeSwitchRequest::switch_mode_no = false
```

true to use mode_no from this command.

#### `switch_transmission_setting`

```cpp
bool mcprotocol::serial::SerialModuleModeSwitchRequest::switch_transmission_setting = false
```

true to use transmission_setting from this command.

#### `switch_communication_speed`

```cpp
bool mcprotocol::serial::SerialModuleModeSwitchRequest::switch_communication_speed = false
```

true to use communication_speed from this command.

#### `mode_no`

```cpp
SerialModuleModeNo mcprotocol::serial::SerialModuleModeSwitchRequest::mode_no = SerialModuleModeNo::McProtocolFormat1
```

Operation mode number. The manual requires a valid non-zero value even when switch_mode_no is false.

#### `transmission_setting`

```cpp
std::uint8_t mcprotocol::serial::SerialModuleModeSwitchRequest::transmission_setting = 0
```

Raw transmission-setting bit field used when switch_transmission_setting is true.

#### `communication_speed`

```cpp
SerialModuleCommunicationSpeed mcprotocol::serial::SerialModuleModeSwitchRequest::communication_speed = SerialModuleCommunicationSpeed::Bps300
```

Communication speed used when switch_communication_speed is true.

### Struct `mcprotocol::serial::HostBufferReadRequest`

#### Fields

#### `start_address`

```cpp
std::uint32_t mcprotocol::serial::HostBufferReadRequest::start_address = 0
```

Starting host-buffer word address.

#### `word_length`

```cpp
std::uint16_t mcprotocol::serial::HostBufferReadRequest::word_length = 0
```

Number of words to read.

### Struct `mcprotocol::serial::HostBufferWriteRequest`

Host-buffer write request (1613).

#### Fields

#### `start_address`

```cpp
std::uint32_t mcprotocol::serial::HostBufferWriteRequest::start_address = 0
```

Starting host-buffer word address.

#### `words`

```cpp
std::span<const std::uint16_t> mcprotocol::serial::HostBufferWriteRequest::words {}
```

Caller-owned words written sequentially from start_address.

### Struct `mcprotocol::serial::ModuleBufferReadRequest`

Module-buffer byte read request (0601 helper path).

#### Fields

#### `start_address`

```cpp
std::uint32_t mcprotocol::serial::ModuleBufferReadRequest::start_address = 0
```

Starting module-buffer byte address.

#### `bytes`

```cpp
std::uint16_t mcprotocol::serial::ModuleBufferReadRequest::bytes = 0
```

Number of bytes to read.

#### `module_number`

```cpp
std::uint16_t mcprotocol::serial::ModuleBufferReadRequest::module_number = 0
```

Module number used by the addressed special-function module.

### Struct `mcprotocol::serial::ModuleBufferWriteRequest`

Module-buffer byte write request (1601 helper path).

#### Fields

#### `start_address`

```cpp
std::uint32_t mcprotocol::serial::ModuleBufferWriteRequest::start_address = 0
```

Starting module-buffer byte address.

#### `module_number`

```cpp
std::uint16_t mcprotocol::serial::ModuleBufferWriteRequest::module_number = 0
```

Module number used by the addressed special-function module.

#### `bytes`

```cpp
std::span<const std::byte> mcprotocol::serial::ModuleBufferWriteRequest::bytes {}
```

Caller-owned raw bytes written starting at start_address.

### Struct `mcprotocol::serial::CpuModelInfo`

#### Fields

#### `model_name`

```cpp
std::array<char, kCpuModelNameLength + 1> mcprotocol::serial::CpuModelInfo::model_name {}
```

Null-terminated CPU model name with trailing spaces already trimmed by the parser.

#### `model_code`

```cpp
std::uint16_t mcprotocol::serial::CpuModelInfo::model_code = 0
```

Raw model code returned by the target.

### Struct `mcprotocol::serial::Rs485Hooks`

Optional RS-485 callbacks used by the async client around TX start/end.

#### Fields

#### `on_tx_begin`

```cpp
void(* mcprotocol::serial::Rs485Hooks::on_tx_begin) (void *user) = nullptr
```

Optional callback fired immediately before the client expects TX to start.

#### `on_tx_end`

```cpp
void(* mcprotocol::serial::Rs485Hooks::on_tx_end) (void *user) = nullptr
```

Optional callback fired after TX completion or after cleanup on failure/cancel.

#### `user`

```cpp
void* mcprotocol::serial::Rs485Hooks::user = nullptr
```

Opaque user pointer passed back to both callbacks.
