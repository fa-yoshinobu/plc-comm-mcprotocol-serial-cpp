#!/usr/bin/env python3
"""Generate or verify the repository API reference from its fixed public inputs."""

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]

INPUTS = (
    "include/mcprotocol_serial.hpp",
    "include/mcprotocol/serial/types.hpp",
    "include/mcprotocol/serial/status.hpp",
    "include/mcprotocol/serial/byte.hpp",
    "include/mcprotocol/serial/span.hpp",
    "include/mcprotocol/serial/codec.hpp",
    "include/mcprotocol/serial/client.hpp",
    "include/mcprotocol/serial/high_level.hpp",
    "include/mcprotocol/serial/host_sync.hpp",
    "include/mcprotocol/serial/posix_serial.hpp",
    "include/mcprotocol/serial/link_direct.hpp",
    "include/mcprotocol/serial/qualified_buffer.hpp",
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="fail if the generated document is stale")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    command = [
        sys.executable,
        "scripts/generate_api_reference.py",
        "--title",
        "MC Protocol Serial C++ API Reference",
        "--output",
        "docsrc/user/API_REFERENCE.md",
    ]
    for path in INPUTS:
        command.extend(("--input", path))
    command.extend(("--predefine", "MCPROTOCOL_SERIAL_ENABLE_HOST_API=1"))
    if args.check:
        command.append("--check")
    return subprocess.run(command, cwd=ROOT, check=False).returncode


if __name__ == "__main__":
    raise SystemExit(main())
