# Manual-Derived Rules

Keep these rules aligned with the current Mitsubishi serial manuals and validated hardware results.

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
- Do not treat direct `LTS/LTC/LSTS/LSTC` probes as part of the supported interface.

## Qualified Access

- `U...\\G...` and `U...\\HG...` are supported only through the helper path built on `0601/1601`.
- Native qualified commands remain diagnostic-only and are not part of the supported workflow.

## FX5U Serial Scope

- On `FX5UC-32MT/D` serial `3C/4C`, treat `0801/0802` as unsupported.
- On the same path, keep `DX`, `DY`, `V`, and `ZR` out of the validated contiguous subset.

## Validation Discipline

- Check manual-backed request shape first.
- Then compare the local codec path.
- Then use hardware to break ties.
- Do not treat unsupported access paths as active holds.
