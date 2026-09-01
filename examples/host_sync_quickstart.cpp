#include <array>
#include <cstdint>
#include <cstdio>

#include "mcprotocol_serial.hpp"

int main() {
  using mcprotocol::serial::CpuModelInfo;
  using mcprotocol::serial::HardwareFlowControl;
  using mcprotocol::serial::PlcProfile;
  using mcprotocol::serial::HostSerialConfig;
  using mcprotocol::serial::HostSyncClient;
  using mcprotocol::serial::SerialParity;
  using mcprotocol::serial::SumCheckMode;
  using mcprotocol::serial::highlevel::make_c4_ascii_format4_protocol;

  HostSyncClient plc;

  // The serial port settings must match the PLC serial module exactly.
  const HostSerialConfig serial(
#if defined(_WIN32)
      "COM3",
#else
      "/dev/ttyUSB0",
#endif
      19200,
      8,
      1,
      SerialParity::Even,
      HardwareFlowControl::None);

  // Keep the PLC profile explicit. See docsrc/user/GOTCHAS.md before changing it.
  auto protocol = make_c4_ascii_format4_protocol(
      PlcProfile::MelsecQ,
      SumCheckMode::Disabled,
      mcprotocol::serial::RouteConfig {mcprotocol::serial::HostStationRoute {}});

  // Open configures the blocking host facade and the underlying async client.
  mcprotocol::serial::Status status = plc.open(serial, protocol);
  if (!status.ok()) {
    std::fprintf(stderr, "open failed: %s\n", status.message);
    return 1;
  }

  CpuModelInfo model {};
  // Read-only model information is a useful first sanity check after open.
  status = plc.read_cpu_model(model);
  if (!status.ok()) {
    std::fprintf(stderr, "cpu-model failed: %s\n", status.message);
    return 1;
  }

  std::array<std::uint16_t, 2> words {};
  // Batch reads are the simplest data path for contiguous word devices.
  status = plc.read_words_single_request("D100", words);
  if (!status.ok()) {
    std::fprintf(stderr, "read_words_single_request failed: %s\n", status.message);
    return 1;
  }

  std::uint16_t sparse_d100 = 0;
  // Random read shows the sparse-device path without writing to the PLC.
  status = plc.read_random_word("D100", sparse_d100);
  if (!status.ok()) {
    std::fprintf(stderr, "read_random_word failed: %s\n", status.message);
    return 1;
  }

  std::printf(
      "sync example ok: model=%s code=0x%04X D100=0x%04X D101=0x%04X sparseD100=0x%04X\n",
      model.model_name.data(),
      model.model_code,
      words[0],
      words[1],
      static_cast<std::uint16_t>(sparse_d100 & 0xFFFFU));
  return 0;
}
