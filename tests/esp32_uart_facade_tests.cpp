#include "test_assert.hpp"
#include "mcprotocol_serial_arduino_esp32.hpp"
using namespace mcprotocol::serial;
int main() {
  Esp32UartConfig config;
  config.rx_pin = 39; config.tx_pin = 0;
  config.direction = Esp32Direction::Rs485Rts; config.rts_pin = 46;
  const auto protocol = ProtocolConfig::c4_binary(PlcProfile::MelsecIqF,
      SumCheckMode::Enabled, RouteConfig{HostStationRoute{}}, TimeoutConfig{20, 5});
  // Constructing/destroying a rejected adapter must not close another UART owner.
  fake_esp32::installed = true;
  {
    Esp32UartClient client(1);
    assert(client.begin(config, protocol).code == StatusCode::Busy);
  }
  assert(fake_esp32::installed && fake_esp32::destroyed == 0);
  fake_esp32::installed = false;
  {
    Esp32UartClient invalid(255);
    assert(invalid.begin(config, protocol).code == StatusCode::InvalidArgument);
  }
  assert(fake_esp32::destroyed == 0);
  {
    Esp32UartClient client(1);
    auto bad = config; bad.format = SERIAL_7E1;
    assert(client.begin(bad, protocol).code == StatusCode::InvalidArgument);
    bad = config; bad.rts_pin = bad.tx_pin;
    assert(client.begin(bad, protocol).code == StatusCode::InvalidArgument);
    fake_esp32::init_fails = true;
    assert(client.begin(config, protocol).code == StatusCode::Transport);
    fake_esp32::init_fails = false;
    fake_esp32::setup_fails = true;
    assert(client.begin(config, protocol).code == StatusCode::Transport);
    fake_esp32::setup_fails = false;
    assert(!fake_esp32::installed);
    assert(client.begin(config, protocol).ok());
    assert(client.begin(config, protocol).code == StatusCode::Busy);
    std::uint16_t value = 777;
    assert(client.async_read_words({DeviceCode::D, 100}, {&value, 1}, nullptr).ok());
    assert(client.end().code == StatusCode::Busy);
    assert(client.recover(protocol).code == StatusCode::Busy);
    client.cancel();
    assert(client.read_word({DeviceCode::D, 100}, value).code == StatusCode::Timeout);
    assert(client.requires_transport_reset() && value == 777);
    assert(!client.configure(protocol).ok());
    assert(client.recover(protocol).ok());
    assert(!client.requires_transport_reset());
    assert(client.end().ok());
    assert(fake_esp32::gpio_levels[46] == 0);
  }
  // Destruction after end() must not later terminate a replacement owner.
  {
    Esp32UartClient client(1);
    assert(client.begin(config, protocol).ok());
    assert(client.end().ok());
    fake_esp32::installed = true;
  }
  assert(fake_esp32::installed);
}
