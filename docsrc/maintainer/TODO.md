# TODO

Current active follow-up items only.

## Current Status

| Area | Status | Notes |
|---|---|---|
| Implementation gaps | none open | The Format1/2 ASCII link-direct partial-response decoder crash is fixed and covered by regression tests; see [FORMAT1_ASCII_IQR_LINK_DIRECT_INVESTIGATION.md](FORMAT1_ASCII_IQR_LINK_DIRECT_INVESTIGATION.md) and [FORMAT2_ASCII_IQR_LINK_DIRECT_INVESTIGATION.md](FORMAT2_ASCII_IQR_LINK_DIRECT_INVESTIGATION.md). |
| Command-family holds | none open | No whole native command family is blocked on the currently validated targets. |
| Target-dependent validation | one open | `RJ71C24-R2` remote password unlock/lock still needs a focused hardware recheck. |
| Specification-policy investigations | none open | `remote_reset` no-response-as-success is closed 2026-07-02 as manual-derived behavior (SH-080003-AF p.173); rule recorded in [MANUAL_DERIVED_RULES.md](MANUAL_DERIVED_RULES.md). |

## Active Follow-Up

- [ ] **RJ71C24-R2 remote password (`1630` / `1631`)**

| Field | Detail |
|---|---|
| Type | Target-dependent hardware validation, not a known request encoder bug. |
| Focused setup | `RJ71C24-R2`, `R08CPU`, `COM3`, `28800 / 8E2`, `c4-binary`, station `0`, sum-check on, `--plc-profile melsec:iq-r`. |
| Known evidence | Historical `6`-character `unlock` returned `0x7FE7`; `lock` and longer `unlock` attempts returned `0x7F22`. On 2026-06-12, configured password `123456melsec` returned `0x7F22` for both `unlock` and `lock`; changed password `abcdef1` returned `0x7FE7` for `unlock`. |
| Link sanity | `cpu-model` and `read-words D0 1` still passed after each failure. |
| Next action | Re-run only after the target-side remote password setting and CPU/module state are known. Capture raw TX/RX frames and immediate read-only sanity results. |
| Helper | [scripts/recheck_remote_password.ps1](../../scripts/recheck_remote_password.ps1) runs read-only sanity by default and requires `-AllowRemotePasswordCommands` before sending `1630` / `1631`. |
| Evidence file | Keep setup, raw frames, and interpretation in [TARGET_DEPENDENT_NATIVE_COMMANDS.md](TARGET_DEPENDENT_NATIVE_COMMANDS.md). |
| Close when | A known target-side password configuration produces either a successful unlock/lock sequence or a deterministic PLC end code that is explained by target settings and documented in the validation reports. |

## Notes

- Do not add unsupported access paths here.
- Do not add TODOs for manual families that are explicitly not needed by this library. The current
  omitted-family policy is documented in [MANUAL_DERIVED_RULES.md](MANUAL_DERIVED_RULES.md).
- Do not mark a target-dependent PLC rejection as an implementation bug unless request-shape tests
  or new hardware evidence point to the encoder/client code.
- For long timer / long retentive timer contact+coil devices, use the structured `LTN/LSTN`
  `0401` path instead of direct probes.
