# Docs And CI

## Local Documentation Tasks

Generate API docs:

```bash
cmake --build build --target docs
```

The generated HTML is written to `build/docs/api`.

Or run Doxygen directly from the repository root:

```bash
doxygen Doxyfile
```

That direct run also writes to `build/docs/api`.

Check Markdown links:

```bash
cmake --build build --target check-markdown-links
```

Run both:

```bash
cmake --build build --target docs-all
```

## GitHub Automation

GitHub Actions verifies:

- host build
- `ctest`
- Doxygen generation
- Markdown link checks
- PlatformIO compile checks for the supported example environments

GitHub Pages publishes the generated API docs from CI artifacts. The generated HTML under
`build/docs/api` is not tracked in `main`.
