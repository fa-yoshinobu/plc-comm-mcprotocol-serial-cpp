# Release Process

This repository uses the tag name as the release trigger.

## Files To Update For A Version Bump

- `CMakeLists.txt`
- `library.json`
- `library.properties`
- `CHANGELOG.md`

## Local Validation Before Tagging

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
cmake --build build --target check-markdown-links
pio run -e native-example
pio run -e rpipico-arduino-example
pio run -e esp32-c3-devkitm-1-example
pio run -e rpipico-arduino-uart-example
pio run -e esp32-c3-devkitm-1-uart-example
pio run -e mega2560-arduino-uart-example
pio run -e native-example-ultra-minimal
pio run -e rpipico-arduino-example-ultra-minimal
pio run -e esp32-c3-devkitm-1-example-ultra-minimal
```

## Tagging

Create and push an annotated tag:

```bash
git tag -a vX.Y.Z -m "vX.Y.Z"
git push origin vX.Y.Z
```

## GitHub Automation

`.github/workflows/release.yml` publishes a GitHub release when a `v*` tag is pushed.
