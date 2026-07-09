# MC Serial Error Code Evidence

This page records live PLC/module observations for MC Protocol Serial response
codes. It is maintainer evidence, not a public error-code promise. Do not
publish unobserved code meanings from this file without a separate manual or
hardware decision.

## 2026-07-04: iQ-F / FX5, CLI `c4-ascii-f1`

| Field | Value |
| --- | --- |
| Profile | `melsec:iq-f` |
| Link | `COM4`, `19200bps`, `8E1` |
| MC frame selected in CLI | `c4-ascii-f1` |
| Station | `0` |
| Sum-check | off |
| Scope | Error-code investigation only; not a broad ASCII Format1 support decision. |

Sanity checks:

- `0401/0000` read of `D100`, 1 point, returned success.
- `1401/0000` write of `D100=0x5A3C` returned success, and a follow-up read
  returned `0x5A3C`.

Observed abnormal responses:

| Request shape | Target | Observed response | Note |
| --- | --- | --- | --- |
| `9999/0000` raw command | `D100`, 1 point | `0x7E40` | Invalid command probe. |
| `0401/FFFF` raw subcommand | `D100`, 1 point | `0x7E40` | Invalid subcommand probe. |
| `0802/0000` raw monitor read without prior registration | none | `0x7E40` | The tested `c4-ascii-f1` path does not expose the expected monitor path on this setup. |
| `0401/0000` raw read | `DX0`, 1 point | `0x7F21` | Unsupported device or route on this setup. |
| `1401/0000` raw word write | `DX0=0x5A3C`, 1 point | `0x7F21` | Same code as the matching `DX0` read probe. |
| `0401/0000` raw read | `S0`, 1 point | success | Logged observation only; do not change the public `S` support policy without a dedicated profile/manual decision. |
| `c1-ascii-f1` with sum-check enabled | `WR0` read of `D100`, correct sum-check `2B` | success, `0x0000`, response sum `AF` |
| `c4-ascii-f1` with sum-check enabled | `0401/0000` read of `D100`, correct sum-check `4E` | success, `0x0000`, response sum `3C` |
| `c1-ascii-f1` with sum-check enabled | `WR0` read of `D100`, intentionally bad sum-check `00` | NAK `0x02` |
| `c4-ascii-f1` with sum-check enabled | `0401/0000` read of `D100`, intentionally bad sum-check `00` | `0x7F24` |
| `c1-ascii-f1` with sum-check enabled | `WW0` write of `D100=0x1357`, 1 point | success |
| `c1-ascii-f1` with sum-check enabled | `WR0` readback of `D100`, 1 point | success, `0x1357`, response sum `BF` |
| `c4-ascii-f1` with sum-check enabled | `0401/0000` readback of `D100`, 1 point | success, `0x1357`, response sum `4C` |
| `c4-ascii-f1` with sum-check enabled | `1401/0000` write of `D100=0x2468`, 1 point | success |
| `c4-ascii-f1` with sum-check enabled | `0401/0000` readback of `D100`, 1 point | success, `0x2468`, response sum `50` |
| `c1-ascii-f1` with sum-check enabled | `WR0` readback of `D100`, 1 point | success, `0x2468`, response sum `C3` |

Raw logs were captured under the workspace evidence directory:

- `D:\APP\evidence\mcserial_error_codes\20260704_iqf_com4_format1_normal_read.log`
- `D:\APP\evidence\mcserial_error_codes\20260704_iqf_com4_format1_abnormal_7fxx.log`
- `D:\APP\evidence\mcserial_error_codes\20260704_iqf_com4_format1_raw_7fxx_probe2.log`
- `D:\APP\evidence\mcserial_error_codes\20260704_iqf_com4_format1_write_and_dx_raw.log`
- `D:\APP\evidence\mcserial_error_codes\20260704_iqf_com4_format1_sumcheck_enabled.log`
- `D:\APP\evidence\mcserial_error_codes\20260704_iqf_com4_format1_sumcheck_write_readback.log`

## 2026-07-04: iQ-F / FX5, CLI `c1-ascii-f1`

| Field | Value |
| --- | --- |
| Profile selected in CLI | `melsec:a` |
| Physical target | iQ-F / FX5 bench |
| Link | `COM4`, `19200bps`, `8E1` |
| MC frame selected in CLI | `c1-ascii-f1` |
| Station / PC | `00` / `FF` |
| Sum-check | off |
| Scope | 1C Format1 availability and representative NAK-code observations. |

Sanity check:

- `WR0` read of `D100`, 1 point, sent as `.00FFWR0D010001`, returned
  `.00FF5A3C.`. The value matches the prior `D100=0x5A3C` write/read sanity
  check.

Observed abnormal responses:

| Request shape | Target | Observed response | Note |
| --- | --- | --- | --- |
| `ZZ0` raw command | `D100`, 1 point | NAK `0x03` | Nonexistent 1C command probe. |
| `WR0` raw read | invalid device `@0100`, 1 point | NAK `0x07` | Invalid 1C device-code probe. |
| `WR0` raw read | `D100`, 0 points | NAK `0x06` | Invalid point-count probe. |

Raw logs:

- `D:\APP\evidence\mcserial_error_codes\20260704_iqf_com4_c1_ascii_f1_d100_read.log`
- `D:\APP\evidence\mcserial_error_codes\20260704_iqf_com4_c1_ascii_f1_abnormal_error_codes.log`

## Additional 1C Error-Code Evidence Policy

The `c1-ascii-f1` probes confirm this bench can accept the library `C1`
Format1 read path and can return representative 1C NAK codes.

The Mitsubishi manuals also describe 1C frames as supporting protocol formats 1
and 4. If a future diagnostic case needs broader 1C coverage, investigate the
specific abnormal response with `c1-ascii-f1` or `c1-ascii-f4`, not by reusing
the `c4-ascii-f1` evidence above. This is an evidence policy, not an open TODO.

## 2026-07-04: iQ-F / FX5, Format4 Sanity Reads

After the PLC serial setting was switched to protocol format 4, read-only
sanity checks and representative abnormal probes succeeded for both CLI frame
selections:

| CLI frame | CLI profile | Request | Observed response |
| --- | --- | --- | --- |
| `c1-ascii-f4` | `melsec:a` | `WR0` read of `D100`, 1 point | success, `0x0000` |
| `c4-ascii-f4` | `melsec:iq-f` | `0401/0000` read of `D100`, 1 point | success, `0x0000` |
| `c1-ascii-f4` | raw | `ZZ0` command, `D100`, 1 point | NAK `0x03` |
| `c1-ascii-f4` | raw | `WR0` read with invalid device `@0100`, 1 point | NAK `0x07` |
| `c1-ascii-f4` | raw | `WR0` read of `D100`, 0 points | NAK `0x06` |
| `c4-ascii-f4` | raw | `9999/0000`, `D100`, 1 point | `0x7E40` |
| `c4-ascii-f4` | raw | `0401/FFFF`, `D100`, 1 point | `0x7E40` |
| `c4-ascii-f4` | raw | `0802/0000` monitor read without prior registration | `0x7E40` |
| `c4-ascii-f4` | raw | `0401/0000` read of `DX0`, 1 point | `0x7F21` |
| `c4-ascii-f4` | raw | `1401/0000` word write of `DX0=0x5A3C`, 1 point | `0x7F21` |
| `c1-ascii-f4` | raw | `WR0` read of `D100`, station changed from `00` to `01` | no response |
| `c1-ascii-f4` | raw | `WR0` read of `D100`, PC changed from `FF` to `00` | NAK `0x10` |
| `c4-ascii-f4` | raw | `0401/0000` read of `D100`, station-like header field changed from `00` to `01` | no response |
| `c4-ascii-f4` | raw | `0401/0000` read of `D100`, PC field changed from `FF` to `00` | no response |
| `c4-ascii-f4` | raw | `0401/0000` read of `D100`, destination-station-like field changed from `00` to `01` | no response |
| `c1-ascii-f4` | `melsec:a` | `WW0` write of `D100=0x3C5A`, 1 point | success |
| `c1-ascii-f4` | `melsec:a` | `WR0` readback of `D100`, 1 point | success, `0x3C5A` |
| `c4-ascii-f4` | `melsec:iq-f` | `0401/0000` readback of `D100`, 1 point | success, `0x3C5A` |
| `c4-ascii-f4` | `melsec:iq-f` | `1401/0000` write of `D100=0xA53C`, 1 point | success |
| `c4-ascii-f4` | `melsec:iq-f` | `0401/0000` readback of `D100`, 1 point | success, `0xA53C` |
| `c1-ascii-f4` | `melsec:a` | `WR0` readback of `D100`, 1 point | success, `0xA53C` |
| `c1-ascii-f4` with sum-check enabled | `melsec:a` | `WR0` read of `D100`, correct sum-check `2B` | success, `0x0000`, response sum `AF` |
| `c4-ascii-f4` with sum-check enabled | `melsec:iq-f` | `0401/0000` read of `D100`, correct sum-check `4E` | success, `0x0000`, response sum `3C` |
| `c1-ascii-f4` with sum-check enabled | raw | `WR0` read of `D100`, intentionally bad sum-check `00` | NAK `0x02` |
| `c4-ascii-f4` with sum-check enabled | raw | `0401/0000` read of `D100`, intentionally bad sum-check `00` | `0x7F24` |

This confirms that the current iQ-F bench can accept both the library `C1`
Format4 read path and the `C4` ASCII Format4 read path. The observed abnormal
responses match the earlier Format1 probes for the same request shapes.

Raw logs:

- `D:\APP\evidence\mcserial_error_codes\20260704_iqf_com4_format4_sanity_reads.log`
- `D:\APP\evidence\mcserial_error_codes\20260704_iqf_com4_format4_abnormal_error_codes.log`
- `D:\APP\evidence\mcserial_error_codes\20260704_iqf_com4_format4_station_pc_mismatch.log`
- `D:\APP\evidence\mcserial_error_codes\20260704_iqf_com4_format4_c1_write_readback.log`
- `D:\APP\evidence\mcserial_error_codes\20260704_iqf_com4_format4_c4_write_readback.log`
- `D:\APP\evidence\mcserial_error_codes\20260704_iqf_com4_format4_sumcheck_enabled_retest.log`

Earlier sum-check attempts before the final PLC-side setting adjustment are kept
as raw workspace logs, but the row above is the decision evidence for the
sum-check-enabled setup.
