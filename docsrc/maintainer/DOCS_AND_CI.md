# Docs And CI

## Local Documentation Tasks

Check Markdown links:

```bash
cmake --build build --target check-markdown-links
```

The public documentation is maintained as tracked Markdown and collected by `plc-comm-docs-site`.

## GitHub Automation

GitHub Actions verifies:

- host build
- `ctest`
- Markdown link checks
- PlatformIO compile checks for the supported example environments

The central documentation site is rebuilt from tracked Markdown through `plc-comm-docs-site`.
