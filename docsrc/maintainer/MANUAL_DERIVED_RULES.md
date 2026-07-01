# Manual-Derived Rules

Keep these rules aligned with the current MELSEC serial manuals and validated hardware results.

## PLC Profile Branch Provenance

Use this table before changing `SUPPORTED_REGISTERS.md` or adding profile-specific rejection
logic. A profile branch should not become a public read/write support rule until its source is
separated into one of these buckets:

- Manual-derived: the MELSEC manual describes the command shape, device form, or applicability.
- Library policy: a local API/configuration decision, not a manual term.
- Hardware observation: repeatable target evidence, but still target/module dependent.
- Pending manual audit: do not treat it as a final support or reject contract yet.

| Branch or rule | Source status | Maintainer note |
| --- | --- | --- |
| Canonical strings such as `melsec:iq-r` and `melsec:q-l` | Library policy | The manuals describe PLC families and modules, not these exact saved configuration strings. |
| `PlcProfile` to `PlcSeries` mapping | Library policy informed by manual family names | Treat it as an implementation grouping. Recheck the manuals before adding diverging behavior inside a grouped profile. |
| iQ-R vs non-iQ-R device reference widths, device code widths, and normal/extended subcommands | Manual-derived | This covers request-shape branches such as `0000/0001` vs `0002/0003` and `0080/0081` vs `0082/0083`. Keep page citations with any new change. |
| 1C/1E A-series and QnA command-family selection, including `ER/EW` and `NR/NW` paths | Manual-derived | These are command-family branches, not read/write device-support policy. |
| Link-direct `Jn\X/Y/B/W/SB/SW` request shape | Manual-derived request shape plus hardware observation | SH-080003-AF describes link-direct access in the extended-device appendix. Actual availability still depends on target/module/frame settings. |
| Qualified `Un\G` / `Un\HG` request shape | Manual-derived request shape plus hardware observation | SH-080003-AF describes unit access and CPU buffer access devices. The practical helper path and native-command behavior must stay separated. |
| `0x7F22` interpretation for serial module responses | Manual-derived error meaning plus local interpretation | SH-081249-L lists `7F22H` as a command error. The exact cause for a given trace is still a diagnostic conclusion. |
| `melsec:iq-r` read/write device-family list | Pending manual audit | Many device codes and layouts are manual-derived, but the final per-profile read/write support contract still needs page-backed review. |
| `melsec:iq-l` command/device layout | Manual evidence plus hardware observation and implementation policy | SH-082159CHN-F says iQ-L serial MC communication through MELSEC-L serial modules must use MELSEC-Q/L subcommands, not iQ-R subcommands. Keep the public profile separate. Current implementation uses Q/L-compatible serial MC request shapes and rejects the long timer/counter family, `LZ`, `RD`, and `Un\HG` for iQ-L serial MC. |
| FX5 / MELSEC iQ-F MC protocol layout | Manual-derived source added / pending profile policy | SH-082624-J describes FX5 MC protocol with subcommand bit fields for data size, device-reference width, and device-memory extension. Treat FX5/iQ-F as a separate audit input; do not fold it into Q/L or iQ-R only by name. |
| `melsec:q-l` read/write device-family allowlist | Pending manual audit / candidate policy | The current candidate list came from target-specific requirements and verification work. Do not enforce it in the codec until manual or repeatable evidence is recorded. |
| `LZ` treated as iQ-R-only in validators | Pending page citation | Existing implementation behavior remains, but do not expand this style of per-profile device rejection without a manual citation. |
| iQ-R binary link-direct compatibility fallback to Q/L wire layout | Hardware observation | Keep it documented as a compatibility exception, not as a manual-derived rule. |
| 4C ASCII Format4 native extended-access rejection for `Jn\...` and `Un\...` | Hardware observation | The Q/L and iQ-R observations are measurement evidence only; encoder/client bugs are not fully ruled out. |
| Separate public `melsec:iq-l` profile | Library policy plus target-family risk control | Keep this profile separate so iQ-L can be switched toward iQ-R-compatible behavior or another verified layout without changing saved configuration names. |

## Address and Device Parsing

- `ZR` is decimal on the validated serial targets in this repo.
- `R` and `ZR` share the same internal register space on the validated `RJ71C24-R2` target.
- `X`, `Y`, `B`, `W`, `SB`, `SW`, `DX`, and `DY` use hexadecimal address parsing.
- `M`, `L`, `SM`, `F`, `V`, `D`, `SD`, `RD`, `S`, `Z`, `R`, `ZR`, `LTN`, `LSTN`, `LCN`, and `LZ`
  use decimal address parsing.

## Bit Packing

- Binary single-point bit reads return the addressed value in the high nibble.
- Binary single-point bit writes carry the addressed value in the high nibble.
- Binary word-unit bit-block packing uses `bit0 -> LSB` for the head device.
- Keep request-shape tests aligned with this rule before treating a mismatch as a hardware issue.

## Long Timer and Counter Devices

- Treat `LTS/LTC/LSTS/LSTC` as structured data carried by `LTN/LSTN` `0401` responses.
- On the validated `RJ71C24-R2 + R120PCPU` setup:
  - the first two words of `LTN/LSTN` hold the current value
  - the third word holds contact/coil bits
  - `0x0001 = coil`, `0x0002 = contact`
- Treat `LCS/LCC` as long-state helper targets that use direct bit access internally.
- Do not treat ordinary direct `read-bits` calls for `LTS/LTC/LSTS/LSTC` as part of the supported public interface; use the long-state helper.
- For `melsec:iq-l`, do not expose `LTS/LTC/LSTS/LSTC/LCS/LCC/LTN/LSTN/LCN/LZ/RD` as serial MC supported devices.
- For `melsec:iq-l`, treat `S` as read-only on serial MC; observed write attempts returned `0x4030`.

## Link-Direct Access

- Link-direct `Jn\...` helpers intentionally accept only `X/Y/B/SB` bit devices and `W/SW` word devices.
- Do not map `LCS/LCC` long-state names onto link-direct access; use the long-state helper for those devices.

## Qualified Access

- `G` and `HG` are not standalone plain-device support entries in any profile.
- Use qualified forms such as `U...\\G...` and `U...\\HG...` only when the selected profile supports them.
- Keep route selection profile-specific. Do not conflate the `0601/1601` qualified-buffer helper path with the `0401/1401` native-qualified path.

## FX5U Serial Scope

- On `FX5UC-32MT/D` serial `3C/4C`, treat `0801/0802` as unsupported.
- On the same path, keep `DX`, `DY`, `V`, and `ZR` out of the validated contiguous subset.

## Validation Discipline

- Check manual-backed request shape first.
- Then compare the local codec path.
- Then use hardware to break ties.
- Do not treat unsupported access paths as active holds.
