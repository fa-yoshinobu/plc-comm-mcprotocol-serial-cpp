# Examples

This directory contains compile-checked examples for host and MCU-oriented integration.

## Available Example

- `mcu_async_batch_read.cpp`
  - generic microcontroller-style asynchronous integration
  - shows `pending_tx_frame()`, `notify_tx_complete()`, `on_rx_bytes()`, and `poll()`
  - uses a small simulated response so the example can be built and run on a host toolchain
  - the same structure can be copied into a UART ISR / DMA based firmware task

## Build

```bash
cmake -S . -B build
cmake --build build --target mcprotocol_example_mcu_async
```

## Run

```bash
./build/mcprotocol_example_mcu_async
```

Expected output:

```text
example read ok: D100=0x1234 D101=0x5678
```

## Porting Notes

Replace the simulated transport hooks in `mcu_async_batch_read.cpp` with your board-specific pieces:

- UART TX start by polling, interrupt, or DMA
- TX complete notification
- RX byte or chunk delivery
- monotonic millisecond tick source
