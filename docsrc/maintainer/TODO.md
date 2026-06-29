# TODO

Current active follow-up items only. As of 2026-06-13, there are no known
codec/client/CLI implementation gaps waiting for code changes.

## Current Status

| Area | Status | Notes |
|---|---|---|
| Implementation gaps | none open | `1630` / `1631` encoder, client, sync wrapper, CLI, and tests already exist. |
| Command-family holds | none open | No whole native command family is blocked on the currently validated targets. |
| Target-dependent validation | one open | `RJ71C24-R2` remote password unlock/lock still needs a focused hardware recheck. |
| Specification-policy investigations | one open | `remote_reset` no-response timeout is currently treated as success; verify whether this is valid MC protocol behavior or should change. |

## Active Follow-Up

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
