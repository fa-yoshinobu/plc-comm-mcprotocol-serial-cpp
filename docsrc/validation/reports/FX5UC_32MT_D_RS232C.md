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
| Native `0403` | pass | focused word/bit rechecks passed |
| Native `1402` words | pass | focused word rechecks passed with restore |
| Native `1402` bits | pass | focused bit rechecks passed with restore |
| Native `0406/1406` | pass | capture-driven fixes made both pass |
| Native `0801/0802` | not applicable / unsupported | FX5 serial `3C/4C` command list does not include `0801/0802`; probes returned `0x7E40` |
| Host/module buffer | not applicable / unresolved | probes returned `0x7E40` |
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

## Historical Note

- Earlier native `0403/1402/0406/1406` failures on this target were host-side compatibility bugs.
- After the documented binary count and bit-packing fixes, those native families pass on FX5U.
- The canonical matrix is [HARDWARE_VALIDATION.md](HARDWARE_VALIDATION.md).
