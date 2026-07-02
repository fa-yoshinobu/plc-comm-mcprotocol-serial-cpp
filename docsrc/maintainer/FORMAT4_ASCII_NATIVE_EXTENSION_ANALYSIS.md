# 4C ASCII Format4 Native Extension Investigation

Status: resolved on 2026-07-02.

## Summary

| Item | Result |
| --- | --- |
| Affected routes | Native extended devices: `Jn\X/Y/B/W/SB/SW`, `Un\G`, `Un\HG` |
| Affected frame | 4C ASCII Format4 |
| Symptom | Plain devices worked, but native extended routes returned `0x7F22` or timed out. |
| Root cause | The ASCII extended-device encoder omitted the trailing device-modification field after the device number. |
| Manual basis | SH-080003-AF shows the trailing field for `Jn\...` and `Un\...` ASCII device-extension forms. |
| Fix | Append the trailing device-modification field in both ASCII extended-reference builders in `src/codec.cpp`. |
| Current decision | Format4 ASCII native extended access is supported for the validated iQ-R and Q/L serial targets. |

## Hardware Recheck

Post-fix checks passed on both target families.

| Target | Profile / settings | Result |
| --- | --- | --- |
| `R120PCPU` via C24 | `melsec:iq-r`, `COM3`, `19200 / 8E1`, station `0`, sum-check off, `c4-ascii-f4` | `Jn\...`, `Un\G`, and `Un\HG` reads passed. Representative writes, random, multi-block, and monitor checks passed. |
| `Q06UDVCPU` via C24 | `melsec:qcpu`, `COM3`, `19200 / 8E1`, station `0`, sum-check off, `c4-ascii-f4` | All previously failing `Jn\...` and `U2\G1000` reads passed. Representative writes, random, multi-block, and monitor checks passed. |

## Notes

- The serial module is configured for one MC protocol format at a time. Do not
  expect Format4 ASCII and Format5 Binary to respond simultaneously.
- `S` failures are outside this issue because `S` is not part of the supported
  serial MC device surface.
- Historical `0x7F22` tables from pre-fix runs are no longer current behavior.
  Before recording new NG evidence, confirm the CLI binary is rebuilt after the
  encoder fix and preserve fresh TX/RX frames.

No active Format4 follow-up remains.
