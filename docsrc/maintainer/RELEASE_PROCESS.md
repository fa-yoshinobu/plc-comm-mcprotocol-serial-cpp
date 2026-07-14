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

PlatformIO checks are mandatory for a registry release because the published package supports
embedded consumers. They may be omitted only for an explicitly scoped host-only development check:

```bash
run_ci.bat --with-platformio
```

The command must locate PlatformIO from PATH or its standard Windows installation without a manual
PATH edit. Treat warnings emitted from repository-owned source as release findings; warnings from a
pinned external framework must be identified separately rather than mixed with library warnings.

## Final Publication Integrity Gate

Before publishing, confirm all of the following:

1. `release_check.bat --with-platformio` passes from a clean release branch before the version is published.
2. Every unchecked repository TODO or maintainer checkbox is passed, explicitly not required, or has an item-by-item release disposition in the active release GOAL.
3. The target version owns all intended CHANGELOG entries and the immutable tag points to the inspected commit.
4. GitHub Release assets are inspected before registry publication.
5. After publication, compare the extracted PlatformIO registry package with the GitHub `.tar.gz`; file sets and normalized contents must match.
6. Record any permitted unverified hardware scope in the final release summary and do not describe it as a live pass.
7. Confirm the final Release state, tag target, assets, latest docs deployment, open release PR count, and clean working tree.

## Tagging

Create and push an annotated tag:

```bash
git tag -a vX.Y.Z -m "vX.Y.Z"
git push origin vX.Y.Z
```

## GitHub Automation

`.github/workflows/release.yml` publishes a GitHub release when a `v*` tag is pushed.
