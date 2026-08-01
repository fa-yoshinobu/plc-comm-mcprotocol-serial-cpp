# Cross-library contract implementation record (2026-08-01)

This record applies the approved workspace decisions to MC Protocol Serial C++. It is a GOAL-style
target and acceptance record; it is not a release-execution log. All behavior described here is
deterministically verifiable without a live PLC.

## MCS-SERIAL-DEFER-001 — Complete single-request capacity

Implementation scope: every public codec, asynchronous client, synchronous host, and caller-output
path, in ASCII and binary modes and every enabled frame family/build profile.

### Target contract

An accepted single request fits request construction, worst-case wire encoding, receive, unescape,
decode, and caller output. Binary calculations assume that every DLE-escapable byte expands. An
over-limit request returns `InvalidArgument` before observable request-state mutation; an
independently undersized caller span returns `BufferTooSmall`. No single-request API splits, retries
a smaller count, or grows fixed capacity.

Compatibility impact: nominal protocol maxima can now be rejected when the configured/build frame
capacity cannot carry their worst case. Callers issue explicitly smaller requests.

### Machine-verifiable acceptance criteria

1. ASCII/binary and each enabled frame family calculate complete request/response envelopes.
2. Binary request and response checks cover all-DLE worst-case expansion.
3. Exact capacity is accepted and capacity plus one is rejected before frame/callback/output state
   changes.
4. Every read validates caller output independently of protocol capacity.
5. Single-request operations produce at most one frame and never split or resize.

### Acceptance tracking

- [x] Implementation completed in this repository.
- [x] Tests cover exact/max-plus-one, worst-case DLE, output capacity, and no-mutation behavior.
- [x] Full/reduced/ultra, host, examples, PlatformIO consumers, and archive checks passed.
- [x] Codex diff/API/state/error self-review completed and accepted findings corrected.
- [x] Live PLC verification is not required; this is deterministic capacity arithmetic.
- [x] Documentation, migration notes, changelog, and generated API agree.
- [x] Final acceptance criteria verified.

## MCS-SERIAL-DEFER-002 — One absolute transaction deadline

Implementation scope: `MelsecSerialClient`, POSIX/Win32 transports, synchronous host wrapper, CLI,
examples, and timeout documentation.

### Target contract

One absolute deadline starts immediately before the first actual TX write and covers partial/zero
TX progress, physical drain, every RX chunk, and complete response validation. Progress never
extends it. Expiry at or beyond the boundary returns `Timeout` and requires transport reset in every
format. The removed inter-byte timeout has no compatibility alias.

Compatibility impact: transports call `notify_tx_started()` before first write and use
`write_all_until()`, `drain_tx_until()`, and `read_some_until()`. Applications remove
`inter_byte_timeout_ms` configuration and CLI options.

### Machine-verifiable acceptance criteria

1. Deadline starts at first-write notification and is not restarted at TX completion or RX chunks.
2. TX, drain, and RX use the same wrap-safe absolute deadline.
3. Exact-boundary expiry, trickle RX, partial/zero write, and drain timeout return `Timeout`.
4. Every timeout requires close/drain/reconfigure before reuse.
5. State-changing post-send timeout is `OperationOutcomeUnknown` with `cause == Timeout`.

### Acceptance tracking

- [x] Implementation completed in this repository.
- [x] Deadline, boundary, wrap, trickle, TX-complete, and structured-cause tests added.
- [x] Full host/toolchain/package checks passed.
- [x] Codex timeout/cancellation/transport self-review completed.
- [x] Live PLC verification is not required; fake time/transport evidence is authoritative.
- [x] Documentation, migration notes, changelog, examples, and generated API agree.
- [x] Final acceptance criteria verified.

## MCS-SERIAL-DEFER-006 — Busy admission and instance independence

Implementation scope: every `MelsecSerialClient::async_*` operation and request-owned state.

### Target contract

While one request is active, every colliding operation returns `Busy` before encoding or changing
the active frame, outputs, callback, expected response size, monitor metadata, or copied request
data. No queue is added. Separate client instances progress independently. Supported use does not
perform concurrent calls on the same instance from multiple threads.

Compatibility impact: overlap that previously reached operation-specific validation now receives
`Busy` first. Applications serialize one client or use separate instances.

### Machine-verifiable acceptance criteria

1. Every public async operation checks admission before request construction or state mutation.
2. A colliding write/control call preserves the first request frame, result target, and callback.
3. Rejection emits no frame and invokes no callback.
4. Two configured instances can hold and complete independent requests.

### Acceptance tracking

- [x] Implementation completed in this repository.
- [x] Cross-operation no-mutation and independent-instance tests added.
- [x] All relevant builds and tests passed.
- [x] Codex public-entry/state-transition review completed.
- [x] Live PLC verification is not required; admission is local state behavior.
- [x] Documentation, changelog, and generated API agree.
- [x] Final acceptance criteria verified.

## MCS-ERROR-DEFER-001 — Dedicated lifecycle and outcome causes

Implementation scope: status API, async client, host transports/wrapper, and CLI-facing behavior.

### Target contract

Timeout, cancellation, local close, not-connected/configured, transport, framing, parse, PLC, and
ambiguous state-changing outcomes have stable machine-readable classifications.
`OperationOutcomeUnknown` carries its originating reason in `Status::cause`; callers never parse a
message to choose recovery. Ambiguous state-changing operations are never retried automatically.

Compatibility impact: consumers can switch on `NotConnected`, `Closed`, and `cause`; generic
transport handling should be updated where it previously collapsed those states.

### Machine-verifiable acceptance criteria

1. Closed and not-connected conditions do not report generic transport errors.
2. Every post-send ambiguous state-changing failure preserves its cause.
3. Pre-send failures remain their direct cause and are not outcome-unknown.
4. Documentation maps each code to safe retry/reopen/state-resolution behavior.

### Acceptance tracking

- [x] Implementation completed in this repository.
- [x] Timeout, cancellation, transport, and outcome-cause tests updated.
- [x] All relevant builds and package checks passed.
- [x] Codex error-classification self-review completed.
- [x] Live PLC verification is not required; injected failures are direct evidence.
- [x] Documentation, changelog, and generated API agree.
- [x] Final acceptance criteria verified.

## MCS-AGGREGATE-DEFER-001 — Read-only aggregate visibility

Implementation scope: all public operations; specifically the multi-point
`PosixSyncClient::read_long_state_bits()` status-block route.

### Target contract

Every operation except the identified long-state helper is one wire request. For
`LTS`/`LTC`/`LSTS`/`LSTC`, a multi-point long-state call is an explicit read-only aggregate: it
validates and snapshots the complete contiguous plan before send, reads one independent four-word
status block per point in address order, owns the private client for the synchronous call, stops on
the first failure, and commits caller output only after total success. It is non-atomic across PLC
scan times. `LCS`/`LCC` remain one direct request. No state-changing aggregate splits.

Compatibility impact: the existing hidden multi-request behavior is now explicit and failure no
longer exposes partially updated caller output. Coherent readers use a one-point/single-request read
or PLC-side snapshot/handshake.

### Machine-verifiable acceptance criteria

1. The complete address/profile/request/response plan is encoded and validated before first send.
2. Internal reads preserve increasing address order and cannot split a four-word status block.
3. First failure stops execution and leaves all caller output unchanged.
4. Exact maximum representable point count uses bounded fixed staging and maps every result.
5. Documentation says non-atomic and identifies the coherence alternative.
6. All writes and all other public methods emit at most one request.

### Acceptance tracking

- [x] Implementation completed in this repository.
- [x] Order, maximum boundary, intermediate failure, and no-partial-output tests added.
- [x] All relevant builds and package checks passed.
- [x] Codex aggregate classification and diff self-review completed.
- [x] Live PLC verification is not required; planning/publication semantics are deterministic.
- [x] Documentation, changelog, and generated API agree.
- [x] Final acceptance criteria verified.

## MCS-BOOL-DEFER-001 — Native bool bit contract

Implementation scope: every bit request/result, helper, client, host wrapper, example, and API doc.

### Target contract

Public bit values are native `bool`. `BitValue` is only a readable alias for `bool`; there are no
`Off`/`On` enum members and no integer/unknown third state. Packed multi-block word payloads remain
explicit `uint16_t` storage where the protocol contract is a bit field in a word.

Compatibility impact: replace `BitValue::Off`/`On` with `false`/`true`.

### Machine-verifiable acceptance criteria

1. `std::is_same<BitValue, bool>` is true.
2. All public bit inputs/outputs compile with bool spans and values.
3. Documentation/examples contain no removed enum members.
4. Word-packed protocol fields remain word typed and preserve bit order.

### Acceptance tracking

- [x] Implementation completed in this repository.
- [x] Type identity, bit codec, packed-word, and example compile coverage updated.
- [x] All relevant builds and package checks passed.
- [x] Codex public-type self-review completed.
- [x] Live PLC verification is not required; representation is compile/codec behavior.
- [x] Documentation, changelog, and generated API agree.
- [x] Final acceptance criteria verified.

## MCS-IPV4-AUDIT-001 — IPv4-only network policy applicability

Implementation scope: the complete public endpoint and transport surface.

### Target contract

Not applicable. This library communicates over a caller-selected local serial device and exposes no
IP address, hostname, socket, TCP, or UDP endpoint. It therefore cannot accept IPv4 or IPv6 and must
not add a fictitious IP validation setting for cross-library symmetry.

### Machine-verifiable acceptance criteria

1. Public headers expose no network endpoint or address-family option.
2. User documentation describes serial configuration only.
3. Future network transports must reopen the workspace IPv4-only decision before becoming public.

### Acceptance tracking

- [x] Public API and documentation applicability audit completed: IPv4 policy is N/A.
- [x] No implementation or live PLC verification is required.
- [x] Codex final public-surface search recorded.
- [x] Final acceptance criteria verified.

## MCS-PROFILE-IDENTITY-001 — Exact explicit profile applicability

Implementation scope: `ProtocolConfig`, `MelsecSerialClient`, host wrapper, codecs, and CLI.

### Target contract

Every operation uses the one exact, explicit `PlcProfile` stored in its validated
`ProtocolConfig`. There is no automatic detection, profile family fallback, profile alias, or
second per-call profile argument that could disagree. Reconfiguration is explicit and is rejected
while a request is active.

Compatibility impact: none beyond existing mandatory profile validation.

### Machine-verifiable acceptance criteria

1. An unconfigured/unknown profile is rejected before request construction.
2. Request and response processing use the same immutable in-flight configuration.
3. No profile fallback or profile-derived resend exists.
4. Public operations expose no ambiguous secondary profile selector.

### Acceptance tracking

- [x] Applicability and implementation audit completed; the existing exact-profile design conforms.
- [x] Existing unknown-profile, reconfiguration, and response-identity tests provide coverage.
- [x] All relevant builds and package checks passed.
- [x] Codex final profile/fallback search recorded.
- [x] Live PLC verification is not required for configuration identity.
- [x] Documentation and generated API agree.
- [x] Final acceptance criteria verified.

## Verification evidence and self-review disposition

Final source state evidence:

- GCC/UCRT64 strict C++17 full build: 4/4 CTest executables passed; `codec_tests.cpp` contains 239
  named deterministic test functions.
- MSVC 19.50 strict build: 4/4 CTest executables passed.
- GCC reduced and ultra core profiles compiled successfully with their intended test targets
  disabled by the profile contract.
- The packed PlatformIO package compiled and linked its then-current native/AVR consumers. AVR
  support was subsequently removed by GOAL-MCS-001; the maintained gate now uses `native-core` and
  `esp32-c3-core`.
- A synthetic worktree source archive containing modified, untracked, and
  deleted paths passed contents, extracted build, 4/4 CTest, Markdown-link,
  generated-API, and packed PlatformIO consumer checks (104 files).
- `git diff --check`, Markdown links, API freshness, removed inter-byte API search, removed bit-enum
  search, IPv4/IPv6 endpoint search, profile-fallback search, and public async admission review
  passed. The final whitespace-only cleanup did not change compiled behavior.

Codex self-review inspected the actual public API/diff, capacity ordering and formulas, operation
classification, state transitions, timeout/cancellation boundaries, Win32/POSIX TX/drain/RX waits,
caller-output publication, examples, CLI, package contents, generated API, and changelog.

Accepted findings, corrected and reverified:

1. The provisional deadline and RS-485 begin hook were initially armed during request construction.
   They now start only in `notify_tx_started()` immediately before real transport write; pre-start
   polling cannot expire a transaction and successful TX completion without start is rejected.
2. Pre-TX cancellation was initially treated like in-progress TX. It now completes directly as
   `Cancelled`, invokes no TX hook, requires no ambiguity classification, and preserves the
   post-start deferred-cancellation contract.
3. The direct RX exact-deadline path initially retained Format 2 reuse. Every deadline path now
   requires transport reset, including sequenced Format 2.
4. The existing long-state multi-point host helper was initially classified as non-aggregate.
   Review identified its hidden one-request-per-point behavior; it is now explicitly aggregate,
   completely preflighted, ordered, fixed-staged, non-atomic, and all-or-error.

No finding was rejected or left deferred. Historical maintainer records retain their historical
terminology; current user/API/changelog documentation contains the migration contract. No live PLC
test is required because all changed acceptance criteria concern local validation, transport state,
fixed time, build/package shape, or injected response behavior.

## MCS-ARTIFACT-001 — Complete worktree source and packed consumers

Implementation scope: source-archive worktree mode, extracted host CI,
PlatformIO package construction, native/ESP32-C3 packed consumers, and CI tooling.

Target contract: worktree source archives are created from one synthetic Git
tree containing every modified and untracked non-ignored file and every tracked
deletion. The extracted archive alone passes host build/tests, Markdown and API
freshness, then packs its own PlatformIO artifact and builds both native and
supported ESP32-C3 consumers from that tarball.

Compatibility impact: none; runtime and public C++ contracts are unchanged.

Machine-verifiable acceptance criteria:

1. Synthetic worktree construction uses an alternate Git index and never
   changes the repository index or overlays files onto a `HEAD` archive.
2. The deleted compatibility header remains absent while replacement public and
   detail headers and every worktree modification are compiled from extraction.
3. Extracted CMake/CTest, Markdown-link, and generated-API checks pass.
4. The extracted tree's packed tarball builds `native-core` and
   `esp32-c3-core`, links `client.cpp` and `codec.cpp`, and excludes host-only
   objects from both consumers.

Self-review finding disposition: accepted. The previous worktree option only
changed attribute lookup while still archiving `HEAD`, so its result was not
evidence for modified, untracked, or deleted content.

- [x] Implementation completed in this repository.
- [x] Synthetic archive and packed-consumer validation are permanent gates.
- [x] Extracted host build, 4/4 CTest, docs checks, and both PlatformIO consumers passed.
- [x] Codex self-review completed against complete worktree and packed-artifact scope.
- [x] Live PLC verification is not required; archive and compilation behavior are deterministic.
- [x] Maintainer record, changelog, and CI workflow agree with the implemented gate.
- [x] Final acceptance criteria verified and the item marked complete.

## GOAL-MCS-001 — Remove unsupported Arduino Mega 2560 support

Implementation scope: PlatformIO environments, samples, package metadata, CI/package consumers,
user documentation, maintainer documentation, and changelog.

Target contract: ESP32 and RP2040 remain supported MCU targets. Arduino Mega 2560 and every other
AVR/8-bit target are unsupported and are not presented by a tracked artifact or gate. The public
decode API is unchanged by this support removal.

Compatibility impact: Mega/AVR users migrate to ESP32 or maintain an unsupported downstream port.

Machine-verifiable acceptance criteria:

1. No Mega/AVR PlatformIO environment, sample, package platform wildcard, or CI consumer remains.
2. The packed embedded consumer gate builds `esp32-c3-core`, and the maintained ESP32 examples build.
3. User/release documentation names the support removal and migration path.
4. No public decode header changes as part of this item.

- [x] Implementation completed in this repository.
- [x] Targeted source, metadata, sample, and documentation searches added to self-review evidence.
- [x] Relevant ESP32 and package-consumer build checks passed.
- [x] Codex self-review completed against the approved contract.
- [x] Live PLC verification is not required; support metadata and build targets are deterministic.
- [x] Documentation, migration guidance, and changelog agree.
- [x] Final acceptance criteria verified.

## GOAL-MCS-002 — Preserve core completion status in the synchronous API

Implementation scope: `PosixSyncClient`, its internal synchronous runner, deterministic fault
injection tests, user guidance, and changelog.

Target contract: once transmission starts, a completed core callback is the sole returned result.
No wrapper-owned state-changing command list can replace that result. Normal and extended monitor
registration preserve `OperationOutcomeUnknown` and `cause`; pre-send failures remain direct. A
receive-side failure is reported to the core with its actual `Status`, not converted to caller
cancellation.

Compatibility impact: monitor-registration timeout/transport results can change from a direct
failure to `OperationOutcomeUnknown`; callers must reconcile PLC state before retrying.

Machine-verifiable acceptance criteria:

1. Flush/pre-send failure remains direct.
2. Injected write, drain, receive `Transport`, receive `Timeout`, and core timeout return the core
   callback status with the originating cause.
3. Normal and extended monitor registration are covered.
4. Read-only injected failures never become outcome-unknown.

- [x] Implementation completed in this repository.
- [x] Deterministic sync-runner fault tests added for every classification branch.
- [x] Targeted build and tests passed.
- [x] Codex self-review completed against the approved contract.
- [x] Live PLC verification is not required; injected transport/core evidence is authoritative.
- [x] User guidance and changelog agree.
- [x] Final acceptance criteria verified.

## GOAL-MCS-003 — Preserve core completion status in the CLI

Implementation scope: common CLI request driving, diagnostic formatting, deterministic fake-port
tests, user guidance, and changelog.

Target contract: after notify/cancel completes the core operation, the callback status is the sole
CLI result. Diagnostics print the machine status name and the structured cause for
`OperationOutcomeUnknown`; the CLI has no command-specific ambiguity list. Transport receive
failures use the same core notification path as the synchronous facade and retain their actual
status code.

Compatibility impact: post-send CLI diagnostics can change classification and text; scripts must
consume the corrected status/cause output.

Machine-verifiable acceptance criteria:

1. Injected write, drain, receive `Transport`, and receive `Timeout` failures return a completed
   callback result.
2. State-changing requests preserve outcome-unknown and cause.
3. Read-only requests preserve direct failure/cancellation classification.
4. Rendered unknown-outcome diagnostics contain both machine classification and cause.

- [x] Implementation completed in this repository.
- [x] Deterministic CLI fake-port and diagnostic tests added.
- [x] Targeted build and tests passed.
- [x] Codex self-review completed against the approved contract.
- [x] Live PLC verification is not required; injected transport and formatting checks are deterministic.
- [x] User guidance and changelog agree.
- [x] Final acceptance criteria verified.

## GOAL-MCS-004 — Reject Win32 receive sizes above MAXDWORD

Implementation scope: Win32 serial send/receive size validation, Windows unit tests, user-visible
release documentation, and self-review of narrowing conversions.

Target contract: sizes through `MAXDWORD` pass common Win32 validation. A larger receive span
returns `InvalidArgument` before timeout configuration or `ReadFile`; no truncation or clamping is
allowed.

Compatibility impact: oversized receive spans are explicitly rejected instead of truncated.

Machine-verifiable acceptance criteria:

1. `MAXDWORD` passes and `MAXDWORD + 1` fails common validation.
2. Both send and receive call the common validator before their Win32 I/O function.
3. Every size-to-`DWORD` conversion is dominated by that validation.

- [x] Implementation completed in this repository.
- [x] Windows boundary tests added.
- [x] Targeted Windows build and tests passed.
- [x] Codex self-review completed against the approved contract.
- [x] Live PLC verification is not required; integer-boundary validation is deterministic.
- [x] Changelog records the corrected behavior.
- [x] Final acceptance criteria verified.

### GOAL-MCS-001..004 verification and self-review disposition

Verification used the final source state: the Windows CMake build and all 6 CTest tests passed;
Markdown links, generated API freshness, JSON parsing, and `git diff --check` passed; four maintained
ESP32-C3 example environments passed; and the packed package built and linked both `native-core`
and `esp32-c3-core` consumers without host-only objects.

Codex self-review inspected the actual diff, public headers and decode surface, synchronous and CLI
validation order, callback result ownership, failure/cancellation/timeout transitions, diagnostic
formatting, Win32 narrowing boundaries, deterministic tests, examples, package metadata, user and
maintainer documentation, and changelog.

Accepted findings, corrected and reverified:

1. The first ESP32-C3 packed-consumer run retained a conflicting explicit C++ standard flag from
   the platform. The consumer gate now removes every supported GNU/ISO standard spelling before
   adding exactly `-std=c++17`; native and ESP32-C3 package builds then passed.
2. Several specialized CLI probe paths still printed only a message or PLC code. They now use the
   common status formatter, so every path preserves the machine classification and structured
   unknown-outcome cause without a command-specific ambiguity list.
3. Independent final review found that synchronous and CLI receive failures called argument-less
   `cancel()`, replacing the actual `Transport` or `Timeout` with `Cancelled`. The core now exposes
   `notify_rx_failure(Status)`; both drivers use it, and core/sync/CLI tests cover receive
   `Transport` and `Timeout` for state-changing and read-only requests.

No self-review finding was rejected, duplicated, or deferred. Live PLC work is not required because
these acceptance criteria are fully determined by local lifecycle injection, integer-boundary,
metadata, compilation, package-content, and documentation checks.
