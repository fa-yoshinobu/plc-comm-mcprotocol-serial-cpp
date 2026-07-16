#!/usr/bin/env python3
"""Validate the public GitHub Release source archive."""

from __future__ import annotations

import argparse
from pathlib import Path
from zipfile import BadZipFile, ZipFile


FORBIDDEN_FILES = {"AGENTS.md", "TODO.md"}
REQUIRED_FILES = {"LICENSE", "README.md", "library.json", "library.properties"}
REQUIRED_DIRECTORIES = {"examples/", "include/"}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("archive", type=Path, help="release ZIP to validate")
    args = parser.parse_args()

    try:
        with ZipFile(args.archive) as archive:
            entries = {entry.filename.replace("\\", "/") for entry in archive.infolist()}
    except (FileNotFoundError, BadZipFile) as error:
        raise SystemExit(f"Invalid release archive {args.archive}: {error}") from error

    forbidden = sorted(
        entry for entry in entries if entry.rstrip("/").rsplit("/", 1)[-1] in FORBIDDEN_FILES
    )
    if forbidden:
        raise SystemExit(
            "Release archive contains maintainer-only files: " + ", ".join(forbidden)
        )

    missing_files = sorted(REQUIRED_FILES - entries)
    missing_directories = sorted(
        prefix
        for prefix in REQUIRED_DIRECTORIES
        if not any(entry.startswith(prefix) for entry in entries)
    )
    if missing_files or missing_directories:
        missing = missing_files + missing_directories
        raise SystemExit("Release archive is missing required content: " + ", ".join(missing))

    print(f"Release archive content OK: {args.archive}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
