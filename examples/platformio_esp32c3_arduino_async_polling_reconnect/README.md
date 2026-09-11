# ESP32-C3 Arduino Polling and Explicit Recovery

This read-only example uses `Esp32UartClient` on UART1 (RX=6, TX=7).
Do not initialize `Serial1` separately. It polls D100-D103 and logs
`connected`, `reconnecting`, and `recovered`.
The existing environment is `esp32-c3-devkitm-1-polling-reconnect`.
PLC defaults are 19200 baud, 8E1, C4 ASCII Format4, MelsecQ, no sum check.
Direction control is external/automatic unless explicitly configured.

## Error handling

| Result | Action |
| --- | --- |
| Success | Read again after the poll interval |
| Complete PLC error response, no transport reset required | Log PLC error code and retry this read after the interval |
| Timeout, incomplete/malformed response, transport failure requiring reset | Stop; wait for explicit recovery |
| Configuration/admission error without a reset requirement | Stop; correct configuration and reset |

The reset requirement takes priority over the error category.
Elapsed backoff time or an empty RX buffer is not proof that an old reply
can no longer arrive. This example no longer automatically clears the core state
after a timeout.

## Recovery procedure

1. Stop sending requests. The example does this automatically on a reset-required error.
2. Correct wiring/settings and perform the PLC/interface-specific communication
   reset or isolation procedure that guarantees no old reply can still arrive.
   Use the PLC/interface documentation for timing. There is no universal safe delay.
3. Only after that condition is established, send lowercase **r** on the debug console.
4. The adapter closes/reopens the UART, discards local receive state and reconfigures
   the core. A failed recovery remains stopped. A successful recovery starts a new read.

The console command acknowledges step 2; it cannot verify the physical line.
The adapter stops hardware TX on a TX failure before releasing the transaction.
UART reopening alone is insufficient to establish step 2. If initialization failed,
correct the configuration and reset the MCU instead of using r.

This is a read-only example. Do not extend its retry policy directly to writes:
a timed-out write may already have executed at the PLC. Check the device state or
use an application-level acknowledgment before deciding whether to issue a new write.

## Verification

Host tests include the actual example source and check timeout stop, no blind retry,
explicit restart, PLC-error read retry, and millis wrap. The adapter tests cover
partial transmission, TX deadlines, cancellation and response handling.
Hardware-specific delayed-response exclusion must be validated on the actual system.
