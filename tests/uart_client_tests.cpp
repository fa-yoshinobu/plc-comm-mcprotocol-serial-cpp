#include <algorithm>
#include <array>
#include <vector>
#include "test_assert.hpp"
#include "mcprotocol/serial/detail/uart_client.hpp"

using namespace mcprotocol::serial;

struct FakeUart {
  std::uint32_t now = 0;
  bool open = true, done = true, fail_write = false, fail_read = false;
  bool fail_done = false;
  int chunk = 3, rx_chunk = 1, aborts = 0;
  std::vector<std::uint8_t> tx, rx;
  std::size_t rx_offset = 0;
  std::uint32_t now_ms() const { return now; }
  void idle() { ++now; }
  bool ready() const { return open; }
  int write_some(const std::uint8_t* bytes, std::size_t size) {
    if (fail_write) return -1;
    const int n = static_cast<int>(std::min(size, static_cast<std::size_t>(chunk)));
    tx.insert(tx.end(), bytes, bytes + n);
    return n;
  }
  Status tx_complete(bool& complete) {
    complete = done;
    return fail_done ? make_status(StatusCode::Transport, "injected") : ok_status();
  }
  int read_some(std::uint8_t* bytes, std::size_t size) {
    if (fail_read) return -1;
    const auto n = std::min({size, rx.size() - rx_offset, static_cast<std::size_t>(rx_chunk)});
    if (n) std::copy_n(rx.data() + rx_offset, n, bytes);
    rx_offset += n;
    return static_cast<int>(n);
  }
  void abort() { open = false; ++aborts; }
};
using Client = detail::UartClient<FakeUart>;
const DeviceAddress d100 {DeviceCode::D, 100};
ProtocolConfig protocol() {
  return ProtocolConfig::c4_binary(PlcProfile::MelsecIqF, SumCheckMode::Enabled,
      RouteConfig{HostStationRoute{}}, TimeoutConfig{100, 20});
}
void response(FakeUart& uart, Span<const std::uint8_t> data, std::uint16_t error = 0) {
  std::array<std::uint8_t, 128> frame {};
  std::size_t size = 0;
  auto s = error ? FrameCodec::encode_error_response(protocol(), error, frame, size)
                : FrameCodec::encode_success_response(protocol(), data, frame, size);
  assert(s.ok());
  uart.rx.assign(frame.begin(), frame.begin() + size);
  uart.rx_offset = 0;
}
struct Completion {
  int count = 0;
  Status result {};
  static void callback(void* user, Status s) {
    auto& self = *static_cast<Completion*>(user);
    ++self.count; self.result = s;
  }
};
void pump(Client& client, FakeUart& uart) {
  for (int i = 0; client.busy() && i < 300; ++i) { client.update(); ++uart.now; }
  assert(!client.busy());
}
void test_read_and_admission() {
  FakeUart uart; Client client(uart); Completion completion;
  assert(client.configure(protocol()).ok());
  const std::uint8_t data[] = {0x34, 0x12};
  response(uart, {data, 2});
  std::uint16_t value = 0, other = 999;
  assert(client.async_read_words(d100, {&value, 1}, Completion::callback, &completion).ok());
  assert(client.read_word(d100, other).code == StatusCode::Busy);
  assert(other == 999);
  assert(client.configure(protocol()).code == StatusCode::Busy);
  pump(client, uart);
  assert(value == 0x1234 && completion.count == 1 && completion.result.ok());
  assert(uart.tx.size() > 3); // exercised multiple partial writes
  client.update(); assert(completion.count == 1);
  assert(!client.requires_transport_reset());
  response(uart, {data, 2});
  assert(client.read_word(d100, value).ok());
  assert(client.async_read_words(d100, Span<std::uint16_t>{}, nullptr).code == StatusCode::InvalidArgument);
  assert(client.async_read_words(d100, {&value, 65536}, nullptr).code == StatusCode::InvalidArgument);
}
void test_failures_and_recovery() {
  FakeUart uart; Client client(uart); Completion completion;
  assert(client.configure(protocol()).ok());
  std::uint16_t value = 777;
  assert(client.async_read_words(d100, {&value, 1}, Completion::callback, &completion).ok());
  pump(client, uart);
  assert(completion.result.code == StatusCode::Timeout && completion.count == 1);
  assert(value == 777 && client.requires_transport_reset());
  assert(!client.configure(protocol()).ok());
  assert(!client.read_word(d100, value).ok());
  // Fake explicit recovery: discard old response, reopen, acknowledge reset.
  uart.abort(); uart.open = true; uart.rx.clear(); uart.rx_offset = 0;
  assert(client.configure_after_recovery(protocol()).ok());
  const std::uint8_t data[] = {1, 0}; response(uart, {data, 2});
  assert(client.read_word(d100, value).ok() && value == 1);
}
void test_tx_deadline_and_cancel() {
  for (int mode = 0; mode < 4; ++mode) {
    FakeUart uart; Client client(uart); Completion completion;
    uart.now = 0xfffffff0U; // deadline crosses millis() wrap
    assert(client.configure(protocol()).ok());
    std::uint16_t value = 0;
    assert(client.async_read_words(d100, {&value, 1}, Completion::callback, &completion).ok());
    if (mode == 0) { client.cancel(); assert(uart.aborts == 0); }
    if (mode == 1) { client.update(); client.cancel(); assert(uart.aborts == 1); }
    if (mode == 2) { uart.done = false; pump(client, uart); assert(uart.aborts == 1); }
    if (mode == 3) { uart.chunk = 0; pump(client, uart); assert(uart.aborts == 1); }
    assert(completion.count == 1);
    assert(completion.result.code == (mode < 2 ? StatusCode::Cancelled : StatusCode::Timeout));
  }
}
void test_transport_errors() {
  for (int mode = 0; mode < 3; ++mode) {
    FakeUart uart; Client client(uart);
    assert(client.configure(protocol()).ok());
    uart.fail_write = mode == 0; uart.fail_done = mode == 1; uart.fail_read = mode == 2;
    std::uint16_t value = 42;
    assert(client.read_word(d100, value).code == StatusCode::Transport);
    assert(client.requires_transport_reset() && value == 42);
  }
}
void test_plc_error_and_writes() {
  FakeUart uart; Client client(uart);
  assert(client.configure(protocol()).ok());
  response(uart, {}, 0xC051);
  std::uint16_t value = 42;
  auto s = client.read_word(d100, value);
  assert(s.code == StatusCode::PlcError && s.plc_error_code == 0xC051 && value == 42);
  response(uart, {});
  assert(client.write_word(d100, 1234).ok());
  uart.rx.clear(); uart.rx_offset = 0;
  assert(client.write_word(d100, 1234).code == StatusCode::OperationOutcomeUnknown);
}
void test_bits_and_inter_byte_timeout() {
  FakeUart uart; Client client(uart);
  assert(client.configure(protocol()).ok());
  const std::uint8_t bit[] = {0x10}; response(uart, {bit, 1});
  bool value = false;
  assert(client.read_bit({DeviceCode::M, 100}, value).ok() && value);
  response(uart, {});
  const BitValue values[] = {true};
  assert(client.write_bits({DeviceCode::M, 100}, {values, 1}).ok());
  const std::uint8_t word[] = {1, 0}; response(uart, {word, 2});
  uart.rx.resize(5); // recognizable frame prefix, then silence
  std::uint16_t out = 123;
  const auto start = uart.now;
  assert(client.read_word(d100, out).code == StatusCode::Timeout);
  assert(uart.now - start < 100 && out == 123);
}
void test_reentrant_callback() {
  FakeUart uart; Client client(uart);
  assert(client.configure(protocol()).ok());
  struct Context { Client* client; std::uint16_t value; Status nested; } context{&client, 0, {}};
  const std::uint8_t data[] = {1, 0}; response(uart, {data, 2});
  assert(client.async_read_words(d100, {&context.value, 1}, [](void* u, Status s) {
    auto& c = *static_cast<Context*>(u); assert(s.ok());
    c.nested = c.client->read_word(d100, c.value);
    c.client->update(); c.client->cancel();
  }, &context).ok());
  pump(client, uart);
  assert(context.nested.code == StatusCode::Busy);
}
void test_advanced_operations() {
  // Compare the complete transmitted frames against the standalone core, then
  // exercise both adapter entry points with fragmented replies.
  for (int operation = 0; operation < 5; ++operation) {
    for (bool synchronous : {false, true}) {
      FakeUart uart; Client client(uart); MelsecSerialClient core; Completion completion;
      assert(client.configure(protocol()).ok() && core.configure(protocol()).ok());
      const RandomReadWordItem reads[] = {{d100}, {{DeviceCode::D, 200}}};
      const RandomReadDWordItem dreads[] = {{{DeviceCode::D, 500}}};
      const RandomReadRequest random_request({reads, 2}, {dreads, 1});
      const RandomWriteWordItem writes[] = {{d100, 123}, {{DeviceCode::D, 200}, 456}};
      const RandomWriteDWordItem dwrites[] = {{{DeviceCode::D, 500}, 0x12345678}};
      const RandomWriteBitItem bwrites[] = {{{DeviceCode::M, 100}, true}, {{DeviceCode::M, 200}, false}};
      const MultiBlockReadBlock blocks[] = {{d100, 2, false}, {{DeviceCode::M, 100}, 1, true}};
      const MultiBlockReadRequest block_request({blocks, 2});
      std::uint16_t words[2] {}, reference_words[2] {};
      std::uint32_t dwords[1] {}, reference_dwords[1] {};
      BitValue bits[16] {}, reference_bits[16] {};
      MultiBlockReadBlockResult results[2] {}, reference_results[2] {};
      const std::uint16_t write_values[] = {123, 456};
      const BitValue write_bits[16] = {true};
      const MultiBlockWriteBlock write_blocks[] = {
          {d100, 2, Span<const std::uint16_t>(write_values, 2)},
          {{DeviceCode::M, 100}, 1, Span<const BitValue>(write_bits, 16)}};
      const MultiBlockWriteRequest block_write({write_blocks, 2});
      const std::uint8_t random_data[] = {0x34, 0x12, 0x78, 0x56, 0x78, 0x56, 0x34, 0x12};
      const std::uint8_t block_data[] = {0x34, 0x12, 0x78, 0x56, 1, 0};
      response(uart, operation == 0 ? Span<const std::uint8_t>(random_data, 8)
                    : operation == 3 ? Span<const std::uint8_t>(block_data, 6)
                                     : Span<const std::uint8_t>{});
      Status expected, actual;
      switch (operation) {
        case 0:
          expected = core.async_random_read(0, random_request, reference_words, reference_dwords, [](void*, Status) {}, nullptr);
          actual = synchronous ? client.random_read(random_request, words, dwords)
              : client.async_random_read(random_request, words, dwords, Completion::callback, &completion);
          break;
        case 1:
          expected = core.async_random_write_words(0, writes, dwrites, [](void*, Status) {}, nullptr);
          actual = synchronous ? client.random_write_words(writes, dwrites)
              : client.async_random_write_words(writes, dwrites, Completion::callback, &completion);
          break;
        case 2:
          expected = core.async_random_write_bits(0, bwrites, [](void*, Status) {}, nullptr);
          actual = synchronous ? client.random_write_bits(bwrites)
              : client.async_random_write_bits(bwrites, Completion::callback, &completion);
          break;
        case 3:
          expected = core.async_multi_block_read(0, block_request, reference_words, reference_bits, reference_results, [](void*, Status) {}, nullptr);
          actual = synchronous ? client.multi_block_read(block_request, words, bits, results)
              : client.async_multi_block_read(block_request, words, bits, results, Completion::callback, &completion);
          break;
        default:
          expected = core.async_multi_block_write(0, block_write, [](void*, Status) {}, nullptr);
          actual = synchronous ? client.multi_block_write(block_write)
              : client.async_multi_block_write(block_write, Completion::callback, &completion);
      }
      if (!expected.ok() || !actual.ok())
        std::fprintf(stderr, "operation=%d sync=%d core=%s adapter=%s\n", operation,
                     synchronous, expected.message, actual.message);
      assert(expected.ok() && actual.ok());
      if (!synchronous) {
        assert(client.async_random_write_bits(bwrites, nullptr).code == StatusCode::Busy);
        pump(client, uart);
        assert(completion.count == 1 && completion.result.ok());
      }
      const auto frame = core.pending_tx_frame();
      assert(uart.tx.size() == frame.size());
      assert(std::equal(uart.tx.begin(), uart.tx.end(), reinterpret_cast<const std::uint8_t*>(frame.data())));
      if (operation == 0 || operation == 3) assert(words[0] == 0x1234 && words[1] == 0x5678);
      if (operation == 0) assert(dwords[0] == 0x12345678);
      if (operation == 3) {
        assert(bits[0] && !bits[1] && !bits[15]);
        assert(results[0].data_count == 2 && results[1].bit_block && results[1].data_count == 16);
      }
    }
  }
}

void test_advanced_rejection_and_uncertain_write() {
  FakeUart uart; Client client(uart); Completion completion;
  assert(client.configure(protocol()).ok());
  assert(!client.async_random_read(RandomReadRequest({}, {}), {}, {}, Completion::callback, &completion).ok());
  assert(!client.async_random_write_words({}, {}, Completion::callback, &completion).ok());
  assert(!client.async_random_write_bits({}, Completion::callback, &completion).ok());
  assert(!client.async_multi_block_read(MultiBlockReadRequest(Span<const MultiBlockReadBlock>{}), {}, {}, {}, Completion::callback, &completion).ok());
  assert(!client.async_multi_block_write(MultiBlockWriteRequest(Span<const MultiBlockWriteBlock>{}), Completion::callback, &completion).ok());
  const MultiBlockReadBlock blocks[] = {{d100, 2, false}};
  assert(client.multi_block_read(MultiBlockReadRequest(blocks), {}, {}, {}).code == StatusCode::BufferTooSmall);
  assert(!client.busy() && completion.count == 0 && uart.tx.empty());
  const RandomWriteWordItem writes[] = {{d100, 123}};
  response(uart, {}, 0xC051);
  const auto error = client.random_write_words(writes, {});
  assert(error.code == StatusCode::PlcError && error.plc_error_code == 0xC051);
  uart.rx.clear(); uart.rx_offset = 0;
  assert(client.random_write_words(writes, {}).code == StatusCode::OperationOutcomeUnknown);
  assert(client.requires_transport_reset());
}

void test_signed_word() {
  FakeUart uart; Client client(uart);
  assert(client.configure(protocol()).ok());
  const std::uint16_t raw[] = {0, 32767, 32768, 65535, 65470};
  const std::int16_t expected[] = {0, 32767, -32768, -1, -66};
  std::int16_t value = 123;
  for (unsigned i = 0; i < 5; ++i) {
    const std::uint8_t data[] = {static_cast<std::uint8_t>(raw[i]),
        static_cast<std::uint8_t>(raw[i] >> 8)};
    response(uart, data);
    assert(client.read_word(d100, value).ok() && value == expected[i]);
    value = 123;
    response(uart, data);
    struct Context { std::int16_t* value; std::int16_t expected; int calls; } context{&value, expected[i], 0};
    assert(client.async_read_word(d100, value, [](void* user, Status status) {
      auto& c = *static_cast<Context*>(user);
      assert(status.ok() && *c.value == c.expected);
      ++c.calls;
    }, &context).ok());
    std::int16_t other = 99;
    assert(client.async_read_word(d100, other, nullptr).code == StatusCode::Busy);
    assert(value == 123);
    pump(client, uart);
    assert(value == expected[i] && context.calls == 1 && other == 99);
  }
  response(uart, {}, 0xC051);
  assert(client.read_word(d100, value).code == StatusCode::PlcError && value == -66);
  response(uart, {}, 0xC051);
  Completion completion;
  assert(client.async_read_word(d100, value, Completion::callback, &completion).ok());
  pump(client, uart);
  assert(completion.count == 1 && completion.result.code == StatusCode::PlcError && value == -66);
  assert(client.async_read_word(d100, value, nullptr).ok());
  client.cancel();
  assert(value == -66 && !client.busy());
  uart.rx.clear(); uart.rx_offset = 0;
  assert(client.read_word(d100, value).code == StatusCode::Timeout && value == -66);
}

int main() {
  test_read_and_admission(); test_failures_and_recovery(); test_tx_deadline_and_cancel();
  test_transport_errors(); test_plc_error_and_writes(); test_bits_and_inter_byte_timeout();
  test_reentrant_callback();
  test_advanced_operations(); test_advanced_rejection_and_uncertain_write();
  test_signed_word();
}
