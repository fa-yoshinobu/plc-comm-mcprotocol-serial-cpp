# FX5UC-32MT/D RS-232C Report

Audience: maintainers and users who need the current validated view of the `FX5UC-32MT/D` path.

This page is the single report for this hardware target.

## Target

- PLC CPU: Mitsubishi iQ-F `FX5UC-32MT/D`
- Link: `RS-232C`
- Settings: `38400 / 8E2 / MC Protocol Format5 Binary / sum-check on / station 0`
- Practical family selection: `ql`

## Summary

| Area | Current status | Notes |
|---|---|---|
| CPU identification | pass | `cpu-model` returns `FX5UC-32MT/D`, `0x4A91` |
| Contiguous read/write | pass, narrow subset | validated `21`-target subset |
| Supported-device soak | pass | two `180` second runs passed on the screened subset |
| Native `0403` | pass | validated on the current setup |
| Native `1402` words | pass | validated with restore on the current setup |
| Native `1402` bits | pass | validated with restore on the current setup |
| Native `0406/1406` | pass | validated with restore on the current setup |
| Native `0801/0802` | not applicable / unsupported | FX5 serial `3C/4C` command list does not include `0801/0802`; probes returned `0x7E40` |
| Host/module buffer | not applicable / unsupported | probes returned `0x7E40`, and the FX5 serial `3C/4C` command list does not list `0613`, `1613`, `0601`, or `1601` |
| Qualified helper/native access | not applicable / unsupported | helper `U3E0\\G10` / `U3E0\\HG20` returned `0x7E40`; native `U3E0\\G10` returned `0x7E43` |
| `DX/DY` | inaccessible on this path | observed `0x7E43`; FX5 serial `3C/4C` accessible-device table marks them inaccessible |
| `V/ZR` | not applicable on this path | FX5 serial `3C/4C` accessible-device table marks them inaccessible |

## Device-family Note

The validated contiguous subset on this target is:

- `STS10`, `STC10`, `STN10`, `TS10`, `TC10`, `TN10`
- `CS10`, `CC10`, `CN10`
- `SB10`, `SW10`
- `X10`, `Y10`
- `M100`, `L100`, `F100`
- `B10`, `D100`, `W10`, `Z10`, `R100`

Keep `DX`, `DY`, `V`, and `ZR` out of the FX5U serial `3C/4C` validated subset.

The FX5 communication manual `sh082625engh.pdf` `43.1 List of Commands and Functions`
(`3C/4C frame`, printed page `718-719`) lists `0401/1401`, `0403/1402`, `0406/1406`,
remote control, loopback, and `1617`, but does not list `0613`, `1613`, `0601`, or `1601`.

The canonical matrix is [HARDWARE_VALIDATION.md](HARDWARE_VALIDATION.md).
