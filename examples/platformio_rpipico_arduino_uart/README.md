# Raspberry Pi Pico Arduino UART Example

This sample is dedicated to the `rpipico-arduino-uart-example` environment.

Main file:

- [platformio_rpipico_arduino_uart.cpp](platformio_rpipico_arduino_uart.cpp)

It uses `Serial1` for the PLC line and keeps the Pico-specific real-UART flow in its own file.

Serial1 must remain on UART0 (TX GPIO0, RX GPIO1). The sample checks the RP2040
UART0 BUSY flag for physical transmission completion; Arduino Mbed's flush only
checks FIFO writability. Early receive data is retained until TX completion.
The Arduino write call can block while filling the FIFO; after it returns, the
sample checks the byte count and the absolute deadline. Subsequent TX completion
waiting is polled without blocking. Partial TX or a TX deadline stops the UART
before notifying the core; the notification uses the current timestamp.

Every error stops polling. Correct the cause and perform the PLC/interface-specific
reset or isolation procedure to exclude delayed responses before resetting the MCU.
MCU reset alone does not exclude an old PLC response. No writes or automatic
recovery are performed. Poll timing handles millis wrap.

The PlatformIO target builds with Arduino Mbed. Host tests cover partial TX,
physical TX timeout, retained early RX, error stop and millis wrap. Physical PLC
communication and recovery have not been verified for this revised Pico example.
