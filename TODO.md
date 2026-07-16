# TODO

Current active TODOs only.

## Current Status

Static implementation and mock/CLI validation for D-087 through D-092 are complete. The following
physical checks remain intentionally deferred until the matching PLC/module is connected and the
user explicitly approves the stated batch.

## D-087 through D-092 live serial verification

### MELSEC iQ-R / `melsec:iq-r`

- [ ] Target: RCPU with RJ71C24-R2-class module; endpoint `COM3`; explicit settings `19200`, `8E1`,
  `HardwareFlowControl::None`; read-only `D100`, 1 word. Purpose: confirm the new required typed
  configuration opens the known endpoint and preserves the confirmed C4 frame behavior. Expected
  evidence: successful model/read result plus recorded effective host serial settings. Restoration:
  none (read-only). The 2026-07-16 FTDI/cable/gender-changer setup is not valid evidence for this
  row because this FTDI USB-COM setup requires host RTS asserted; attempts with `None` timed out
  without a response. Disposition for the current verification session: not tested further; leave
  this row `unverified` until a separate RTS-independent physical setup is available.
- [x] Windows RTS/CTS setup: `R120PCPU` with `RJ71C24-R2` CH2, FTDI `COM3`, corrected cable and new
  gender changer, explicit `HardwareFlowControl::RtsCts`, `19200 / 8E1`, C4 binary Format 5, sum
  check disabled, station `0`; read-only `cpu-model` returned `R120PCPU / 0x4844` and `D100`, 1 word,
  returned `0x0000`. This FTDI USB-COM adapter plus cable/gender-changer combination requires RTS
  `ON`. No PLC write was sent.
- [ ] POSIX RTS/CTS setup: matching target, adapter, and endpoint not selected; live mapping remains
  unverified independently of the completed Windows result.

### MELSEC-Q / `melsec:qcpu`

- [ ] Target: Q06UDVCPU with QJ71C24N; endpoint `COM3`; explicit settings `19200`, `8E1`,
  `HardwareFlowControl::None`; read-only `D100`, 1 word. Purpose: confirm required settings and
  validation order against the known Q-series setup. Expected evidence: successful model/read result
  plus recorded effective host serial settings. Restoration: none (read-only).

### Alternate serial formats

- [ ] Target/profile/endpoint: to be selected only from available test hardware configured for ASCII
  with 7 data bits and/or two stop bits. Device: `D100`, 1-word read only. Purpose: confirm the
  explicitly supported ASCII+7 and stop-bit 2 backend paths. Expected evidence: configuration
  snapshot and successful response using the same explicit library values. Restoration: return the
  serial module to its recorded original data/parity/stop settings.

## Resolved: R120PCPU / RJ71C24-R2 COM3 investigation (2026-07-16)

Status: resolved as a physical interconnect/control-line condition, not a current codec or Windows
serial-backend regression. The FTDI USB-COM adapter with the corrected cable and new gender changer
communicates reliably only when the host selects `HardwareFlowControl::RtsCts` and therefore
asserts RTS.

Final read-only evidence: `cpu-model` returned `R120PCPU / 0x4844`, and `read-words D100 1` returned
`0x0000` with normal completion. The Windows RTS/CTS row above is complete. The separate
`HardwareFlowControl::None` row remains open because this physical setup requires RTS `ON`.

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
- Remaining work is limited to the independent `HardwareFlowControl::None` and POSIX rows above;
  neither is converted to a pass by the completed Windows RTS/CTS result. The `None` row will not
  be tested further in the current verification session and remains `unverified` pending a separate
  RTS-independent physical setup.

Each unavailable row remains `unverified`; release disposition must be decided row by row. Do not
change module settings or communicate with `COM3` until the user identifies the connected PLC and
explicitly says `OK` for the exact batch.

## Notes

- Do not add unsupported access paths here.
- Do not add TODOs for manual families that are explicitly not needed by this library. The current
  omitted-family policy is documented in [MANUAL_DERIVED_RULES.md](docsrc/maintainer/MANUAL_DERIVED_RULES.md).
- Do not mark a target-dependent PLC rejection as an implementation bug unless request-shape tests
  or new hardware evidence point to the encoder/client code.
- For long timer / long retentive timer contact+coil devices, use the structured `LTN/LSTN`
  `0401` path instead of direct probes.
