#!/usr/bin/env python3
"""Verify the approved host-sync API identity and thin-wrapper source contract."""

from __future__ import annotations

import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
HEADER = (ROOT / "include/mcprotocol/serial/host_sync.hpp").read_text(encoding="utf-8")
SOURCE = (ROOT / "src/host_sync.cpp").read_text(encoding="utf-8")
HOST_SERIAL = (ROOT / "include/mcprotocol/serial/host_serial.hpp").read_text(
    encoding="utf-8"
)
COMPAT_SERIAL = (ROOT / "include/mcprotocol/serial/posix_serial.hpp").read_text(
    encoding="utf-8"
)


def compact(text: str) -> str:
    return re.sub(r"\s+", "", text)


def method_body(name: str) -> str:
    marker = f"Status HostSyncClient::{name}("
    start = SOURCE.find(marker)
    if start < 0:
        raise AssertionError(f"missing canonical source definition: {name}")
    brace = SOURCE.find("{", start)
    if brace < 0:
        raise AssertionError(f"missing body: {name}")
    depth = 0
    for index in range(brace, len(SOURCE)):
        if SOURCE[index] == "{":
            depth += 1
        elif SOURCE[index] == "}":
            depth -= 1
            if depth == 0:
                return SOURCE[brace + 1 : index]
    raise AssertionError(f"unterminated body: {name}")


SIGNATURES = {
    "read_random_link_direct_words": """
        Status read_random_link_direct_words(
            Span<const LinkDirectRandomReadWordItem> word_items,
            Span<std::uint16_t> out_words) noexcept;
    """,
    "write_random_link_direct_words": """
        Status write_random_link_direct_words(
            Span<const LinkDirectRandomWriteWordItem> items) noexcept;
    """,
    "write_random_link_direct_bits": """
        Status write_random_link_direct_bits(
            Span<const LinkDirectRandomWriteBitItem> items) noexcept;
    """,
    "self_test_loopback": """
        Status self_test_loopback(
            Span<const char> hex_ascii,
            Span<char> out_echoed) noexcept;
    """,
    "read_block": """
        Status read_block(
            const MultiBlockReadRequest& request,
            Span<std::uint16_t> out_words,
            Span<BitValue> out_bits,
            Span<MultiBlockReadBlockResult> out_results) noexcept;
    """,
    "write_block": """
        Status write_block(const MultiBlockWriteRequest& request) noexcept;
    """,
    "read_link_direct_block": """
        Status read_link_direct_block(
            const LinkDirectMultiBlockReadRequest& request,
            Span<std::uint16_t> out_words,
            Span<BitValue> out_bits,
            Span<MultiBlockReadBlockResult> out_results) noexcept;
    """,
    "write_link_direct_block": """
        Status write_link_direct_block(
            const LinkDirectMultiBlockWriteRequest& request) noexcept;
    """,
    "register_link_direct_monitor_devices": """
        Status register_link_direct_monitor_devices(
            const LinkDirectMonitorRegistration& request) noexcept;
    """,
}

ASYNC_TARGETS = {
    "read_random_link_direct_words": "async_link_direct_random_read",
    "write_random_link_direct_words": "async_link_direct_random_write_words",
    "write_random_link_direct_bits": "async_link_direct_random_write_bits",
    "self_test_loopback": "async_loopback",
    "read_block": "async_multi_block_read",
    "write_block": "async_multi_block_write",
    "read_link_direct_block": "async_link_direct_multi_block_read",
    "write_link_direct_block": "async_link_direct_multi_block_write",
    "register_link_direct_monitor_devices": "async_link_direct_register_monitor",
}


def main() -> int:
    normalized_header = compact(HEADER)
    for name, signature in SIGNATURES.items():
        if compact(signature) not in normalized_header:
            raise AssertionError(f"public signature drift: {name}")
        body = method_body(name)
        async_name = ASYNC_TARGETS[name]
        if body.count(async_name) != 1:
            raise AssertionError(f"{name} must invoke {async_name} exactly once")
        if body.count("run_until_complete()") != 1:
            raise AssertionError(f"{name} must use the existing sync driver exactly once")
        for forbidden in ("retry", "fallback", "split", "std::vector", "new "):
            if forbidden in body:
                raise AssertionError(f"{name} contains forbidden wrapper behavior: {forbidden}")

    public_headers = "\n".join(
        path.read_text(encoding="utf-8")
        for path in (ROOT / "include/mcprotocol/serial").glob("*.hpp")
    )
    for forbidden_type in (
        "LinkDirectRandomReadWordSpec",
        "LinkDirectRandomWriteWordSpec",
        "LinkDirectRandomWriteBitSpec",
    ):
        if forbidden_type in public_headers:
            raise AssertionError(f"unapproved public helper type: {forbidden_type}")
    if "run_link_direct_monitor_cycle" in public_headers:
        raise AssertionError("link-direct monitor must use the common run_monitor_cycle API")

    if "class HostSyncClient" not in HEADER or "using PosixSyncClient" not in HEADER:
        raise AssertionError("HostSyncClient canonical type or PosixSyncClient alias is missing")
    if "struct HostSerialConfig" not in HOST_SERIAL or "class HostSerialPort" not in HOST_SERIAL:
        raise AssertionError("OS-neutral host serial types are missing")
    if '#include "mcprotocol/serial/host_serial.hpp"' not in COMPAT_SERIAL:
        raise AssertionError("posix_serial.hpp must remain a compatibility include")

    print("host sync API identity and thin-wrapper contracts passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
