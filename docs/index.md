---
title: plc-comm-mcprotocol-serial-cpp
---

# plc-comm-mcprotocol-serial-cpp

This Pages entry exists so GitHub Pages can build successfully from the repository `docs/` source.

The maintained documentation lives in the repository itself:

- [Repository README](https://github.com/fa-yoshinobu/plc-comm-mcprotocol-serial-cpp/blob/main/README.md)
- [MCU Quickstart](https://github.com/fa-yoshinobu/plc-comm-mcprotocol-serial-cpp/blob/main/docsrc/user/MCU_QUICKSTART.md)
- [Wiring Guide](https://github.com/fa-yoshinobu/plc-comm-mcprotocol-serial-cpp/blob/main/docsrc/user/WIRING_GUIDE.md)
- [FAQ](https://github.com/fa-yoshinobu/plc-comm-mcprotocol-serial-cpp/blob/main/docsrc/user/FAQ.md)
- [Hardware Validation Matrix](https://github.com/fa-yoshinobu/plc-comm-mcprotocol-serial-cpp/blob/main/docsrc/validation/reports/HARDWARE_VALIDATION.md)
- [Developer Notes](https://github.com/fa-yoshinobu/plc-comm-mcprotocol-serial-cpp/blob/main/docsrc/maintainer/DEVELOPER_NOTES.md)
- [Native Command Backlog](https://github.com/fa-yoshinobu/plc-comm-mcprotocol-serial-cpp/blob/main/docsrc/maintainer/NATIVE_COMMAND_BACKLOG.md)

Generated API documentation is produced by the host build:

```bash
cmake -S . -B build
cmake --build build --target docs
```

The generated files are written under `build/docs/doxygen/html`.
