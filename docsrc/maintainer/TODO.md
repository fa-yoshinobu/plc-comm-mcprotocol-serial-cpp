# TODO

Current active follow-up items only.

## Current Status

| Area | Status | Notes |
|---|---|---|
| Implementation gaps | none open | 2026-07-02: ASCII device extension specification encoder fix was applied and covered by request-shape tests. |
| Command-family holds | none open | No whole native command family is blocked on the currently validated targets. |
| Target-dependent validation | two open | Format4 ASCII native extended access needs a live hardware recheck after the encoder fix; `RJ71C24-R2` remote password unlock/lock still needs a focused hardware recheck. |
| Specification-policy investigations | one open | `remote_reset` no-response timeout is currently treated as success; verify whether this is valid MC protocol behavior or should change. |

## Active Follow-Up

- [ ] **ASCII device extension specification: hardware recheck after encoder fix**

| Field | Detail |
|---|---|
| Type | Fixed encoder bug; hardware recheck pending. |
| Root cause | `append_link_direct_device_reference_ascii` and `append_extended_device_reference_ascii` in `src/codec.cpp` end the extended device specification at the device number. SH-080003-AF p.430-431 requires a trailing device-modification field (`000` for Q/L subcommands, `0000` for iQ-R subcommands) before the points field. |
| Evidence | Manual example `J1\W100` (`0080 00 J001 000 W* 000100 000`) vs observed `J1\SB10` frame (`0083 00 J001 0000 SB** 0000000010` + points, no trailing `0000`). `7F22H` is the C24's own command-parse rejection (SH-081249-L, PRO category). Binary Format5 encoders emit the full layout and passed on the same targets. |
| Fix applied | `src/codec.cpp` now appends the trailing `append_device_modification_ascii` after the device number in both ASCII extended-reference builders. `tests/codec_tests.cpp` covers the Q/L manual example and iQ-R `Jn\...` / `Un\G` shapes. |
| Next action | Re-run the Format4 ASCII recheck (`J1\SB10`, `U3E0\G10`, `U2\G1000`) on live targets. |
| Evidence file | [FORMAT4_ASCII_NATIVE_EXTENSION_ANALYSIS.md](FORMAT4_ASCII_NATIVE_EXTENSION_ANALYSIS.md) |
| Close when | The previously failing native extended reads pass on hardware in ASCII Format4, or any remaining target-side rejection is separately explained with new raw TX/RX evidence. |

- [ ] **Remote RESET no-response timeout policy**

| Field | Detail |
|---|---|
| Type | Specification-policy investigation, not yet classified as an implementation bug. |
| Current behavior | `remote_reset` treats a pure response timeout with no received bytes as success because some targets may reset before returning a response. |
| Affected code | `src/client.cpp` maps the no-response timeout to `StatusCode::Ok` with `Remote RESET completed without a response`; headers and CLI help describe the same behavior. |
| Risk | A true communication failure can look like a successful reset if the target did not actually receive or execute the command. |
| Next action | Re-check Mitsubishi MC protocol documentation and hardware behavior for Remote RESET. Decide whether no-response success is a required specification, target-dependent behavior that needs an explicit option, or a behavior that should become timeout/error. |
| Close when | The chosen policy is backed by manual evidence or repeatable hardware evidence, and the client, CLI, tests, and user documentation are updated consistently. |

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
  omitted-family policy is documented in Manual command coverage.
- Do not mark a target-dependent PLC rejection as an implementation bug unless request-shape tests
  or new hardware evidence point to the encoder/client code.
- For long timer / long retentive timer contact+coil devices, use the structured `LTN/LSTN`
  `0401` path instead of direct probes.
