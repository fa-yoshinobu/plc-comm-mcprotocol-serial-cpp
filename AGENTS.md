# Agent Notes

## Serial / COM Port Access

- Do not open the same COM port from multiple commands or agents at the same time.
- Run PLC serial checks sequentially when they use `COM3` or any other physical serial port.
- Do not parallelize commands that open a serial port, even if the operations look read-only.
- If a serial command fails with permission denied or access denied, first assume another process still has the port open.
