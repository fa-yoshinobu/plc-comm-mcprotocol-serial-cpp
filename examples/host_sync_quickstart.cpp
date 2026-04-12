#include <array>
#include <cstdint>
#include <cstdio>

#include "mcprotocol_serial.hpp"

int main() {
  using mcprotocol::serial::CpuModelInfo;
  using mcprotocol::serial::PosixSerialConfig;
  using mcprotocol::serial::PosixSyncClient;
  using mcprotocol::serial::highlevel::make_c4_binary_protocol;

  PosixSyncClient plc;

  // Replace these values with the settings validated for your actual target.
  PosixSerialConfig serial {
#if defined(_WIN32)
      .device_path = "COM3",
#else
      .device_path = "/dev/ttyUSB0",
#endif
      .baud_rate = 19200,
      .data_bits = 8,
      .stop_bits = 2,
      .parity = 'E',
      .rts_cts = false,
  };
  auto protocol = make_c4_binary_protocol();
  protocol.route.station_no = 0;

  mcprotocol::serial::Status status = plc.open(serial, protocol);
  if (!status.ok()) {
    std::fprintf(stderr, "open failed: %s\n", status.message);
    return 1;
  }

  CpuModelInfo model {};
  status = plc.read_cpu_model(model);
  if (!status.ok()) {
    std::fprintf(stderr, "cpu-model failed: %s\n", status.message);
    return 1;
  }

  std::array<std::uint16_t, 2> words {};
  status = plc.read_words("D100", words);
  if (!status.ok()) {
    std::fprintf(stderr, "read_words failed: %s\n", status.message);
    return 1;
  }

  std::uint32_t sparse_d100 = 0;
  status = plc.random_read("D100", sparse_d100);
  if (!status.ok()) {
    std::fprintf(stderr, "random_read failed: %s\n", status.message);
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
