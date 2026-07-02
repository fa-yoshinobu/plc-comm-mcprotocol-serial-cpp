# Target-Dependent Native Command Follow-up

Audience: maintainers preparing focused real-hardware rechecks for target-dependent MC protocol
serial native-command results.

This page preserves the active hold and recently resolved evidence that used to live in the
workspace-root `TARGET_DEPENDENT_NATIVE_COMMANDS.md`. The policy-level backlog remains in
[NATIVE_COMMAND_BACKLOG.md](NATIVE_COMMAND_BACKLOG.md), while target reports and raw validation
history live under validation reports.

## Focused Setup

Current target for the remaining hold:

- PLC: `R08CPU`
- Serial module: `RJ71C24-R2`
- Port: `COM3`
- Serial settings: `28800 / 8E2`
- Frame: `c4-binary`, MC Protocol `4C Format5 Binary`
- Station: `0`
- Sum-check: on
- Focused profile setting: `--plc-profile melsec:iq-r`
- Manual command applicability table was used for command applicability checks.

CLI prefix used for the 2026-06-12 focused checks:

```text
.\build\manual\mcprotocol_cli.exe --device COM3 --baud 28800 --data-bits 8 --parity E --stop-bits 2 --frame c4-binary --plc-profile melsec:iq-r --station 0 --sum-check on --response-timeout-ms 5000 --inter-byte-timeout-ms 250
```

The helper script [scripts/recheck_remote_password.ps1](../../scripts/recheck_remote_password.ps1)
captures the same focused setup with `--dump-frames on` and writes a timestamped log under
`logs/`. Without the explicit opt-in switch it runs only the read-only sanity checks:

```powershell
.\scripts\recheck_remote_password.ps1
```

Run the native `1630` / `1631` password commands only after the target-side remote password
configuration is known:

```powershell
.\scripts\recheck_remote_password.ps1 -Password "<known-target-password>" -AllowRemotePasswordCommands
```

The raw frame log includes password bytes in hexadecimal, so preserve it as validation evidence and
avoid sharing it as a public user-facing artifact.

For general contiguous/helper traffic on the same hardware, use the target-family profile for the
connected PLC. The `--plc-profile melsec:iq-r` setting was used for iQ-R-only spot devices such as
`SM`, `SD`, `RD`, `LZ`, `J1\...`, and long current-value focused checks.

## COM3 Sanity Check

Date: 2026-06-12

Read-only communication worked on the focused setup:

- `cpu-model` returned `R08CPU`, model code `0x4801`.
- `read-words D0 1` returned `0x0000`.

This confirms that the expected `R08CPU` target on `COM3` is reachable with these serial/frame
settings. The check is read-only, so it confirms link/settings only; it does not close the remote
password hold below.

## Current Open Hold

| Area | Command | Observed result | Current status |
|---|---:|---|---|
| Remote password unlock | `1630` | historical 6-character unlock returned `0x7FE7`; `123456melsec` on `R08CPU` returned `0x7F22`; `abcdef1` on `R08CPU` returned `0x7FE7` | unresolved |
| Remote password lock | `1631` | historical lock returned `0x7F22`; `123456melsec` on `R08CPU` also returned `0x7F22` | unresolved |

Read-only sanity checks such as `cpu-model` and `read-words D0 1` remained available after the
remote password failures.

## 4C ASCII Format4 Native Extended Recheck

Date: 2026-07-01, resolved 2026-07-02

Resolved 2026-07-02: this was a client encoder bug, now fixed and verified on
both iQ-R and Q/L live targets. The section is kept as the recheck record; the
full analysis and verification tables are in
[FORMAT4_ASCII_NATIVE_EXTENSION_ANALYSIS.md](FORMAT4_ASCII_NATIVE_EXTENSION_ANALYSIS.md).

Update 2026-07-02: desk analysis against SH-080003-AF identified the root
cause as a client encoder bug. The ASCII extended device specification omits
the trailing device-modification field (`000` for Q/L subcommands, `0000` for
iQ-R subcommands) required after the device number (SH-080003-AF p.430-431
formats and worked example). `7F22H` is the C24's own command-parse rejection
(SH-081249-L, PRO category), which is why Binary Format5 worked on the same
targets. The fix (trailing `append_device_modification_ascii` in the two ASCII
extended-reference builders in `src/codec.cpp`) has been applied.

Post-fix iQ-R direct recheck settings:

- MC Serial: `COM3`, `19200 / 8E1`, station `0`
- MC Serial profile: `melsec:iq-r`
- Frame: `c4-ascii-f4`, MC Protocol 4C ASCII Format4
- Sum-check: off
- Note: the serial module is configured for one MC protocol format at a time;
  do not expect Format4 ASCII and Format5 Binary to respond simultaneously.
  A Format4/Format5 setting mismatch is a configuration issue, not a password
  symptom.

Post-fix iQ-R direct recheck result (`R120PCPU`, `cpu-model` `0x4844`):

| Access | Result |
|---|---|
| `read-words D0 1` | OK, `0x0000` |
| `read-native-qualified-words U3E0\G10 1` | OK, `0x0000` |
| `read-native-qualified-words U3E0\HG11 1` | OK, `0x0000` |
| `read-link-direct-bits J1\X10 1` / `J1\SB10 1` | OK |
| `read-link-direct-words J1\W10 1` / `J1\W40 1` | OK, `0x0000` |
| `random-read-link-direct J1\W40 J1\SB10` (0403) | OK |
| `multi-block-read-link-direct-words J1\W40:2` / `-bits J1\SB10:1` (0406) | OK |
| `monitor-link-direct J1\W40` (0801/0802) | OK |
| `write-link-direct-words J1\W40 0x1234` (1401) + readback + restore `0` | OK |
| `random-write-link-direct-words J1\W40=0x5678` (1402) + readback + restore `0` | OK |
| `write-native-qualified-words U3E0\G10 0x0042` (1401) + readback + restore `0` | OK |

An interim recheck recorded `J1\X10` / `J1\W10` as "NG, partial response frame
only". That run used the stale pre-fix `build/Release` binary (TX 55 chars, no
trailing `0000`); deliberately re-running the stale binary in the same session
reproduced the `Jn\` timeout and the `Un\G` `7F22H`. After rebuilding, the same
accesses passed. All build dirs (`build/Release`, `build/Debug`, `build_win`)
were rebuilt post-fix on 2026-07-02; verify the CLI binary timestamp before
recording new NG evidence. After the stale-binary timeout, ASCII `EOT CRLF`
recovery restored normal `D0` Format4 reads.

### Q/L target

Conditions:

- PLC: `Q06UDVCPU`
- SLMP: `192.168.250.100:1025`, Q/L-compatible profile
- MC Serial: `COM3`, `19200 / 8E1`, station `0`
- MC Serial profile: `melsec:qcpu`
- Frame: `c4-ascii-f4`, MC Protocol 4C ASCII Format4
- Sum-check: off

Cross-verify summary:

- Full run: `total: 91`, `ok: 84`, `ng: 7`
- Plain devices passed on both SLMP and MC Serial.
- NG items were only special/native routes: `J1\X0`, `J1\Y0`, `J1\B0`, `J1\W0`, `J1\SB10`,
  `J1\SW10`, and `U2\G1000`.
- Logs are in the cross-verify workspace:
  `logs/format4_run_20260701_223923/`,
  `logs/format4_j_readonly_20260701_224128/`,
  `logs/format4_ug_readonly_20260701_224220/`.

Additional direct MC Serial probes with the same Format4 settings:

| Access | Result |
|---|---|
| `write-link-direct-bits J1\X0` | PLC error `0x7F22` |
| `write-link-direct-bits J1\Y0` | PLC error `0x7F22` |
| `write-link-direct-bits J1\B0` | PLC error `0x7F22` |
| `write-link-direct-words J1\W0` | timeout |
| `write-native-qualified-words U2\G1000` | PLC error `0x7F22` |
| read-only `Jn\...` | PLC error `0x7F22` |
| read-only `U2\G1000` | PLC error `0x7F22` |

Interpretation: this Q/L run looks similar to the prior `melsec:iq-r` Format4 observation:
regular/plain device access works, while native extended `Jn\...` and `Un\...` access does not.
Do not treat this as proof of a PLC limitation yet. Preserve it as measurement evidence and recheck
with raw `MC TX` / `MC RX` frame dumps before changing public support claims.

Post-fix Q/L direct recheck (2026-07-02, `Q06UDVCPU` / `cpu-model` `0x0368`,
same serial settings, `c4-ascii-f4`, `melsec:qcpu`, post-fix binary):

| Access | Result |
|---|---|
| `cpu-model`, `read-words D0 1` (controls) | OK |
| reads: `J1\SB10`, `J1\SW10`, `J1\X0`, `J1\Y0`, `J1\B0`, `J1\W0`, `U2\G1000` | all OK |
| `write-link-direct-bits J1\B0 1` + readback `1` + restore `0` | OK |
| `write-link-direct-bits J1\Y0 1` / `J1\X0 1` + restore `0` | OK |
| `write-link-direct-words J1\W0 0x1234` + readback + restore `0` | OK |
| `write-native-qualified-words U2\G1000 0x0042` + readback + restore `0` | OK |
| `random-read-link-direct J1\W0 J1\SB10` (0403) | OK |
| `multi-block-read-link-direct-words J1\W0:2` (0406) | OK |
| `monitor-link-direct J1\W0` (0801/0802) | OK |

Every item in the pre-fix NG list passed. The Q/L side is closed; the earlier
NG table above is historical pre-fix evidence.

Additional Q/L control check after the target was intentionally set to Format4
on 2026-07-02:

| Access | Result |
|---|---|
| `read-words D0 1` | OK, `0x0000` |
| `cpu-model` | OK, `Q06UDVCPU` / `0x0368` |

This confirms that the active Q serial path currently responds to Format4 when
the module setting and CLI frame mode match. It is independent of remote
password handling.

## Remote Password 1630/1631 Recheck

Date: 2026-06-12

Conditions: `R08CPU`, `RJ71C24-R2` on `COM3`, `c4-binary`, `28800 / 8E2`, sum-check on,
station `0`, `--plc-profile melsec:iq-r`.

Sequence and results:

| Step | Command | Result |
|---|---|---|
| Sanity before | `cpu-model` | `R08CPU` / `0x4801` |
| Unlock | `unlock 123456melsec` | PLC error `0x7F22` |
| Link check | `read-words D0 1` | `0x0000` |
| Lock | `lock 123456melsec` | PLC error `0x7F22` |
| Sanity after | `cpu-model` | `R08CPU` / `0x4801` |
| After PLC restart | `cpu-model` -> `unlock 123456melsec` -> `read-words D0 1` | `R08CPU` / `0x4801`; unlock still `0x7F22`; `D0=0x0000` |
| Password changed | `unlock abcdef1` -> `read-words D0 1` | unlock returned `0x7FE7`; `D0=0x0000` |

Raw frames:

```text
unlock tx: 10 02 1A 00 F8 00 00 FF FF 03 00 00 30 16 00 00 0C 00 31 32 33 34 35 36 6D 65 6C 73 65 63 10 03 31 33
unlock rx: 10 02 0C 00 F8 00 00 FF FF 03 00 00 FF FF 22 7F 10 03 41 34
lock   tx: 10 02 1A 00 F8 00 00 FF FF 03 00 00 31 16 00 00 0C 00 31 32 33 34 35 36 6D 65 6C 73 65 63 10 03 31 34
lock   rx: 10 02 0C 00 F8 00 00 FF FF 03 00 00 FF FF 22 7F 10 03 41 34
abcdef1 unlock tx: 10 02 15 00 F8 00 00 FF FF 03 00 00 30 16 00 00 07 00 61 62 63 64 65 66 31 10 03 45 31
abcdef1 unlock rx: 10 02 0C 00 F8 00 00 FF FF 03 00 00 FF FF E7 7F 10 03 36 39
```

Interpretation:

- The request data is shaped as documented for iQ-R: `1630/0000`, password length as little-endian
  binary, then ASCII password bytes.
- The DLE/STX/ETX bytes and sum-check bytes are frame-level data, not extra password characters.
- The serial link remained healthy after each failure.
- A PLC restart did not change the `123456melsec` result.
- Changing the password to `abcdef1` changed the PLC end code from `0x7F22` to `0x7FE7`, but the
  command still did not unlock.

The remaining question is target-side applicability/configuration for remote-password control on
this serial path, not a general communication failure.

## Resolved Evidence

These items are no longer active holds.

| Area | Command | Result | Status |
|---|---:|---|---|
| 4C ASCII Format4 native extended access | `0401`/`1401`/`0403`/`1402`/`0406`/`0801` with `Jn\...`, `Un\G`, `Un\HG` | encoder omitted the trailing ASCII device-modification field (SH-080003-AF p.430-431); post-fix reads/writes passed on `R120PCPU` (iQ-R) and `Q06UDVCPU` (Q/L) on 2026-07-02 | resolved: client encoder bug, fixed in `src/codec.cpp` and covered by request-shape tests |
| Remote latch clear | `1005` | `latch-clear` in RUN returned `0x4013` on both `R120PCPU` and `R08CPU`; on `R08CPU` (2026-06-12) `remote-stop` then `latch-clear` returned `ok` | resolved: `0x4013` means the CPU was not in STOP |
| Long index register write | native `1402` to `LZ1` | on `R08CPU` (2026-06-12): full write/readback/restore passed | resolved: request shape is valid; prior `R120PCPU` unchanged-readback is historical target-side evidence only |

### LZ1 Native 1402 Recheck

Date: 2026-06-12

Focused setup: `COM3 / 28800 / 8E2 / c4-binary / station 0 / sum-check on / --plc-profile melsec:iq-r`.

- Before: `random-read LZ0 LZ1` returned `LZ0=0x00000000`, `LZ1=0x00000000`.
- `random-write-words LZ1=5678` returned `ok`; immediate `random-read LZ1 LZ0` returned
  `LZ1=0x0000162E (5678)`, `LZ0=0x00000000`.
- Full 32-bit check: `random-write-words LZ1=0x89ABCDEF` returned `ok`; immediate readback returned
  `LZ1=0x89ABCDEF`.
- Restore: `random-write-words LZ1=0` returned `ok`; `random-read LZ0 LZ1` returned both `0`.
- Read-only sanity after the sequence: `cpu-model` returned `R08CPU` / `0x4801`, `read-words D0 1`
  returned `0x0000`.

Raw frames for the first write/readback pair:

```text
write tx: 10 02 18 00 F8 00 00 FF FF 03 00 00 02 14 02 00 00 01 01 00 00 00 62 00 2E 16 00 00 10 03 44 31
write rx: 10 02 0C 00 F8 00 00 FF FF 03 00 00 FF FF 00 00 10 03 30 33
read  tx: 10 02 1A 00 F8 00 00 FF FF 03 00 00 03 04 02 00 00 02 01 00 00 00 62 00 00 00 00 00 62 00 10 03 45 33
read  rx: 10 02 14 00 F8 00 00 FF FF 03 00 00 FF FF 00 00 2E 16 00 00 00 00 00 00 10 03 34 46
```

Interpretation: the `1402` dword-section encoding for `LZ1` (device number `1`, code `0x0062`) is
accepted and applied by `R08CPU`. The earlier `R120PCPU` unchanged-readback result remains
historical target-side evidence, not an encoder bug.

### Remote Latch Clear 1005 Recheck

Date: 2026-06-12

The manual's execution condition is to put the accessed unit into STOP state before executing
remote latch clear. If the target was put into remote STOP or remote PAUSE by a request from another
external device, remote latch clear cannot be executed until that remote STOP/PAUSE is released.

Focused setup: `COM3 / 28800 / 8E2 / c4-binary / station 0 / sum-check on / --plc-profile melsec:iq-r`.

- `latch-clear` with the CPU in its normal RUN state reproduced `0x4013`; `read-words D0 1` still
  passed immediately afterward.
- `remote-stop` returned `ok`, the immediate `latch-clear` returned `ok`, and
  `remote-run no-force no-clear` restored the original state.
- Read-only sanity after the sequence: `cpu-model` returned `R08CPU` / `0x4801`, `read-words D0 1`
  returned `0x0000`.

Interpretation: `0x4013` is the CPU-state rejection for latch clear while the CPU is not in STOP.
The earlier `R120PCPU` result was the same condition, not an encoder or target-parameter problem.
The validated same-channel sequence is `remote-stop` -> `latch-clear` -> `remote-run`.

## Current Interpretation

- `1005` remote latch clear is resolved as a CPU-state precondition: run it after putting the CPU
  into STOP from the same channel.
- `LZ1` native `1402` is resolved on the connected `R08CPU`; the earlier `R120PCPU` unchanged
  readback remains historical target-dependent evidence, not an encoder bug.
- 4C ASCII Format4 native extended access is resolved and verified on iQ-R and
  Q/L. A Format4/Format5 mismatch must be diagnosed as serial-module
  configuration, not as remote-password behavior.
- The only remaining active hold here is remote password `1630` / `1631`.
- The remaining remote-password result looks more like target-side remote-password
  applicability/configuration than a general serial-link failure.
- Do not add silent fallback behavior. A rejected native command should stay visible to the caller.
- Do not broaden this list with unsupported access paths.

## Remaining Recheck Plan

Before re-running the remaining remote-password hold, record:

- PLC model, serial module, firmware if available, and CPU state.
- Serial settings, frame format, station, sum-check, and `--plc-profile` value.
- Target-side remote password and remote operation parameters.

Suggested order:

1. Run read-only sanity checks first: `cpu-model` and `read-words D0 1`.
2. Recheck `1630` / `1631` only with a known target-side remote password configuration.
3. After each failed native command, immediately run a read-only sanity check to confirm the link is
   still alive.

## Evidence To Preserve

For every new result, update:

- [TODO.md](TODO.md) for the active hold status.
- the maintainer validation archive for the consolidated
  status.
- the target-specific maintainer archive for target-specific command
  examples and raw evidence.

Keep the raw PLC end code, command arguments, target state, and immediate follow-up read result in
the report. Do not replace a target-dependent result with a generic pass/fail note.
