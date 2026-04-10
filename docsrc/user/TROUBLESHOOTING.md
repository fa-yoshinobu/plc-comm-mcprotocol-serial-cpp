# Troubleshooting

## `permission denied` on `/dev/ttyUSB0`

Your Linux user probably does not have serial-port access.

Typical fix:

```bash
sudo usermod -aG dialout "$USER"
```

Then log out and log back in.

## `Timed out while waiting for a response`

Check the physical and protocol basics first:

- correct adapter type
- correct `RS-232C` vs `RS-485`
- correct baud rate
- correct parity and stop bits
- correct `Format4 ASCII` vs another frame format
- correct station number

## `0x7F22` or `0x7F23` from `RJ71C24-R2`

On the validated `RJ71C24-R2` setup, that usually means one of these:

- wrong command / subcommand for the configured module behavior
- wrong device code
- trying a native command path that this setup rejects

Current known native hold items:

- `0403`
- `1402`
- `0406`
- `1406`
- `0801`
- `0802`

Those commands currently fail natively on the validated setup. The CLI does not replace them with other command families.

## MCU board compiles but never receives data

Check these first:

- you used a real `RS-232C` level shifter between the MCU and PLC
- your `Serial1` pins match the actual board routing
- debug USB serial is not colliding with the PLC UART
- TX completion is actually signaled with `notify_tx_complete()`
- received bytes are really being fed into `on_rx_bytes()`

## Doxygen build fails

Run:

```bash
cmake --build build --target docs
```

or:

```bash
doxygen Doxyfile
```

If it says `Doxygen not found`, install Doxygen system-wide or place a local bundle under `.tools/doxygen`.

## Markdown link check fails

Run:

```bash
cmake --build build --target check-markdown-links
```

That checker verifies local relative Markdown links. Fix missing files or broken relative paths and rerun it.
