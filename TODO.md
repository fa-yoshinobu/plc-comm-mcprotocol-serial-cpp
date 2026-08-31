# TODO

Current active TODOs only.

## Current Status

Seven approved naming decisions are recorded as `MCS-API-NAME-001` through
`MCS-API-NAME-007`. Five additive groups, totaling nine provisional methods, are candidates for the
later cross-library review and are not approved for implementation. Two technical investigations
also remain open.

## MCS-API-NAME-001: Use OS-neutral names for the host serial API (approved 2026-08-31)

### Implementation scope

- Public host types, declarations, definitions, generated API reference, examples, user guides,
  migration notes, changelog, and focused source-compatibility tests.

### Target contract

- Canonical public names are `HostSerialConfig`, `HostSerialPort`, and `HostSyncClient` because the
  same host API supports Windows and POSIX systems.
- Existing `PosixSerialConfig`, `PosixSerialPort`, and `PosixSyncClient` names remain available as
  deprecated compatibility aliases for one compatibility release.
- This naming change does not alter serial settings, request construction, command/subcommand,
  device/address encoding, response handling, timeout behavior, or any PLC communication behavior.

### Compatibility impact

New source should use the `Host*` names. Existing source using the `Posix*` names continues to
compile during the compatibility period with a deprecation diagnostic, then migrates to the
corresponding `Host*` name.

### Machine-verifiable acceptance criteria

1. All three canonical `Host*` types compile and provide the current public behavior on Windows and
   POSIX builds.
2. Each old `Posix*` type resolves to its corresponding `Host*` type during the compatibility period
   and is marked deprecated.
3. Public documentation and examples use only the canonical `Host*` names except in migration notes.
4. Existing protocol, transport, timeout, and wire-behavior tests pass without behavioral changes.

### Completion checklist

- [ ] Implementation completed in this repository.
- [ ] Tests added or updated for every acceptance criterion.
- [ ] Relevant static checks, unit tests, integration tests, examples, and package/build checks passed.
- [ ] Codex self-review completed against the approved contract and cross-language consistency requirements.
- [ ] Live PLC verification disposition recorded; no live communication is expected for a type-name-only change.
- [ ] Documentation, migration notes, changelog, and generated API reference agree with the implementation.
- [ ] Final acceptance criteria verified and the item marked complete.

## MCS-API-NAME-002: Use `qualified_buffer` consistently (approved 2026-08-31)

### Implementation scope

- Public asynchronous client methods, synchronous host methods, bit-in-word helper names, generated
  API reference, examples, user guides, migration notes, changelog, and focused compatibility tests.

### Target contract

- The `Un\\G` and `Un\\HG` qualified-buffer route uses `qualified_buffer` consistently across the
  public API.
- Canonical method names are `async_qualified_buffer_batch_read_words`,
  `async_qualified_buffer_batch_write_words`, `read_qualified_buffer_words`,
  `write_qualified_buffer_words`, and `write_qualified_buffer_bit_in_word`.
- Existing `begin_qualified_buffer` remains unchanged.
- Existing `async_extended_batch_*` and `*_native_qualified_*` names remain available as deprecated
  compatibility aliases for one compatibility release.
- The rename does not change request construction, command/subcommand, device/address encoding,
  response handling, timeout behavior, or PLC communication behavior.

### Compatibility impact

New source uses the canonical `qualified_buffer` names. Existing source using the replaced names
continues to compile during the compatibility period with a deprecation diagnostic.

### Machine-verifiable acceptance criteria

1. Every canonical name calls the same implementation and produces the same request as its replaced
   name for identical input.
2. Each replaced name remains as a deprecated compatibility alias for one compatibility release.
3. Public documentation and examples use only `qualified_buffer` except in migration notes.
4. Existing codec, client, host, and wire-vector tests pass without behavioral changes.

### Completion checklist

- [ ] Implementation completed in this repository.
- [ ] Tests added or updated for every acceptance criterion.
- [ ] Relevant static checks, unit tests, integration tests, examples, and package/build checks passed.
- [ ] Codex self-review completed against the approved contract and cross-language consistency requirements.
- [ ] Live PLC verification disposition recorded; no live communication is expected for a method-name-only change.
- [ ] Documentation, migration notes, changelog, and generated API reference agree with the implementation.
- [ ] Final acceptance criteria verified and the item marked complete.

## MCS-API-NAME-003: Put the operation first in synchronous direct extended file-register names (approved 2026-08-31)

### Implementation scope

- The three synchronous high-level host methods, their declarations and definitions, generated API
  reference, examples, user guides, migration notes, changelog, and focused compatibility tests.
- Asynchronous client methods and low-level codec functions are outside this decision and retain
  their existing names.

### Target contract

- Canonical synchronous method names are `read_direct_extended_file_register_words`,
  `write_direct_extended_file_register_words`, and
  `write_direct_extended_file_register_bit_in_word`.
- Existing `direct_read_extended_file_register_words`,
  `direct_write_extended_file_register_words`, and
  `direct_write_extended_file_register_bit_in_word` remain available as deprecated compatibility
  wrappers for one compatibility release.
- The rename does not change arguments, return values, request construction, command/subcommand,
  device/address encoding, response handling, timeout behavior, or PLC communication behavior.

### Compatibility impact

New synchronous source uses the operation-first names, matching the other synchronous `read_*` and
`write_*` methods. Existing source using the direct-first names continues to compile during the
compatibility period with a deprecation diagnostic.

### Machine-verifiable acceptance criteria

1. Each canonical method has the same signature and calls the same implementation as its replaced
   method.
2. For identical input, each canonical and deprecated name produces the same request and result.
3. Each replaced name remains as a deprecated compatibility wrapper for one compatibility release.
4. Public documentation and examples use only the canonical operation-first names except in
   migration notes.
5. Existing codec, client, host, and wire-vector tests pass without behavioral changes.

### Completion checklist

- [ ] Implementation completed in this repository.
- [ ] Tests added or updated for every acceptance criterion.
- [ ] Relevant static checks, unit tests, integration tests, examples, and package/build checks passed.
- [ ] Codex self-review completed against the approved contract and cross-language consistency requirements.
- [ ] Live PLC verification disposition recorded; no live communication is expected for a method-name-only change.
- [ ] Documentation, migration notes, changelog, and generated API reference agree with the implementation.
- [ ] Final acceptance criteria verified and the item marked complete.

## MCS-API-NAME-004: Name the long timer/counter state-read API explicitly (approved 2026-08-31)

### Implementation scope

- Both synchronous high-level host overloads currently named `read_long_state_bits`, their
  declarations and definitions, diagnostics that direct callers to the helper, generated API
  reference, examples, user guides, migration notes, changelog, and focused compatibility tests.

### Target contract

- The canonical name for both overloads is `read_long_timer_counter_state_bits`.
- The canonical method accepts the same supported state-device families as the current method:
  `LTS`, `LTC`, `LSTS`, `LSTC`, `LCS`, and `LCC`.
- Both existing `read_long_state_bits` overloads remain available as deprecated compatibility
  wrappers for one compatibility release.
- The rename does not change arguments, return values, aggregation, request construction,
  command/subcommand, device/address encoding, response handling, timeout behavior, or PLC
  communication behavior.

### Compatibility impact

New synchronous source uses the explicit long timer/counter name. Existing source using
`read_long_state_bits` continues to compile during the compatibility period with a deprecation
diagnostic.

### Machine-verifiable acceptance criteria

1. Both canonical overloads have the same signatures and call the same implementation as their
   replaced overloads.
2. For every supported state-device family, the canonical and deprecated names produce the same
   request sequence and result for identical input.
3. Both replaced overloads remain as deprecated compatibility wrappers for one compatibility
   release.
4. Public diagnostics, documentation, and examples use the canonical name except in migration
   notes.
5. Existing codec, client, host, aggregate-read, and wire-vector tests pass without behavioral
   changes.

### Completion checklist

- [ ] Implementation completed in this repository.
- [ ] Tests added or updated for every acceptance criterion.
- [ ] Relevant static checks, unit tests, integration tests, examples, and package/build checks passed.
- [ ] Codex self-review completed against the approved contract and cross-language consistency requirements.
- [ ] Live PLC verification disposition recorded; no live communication is expected for a method-name-only change.
- [ ] Documentation, migration notes, changelog, and generated API reference agree with the implementation.
- [ ] Final acceptance criteria verified and the item marked complete.

## MCS-API-NAME-005: Identify user-frame registration management explicitly (approved 2026-08-31)

### Implementation scope

- The three asynchronous client methods and three synchronous high-level host methods that read,
  write, or delete user-frame registration data; their three public request types; declarations,
  definitions, generated API reference, examples, user guides, migration notes, changelog, and
  focused compatibility tests.
- `UserFrameRegistrationData` remains unchanged.
- Low-level codec function names are outside this decision and retain their existing names.

### Target contract

- Canonical asynchronous method names are `async_read_user_frame_registration`,
  `async_write_user_frame_registration`, and `async_delete_user_frame_registration`.
- Canonical synchronous method names are `read_user_frame_registration`,
  `write_user_frame_registration`, and `delete_user_frame_registration`.
- Canonical request type names are `UserFrameRegistrationReadRequest`,
  `UserFrameRegistrationWriteRequest`, and `UserFrameRegistrationDeleteRequest`.
- Existing method and request type names remain available as deprecated compatibility wrappers or
  aliases for one compatibility release.
- The rename does not change arguments, return values, registration data, request construction,
  command/subcommand, response handling, timeout behavior, or PLC communication behavior.

### Compatibility impact

New source names the stored user-frame registration explicitly instead of appearing to transmit or
receive a user frame. Existing source using `*_user_frame` methods or `UserFrame*Request` types
continues to compile during the compatibility period with a deprecation diagnostic.

### Machine-verifiable acceptance criteria

1. Each canonical method and request type has the same public contract and uses the same
   implementation as its replaced name.
2. For identical input, each canonical and deprecated method produces the same request and result.
3. Every replaced method and request type remains as a deprecated compatibility wrapper or alias
   for one compatibility release.
4. Public documentation and examples use only the canonical registration names except in migration
   notes.
5. Existing codec, client, host, and wire-vector tests pass without behavioral changes.

### Completion checklist

- [ ] Implementation completed in this repository.
- [ ] Tests added or updated for every acceptance criterion.
- [ ] Relevant static checks, unit tests, integration tests, examples, and package/build checks passed.
- [ ] Codex self-review completed against the approved contract and cross-language consistency requirements.
- [ ] Live PLC verification disposition recorded; no live communication is expected for a naming-only change.
- [ ] Documentation, migration notes, changelog, and generated API reference agree with the implementation.
- [ ] Final acceptance criteria verified and the item marked complete.

## MCS-API-NAME-006: Align the standard monitor lifecycle names (approved 2026-08-31)

### Implementation scope

- The standard-device monitor registration and execution methods on the asynchronous client and
  synchronous high-level host facade, their declarations and definitions, diagnostics, generated
  API reference, examples, user guides, migration notes, changelog, and focused compatibility tests.
- Single-item registration helpers and extended file-register or link-direct monitor routes are
  outside this decision and retain their existing names.

### Target contract

- Canonical asynchronous method names are `async_register_monitor_devices` and
  `async_run_monitor_cycle`.
- Canonical synchronous method names are `register_monitor_devices` and `run_monitor_cycle`.
- Existing `async_register_monitor`, `async_read_monitor`, `register_monitor`, and `read_monitor`
  remain available as deprecated compatibility wrappers for one compatibility release.
- Registration remains a separate operation from each monitor cycle; no method implicitly performs
  the other operation.
- The rename does not change arguments, return values, retained registration state, request
  construction, command/subcommand, response handling, timeout behavior, or PLC communication
  behavior.

### Compatibility impact

New source uses the same `register_monitor_devices` and `run_monitor_cycle` lifecycle terminology as
the other SLMP-family libraries. Existing source using the replaced names continues to compile
during the compatibility period with a deprecation diagnostic.

### Machine-verifiable acceptance criteria

1. Each canonical method has the same signature and uses the same implementation as its replaced
   method.
2. For identical registration and output storage, canonical and deprecated names produce the same
   registration request, monitor-cycle request, retained state, and result.
3. Every replaced method remains as a deprecated compatibility wrapper for one compatibility
   release.
4. Public documentation and examples describe registration followed by one or more explicit monitor
   cycles and use only the canonical names except in migration notes.
5. Existing codec, client, host, monitor-state, and wire-vector tests pass without behavioral
   changes.

### Completion checklist

- [ ] Implementation completed in this repository.
- [ ] Tests added or updated for every acceptance criterion.
- [ ] Relevant static checks, unit tests, integration tests, examples, and package/build checks passed.
- [ ] Codex self-review completed against the approved contract and cross-language consistency requirements.
- [ ] Live PLC verification disposition recorded; no live communication is expected for a naming-only change.
- [ ] Documentation, migration notes, changelog, and generated API reference agree with the implementation.
- [ ] Final acceptance criteria verified and the item marked complete.

## MCS-API-NAME-007: Put the operation first in synchronous random-access names (approved 2026-08-31)

### Implementation scope

- The ten synchronous high-level host methods for standard and extended file-register random access,
  their declarations and definitions, diagnostics, generated API reference, examples, user guides,
  migration notes, changelog, and focused compatibility tests.
- Asynchronous client methods and low-level codec functions are outside this decision and retain
  their existing names.

### Target contract

- Canonical read method names are `read_random`, `read_random_word`, and `read_random_dword`.
- Canonical write method names are `write_random_words`, `write_random_dwords`,
  `write_random_word`, `write_random_dword`, `write_random_bits`, `write_random_bit`, and
  `write_random_extended_file_register_words`.
- Existing `random_read*` and `random_write*` method names remain available as deprecated
  compatibility wrappers for one compatibility release.
- The rename does not change arguments, return values, request construction, command/subcommand,
  device/address encoding, response handling, timeout behavior, or PLC communication behavior.

### Compatibility impact

New synchronous source uses the operation-first `read_random*` and `write_random*` convention used
by the other protocol libraries. Existing source using random-first names continues to compile
during the compatibility period with a deprecation diagnostic.

### Machine-verifiable acceptance criteria

1. Each canonical method has the same signature and uses the same implementation as its replaced
   method.
2. For identical input, each canonical and deprecated name produces the same request and result.
3. Every replaced method remains as a deprecated compatibility wrapper for one compatibility
   release.
4. Public documentation and examples use only the canonical operation-first names except in
   migration notes.
5. Existing codec, client, host, random-access validation, and wire-vector tests pass without
   behavioral changes.

### Completion checklist

- [ ] Implementation completed in this repository.
- [ ] Tests added or updated for every acceptance criterion.
- [ ] Relevant static checks, unit tests, integration tests, examples, and package/build checks passed.
- [ ] Codex self-review completed against the approved contract and cross-language consistency requirements.
- [ ] Live PLC verification disposition recorded; no live communication is expected for a naming-only change.
- [ ] Documentation, migration notes, changelog, and generated API reference agree with the implementation.
- [ ] Final acceptance criteria verified and the item marked complete.

## Candidate API additions for the later cross-library review

Status: `candidate`. These entries record existing asynchronous capabilities that could be exposed
through the synchronous high-level facade. They are not approved for implementation. Method and
type names below are provisional and do not establish the cross-library target contract.

| ID | Provisional synchronous API | Existing implementation to wrap | Provisional added types |
|---|---|---|---|
| `MCS-API-ADD-001` | `read_random_link_direct_words`, `write_random_link_direct_words`, `write_random_link_direct_bits` | `async_link_direct_random_read`, `async_link_direct_random_write_words`, `async_link_direct_random_write_bits` | `LinkDirectRandomReadWordSpec`, `LinkDirectRandomWriteWordSpec`, `LinkDirectRandomWriteBitSpec` |
| `MCS-API-ADD-002` | `self_test_loopback` | `async_loopback` | None |
| `MCS-API-ADD-003` | `read_block`, `write_block` | `async_multi_block_read`, `async_multi_block_write` | None; reuse existing request/output types |
| `MCS-API-ADD-004` | `read_link_direct_block`, `write_link_direct_block` | `async_link_direct_multi_block_read`, `async_link_direct_multi_block_write` | None; reuse existing request/output types |
| `MCS-API-ADD-005` | `register_link_direct_monitor` (provisional name) | `async_link_direct_register_monitor` | None; reuse `LinkDirectMonitorRegistration` |

Candidate total: nine synchronous methods and three provisional convenience spec types.

All candidates use existing commands, encoders, parsers, and asynchronous operations. They do not
propose a new command/subcommand, fallback route, implicit request splitting, or automatic retry.
Host-buffer and module-buffer synchronous wrappers are intentionally not candidates because their
high-level need has not been established.

- [ ] During the full cross-library review, compare the user-visible operation granularity across
  all libraries instead of matching method counts.
- [ ] Decide add or omit for each candidate group, one group at a time.
- [ ] For every selected group, separately approve the canonical name, input/output contract,
  compatibility impact, and machine-verifiable acceptance criteria.
- [ ] Do not implement any candidate before that review and approval.

## Open: 1C direct extended file-register profile validation (2026-08-31)

- [ ] Distinguish `melsec:ana-anu` from `melsec:qna` when validating the 1C `NR`/`NW` direct
  extended file-register commands. The manual defines `NR`/`NW` as AnA/AnUCPU common commands and
  marks QnA CPU access unavailable, but the current implementation accepts both profiles because
  `PlcSeries::AnA_AnU` aliases `PlcSeries::QnA`.
- Affected public API surfaces: asynchronous and synchronous direct read, direct write, and direct
  bit-in-word read-modify-write (six APIs in total).
- Acceptance: `melsec:ana-anu` produces the documented `NR`/`NW` request, `melsec:qna` is rejected
  before transmission with the correct reason, and codec/client tests cover both profiles and all
  affected API paths.
- Manual evidence: *MELSEC Communication Protocol Reference Manual* SH-080003-AF, 1C command table
  and Appendix 6 access-target table (printed pages 337, 340, and 464).

## Open: `LT/LST` state read at the final three device numbers (2026-08-31)

- [ ] Determine whether the `0x4031` response is caused by `RJ71C24-R2` firmware, `R120PCPU`
  firmware, or another receiver-side MC serial limitation. Do not classify it as a library defect
  or implement a library workaround until that distinction is supported by evidence.
- The C4 binary Format 5 request shape matches the MC Protocol manual: command/subcommand
  `0401/0002`, head device `LTN N` or `LSTN N`, and four words for one `LT/LST` state record.
- On the tested `R120PCPU` + `RJ71C24-R2` serial path, heads `262128` through `262140` completed
  normally. Heads `262141`, `262142`, and `262143` returned `0x4031` when the point count was four.
  The boundary matches a receiver-side calculation of `head + point_count - 1`: `262140 + 4 - 1`
  is the configured maximum `262143`, while the next head exceeds it.
- `LTN/LSTN 262128` through `262143` were independently accepted by random read/write commands,
  so the logical devices exist. Over SLMP Ethernet, the exact `0401/0002`, `LTN262141` through
  `LTN262143`, count `4` requests all returned `0x0000`; a separate request with head `LTN262128`
  and count `64` also read all 16 logical timers through `LTN262143`. These comparisons point to
  the serial receiver path rather than the public API's address mapping or the CPU device assignment.
- This may be a PLC/module firmware defect or limitation and therefore may not be repairable in this
  library. If confirmed, retain the observation as a target limitation unless a separately approved,
  technically valid workaround exists.

## Notes

- Do not add unsupported access paths here.
- Do not add TODOs for manual families that are explicitly not needed by this library. The current
  omitted-family policy is documented in [MANUAL_DERIVED_RULES.md](docsrc/maintainer/MANUAL_DERIVED_RULES.md).
- Do not mark a target-dependent PLC rejection as an implementation bug unless request-shape tests
  or new hardware evidence point to the encoder/client code.
- For long timer / long retentive timer contact+coil devices, use the structured `LTN/LSTN`
  `0401` path instead of direct probes.
