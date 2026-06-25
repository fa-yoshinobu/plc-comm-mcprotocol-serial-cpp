# Development History

## 2026-06-25 Explicit frame/profile documentation

- Clarified in user docs that MC Protocol Serial does not infer the serial frame or PLC profile from CPU model text, serial settings, response data, or device strings.
- Recorded that `PlcProfile::Unspecified` is a configuration error rather than a fallback profile.
- Recorded that Linux CLI wrappers require both `MCPROTOCOL_FRAME` and `MCPROTOCOL_PLC_PROFILE` before sending requests.

## 2026-06-11 Archived Refactor Plan

The previous refactor instructions were archived into this history file.

### Scope

- Library: PlatformIO Registry package `mcprotocol-serial-cpp` `0.2.4`.
- Primary task: improve internal readability of `src/codec.cpp` through file-local sectioning and move-only organization.
- File splitting and CLI refactoring were explicitly proposal-only.

### Contracts To Preserve

- Public headers under `include/mcprotocol/serial/`.
- Exact transmitted serial MC Protocol frame bytes covered by codec tests.
- No dynamic allocation in core paths.
- AVR compatibility shims directly under `include/`.
- CMake, PlatformIO, `library.json`, CI target matrix, version, and changelog.

### Debt Notes

- D1: `codec.cpp` had a large anonymous namespace and `CommandCodec` implementation whose helper relationships were hard to scan.
- D2: `tools/mcprotocol_cli.cpp` was large but maintained as report-only.
- Existing tests and CI were already strong, so changes were to remain small and reversible.

### Recorded Result

- Baseline with default Visual Studio generator failed near standard-header handling.
- Reconfigured with Ninja and `g++` successfully.
- Build succeeded with existing `include_next` shim warnings only.
- `ctest --test-dir build --output-on-failure`: `1/1` passed.
- `python scripts/check_markdown_links.py`: passed.
- `pio run -e native-example`: not run because `pio` was not on `PATH`.

### Implemented Change

- Only `src/codec.cpp` was changed.
- Added section heading comments in the anonymous namespace, `FrameCodec`, and `CommandCodec`.
- Function bodies, signatures, public headers, frame bytes, build settings, tests, `tools/`, and compatibility shims were not changed.

### Follow-Up Notes

- Further file splitting should be handled as a separate task because it can affect CI, PlatformIO packaging, and the single-translation-unit design.
- CLI splitting remains a future proposal.
