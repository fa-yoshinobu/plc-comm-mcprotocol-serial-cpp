# Release Process

This repository uses the tag name as the release trigger.

## Files To Update For A Version Bump

- `CMakeLists.txt`
- `library.json`
- `library.properties`
- `CHANGELOG.md`

## Local Validation Before Tagging

```bash
run_ci.bat
```

PlatformIO checks are optional because they download large toolchains and may use external package mirrors:

```bash
run_ci.bat --with-platformio
```

## Tagging

Create and push an annotated tag:

```bash
git tag -a vX.Y.Z -m "vX.Y.Z"
git push origin vX.Y.Z
```

## GitHub Automation

`.github/workflows/release.yml` publishes a GitHub release when a `v*` tag is pushed.
