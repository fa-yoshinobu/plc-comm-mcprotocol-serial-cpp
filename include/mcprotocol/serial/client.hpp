#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "mcprotocol/serial/codec.hpp"

namespace mcprotocol::serial {

class MelsecSerialClient {
 public:
  MelsecSerialClient() = default;

  [[nodiscard]] Status configure(const ProtocolConfig& config) noexcept;
  void set_rs485_hooks(const Rs485Hooks& hooks) noexcept;

  [[nodiscard]] bool busy() const noexcept;
  [[nodiscard]] std::span<const std::byte> pending_tx_frame() const noexcept;

  [[nodiscard]] Status notify_tx_complete(
      std::uint32_t now_ms,
      Status transport_status = ok_status()) noexcept;

  void on_rx_bytes(std::uint32_t now_ms, std::span<const std::byte> bytes) noexcept;
  void poll(std::uint32_t now_ms) noexcept;
  void cancel() noexcept;

  [[nodiscard]] Status async_batch_read_words(
      std::uint32_t now_ms,
      const BatchReadWordsRequest& request,
      std::span<std::uint16_t> out_words,
      CompletionHandler callback,
      void* user) noexcept;

  [[nodiscard]] Status async_batch_read_bits(
      std::uint32_t now_ms,
      const BatchReadBitsRequest& request,
      std::span<BitValue> out_bits,
      CompletionHandler callback,
      void* user) noexcept;

  [[nodiscard]] Status async_batch_write_words(
      std::uint32_t now_ms,
      const BatchWriteWordsRequest& request,
      CompletionHandler callback,
      void* user) noexcept;

  [[nodiscard]] Status async_batch_write_bits(
      std::uint32_t now_ms,
      const BatchWriteBitsRequest& request,
      CompletionHandler callback,
      void* user) noexcept;

  [[nodiscard]] Status async_random_read(
      std::uint32_t now_ms,
      const RandomReadRequest& request,
      std::span<std::uint32_t> out_values,
      CompletionHandler callback,
      void* user) noexcept;

  [[nodiscard]] Status async_random_write_words(
      std::uint32_t now_ms,
      std::span<const RandomWriteWordItem> items,
      CompletionHandler callback,
      void* user) noexcept;

  [[nodiscard]] Status async_random_write_bits(
      std::uint32_t now_ms,
      std::span<const RandomWriteBitItem> items,
      CompletionHandler callback,
      void* user) noexcept;

  [[nodiscard]] Status async_multi_block_read(
      std::uint32_t now_ms,
      const MultiBlockReadRequest& request,
      std::span<std::uint16_t> out_words,
      std::span<BitValue> out_bits,
      std::span<MultiBlockReadBlockResult> out_results,
      CompletionHandler callback,
      void* user) noexcept;

  [[nodiscard]] Status async_multi_block_write(
      std::uint32_t now_ms,
      const MultiBlockWriteRequest& request,
      CompletionHandler callback,
      void* user) noexcept;

  [[nodiscard]] Status async_register_monitor(
      std::uint32_t now_ms,
      const MonitorRegistration& request,
      CompletionHandler callback,
      void* user) noexcept;

  [[nodiscard]] Status async_read_monitor(
      std::uint32_t now_ms,
      std::span<std::uint32_t> out_values,
      CompletionHandler callback,
      void* user) noexcept;

  [[nodiscard]] Status async_read_host_buffer(
      std::uint32_t now_ms,
      const HostBufferReadRequest& request,
      std::span<std::uint16_t> out_words,
      CompletionHandler callback,
      void* user) noexcept;

  [[nodiscard]] Status async_write_host_buffer(
      std::uint32_t now_ms,
      const HostBufferWriteRequest& request,
      CompletionHandler callback,
      void* user) noexcept;

  [[nodiscard]] Status async_read_module_buffer(
      std::uint32_t now_ms,
      const ModuleBufferReadRequest& request,
      std::span<std::byte> out_bytes,
      CompletionHandler callback,
      void* user) noexcept;

  [[nodiscard]] Status async_write_module_buffer(
      std::uint32_t now_ms,
      const ModuleBufferWriteRequest& request,
      CompletionHandler callback,
      void* user) noexcept;

  [[nodiscard]] Status async_read_cpu_model(
      std::uint32_t now_ms,
      CpuModelInfo& out_info,
      CompletionHandler callback,
      void* user) noexcept;

  [[nodiscard]] Status async_loopback(
      std::uint32_t now_ms,
      std::span<const char> hex_ascii,
      std::span<char> out_echoed,
      CompletionHandler callback,
      void* user) noexcept;

 private:
  enum class OperationKind : std::uint8_t {
    None,
    BatchReadWords,
    BatchReadBits,
    BatchWriteWords,
    BatchWriteBits,
    RandomRead,
    RandomWriteWords,
    RandomWriteBits,
    MultiBlockRead,
    MultiBlockWrite,
    RegisterMonitor,
    ReadMonitor,
    ReadHostBuffer,
    WriteHostBuffer,
    ReadModuleBuffer,
    WriteModuleBuffer,
    ReadCpuModel,
    Loopback
  };

  [[nodiscard]] Status start_request(
      std::uint32_t now_ms,
      OperationKind operation,
      std::size_t request_data_size,
      CompletionHandler callback,
      void* user) noexcept;

  [[nodiscard]] Status handle_response(std::span<const std::uint8_t> response_data) noexcept;
  void complete(Status status) noexcept;
  void clear_pending_outputs() noexcept;
  void clear_pending_copies() noexcept;

  ProtocolConfig config_ {};
  Rs485Hooks rs485_hooks_ {};
  bool configured_ = false;
  bool busy_ = false;
  bool awaiting_write_complete_ = false;
  OperationKind operation_ = OperationKind::None;
  CompletionHandler callback_ = nullptr;
  void* callback_user_ = nullptr;
  std::uint32_t response_deadline_ms_ = 0;
  std::uint32_t inter_byte_deadline_ms_ = 0;

  std::array<std::uint8_t, kMaxRequestFrameBytes> tx_frame_ {};
  std::size_t tx_frame_size_ = 0;
  std::array<std::uint8_t, kMaxResponseFrameBytes> rx_frame_ {};
  std::size_t rx_frame_size_ = 0;
  std::array<std::uint8_t, kMaxRequestDataBytes> request_data_ {};

  BatchReadWordsRequest batch_read_words_request_ {};
  BatchReadBitsRequest batch_read_bits_request_ {};
  HostBufferReadRequest host_buffer_read_request_ {};
  ModuleBufferReadRequest module_buffer_read_request_ {};

  std::span<std::uint16_t> out_words_ {};
  std::span<BitValue> out_bits_ {};
  std::span<std::uint32_t> out_values_ {};
  std::span<std::byte> out_bytes_ {};
  std::span<char> out_chars_ {};
  std::span<MultiBlockReadBlockResult> out_block_results_ {};
  CpuModelInfo* out_cpu_model_ = nullptr;

  std::array<RandomReadItem, kMaxRandomAccessItems> pending_random_items_ {};
  std::size_t pending_random_item_count_ = 0;
  std::array<RandomReadItem, kMaxMonitorItems> monitor_items_ {};
  std::size_t monitor_item_count_ = 0;
  bool monitor_registered_ = false;
  std::array<MultiBlockReadBlock, kMaxMultiBlockCount> pending_multi_blocks_ {};
  std::size_t pending_multi_block_count_ = 0;
  std::array<char, kMaxLoopbackBytes> pending_loopback_ {};
  std::size_t pending_loopback_size_ = 0;
};

}  // namespace mcprotocol::serial
