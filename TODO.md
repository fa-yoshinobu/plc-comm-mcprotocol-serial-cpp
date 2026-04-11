# TODO

Current follow-up items for this repository.

## Native Command Investigation

These command families are implemented in the codebase but are still unresolved on the validated
`RJ71C24-R2 / RS-232C / 19200 / 8E1 / MC Protocol Format4 ASCII / CRLF / sum-check off / station 0`
setup.

- `0403` random read
  Current result: `native ng`, PLC end code `0x7F22`
- `1402` random write words
  Current result: `native ng`, PLC end code `0x7F22`
- `1402` random write bits
  Current result: `native ng`, PLC end code `0x7F23`
- `0406` multi-block read
  Current result: `native ng`, PLC end code `0x7F22`
- `1406` multi-block write
  Current result: `native ng`, PLC end code `0x7F22`
- `0801/0802` monitor register/read
  Current result: `native ng`, PLC end code `0x7F22`

## Extended Device `G/HG`

`U...\\G...` and `U...\\HG...` currently have two separate paths in this repository.

- Helper path over `0601/1601`
  Status: usable
  Notes:
  `read-qualified-words 'U3E0\\G10' 1` returned `0x83BD`
  `read-qualified-words 'U3E0\\HG20' 1` passed
  `write-qualified-words 'U3E0\\HG20' 0x1234` with readback and restore passed
- Native extended-device path over `0401/1401 + 0080/0082`
  Status: hold
  Notes:
  `read-native-qualified-words 'U3E0\\G10' 1 --series iqr` returned `0x7F22`
  `read-native-qualified-words 'U3E0\\HG20' 1 --series iqr` timed out

## Constraints

- Keep the current native-only policy for unsupported commands.
- Do not add CLI fallback behavior for the unresolved native command families above.
- Treat `0601/1601` helper paths as the practical path until native `0080/0082` is revalidated.

## Useful References

- [README.md](README.md)
- [docsrc/user/USAGE_GUIDE.md](docsrc/user/USAGE_GUIDE.md)
- [docsrc/validation/reports/HARDWARE_VALIDATION.md](docsrc/validation/reports/HARDWARE_VALIDATION.md)
- [docsrc/validation/reports/RJ71C24_R2_RS232C_FORMAT4_2026-04-10.md](docsrc/validation/reports/RJ71C24_R2_RS232C_FORMAT4_2026-04-10.md)
- [docsrc/maintainer/DEVELOPER_NOTES.md](docsrc/maintainer/DEVELOPER_NOTES.md)
