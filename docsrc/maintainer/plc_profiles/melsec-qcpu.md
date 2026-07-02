# `melsec:qcpu` Profile Specification

Status: target-validated maintainer specification for the tested Q-series setup.

This file records the `melsec:qcpu` specification confirmed on the local MELSEC-Q
test setup. It is suitable as the current Q-series profile basis for this
library, but still keep exact target details with any future expansion.

## Target family

MELSEC-Q serial communication paths, for example QCPU plus QJ71C24-class serial
modules.

## Evidence status

| Topic | Status | Notes |
| --- | --- | --- |
| Public profile string | Library policy | `melsec:qcpu` is the explicit Q-series user-facing name. |
| Manual family | Manual-derived family name | MC Protocol manuals distinguish Q/L and iQ-R request-shape tables. |
| Current repository behavior | Q/L request-shape branch | `melsec:qcpu` maps to the Q/L serial request-shape branch while remaining a separate public profile. |
| Hardware observations | Confirmed for tested target | `Q06UDVCPU` with `QJ71C24N`, tested on 2026-07-01 and rechecked on 2026-07-02. |
| Normal device support list | Confirmed for tested target | Normal plain devices below passed MC Serial and SLMP cross-checks. |
| Special routes | Confirmed with dedicated routes | Link-direct and native-qualified routes are supported only through their dedicated API routes. |

## Confirmed test setup

| Item | Value |
| --- | --- |
| PLC | `Q06UDVCPU` |
| Serial module | `QJ71C24N` |
| MC Serial port | `COM3` |
| MC Serial settings | `19200`, `8E1`, no RTS/CTS, station `0`, sum-check off |
| SLMP peer | `192.168.250.100:1025` |
| Validation log | Local cross-verify run, stored outside this repository. |
| Result summary | Initial pre-fix cross-verify was `total: 91`, `ok: 84`, `ng: 7`; the 7 NG cases were special/native routes. After the ASCII native-extension fix, the special routes passed direct recheck. |

Because the target is Q-series hardware, this evidence is assigned to the
explicit `melsec:qcpu` profile.

## Confirmed request shape

| Item | Confirmed Q-series behavior |
| --- | --- |
| Normal word subcommand | `0000` style. |
| Normal bit subcommand | `0001` style. |
| Extended word subcommand | `0080` style. |
| Extended bit subcommand | `0081` style. |
| Device reference width | Shorter Q/L-style form for normal devices. |
| Confirmed frame/code modes | C4 binary / Format5 and C4 ASCII / Format4 are confirmed for the tested Q target. The serial module must be configured for the same format selected by the client; Format4 and Format5 are not expected to respond simultaneously. |

## Confirmed support devices

These are confirmed for `melsec:qcpu` on the tested Q target.

| Support class | Device families |
| --- | --- |
| Plain bit read/write | `X`, `Y`, `M`, `L`, `SM`, `F`, `V`, `B`, `TS`, `TC`, `STS`, `STC`, `CS`, `CC`, `SB`, `DX`, `DY` |
| Plain word read/write | `D`, `SD`, `W`, `TN`, `STN`, `CN`, `SW`, `Z`, `R`, `ZR` |
| Link-direct read/write | `Jn\X`, `Jn\Y`, `Jn\B`, `Jn\W` |
| Link-direct read-only | `Jn\SB`, `Jn\SW` |
| Native-qualified read/write | `Un\G` |

## Special route details

These device families are supported through dedicated API routes, not as plain
device strings.

| Route | Observed status |
| --- | --- |
| `Jn\X/Y/B` link-direct bits | Read/write supported through the link-direct route on C4 binary and post-fix C4 ASCII Format4. Representative addresses were used only as validation points; do not infer device-number restrictions from them. |
| `Jn\W` link-direct words | Read/write supported through the link-direct route on C4 binary and post-fix C4 ASCII Format4. Representative addresses were used only as validation points; do not infer device-number restrictions from them. |
| `Jn\SB/SW` link-direct special devices | Read supported. Write commands returned success on the tested Q target, but the readback value did not change; keep these as read-only support. |
| `Un\G` native qualified | Supported through the native device-access route on C4 binary and post-fix C4 ASCII Format4. The validated address was only a test point; do not infer a device-number restriction from it. |

For `Un\G`, use the native-qualified route (`0401/1401` with subcommand `0080`)
as the correct `melsec:qcpu` route. Add an implementation guard so `Un\G` cannot
be accessed through the wrong route for this profile. The `0601/1601` qualified
helper route also responded during validation, but it did not read the same
buffer value as the native route; reject that helper route for `melsec:qcpu`
`Un\G` access rather than silently using it as a fallback.

## Confirmed unsupported / not present

These device families are not present for the tested MELSEC-Q profile and are
therefore excluded from `melsec:qcpu` support.

| Category | Device families |
| --- | --- |
| Long timer/counter bit devices | `LTS`, `LTC`, `LSTS`, `LSTC`, `LCS`, `LCC` |
| Long timer/counter word devices | `LTN`, `LSTN`, `LCN` |
| iQ-R long/index/module families | `LZ`, `RD` |
| iQ-R CPU buffer memory | `Un\HG` |
| Step relay | `S` |

## Q target safety notes

- For link direct `Jn\...`, use the link-direct API route. Do not treat these as plain device strings.
- Treat SLMP/MC agreement as comparison evidence, not as proof that MC Serial write semantics are correct.

## Command-specific notes

| Command family | Notes |
| --- | --- |
| Batch read/write | Confirmed for all supported plain read/write families. |
| Native random read | Confirmed for `DX` and `DY`. Returned `0x4032` for `TS`, `TC`, `STS`, `STC`, `CS`, and `CC`. Treat this as a random-read limitation, not a batch-read exclusion. |
| Native multi-block read/write (`0406`/`1406`) | Confirmed on the tested Q serial path for supported plain devices. Do not copy the SLMP built-in-Ethernet Q-series block-command guard into this serial profile without fresh serial-module evidence. |
