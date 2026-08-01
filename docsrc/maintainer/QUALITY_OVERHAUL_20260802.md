# Quality Overhaul Decisions — 2026-08-02

This maintainer record preserves approved target-state decisions before implementation. A checked
acceptance box requires recorded evidence; approval or intent alone is not completion evidence.

## MCS-SERIAL-TX-DEADLINE-001 — Defer timeout completion until physical TX completion or abort

Decision status: implemented and verified on 2026-08-02.

### Implementation scope

`MelsecSerialClient` timeout handling while `awaiting_write_complete_` is true, including
`poll()`, `notify_tx_complete()`, RS-485 direction hooks, completion callbacks, transport-reset
state, host wrappers, tests, examples, and generated/user API documentation.

### Target state

The absolute transaction deadline remains fixed and logically expires at its existing boundary.
If it expires after `notify_tx_started()` but before physical transmission has completed or been
aborted, `poll()` latches the timeout and the transport-reset requirement but does not call
`on_tx_end`, release the completion callback, clear the busy state, or permit another request.

The application must finish or abort the UART/DMA operation and report that physical fact through
`notify_tx_complete()`. That notification releases `on_tx_end` exactly once and publishes the
previously latched result: `Timeout` for a read-only request, or `OperationOutcomeUnknown` with
`cause == Timeout` for a state-changing request. Once the deadline has been latched, a later
transport status does not replace the timeout cause. If physical completion or abort is never
reported, the request intentionally remains busy because the library cannot safely infer the
external transport state.

### Compatibility and operational impact

Timeout detection still occurs at the configured absolute deadline, but the completion callback
may be delivered later while the library waits for physical TX completion or abort. Integrations
must always call `notify_tx_complete()` after `notify_tx_started()`, including after observing that
the deadline expired, and must reset/reconfigure the transport before reuse. Code that relied on
an immediate callback or RS-485 direction release from `poll()` must instead abort or drain the
physical transport and notify the client.

### Machine-verifiable acceptance criteria

1. At and beyond the exact deadline, `poll()` while physical TX is pending latches timeout and
   `requires_transport_reset()` becomes true, but `busy()` remains true.
2. A latched TX timeout does not call `on_tx_end` or the completion callback before
   `notify_tx_complete()` reports physical completion or abort.
3. The first valid `notify_tx_complete()` after a latched timeout calls `on_tx_end` exactly once,
   releases the completion callback exactly once, and ends the busy request.
4. A read-only request completes as `StatusCode::Timeout` after the physical notification.
5. A state-changing request completes as `StatusCode::OperationOutcomeUnknown` with
   `cause == StatusCode::Timeout` after the physical notification.
6. The latched timeout cause wins over a transport status reported after the deadline; transport
   failure reported before deadline retains its existing classification.
7. A second request remains `Busy` until the physical notification completes the timed-out
   request, and reuse remains blocked by the transport-reset requirement afterward.
8. Repeated `poll()` and late or duplicate completion notifications cannot release the RS-485 hook
   or completion callback more than once.
9. Cancellation-before-TX and cancellation-during-TX behavior outside the timeout race remains
   unchanged and is covered by regression tests.

### Acceptance tracking

Evidence recorded 2026-08-02: `run_ci.bat` passed all six CTest targets, Markdown links, and
generated API freshness. All eight PlatformIO example environments passed, and
`check_platformio_package_consumers.py` passed native and ESP32-C3 builds from the packed artifact.
The deterministic callback/hook tests cover the exact boundary, repeated polling, late and duplicate
notifications, timeout-cause priority, Busy/reset gating, reads, state-changing writes, and the
pre-deadline transport-failure regression. Codex self-review found no remaining accepted findings in
the runtime diff, public API, validation order, state transitions, cancellation/timeout behavior,
tests, examples, documentation, or packaging. This item requires no live PLC: it concerns ownership
of an external UART/DMA operation before any response can be consumed, and the mock state-machine
tests exercise every approved observable. Release disposition: live verification is not required.

- [x] Implementation completed in this repository.
- [x] Tests added or updated for every acceptance criterion.
- [x] Relevant static checks, unit tests, integration tests, examples, and package/build checks passed.
- [x] Codex self-review completed against the approved contract and cross-language consistency requirements.
- [x] Required live-PLC checks passed, or each unavailable check has an explicit release disposition.
- [x] Documentation, migration notes, changelog, and generated API reference agree with the implementation.
- [x] Final acceptance criteria verified and the item marked complete.
