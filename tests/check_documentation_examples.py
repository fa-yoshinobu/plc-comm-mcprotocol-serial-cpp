#!/usr/bin/env python3
"""Compile maintained C++ fences and enforce their state-change safety contract."""

from __future__ import annotations

import argparse
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
GUIDE = ROOT / "docsrc" / "user" / "USAGE_GUIDE.md"


def cpp_fence_after_heading(markdown: str, heading: str) -> str:
    heading_at = markdown.find(heading)
    if heading_at < 0:
        raise AssertionError(f"missing heading: {heading}")
    fence_at = markdown.find("```cpp\n", heading_at)
    if fence_at < 0:
        raise AssertionError(f"missing C++ fence after: {heading}")
    code_at = fence_at + len("```cpp\n")
    fence_end = markdown.find("\n```", code_at)
    if fence_end < 0:
        raise AssertionError(f"unterminated C++ fence after: {heading}")
    return markdown[code_at:fence_end] + "\n"


def require_order(code: str, *needles: str) -> None:
    position = -1
    for needle in needles:
        next_position = code.find(needle, position + 1)
        if next_position < 0:
            raise AssertionError(f"missing ordered documentation token: {needle}")
        position = next_position


def check_contract(markdown: str) -> dict[str, str]:
    write_words = cpp_fence_after_heading(markdown, "### Write words")
    random_write = cpp_fence_after_heading(markdown, "### Random read and random write")
    remote_control = cpp_fence_after_heading(
        markdown, "### Remote control and CPU model"
    )

    require_order(
        write_words,
        'plc.read_words_single_request("D100", original)',
        'plc.write_words_single_request("D100", words)',
        "StatusCode::OperationOutcomeUnknown",
        "if (!write_status.ok())",
        'plc.write_words_single_request("D100", original)',
        "restore_status.code ==",
        "if (!restore_status.ok())",
        "inspect and reconcile D100-D101 manually",
    )
    require_order(
        random_write,
        'plc.read_words_single_request("D101", original_d101)',
        "plc.write_random_words(writes)",
        "StatusCode::OperationOutcomeUnknown",
        "if (!write_status.ok())",
        'plc.write_words_single_request("D101", original_d101)',
        "restore_status.code ==",
        "if (!restore_status.ok())",
    )
    require_order(
        remote_control,
        "kControlledTestApproved = false",
        "kCpuWasRunningBeforeTest = false",
        "if (!kControlledTestApproved)",
        "if (!kCpuWasRunningBeforeTest)",
        "plc.remote_stop()",
        "STOP outcome unknown",
        "plc.remote_run(",
        "RUN outcome unknown",
        "PLC remains STOPped",
    )

    required_prose = (
        "Run this example only on a controlled test PLC",
        "do not retry\nor attempt an immediate restore",
        "Run the random-write portion only on a controlled test PLC",
        "reconciling it manually under an explicit safety policy",
        "Remote STOP/RUN changes CPU execution state",
        "the PLC may remain STOPped",
        "instead of retrying automatically",
    )
    for phrase in required_prose:
        if phrase not in markdown:
            raise AssertionError(f"missing safety guidance: {phrase}")

    return {
        "write_words": write_words,
        "random_write": random_write,
        "remote_control": remote_control,
    }


def compile_fences(compiler: str, fences: dict[str, str]) -> None:
    with tempfile.TemporaryDirectory(
        prefix=".documentation-examples-", dir=ROOT
    ) as temp:
        temp_dir = Path(temp)
        for name, code in fences.items():
            source = temp_dir / f"{name}.cpp"
            source.write_text(code, encoding="utf-8", newline="\n")
            command = [
                compiler,
                "-std=c++17",
                "-pedantic-errors",
                "-Wall",
                "-Wextra",
                f"-I{ROOT / 'include'}",
                "-fsyntax-only",
                str(source),
            ]
            result = subprocess.run(command, cwd=ROOT, text=True, capture_output=True)
            if result.returncode != 0:
                raise AssertionError(
                    f"C++17 syntax check failed for {name}:\n{result.stdout}{result.stderr}"
                )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--compiler", required=True)
    args = parser.parse_args()

    markdown = GUIDE.read_text(encoding="utf-8").replace("\r\n", "\n")
    fences = check_contract(markdown)
    compile_fences(args.compiler, fences)
    print("documentation example safety and C++17 checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
