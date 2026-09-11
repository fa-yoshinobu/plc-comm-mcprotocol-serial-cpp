#pragma once
#include <Arduino.h>
#define SOC_UART_NUM 3
using uart_port_t = int;
using esp_err_t = int;
constexpr int ESP_OK = 0, ESP_ERR_TIMEOUT = 1;
constexpr int UART_HW_FLOWCTRL_DISABLE = 0;
constexpr int UART_MODE_RS485_HALF_DUPLEX = 1, UART_MODE_UART = 0;
inline bool uart_is_driver_installed(int) { return fake_esp32::installed; }
inline int uart_get_tx_buffer_free_size(int, std::size_t* size) { *size = 256; return 0; }
inline int uart_write_bytes(int, const char*, std::size_t size) { return static_cast<int>(size); }
inline int uart_wait_tx_done(int, int) { return ESP_OK; }
inline int uart_read_bytes(int, std::uint8_t*, std::size_t, int) { return 0; }
