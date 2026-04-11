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

If the validated `RJ71C24-R2` ASCII link gets stuck after a failed native probe, recover the C24
transmission sequence before the next command:

```bash
./build/mcprotocol_cli ... recover-c24
```

That sends ASCII `EOT CRLF` and expects no response payload.

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

Useful follow-up checks:

- rerun with `--dump-frames on` to inspect the exact TX/RX bytes
- keep `--sum-check off` on the validated setup; turning it on can produce `0x7F23` even for `0401`
- if a `dword-only` random or monitor request is involved, expect `0x7F23` more often than `0x7F22`

## `0x7F22` even on simple `read-words` / `read-bits` after swapping hardware

Check the CLI family selection against the actual CPU family.

Known example from `2026-04-11`:

- `L26CPU-BT + LJ71C24 + Format5/Binary` required `--series ql`
- the same setup under `--series iqr` returned `0x7F22` even for `read-words D100 1` and `read-bits M100 1`
- `FX5UC-32MT/D + Format5/Binary + 38400 / 8E2` also required `--series ql`
- the same FX setup under `--series iqr` returned `0x7E40` even for `read-words D100 1` and `read-bits M100 1`

If you changed the PLC CPU or serial module family, rerun `cpu-model` first and then pick the
matching `--series` before diagnosing anything else.

## `0x7E40` or `0x7E43` on `FX5UC-32MT/D`

On the `2026-04-11` FX5U validation target:

- `--series iqr` caused contiguous `D100` / `M100` reads to fail with `0x7E40`
- `--series ql` was the practical setting
- the FX5 communication manual's serial `3C/4C` accessible-device table marks `DX`, `DY`, `V`, and `ZR` inaccessible on this target class
- `DX0`, `DX1`, `DY0`, `DY1`, `DX10`, and `DY10` all returned `0x7E43` under `--series ql`
- host-buffer, module-buffer, and `U3E0\\...` qualified probes also returned `0x7E40` or `0x7E43`

If you are trying to reproduce the validated FX5U contiguous soak, use:

```bash
./examples/linux_cli/fx5u_supported_device_rw_soak.sh
```

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
