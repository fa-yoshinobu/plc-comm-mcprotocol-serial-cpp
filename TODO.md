# TODO

Current status and recently completed live-verification evidence.

## Current Status

No active confirmed-defect TODOs are currently tracked. The 2026-09-01 correction batch and D-093
are complete.

## Resolved: D-093 C24 `1615` send-completion contract

Implementation scope: synchronous and asynchronous transmission-sequence initialization in this
repository. Target contract: because the tested C24 returns no frame for `1615/0000`, successful
serial transmission completes the API successfully without waiting for RX. The result states that
PLC internal state is not confirmed. Compatibility impact: a former response timeout becomes a
successful send-completion result; the request frame is unchanged.

1. `1615/0000` is emitted unchanged in C4 binary Format 5.
2. Successful TX completion returns `StatusCode::Ok` without waiting for RX.
3. A TX transport failure remains `OperationOutcomeUnknown`.
4. Sync and async public APIs have the same completion contract.

- [x] Implementation completed in this repository.
- [x] Tests added for every acceptance criterion.
- [x] Focused static, unit, adapter-build, and live checks passed.
- [x] Codex self-review completed against the approved contract.
- [x] Required live-PLC checks passed.
- [x] Public API comments and CLI output agree with the implementation.
- [x] Final acceptance criteria verified and the item marked complete.

## Resolved: D-087 through D-092 live serial verification (2026-07-18)

### MELSEC iQ-R / `melsec:iq-r`

- [x] Linux POSIX no-flow setup: `R120PCPU` with `RJ71C24-R2` CH2 and the motherboard RS-232 port
  `/dev/ttyS0` (PNP0501, I/O `0x3f8`), explicit `HardwareFlowControl::None`, `19200 / 8E1`, C4
  binary Format 5, sum check disabled, station `0`; read-only `cpu-model` returned
  `R120PCPU / 0x4844` and `D100`, 1 word, returned `0x0000` on 2026-07-18. The post-run termios
  snapshot reported `19200`, `parenb`, even parity, `cs8`, one stop bit, and `-crtscts`. This
  motherboard path is the separate RTS-independent physical setup required by this row. The
  2026-07-16 FTDI timeout remains adapter/interconnect-specific evidence. No PLC write was sent.
- [x] Windows RTS/CTS setup: `R120PCPU` with `RJ71C24-R2` CH2, FTDI `COM3`, corrected cable and new
  gender changer, explicit `HardwareFlowControl::RtsCts`, `19200 / 8E1`, C4 binary Format 5, sum
  check disabled, station `0`; read-only `cpu-model` returned `R120PCPU / 0x4844` and `D100`, 1 word,
  returned `0x0000`. This FTDI USB-COM adapter plus cable/gender-changer combination requires RTS
  `ON`. No PLC write was sent.
- [x] Linux POSIX RTS/CTS setup: `R120PCPU` with `RJ71C24-R2` CH2 and the motherboard RS-232 port
  `/dev/ttyS0` (PNP0501, I/O `0x3f8`), explicit `HardwareFlowControl::RtsCts`, `19200 / 8E1`, C4
  binary Format 5, sum check disabled, station `0`; read-only `cpu-model` returned
  `R120PCPU / 0x4844` and `D100`, 1 word, returned `0x0000` on 2026-07-18. The post-run termios
  snapshot reported `19200`, `parenb`, even parity, `cs8`, one stop bit, and `crtscts`. No PLC write
  was sent.

### MELSEC-Q / `melsec:qcpu`

- [x] Linux POSIX Q-series setup: `Q06UDVCPU` with `QJ71C24N` and the motherboard RS-232 port
  `/dev/ttyS0`, explicit `19200 / 7E2`, `HardwareFlowControl::None`, C4 ASCII Format 4, sum check
  disabled, station `0`; after the user applied the parameters and reset the unit, read-only
  `cpu-model` returned `Q06UDVCPU / 0x0368` and `D100`, 1 word, returned `0x0000` on 2026-07-18.
  The post-run termios snapshot reported `19200`, `parenb`, even parity, `cs7`, two stop bits, and
  `-crtscts`. The pre-reset attempt received no bytes and timed out; the same command passed after
  the reset. No PLC write was sent.

### Alternate serial formats

- [x] The Q-series batch above independently confirmed the ASCII+7 and stop-bit 2 backend paths with
  explicit `19200 / 7E2`, C4 ASCII Format 4 values on both the PLC module and Linux CLI. The
  effective termios snapshot matched those values, and the read-only `D100` response succeeded.
  The user selected this as the active module configuration for the batch; no PLC value restoration
  was required.

## Resolved: R120PCPU / RJ71C24-R2 COM3 investigation (2026-07-16)

Status: resolved as a physical interconnect/control-line condition, not a current codec or Windows
serial-backend regression. The FTDI USB-COM adapter with the corrected cable and new gender changer
communicates reliably only when the host selects `HardwareFlowControl::RtsCts` and therefore
asserts RTS.

Final read-only evidence: `cpu-model` returned `R120PCPU / 0x4844`, and `read-words D100 1` returned
`0x0000` with normal completion. The Windows RTS/CTS row above is complete. At the end of the
2026-07-16 FTDI investigation, the separate `HardwareFlowControl::None` row was still open because
that physical setup requires RTS `ON`; the Linux motherboard-port check closed it on 2026-07-18.

### Root cause

- The original cable/gender-changer wiring was incorrect.
- After correcting the wiring and installing the new gender changer, this FTDI USB-COM conversion
  setup still required the PC RTS output to be asserted. `--hardware-flow none` left RTS `OFF` and
  the PLC request timed out; `--hardware-flow rts-cts` managed/asserted RTS and both read-only
  checks completed normally. This is evidence for the tested USB-COM adapter and wiring, not a rule
  that every USB-COM product requires RTS/CTS.
- The failure was therefore the combination of the physical interconnect and host flow-control
  mode. The successful CPU-model and `D100` responses rule out `19200 / 8E1`, C4 binary Format 5,
  sum-check-off framing, and the current request encoder as the cause on this setup.

### Pre-resolution setup and observations

- The connected target was identified finally as `R120PCPU` with `RJ71C24-R2`; the PC endpoint was an
  FTDI USB serial adapter on `COM3` connected through a cross cable. CH1 and CH2 were tested
  sequentially.
- The GX Works3 parameter view showed the same values for CH1 and CH2: MC Protocol Format 5,
  `19200`, independent operation, 8 data bits, parity enabled/even, 1 stop bit, sum check disabled,
  station `0`, RTS signal state `ON`, and DTR signal state `ON`. During the failing attempts,
  DTR/DSR transmission control was selected with both DC1/DC3 and DC2/DC4 control disabled. The
  user reported repeated module resets without a change in the result. Before the successful run,
  RS/CS control was set to disabled and transmission control to DC-code control with both DC control
  modes disabled.
- The Windows port state after the CLI run reported `19200 / 8E1`, XON/XOFF disabled, CTS and DSR
  handshaking disabled, DTR `ON`, and RTS `OFF`. A separate no-data modem-status check reported PC
  outputs DTR=`ON`, RTS=`OFF` and PC inputs DSR=`ON`, CTS=`OFF`, CD=`ON`; it transmitted no bytes.
- The CLI was rebuilt from repository HEAD `86037d2` before the final CH2 check. The rebuilt binary
  sent one 18-byte CPU-model request:
  `10 02 0C 00 F8 00 00 FF FF 03 00 00 01 01 00 00 10 03`.
- CH1 indicated receive activity but returned no bytes. The user reported a parity-error indication
  during the PC-adapter checks, but the exact fresh module error code was not captured.
- CH2 repeatedly indicated RD activity, no SD activity, and no new module communication error; the
  CPU-model operation timed out without receiving any response bytes.
- `D100` was not requested during the failing attempts. After the wiring/gender-changer correction
  and RTS/CTS selection, the CPU-model sanity check and the read-only `D100` request both passed. No
  PLC write was performed.
- Existing repository evidence remains distinct: the previously validated iQ-R target used an
  `R120PCPU`/RJ71C24-R2-class path on `COM3` with `19200 / 8E1`, C4 binary Format 5, station `0`, sum
  check disabled, and no RTS/CTS, and completed the recorded profile cross-check. The 2026-07-16
  timeout does not by itself invalidate that support evidence.

### Resolution evidence and disposition

- On the corrected CH2 setup, the current rebuilt CLI completed both approved read-only checks on
  2026-07-16 with `--hardware-flow rts-cts`: `cpu-model` returned `R120PCPU / 0x4844`, and
  `read-words D100 1` returned `0x0000` with a normal response.
- The same current CLI and request frame had timed out with `--hardware-flow none`. The decisive
  operational difference was the corrected physical wiring/new gender changer plus asserted RTS;
  this setup does not provide evidence that the no-flow backend path is defective.
- The current C4 binary Format 5 encoder and Windows backend are therefore not classified as a
  regression. The earlier CH1 parity indication and CH2 RD-only timeouts are superseded setup
  observations and do not require separate protocol-support decisions.
- No PLC write was sent, so no value restoration was required. The module remained at
  `19200 / 8E1`, Format 5, sum check disabled, station `0`, RS/CS control disabled, DC-code
  transmission control selected, and both DC control modes disabled.
- On 2026-07-18, the current Release-built Linux CLI at repository HEAD completed the same approved
  read-only checks through the POSIX backend and motherboard `/dev/ttyS0` with RTS/CTS. The CLI
  returned `R120PCPU / 0x4844` and `D100 = 0x0000`; the post-run termios snapshot confirmed
  `19200 / 8E1` with `crtscts`. This closes the independent POSIX RTS/CTS row.
- The same Linux motherboard path also completed `cpu-model` and `D100`, 1 word, with
  `HardwareFlowControl::None`; the post-run termios snapshot confirmed `-crtscts`. This closes the
  independent no-flow row without changing the earlier FTDI-specific disposition.
- The iQ-R, MELSEC-Q, and alternate serial-format live rows in this section are complete.

Future live batches still require an identified PLC, endpoint, exact read/write scope, and explicit
user approval before changing module settings or sending requests.

## Notes

- Do not add unsupported access paths here.
- Do not add TODOs for manual families that are explicitly not needed by this library. The current
  omitted-family policy is documented in [MANUAL_DERIVED_RULES.md](docsrc/maintainer/MANUAL_DERIVED_RULES.md).
- Do not mark a target-dependent PLC rejection as an implementation bug unless request-shape tests
  or new hardware evidence point to the encoder/client code.
- For long timer / long retentive timer contact+coil devices, use the structured `LTN/LSTN`
  `0401` path instead of direct probes.
