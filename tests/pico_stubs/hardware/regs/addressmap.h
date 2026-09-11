#pragma once
inline std::uint32_t fake_uart_flags = 0;
#define UART0_BASE reinterpret_cast<std::uintptr_t>(&fake_uart_flags)
