#pragma once
#include <cstddef>
#include <cstdint>
#define SERIAL_7N1 0x08000018U
#define SERIAL_7N2 0x08000038U
#define SERIAL_7E1 0x0800001aU
#define SERIAL_7E2 0x0800003aU
#define SERIAL_7O1 0x0800001bU
#define SERIAL_7O2 0x0800003bU
#define SERIAL_8N1 0x0800001cU
#define SERIAL_8N2 0x0800003cU
#define SERIAL_8E1 0x0800001eU
#define SERIAL_8E2 0x0800003eU
#define SERIAL_8O1 0x0800001fU
#define SERIAL_8O2 0x0800003fU
namespace fake_esp32 {
inline std::uint32_t now = 0;
inline bool installed = false, init_fails = false, setup_fails = false;
inline int destroyed = 0, gpio_levels[64] = {};
}
inline std::uint32_t millis() { return fake_esp32::now; }
inline void delay(unsigned n) { fake_esp32::now += n; }
