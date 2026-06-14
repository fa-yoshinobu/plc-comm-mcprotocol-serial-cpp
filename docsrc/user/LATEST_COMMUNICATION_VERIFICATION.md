# Latest communication verification

Live-device verification for `plc-comm-mcprotocol-serial-cpp` is summarized here. Detailed command evidence, target-dependent notes, and retained raw observations are maintained in the linked validation reports.

## Latest verified set

| PLC / CPU | Serial module | Canonical profile | Transport | Verified scope | Limitations | Record |
| --- | --- | --- | --- | --- | --- | --- |
| MELSEC iQ-R `R120PCPU` | `RJ71C24-R2` | `melsec:q-l`, `melsec:iq-r` | RS-232C, MC Protocol serial | CPU model, loopback, device-family probes, contiguous read/write, random, multi-block, monitor, host/module buffer, qualified helper, remote control, and user-frame paths. | Some ASCII `2C` and native qualified paths are target-dependent or diagnostic-only; use the report for frame/profile details. | [RJ71C24-R2 RS-232C report](../validation/reports/RJ71C24_R2_RS232C.md) |
| MELSEC iQ-R `R08CPU` | `RJ71C24-R2` | `melsec:iq-r` | RS-232C, MC Protocol serial | Focused rechecks for `LZ1`, remote latch clear, and remote password behavior on 2026-06-12. | Remote password lock/unlock remains target-dependent; read-only sanity checks stayed available. | [RJ71C24-R2 RS-232C report](../validation/reports/RJ71C24_R2_RS232C.md) |
| MELSEC iQ-F `FX5UC-32MT/D` | Built-in serial path | `melsec:q-l` | RS-232C, MC Protocol serial | CPU model, validated contiguous subset, supported-device soak, random, and multi-block paths. | Host/module buffer, qualified helper/native access, `DX`, `DY`, `V`, `ZR`, and monitor are unsupported or not applicable on this serial path. | [FX5UC-32MT/D RS-232C report](../validation/reports/FX5UC_32MT_D_RS232C.md) |
| MELSEC L-series `L26CPU-BT` | `LJ71C24` | `melsec:q-l` | RS-232C, MC Protocol serial | CPU model, contiguous read/write, supported-device soak, random, multi-block, monitor, host/module buffer, and qualified helper paths. | Native qualified access is outside the supported path; helper access is the retained path. | [LJ71C24 RS-232C report](../validation/reports/LJ71C24_RS232C.md) |
| MELSEC Q-series `Q06UDVCPU` | `QJ71C24N` | `melsec:q-l` | RS-232C, MC Protocol serial | CPU model, contiguous read/write, supported-device soak, random, multi-block, monitor, host/module buffer, and qualified helper paths. | Native qualified access is outside the supported path; helper access is the retained path. | [QJ71C24N RS-232C report](../validation/reports/QJ71C24N_RS232C.md) |

## Retained evidence

Use [Hardware validation](../validation/reports/HARDWARE_VALIDATION.md) for:

- current target matrices and stress snapshots
- exact serial settings used during retained validation
- command-family pass, target-dependent, and unsupported decisions
- links to target-specific reports

Do not move the full validation matrix into README. README should stay as the project entry point.
