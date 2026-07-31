#!/usr/bin/env python3
"""Check that local relative Markdown links point to existing files."""

from __future__ import annotations

import re
import sys
from pathlib import Path
from urllib.parse import unquote


REPO_ROOT = Path(__file__).resolve().parent.parent
MARKDOWN_PATHS = [
    REPO_ROOT / "README.md",
    REPO_ROOT / "CHANGELOG.md",
    REPO_ROOT / "docsrc",
    REPO_ROOT / "examples",
]
LINK_RE = re.compile(r"(?<!\!)\[[^\]]*\]\(([^)]+)\)")
FENCE_RE = re.compile(r"^ {0,3}(`{3,}|~{3,})")


def iter_markdown_files() -> list[Path]:
    files: list[Path] = []
    for path in MARKDOWN_PATHS:
        if path.is_file():
            files.append(path)
            continue
        files.extend(sorted(path.rglob("*.md")))
    return files


def should_skip(target: str) -> bool:
    return (
        not target
        or target.startswith("#")
        or target.startswith("http://")
        or target.startswith("https://")
        or target.startswith("mailto:")
        or target.startswith("app://")
    )


def without_fenced_code(text: str) -> str:
    """Remove fenced code so C++ expressions such as operator[](x) are not links."""
    output: list[str] = []
    fence_character = ""
    fence_length = 0
    for line in text.splitlines():
        match = FENCE_RE.match(line)
        if match:
            marker = match.group(1)
            if not fence_character:
                fence_character = marker[0]
                fence_length = len(marker)
            elif marker[0] == fence_character and len(marker) >= fence_length:
                fence_character = ""
                fence_length = 0
            output.append("")
        elif fence_character:
            output.append("")
        else:
            output.append(line)
    return "\n".join(output)


def main() -> int:
    errors: list[str] = []
    for markdown_file in iter_markdown_files():
        text = without_fenced_code(markdown_file.read_text(encoding="utf-8"))
        for raw_target in LINK_RE.findall(text):
            target = raw_target.strip()
            if should_skip(target):
                continue

            target_path = unquote(target.split("#", 1)[0])
            resolved = (markdown_file.parent / target_path).resolve()
            try:
                resolved.relative_to(REPO_ROOT)
            except ValueError:
                errors.append(
                    f"{markdown_file.relative_to(REPO_ROOT)}: external path outside repo: {target}"
                )
                continue

            if not resolved.exists():
                errors.append(
                    f"{markdown_file.relative_to(REPO_ROOT)}: missing target: {target}"
                )

    if errors:
        for error in errors:
            print(error, file=sys.stderr)
        return 1

    print("Markdown link check passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
