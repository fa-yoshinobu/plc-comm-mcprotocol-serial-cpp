# Release Publication - 2026-06-12

Package:

- repository: `plc-comm-mcprotocol-serial-cpp`
- tag: `v0.2.8`
- package: `mcprotocol-serial-cpp`
- registry: PlatformIO
- GitHub Release: https://github.com/fa-yoshinobu/plc-comm-mcprotocol-serial-cpp/releases/tag/v0.2.8

Publication status:

- GitHub Release created.
- PlatformIO publish completed.
- `pio pkg show` confirmation completed.

Distribution audit:

- PlatformIO package export excludes maintainer docs, validation docs, tests,
  scripts, and build outputs.
- Arduino Library Manager publication is intentionally not part of this release
  flow.

Known follow-up:

- RJ71C24-R2 remote password `1630` / `1631` remains a target-dependent
  follow-up in `TODO.md`.
- This does not block the release of the currently validated command families.

Release process note:

- Before future publication, run the repository release check and confirm that
  the target version does not already exist in PlatformIO.
- Version number gaps are acceptable; duplicate publication of the same version
  is not acceptable.
