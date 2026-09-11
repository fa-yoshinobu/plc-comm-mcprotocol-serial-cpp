#pragma once

#include "mcprotocol/serial/client.hpp"

namespace mcprotocol::serial::detail {

// Transport contract: now_ms(), idle(), ready(), write_some(data,size),
// tx_complete(bool&), read_some(data,size), abort(). All I/O is nonblocking.
// A negative byte count is a transport error. abort() must stop physical TX.
// Kept independent of Arduino so the actual state machine is host-testable.
template <class Transport>
class UartClient {
 public:
  explicit UartClient(Transport& transport) noexcept : transport_(transport) {}
  UartClient(const UartClient&) = delete;
  UartClient& operator=(const UartClient&) = delete;

  [[nodiscard]] Status configure(const ProtocolConfig& protocol) noexcept {
    if (active_ || in_callback_) return busy_status();
    if (!transport_.ready()) return closed_status();
    if (core_.requires_transport_reset())
      return make_status(StatusCode::NotConnected, "Explicit UART recovery is required");
    return core_.configure(protocol);
  }

  // Called by the platform facade only AFTER explicit transport recovery.
  [[nodiscard]] Status configure_after_recovery(const ProtocolConfig& protocol) noexcept {
    if (active_ || in_callback_) return busy_status();
    if (!transport_.ready()) return closed_status();
    return core_.configure(protocol);
  }
  [[nodiscard]] bool busy() const noexcept { return active_ || in_callback_; }
  [[nodiscard]] bool requires_transport_reset() const noexcept {
    return core_.requires_transport_reset() || !transport_.ready();
  }

  [[nodiscard]] Status async_read_words(DeviceAddress address, Span<std::uint16_t> out,
      CompletionHandler callback, void* user = nullptr) noexcept {
    Status s = admit(out.size(), callback, user);
    if (!s.ok()) return s;
    return started(core_.async_batch_read_words(transport_.now_ms(),
        BatchReadWordsRequest(address, static_cast<std::uint16_t>(out.size())),
        out, completed, this));
  }
  [[nodiscard]] Status async_read_bits(DeviceAddress address, Span<bool> out,
      CompletionHandler callback, void* user = nullptr) noexcept {
    Status s = admit(out.size(), callback, user);
    if (!s.ok()) return s;
    return started(core_.async_batch_read_bits(transport_.now_ms(),
        BatchReadBitsRequest(address, static_cast<std::uint16_t>(out.size())),
        out, completed, this));
  }
  [[nodiscard]] Status async_write_words(DeviceAddress address, Span<const std::uint16_t> data,
      CompletionHandler callback, void* user = nullptr) noexcept {
    Status s = admit(data.size(), callback, user);
    if (!s.ok()) return s;
    return started(core_.async_batch_write_words(transport_.now_ms(),
        BatchWriteWordsRequest(address, data), completed, this));
  }
  [[nodiscard]] Status async_write_bits(DeviceAddress address, Span<const BitValue> data,
      CompletionHandler callback, void* user = nullptr) noexcept {
    Status s = admit(data.size(), callback, user);
    if (!s.ok()) return s;
    return started(core_.async_batch_write_bits(transport_.now_ms(),
        BatchWriteBitsRequest(address, data), completed, this));
  }

  [[nodiscard]] Status read_words(DeviceAddress address, Span<std::uint16_t> out) noexcept {
    return wait(async_read_words(address, out, nullptr));
  }
  [[nodiscard]] Status read_word(DeviceAddress address, std::uint16_t& out) noexcept {
    return read_words(address, Span<std::uint16_t>(&out, 1));
  }
  [[nodiscard]] Status read_bits(DeviceAddress address, Span<bool> out) noexcept {
    return wait(async_read_bits(address, out, nullptr));
  }
  [[nodiscard]] Status read_bit(DeviceAddress address, bool& out) noexcept {
    return read_bits(address, Span<bool>(&out, 1));
  }
  [[nodiscard]] Status write_words(DeviceAddress address, Span<const std::uint16_t> data) noexcept {
    return wait(async_write_words(address, data, nullptr));
  }
  [[nodiscard]] Status write_word(DeviceAddress address, std::uint16_t value) noexcept {
    return write_words(address, Span<const std::uint16_t>(&value, 1));
  }
  [[nodiscard]] Status write_bits(DeviceAddress address, Span<const BitValue> data) noexcept {
    return wait(async_write_bits(address, data, nullptr));
  }
  [[nodiscard]] Status write_bit(DeviceAddress address, BitValue value) noexcept {
    return write_bits(address, Span<const BitValue>(&value, 1));
  }

  // Native advanced operations use the core's request validation and buffers.
  [[nodiscard]] Status async_random_read(const RandomReadRequest& request, Span<std::uint16_t> out_words,
      Span<std::uint32_t> out_dwords,
      CompletionHandler callback, void* user = nullptr) noexcept {
    Status s = admit(callback, user);
    if (!s.ok()) return s;
    return started(core_.async_random_read(transport_.now_ms(),
        request, out_words, out_dwords, completed, this));
  }
  [[nodiscard]] Status random_read(const RandomReadRequest& request, Span<std::uint16_t> out_words,
      Span<std::uint32_t> out_dwords) noexcept {
    return wait(async_random_read(request, out_words, out_dwords, nullptr));
  }

  [[nodiscard]] Status async_random_write_words(Span<const RandomWriteWordItem> word_items,
      Span<const RandomWriteDWordItem> dword_items,
      CompletionHandler callback, void* user = nullptr) noexcept {
    Status s = admit(callback, user);
    if (!s.ok()) return s;
    return started(core_.async_random_write_words(transport_.now_ms(),
        word_items, dword_items, completed, this));
  }
  [[nodiscard]] Status random_write_words(Span<const RandomWriteWordItem> word_items,
      Span<const RandomWriteDWordItem> dword_items) noexcept {
    return wait(async_random_write_words(word_items, dword_items, nullptr));
  }

  [[nodiscard]] Status async_random_write_bits(Span<const RandomWriteBitItem> items,
      CompletionHandler callback, void* user = nullptr) noexcept {
    Status s = admit(callback, user);
    if (!s.ok()) return s;
    return started(core_.async_random_write_bits(transport_.now_ms(),
        items, completed, this));
  }
  [[nodiscard]] Status random_write_bits(Span<const RandomWriteBitItem> items) noexcept {
    return wait(async_random_write_bits(items, nullptr));
  }

  [[nodiscard]] Status async_multi_block_read(const MultiBlockReadRequest& request, Span<std::uint16_t> out_words,
      Span<BitValue> out_bits, Span<MultiBlockReadBlockResult> out_results,
      CompletionHandler callback, void* user = nullptr) noexcept {
    Status s = admit(callback, user);
    if (!s.ok()) return s;
    return started(core_.async_multi_block_read(transport_.now_ms(),
        request, out_words, out_bits, out_results, completed, this));
  }
  [[nodiscard]] Status multi_block_read(const MultiBlockReadRequest& request, Span<std::uint16_t> out_words,
      Span<BitValue> out_bits, Span<MultiBlockReadBlockResult> out_results) noexcept {
    return wait(async_multi_block_read(request, out_words, out_bits, out_results, nullptr));
  }

  [[nodiscard]] Status async_multi_block_write(const MultiBlockWriteRequest& request,
      CompletionHandler callback, void* user = nullptr) noexcept {
    Status s = admit(callback, user);
    if (!s.ok()) return s;
    return started(core_.async_multi_block_write(transport_.now_ms(),
        request, completed, this));
  }
  [[nodiscard]] Status multi_block_write(const MultiBlockWriteRequest& request) noexcept {
    return wait(async_multi_block_write(request, nullptr));
  }
  // One bounded TX attempt / RX chunk per call. No callbacks from ISR/task context.
  void update() noexcept {
    if (!active_ || in_callback_) return;
    if (!tx_started_) {
      tx_started_ = true;
      Status s = core_.notify_tx_started(transport_.now_ms());
      if (!s.ok()) {
        if (active_) { core_.cancel(); }
        return;
      }
    }
    core_.poll(transport_.now_ms());
    if (!active_) return;
    if (!tx_finished_) {
      if (static_cast<std::int32_t>(transport_.now_ms() - core_.transaction_deadline_ms()) >= 0) {
        abort_tx(make_status(StatusCode::Timeout, "UART transmission deadline expired"));
        return;
      }
      const auto frame = core_.pending_tx_frame();
      if (tx_offset_ < frame.size()) {
        const int sent = transport_.write_some(
            reinterpret_cast<const std::uint8_t*>(frame.data()) + tx_offset_,
            frame.size() - tx_offset_);
        if (sent < 0 || static_cast<std::size_t>(sent) > frame.size() - tx_offset_) {
          abort_tx(make_status(StatusCode::Transport, "UART write failed"));
          return;
        }
        tx_offset_ += static_cast<std::size_t>(sent);
        if (tx_offset_ < frame.size()) return;
      }
      bool done = false;
      Status s = transport_.tx_complete(done);
      if (!s.ok()) { abort_tx(s); return; }
      if (!done) return;
      tx_finished_ = true;
      s = core_.notify_tx_complete(transport_.now_ms(), ok_status());
      if (!s.ok() || !active_) return;
    }
    std::uint8_t bytes[64];
    const int count = transport_.read_some(bytes, sizeof(bytes));
    if (count < 0 || count > static_cast<int>(sizeof(bytes))) {
      const auto ignored = core_.notify_rx_failure(make_status(StatusCode::Transport, "UART read failed"));
      (void)ignored;
      return;
    }
    if (count > 0) core_.on_rx_bytes(transport_.now_ms(),
        Span<const Byte>(reinterpret_cast<const Byte*>(bytes), static_cast<std::size_t>(count)));
    if (active_) core_.poll(transport_.now_ms());
  }

  void cancel() noexcept {
    if (!active_ || in_callback_) return;
    const bool transmitting = tx_started_ && !tx_finished_;
    core_.cancel();
    if (transmitting && active_) abort_tx(make_status(StatusCode::Cancelled, "UART transmission cancelled"));
  }

 private:
  static Status busy_status() noexcept { return make_status(StatusCode::Busy, "UART client is busy"); }
  static Status closed_status() noexcept { return make_status(StatusCode::Closed, "UART is not open"); }
  Status admit(std::size_t count, CompletionHandler callback, void* user) noexcept {
    if (busy()) return busy_status();
    if (!transport_.ready()) return closed_status();
    if (count == 0 || count > 65535U)
      return make_status(StatusCode::InvalidArgument, "Point count must be 1..65535");
    return admit(callback, user);
  }
  Status admit(CompletionHandler callback, void* user) noexcept {
    if (busy()) return busy_status();
    if (!transport_.ready()) return closed_status();
    callback_ = callback;
    user_ = user;
    active_ = true;
    tx_started_ = false;
    tx_finished_ = false;
    tx_offset_ = 0;
    return ok_status();
  }
  Status started(Status s) noexcept {
    if (!s.ok()) { active_ = false; callback_ = nullptr; user_ = nullptr; }
    return s;
  }
  static void completed(void* user, Status s) noexcept {
    auto& self = *static_cast<UartClient*>(user);
    self.active_ = false;
    self.result_ = s;
    const auto callback = self.callback_;
    self.callback_ = nullptr;
    self.in_callback_ = true;
    if (callback) callback(self.user_, s);
    self.in_callback_ = false;
  }
  void abort_tx(Status s) noexcept {
    transport_.abort(); // stop hardware BEFORE releasing the core's TX ownership
    tx_finished_ = true;
    const auto ignored = core_.notify_tx_complete(transport_.now_ms(), s);
    (void)ignored;
  }
  Status wait(Status s) noexcept {
    if (!s.ok()) return s;
    while (active_) { update(); if (active_) transport_.idle(); }
    return result_;
  }
  Transport& transport_;
  MelsecSerialClient core_;
  CompletionHandler callback_ = nullptr;
  void* user_ = nullptr;
  Status result_ {};
  std::size_t tx_offset_ = 0;
  bool active_ = false;
  bool in_callback_ = false;
  bool tx_started_ = false;
  bool tx_finished_ = false;
};
} // namespace mcprotocol::serial::detail
