#!/usr/bin/env python3
"""Verify CLI serial settings are explicit and rejected before OS port access."""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path


def run(cli: Path, arguments: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [str(cli), *arguments],
        check=False,
        capture_output=True,
        text=True,
        timeout=10,
    )


def require_parse_error(cli: Path, arguments: list[str], label: str) -> None:
    result = run(cli, arguments)
    if result.returncode != 2 or "Usage:" not in result.stderr:
        raise AssertionError(
            f"{label}: expected parse rejection (exit 2), got {result.returncode}\n"
            f"stdout={result.stdout!r}\nstderr={result.stderr!r}"
        )


def require_validation_error(cli: Path, arguments: list[str], label: str) -> None:
    result = run(cli, arguments)
    if result.returncode != 1 or "Invalid serial configuration" not in result.stderr:
        raise AssertionError(
            f"{label}: expected pre-open validation rejection (exit 1), got {result.returncode}\n"
            f"stdout={result.stdout!r}\nstderr={result.stderr!r}"
        )


def require_protocol_validation_error(cli: Path, arguments: list[str], label: str) -> None:
    result = run(cli, arguments)
    if result.returncode != 1 or "Invalid protocol configuration" not in result.stderr:
        raise AssertionError(
            f"{label}: expected pre-open protocol rejection (exit 1), got {result.returncode}\n"
            f"stdout={result.stdout!r}\nstderr={result.stderr!r}"
        )


def without_option(arguments: list[str], option: str) -> list[str]:
    index = arguments.index(option)
    return arguments[:index] + arguments[index + 2 :]


def main() -> int:
    if len(sys.argv) != 2:
        raise SystemExit("usage: cli_serial_config_tests.py CLI_PATH")
    cli = Path(sys.argv[1])

    explicit_serial = [
        "--device",
        "__must_not_be_opened__",
        "--baud",
        "19200",
        "--data-bits",
        "8",
        "--stop-bits",
        "1",
        "--parity",
        "E",
        "--hardware-flow",
        "none",
        "recover-c24",
    ]
    for option in (
        "--device",
        "--baud",
        "--data-bits",
        "--stop-bits",
        "--parity",
        "--hardware-flow",
    ):
        require_parse_error(cli, without_option(explicit_serial, option), f"missing {option}")

    unknown_parity = explicit_serial.copy()
    unknown_parity[unknown_parity.index("--parity") + 1] = "mark"
    require_parse_error(cli, unknown_parity, "unknown parity")

    unknown_flow = explicit_serial.copy()
    unknown_flow[unknown_flow.index("--hardware-flow") + 1] = "xon-xoff"
    require_parse_error(cli, unknown_flow, "unknown hardware flow")

    for option, value, label in (
        ("--baud", "0", "zero baud"),
        ("--data-bits", "5", "five data bits"),
        ("--data-bits", "6", "six data bits"),
        ("--data-bits", "263", "out-of-range data bits"),
        ("--stop-bits", "0", "zero stop bits"),
        ("--stop-bits", "3", "three stop bits"),
        ("--stop-bits", "257", "out-of-range stop bits"),
    ):
        invalid = explicit_serial.copy()
        invalid[invalid.index(option) + 1] = value
        require_validation_error(cli, invalid, label)

    binary_seven = explicit_serial[:-1] + [
        "--frame",
        "c4-binary",
        "--plc-profile",
        "melsec:qcpu",
        "--sum-check",
        "on",
        "--route",
        "host",
        "cpu-model",
    ]
    binary_seven[binary_seven.index("--data-bits") + 1] = "7"
    require_validation_error(cli, binary_seven, "binary with seven data bits")

    missing_sum_check = [
        *explicit_serial[:-1],
        "--frame",
        "c4-binary",
        "--plc-profile",
        "melsec:qcpu",
        "--route",
        "host",
        "cpu-model",
    ]
    require_parse_error(cli, missing_sum_check, "missing sum-check mode")

    missing_route = [
        *explicit_serial[:-1],
        "--frame",
        "c4-binary",
        "--plc-profile",
        "melsec:qcpu",
        "--sum-check",
        "on",
        "cpu-model",
    ]
    require_parse_error(cli, missing_route, "missing route")

    host_with_station = [
        *explicit_serial[:-1],
        "--frame",
        "c4-binary",
        "--plc-profile",
        "melsec:qcpu",
        "--sum-check",
        "on",
        "--route",
        "host",
        "--station",
        "0",
        "cpu-model",
    ]
    require_parse_error(cli, host_with_station, "host route with mutable station")

    multidrop_missing_station = [
        *explicit_serial[:-1],
        "--frame",
        "c4-binary",
        "--plc-profile",
        "melsec:qcpu",
        "--sum-check",
        "on",
        "--route",
        "multidrop",
        "--network",
        "0",
        "--pc-target",
        "connected",
        "cpu-model",
    ]
    require_parse_error(cli, multidrop_missing_station, "multidrop missing station")

    multidrop_missing_network = [
        *explicit_serial[:-1],
        "--frame",
        "c4-binary",
        "--plc-profile",
        "melsec:qcpu",
        "--sum-check",
        "on",
        "--route",
        "multidrop",
        "--station",
        "0",
        "--pc-target",
        "connected",
        "cpu-model",
    ]
    require_parse_error(cli, multidrop_missing_network, "4C multidrop missing network")

    explicit_zero_multidrop = [
        *explicit_serial[:-1],
        "--frame",
        "c4-binary",
        "--plc-profile",
        "melsec:qcpu",
        "--sum-check",
        "on",
        "--route",
        "multidrop",
        "--station",
        "0",
        "--network",
        "0",
        "--pc-target",
        "connected",
        "cpu-model",
    ]
    explicit_zero_multidrop[explicit_zero_multidrop.index("--data-bits") + 1] = "7"
    require_validation_error(cli, explicit_zero_multidrop, "explicit zero station and network")

    multidrop_missing_pc_target = [
        *explicit_serial[:-1],
        "--frame",
        "c4-binary",
        "--plc-profile",
        "melsec:qcpu",
        "--sum-check",
        "on",
        "--route",
        "multidrop",
        "--station",
        "0",
        "--network",
        "0",
        "cpu-model",
    ]
    require_parse_error(cli, multidrop_missing_pc_target, "4C multidrop missing PC target")

    invalid_pc_target = [
        *explicit_zero_multidrop,
    ]
    invalid_pc_target[invalid_pc_target.index("--data-bits") + 1] = "8"
    invalid_pc_target[invalid_pc_target.index("--pc-target") + 1] = "256"
    require_protocol_validation_error(cli, invalid_pc_target, "out-of-range PC target")

    for value, label in (("0", "zero PC target"),):
        invalid_number = invalid_pc_target.copy()
        invalid_number[invalid_number.index("--pc-target") + 1] = value
        require_protocol_validation_error(cli, invalid_number, label)

    for value, label in (("-1", "negative PC target"), ("invalid", "nonnumeric PC target")):
        invalid_text = invalid_pc_target.copy()
        invalid_text[invalid_text.index("--pc-target") + 1] = value
        require_parse_error(cli, invalid_text, label)

    named_pc_target = explicit_zero_multidrop.copy()
    named_pc_target[named_pc_target.index("--pc-target") + 1] = "control"
    require_validation_error(cli, named_pc_target, "named control-system PC target")

    for value, label in (
        ("0x7d", "raw control-system PC target"),
        ("0x7e", "raw standby-system PC target"),
        ("0xfe", "raw FE PC target"),
        ("0xff", "raw connected-station PC target"),
    ):
        canonical_special = explicit_zero_multidrop.copy()
        canonical_special[canonical_special.index("--pc-target") + 1] = value
        require_validation_error(cli, canonical_special, label)

    e1_raw_connected = [
        *explicit_serial[:-1],
        "--frame",
        "e1-binary",
        "--plc-profile",
        "melsec:a",
        "--sum-check",
        "on",
        "--route",
        "multidrop",
        "--pc-target",
        "0xff",
        "cpu-model",
    ]
    e1_raw_connected[e1_raw_connected.index("--data-bits") + 1] = "7"
    require_validation_error(cli, e1_raw_connected, "1E raw connected-station PC target")

    host_with_pc_target = host_with_station.copy()
    station_index = host_with_pc_target.index("--station")
    host_with_pc_target[station_index : station_index + 2] = ["--pc-target", "connected"]
    require_parse_error(cli, host_with_pc_target, "host route with PC target")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
