#pragma once
#include <Arduino.h>
#define GPIO_IS_VALID_GPIO(p) ((p) >= 0 && (p) < 49)
#define GPIO_IS_VALID_OUTPUT_GPIO(p) GPIO_IS_VALID_GPIO(p)
using gpio_num_t = int;
constexpr int GPIO_MODE_OUTPUT = 1;
inline int gpio_reset_pin(int) { return 0; }
inline int gpio_set_level(int pin, int level) { fake_esp32::gpio_levels[pin] = level; return 0; }
inline int gpio_set_direction(int, int) { return 0; }
