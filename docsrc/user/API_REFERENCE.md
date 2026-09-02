# MC Protocol Serial C++ API Reference

This file is generated from Doxygen XML for the public C++ headers.
Do not edit it manually; run `scripts/update_api_reference.py` instead.

## Header Inputs

- `include/mcprotocol_serial.hpp`
- `include/mcprotocol/serial/types.hpp`
- `include/mcprotocol/serial/status.hpp`
- `include/mcprotocol/serial/byte.hpp`
- `include/mcprotocol/serial/span.hpp`
- `include/mcprotocol/serial/codec.hpp`
- `include/mcprotocol/serial/client.hpp`
- `include/mcprotocol/serial/high_level.hpp`
- `include/mcprotocol/serial/host_sync.hpp`
- `include/mcprotocol/serial/host_serial.hpp`
- `include/mcprotocol/serial/posix_serial.hpp`
- `include/mcprotocol/serial/link_direct.hpp`
- `include/mcprotocol/serial/qualified_buffer.hpp`

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

## Namespaces

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
ProtocolConfig mcprotocol::serial::highlevel::make_c4_binary_protocol(PlcProfile profile, SumCheckMode sum_check_mode, RouteConfig route) noexcept
```

Returns a practical Format5 / Binary / C4 configuration for an explicit PLC profile.

#### `make_c4_ascii_format4_protocol`

```cpp
ProtocolConfig mcprotocol::serial::highlevel::make_c4_ascii_format4_protocol(PlcProfile profile, SumCheckMode sum_check_mode, RouteConfig route) noexcept
```

Returns a practical Format4 / ASCII / C4 configuration for an explicit PLC profile.

#### `make_c4_ascii_format2_protocol`

```cpp
ProtocolConfig mcprotocol::serial::highlevel::make_c4_ascii_format2_protocol(PlcProfile profile, SumCheckMode sum_check_mode, RouteConfig route) noexcept
```

Returns a Format2 / ASCII / C4 configuration with explicit profile and sum-check mode.

Format2 is the Format1 style ENQ/ACK/NAK/STX/ETX link with an extra 1-byte block number inserted before the frame ID. Block-number lifecycle is addressed separately by D-096.

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
Status mcprotocol::serial::highlevel::decode_long_state_bit(const LongStateReadSpec &spec, mcprotocol::serial::Span< const std::uint16_t > status_block_words, BitValue &out_value) noexcept
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
Status mcprotocol::serial::highlevel::make_batch_write_words_request(std::string_view head_device, mcprotocol::serial::Span< const std::uint16_t > words, BatchWriteWordsRequest &out_request) noexcept
```

Builds a contiguous word-write request from a string address such as D100.

#### `make_batch_write_bits_request`

```cpp
Status mcprotocol::serial::highlevel::make_batch_write_bits_request(std::string_view head_device, mcprotocol::serial::Span< const BitValue > bits, BatchWriteBitsRequest &out_request) noexcept
```

Builds a contiguous bit-write request from a string address such as M100.

#### `make_random_read_word_item`

```cpp
Status mcprotocol::serial::highlevel::make_random_read_word_item(std::string_view device, RandomReadWordItem &out_item) noexcept
```

Builds one explicitly word-width sparse random-read item from a string address.

#### `make_random_read_dword_item`

```cpp
Status mcprotocol::serial::highlevel::make_random_read_dword_item(std::string_view device, RandomReadDWordItem &out_item) noexcept
```

Builds one explicitly double-word-width sparse random-read item.

#### `make_random_write_word_item`

```cpp
Status mcprotocol::serial::highlevel::make_random_write_word_item(std::string_view device, std::uint16_t value, RandomWriteWordItem &out_item) noexcept
```

Builds one sparse random word-write item from a string address.

#### `make_random_write_dword_item`

```cpp
Status mcprotocol::serial::highlevel::make_random_write_dword_item(std::string_view device, std::uint32_t value, RandomWriteDWordItem &out_item) noexcept
```

Builds one explicitly double-word-width sparse random write item.

#### `make_random_write_bit_item`

```cpp
Status mcprotocol::serial::highlevel::make_random_write_bit_item(std::string_view device, BitValue value, RandomWriteBitItem &out_item) noexcept
```

Builds one sparse random bit-write item from a string address.

#### `make_random_read_request`

```cpp
Status mcprotocol::serial::highlevel::make_random_read_request(mcprotocol::serial::Span< const RandomReadWordSpec > word_specs, mcprotocol::serial::Span< const RandomReadDWordSpec > dword_specs, mcprotocol::serial::Span< RandomReadWordItem > out_word_items, mcprotocol::serial::Span< RandomReadDWordItem > out_dword_items, RandomReadRequest &out_request) noexcept
```

Builds a sparse random-read request from string-address specs.

Use this when you want 0403 style sparse addressing without hand-filling the explicit-width Word and DWord item types.

#### `make_monitor_registration`

```cpp
Status mcprotocol::serial::highlevel::make_monitor_registration(mcprotocol::serial::Span< const RandomReadWordSpec > word_specs, mcprotocol::serial::Span< const RandomReadDWordSpec > dword_specs, mcprotocol::serial::Span< RandomReadWordItem > out_word_items, mcprotocol::serial::Span< RandomReadDWordItem > out_dword_items, MonitorRegistration &out_request) noexcept
```

Builds a sparse monitor registration payload from string-address specs.

The resulting payload is intended for 0801. Readback still happens through the normal monitor read API.

#### `make_random_write_word_items`

```cpp
Status mcprotocol::serial::highlevel::make_random_write_word_items(mcprotocol::serial::Span< const RandomWriteWordSpec > specs, mcprotocol::serial::Span< RandomWriteWordItem > out_items, mcprotocol::serial::Span< const RandomWriteWordItem > &out_item_view) noexcept
```

Builds sparse random word-write items from string-address specs.

#### `make_random_write_dword_items`

```cpp
Status mcprotocol::serial::highlevel::make_random_write_dword_items(mcprotocol::serial::Span< const RandomWriteDWordSpec > specs, mcprotocol::serial::Span< RandomWriteDWordItem > out_items, mcprotocol::serial::Span< const RandomWriteDWordItem > &out_item_view) noexcept
```

Builds sparse explicit double-word write items from string-address specs.

#### `make_random_write_bit_items`

```cpp
Status mcprotocol::serial::highlevel::make_random_write_bit_items(mcprotocol::serial::Span< const RandomWriteBitSpec > specs, mcprotocol::serial::Span< RandomWriteBitItem > out_items, mcprotocol::serial::Span< const RandomWriteBitItem > &out_item_view) noexcept
```

Builds sparse random bit-write items from string-address specs.

### Namespace `mcprotocol::serial::CommandCodec`

Command-payload codec helpers below the frame layer.

These helpers operate on request/response data only. They do not add or remove the surrounding C1/C2/C3/C4 frame bytes.

#### Functions

#### `encode_batch_read_words`

```cpp
Status mcprotocol::serial::CommandCodec::encode_batch_read_words(const ProtocolConfig &config, const BatchReadWordsRequest &request, mcprotocol::serial::Span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_read_extended_file_register_words`

```cpp
Status mcprotocol::serial::CommandCodec::encode_read_extended_file_register_words(const ProtocolConfig &config, const ExtendedFileRegisterBatchReadWordsRequest &request, mcprotocol::serial::Span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_direct_read_extended_file_register_words`

```cpp
Status mcprotocol::serial::CommandCodec::encode_direct_read_extended_file_register_words(const ProtocolConfig &config, const ExtendedFileRegisterDirectBatchReadWordsRequest &request, mcprotocol::serial::Span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_extended_batch_read_words`

```cpp
Status mcprotocol::serial::CommandCodec::encode_extended_batch_read_words(const ProtocolConfig &config, const QualifiedBufferWordDevice &device, std::uint16_t points, mcprotocol::serial::Span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_link_direct_batch_read_words`

```cpp
Status mcprotocol::serial::CommandCodec::encode_link_direct_batch_read_words(const ProtocolConfig &config, const LinkDirectDevice &device, std::uint16_t points, mcprotocol::serial::Span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `parse_batch_read_words_response`

```cpp
Status mcprotocol::serial::CommandCodec::parse_batch_read_words_response(const ProtocolConfig &config, const BatchReadWordsRequest &request, mcprotocol::serial::Span< const std::uint8_t > response_data, mcprotocol::serial::Span< std::uint16_t > out_words) noexcept
```

#### `parse_read_extended_file_register_words_response`

```cpp
Status mcprotocol::serial::CommandCodec::parse_read_extended_file_register_words_response(const ProtocolConfig &config, std::uint16_t points, mcprotocol::serial::Span< const std::uint8_t > response_data, mcprotocol::serial::Span< std::uint16_t > out_words) noexcept
```

#### `parse_extended_batch_read_words_response`

```cpp
Status mcprotocol::serial::CommandCodec::parse_extended_batch_read_words_response(const ProtocolConfig &config, std::uint16_t points, mcprotocol::serial::Span< const std::uint8_t > response_data, mcprotocol::serial::Span< std::uint16_t > out_words) noexcept
```

#### `encode_batch_read_bits`

```cpp
Status mcprotocol::serial::CommandCodec::encode_batch_read_bits(const ProtocolConfig &config, const BatchReadBitsRequest &request, mcprotocol::serial::Span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_link_direct_batch_read_bits`

```cpp
Status mcprotocol::serial::CommandCodec::encode_link_direct_batch_read_bits(const ProtocolConfig &config, const LinkDirectDevice &device, std::uint16_t points, mcprotocol::serial::Span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `parse_batch_read_bits_response`

```cpp
Status mcprotocol::serial::CommandCodec::parse_batch_read_bits_response(const ProtocolConfig &config, const BatchReadBitsRequest &request, mcprotocol::serial::Span< const std::uint8_t > response_data, mcprotocol::serial::Span< BitValue > out_bits) noexcept
```

#### `encode_batch_write_words`

```cpp
Status mcprotocol::serial::CommandCodec::encode_batch_write_words(const ProtocolConfig &config, const BatchWriteWordsRequest &request, mcprotocol::serial::Span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_write_extended_file_register_words`

```cpp
Status mcprotocol::serial::CommandCodec::encode_write_extended_file_register_words(const ProtocolConfig &config, const ExtendedFileRegisterBatchWriteWordsRequest &request, mcprotocol::serial::Span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_direct_write_extended_file_register_words`

```cpp
Status mcprotocol::serial::CommandCodec::encode_direct_write_extended_file_register_words(const ProtocolConfig &config, const ExtendedFileRegisterDirectBatchWriteWordsRequest &request, mcprotocol::serial::Span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_link_direct_batch_write_words`

```cpp
Status mcprotocol::serial::CommandCodec::encode_link_direct_batch_write_words(const ProtocolConfig &config, const LinkDirectDevice &device, mcprotocol::serial::Span< const std::uint16_t > words, mcprotocol::serial::Span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_extended_batch_write_words`

```cpp
Status mcprotocol::serial::CommandCodec::encode_extended_batch_write_words(const ProtocolConfig &config, const QualifiedBufferWordDevice &device, mcprotocol::serial::Span< const std::uint16_t > words, mcprotocol::serial::Span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_batch_write_bits`

```cpp
Status mcprotocol::serial::CommandCodec::encode_batch_write_bits(const ProtocolConfig &config, const BatchWriteBitsRequest &request, mcprotocol::serial::Span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_link_direct_batch_write_bits`

```cpp
Status mcprotocol::serial::CommandCodec::encode_link_direct_batch_write_bits(const ProtocolConfig &config, const LinkDirectDevice &device, mcprotocol::serial::Span< const BitValue > bits, mcprotocol::serial::Span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_link_direct_random_read`

```cpp
Status mcprotocol::serial::CommandCodec::encode_link_direct_random_read(const ProtocolConfig &config, mcprotocol::serial::Span< const LinkDirectRandomReadWordItem > word_items, mcprotocol::serial::Span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_random_read`

```cpp
Status mcprotocol::serial::CommandCodec::encode_random_read(const ProtocolConfig &config, const RandomReadRequest &request, mcprotocol::serial::Span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `parse_random_read_response`

```cpp
Status mcprotocol::serial::CommandCodec::parse_random_read_response(const ProtocolConfig &config, const RandomReadRequest &request, mcprotocol::serial::Span< const std::uint8_t > response_data, mcprotocol::serial::Span< std::uint16_t > out_words, mcprotocol::serial::Span< std::uint32_t > out_dwords) noexcept
```

#### `encode_random_write_words`

```cpp
Status mcprotocol::serial::CommandCodec::encode_random_write_words(const ProtocolConfig &config, mcprotocol::serial::Span< const RandomWriteWordItem > word_items, mcprotocol::serial::Span< const RandomWriteDWordItem > dword_items, mcprotocol::serial::Span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_random_write_extended_file_register_words`

```cpp
Status mcprotocol::serial::CommandCodec::encode_random_write_extended_file_register_words(const ProtocolConfig &config, mcprotocol::serial::Span< const ExtendedFileRegisterRandomWriteWordItem > items, mcprotocol::serial::Span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_link_direct_random_write_words`

```cpp
Status mcprotocol::serial::CommandCodec::encode_link_direct_random_write_words(const ProtocolConfig &config, mcprotocol::serial::Span< const LinkDirectRandomWriteWordItem > items, mcprotocol::serial::Span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_random_write_bits`

```cpp
Status mcprotocol::serial::CommandCodec::encode_random_write_bits(const ProtocolConfig &config, mcprotocol::serial::Span< const RandomWriteBitItem > items, mcprotocol::serial::Span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_link_direct_random_write_bits`

```cpp
Status mcprotocol::serial::CommandCodec::encode_link_direct_random_write_bits(const ProtocolConfig &config, mcprotocol::serial::Span< const LinkDirectRandomWriteBitItem > items, mcprotocol::serial::Span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_link_direct_multi_block_read`

```cpp
Status mcprotocol::serial::CommandCodec::encode_link_direct_multi_block_read(const ProtocolConfig &config, const LinkDirectMultiBlockReadRequest &request, mcprotocol::serial::Span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_multi_block_read`

```cpp
Status mcprotocol::serial::CommandCodec::encode_multi_block_read(const ProtocolConfig &config, const MultiBlockReadRequest &request, mcprotocol::serial::Span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `parse_multi_block_read_response`

```cpp
Status mcprotocol::serial::CommandCodec::parse_multi_block_read_response(const ProtocolConfig &config, mcprotocol::serial::Span< const MultiBlockReadBlock > blocks, mcprotocol::serial::Span< const std::uint8_t > response_data, mcprotocol::serial::Span< std::uint16_t > out_words, mcprotocol::serial::Span< BitValue > out_bits, mcprotocol::serial::Span< MultiBlockReadBlockResult > out_results) noexcept
```

#### `encode_multi_block_write`

```cpp
Status mcprotocol::serial::CommandCodec::encode_multi_block_write(const ProtocolConfig &config, const MultiBlockWriteRequest &request, mcprotocol::serial::Span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_link_direct_multi_block_write`

```cpp
Status mcprotocol::serial::CommandCodec::encode_link_direct_multi_block_write(const ProtocolConfig &config, const LinkDirectMultiBlockWriteRequest &request, mcprotocol::serial::Span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_register_monitor`

```cpp
Status mcprotocol::serial::CommandCodec::encode_register_monitor(const ProtocolConfig &config, const MonitorRegistration &request, mcprotocol::serial::Span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_register_extended_file_register_monitor`

```cpp
Status mcprotocol::serial::CommandCodec::encode_register_extended_file_register_monitor(const ProtocolConfig &config, const ExtendedFileRegisterMonitorRegistration &request, mcprotocol::serial::Span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_link_direct_register_monitor`

```cpp
Status mcprotocol::serial::CommandCodec::encode_link_direct_register_monitor(const ProtocolConfig &config, const LinkDirectMonitorRegistration &request, mcprotocol::serial::Span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_read_monitor`

```cpp
Status mcprotocol::serial::CommandCodec::encode_read_monitor(const ProtocolConfig &config, mcprotocol::serial::Span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_read_monitor`

```cpp
Status mcprotocol::serial::CommandCodec::encode_read_monitor(const ProtocolConfig &config, const MonitorRegistration &registration, mcprotocol::serial::Span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_read_extended_file_register_monitor`

```cpp
Status mcprotocol::serial::CommandCodec::encode_read_extended_file_register_monitor(const ProtocolConfig &config, mcprotocol::serial::Span< const ExtendedFileRegisterAddress > items, mcprotocol::serial::Span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `parse_read_monitor_response`

```cpp
Status mcprotocol::serial::CommandCodec::parse_read_monitor_response(const ProtocolConfig &config, const MonitorRegistration &registration, mcprotocol::serial::Span< const std::uint8_t > response_data, mcprotocol::serial::Span< std::uint16_t > out_words, mcprotocol::serial::Span< std::uint32_t > out_dwords) noexcept
```

#### `parse_read_extended_file_register_monitor_response`

```cpp
Status mcprotocol::serial::CommandCodec::parse_read_extended_file_register_monitor_response(const ProtocolConfig &config, mcprotocol::serial::Span< const ExtendedFileRegisterAddress > items, mcprotocol::serial::Span< const std::uint8_t > response_data, mcprotocol::serial::Span< std::uint16_t > out_words) noexcept
```

#### `encode_read_user_frame`

```cpp
Status mcprotocol::serial::CommandCodec::encode_read_user_frame(const ProtocolConfig &config, const UserFrameRegistrationReadRequest &request, mcprotocol::serial::Span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `parse_read_user_frame_response`

```cpp
Status mcprotocol::serial::CommandCodec::parse_read_user_frame_response(const ProtocolConfig &config, mcprotocol::serial::Span< const std::uint8_t > response_data, UserFrameRegistrationData &out_data) noexcept
```

#### `encode_write_user_frame`

```cpp
Status mcprotocol::serial::CommandCodec::encode_write_user_frame(const ProtocolConfig &config, const UserFrameRegistrationWriteRequest &request, mcprotocol::serial::Span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_delete_user_frame`

```cpp
Status mcprotocol::serial::CommandCodec::encode_delete_user_frame(const ProtocolConfig &config, const UserFrameRegistrationDeleteRequest &request, mcprotocol::serial::Span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_control_global_signal`

```cpp
Status mcprotocol::serial::CommandCodec::encode_control_global_signal(const ProtocolConfig &config, const GlobalSignalControlRequest &request, mcprotocol::serial::Span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_switch_serial_module_mode`

```cpp
Status mcprotocol::serial::CommandCodec::encode_switch_serial_module_mode(const ProtocolConfig &config, const SerialModuleModeSwitchRequest &request, mcprotocol::serial::Span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_initialize_transmission_sequence`

```cpp
Status mcprotocol::serial::CommandCodec::encode_initialize_transmission_sequence(const ProtocolConfig &config, mcprotocol::serial::Span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_read_host_buffer`

```cpp
Status mcprotocol::serial::CommandCodec::encode_read_host_buffer(const ProtocolConfig &config, const HostBufferReadRequest &request, mcprotocol::serial::Span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `parse_read_host_buffer_response`

```cpp
Status mcprotocol::serial::CommandCodec::parse_read_host_buffer_response(const ProtocolConfig &config, const HostBufferReadRequest &request, mcprotocol::serial::Span< const std::uint8_t > response_data, mcprotocol::serial::Span< std::uint16_t > out_words) noexcept
```

#### `encode_write_host_buffer`

```cpp
Status mcprotocol::serial::CommandCodec::encode_write_host_buffer(const ProtocolConfig &config, const HostBufferWriteRequest &request, mcprotocol::serial::Span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_read_module_buffer`

```cpp
Status mcprotocol::serial::CommandCodec::encode_read_module_buffer(const ProtocolConfig &config, const ModuleBufferReadRequest &request, mcprotocol::serial::Span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `parse_read_module_buffer_response`

```cpp
Status mcprotocol::serial::CommandCodec::parse_read_module_buffer_response(const ProtocolConfig &config, const ModuleBufferReadRequest &request, mcprotocol::serial::Span< const std::uint8_t > response_data, mcprotocol::serial::Span< mcprotocol::serial::Byte > out_bytes) noexcept
```

#### `encode_write_module_buffer`

```cpp
Status mcprotocol::serial::CommandCodec::encode_write_module_buffer(const ProtocolConfig &config, const ModuleBufferWriteRequest &request, mcprotocol::serial::Span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_read_cpu_model`

```cpp
Status mcprotocol::serial::CommandCodec::encode_read_cpu_model(const ProtocolConfig &config, mcprotocol::serial::Span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `parse_read_cpu_model_response`

```cpp
Status mcprotocol::serial::CommandCodec::parse_read_cpu_model_response(const ProtocolConfig &config, mcprotocol::serial::Span< const std::uint8_t > response_data, CpuModelInfo &out_info) noexcept
```

#### `encode_remote_run`

```cpp
Status mcprotocol::serial::CommandCodec::encode_remote_run(const ProtocolConfig &config, RemoteOperationMode mode, RemoteRunClearMode clear_mode, mcprotocol::serial::Span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_remote_stop`

```cpp
Status mcprotocol::serial::CommandCodec::encode_remote_stop(const ProtocolConfig &config, mcprotocol::serial::Span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_remote_pause`

```cpp
Status mcprotocol::serial::CommandCodec::encode_remote_pause(const ProtocolConfig &config, RemoteOperationMode mode, mcprotocol::serial::Span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_remote_latch_clear`

```cpp
Status mcprotocol::serial::CommandCodec::encode_remote_latch_clear(const ProtocolConfig &config, mcprotocol::serial::Span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_remote_reset`

```cpp
Status mcprotocol::serial::CommandCodec::encode_remote_reset(const ProtocolConfig &config, mcprotocol::serial::Span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_unlock_remote_password`

```cpp
Status mcprotocol::serial::CommandCodec::encode_unlock_remote_password(const ProtocolConfig &config, std::string_view remote_password, mcprotocol::serial::Span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_lock_remote_password`

```cpp
Status mcprotocol::serial::CommandCodec::encode_lock_remote_password(const ProtocolConfig &config, std::string_view remote_password, mcprotocol::serial::Span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_clear_error_information`

```cpp
Status mcprotocol::serial::CommandCodec::encode_clear_error_information(const ProtocolConfig &config, mcprotocol::serial::Span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `encode_loopback`

```cpp
Status mcprotocol::serial::CommandCodec::encode_loopback(const ProtocolConfig &config, mcprotocol::serial::Span< const char > hex_ascii, mcprotocol::serial::Span< std::uint8_t > out_request_data, std::size_t &out_size) noexcept
```

#### `parse_loopback_response`

```cpp
Status mcprotocol::serial::CommandCodec::parse_loopback_response(const ProtocolConfig &config, mcprotocol::serial::Span< const std::uint8_t > response_data, mcprotocol::serial::Span< char > out_echoed) noexcept
```

#### `module_buffer_start_address`

```cpp
Status mcprotocol::serial::CommandCodec::module_buffer_start_address(std::uint32_t buffer_memory_address, std::uint32_t module_additional_value, std::uint32_t &out_start_address) noexcept
```

Converts a logical buffer-memory word address plus module offset into a byte start address.

### Namespace `mcprotocol::serial`

#### Aliases

#### `BitValue`

```cpp
using mcprotocol::serial::BitValue = bool
```

Native Boolean value used by every individual bit read/write API.

Packed block words remain std::uint16_t; they are not individual bit-value inputs.

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
| `NotConnected` | The operation requires an established/configured connection or serial session. |
| `Closed` | A local lifecycle close interrupted or rejected the operation. |
| `Transport` |  |
| `Framing` |  |
| `SumCheckMismatch` |  |
| `Parse` |  |
| `UnsupportedConfiguration` |  |
| `PlcError` |  |
| `BufferTooSmall` |  |
| `Cancelled` |  |
| `OperationOutcomeUnknown` | A state-changing request may have reached the PLC, but its result was not confirmed. |
| `OutOfMemory` | Host-side temporary storage could not be allocated before communication started. |

#### `Byte`

One raw, non-arithmetic octet used by the public C++17 API.

Convert deliberately with byte_to_integer<Integer>(); no implicit numeric conversion or arithmetic operator is provided.

#### `SerialParity`

Explicit parity selection for a host serial port.

| Value | Description |
| --- | --- |
| `None` |  |
| `Even` |  |
| `Odd` |  |

#### `QualifiedBufferDeviceKind`

Qualified buffer-memory family used by helper U... accessors.

| Value | Description |
| --- | --- |
| `G` |  |
| `HG` |  |

#### `HardwareFlowControl`

Explicit hardware flow-control selection for a host serial port.

| Value | Description |
| --- | --- |
| `None` |  |
| `RtsCts` |  |

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
| `C2` | Smallest ASCII serial frame: compact selectors are exactly 0401/0001=1, 0401/0000=2, 1401/0001=3, 1401/0000=4, 0403/0000=5, 1402/0001=6, 1402/0000=7, 0801/0000=8, and 0802/0000=9; only loopback 0619/0000 uses the full command header; every other pair returns UnsupportedConfiguration before transmission, while the same public operation remains usable through 3C or 4C when supported there. |
| `C1` | Legacy ASCII serial frame with its own command naming and routing rules. |

#### `AsciiFrameKind`

Frame families whose public configuration includes an explicit ASCII format.

| Value | Description |
| --- | --- |
| `C4 = static_cast<std::uint8_t>(FrameKind::C4)` |  |
| `C3 = static_cast<std::uint8_t>(FrameKind::C3)` |  |
| `C2 = static_cast<std::uint8_t>(FrameKind::C2)` |  |
| `C1 = static_cast<std::uint8_t>(FrameKind::C1)` |  |

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

#### `SumCheckMode`

Explicit sum-check policy for frame families that support configuration.

| Value | Description |
| --- | --- |
| `Disabled` |  |
| `Enabled` |  |

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
| `Unspecified` | No route was selected. This value is observable but cannot encode a request. |
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

#### `C34PcTargetKind`

Meaning of a 3C/4C routed PC target.

| Value | Description |
| --- | --- |
| `Number` |  |
| `ControlSystem` |  |
| `StandbySystem` |  |
| `SpecialFe` |  |
| `ConnectedStation` |  |

#### `C4DestinationModuleKind`

Meaning of a mandatory 4C request-destination module target.

| Value | Description |
| --- | --- |
| `OwnStation` |  |
| `MultipleCpu` |  |
| `RedundantControlSystemCpu` |  |
| `RedundantStandbySystemCpu` |  |
| `RedundantSystemACpu` |  |
| `RedundantSystemBCpu` |  |
| `Explicit` |  |

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

#### `byte_to_integer`

```cpp
Integer mcprotocol::serial::byte_to_integer(Byte value) noexcept
```

#### `ok_status`

```cpp
Status mcprotocol::serial::ok_status() noexcept
```

Returns the default success status.

#### `make_status`

```cpp
Status mcprotocol::serial::make_status(StatusCode code, const char *message, std::uint16_t plc_error_code=0, StatusCode cause=StatusCode::Ok) noexcept
```

Builds a status value with an optional PLC end code.

#### `validate_serial_config`

```cpp
Status mcprotocol::serial::validate_serial_config(const HostSerialConfig &config) noexcept
```

#### `make_outcome_unknown_status`

```cpp
Status mcprotocol::serial::make_outcome_unknown_status(StatusCode cause, const char *message) noexcept
```

Builds an outcome-unknown status while retaining its machine-readable root reason.

#### `qualified_buffer_kind_name`

```cpp
const char * mcprotocol::serial::qualified_buffer_kind_name(QualifiedBufferDeviceKind kind) noexcept
```

Returns "G" or "HG" for the helper device kind.

#### `validate_mc_serial_config`

```cpp
Status mcprotocol::serial::validate_mc_serial_config(const HostSerialConfig &serial_config, const ProtocolConfig &protocol_config) noexcept
```

#### `qualified_buffer_word_to_byte_address`

```cpp
Status mcprotocol::serial::qualified_buffer_word_to_byte_address(std::uint32_t word_address, std::uint32_t &out_byte_address) noexcept
```

Converts a qualified word address to the corresponding module-buffer byte address.

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

#### `validate_qualified_buffer_helper_route`

```cpp
Status mcprotocol::serial::validate_qualified_buffer_helper_route(PlcProfile profile, const QualifiedBufferWordDevice &device) noexcept
```

Validates whether the helper 0601/1601 route may be used for a profile.

This helper route maps non-CPU Un\\G text onto module-buffer commands. CPU-buffer G/HG targets and profiles such as iQ-R, MELSEC-Q, MELSEC-L, iQ-L, and iQ-F require native access.

#### `parse_qualified_buffer_word_device`

```cpp
Status mcprotocol::serial::parse_qualified_buffer_word_device(std::string_view text, QualifiedBufferWordDevice &out_device) noexcept
```

Parses a qualified device string such as U3E0\\G10 or U3E0\\HG20.

#### `parse_link_direct_device`

```cpp
Status mcprotocol::serial::parse_link_direct_device(std::string_view text, LinkDirectDevice &out_device) noexcept
```

Parses a Jn\\... link-direct device string such as J1\\W100 or J1\\X10.

#### `frame_kind`

```cpp
FrameKind mcprotocol::serial::frame_kind(AsciiFrameKind value) noexcept
```

#### `is_valid_frame_kind`

```cpp
bool mcprotocol::serial::is_valid_frame_kind(FrameKind frame_kind) noexcept
```

Returns whether frame_kind is a defined public frame-family value.

#### `is_valid_code_mode`

```cpp
bool mcprotocol::serial::is_valid_code_mode(CodeMode code_mode) noexcept
```

Returns whether code_mode is a defined public payload-encoding value.

#### `is_valid_sum_check_mode`

```cpp
bool mcprotocol::serial::is_valid_sum_check_mode(SumCheckMode mode) noexcept
```

Returns whether mode is a defined public sum-check value.

#### `make_qualified_buffer_read_words_request`

```cpp
Status mcprotocol::serial::make_qualified_buffer_read_words_request(const QualifiedBufferWordDevice &device, std::uint16_t word_length, ModuleBufferReadRequest &out_request) noexcept
```

Builds a module-buffer read request for a non-CPU Un\\G helper range.

#### `is_valid_ascii_format`

```cpp
bool mcprotocol::serial::is_valid_ascii_format(AsciiFormat format) noexcept
```

Returns whether format is a defined public ASCII framing value.

#### `encode_qualified_buffer_word_values`

```cpp
Status mcprotocol::serial::encode_qualified_buffer_word_values(mcprotocol::serial::Span< const std::uint16_t > words, mcprotocol::serial::Span< mcprotocol::serial::Byte > out_bytes, std::size_t &out_size) noexcept
```

Encodes helper qualified word values into little-endian module-buffer bytes.

#### `plc_profile_name`

```cpp
const char * mcprotocol::serial::plc_profile_name(PlcProfile profile) noexcept
```

Returns the canonical saved string for a PLC profile.

#### `make_qualified_buffer_write_words_request`

```cpp
Status mcprotocol::serial::make_qualified_buffer_write_words_request(const QualifiedBufferWordDevice &device, mcprotocol::serial::Span< const std::uint16_t > words, mcprotocol::serial::Span< mcprotocol::serial::Byte > byte_storage, ModuleBufferWriteRequest &out_request, std::size_t &out_byte_count) noexcept
```

Builds a module-buffer write request for non-CPU Un\\G helper access.

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

#### `decode_qualified_buffer_word_values`

```cpp
Status mcprotocol::serial::decode_qualified_buffer_word_values(mcprotocol::serial::Span< const mcprotocol::serial::Byte > bytes, mcprotocol::serial::Span< std::uint16_t > out_words) noexcept
```

Decodes little-endian module-buffer bytes into helper qualified word values.

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

Named request-destination module I/O numbers used by 4C serial routing.

The CPU constants are useful when a 4C request is intentionally routed to a multi-CPU or redundant-CPU target. The serial request header accepts the documented request-destination module I/O number field; common CPU values are 0x03D0..0x03D3, 0x03E0..0x03E3, and own station 0x03FF. Remote-head names are provided as vocabulary aliases for parity with the other plc-comm implementations; do not assume a remote-head route is valid on serial hardware unless the selected module, PLC family, and configuration define it.

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

The intended MCU-side workflow is: call configure() start an async_* request call notify_tx_started(now_ms) immediately before the first UART write transmit pending_tx_frame() with the board UART layer call notify_tx_complete(now_ms, transport_status) when TX finishes or aborts feed received bytes with on_rx_bytes() call notify_rx_failure(receive_status) if the transport aborts response reception call poll() from the main loop or scheduler for deadline handling

Output spans passed to async_* requests must remain valid until the completion callback fires. Only one request may be active. A second enabled async_* request returns Busy before changing the active request's output storage, request metadata, or monitor state. Same-instance calls from different operating-system threads are prohibited; the caller owns scheduling. Separate instances are independent and may progress concurrently. Cancelling before notify_tx_started() completes immediately as Cancelled. Cancelling during TX records the cancellation but does not complete the request until the UART reports physical TX completion or abort through notify_tx_complete(). Likewise, a deadline reached while physical TX is pending is latched: poll() marks the transport for reset but keeps the request busy and leaves the RS-485 direction/completion callbacks pending until notify_tx_complete() reports that TX finished or was aborted. Once the decoder retains a possible response, TimeoutConfig::inter_byte_timeout_ms also bounds inactivity between chunks without extending the absolute transaction deadline. After transmission may have begun, any unconfirmed state-changing command completes as OperationOutcomeUnknown; the client never retries it automatically.

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

Configures a session, or acknowledges that the caller reset the underlying transport.

When requires_transport_reset() is true, drain/close/reopen the UART transport before calling this again. A successful call clears the flag and allows new requests.

#### `set_rs485_hooks`

```cpp
Status mcprotocol::serial::MelsecSerialClient::set_rs485_hooks(const Rs485Hooks &hooks) noexcept
```

Installs optional RS-485 TX begin/end hooks used by the async workflow.

Both callbacks must be supplied together or both omitted. Hooks cannot be changed while a request is in flight, which guarantees that each TX begin callback is paired with the matching TX end callback and user pointer.

#### `busy`

```cpp
bool mcprotocol::serial::MelsecSerialClient::busy() const noexcept
```

Returns whether a request is currently in flight.

#### `requires_transport_reset`

```cpp
bool mcprotocol::serial::MelsecSerialClient::requires_transport_reset() const noexcept
```

Returns true after an unsequenced ambiguous receive/transport failure until reset plus reconfiguration.

Format2 has a per-request block identity and can discard its own late response. Other frame families cannot safely distinguish a same-route late response from the next request.

#### `pending_tx_frame`

```cpp
mcprotocol::serial::Span< const mcprotocol::serial::Byte > mcprotocol::serial::MelsecSerialClient::pending_tx_frame() const noexcept
```

Returns the encoded frame that should be sent to the UART layer.

#### `notify_tx_started`

```cpp
Status mcprotocol::serial::MelsecSerialClient::notify_tx_started(std::uint32_t now_ms) noexcept
```

Starts the one absolute TX/drain/RX transaction deadline.

Call this immediately before the first transport write attempt. The same deadline remains in force through physical drain, receive, correlation, and decode. It is never restarted by partial progress. Calling it more than once for a request is rejected.

#### `transaction_deadline_ms`

```cpp
std::uint32_t mcprotocol::serial::MelsecSerialClient::transaction_deadline_ms() const noexcept
```

Returns the active absolute deadline, or zero before TX start / with no active request.

#### `notify_tx_complete`

```cpp
Status mcprotocol::serial::MelsecSerialClient::notify_tx_complete(std::uint32_t now_ms, Status transport_status) noexcept
```

Advances the state machine after the transport finished or aborted the pending TX.

transport_status is mandatory. Pass ok_status() only after confirmed physical TX completion; otherwise pass the actual transport failure or cancellation status. If poll() already latched the absolute deadline, this call releases TX ownership exactly once and publishes the latched timeout result regardless of the later transport status.

#### `on_rx_bytes`

```cpp
void mcprotocol::serial::MelsecSerialClient::on_rx_bytes(std::uint32_t now_ms, mcprotocol::serial::Span< const mcprotocol::serial::Byte > bytes) noexcept
```

Feeds received bytes into the response decoder.

#### `notify_rx_failure`

```cpp
Status mcprotocol::serial::MelsecSerialClient::notify_rx_failure(Status receive_status) noexcept
```

Completes an active response wait with the actual receive failure.

Call this only after successful TX completion when the transport cannot continue receiving. receive_status must be an error. The completion callback receives that status for a read-only request, or OperationOutcomeUnknown with receive_status.code as its cause for an unconfirmed state-changing request.

#### `poll`

```cpp
void mcprotocol::serial::MelsecSerialClient::poll(std::uint32_t now_ms) noexcept
```

Checks timeouts for the current in-flight request.

A timeout during pending physical TX is latched without ending the RS-485 hook, firing the completion callback, or clearing busy(). The application must finish or abort TX and call notify_tx_complete().

#### `cancel`

```cpp
void mcprotocol::serial::MelsecSerialClient::cancel() noexcept
```

Requests cancellation of the in-flight request.

Before TX starts, cancellation completes immediately. During TX, completion is deferred until notify_tx_complete() confirms that physical TX has completed or stopped, so an active RS-485 direction hook can always be released exactly once.

#### `async_batch_read_words`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_batch_read_words(std::uint32_t now_ms, const BatchReadWordsRequest &request, mcprotocol::serial::Span< std::uint16_t > out_words, CompletionHandler callback, void *user) noexcept
```

Starts contiguous word read (0401).

#### `async_read_extended_file_register_words`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_read_extended_file_register_words(std::uint32_t now_ms, const ExtendedFileRegisterBatchReadWordsRequest &request, mcprotocol::serial::Span< std::uint16_t > out_words, CompletionHandler callback, void *user) noexcept
```

Starts extended file-register word read.

#### `async_direct_read_extended_file_register_words`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_direct_read_extended_file_register_words(std::uint32_t now_ms, const ExtendedFileRegisterDirectBatchReadWordsRequest &request, mcprotocol::serial::Span< std::uint16_t > out_words, CompletionHandler callback, void *user) noexcept
```

Starts direct extended file-register word read.

#### `async_link_direct_batch_read_words`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_link_direct_batch_read_words(std::uint32_t now_ms, const LinkDirectDevice &device, std::uint16_t points, mcprotocol::serial::Span< std::uint16_t > out_words, CompletionHandler callback, void *user) noexcept
```

Starts Jn\\... link-direct word read over device extension specification.

#### `async_batch_read_bits`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_batch_read_bits(std::uint32_t now_ms, const BatchReadBitsRequest &request, mcprotocol::serial::Span< BitValue > out_bits, CompletionHandler callback, void *user) noexcept
```

Starts contiguous bit read (0401 bit path).

#### `async_link_direct_batch_read_bits`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_link_direct_batch_read_bits(std::uint32_t now_ms, const LinkDirectDevice &device, std::uint16_t points, mcprotocol::serial::Span< BitValue > out_bits, CompletionHandler callback, void *user) noexcept
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
Status mcprotocol::serial::MelsecSerialClient::async_link_direct_batch_write_words(std::uint32_t now_ms, const LinkDirectDevice &device, mcprotocol::serial::Span< const std::uint16_t > words, CompletionHandler callback, void *user) noexcept
```

Starts Jn\\... link-direct contiguous word write over device extension specification.

#### `async_batch_write_bits`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_batch_write_bits(std::uint32_t now_ms, const BatchWriteBitsRequest &request, CompletionHandler callback, void *user) noexcept
```

Starts contiguous bit write (1401 bit path).

#### `async_link_direct_batch_write_bits`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_link_direct_batch_write_bits(std::uint32_t now_ms, const LinkDirectDevice &device, mcprotocol::serial::Span< const BitValue > bits, CompletionHandler callback, void *user) noexcept
```

Starts Jn\\... link-direct contiguous bit write over device extension specification.

#### `async_qualified_buffer_batch_read_words`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_qualified_buffer_batch_read_words(std::uint32_t now_ms, const QualifiedBufferWordDevice &device, std::uint16_t points, mcprotocol::serial::Span< std::uint16_t > out_words, CompletionHandler callback, void *user) noexcept
```

Starts native-qualified word read (0401).

#### `async_extended_batch_read_words`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_extended_batch_read_words(std::uint32_t now_ms, const QualifiedBufferWordDevice &device, std::uint16_t points, mcprotocol::serial::Span< std::uint16_t > out_words, CompletionHandler callback, void *user) noexcept
```

Compatibility alias for async_qualified_buffer_batch_read_words.

#### `async_qualified_buffer_batch_write_words`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_qualified_buffer_batch_write_words(std::uint32_t now_ms, const QualifiedBufferWordDevice &device, mcprotocol::serial::Span< const std::uint16_t > words, CompletionHandler callback, void *user) noexcept
```

Starts native-qualified word write (1401).

#### `async_extended_batch_write_words`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_extended_batch_write_words(std::uint32_t now_ms, const QualifiedBufferWordDevice &device, mcprotocol::serial::Span< const std::uint16_t > words, CompletionHandler callback, void *user) noexcept
```

Compatibility alias for async_qualified_buffer_batch_write_words.

#### `async_random_read`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_random_read(std::uint32_t now_ms, const RandomReadRequest &request, mcprotocol::serial::Span< std::uint16_t > out_words, mcprotocol::serial::Span< std::uint32_t > out_dwords, CompletionHandler callback, void *user) noexcept
```

Starts native random read (0403).

#### `async_link_direct_random_read`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_link_direct_random_read(std::uint32_t now_ms, mcprotocol::serial::Span< const LinkDirectRandomReadWordItem > word_items, mcprotocol::serial::Span< std::uint16_t > out_words, CompletionHandler callback, void *user) noexcept
```

Starts native Jn\\... random read (0403 + device extension specification).

#### `async_random_write_words`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_random_write_words(std::uint32_t now_ms, mcprotocol::serial::Span< const RandomWriteWordItem > word_items, mcprotocol::serial::Span< const RandomWriteDWordItem > dword_items, CompletionHandler callback, void *user) noexcept
```

Starts native random word/dword write (1402 word path).

Every item requires an explicit value. Once transmission has started, timeout, cancellation, or an unconfirmed transport failure completes as StatusCode::OperationOutcomeUnknown; the library never retries the write.

#### `async_random_write_extended_file_register_words`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_random_write_extended_file_register_words(std::uint32_t now_ms, mcprotocol::serial::Span< const ExtendedFileRegisterRandomWriteWordItem > items, CompletionHandler callback, void *user) noexcept
```

Starts extended file-register random word write.

#### `async_link_direct_random_write_words`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_link_direct_random_write_words(std::uint32_t now_ms, mcprotocol::serial::Span< const LinkDirectRandomWriteWordItem > items, CompletionHandler callback, void *user) noexcept
```

Starts native Jn\\... random word write (1402 + device extension specification).

Every item requires an explicit value. An unconfirmed result after transmission is StatusCode::OperationOutcomeUnknown and is never retried automatically.

#### `async_random_write_bits`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_random_write_bits(std::uint32_t now_ms, mcprotocol::serial::Span< const RandomWriteBitItem > items, CompletionHandler callback, void *user) noexcept
```

Starts native random bit write (1402 bit path).

Every item requires an explicit Off or On. An unconfirmed result after transmission is StatusCode::OperationOutcomeUnknown and is never retried automatically.

#### `async_link_direct_random_write_bits`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_link_direct_random_write_bits(std::uint32_t now_ms, mcprotocol::serial::Span< const LinkDirectRandomWriteBitItem > items, CompletionHandler callback, void *user) noexcept
```

Starts native Jn\\... random bit write (1402 + device extension specification).

Every item requires an explicit Off or On. An unconfirmed result after transmission is StatusCode::OperationOutcomeUnknown and is never retried automatically.

#### `async_multi_block_read`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_multi_block_read(std::uint32_t now_ms, const MultiBlockReadRequest &request, mcprotocol::serial::Span< std::uint16_t > out_words, mcprotocol::serial::Span< BitValue > out_bits, mcprotocol::serial::Span< MultiBlockReadBlockResult > out_results, CompletionHandler callback, void *user) noexcept
```

Starts native multi-block read (0406).

#### `async_link_direct_multi_block_read`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_link_direct_multi_block_read(std::uint32_t now_ms, const LinkDirectMultiBlockReadRequest &request, mcprotocol::serial::Span< std::uint16_t > out_words, mcprotocol::serial::Span< BitValue > out_bits, mcprotocol::serial::Span< MultiBlockReadBlockResult > out_results, CompletionHandler callback, void *user) noexcept
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

#### `async_register_monitor_devices`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_register_monitor_devices(std::uint32_t now_ms, const MonitorRegistration &request, CompletionHandler callback, void *user) noexcept
```

Starts monitor registration (0801).

#### `async_register_monitor`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_register_monitor(std::uint32_t now_ms, const MonitorRegistration &request, CompletionHandler callback, void *user) noexcept
```

Compatibility alias for async_register_monitor_devices.

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

#### `async_run_monitor_cycle`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_run_monitor_cycle(std::uint32_t now_ms, mcprotocol::serial::Span< std::uint16_t > out_words, mcprotocol::serial::Span< std::uint32_t > out_dwords, CompletionHandler callback, void *user) noexcept
```

Starts monitor read (0802) using the most recent registration.

#### `async_read_monitor`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_read_monitor(std::uint32_t now_ms, mcprotocol::serial::Span< std::uint16_t > out_words, mcprotocol::serial::Span< std::uint32_t > out_dwords, CompletionHandler callback, void *user) noexcept
```

Compatibility alias for async_run_monitor_cycle.

#### `async_read_extended_file_register_monitor`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_read_extended_file_register_monitor(std::uint32_t now_ms, mcprotocol::serial::Span< std::uint16_t > out_words, CompletionHandler callback, void *user) noexcept
```

Starts extended file-register monitor read.

#### `async_read_host_buffer`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_read_host_buffer(std::uint32_t now_ms, const HostBufferReadRequest &request, mcprotocol::serial::Span< std::uint16_t > out_words, CompletionHandler callback, void *user) noexcept
```

Starts host-buffer read (0613).

#### `async_write_host_buffer`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_write_host_buffer(std::uint32_t now_ms, const HostBufferWriteRequest &request, CompletionHandler callback, void *user) noexcept
```

Starts host-buffer write (1613).

#### `async_read_module_buffer`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_read_module_buffer(std::uint32_t now_ms, const ModuleBufferReadRequest &request, mcprotocol::serial::Span< mcprotocol::serial::Byte > out_bytes, CompletionHandler callback, void *user) noexcept
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

Starts remote RUN (1001) with mandatory conflict and clear policies.

After transmission starts, an unconfirmed transport/timeout result is reported as StatusCode::OperationOutcomeUnknown; the library never retries this command.

#### `async_remote_stop`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_remote_stop(std::uint32_t now_ms, CompletionHandler callback, void *user) noexcept
```

Starts remote STOP (1002).

#### `async_remote_pause`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_remote_pause(std::uint32_t now_ms, RemoteOperationMode mode, CompletionHandler callback, void *user) noexcept
```

Starts remote PAUSE (1003) with a mandatory conflict policy.

After transmission starts, an unconfirmed transport/timeout result is reported as StatusCode::OperationOutcomeUnknown; the library never retries with another policy.

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

Completion means the request bytes were transmitted successfully. The command does not wait for a normal response and does not claim that the PLC completed its reset.

#### `async_read_user_frame_registration`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_read_user_frame_registration(std::uint32_t now_ms, const UserFrameRegistrationReadRequest &request, UserFrameRegistrationData &out_data, CompletionHandler callback, void *user) noexcept
```

Starts user-frame registration-data read (0610).

#### `async_read_user_frame`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_read_user_frame(std::uint32_t now_ms, const UserFrameRegistrationReadRequest &request, UserFrameRegistrationData &out_data, CompletionHandler callback, void *user) noexcept
```

Compatibility alias for async_read_user_frame_registration.

#### `async_write_user_frame_registration`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_write_user_frame_registration(std::uint32_t now_ms, const UserFrameRegistrationWriteRequest &request, CompletionHandler callback, void *user) noexcept
```

Starts user-frame registration-data write (1610, subcommand 0000).

#### `async_write_user_frame`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_write_user_frame(std::uint32_t now_ms, const UserFrameRegistrationWriteRequest &request, CompletionHandler callback, void *user) noexcept
```

Compatibility alias for async_write_user_frame_registration.

#### `async_delete_user_frame_registration`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_delete_user_frame_registration(std::uint32_t now_ms, const UserFrameRegistrationDeleteRequest &request, CompletionHandler callback, void *user) noexcept
```

Starts user-frame registration-data delete (1610, subcommand 0001).

#### `async_delete_user_frame`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_delete_user_frame(std::uint32_t now_ms, const UserFrameRegistrationDeleteRequest &request, CompletionHandler callback, void *user) noexcept
```

Compatibility alias for async_delete_user_frame_registration.

#### `async_control_global_signal`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_control_global_signal(std::uint32_t now_ms, const GlobalSignalControlRequest &request, CompletionHandler callback, void *user) noexcept
```

Starts C24 global-signal ON/OFF control (1618).

Completion confirms successful request transmission; it does not confirm the PLC signal state.

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

Completion confirms successful request transmission; it does not confirm the PLC state.

#### `async_loopback`

```cpp
Status mcprotocol::serial::MelsecSerialClient::async_loopback(std::uint32_t now_ms, mcprotocol::serial::Span< const char > hex_ascii, mcprotocol::serial::Span< char > out_echoed, CompletionHandler callback, void *user) noexcept
```

Starts loopback using hexadecimal ASCII payload bytes.

### Class `mcprotocol::serial::FrameCodecContext`

Per-wire-frame identity context kept outside static protocol configuration.

Normal clients allocate Format2 block numbers automatically. format2() exists for raw codec, test, and investigation callers that intentionally construct or decode one explicit wire frame.

#### Member Functions

#### `none`

```cpp
static FrameCodecContext mcprotocol::serial::FrameCodecContext::none() noexcept
```

#### `format2`

```cpp
static FrameCodecContext mcprotocol::serial::FrameCodecContext::format2(std::uint8_t block_number) noexcept
```

#### `has_format2_block_number`

```cpp
bool mcprotocol::serial::FrameCodecContext::has_format2_block_number() const noexcept
```

#### `format2_block_number`

```cpp
std::uint8_t mcprotocol::serial::FrameCodecContext::format2_block_number() const noexcept
```

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

#### `validate_request_capacity`

```cpp
static Status mcprotocol::serial::FrameCodec::validate_request_capacity(const ProtocolConfig &config, std::size_t request_data_size) noexcept
```

Verifies that a request payload fits the configured fixed frame capacity even when every byte eligible for binary DLE escaping expands on the wire.

This is a single-request admission check. InvalidArgument means the requested operation is not representable by this build; it is distinct from a caller-supplied output span being too small (BufferTooSmall).

#### `validate_response_capacity`

```cpp
static Status mcprotocol::serial::FrameCodec::validate_response_capacity(const ProtocolConfig &config, std::size_t response_data_size) noexcept
```

Verifies that a successful response payload fits the complete receive path.

Binary Format5 uses the worst case in which every unescaped payload byte is DLE and therefore occupies two wire bytes.

#### `encode_request`

```cpp
static Status mcprotocol::serial::FrameCodec::encode_request(const ProtocolConfig &config, mcprotocol::serial::Span< const std::uint8_t > request_data, mcprotocol::serial::Span< std::uint8_t > out_frame, std::size_t &out_size) noexcept
```

Wraps command data in the configured serial frame format.

request_data must already contain the command payload generated by CommandCodec.

#### `encode_request`

```cpp
static Status mcprotocol::serial::FrameCodec::encode_request(const ProtocolConfig &config, FrameCodecContext context, mcprotocol::serial::Span< const std::uint8_t > request_data, mcprotocol::serial::Span< std::uint8_t > out_frame, std::size_t &out_size) noexcept
```

Wraps command data using an explicit per-wire-frame identity context.

#### `encode_success_response`

```cpp
static Status mcprotocol::serial::FrameCodec::encode_success_response(const ProtocolConfig &config, mcprotocol::serial::Span< const std::uint8_t > response_data, mcprotocol::serial::Span< std::uint8_t > out_frame, std::size_t &out_size) noexcept
```

Builds a success response frame for tests and local tools.

This is mainly used by tests and local validation helpers. Library users typically decode real target responses instead of constructing synthetic ones.

#### `encode_success_response`

```cpp
static Status mcprotocol::serial::FrameCodec::encode_success_response(const ProtocolConfig &config, FrameCodecContext context, mcprotocol::serial::Span< const std::uint8_t > response_data, mcprotocol::serial::Span< std::uint8_t > out_frame, std::size_t &out_size) noexcept
```

#### `encode_error_response`

```cpp
static Status mcprotocol::serial::FrameCodec::encode_error_response(const ProtocolConfig &config, std::uint16_t error_code, mcprotocol::serial::Span< std::uint8_t > out_frame, std::size_t &out_size) noexcept
```

Builds a PLC-error response frame for tests and local tools.

#### `encode_error_response`

```cpp
static Status mcprotocol::serial::FrameCodec::encode_error_response(const ProtocolConfig &config, FrameCodecContext context, std::uint16_t error_code, mcprotocol::serial::Span< std::uint8_t > out_frame, std::size_t &out_size) noexcept
```

#### `decode_response`

```cpp
static DecodeResult mcprotocol::serial::FrameCodec::decode_response(const ProtocolConfig &config, mcprotocol::serial::Span< const std::uint8_t > bytes) noexcept
```

Decodes one response frame from the front of bytes.

The caller can use bytes_consumed to drop the decoded prefix and continue stream processing.

#### `decode_response`

```cpp
static DecodeResult mcprotocol::serial::FrameCodec::decode_response(const ProtocolConfig &config, FrameCodecContext context, mcprotocol::serial::Span< const std::uint8_t > bytes) noexcept
```

Decodes one response using an explicit per-wire-frame identity context.

### Class `mcprotocol::serial::highlevel::BitInWordWriteOperation`

Explicit non-blocking read-modify-write for one bit inside a 16-bit word device.

begin() validates both the read and write before the first request. The two requests occupy the same client continuously and share one absolute deadline. They are not PLC-atomic: PLC logic or another connection can modify the word between them. The write is always sent after a successful read, even when the selected bit already has the requested state. Keep this object alive until its completion callback runs.

#### Member Functions

#### `BitInWordWriteOperation`

```cpp
mcprotocol::serial::highlevel::BitInWordWriteOperation::BitInWordWriteOperation()=default
```

#### `BitInWordWriteOperation`

```cpp
mcprotocol::serial::highlevel::BitInWordWriteOperation::BitInWordWriteOperation(const BitInWordWriteOperation &)=delete
```

#### `operator=`

```cpp
BitInWordWriteOperation & mcprotocol::serial::highlevel::BitInWordWriteOperation::operator=(const BitInWordWriteOperation &)=delete
```

#### `begin`

```cpp
Status mcprotocol::serial::highlevel::BitInWordWriteOperation::begin(MelsecSerialClient &client, std::uint32_t now_ms, std::string_view word_device, int bit_index, bool value, CompletionHandler callback, void *user) noexcept
```

#### `begin_extended_file_register`

```cpp
Status mcprotocol::serial::highlevel::BitInWordWriteOperation::begin_extended_file_register(MelsecSerialClient &client, std::uint32_t now_ms, ExtendedFileRegisterAddress word_device, int bit_index, bool value, CompletionHandler callback, void *user) noexcept
```

#### `begin_direct_extended_file_register`

```cpp
Status mcprotocol::serial::highlevel::BitInWordWriteOperation::begin_direct_extended_file_register(MelsecSerialClient &client, std::uint32_t now_ms, std::uint32_t word_device_number, int bit_index, bool value, CompletionHandler callback, void *user) noexcept
```

#### `begin_link_direct`

```cpp
Status mcprotocol::serial::highlevel::BitInWordWriteOperation::begin_link_direct(MelsecSerialClient &client, std::uint32_t now_ms, LinkDirectDevice word_device, int bit_index, bool value, CompletionHandler callback, void *user) noexcept
```

#### `begin_qualified_buffer`

```cpp
Status mcprotocol::serial::highlevel::BitInWordWriteOperation::begin_qualified_buffer(MelsecSerialClient &client, std::uint32_t now_ms, QualifiedBufferWordDevice word_device, int bit_index, bool value, CompletionHandler callback, void *user) noexcept
```

#### `cancel`

```cpp
void mcprotocol::serial::highlevel::BitInWordWriteOperation::cancel() noexcept
```

#### `busy`

```cpp
bool mcprotocol::serial::highlevel::BitInWordWriteOperation::busy() const noexcept
```

### Class `mcprotocol::serial::HostSerialPort`

Minimal blocking host-side serial-port wrapper used by host tools.

This class is not required on MCU targets. It exists so the same request/response codec and client logic can be exercised from host-side validation tools on Windows and POSIX systems.

#### Member Functions

#### `HostSerialPort`

```cpp
mcprotocol::serial::HostSerialPort::HostSerialPort()=default
```

#### `~HostSerialPort`

```cpp
mcprotocol::serial::HostSerialPort::~HostSerialPort()
```

#### `HostSerialPort`

```cpp
mcprotocol::serial::HostSerialPort::HostSerialPort(const HostSerialPort &)=delete
```

#### `operator=`

```cpp
HostSerialPort & mcprotocol::serial::HostSerialPort::operator=(const HostSerialPort &)=delete
```

#### `open`

```cpp
Status mcprotocol::serial::HostSerialPort::open(const HostSerialConfig &config) noexcept
```

Opens the serial port and replaces all inherited line settings with config.

Software flow control is disabled. RTS/CTS is disabled for None and owned by the OS RTS/CTS handshake for RtsCts. Input/output/local modes use raw, nonblocking-read settings.

#### `close`

```cpp
void mcprotocol::serial::HostSerialPort::close() noexcept
```

Closes the serial port if it is open.

#### `is_open`

```cpp
bool mcprotocol::serial::HostSerialPort::is_open() const noexcept
```

Returns whether the serial port is currently open.

#### `native_handle`

```cpp
std::intptr_t mcprotocol::serial::HostSerialPort::native_handle() const noexcept
```

Returns the native handle value, or -1 when closed.

#### `write_all_until`

```cpp
Status mcprotocol::serial::HostSerialPort::write_all_until(mcprotocol::serial::Span< const mcprotocol::serial::Byte > bytes, std::uint32_t absolute_deadline_ms) noexcept
```

Writes the entire byte range without exceeding absolute_deadline_ms.

#### `read_some_until`

```cpp
Status mcprotocol::serial::HostSerialPort::read_some_until(mcprotocol::serial::Span< mcprotocol::serial::Byte > buffer, std::uint32_t absolute_deadline_ms, std::size_t &out_size) noexcept
```

Reads up to buffer.size() bytes without exceeding the same transaction deadline.

#### `flush_rx`

```cpp
Status mcprotocol::serial::HostSerialPort::flush_rx() noexcept
```

Drops unread RX data that is already buffered by the driver.

#### `drain_tx_until`

```cpp
Status mcprotocol::serial::HostSerialPort::drain_tx_until(std::uint32_t absolute_deadline_ms) noexcept
```

Waits until queued TX data has physically drained, bounded by the transaction deadline.

Queue polling yields before its first recheck and after observed progress. Only consecutive no-progress observations use a bounded sleep of at most one millisecond.

#### `set_rts`

```cpp
Status mcprotocol::serial::HostSerialPort::set_rts(bool enabled) noexcept
```

Sets the RTS line when the underlying driver supports it.

### Class `mcprotocol::serial::HostSyncClient`

Host-side synchronous convenience wrapper built on HostSerialPort and MelsecSerialClient.

This class is intentionally small:

- it keeps the existing low-level client unchanged - it opens a host-side serial port - it runs one request synchronously from TX to completion - it exposes string-address helpers for common contiguous, sparse random, and monitor operations

Use it on Windows or POSIX hosts when you want a simpler bring-up path than manually driving pending_tx_frame(), notify_tx_complete(), on_rx_bytes(), and poll(). State-changing methods return OperationOutcomeUnknown whenever transmission may have begun but the PLC result cannot be confirmed. They are not retried automatically.

#### Member Functions

#### `HostSyncClient`

```cpp
mcprotocol::serial::HostSyncClient::HostSyncClient()=default
```

#### `HostSyncClient`

```cpp
mcprotocol::serial::HostSyncClient::HostSyncClient(const HostSyncClient &)=delete
```

#### `operator=`

```cpp
HostSyncClient & mcprotocol::serial::HostSyncClient::operator=(const HostSyncClient &)=delete
```

#### `open`

```cpp
Status mcprotocol::serial::HostSyncClient::open(const HostSerialConfig &serial_config, const ProtocolConfig &protocol_config) noexcept
```

Opens the serial port and configures the underlying MC protocol client.

#### `close`

```cpp
void mcprotocol::serial::HostSyncClient::close() noexcept
```

Closes the serial port and clears any in-flight request state.

#### `is_open`

```cpp
bool mcprotocol::serial::HostSyncClient::is_open() const noexcept
```

Returns whether the underlying serial port is open.

#### `protocol_config`

```cpp
const ProtocolConfig & mcprotocol::serial::HostSyncClient::protocol_config() const noexcept
```

Returns the currently configured protocol settings.

#### `read_cpu_model`

```cpp
Status mcprotocol::serial::HostSyncClient::read_cpu_model(CpuModelInfo &out_info) noexcept
```

Reads the remote CPU model synchronously.

#### `remote_run`

```cpp
Status mcprotocol::serial::HostSyncClient::remote_run(RemoteOperationMode mode, RemoteRunClearMode clear_mode) noexcept
```

Issues remote RUN (1001) synchronously.

Both the conflict policy and clear scope are mandatory. If transmission starts but the response cannot be confirmed, this returns StatusCode::OperationOutcomeUnknown.

#### `remote_stop`

```cpp
Status mcprotocol::serial::HostSyncClient::remote_stop() noexcept
```

Issues remote STOP (1002) synchronously.

#### `remote_pause`

```cpp
Status mcprotocol::serial::HostSyncClient::remote_pause(RemoteOperationMode mode) noexcept
```

Issues remote PAUSE (1003) synchronously.

The conflict policy is mandatory. If transmission starts but the response cannot be confirmed, this returns StatusCode::OperationOutcomeUnknown.

#### `remote_latch_clear`

```cpp
Status mcprotocol::serial::HostSyncClient::remote_latch_clear() noexcept
```

Issues remote latch clear (1005) synchronously.

#### `unlock_remote_password`

```cpp
Status mcprotocol::serial::HostSyncClient::unlock_remote_password(std::string_view remote_password) noexcept
```

Unlocks remote-password-protected access (1630) synchronously.

#### `lock_remote_password`

```cpp
Status mcprotocol::serial::HostSyncClient::lock_remote_password(std::string_view remote_password) noexcept
```

Locks remote-password-protected access (1631) synchronously.

#### `clear_error_information`

```cpp
Status mcprotocol::serial::HostSyncClient::clear_error_information() noexcept
```

Clears serial/C24 error information (1617) synchronously.

#### `remote_reset`

```cpp
Status mcprotocol::serial::HostSyncClient::remote_reset() noexcept
```

Issues remote RESET (1006) synchronously.

Completion means the request bytes were transmitted successfully. It does not confirm the resulting PLC reset state.

#### `read_user_frame_registration`

```cpp
Status mcprotocol::serial::HostSyncClient::read_user_frame_registration(const UserFrameRegistrationReadRequest &request, UserFrameRegistrationData &out_data) noexcept
```

Reads user-frame registration data synchronously (0610).

#### `read_user_frame`

```cpp
Status mcprotocol::serial::HostSyncClient::read_user_frame(const UserFrameRegistrationReadRequest &request, UserFrameRegistrationData &out_data) noexcept
```

Compatibility alias for read_user_frame_registration.

#### `write_user_frame_registration`

```cpp
Status mcprotocol::serial::HostSyncClient::write_user_frame_registration(const UserFrameRegistrationWriteRequest &request) noexcept
```

Writes user-frame registration data synchronously (1610, subcommand 0000).

#### `write_user_frame`

```cpp
Status mcprotocol::serial::HostSyncClient::write_user_frame(const UserFrameRegistrationWriteRequest &request) noexcept
```

Compatibility alias for write_user_frame_registration.

#### `delete_user_frame_registration`

```cpp
Status mcprotocol::serial::HostSyncClient::delete_user_frame_registration(const UserFrameRegistrationDeleteRequest &request) noexcept
```

Deletes user-frame registration data synchronously (1610, subcommand 0001).

#### `delete_user_frame`

```cpp
Status mcprotocol::serial::HostSyncClient::delete_user_frame(const UserFrameRegistrationDeleteRequest &request) noexcept
```

Compatibility alias for delete_user_frame_registration.

#### `control_global_signal`

```cpp
Status mcprotocol::serial::HostSyncClient::control_global_signal(const GlobalSignalControlRequest &request) noexcept
```

Controls C24 global signal ON/OFF synchronously (1618).

Success confirms request transmission; it does not confirm the PLC signal state.

#### `switch_serial_module_mode`

```cpp
Status mcprotocol::serial::HostSyncClient::switch_serial_module_mode(const SerialModuleModeSwitchRequest &request) noexcept
```

Switches C24 operation mode / transmission settings synchronously (1612).

#### `initialize_c24_transmission_sequence`

```cpp
Status mcprotocol::serial::HostSyncClient::initialize_c24_transmission_sequence() noexcept
```

Initializes C24 format-5 transmission sequence synchronously (1615).

Success confirms request transmission; it does not confirm the PLC state.

#### `read_words_single_request`

```cpp
Status mcprotocol::serial::HostSyncClient::read_words_single_request(std::string_view head_device, std::uint16_t points, mcprotocol::serial::Span< std::uint16_t > out_words) noexcept
```

Reads contiguous words as exactly one PLC request.

#### `read_words_single_request`

```cpp
Status mcprotocol::serial::HostSyncClient::read_words_single_request(std::string_view head_device, mcprotocol::serial::Span< std::uint16_t > out_words) noexcept
```

Reads contiguous words as exactly one PLC request using out_words.size().

#### `read_words`

```cpp
Status mcprotocol::serial::HostSyncClient::read_words(std::string_view head_device, std::uint16_t points, mcprotocol::serial::Span< std::uint16_t > out_words) noexcept
```

Compatibility alias for read_words_single_request.

#### `read_words`

```cpp
Status mcprotocol::serial::HostSyncClient::read_words(std::string_view head_device, mcprotocol::serial::Span< std::uint16_t > out_words) noexcept
```

Compatibility alias for read_words_single_request.

#### `read_extended_file_register_words`

```cpp
Status mcprotocol::serial::HostSyncClient::read_extended_file_register_words(const ExtendedFileRegisterBatchReadWordsRequest &request, mcprotocol::serial::Span< std::uint16_t > out_words) noexcept
```

Reads extended file-register words synchronously.

#### `read_direct_extended_file_register_words`

```cpp
Status mcprotocol::serial::HostSyncClient::read_direct_extended_file_register_words(const ExtendedFileRegisterDirectBatchReadWordsRequest &request, mcprotocol::serial::Span< std::uint16_t > out_words) noexcept
```

Reads direct extended file-register words synchronously.

#### `direct_read_extended_file_register_words`

```cpp
Status mcprotocol::serial::HostSyncClient::direct_read_extended_file_register_words(const ExtendedFileRegisterDirectBatchReadWordsRequest &request, mcprotocol::serial::Span< std::uint16_t > out_words) noexcept
```

Compatibility alias for read_direct_extended_file_register_words.

#### `read_bits_single_request`

```cpp
Status mcprotocol::serial::HostSyncClient::read_bits_single_request(std::string_view head_device, std::uint16_t points, mcprotocol::serial::Span< BitValue > out_bits) noexcept
```

Reads contiguous bits as exactly one PLC request.

#### `read_bits_single_request`

```cpp
Status mcprotocol::serial::HostSyncClient::read_bits_single_request(std::string_view head_device, mcprotocol::serial::Span< BitValue > out_bits) noexcept
```

Reads contiguous bits as exactly one PLC request using out_bits.size().

#### `read_bits`

```cpp
Status mcprotocol::serial::HostSyncClient::read_bits(std::string_view head_device, std::uint16_t points, mcprotocol::serial::Span< BitValue > out_bits) noexcept
```

Compatibility alias for read_bits_single_request.

#### `read_bits`

```cpp
Status mcprotocol::serial::HostSyncClient::read_bits(std::string_view head_device, mcprotocol::serial::Span< BitValue > out_bits) noexcept
```

Compatibility alias for read_bits_single_request.

#### `read_link_direct_words`

```cpp
Status mcprotocol::serial::HostSyncClient::read_link_direct_words(std::string_view head_device, std::uint16_t points, mcprotocol::serial::Span< std::uint16_t > out_words) noexcept
```

Reads contiguous Jn\\... link-direct words synchronously.

#### `read_link_direct_bits`

```cpp
Status mcprotocol::serial::HostSyncClient::read_link_direct_bits(std::string_view head_device, std::uint16_t points, mcprotocol::serial::Span< BitValue > out_bits) noexcept
```

Reads contiguous Jn\\... link-direct bits synchronously.

#### `read_qualified_buffer_words`

```cpp
Status mcprotocol::serial::HostSyncClient::read_qualified_buffer_words(std::string_view head_device, std::uint16_t points, mcprotocol::serial::Span< std::uint16_t > out_words) noexcept
```

Reads qualified-buffer Un\\Gn or Un\\HGn words.

Use this for profiles whose qualified access route is native device access (0401). The 0601 helper route is profile/target-specific and may be rejected.

#### `read_native_qualified_words`

```cpp
Status mcprotocol::serial::HostSyncClient::read_native_qualified_words(std::string_view head_device, std::uint16_t points, mcprotocol::serial::Span< std::uint16_t > out_words) noexcept
```

Compatibility alias for read_qualified_buffer_words.

#### `read_long_timer_counter_state_bits`

```cpp
Status mcprotocol::serial::HostSyncClient::read_long_timer_counter_state_bits(std::string_view head_device, std::uint16_t points, mcprotocol::serial::Span< BitValue > out_bits) noexcept
```

Reads long timer/counter contact or coil states through the dedicated status-block path.

LTS/LTC/LSTS/LSTC with more than one point are explicitly aggregate reads: one four-word status-block request is issued per point, in address order. The complete plan is validated before transmission, the result is non-atomic across PLC scan times, and caller output is changed only after every internal request succeeds. LCS/LCC use one direct bit request and are not split. The host aggregate allocates ceil(points / 8) staging bytes before the first send and returns StatusCode::OutOfMemory if that allocation fails.

#### `read_long_timer_counter_state_bits`

```cpp
Status mcprotocol::serial::HostSyncClient::read_long_timer_counter_state_bits(std::string_view head_device, mcprotocol::serial::Span< BitValue > out_bits) noexcept
```

Reads long timer/counter states using out_bits.size() as the point count.

#### `read_long_state_bits`

```cpp
Status mcprotocol::serial::HostSyncClient::read_long_state_bits(std::string_view head_device, std::uint16_t points, mcprotocol::serial::Span< BitValue > out_bits) noexcept
```

Compatibility alias for read_long_timer_counter_state_bits.

#### `read_long_state_bits`

```cpp
Status mcprotocol::serial::HostSyncClient::read_long_state_bits(std::string_view head_device, mcprotocol::serial::Span< BitValue > out_bits) noexcept
```

Compatibility alias for read_long_timer_counter_state_bits.

#### `write_words_single_request`

```cpp
Status mcprotocol::serial::HostSyncClient::write_words_single_request(std::string_view head_device, mcprotocol::serial::Span< const std::uint16_t > words) noexcept
```

Writes contiguous words as exactly one PLC request.

#### `write_words`

```cpp
Status mcprotocol::serial::HostSyncClient::write_words(std::string_view head_device, mcprotocol::serial::Span< const std::uint16_t > words) noexcept
```

Compatibility alias for write_words_single_request.

#### `write_bit_in_word`

```cpp
Status mcprotocol::serial::HostSyncClient::write_bit_in_word(std::string_view word_device, int bit_index, bool value) noexcept
```

Writes one bit inside an ordinary 16-bit word by one read-modify-write turn.

The complete two-request plan is validated before transmission. The read and write share one absolute deadline, and the write is always issued after a successful read even when the bit is already in the requested state. The operation is not PLC-atomic: PLC logic or another connection can modify the word between the read and write.

#### `write_extended_file_register_bit_in_word`

```cpp
Status mcprotocol::serial::HostSyncClient::write_extended_file_register_bit_in_word(ExtendedFileRegisterAddress word_device, int bit_index, bool value) noexcept
```

Bit-in-word update through the block-addressed extended file-register route.

#### `write_direct_extended_file_register_bit_in_word`

```cpp
Status mcprotocol::serial::HostSyncClient::write_direct_extended_file_register_bit_in_word(std::uint32_t word_device_number, int bit_index, bool value) noexcept
```

Bit-in-word update through the direct extended file-register route.

#### `direct_write_extended_file_register_bit_in_word`

```cpp
Status mcprotocol::serial::HostSyncClient::direct_write_extended_file_register_bit_in_word(std::uint32_t word_device_number, int bit_index, bool value) noexcept
```

Compatibility alias for write_direct_extended_file_register_bit_in_word.

#### `write_link_direct_bit_in_word`

```cpp
Status mcprotocol::serial::HostSyncClient::write_link_direct_bit_in_word(std::string_view word_device, int bit_index, bool value) noexcept
```

Bit-in-word update through one immutable Jn\\... link-direct route.

#### `write_qualified_buffer_bit_in_word`

```cpp
Status mcprotocol::serial::HostSyncClient::write_qualified_buffer_bit_in_word(std::string_view word_device, int bit_index, bool value) noexcept
```

Bit-in-word update through one immutable qualified-buffer route.

#### `write_native_qualified_bit_in_word`

```cpp
Status mcprotocol::serial::HostSyncClient::write_native_qualified_bit_in_word(std::string_view word_device, int bit_index, bool value) noexcept
```

Compatibility alias for write_qualified_buffer_bit_in_word.

#### `write_extended_file_register_words`

```cpp
Status mcprotocol::serial::HostSyncClient::write_extended_file_register_words(const ExtendedFileRegisterBatchWriteWordsRequest &request) noexcept
```

Writes extended file-register words synchronously.

#### `write_direct_extended_file_register_words`

```cpp
Status mcprotocol::serial::HostSyncClient::write_direct_extended_file_register_words(const ExtendedFileRegisterDirectBatchWriteWordsRequest &request) noexcept
```

Writes direct extended file-register words synchronously.

#### `direct_write_extended_file_register_words`

```cpp
Status mcprotocol::serial::HostSyncClient::direct_write_extended_file_register_words(const ExtendedFileRegisterDirectBatchWriteWordsRequest &request) noexcept
```

Compatibility alias for write_direct_extended_file_register_words.

#### `write_bits_single_request`

```cpp
Status mcprotocol::serial::HostSyncClient::write_bits_single_request(std::string_view head_device, mcprotocol::serial::Span< const BitValue > bits) noexcept
```

Writes contiguous bits as exactly one PLC request.

#### `write_bits`

```cpp
Status mcprotocol::serial::HostSyncClient::write_bits(std::string_view head_device, mcprotocol::serial::Span< const BitValue > bits) noexcept
```

Compatibility alias for write_bits_single_request.

#### `write_link_direct_words`

```cpp
Status mcprotocol::serial::HostSyncClient::write_link_direct_words(std::string_view head_device, mcprotocol::serial::Span< const std::uint16_t > words) noexcept
```

Writes contiguous Jn\\... link-direct words synchronously.

#### `write_link_direct_bits`

```cpp
Status mcprotocol::serial::HostSyncClient::write_link_direct_bits(std::string_view head_device, mcprotocol::serial::Span< const BitValue > bits) noexcept
```

Writes contiguous Jn\\... link-direct bits synchronously.

#### `write_qualified_buffer_words`

```cpp
Status mcprotocol::serial::HostSyncClient::write_qualified_buffer_words(std::string_view head_device, mcprotocol::serial::Span< const std::uint16_t > words) noexcept
```

Writes qualified-buffer Un\\Gn or Un\\HGn words.

Use this for profiles whose qualified access route is native device access (1401). The 1601 helper route is profile/target-specific and may be rejected.

#### `write_native_qualified_words`

```cpp
Status mcprotocol::serial::HostSyncClient::write_native_qualified_words(std::string_view head_device, mcprotocol::serial::Span< const std::uint16_t > words) noexcept
```

Compatibility alias for write_qualified_buffer_words.

#### `read_random`

```cpp
Status mcprotocol::serial::HostSyncClient::read_random(mcprotocol::serial::Span< const highlevel::RandomReadWordSpec > word_items, mcprotocol::serial::Span< const highlevel::RandomReadDWordSpec > dword_items, mcprotocol::serial::Span< std::uint16_t > out_words, mcprotocol::serial::Span< std::uint32_t > out_dwords) noexcept
```

Reads sparse Word and DWord items synchronously from explicit-width specs.

#### `random_read`

```cpp
Status mcprotocol::serial::HostSyncClient::random_read(mcprotocol::serial::Span< const highlevel::RandomReadWordSpec > word_items, mcprotocol::serial::Span< const highlevel::RandomReadDWordSpec > dword_items, mcprotocol::serial::Span< std::uint16_t > out_words, mcprotocol::serial::Span< std::uint32_t > out_dwords) noexcept
```

Compatibility alias for read_random.

#### `read_random_word`

```cpp
Status mcprotocol::serial::HostSyncClient::read_random_word(std::string_view device, std::uint16_t &out_value) noexcept
```

Reads one sparse Word item synchronously from a string address.

#### `random_read_word`

```cpp
Status mcprotocol::serial::HostSyncClient::random_read_word(std::string_view device, std::uint16_t &out_value) noexcept
```

Compatibility alias for read_random_word.

#### `read_random_dword`

```cpp
Status mcprotocol::serial::HostSyncClient::read_random_dword(std::string_view device, std::uint32_t &out_value) noexcept
```

Reads one sparse DWord item synchronously from a string address.

#### `random_read_dword`

```cpp
Status mcprotocol::serial::HostSyncClient::random_read_dword(std::string_view device, std::uint32_t &out_value) noexcept
```

Compatibility alias for read_random_dword.

#### `write_random_words`

```cpp
Status mcprotocol::serial::HostSyncClient::write_random_words(mcprotocol::serial::Span< const highlevel::RandomWriteWordSpec > items) noexcept
```

Writes sparse Word items synchronously from string-address specs.

Each spec requires an explicit value. A result that cannot be confirmed after transmission is reported as StatusCode::OperationOutcomeUnknown and is never retried automatically.

#### `random_write_words`

```cpp
Status mcprotocol::serial::HostSyncClient::random_write_words(mcprotocol::serial::Span< const highlevel::RandomWriteWordSpec > items) noexcept
```

Compatibility alias for write_random_words.

#### `write_random_dwords`

```cpp
Status mcprotocol::serial::HostSyncClient::write_random_dwords(mcprotocol::serial::Span< const highlevel::RandomWriteDWordSpec > items) noexcept
```

Writes sparse DWord items synchronously from string-address specs.

Each spec requires an explicit value. A result that cannot be confirmed after transmission is reported as StatusCode::OperationOutcomeUnknown and is never retried automatically.

#### `random_write_dwords`

```cpp
Status mcprotocol::serial::HostSyncClient::random_write_dwords(mcprotocol::serial::Span< const highlevel::RandomWriteDWordSpec > items) noexcept
```

Compatibility alias for write_random_dwords.

#### `write_random_extended_file_register_words`

```cpp
Status mcprotocol::serial::HostSyncClient::write_random_extended_file_register_words(mcprotocol::serial::Span< const ExtendedFileRegisterRandomWriteWordItem > items) noexcept
```

Writes extended file-register words randomly.

#### `random_write_extended_file_register_words`

```cpp
Status mcprotocol::serial::HostSyncClient::random_write_extended_file_register_words(mcprotocol::serial::Span< const ExtendedFileRegisterRandomWriteWordItem > items) noexcept
```

Compatibility alias for write_random_extended_file_register_words.

#### `write_random_word`

```cpp
Status mcprotocol::serial::HostSyncClient::write_random_word(std::string_view device, std::uint16_t value) noexcept
```

Writes one sparse Word item synchronously from a string address.

#### `random_write_word`

```cpp
Status mcprotocol::serial::HostSyncClient::random_write_word(std::string_view device, std::uint16_t value) noexcept
```

Compatibility alias for write_random_word.

#### `write_random_dword`

```cpp
Status mcprotocol::serial::HostSyncClient::write_random_dword(std::string_view device, std::uint32_t value) noexcept
```

Writes one sparse DWord item synchronously from a string address.

#### `random_write_dword`

```cpp
Status mcprotocol::serial::HostSyncClient::random_write_dword(std::string_view device, std::uint32_t value) noexcept
```

Compatibility alias for write_random_dword.

#### `write_random_bits`

```cpp
Status mcprotocol::serial::HostSyncClient::write_random_bits(mcprotocol::serial::Span< const highlevel::RandomWriteBitSpec > items) noexcept
```

Writes sparse bit items synchronously from string-address specs.

Each spec requires an explicit Off or On. A result that cannot be confirmed after transmission is StatusCode::OperationOutcomeUnknown and is never retried automatically.

#### `random_write_bits`

```cpp
Status mcprotocol::serial::HostSyncClient::random_write_bits(mcprotocol::serial::Span< const highlevel::RandomWriteBitSpec > items) noexcept
```

Compatibility alias for write_random_bits.

#### `write_random_bit`

```cpp
Status mcprotocol::serial::HostSyncClient::write_random_bit(std::string_view device, BitValue value) noexcept
```

Writes one sparse bit item synchronously from a string address.

#### `random_write_bit`

```cpp
Status mcprotocol::serial::HostSyncClient::random_write_bit(std::string_view device, BitValue value) noexcept
```

Compatibility alias for write_random_bit.

#### `read_random_link_direct_words`

```cpp
Status mcprotocol::serial::HostSyncClient::read_random_link_direct_words(Span< const LinkDirectRandomReadWordItem > word_items, Span< std::uint16_t > out_words) noexcept
```

Reads sparse link-direct word items in input order.

Bit devices return one 16-point mask in the corresponding output word. The request is never split, retried, or routed through the plain-device random API.

#### `write_random_link_direct_words`

```cpp
Status mcprotocol::serial::HostSyncClient::write_random_link_direct_words(Span< const LinkDirectRandomWriteWordItem > items) noexcept
```

Writes sparse link-direct word items as one request.

#### `write_random_link_direct_bits`

```cpp
Status mcprotocol::serial::HostSyncClient::write_random_link_direct_bits(Span< const LinkDirectRandomWriteBitItem > items) noexcept
```

Writes sparse link-direct bit items as one request.

#### `self_test_loopback`

```cpp
Status mcprotocol::serial::HostSyncClient::self_test_loopback(Span< const char > hex_ascii, Span< char > out_echoed) noexcept
```

Runs the existing MC Serial self-test loopback operation synchronously.

Both spans remain caller-owned. out_echoed is NUL-terminated only when its capacity exceeds the echoed payload length, exactly as in MelsecSerialClient::async_loopback.

#### `read_block`

```cpp
Status mcprotocol::serial::HostSyncClient::read_block(const MultiBlockReadRequest &request, Span< std::uint16_t > out_words, Span< BitValue > out_bits, Span< MultiBlockReadBlockResult > out_results) noexcept
```

Reads native multi-block data into caller-owned flat buffers and block metadata.

Bit-block point counts are 16-bit units and expand to 16 BitValue entries per point. Outputs can be partially updated if response parsing fails.

#### `write_block`

```cpp
Status mcprotocol::serial::HostSyncClient::write_block(const MultiBlockWriteRequest &request) noexcept
```

Writes native multi-block data as one request without automatic retry.

#### `read_link_direct_block`

```cpp
Status mcprotocol::serial::HostSyncClient::read_link_direct_block(const LinkDirectMultiBlockReadRequest &request, Span< std::uint16_t > out_words, Span< BitValue > out_bits, Span< MultiBlockReadBlockResult > out_results) noexcept
```

Reads link-direct multi-block data into caller-owned flat buffers and block metadata.

Result metadata retains each inner DeviceAddress; network numbers remain in request. Outputs can be partially updated if response parsing fails.

#### `write_link_direct_block`

```cpp
Status mcprotocol::serial::HostSyncClient::write_link_direct_block(const LinkDirectMultiBlockWriteRequest &request) noexcept
```

Writes link-direct multi-block data as one request without automatic retry.

#### `register_monitor_devices`

```cpp
Status mcprotocol::serial::HostSyncClient::register_monitor_devices(mcprotocol::serial::Span< const highlevel::RandomReadWordSpec > word_items, mcprotocol::serial::Span< const highlevel::RandomReadDWordSpec > dword_items) noexcept
```

Registers sparse Word and DWord monitor items from explicit-width specs.

#### `register_monitor`

```cpp
Status mcprotocol::serial::HostSyncClient::register_monitor(mcprotocol::serial::Span< const highlevel::RandomReadWordSpec > word_items, mcprotocol::serial::Span< const highlevel::RandomReadDWordSpec > dword_items) noexcept
```

Compatibility alias for register_monitor_devices.

#### `register_monitor_word`

```cpp
Status mcprotocol::serial::HostSyncClient::register_monitor_word(std::string_view device) noexcept
```

Registers one sparse Word monitor item synchronously.

#### `register_monitor_dword`

```cpp
Status mcprotocol::serial::HostSyncClient::register_monitor_dword(std::string_view device) noexcept
```

Registers one sparse DWord monitor item synchronously.

#### `register_extended_file_register_monitor`

```cpp
Status mcprotocol::serial::HostSyncClient::register_extended_file_register_monitor(const ExtendedFileRegisterMonitorRegistration &request) noexcept
```

Registers extended file-register monitor data synchronously.

#### `register_link_direct_monitor_devices`

```cpp
Status mcprotocol::serial::HostSyncClient::register_link_direct_monitor_devices(const LinkDirectMonitorRegistration &request) noexcept
```

Registers link-direct monitor word items without running a monitor cycle.

Use run_monitor_cycle(out_words, {}) explicitly after successful registration.

#### `run_monitor_cycle`

```cpp
Status mcprotocol::serial::HostSyncClient::run_monitor_cycle(mcprotocol::serial::Span< std::uint16_t > out_words, mcprotocol::serial::Span< std::uint32_t > out_dwords) noexcept
```

Reads the most recently registered Word and DWord monitor items synchronously.

#### `read_monitor`

```cpp
Status mcprotocol::serial::HostSyncClient::read_monitor(mcprotocol::serial::Span< std::uint16_t > out_words, mcprotocol::serial::Span< std::uint32_t > out_dwords) noexcept
```

Compatibility alias for run_monitor_cycle.

#### `read_extended_file_register_monitor`

```cpp
Status mcprotocol::serial::HostSyncClient::read_extended_file_register_monitor(mcprotocol::serial::Span< std::uint16_t > out_words) noexcept
```

Reads the most recently registered extended file-register monitor items synchronously.

### Class `mcprotocol::serial::Span`

Non-owning contiguous range used by the public C++17 API.

The library owns this type instead of adding a pre-C++20 span implementation to namespace std, which is undefined behavior. The pointed-to storage must outlive the Span.

#### Aliases

#### `element_type`

```cpp
using mcprotocol::serial::Span< T >::element_type = T
```

#### `value_type`

```cpp
using mcprotocol::serial::Span< T >::value_type = typename detail::SpanValueType<T>::type
```

#### `size_type`

```cpp
using mcprotocol::serial::Span< T >::size_type = std::size_t
```

#### `difference_type`

```cpp
using mcprotocol::serial::Span< T >::difference_type = std::ptrdiff_t
```

#### `pointer`

```cpp
using mcprotocol::serial::Span< T >::pointer = element_type*
```

#### `reference`

```cpp
using mcprotocol::serial::Span< T >::reference = element_type&
```

#### `iterator`

```cpp
using mcprotocol::serial::Span< T >::iterator = pointer
```

#### Member Functions

#### `Span`

```cpp
mcprotocol::serial::Span< T >::Span() noexcept=default
```

#### `Span`

```cpp
mcprotocol::serial::Span< T >::Span(pointer ptr, size_type count) noexcept
```

Creates a view over count live elements beginning at ptr.

ptr must be non-null when count != 0; the caller owns that storage and lifetime.

#### `Span`

```cpp
mcprotocol::serial::Span< T >::Span(pointer first, pointer last) noexcept
```

Creates a view over the valid half-open range [first, last).

Both pointers must belong to the same live array object, and last must not precede first.

#### `Span`

```cpp
mcprotocol::serial::Span< T >::Span(element_type(&values)[N]) noexcept
```

#### `Span`

```cpp
mcprotocol::serial::Span< T >::Span(std::array< value_type, N > &values) noexcept
```

#### `Span`

```cpp
mcprotocol::serial::Span< T >::Span(const std::array< value_type, N > &values) noexcept
```

#### `Span`

```cpp
mcprotocol::serial::Span< T >::Span(std::array< value_type, N > &&)=delete
```

#### `Span`

```cpp
mcprotocol::serial::Span< T >::Span(const std::array< value_type, N > &&)=delete
```

#### `Span`

```cpp
mcprotocol::serial::Span< T >::Span(const Span< value_type > &other) noexcept
```

#### `begin`

```cpp
iterator mcprotocol::serial::Span< T >::begin() const noexcept
```

#### `end`

```cpp
iterator mcprotocol::serial::Span< T >::end() const noexcept
```

#### `operator[]`

```cpp
reference mcprotocol::serial::Span< T >::operator[](size_type index) const noexcept
```

Returns one element; index < size() is a caller precondition.

#### `try_at`

```cpp
pointer mcprotocol::serial::Span< T >::try_at(size_type index) const noexcept
```

Returns a pointer to one element, or null when index is outside the view.

#### `data`

```cpp
pointer mcprotocol::serial::Span< T >::data() const noexcept
```

#### `size`

```cpp
size_type mcprotocol::serial::Span< T >::size() const noexcept
```

#### `empty`

```cpp
bool mcprotocol::serial::Span< T >::empty() const noexcept
```

#### `try_first`

```cpp
bool mcprotocol::serial::Span< T >::try_first(size_type count, Span &out) const noexcept
```

Returns the first count elements through out, or false without an invalid view.

#### `try_subspan`

```cpp
bool mcprotocol::serial::Span< T >::try_subspan(size_type offset, Span &out) const noexcept
```

Returns the suffix at offset through out, or false when offset exceeds size.

#### `try_subspan`

```cpp
bool mcprotocol::serial::Span< T >::try_subspan(size_type offset, size_type count, Span &out) const noexcept
```

Returns the checked [offset, offset + count) range through out.

### Class `mcprotocol::serial::C34PcTarget`

Mandatory typed PC target for 3C/4C multidrop routes.

#### Member Functions

#### `number`

```cpp
static C34PcTarget mcprotocol::serial::C34PcTarget::number(std::uint32_t value) noexcept
```

#### `control_system`

```cpp
static C34PcTarget mcprotocol::serial::C34PcTarget::control_system() noexcept
```

#### `standby_system`

```cpp
static C34PcTarget mcprotocol::serial::C34PcTarget::standby_system() noexcept
```

#### `special_fe`

```cpp
static C34PcTarget mcprotocol::serial::C34PcTarget::special_fe() noexcept
```

#### `connected_station`

```cpp
static C34PcTarget mcprotocol::serial::C34PcTarget::connected_station() noexcept
```

#### `kind`

```cpp
C34PcTargetKind mcprotocol::serial::C34PcTarget::kind() const noexcept
```

#### `value`

```cpp
std::uint32_t mcprotocol::serial::C34PcTarget::value() const noexcept
```

#### `is_valid`

```cpp
bool mcprotocol::serial::C34PcTarget::is_valid() const noexcept
```

### Class `mcprotocol::serial::C4DestinationModule`

Mandatory typed request-destination module for a 4C multidrop route.

#### Member Functions

#### `own_station`

```cpp
static C4DestinationModule mcprotocol::serial::C4DestinationModule::own_station() noexcept
```

#### `multiple_cpu`

```cpp
static C4DestinationModule mcprotocol::serial::C4DestinationModule::multiple_cpu(std::uint32_t cpu_number) noexcept
```

#### `redundant_control_system_cpu`

```cpp
static C4DestinationModule mcprotocol::serial::C4DestinationModule::redundant_control_system_cpu() noexcept
```

#### `redundant_standby_system_cpu`

```cpp
static C4DestinationModule mcprotocol::serial::C4DestinationModule::redundant_standby_system_cpu() noexcept
```

#### `redundant_system_a_cpu`

```cpp
static C4DestinationModule mcprotocol::serial::C4DestinationModule::redundant_system_a_cpu() noexcept
```

#### `redundant_system_b_cpu`

```cpp
static C4DestinationModule mcprotocol::serial::C4DestinationModule::redundant_system_b_cpu() noexcept
```

#### `explicit_target`

```cpp
static C4DestinationModule mcprotocol::serial::C4DestinationModule::explicit_target(std::uint32_t io_number, std::uint32_t station_number) noexcept
```

#### `kind`

```cpp
C4DestinationModuleKind mcprotocol::serial::C4DestinationModule::kind() const noexcept
```

#### `io_number`

```cpp
std::uint32_t mcprotocol::serial::C4DestinationModule::io_number() const noexcept
```

#### `station_number`

```cpp
std::uint32_t mcprotocol::serial::C4DestinationModule::station_number() const noexcept
```

#### `is_own_station_selector`

```cpp
bool mcprotocol::serial::C4DestinationModule::is_own_station_selector() const noexcept
```

#### `is_valid`

```cpp
bool mcprotocol::serial::C4DestinationModule::is_valid() const noexcept
```

### Class `mcprotocol::serial::SelfStationNo`

Mandatory request-source station number for an m:n multidrop route.

#### Member Functions

#### `number`

```cpp
static SelfStationNo mcprotocol::serial::SelfStationNo::number(std::uint32_t value) noexcept
```

#### `value`

```cpp
std::uint32_t mcprotocol::serial::SelfStationNo::value() const noexcept
```

#### `is_valid`

```cpp
bool mcprotocol::serial::SelfStationNo::is_valid() const noexcept
```

### Class `mcprotocol::serial::C1MultidropRoute`

Explicit 1C multidrop route. Network and self-station fields do not exist on this type.

#### Member Functions

#### `C1MultidropRoute`

```cpp
mcprotocol::serial::C1MultidropRoute::C1MultidropRoute(std::uint32_t station_no) noexcept
```

#### `station_no`

```cpp
std::uint32_t mcprotocol::serial::C1MultidropRoute::station_no() const noexcept
```

### Class `mcprotocol::serial::C2StandardMultidropRoute`

Explicit 2C normal/1:n multidrop route. Self-station is fixed to zero.

#### Member Functions

#### `C2StandardMultidropRoute`

```cpp
mcprotocol::serial::C2StandardMultidropRoute::C2StandardMultidropRoute(std::uint32_t station_no) noexcept
```

#### `station_no`

```cpp
std::uint32_t mcprotocol::serial::C2StandardMultidropRoute::station_no() const noexcept
```

### Class `mcprotocol::serial::C2MnMultidropRoute`

Explicit 2C m:n multidrop route with a mandatory self-station number.

#### Member Functions

#### `C2MnMultidropRoute`

```cpp
mcprotocol::serial::C2MnMultidropRoute::C2MnMultidropRoute(std::uint32_t station_no, SelfStationNo self_station_no) noexcept
```

#### `station_no`

```cpp
std::uint32_t mcprotocol::serial::C2MnMultidropRoute::station_no() const noexcept
```

#### `self_station_no`

```cpp
SelfStationNo mcprotocol::serial::C2MnMultidropRoute::self_station_no() const noexcept
```

### Class `mcprotocol::serial::C3StandardMultidropRoute`

Explicit 3C normal/1:n multidrop route. Self-station is fixed to zero.

#### Member Functions

#### `C3StandardMultidropRoute`

```cpp
mcprotocol::serial::C3StandardMultidropRoute::C3StandardMultidropRoute(std::uint32_t station_no, std::uint32_t network_no, C34PcTarget pc_target) noexcept
```

#### `station_no`

```cpp
std::uint32_t mcprotocol::serial::C3StandardMultidropRoute::station_no() const noexcept
```

#### `network_no`

```cpp
std::uint32_t mcprotocol::serial::C3StandardMultidropRoute::network_no() const noexcept
```

#### `pc_target`

```cpp
C34PcTarget mcprotocol::serial::C3StandardMultidropRoute::pc_target() const noexcept
```

### Class `mcprotocol::serial::C3MnMultidropRoute`

Explicit 3C m:n multidrop route with a mandatory self-station number.

#### Member Functions

#### `C3MnMultidropRoute`

```cpp
mcprotocol::serial::C3MnMultidropRoute::C3MnMultidropRoute(std::uint32_t station_no, std::uint32_t network_no, C34PcTarget pc_target, SelfStationNo self_station_no) noexcept
```

#### `station_no`

```cpp
std::uint32_t mcprotocol::serial::C3MnMultidropRoute::station_no() const noexcept
```

#### `network_no`

```cpp
std::uint32_t mcprotocol::serial::C3MnMultidropRoute::network_no() const noexcept
```

#### `pc_target`

```cpp
C34PcTarget mcprotocol::serial::C3MnMultidropRoute::pc_target() const noexcept
```

#### `self_station_no`

```cpp
SelfStationNo mcprotocol::serial::C3MnMultidropRoute::self_station_no() const noexcept
```

### Class `mcprotocol::serial::C4StandardMultidropRoute`

Explicit 4C normal/1:n multidrop route. Self-station is fixed to zero.

#### Member Functions

#### `C4StandardMultidropRoute`

```cpp
mcprotocol::serial::C4StandardMultidropRoute::C4StandardMultidropRoute(std::uint32_t station_no, std::uint32_t network_no, C34PcTarget pc_target, C4DestinationModule destination_module) noexcept
```

#### `station_no`

```cpp
std::uint32_t mcprotocol::serial::C4StandardMultidropRoute::station_no() const noexcept
```

#### `network_no`

```cpp
std::uint32_t mcprotocol::serial::C4StandardMultidropRoute::network_no() const noexcept
```

#### `pc_target`

```cpp
C34PcTarget mcprotocol::serial::C4StandardMultidropRoute::pc_target() const noexcept
```

#### `destination_module`

```cpp
C4DestinationModule mcprotocol::serial::C4StandardMultidropRoute::destination_module() const noexcept
```

### Class `mcprotocol::serial::C4MnMultidropRoute`

Explicit 4C m:n multidrop route with a mandatory self-station number.

#### Member Functions

#### `C4MnMultidropRoute`

```cpp
mcprotocol::serial::C4MnMultidropRoute::C4MnMultidropRoute(std::uint32_t station_no, std::uint32_t network_no, C34PcTarget pc_target, C4DestinationModule destination_module, SelfStationNo self_station_no) noexcept
```

#### `station_no`

```cpp
std::uint32_t mcprotocol::serial::C4MnMultidropRoute::station_no() const noexcept
```

#### `network_no`

```cpp
std::uint32_t mcprotocol::serial::C4MnMultidropRoute::network_no() const noexcept
```

#### `pc_target`

```cpp
C34PcTarget mcprotocol::serial::C4MnMultidropRoute::pc_target() const noexcept
```

#### `destination_module`

```cpp
C4DestinationModule mcprotocol::serial::C4MnMultidropRoute::destination_module() const noexcept
```

#### `self_station_no`

```cpp
SelfStationNo mcprotocol::serial::C4MnMultidropRoute::self_station_no() const noexcept
```

### Class `mcprotocol::serial::RouteConfig`

Explicit route selection for a protocol session.

Default construction represents an omitted route and is rejected before request encoding. Use RouteConfig {HostStationRoute {}} or a frame-specific route type explicitly.

#### Member Functions

#### `RouteConfig`

```cpp
mcprotocol::serial::RouteConfig::RouteConfig() noexcept=default
```

#### `RouteConfig`

```cpp
mcprotocol::serial::RouteConfig::RouteConfig(HostStationRoute) noexcept
```

#### `RouteConfig`

```cpp
mcprotocol::serial::RouteConfig::RouteConfig(C1MultidropRoute route) noexcept
```

#### `RouteConfig`

```cpp
mcprotocol::serial::RouteConfig::RouteConfig(C2StandardMultidropRoute route) noexcept
```

#### `RouteConfig`

```cpp
mcprotocol::serial::RouteConfig::RouteConfig(C2MnMultidropRoute route) noexcept
```

#### `RouteConfig`

```cpp
mcprotocol::serial::RouteConfig::RouteConfig(C3StandardMultidropRoute route) noexcept
```

#### `RouteConfig`

```cpp
mcprotocol::serial::RouteConfig::RouteConfig(C3MnMultidropRoute route) noexcept
```

#### `RouteConfig`

```cpp
mcprotocol::serial::RouteConfig::RouteConfig(C4StandardMultidropRoute route) noexcept
```

#### `RouteConfig`

```cpp
mcprotocol::serial::RouteConfig::RouteConfig(C4MnMultidropRoute route) noexcept
```

#### `kind`

```cpp
RouteKind mcprotocol::serial::RouteConfig::kind() const noexcept
```

#### `is_specified`

```cpp
bool mcprotocol::serial::RouteConfig::is_specified() const noexcept
```

#### `is_host_station`

```cpp
bool mcprotocol::serial::RouteConfig::is_host_station() const noexcept
```

#### `is_multidrop`

```cpp
bool mcprotocol::serial::RouteConfig::is_multidrop() const noexcept
```

#### `supports_frame`

```cpp
bool mcprotocol::serial::RouteConfig::supports_frame(FrameKind frame_kind) const noexcept
```

#### `station_no`

```cpp
std::uint32_t mcprotocol::serial::RouteConfig::station_no() const noexcept
```

#### `network_no`

```cpp
std::uint32_t mcprotocol::serial::RouteConfig::network_no() const noexcept
```

#### `pc_no`

```cpp
std::uint32_t mcprotocol::serial::RouteConfig::pc_no() const noexcept
```

#### `pc_target_valid`

```cpp
bool mcprotocol::serial::RouteConfig::pc_target_valid() const noexcept
```

#### `request_destination_module_io_no`

```cpp
std::uint32_t mcprotocol::serial::RouteConfig::request_destination_module_io_no() const noexcept
```

#### `request_destination_module_station_no`

```cpp
std::uint32_t mcprotocol::serial::RouteConfig::request_destination_module_station_no() const noexcept
```

#### `destination_module_valid`

```cpp
bool mcprotocol::serial::RouteConfig::destination_module_valid() const noexcept
```

#### `destination_module_is_own_station`

```cpp
bool mcprotocol::serial::RouteConfig::destination_module_is_own_station() const noexcept
```

#### `is_mn_multidrop`

```cpp
bool mcprotocol::serial::RouteConfig::is_mn_multidrop() const noexcept
```

#### `self_station_no`

```cpp
std::uint32_t mcprotocol::serial::RouteConfig::self_station_no() const noexcept
```

#### `self_station_valid`

```cpp
bool mcprotocol::serial::RouteConfig::self_station_valid() const noexcept
```

### Class `mcprotocol::serial::ProtocolConfig`

Immutable tagged protocol configuration shared by codecs and client requests.

Treat this as the immutable session configuration for one serial link. The same object is used by:

- FrameCodec for frame wrapping and response decoding - CommandCodec for command subcommand/device-layout differences - MelsecSerialClient and HostSyncClient for runtime request execution

#### Member Functions

#### `ProtocolConfig`

```cpp
mcprotocol::serial::ProtocolConfig::ProtocolConfig()=delete
```

#### `c4_binary`

```cpp
static ProtocolConfig mcprotocol::serial::ProtocolConfig::c4_binary(PlcProfile plc_profile, SumCheckMode sum_check_mode, RouteConfig route, TimeoutConfig timeout={}) noexcept
```

Constructs an explicit C4 Binary/Format5 session.

#### `ascii`

```cpp
static ProtocolConfig mcprotocol::serial::ProtocolConfig::ascii(AsciiFrameKind ascii_frame_kind, AsciiFormat ascii_format, PlcProfile plc_profile, SumCheckMode sum_check_mode, RouteConfig route, TimeoutConfig timeout={}) noexcept
```

Constructs an explicit ASCII session with a mandatory framing format.

#### `frame_kind`

```cpp
FrameKind mcprotocol::serial::ProtocolConfig::frame_kind() const noexcept
```

#### `code_mode`

```cpp
CodeMode mcprotocol::serial::ProtocolConfig::code_mode() const noexcept
```

#### `has_ascii_format`

```cpp
bool mcprotocol::serial::ProtocolConfig::has_ascii_format() const noexcept
```

#### `ascii_format`

```cpp
AsciiFormat mcprotocol::serial::ProtocolConfig::ascii_format() const noexcept
```

#### `plc_profile`

```cpp
PlcProfile mcprotocol::serial::ProtocolConfig::plc_profile() const noexcept
```

#### `sum_check_mode`

```cpp
SumCheckMode mcprotocol::serial::ProtocolConfig::sum_check_mode() const noexcept
```

#### `route`

```cpp
const RouteConfig & mcprotocol::serial::ProtocolConfig::route() const noexcept
```

#### `timeout`

```cpp
const TimeoutConfig & mcprotocol::serial::ProtocolConfig::timeout() const noexcept
```

#### `with_plc_profile`

```cpp
ProtocolConfig mcprotocol::serial::ProtocolConfig::with_plc_profile(PlcProfile value) const noexcept
```

Returns a new immutable session configuration with a different explicit profile.

#### `with_route`

```cpp
ProtocolConfig mcprotocol::serial::ProtocolConfig::with_route(RouteConfig value) const noexcept
```

Returns a new immutable session configuration with a different typed route.

#### `with_timeout`

```cpp
ProtocolConfig mcprotocol::serial::ProtocolConfig::with_timeout(TimeoutConfig value) const noexcept
```

Returns a new immutable session configuration with different host timeout settings.

#### `with_response_timeout_ms`

```cpp
ProtocolConfig mcprotocol::serial::ProtocolConfig::with_response_timeout_ms(std::uint32_t value) const noexcept
```

#### `with_inter_byte_timeout_ms`

```cpp
ProtocolConfig mcprotocol::serial::ProtocolConfig::with_inter_byte_timeout_ms(std::uint32_t value) const noexcept
```

Returns a new configuration with a different retained-frame inactivity timeout.

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

#### `response_identity_mismatch`

```cpp
bool mcprotocol::serial::DecodeResult::response_identity_mismatch = false
```

True when a complete response belongs to a different Format2 or route identity.

### Struct `mcprotocol::serial::highlevel::RandomReadWordSpec`

String-address spec used to build sparse random-read or monitor requests.

#### Fields

#### `device`

```cpp
std::string_view mcprotocol::serial::highlevel::RandomReadWordSpec::device
```

Plain device string such as D100 selected explicitly as 16-bit.

#### Member Functions

#### `RandomReadWordSpec`

```cpp
mcprotocol::serial::highlevel::RandomReadWordSpec::RandomReadWordSpec()=delete
```

#### `RandomReadWordSpec`

```cpp
mcprotocol::serial::highlevel::RandomReadWordSpec::RandomReadWordSpec(std::string_view target_device) noexcept
```

### Struct `mcprotocol::serial::highlevel::RandomReadDWordSpec`

String-address spec selected explicitly for 32-bit sparse read/monitor access.

#### Fields

#### `device`

```cpp
std::string_view mcprotocol::serial::highlevel::RandomReadDWordSpec::device
```

Plain device string such as D100, LZ0, or LCN10 selected explicitly as 32-bit.

#### Member Functions

#### `RandomReadDWordSpec`

```cpp
mcprotocol::serial::highlevel::RandomReadDWordSpec::RandomReadDWordSpec()=delete
```

#### `RandomReadDWordSpec`

```cpp
mcprotocol::serial::highlevel::RandomReadDWordSpec::RandomReadDWordSpec(std::string_view target_device) noexcept
```

### Struct `mcprotocol::serial::highlevel::RandomWriteWordSpec`

String-address spec used to build sparse random word-write items.

Device and value must be supplied together. Explicit zero is valid.

#### Fields

#### `device`

```cpp
std::string_view mcprotocol::serial::highlevel::RandomWriteWordSpec::device
```

Plain device string such as D100 or LZ0.

#### `value`

```cpp
std::uint16_t mcprotocol::serial::highlevel::RandomWriteWordSpec::value
```

Explicit 16-bit word value written to device.

#### Member Functions

#### `RandomWriteWordSpec`

```cpp
mcprotocol::serial::highlevel::RandomWriteWordSpec::RandomWriteWordSpec()=delete
```

#### `RandomWriteWordSpec`

```cpp
mcprotocol::serial::highlevel::RandomWriteWordSpec::RandomWriteWordSpec(std::string_view target_device, std::uint16_t write_value) noexcept
```

### Struct `mcprotocol::serial::highlevel::RandomWriteDWordSpec`

String-address spec used to build an explicit double-word sparse write item.

Device and value must be supplied together. Explicit zero is valid.

#### Fields

#### `device`

```cpp
std::string_view mcprotocol::serial::highlevel::RandomWriteDWordSpec::device
```

Plain device string such as D100 or LZ0.

#### `value`

```cpp
std::uint32_t mcprotocol::serial::highlevel::RandomWriteDWordSpec::value
```

Explicit 32-bit double-word value written to device.

#### Member Functions

#### `RandomWriteDWordSpec`

```cpp
mcprotocol::serial::highlevel::RandomWriteDWordSpec::RandomWriteDWordSpec()=delete
```

#### `RandomWriteDWordSpec`

```cpp
mcprotocol::serial::highlevel::RandomWriteDWordSpec::RandomWriteDWordSpec(std::string_view target_device, std::uint32_t write_value) noexcept
```

### Struct `mcprotocol::serial::highlevel::RandomWriteBitSpec`

String-address spec used to build sparse random bit-write items.

Device and value must be supplied together. Explicit Off is valid.

#### Fields

#### `device`

```cpp
std::string_view mcprotocol::serial::highlevel::RandomWriteBitSpec::device
```

Plain bit-device string such as M100 or X10.

#### `value`

```cpp
BitValue mcprotocol::serial::highlevel::RandomWriteBitSpec::value
```

Bit value written to device.

#### Member Functions

#### `RandomWriteBitSpec`

```cpp
mcprotocol::serial::highlevel::RandomWriteBitSpec::RandomWriteBitSpec()=delete
```

#### `RandomWriteBitSpec`

```cpp
mcprotocol::serial::highlevel::RandomWriteBitSpec::RandomWriteBitSpec(std::string_view target_device, BitValue write_value) noexcept
```

### Struct `mcprotocol::serial::highlevel::LongStateReadSpec`

Mapping from a long-family state device to the helper's internal read route.

#### Fields

#### `route`

```cpp
LongStateReadRoute mcprotocol::serial::highlevel::LongStateReadSpec::route
```

Read route used internally by the long-state helper.

#### `base_code`

```cpp
DeviceCode mcprotocol::serial::highlevel::LongStateReadSpec::base_code
```

Base current-value device read with 0401 word access, or direct bit device for DirectBits.

#### `kind`

```cpp
LongStateReadKind mcprotocol::serial::highlevel::LongStateReadSpec::kind
```

Status bit selected from the third word of the block.

#### Member Functions

#### `LongStateReadSpec`

```cpp
mcprotocol::serial::highlevel::LongStateReadSpec::LongStateReadSpec()=delete
```

#### `LongStateReadSpec`

```cpp
mcprotocol::serial::highlevel::LongStateReadSpec::LongStateReadSpec(LongStateReadRoute read_route, DeviceCode target_base_code, LongStateReadKind state_kind) noexcept
```

### Struct `mcprotocol::serial::HostSerialConfig`

Host-side serial-port configuration used by HostSerialPort.

Every constructor argument is required. device_path accepts /dev/... style paths on POSIX systems and COM3 or \\.\COM10 style names on Windows. The referenced device-path text must remain alive while the configuration is used.

#### Fields

#### `device_path`

```cpp
std::string_view mcprotocol::serial::HostSerialConfig::device_path
```

#### `baud_rate`

```cpp
std::uint32_t mcprotocol::serial::HostSerialConfig::baud_rate
```

#### `data_bits`

```cpp
std::uint32_t mcprotocol::serial::HostSerialConfig::data_bits
```

#### `stop_bits`

```cpp
std::uint32_t mcprotocol::serial::HostSerialConfig::stop_bits
```

#### `parity`

```cpp
SerialParity mcprotocol::serial::HostSerialConfig::parity
```

#### `hardware_flow_control`

```cpp
HardwareFlowControl mcprotocol::serial::HostSerialConfig::hardware_flow_control
```

#### Member Functions

#### `HostSerialConfig`

```cpp
mcprotocol::serial::HostSerialConfig::HostSerialConfig(std::string_view device_path_value, std::uint32_t baud_rate_value, std::uint32_t data_bits_value, std::uint32_t stop_bits_value, SerialParity parity_value, HardwareFlowControl hardware_flow_control_value) noexcept
```

#### `HostSerialConfig`

```cpp
mcprotocol::serial::HostSerialConfig::HostSerialConfig()=delete
```

### Struct `mcprotocol::serial::HostSyncClient::CompletionState`

#### Fields

#### `done`

```cpp
bool mcprotocol::serial::HostSyncClient::CompletionState::done = false
```

#### `status`

```cpp
Status mcprotocol::serial::HostSyncClient::CompletionState::status {}
```

### Struct `mcprotocol::serial::LinkDirectDevice`

Parsed Jn\\... link-direct device reference such as J1\\W100.

#### Fields

#### `network_number`

```cpp
std::uint16_t mcprotocol::serial::LinkDirectDevice::network_number
```

#### `device`

```cpp
DeviceAddress mcprotocol::serial::LinkDirectDevice::device
```

#### Member Functions

#### `LinkDirectDevice`

```cpp
mcprotocol::serial::LinkDirectDevice::LinkDirectDevice()=delete
```

#### `LinkDirectDevice`

```cpp
mcprotocol::serial::LinkDirectDevice::LinkDirectDevice(std::uint16_t target_network_number, DeviceAddress target_device) noexcept
```

### Struct `mcprotocol::serial::LinkDirectRandomReadWordItem`

One sparse Jn\\... item used by native random-read and monitor registration.

#### Fields

#### `device`

```cpp
LinkDirectDevice mcprotocol::serial::LinkDirectRandomReadWordItem::device
```

### Struct `mcprotocol::serial::LinkDirectRandomWriteWordItem`

One sparse Jn\\... word item used by native random word-write.

Device and value must be supplied together. Explicit zero is valid.

#### Fields

#### `device`

```cpp
LinkDirectDevice mcprotocol::serial::LinkDirectRandomWriteWordItem::device
```

#### `value`

```cpp
std::uint16_t mcprotocol::serial::LinkDirectRandomWriteWordItem::value
```

#### Member Functions

#### `LinkDirectRandomWriteWordItem`

```cpp
mcprotocol::serial::LinkDirectRandomWriteWordItem::LinkDirectRandomWriteWordItem()=delete
```

#### `LinkDirectRandomWriteWordItem`

```cpp
mcprotocol::serial::LinkDirectRandomWriteWordItem::LinkDirectRandomWriteWordItem(LinkDirectDevice target_device, std::uint16_t write_value) noexcept
```

### Struct `mcprotocol::serial::LinkDirectRandomWriteBitItem`

One sparse Jn\\... bit item used by native random bit-write.

Device and value must be supplied together. Explicit Off is valid.

#### Fields

#### `device`

```cpp
LinkDirectDevice mcprotocol::serial::LinkDirectRandomWriteBitItem::device
```

#### `value`

```cpp
BitValue mcprotocol::serial::LinkDirectRandomWriteBitItem::value
```

#### Member Functions

#### `LinkDirectRandomWriteBitItem`

```cpp
mcprotocol::serial::LinkDirectRandomWriteBitItem::LinkDirectRandomWriteBitItem()=delete
```

#### `LinkDirectRandomWriteBitItem`

```cpp
mcprotocol::serial::LinkDirectRandomWriteBitItem::LinkDirectRandomWriteBitItem(LinkDirectDevice target_device, BitValue write_value) noexcept
```

### Struct `mcprotocol::serial::LinkDirectMultiBlockReadBlock`

One Jn\\... block used by native multi-block read.

#### Fields

#### `head_device`

```cpp
LinkDirectDevice mcprotocol::serial::LinkDirectMultiBlockReadBlock::head_device
```

#### `points`

```cpp
std::uint16_t mcprotocol::serial::LinkDirectMultiBlockReadBlock::points
```

#### `bit_block`

```cpp
bool mcprotocol::serial::LinkDirectMultiBlockReadBlock::bit_block
```

#### Member Functions

#### `LinkDirectMultiBlockReadBlock`

```cpp
mcprotocol::serial::LinkDirectMultiBlockReadBlock::LinkDirectMultiBlockReadBlock()=delete
```

#### `LinkDirectMultiBlockReadBlock`

```cpp
mcprotocol::serial::LinkDirectMultiBlockReadBlock::LinkDirectMultiBlockReadBlock(LinkDirectDevice first_device, std::uint16_t point_count, bool use_bit_block) noexcept
```

### Struct `mcprotocol::serial::LinkDirectMultiBlockReadRequest`

Jn\\... native multi-block read request.

#### Fields

#### `blocks`

```cpp
mcprotocol::serial::Span<const LinkDirectMultiBlockReadBlock> mcprotocol::serial::LinkDirectMultiBlockReadRequest::blocks
```

#### Member Functions

#### `LinkDirectMultiBlockReadRequest`

```cpp
mcprotocol::serial::LinkDirectMultiBlockReadRequest::LinkDirectMultiBlockReadRequest()=delete
```

#### `LinkDirectMultiBlockReadRequest`

```cpp
mcprotocol::serial::LinkDirectMultiBlockReadRequest::LinkDirectMultiBlockReadRequest(mcprotocol::serial::Span< const LinkDirectMultiBlockReadBlock > request_blocks) noexcept
```

### Struct `mcprotocol::serial::LinkDirectMultiBlockWriteBlock`

One Jn\\... block used by native multi-block write.

#### Fields

#### `head_device`

```cpp
LinkDirectDevice mcprotocol::serial::LinkDirectMultiBlockWriteBlock::head_device
```

#### `points`

```cpp
std::uint16_t mcprotocol::serial::LinkDirectMultiBlockWriteBlock::points
```

#### `bit_block`

```cpp
bool mcprotocol::serial::LinkDirectMultiBlockWriteBlock::bit_block
```

#### `words`

```cpp
mcprotocol::serial::Span<const std::uint16_t> mcprotocol::serial::LinkDirectMultiBlockWriteBlock::words
```

#### `bits`

```cpp
mcprotocol::serial::Span<const BitValue> mcprotocol::serial::LinkDirectMultiBlockWriteBlock::bits
```

#### Member Functions

#### `LinkDirectMultiBlockWriteBlock`

```cpp
mcprotocol::serial::LinkDirectMultiBlockWriteBlock::LinkDirectMultiBlockWriteBlock()=delete
```

#### `LinkDirectMultiBlockWriteBlock`

```cpp
mcprotocol::serial::LinkDirectMultiBlockWriteBlock::LinkDirectMultiBlockWriteBlock(LinkDirectDevice first_device, std::uint16_t point_count, bool use_bit_block, mcprotocol::serial::Span< const std::uint16_t > write_words, mcprotocol::serial::Span< const BitValue > write_bits) noexcept
```

#### `LinkDirectMultiBlockWriteBlock`

```cpp
mcprotocol::serial::LinkDirectMultiBlockWriteBlock::LinkDirectMultiBlockWriteBlock(LinkDirectDevice first_device, std::uint16_t point_count, mcprotocol::serial::Span< const std::uint16_t > write_words) noexcept
```

#### `LinkDirectMultiBlockWriteBlock`

```cpp
mcprotocol::serial::LinkDirectMultiBlockWriteBlock::LinkDirectMultiBlockWriteBlock(LinkDirectDevice first_device, std::uint16_t point_count, mcprotocol::serial::Span< const BitValue > write_bits) noexcept
```

### Struct `mcprotocol::serial::LinkDirectMultiBlockWriteRequest`

Jn\\... native multi-block write request.

#### Fields

#### `blocks`

```cpp
mcprotocol::serial::Span<const LinkDirectMultiBlockWriteBlock> mcprotocol::serial::LinkDirectMultiBlockWriteRequest::blocks
```

#### Member Functions

#### `LinkDirectMultiBlockWriteRequest`

```cpp
mcprotocol::serial::LinkDirectMultiBlockWriteRequest::LinkDirectMultiBlockWriteRequest()=delete
```

#### `LinkDirectMultiBlockWriteRequest`

```cpp
mcprotocol::serial::LinkDirectMultiBlockWriteRequest::LinkDirectMultiBlockWriteRequest(mcprotocol::serial::Span< const LinkDirectMultiBlockWriteBlock > request_blocks) noexcept
```

### Struct `mcprotocol::serial::LinkDirectMonitorRegistration`

Jn\\... monitor registration payload (0801 + device extension specification).

#### Fields

#### `word_items`

```cpp
mcprotocol::serial::Span<const LinkDirectRandomReadWordItem> mcprotocol::serial::LinkDirectMonitorRegistration::word_items
```

#### Member Functions

#### `LinkDirectMonitorRegistration`

```cpp
mcprotocol::serial::LinkDirectMonitorRegistration::LinkDirectMonitorRegistration()=delete
```

#### `LinkDirectMonitorRegistration`

```cpp
mcprotocol::serial::LinkDirectMonitorRegistration::LinkDirectMonitorRegistration(mcprotocol::serial::Span< const LinkDirectRandomReadWordItem > monitor_items) noexcept
```

### Struct `mcprotocol::serial::QualifiedBufferWordDevice`

Parsed U...\\G... or U...\\HG... qualified word device.

#### Fields

#### `kind`

```cpp
QualifiedBufferDeviceKind mcprotocol::serial::QualifiedBufferWordDevice::kind
```

#### `module_number`

```cpp
std::uint16_t mcprotocol::serial::QualifiedBufferWordDevice::module_number
```

#### `word_address`

```cpp
std::uint32_t mcprotocol::serial::QualifiedBufferWordDevice::word_address
```

#### Member Functions

#### `QualifiedBufferWordDevice`

```cpp
mcprotocol::serial::QualifiedBufferWordDevice::QualifiedBufferWordDevice()=delete
```

#### `QualifiedBufferWordDevice`

```cpp
mcprotocol::serial::QualifiedBufferWordDevice::QualifiedBufferWordDevice(QualifiedBufferDeviceKind device_kind, std::uint16_t target_module_number, std::uint32_t target_word_address) noexcept
```

### Struct `mcprotocol::serial::Status`

Result object returned by most public APIs.

plc_error_code is meaningful when code == StatusCode::PlcError. cause records the underlying failure when code == StatusCode::OperationOutcomeUnknown.

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

#### `cause`

```cpp
StatusCode mcprotocol::serial::Status::cause = StatusCode::Ok
```

Machine-readable originating reason when code == OperationOutcomeUnknown.

#### Member Functions

#### `ok`

```cpp
bool mcprotocol::serial::Status::ok() const noexcept
```

### Struct `mcprotocol::serial::TimeoutConfig`

Absolute transaction and retained-frame inactivity timeouts.

#### Fields

#### `response_timeout_ms`

```cpp
std::uint32_t mcprotocol::serial::TimeoutConfig::response_timeout_ms = 3000
```

Maximum complete transaction duration. Partial progress never restarts this limit.

#### `inter_byte_timeout_ms`

```cpp
std::uint32_t mcprotocol::serial::TimeoutConfig::inter_byte_timeout_ms = 250
```

Maximum inactivity after a possible response frame has been retained.

### Struct `mcprotocol::serial::HostStationRoute`

Connected host-station route.

The connected-station header values are protocol constants and therefore are intentionally not exposed as mutable inputs.

### Struct `mcprotocol::serial::DeviceAddress`

Device code plus numeric address.

This is the normalized address form used throughout the library after string-address parsing.

#### Fields

#### `code`

```cpp
DeviceCode mcprotocol::serial::DeviceAddress::code
```

Device family such as D, M, X, LTN, or LZ.

#### `number`

```cpp
std::uint32_t mcprotocol::serial::DeviceAddress::number
```

Numeric index inside the selected device family.

#### Member Functions

#### `DeviceAddress`

```cpp
mcprotocol::serial::DeviceAddress::DeviceAddress()=delete
```

#### `DeviceAddress`

```cpp
mcprotocol::serial::DeviceAddress::DeviceAddress(DeviceCode device_code, std::uint32_t device_number) noexcept
```

### Struct `mcprotocol::serial::ExtendedFileRegisterAddress`

Extended file-register address using block number plus R word number.

This is the block-addressed form used by 1C ACPU-common.

#### Fields

#### `block_number`

```cpp
std::uint16_t mcprotocol::serial::ExtendedFileRegisterAddress::block_number
```

Extended file-register block number.

#### `word_number`

```cpp
std::uint16_t mcprotocol::serial::ExtendedFileRegisterAddress::word_number
```

Word number inside the selected block.

#### Member Functions

#### `ExtendedFileRegisterAddress`

```cpp
mcprotocol::serial::ExtendedFileRegisterAddress::ExtendedFileRegisterAddress()=delete
```

#### `ExtendedFileRegisterAddress`

```cpp
mcprotocol::serial::ExtendedFileRegisterAddress::ExtendedFileRegisterAddress(std::uint16_t target_block_number, std::uint16_t target_word_number) noexcept
```

### Struct `mcprotocol::serial::BatchReadWordsRequest`

#### Fields

#### `head_device`

```cpp
DeviceAddress mcprotocol::serial::BatchReadWordsRequest::head_device
```

First device in the contiguous range.

#### `points`

```cpp
std::uint16_t mcprotocol::serial::BatchReadWordsRequest::points
```

Number of points to read starting at head_device.

#### Member Functions

#### `BatchReadWordsRequest`

```cpp
mcprotocol::serial::BatchReadWordsRequest::BatchReadWordsRequest()=delete
```

#### `BatchReadWordsRequest`

```cpp
mcprotocol::serial::BatchReadWordsRequest::BatchReadWordsRequest(DeviceAddress first_device, std::uint16_t point_count) noexcept
```

### Struct `mcprotocol::serial::BatchReadBitsRequest`

Contiguous bit-read request (0401 bit path).

#### Fields

#### `head_device`

```cpp
DeviceAddress mcprotocol::serial::BatchReadBitsRequest::head_device
```

First bit device in the contiguous range.

#### `points`

```cpp
std::uint16_t mcprotocol::serial::BatchReadBitsRequest::points
```

Number of bit points to read starting at head_device.

#### Member Functions

#### `BatchReadBitsRequest`

```cpp
mcprotocol::serial::BatchReadBitsRequest::BatchReadBitsRequest()=delete
```

#### `BatchReadBitsRequest`

```cpp
mcprotocol::serial::BatchReadBitsRequest::BatchReadBitsRequest(DeviceAddress first_device, std::uint16_t point_count) noexcept
```

### Struct `mcprotocol::serial::BatchWriteWordsRequest`

Contiguous word-write request (1401).

#### Fields

#### `head_device`

```cpp
DeviceAddress mcprotocol::serial::BatchWriteWordsRequest::head_device
```

First device in the contiguous write range.

#### `words`

```cpp
mcprotocol::serial::Span<const std::uint16_t> mcprotocol::serial::BatchWriteWordsRequest::words
```

Caller-owned word data to write starting at head_device.

#### Member Functions

#### `BatchWriteWordsRequest`

```cpp
mcprotocol::serial::BatchWriteWordsRequest::BatchWriteWordsRequest()=delete
```

#### `BatchWriteWordsRequest`

```cpp
mcprotocol::serial::BatchWriteWordsRequest::BatchWriteWordsRequest(DeviceAddress first_device, mcprotocol::serial::Span< const std::uint16_t > write_words) noexcept
```

### Struct `mcprotocol::serial::BatchWriteBitsRequest`

Contiguous bit-write request (1401 bit path).

#### Fields

#### `head_device`

```cpp
DeviceAddress mcprotocol::serial::BatchWriteBitsRequest::head_device
```

First bit device in the contiguous write range.

#### `bits`

```cpp
mcprotocol::serial::Span<const BitValue> mcprotocol::serial::BatchWriteBitsRequest::bits
```

Caller-owned bit data to write starting at head_device.

#### Member Functions

#### `BatchWriteBitsRequest`

```cpp
mcprotocol::serial::BatchWriteBitsRequest::BatchWriteBitsRequest()=delete
```

#### `BatchWriteBitsRequest`

```cpp
mcprotocol::serial::BatchWriteBitsRequest::BatchWriteBitsRequest(DeviceAddress first_device, mcprotocol::serial::Span< const BitValue > write_bits) noexcept
```

### Struct `mcprotocol::serial::ExtendedFileRegisterBatchReadWordsRequest`

#### Fields

#### `head_device`

```cpp
ExtendedFileRegisterAddress mcprotocol::serial::ExtendedFileRegisterBatchReadWordsRequest::head_device
```

First block-addressed file-register word to read.

#### `points`

```cpp
std::uint16_t mcprotocol::serial::ExtendedFileRegisterBatchReadWordsRequest::points
```

Number of words to read from the file-register range.

#### Member Functions

#### `ExtendedFileRegisterBatchReadWordsRequest`

```cpp
mcprotocol::serial::ExtendedFileRegisterBatchReadWordsRequest::ExtendedFileRegisterBatchReadWordsRequest()=delete
```

#### `ExtendedFileRegisterBatchReadWordsRequest`

```cpp
mcprotocol::serial::ExtendedFileRegisterBatchReadWordsRequest::ExtendedFileRegisterBatchReadWordsRequest(ExtendedFileRegisterAddress first_device, std::uint16_t point_count) noexcept
```

### Struct `mcprotocol::serial::ExtendedFileRegisterDirectBatchReadWordsRequest`

Direct extended file-register batch read (NR on 1C AnA/AnUCPU common).

#### Fields

#### `head_device_number`

```cpp
std::uint32_t mcprotocol::serial::ExtendedFileRegisterDirectBatchReadWordsRequest::head_device_number
```

NR/NW direct address on 1C.

#### `points`

```cpp
std::uint16_t mcprotocol::serial::ExtendedFileRegisterDirectBatchReadWordsRequest::points
```

Number of words to read from the direct file-register range.

#### Member Functions

#### `ExtendedFileRegisterDirectBatchReadWordsRequest`

```cpp
mcprotocol::serial::ExtendedFileRegisterDirectBatchReadWordsRequest::ExtendedFileRegisterDirectBatchReadWordsRequest()=delete
```

#### `ExtendedFileRegisterDirectBatchReadWordsRequest`

```cpp
mcprotocol::serial::ExtendedFileRegisterDirectBatchReadWordsRequest::ExtendedFileRegisterDirectBatchReadWordsRequest(std::uint32_t first_device_number, std::uint16_t point_count) noexcept
```

### Struct `mcprotocol::serial::ExtendedFileRegisterBatchWriteWordsRequest`

Extended file-register batch write (EW on 1C ACPU-common).

#### Fields

#### `head_device`

```cpp
ExtendedFileRegisterAddress mcprotocol::serial::ExtendedFileRegisterBatchWriteWordsRequest::head_device
```

First block-addressed file-register word to write.

#### `words`

```cpp
mcprotocol::serial::Span<const std::uint16_t> mcprotocol::serial::ExtendedFileRegisterBatchWriteWordsRequest::words
```

Caller-owned word data to write starting at head_device.

#### Member Functions

#### `ExtendedFileRegisterBatchWriteWordsRequest`

```cpp
mcprotocol::serial::ExtendedFileRegisterBatchWriteWordsRequest::ExtendedFileRegisterBatchWriteWordsRequest()=delete
```

#### `ExtendedFileRegisterBatchWriteWordsRequest`

```cpp
mcprotocol::serial::ExtendedFileRegisterBatchWriteWordsRequest::ExtendedFileRegisterBatchWriteWordsRequest(ExtendedFileRegisterAddress first_device, mcprotocol::serial::Span< const std::uint16_t > write_words) noexcept
```

### Struct `mcprotocol::serial::ExtendedFileRegisterDirectBatchWriteWordsRequest`

Direct extended file-register batch write (NW on 1C AnA/AnUCPU common).

#### Fields

#### `head_device_number`

```cpp
std::uint32_t mcprotocol::serial::ExtendedFileRegisterDirectBatchWriteWordsRequest::head_device_number
```

NR/NW direct address on 1C.

#### `words`

```cpp
mcprotocol::serial::Span<const std::uint16_t> mcprotocol::serial::ExtendedFileRegisterDirectBatchWriteWordsRequest::words
```

Caller-owned word data to write starting at head_device_number.

#### Member Functions

#### `ExtendedFileRegisterDirectBatchWriteWordsRequest`

```cpp
mcprotocol::serial::ExtendedFileRegisterDirectBatchWriteWordsRequest::ExtendedFileRegisterDirectBatchWriteWordsRequest()=delete
```

#### `ExtendedFileRegisterDirectBatchWriteWordsRequest`

```cpp
mcprotocol::serial::ExtendedFileRegisterDirectBatchWriteWordsRequest::ExtendedFileRegisterDirectBatchWriteWordsRequest(std::uint32_t first_device_number, mcprotocol::serial::Span< const std::uint16_t > write_words) noexcept
```

### Struct `mcprotocol::serial::ExtendedFileRegisterRandomWriteWordItem`

One item inside extended file-register random write (ET on 1C).

#### Fields

#### `device`

```cpp
ExtendedFileRegisterAddress mcprotocol::serial::ExtendedFileRegisterRandomWriteWordItem::device
```

Target extended file-register address.

#### `value`

```cpp
std::uint16_t mcprotocol::serial::ExtendedFileRegisterRandomWriteWordItem::value
```

One word written to device.

#### Member Functions

#### `ExtendedFileRegisterRandomWriteWordItem`

```cpp
mcprotocol::serial::ExtendedFileRegisterRandomWriteWordItem::ExtendedFileRegisterRandomWriteWordItem()=delete
```

#### `ExtendedFileRegisterRandomWriteWordItem`

```cpp
mcprotocol::serial::ExtendedFileRegisterRandomWriteWordItem::ExtendedFileRegisterRandomWriteWordItem(ExtendedFileRegisterAddress target_device, std::uint16_t write_value) noexcept
```

### Struct `mcprotocol::serial::ExtendedFileRegisterMonitorRegistration`

Extended file-register monitor registration (EM on 1C).

#### Fields

#### `items`

```cpp
mcprotocol::serial::Span<const ExtendedFileRegisterAddress> mcprotocol::serial::ExtendedFileRegisterMonitorRegistration::items
```

Sparse list of block-addressed file-register items to register for monitoring.

#### Member Functions

#### `ExtendedFileRegisterMonitorRegistration`

```cpp
mcprotocol::serial::ExtendedFileRegisterMonitorRegistration::ExtendedFileRegisterMonitorRegistration()=delete
```

#### `ExtendedFileRegisterMonitorRegistration`

```cpp
mcprotocol::serial::ExtendedFileRegisterMonitorRegistration::ExtendedFileRegisterMonitorRegistration(mcprotocol::serial::Span< const ExtendedFileRegisterAddress > monitor_items) noexcept
```

### Struct `mcprotocol::serial::RandomReadWordItem`

#### Fields

#### `device`

```cpp
DeviceAddress mcprotocol::serial::RandomReadWordItem::device
```

Target device address read as one 16-bit word (bit devices return a 16-point mask word).

### Struct `mcprotocol::serial::RandomReadDWordItem`

One explicitly double-word-width item in a native random-read or monitor request.

#### Fields

#### `device`

```cpp
DeviceAddress mcprotocol::serial::RandomReadDWordItem::device
```

Target device address read as one 32-bit double word.

### Struct `mcprotocol::serial::RandomReadRequest`

Native random-read request with separate word and double-word domains.

#### Fields

#### `word_items`

```cpp
mcprotocol::serial::Span<const RandomReadWordItem> mcprotocol::serial::RandomReadRequest::word_items
```

Sparse 16-bit items, encoded first and returned through the word output span.

#### `dword_items`

```cpp
mcprotocol::serial::Span<const RandomReadDWordItem> mcprotocol::serial::RandomReadRequest::dword_items
```

Sparse 32-bit items, encoded second and returned through the dword output span.

#### Member Functions

#### `RandomReadRequest`

```cpp
mcprotocol::serial::RandomReadRequest::RandomReadRequest()=delete
```

#### `RandomReadRequest`

```cpp
mcprotocol::serial::RandomReadRequest::RandomReadRequest(mcprotocol::serial::Span< const RandomReadWordItem > words, mcprotocol::serial::Span< const RandomReadDWordItem > dwords) noexcept
```

### Struct `mcprotocol::serial::RandomWriteWordItem`

One explicitly word-width item inside native random write (1402 word path).

Device and value are a single construction boundary. Explicit zero is valid; omission is not.

#### Fields

#### `device`

```cpp
DeviceAddress mcprotocol::serial::RandomWriteWordItem::device
```

Target device address for the sparse write.

#### `value`

```cpp
std::uint16_t mcprotocol::serial::RandomWriteWordItem::value
```

One 16-bit word value to write.

#### Member Functions

#### `RandomWriteWordItem`

```cpp
mcprotocol::serial::RandomWriteWordItem::RandomWriteWordItem()=delete
```

#### `RandomWriteWordItem`

```cpp
mcprotocol::serial::RandomWriteWordItem::RandomWriteWordItem(DeviceAddress target_device, std::uint16_t write_value) noexcept
```

### Struct `mcprotocol::serial::RandomWriteDWordItem`

One explicitly double-word-width item inside native random write (1402 word path).

Device and value are a single construction boundary. Explicit zero is valid; omission is not.

#### Fields

#### `device`

```cpp
DeviceAddress mcprotocol::serial::RandomWriteDWordItem::device
```

Target device address for the sparse write.

#### `value`

```cpp
std::uint32_t mcprotocol::serial::RandomWriteDWordItem::value
```

One 32-bit double-word value to write.

#### Member Functions

#### `RandomWriteDWordItem`

```cpp
mcprotocol::serial::RandomWriteDWordItem::RandomWriteDWordItem()=delete
```

#### `RandomWriteDWordItem`

```cpp
mcprotocol::serial::RandomWriteDWordItem::RandomWriteDWordItem(DeviceAddress target_device, std::uint32_t write_value) noexcept
```

### Struct `mcprotocol::serial::RandomWriteBitItem`

One bit item inside native random write (1402 bit path).

Device and value are a single construction boundary. Explicit Off is valid; omission and unknown enum values are rejected.

#### Fields

#### `device`

```cpp
DeviceAddress mcprotocol::serial::RandomWriteBitItem::device
```

Target bit device address for the sparse write.

#### `value`

```cpp
BitValue mcprotocol::serial::RandomWriteBitItem::value
```

Bit value written to device.

#### Member Functions

#### `RandomWriteBitItem`

```cpp
mcprotocol::serial::RandomWriteBitItem::RandomWriteBitItem()=delete
```

#### `RandomWriteBitItem`

```cpp
mcprotocol::serial::RandomWriteBitItem::RandomWriteBitItem(DeviceAddress target_device, BitValue write_value) noexcept
```

### Struct `mcprotocol::serial::MultiBlockReadBlock`

One block inside native multi-block read (0406).

#### Fields

#### `head_device`

```cpp
DeviceAddress mcprotocol::serial::MultiBlockReadBlock::head_device
```

First device in this contiguous block.

#### `points`

```cpp
std::uint16_t mcprotocol::serial::MultiBlockReadBlock::points
```

Number of points in this block.

#### `bit_block`

```cpp
bool mcprotocol::serial::MultiBlockReadBlock::bit_block
```

true for bit blocks, false for word blocks.

#### Member Functions

#### `MultiBlockReadBlock`

```cpp
mcprotocol::serial::MultiBlockReadBlock::MultiBlockReadBlock()=delete
```

#### `MultiBlockReadBlock`

```cpp
mcprotocol::serial::MultiBlockReadBlock::MultiBlockReadBlock(DeviceAddress first_device, std::uint16_t point_count, bool use_bit_block) noexcept
```

### Struct `mcprotocol::serial::MultiBlockReadRequest`

Native multi-block read request composed of multiple contiguous blocks.

#### Fields

#### `blocks`

```cpp
mcprotocol::serial::Span<const MultiBlockReadBlock> mcprotocol::serial::MultiBlockReadRequest::blocks
```

Ordered block list encoded into the native multi-block read request.

#### Member Functions

#### `MultiBlockReadRequest`

```cpp
mcprotocol::serial::MultiBlockReadRequest::MultiBlockReadRequest()=delete
```

#### `MultiBlockReadRequest`

```cpp
mcprotocol::serial::MultiBlockReadRequest::MultiBlockReadRequest(mcprotocol::serial::Span< const MultiBlockReadBlock > request_blocks) noexcept
```

### Struct `mcprotocol::serial::MultiBlockWriteBlock`

One block inside native multi-block write (1406).

#### Fields

#### `head_device`

```cpp
DeviceAddress mcprotocol::serial::MultiBlockWriteBlock::head_device
```

First device in this contiguous block.

#### `points`

```cpp
std::uint16_t mcprotocol::serial::MultiBlockWriteBlock::points
```

Point count for this block.

#### `bit_block`

```cpp
bool mcprotocol::serial::MultiBlockWriteBlock::bit_block
```

true when bits is used, false when words is used.

#### `words`

```cpp
mcprotocol::serial::Span<const std::uint16_t> mcprotocol::serial::MultiBlockWriteBlock::words
```

Caller-owned word data for word blocks.

#### `bits`

```cpp
mcprotocol::serial::Span<const BitValue> mcprotocol::serial::MultiBlockWriteBlock::bits
```

Caller-owned bit data for bit blocks.

#### Member Functions

#### `MultiBlockWriteBlock`

```cpp
mcprotocol::serial::MultiBlockWriteBlock::MultiBlockWriteBlock()=delete
```

#### `MultiBlockWriteBlock`

```cpp
mcprotocol::serial::MultiBlockWriteBlock::MultiBlockWriteBlock(DeviceAddress first_device, std::uint16_t point_count, bool use_bit_block, mcprotocol::serial::Span< const std::uint16_t > write_words, mcprotocol::serial::Span< const BitValue > write_bits) noexcept
```

#### `MultiBlockWriteBlock`

```cpp
mcprotocol::serial::MultiBlockWriteBlock::MultiBlockWriteBlock(DeviceAddress first_device, std::uint16_t point_count, mcprotocol::serial::Span< const std::uint16_t > write_words) noexcept
```

#### `MultiBlockWriteBlock`

```cpp
mcprotocol::serial::MultiBlockWriteBlock::MultiBlockWriteBlock(DeviceAddress first_device, std::uint16_t point_count, mcprotocol::serial::Span< const BitValue > write_bits) noexcept
```

### Struct `mcprotocol::serial::MultiBlockWriteRequest`

Native multi-block write request composed of multiple contiguous blocks.

#### Fields

#### `blocks`

```cpp
mcprotocol::serial::Span<const MultiBlockWriteBlock> mcprotocol::serial::MultiBlockWriteRequest::blocks
```

Ordered block list encoded into the native multi-block write request.

#### Member Functions

#### `MultiBlockWriteRequest`

```cpp
mcprotocol::serial::MultiBlockWriteRequest::MultiBlockWriteRequest()=delete
```

#### `MultiBlockWriteRequest`

```cpp
mcprotocol::serial::MultiBlockWriteRequest::MultiBlockWriteRequest(mcprotocol::serial::Span< const MultiBlockWriteBlock > request_blocks) noexcept
```

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
DeviceAddress mcprotocol::serial::MultiBlockReadBlockResult::head_device {DeviceCode::D, 0U}
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

#### `word_items`

```cpp
mcprotocol::serial::Span<const RandomReadWordItem> mcprotocol::serial::MonitorRegistration::word_items
```

Sparse 16-bit items registered first.

#### `dword_items`

```cpp
mcprotocol::serial::Span<const RandomReadDWordItem> mcprotocol::serial::MonitorRegistration::dword_items
```

Sparse 32-bit items registered second. Unsupported for 1C monitor commands.

#### Member Functions

#### `MonitorRegistration`

```cpp
mcprotocol::serial::MonitorRegistration::MonitorRegistration()=delete
```

#### `MonitorRegistration`

```cpp
mcprotocol::serial::MonitorRegistration::MonitorRegistration(mcprotocol::serial::Span< const RandomReadWordItem > words, mcprotocol::serial::Span< const RandomReadDWordItem > dwords) noexcept
```

### Struct `mcprotocol::serial::UserFrameRegistrationReadRequest`

#### Fields

#### `frame_no`

```cpp
std::uint16_t mcprotocol::serial::UserFrameRegistrationReadRequest::frame_no
```

User-frame number to read, typically in the documented 0x0000..0x03FF or 0x8001..0x801F ranges.

#### Member Functions

#### `UserFrameRegistrationReadRequest`

```cpp
mcprotocol::serial::UserFrameRegistrationReadRequest::UserFrameRegistrationReadRequest()=delete
```

#### `UserFrameRegistrationReadRequest`

```cpp
mcprotocol::serial::UserFrameRegistrationReadRequest::UserFrameRegistrationReadRequest(std::uint16_t target_frame_no) noexcept
```

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
std::array<mcprotocol::serial::Byte, kMaxUserFrameRegistrationBytes> mcprotocol::serial::UserFrameRegistrationData::registration_data {}
```

Raw user-frame registration bytes as returned by the target.

### Struct `mcprotocol::serial::UserFrameRegistrationWriteRequest`

User-frame registration-data write request (1610, subcommand 0000).

#### Fields

#### `frame_no`

```cpp
std::uint16_t mcprotocol::serial::UserFrameRegistrationWriteRequest::frame_no
```

User-frame number to overwrite.

#### `frame_bytes`

```cpp
std::uint16_t mcprotocol::serial::UserFrameRegistrationWriteRequest::frame_bytes
```

Frame-byte count encoded into the 1610 payload.

#### `registration_data`

```cpp
mcprotocol::serial::Span<const mcprotocol::serial::Byte> mcprotocol::serial::UserFrameRegistrationWriteRequest::registration_data
```

Raw user-frame registration bytes to store.

#### Member Functions

#### `UserFrameRegistrationWriteRequest`

```cpp
mcprotocol::serial::UserFrameRegistrationWriteRequest::UserFrameRegistrationWriteRequest()=delete
```

#### `UserFrameRegistrationWriteRequest`

```cpp
mcprotocol::serial::UserFrameRegistrationWriteRequest::UserFrameRegistrationWriteRequest(std::uint16_t target_frame_no, std::uint16_t target_frame_bytes, mcprotocol::serial::Span< const mcprotocol::serial::Byte > target_registration_data) noexcept
```

### Struct `mcprotocol::serial::UserFrameRegistrationDeleteRequest`

User-frame registration-data delete request (1610, subcommand 0001).

#### Fields

#### `frame_no`

```cpp
std::uint16_t mcprotocol::serial::UserFrameRegistrationDeleteRequest::frame_no
```

User-frame number to clear.

#### Member Functions

#### `UserFrameRegistrationDeleteRequest`

```cpp
mcprotocol::serial::UserFrameRegistrationDeleteRequest::UserFrameRegistrationDeleteRequest()=delete
```

#### `UserFrameRegistrationDeleteRequest`

```cpp
mcprotocol::serial::UserFrameRegistrationDeleteRequest::UserFrameRegistrationDeleteRequest(std::uint16_t target_frame_no) noexcept
```

### Struct `mcprotocol::serial::GlobalSignalControlRequest`

C24 global-signal ON/OFF request (1618).

#### Fields

#### `target`

```cpp
GlobalSignalTarget mcprotocol::serial::GlobalSignalControlRequest::target
```

Which global signal destination should be controlled.

#### `value`

```cpp
BitValue mcprotocol::serial::GlobalSignalControlRequest::value
```

Explicit ON/OFF state.

#### Member Functions

#### `GlobalSignalControlRequest`

```cpp
mcprotocol::serial::GlobalSignalControlRequest::GlobalSignalControlRequest()=delete
```

#### `GlobalSignalControlRequest`

```cpp
mcprotocol::serial::GlobalSignalControlRequest::GlobalSignalControlRequest(GlobalSignalTarget signal_target, BitValue signal_value) noexcept
```

### Struct `mcprotocol::serial::SerialModuleModeSwitchRequest`

C24 mode switching request (1612).

The three switch_* flags form the documented switching instruction byte: bit0 = mode number, bit1 = transmission setting, bit2 = communication speed. When a flag is false, the C24 uses the Engineering tool setting for that field.

#### Fields

#### `channel`

```cpp
SerialModuleChannel mcprotocol::serial::SerialModuleModeSwitchRequest::channel
```

Target interface.

#### `switch_mode_no`

```cpp
bool mcprotocol::serial::SerialModuleModeSwitchRequest::switch_mode_no
```

true to use mode_no from this command.

#### `switch_transmission_setting`

```cpp
bool mcprotocol::serial::SerialModuleModeSwitchRequest::switch_transmission_setting
```

true to use transmission_setting from this command.

#### `switch_communication_speed`

```cpp
bool mcprotocol::serial::SerialModuleModeSwitchRequest::switch_communication_speed
```

true to use communication_speed from this command.

#### `mode_no`

```cpp
SerialModuleModeNo mcprotocol::serial::SerialModuleModeSwitchRequest::mode_no
```

Operation mode number. The manual requires a valid non-zero value even when switch_mode_no is false.

#### `transmission_setting`

```cpp
std::uint8_t mcprotocol::serial::SerialModuleModeSwitchRequest::transmission_setting
```

Raw transmission-setting bit field used when switch_transmission_setting is true.

#### `communication_speed`

```cpp
SerialModuleCommunicationSpeed mcprotocol::serial::SerialModuleModeSwitchRequest::communication_speed
```

Communication speed used when switch_communication_speed is true.

#### Member Functions

#### `SerialModuleModeSwitchRequest`

```cpp
mcprotocol::serial::SerialModuleModeSwitchRequest::SerialModuleModeSwitchRequest()=delete
```

#### `SerialModuleModeSwitchRequest`

```cpp
mcprotocol::serial::SerialModuleModeSwitchRequest::SerialModuleModeSwitchRequest(SerialModuleChannel target_channel, bool use_mode_no, bool use_transmission_setting, bool use_communication_speed, SerialModuleModeNo target_mode_no, std::uint8_t target_transmission_setting, SerialModuleCommunicationSpeed target_communication_speed) noexcept
```

### Struct `mcprotocol::serial::HostBufferReadRequest`

#### Fields

#### `start_address`

```cpp
std::uint32_t mcprotocol::serial::HostBufferReadRequest::start_address
```

Starting host-buffer word address.

#### `word_length`

```cpp
std::uint16_t mcprotocol::serial::HostBufferReadRequest::word_length
```

Number of words to read.

#### Member Functions

#### `HostBufferReadRequest`

```cpp
mcprotocol::serial::HostBufferReadRequest::HostBufferReadRequest()=delete
```

#### `HostBufferReadRequest`

```cpp
mcprotocol::serial::HostBufferReadRequest::HostBufferReadRequest(std::uint32_t first_address, std::uint16_t length_words) noexcept
```

### Struct `mcprotocol::serial::HostBufferWriteRequest`

Host-buffer write request (1613).

#### Fields

#### `start_address`

```cpp
std::uint32_t mcprotocol::serial::HostBufferWriteRequest::start_address
```

Starting host-buffer word address.

#### `words`

```cpp
mcprotocol::serial::Span<const std::uint16_t> mcprotocol::serial::HostBufferWriteRequest::words
```

Caller-owned words written sequentially from start_address.

#### Member Functions

#### `HostBufferWriteRequest`

```cpp
mcprotocol::serial::HostBufferWriteRequest::HostBufferWriteRequest()=delete
```

#### `HostBufferWriteRequest`

```cpp
mcprotocol::serial::HostBufferWriteRequest::HostBufferWriteRequest(std::uint32_t first_address, mcprotocol::serial::Span< const std::uint16_t > write_words) noexcept
```

### Struct `mcprotocol::serial::ModuleBufferReadRequest`

Module-buffer byte read request (0601 helper path).

#### Fields

#### `start_address`

```cpp
std::uint32_t mcprotocol::serial::ModuleBufferReadRequest::start_address
```

Starting module-buffer byte address.

#### `bytes`

```cpp
std::uint16_t mcprotocol::serial::ModuleBufferReadRequest::bytes
```

Number of bytes to read.

#### `module_number`

```cpp
std::uint16_t mcprotocol::serial::ModuleBufferReadRequest::module_number
```

Module number used by the addressed special-function module.

#### Member Functions

#### `ModuleBufferReadRequest`

```cpp
mcprotocol::serial::ModuleBufferReadRequest::ModuleBufferReadRequest()=delete
```

#### `ModuleBufferReadRequest`

```cpp
mcprotocol::serial::ModuleBufferReadRequest::ModuleBufferReadRequest(std::uint32_t first_address, std::uint16_t byte_count, std::uint16_t target_module_number) noexcept
```

### Struct `mcprotocol::serial::ModuleBufferWriteRequest`

Module-buffer byte write request (1601 helper path).

#### Fields

#### `start_address`

```cpp
std::uint32_t mcprotocol::serial::ModuleBufferWriteRequest::start_address
```

Starting module-buffer byte address.

#### `module_number`

```cpp
std::uint16_t mcprotocol::serial::ModuleBufferWriteRequest::module_number
```

Module number used by the addressed special-function module.

#### `bytes`

```cpp
mcprotocol::serial::Span<const mcprotocol::serial::Byte> mcprotocol::serial::ModuleBufferWriteRequest::bytes
```

Caller-owned raw bytes written starting at start_address.

#### Member Functions

#### `ModuleBufferWriteRequest`

```cpp
mcprotocol::serial::ModuleBufferWriteRequest::ModuleBufferWriteRequest()=delete
```

#### `ModuleBufferWriteRequest`

```cpp
mcprotocol::serial::ModuleBufferWriteRequest::ModuleBufferWriteRequest(std::uint32_t first_address, std::uint16_t target_module_number, mcprotocol::serial::Span< const mcprotocol::serial::Byte > write_bytes) noexcept
```

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

on_tx_begin and on_tx_end are installed as a pair. Leaving both null disables library-side direction control; user may remain null even when the callback pair is installed.

#### Fields

#### `on_tx_begin`

```cpp
void(* mcprotocol::serial::Rs485Hooks::on_tx_begin) (void *user) = nullptr
```

Callback fired immediately before the client expects TX to start.

#### `on_tx_end`

```cpp
void(* mcprotocol::serial::Rs485Hooks::on_tx_end) (void *user) = nullptr
```

Matching callback fired after physical TX completion or abort is reported.

#### `user`

```cpp
void* mcprotocol::serial::Rs485Hooks::user = nullptr
```

Opaque user pointer passed back to both callbacks.
