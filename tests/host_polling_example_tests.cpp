#include "test_assert.hpp"
#define main example_main
#include "../examples/host_sync_polling_reconnect.cpp"
#undef main

int main() {
  unsigned value = 0;
  assert(parse_unsigned("115200", value) && value == 115200);
  for (const char* text : {"-1", "+1", " 1", "", "1x", "9999999999999999999999999"})
    assert(!parse_unsigned(text, value));
  using namespace mcprotocol::serial;
  assert(may_retry_read(ok_status()));
  assert(may_retry_read(make_status(StatusCode::PlcError, "PLC error")));
  for (auto code : {StatusCode::Timeout, StatusCode::Transport, StatusCode::Framing,
                    StatusCode::Parse, StatusCode::OperationOutcomeUnknown})
    assert(!may_retry_read(make_status(code, "stop")));
}
