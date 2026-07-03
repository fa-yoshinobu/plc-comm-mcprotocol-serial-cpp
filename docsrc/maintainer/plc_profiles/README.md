# PLC Profile Draft Specifications

These files are maintainer working notes for profile design and verification.
They are not user-facing support contracts.

Before changing `PlcProfile`, `PlcSeries`, codec branching, or
`SUPPORTED_REGISTERS.md`, separate each claim into one of these buckets:

- Manual-derived: the MELSEC manual describes the command shape, device form, or applicability.
- Hardware observation: repeatable result from a named PLC/module/frame setup.
- Library policy: a local API naming or grouping decision.
- Scope boundary: not promoted to user-facing support.

## Profile files

| Profile | File |
| --- | --- |
| `melsec:iq-r` | [melsec-iq-r.md](melsec-iq-r.md) |
| `melsec:iq-l` | [melsec-iq-l.md](melsec-iq-l.md) |
| `melsec:iq-f` | [melsec-iq-f.md](melsec-iq-f.md) |
| `melsec:qcpu` | [melsec-qcpu.md](melsec-qcpu.md) |
| `melsec:lcpu` | [melsec-lcpu.md](melsec-lcpu.md) |
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
7. Mark each device family as manual-derived, observed, rejected by target, or outside the current support surface.

Common device-form rule: `G` and `HG` are not standalone plain devices for any
MELSEC profile. Record support only for qualified forms such as `Un\G` and
`Un\HG`, and keep the accepted route profile-specific.

Common step-relay rule: `S` is not part of the supported serial MC device
surface for this library. Keep it out of every profile unless a future manual
and hardware pass deliberately reopens the decision.

Do not reintroduce a combined Q/L public profile. Keep Q and L as separate
profile names, even when the current implementation maps both to the Q/L
request-shape branch.

## Profiles Without Current Hardware Contract

These profiles are maintained by manual-derived inference and codec-level tests.
They are not promoted to user-facing hardware-observed support:

| Profile | Current verification basis |
| --- | --- |
| `melsec:qna` | Manual-derived command-family inference; no hardware-observed support contract. |
| `melsec:ana-anu` | Manual-derived command-family inference; no hardware-observed support contract. |
| `melsec:a` | Manual-derived ACPU command inference; no hardware-observed support contract. |

Do not label results for these profiles as hardware-observed, and do not promote
their device inventories to user-facing read/write support without a deliberate
support-contract update.
