# 4C ASCII Format1 iQ-R Link-Direct Investigation

Status: resolved on 2026-07-03.

## Summary

| Item | Result |
| --- | --- |
| Target | `R120PCPU` via C24, `COM3`, `19200 / 8E1`, station `0`, sum-check off |
| Profile / frame | `melsec:iq-r`, `c4-ascii-f1` |
| Symptom | Link-direct responses could terminate the CLI with access violation `0xC0000005`. |
| Root cause | `FrameCodec::decode_response` searched for `ETX` before checking that the received chunk was at least the Format1 STX response prefix length. A short serial chunk could make the search start past the span end. |
| Fix | Return `Incomplete` when `bytes.size() < prefix_size` before searching for `ETX`. The fix is shared by ASCII Format1/2/4. |
| Regression coverage | Recorded partial chunks plus a response-prefix sweep across C-frame formats. |

## Hardware Recheck

After rebuilding the fixed binary, Format1 passed the previously failing access
routes:

- `read-link-direct-bits`: `J1\X10`, `J1\Y10`, `J1\B10`, `J1\SB10`
- `read-link-direct-words`: `J1\W40`, `J1\SW10`
- Normal controls: `read-words D0 1`, `cpu-model`
- Full Format1 suite: link-direct read/write, random, multi-block, and monitor routes

## Non-Bug Observations

These observations are not Format1 defects:

- `S` is unsupported for serial MC and is locally rejected by the library.
- Raw monitor read `0802` without prior registration can return monitor-not-registered behavior.
- Module-buffer helper access is outside the supported iQ-R route; use native-qualified access.

No active Format1 follow-up remains.
