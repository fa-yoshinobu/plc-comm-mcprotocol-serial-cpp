# PLC Profile Draft Specifications

These files are maintainer working notes for profile design and verification.
They are not user-facing support contracts.

Before changing `PlcProfile`, `PlcSeries`, codec branching, or
`SUPPORTED_REGISTERS.md`, separate each claim into one of these buckets:

- Manual-derived: the MELSEC manual describes the command shape, device form, or applicability.
- Hardware observation: repeatable result from a named PLC/module/frame setup.
- Library policy: a local API naming or grouping decision.
- Pending: not decided yet.

## Profile files

| Profile | File |
| --- | --- |
| `melsec:iq-r` | [melsec-iq-r.md](melsec-iq-r.md) |
| `melsec:iq-l` | [melsec-iq-l.md](melsec-iq-l.md) |
| `melsec:iq-f` | [melsec-iq-f.md](melsec-iq-f.md) |
| `melsec:q` | [melsec-q.md](melsec-q.md) |
| `melsec:l` | [melsec-l.md](melsec-l.md) |
| `melsec:qna` | [melsec-qna.md](melsec-qna.md) |
| `melsec:ana-anu` | [melsec-ana-anu.md](melsec-ana-anu.md) |
| `melsec:a` | [melsec-a.md](melsec-a.md) |

## Common verification checklist

Use this checklist per profile before promoting a device family to user-facing
read/write support.

1. Confirm the manual command shape for the selected frame and code mode.
2. Encode local request bytes and compare them with the manual shape.
3. Run read-only probes first: CPU model, loopback, and safe device reads.
4. Run write probes only on ranges reserved for testing.
5. Compare MC Serial results against SLMP/MC only as diagnostic evidence, not as proof by itself.
6. Record target model, module, firmware if known, frame type, code mode, serial settings, and station route.
7. Mark each device family as manual-derived, observed, rejected by target, or not tested.

Common device-form rule: `G` and `HG` are not standalone plain devices for any
MELSEC profile. Record support only for qualified forms such as `Un\G` and
`Un\HG`, and keep the accepted route profile-specific.

`melsec:q-l` is a legacy combined spelling from earlier library work. It should
not be used as the source of truth while deciding the new per-family profiles.

## Profiles without local test equipment

These profiles are maintained by manual-derived inference and codec-level tests
until matching PLC hardware becomes available:

| Profile | Current verification basis |
| --- | --- |
| `melsec:qna` | Manual-derived command-family inference; no local QnA target. |
| `melsec:ana-anu` | Manual-derived command-family inference; no local AnA/AnU target. |
| `melsec:a` | Manual-derived ACPU command inference; no local A-series target. |

Do not label results for these profiles as hardware-observed, and do not promote
their device inventories to user-facing read/write support without later target
evidence.
