# v2.0.0

## BREAKING

The canonical `mcprotocol::serial::module_io` vocabulary is now the release guidance for request-destination module I/O routing. Existing code that used raw literals should move to named constants.

| Raw value | Named constant |
| --- | --- |
| `0x03D0` | `module_io::ControlSystemCpu` |
| `0x03D1` | `module_io::StandbySystemCpu` |
| `0x03D2` | `module_io::SystemACpu` |
| `0x03D3` | `module_io::SystemBCpu` |
| `0x03E0` to `0x03E3` | `module_io::MultipleCpu1` to `module_io::MultipleCpu4` |
| `0x03FF` | `module_io::OwnStation` |

## Package Name

| Registry | Package |
| --- | --- |
| PlatformIO | `fa-yoshinobu/mcprotocol-serial-cpp` unchanged |

## Highlights

- CMake, PlatformIO, Arduino, and public version-header metadata bumped to 2.0.0.
- Added the 13 canonical module I/O constants and API reference coverage.
- Install examples now use `fa-yoshinobu/mcprotocol-serial-cpp@^2.0.0`.
- README links to the plc-comm package matrix.

Package matrix: https://fa-yoshinobu.github.io/plc-comm-docs-site/package-matrix/
