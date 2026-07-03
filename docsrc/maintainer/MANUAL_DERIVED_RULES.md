# Manual-Derived Rules

Keep these rules aligned with the current MELSEC serial manuals and validated hardware results.

## PLC Profile Branch Provenance

Use this table before changing `SUPPORTED_REGISTERS.md` or adding profile-specific rejection
logic. A profile branch should not become a public read/write support rule until its source is
separated into one of these buckets:

- Manual-derived: the MELSEC manual describes the command shape, device form, or applicability.
- Library policy: a local API/configuration decision, not a manual term.
- Hardware observation: repeatable target evidence, but still target/module dependent.
- Manual audit required: do not treat it as a final support or reject contract.

| Branch or rule | Source status | Maintainer note |
| --- | --- | --- |
| Canonical strings such as `melsec:iq-r`, `melsec:qcpu`, and `melsec:lcpu` | Library policy | The manuals describe PLC families and modules, not these exact saved configuration strings. |
| `PlcProfile` to `PlcSeries` mapping | Library policy informed by manual family names | Treat it as an implementation grouping. Recheck the manuals before adding diverging behavior inside a grouped profile. |
| iQ-R vs non-iQ-R device reference widths, device code widths, and normal/extended subcommands | Manual-derived | This covers request-shape branches such as `0000/0001` vs `0002/0003` and `0080/0081` vs `0082/0083`. Keep page citations with any new change. |
| 1C/1E A-series and QnA command-family selection, including `ER/EW` and `NR/NW` paths | Manual-derived | These are command-family branches, not read/write device-support policy. |
| Link-direct `Jn\X/Y/B/W/SB/SW` request shape | Manual-derived request shape plus hardware observation | SH-080003-AF describes link-direct access in the extended-device appendix. Actual availability still depends on target/module/frame settings. |
| Qualified `Un\G` / `Un\HG` request shape | Manual-derived request shape plus hardware observation | SH-080003-AF describes unit access and CPU buffer access devices. The practical helper path and native-command behavior must stay separated. |
| `0x7F22` interpretation for serial module responses | Manual-derived error meaning plus local interpretation | SH-081249-L p.530 lists `7F22H` as a command error ("nonexistent command/subcommand/device specified"). The exact cause for a given trace is still a diagnostic conclusion. |
| `S` device over serial MC | Manual-derived plus hardware observation | `S` is absent from the SH-080003-AF MC device-code list (p.68), and p.69 says unlisted devices cannot be specified. The C24 NAKs ASCII `S***` reads with `7F22H` before CPU forwarding (verified live on iQ-R Format1/Format2, 2026-07-03). Historical binary Format5 `S` reads are treated as non-contract observations; the library rejects `S` for serial MC access. |
| Monitor request before registration | Manual-derived | SH-081249-L p.518: `7155H` is the monitor-not-registered error for a `0802` request issued before `0801` registration. Raw `0802` passes right after a registration in the same module state (verified live, Format2, 2026-07-03). |
| `0601/1601` module-buffer helper scope | Manual-derived plus hardware observation | SH-080003-AF p.155 scopes `0601/1601` to MELSEC-QnA-series special function modules. On the iQ-R rack the probe returns CPU error `0x4043`; use the native-qualified `Un\G` route for iQ-R module access. |
| `melsec:iq-r` read/write device-family list | Manual-derived request shape plus hardware observation | Maintain the current support list in the iQ-R profile specification and keep page-backed notes with future additions. |
| `melsec:iq-l` command/device layout | Manual evidence plus hardware observation and implementation policy | SH-082159CHN-F says iQ-L serial MC communication through MELSEC-L serial modules must use MELSEC-Q/L subcommands, not iQ-R subcommands. Keep the public profile separate. Current implementation uses Q/L-compatible serial MC request shapes and rejects the long timer/counter family, `LZ`, `RD`, and `Un\HG` for iQ-L serial MC. |
| FX5 / MELSEC iQ-F MC protocol layout | Manual-derived request shape plus hardware observation | SH-082624-J describes FX5 MC protocol with subcommand bit fields for data size, device-reference width, and device-memory extension. Keep FX5/iQ-F separate from Q/L and iQ-R. |
| `melsec:qcpu` and `melsec:lcpu` device-family lists | Manual-derived request shape plus hardware observation | Keep Q and L as separate public profiles even though both map to the Q/L serial request-shape branch. |
| `LZ` profile support | Manual-derived request shape plus hardware observation | Keep `LZ` supported only on profiles whose specifications include a native random double-word route. It is supported for iQ-R and iQ-F, and rejected for Q, L, and iQ-L. |
| iQ-R binary link-direct Q/L wire body exception | Hardware observation | Keep the exception isolated to link-direct native traffic; it is not a public combined Q/L profile path. |
| 4C ASCII Format4 native extended-access shape for `Jn\...` and `Un\...` | Manual-derived request shape plus hardware verification | SH-080003-AF p.430-431 requires the trailing device-modification field in ASCII extended-device references. Keep that field in the encoder; reads/writes are verified on iQ-R (`R120PCPU`) and Q/L (`Q06UDVCPU`). |
| MC protocol format selection | Hardware observation plus serial-module configuration discipline | The serial module is configured for one MC protocol format at a time. Diagnose Format4/Format5 mismatches as frame-mode configuration first; do not treat ordinary Format4 read success/failure as remote-password evidence. |
| Remote RESET (`1006`) no-response-timeout-as-success policy | Manual-derived | SH-080003-AF p.173 states that on remote RESET the access target is reset, so the response message may not be returned. Accept both a normal response and a pure no-response after TX as success; surface explicit error responses (non-STOP state, reset prohibited by parameter) as errors. A no-response is indistinguishable from a link failure at protocol level; confirm with a post-reset read-only check after the reset settles. |
| Separate public Q, L, iQ-L, and iQ-F profiles | Library policy plus target-family risk control | Do not reintroduce a combined Q/L saved profile. Keep profile names aligned with the selected target family. |

## Address and Device Parsing

- `ZR` is decimal on the validated serial targets in this repo.
- `R` and `ZR` share the same internal register space on the validated `RJ71C24-R2` target.
- `X`, `Y`, `B`, `W`, `SB`, `SW`, `DX`, and `DY` use hexadecimal address parsing.
- `M`, `L`, `SM`, `F`, `V`, `D`, `SD`, `RD`, `Z`, `R`, `ZR`, `LTN`, `LSTN`, `LCN`, and `LZ`
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
- For `melsec:iq-f`, do not expose `LTS/LTC/LTN/LSTS/LSTC/LSTN` as serial MC supported devices.
- Treat `S` as unsupported for serial MC access in every profile.

## Link-Direct Access

- Link-direct `Jn\...` helpers intentionally accept only `X/Y/B/SB` bit devices and `W/SW` word devices.
- Do not map `LCS/LCC` long-state names onto link-direct access; use the long-state helper for those devices.

## Qualified Access

- `G` and `HG` are not standalone plain-device support entries in any profile.
- Use qualified forms such as `U...\\G...` and `U...\\HG...` only when the selected profile supports them.
- Keep route selection profile-specific. Do not conflate the `0601/1601` qualified-buffer helper path with the `0401/1401` native-qualified path.

## FX5U Serial Scope

- On iQ-F serial `3C/4C`, treat `0801/0802` as unsupported.
- On iQ-F serial, keep `DX`, `DY`, `V`, and `ZR` out of the supported profile surface.

## Validation Discipline

- Check manual-backed request shape first.
- Then compare the local codec path.
- Then use hardware to break ties.
- Do not treat unsupported access paths as active holds.
