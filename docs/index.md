---
title: plc-comm-mcprotocol-serial-cpp
---

# plc-comm-mcprotocol-serial-cpp

Use this page as the GitHub Pages entry for the repository.

## Start Here

### For Users

1. Choose the sample that matches your environment.

- [Examples Index](https://github.com/fa-yoshinobu/plc-comm-mcprotocol-serial-cpp/blob/main/examples/README.md)
- [MCU Quickstart](https://github.com/fa-yoshinobu/plc-comm-mcprotocol-serial-cpp/blob/main/docsrc/user/MCU_QUICKSTART.md)

2. Confirm the exact target settings before wiring or building against real hardware.

- [Hardware Validation Matrix](https://github.com/fa-yoshinobu/plc-comm-mcprotocol-serial-cpp/blob/main/docsrc/validation/reports/HARDWARE_VALIDATION.md)
- [Wiring Guide](https://github.com/fa-yoshinobu/plc-comm-mcprotocol-serial-cpp/blob/main/docsrc/user/WIRING_GUIDE.md)
- [FAQ](https://github.com/fa-yoshinobu/plc-comm-mcprotocol-serial-cpp/blob/main/docsrc/user/FAQ.md)

3. Use the generated API reference once you know which entrypoint you want.

- [Generated API Docs](api/index.html)

### For Maintainers

- [Maintainer Docs Index](https://github.com/fa-yoshinobu/plc-comm-mcprotocol-serial-cpp/blob/main/docsrc/maintainer/README.md)
- [Developer Notes](https://github.com/fa-yoshinobu/plc-comm-mcprotocol-serial-cpp/blob/main/docsrc/maintainer/DEVELOPER_NOTES.md)
- [Manual Command Coverage](https://github.com/fa-yoshinobu/plc-comm-mcprotocol-serial-cpp/blob/main/docsrc/maintainer/MANUAL_COMMAND_COVERAGE.md)
- [TODO / Current Follow-up](https://github.com/fa-yoshinobu/plc-comm-mcprotocol-serial-cpp/blob/main/docsrc/maintainer/TODO.md)
- [Native Command Backlog](https://github.com/fa-yoshinobu/plc-comm-mcprotocol-serial-cpp/blob/main/docsrc/maintainer/NATIVE_COMMAND_BACKLOG.md)

## Repository Docs

Most maintained docs live in repository Markdown pages:

- [Repository README](https://github.com/fa-yoshinobu/plc-comm-mcprotocol-serial-cpp/blob/main/README.md)
- [Examples Index](https://github.com/fa-yoshinobu/plc-comm-mcprotocol-serial-cpp/blob/main/examples/README.md)
- [Hardware Validation Matrix](https://github.com/fa-yoshinobu/plc-comm-mcprotocol-serial-cpp/blob/main/docsrc/validation/reports/HARDWARE_VALIDATION.md)
- [Maintainer Docs Index](https://github.com/fa-yoshinobu/plc-comm-mcprotocol-serial-cpp/blob/main/docsrc/maintainer/README.md)

Generated API documentation is produced by the host build:

```bash
cmake -S . -B build
cmake --build build --target docs
```

The generated API files are written under `docs/api/`.
