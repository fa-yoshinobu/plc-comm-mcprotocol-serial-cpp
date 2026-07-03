# Native Command Backlog

Audience: maintainers running focused real-hardware checks on target-dependent native command behavior.

This page complements the repository-root TODO. Keep normal user docs focused on known-good
workflows and keep this page for native-command check policy and interpretation.

## Public Policy

- Keep target-dependent native command behavior visible. Do not add fallback behavior that
  silently swaps in a different command family.
- Keep target-dependent native commands in the public API, but describe them as native probes on
  the validated real-hardware setups instead of known-good workflows.
- Treat `read-qualified-words` / `write-qualified-words` over `0601/1601` as the helper route
  only. Profiles that require native-qualified access must reject this helper route.
- Treat `read-native-qualified-words` / `write-native-qualified-words` over `0401/1401` as the
  supported `Un\G` / `Un\HG` route only for profiles that explicitly include those devices.

## Active Items

Track current active items in the repository-root [TODO.md](../../TODO.md).

## Implementation Gaps

There are no current implementation gaps. If a new codec/client/CLI gap is found, track it in
the repository-root [TODO.md](../../TODO.md) with the affected command, request shape, validation evidence, and close
criteria.

## Check Rules

- Add every new hardware result to the maintainer archive.
- Add or extend the consolidated report under the maintainer archive for that hardware
  target, including raw evidence and command examples.
- Keep the top-level `README.md` summary short. Push detailed failure evidence into validation docs.
- Preserve request-shape conformance tests before treating hardware rejection as an encoder bug.
- Record the exact serial settings, PLC model, and native PLC end code for every new result.
- Run shared real-UART probes strictly serially. Parallel access on the same serial port can
  produce mixed RX fragments and invalidate the result.
- Keep FX5U notes aligned with its serial manual: `0801/0802` unsupported, `DX/DY/V/ZR` outside
  the validated subset.
- Do not turn unsupported access paths into backlog items. For long timer / long retentive timer
  contact+coil devices, keep `LTS/LTC/LSTS/LSTC` on the structured `LTN/LSTN` `0401` path.
- Re-read the device-specific considerations before assuming that a rejected command family is a
  bug. `LTN/LSTN/LCN` are not treated as monitor targets here because the manual-listed paths are
  `0401`, `0403`, and `1402` (`LCN` also `1401`), while `LZ` explicitly lists `0801`.
