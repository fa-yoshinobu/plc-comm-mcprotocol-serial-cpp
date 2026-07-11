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


def require_open_error(cli: Path, arguments: list[str], label: str) -> None:
    result = run(cli, arguments)
    if result.returncode != 1 or "Failed to open serial port" not in result.stderr:
        raise AssertionError(
            f"{label}: expected arguments to pass validation and reach serial open, "
            f"got {result.returncode}\nstdout={result.stdout!r}\nstderr={result.stderr!r}"
        )


def without_option(arguments: list[str], option: str) -> list[str]:
    index = arguments.index(option)
    return arguments[:index] + arguments[index + 2 :]


def main() -> int:
    if len(sys.argv) != 2:
        raise SystemExit("usage: cli_serial_config_tests.py CLI_PATH")
    cli = Path(sys.argv[1])

    usage = run(cli, [])
    if "response timeout after TX (default: 3000)" not in usage.stderr:
        raise AssertionError("CLI usage must advertise the 3000 ms response timeout default")
    if "1E timer in exact 250 ms units (default: 4000)" not in usage.stderr:
        raise AssertionError("CLI usage must advertise the independent 4000 ms 1E timer default")
    if "Inter-byte timeout in milliseconds (default: 250)" not in usage.stderr:
        raise AssertionError("CLI usage must advertise the 250 ms inter-byte timeout default")
    if "remote-run requires both conflict mode and clear mode" not in usage.stderr:
        raise AssertionError("CLI usage must require both remote-run policies")
    if "remote-run defaults to" in usage.stderr:
        raise AssertionError("CLI usage must not advertise remote-run policy defaults")
    if "remote-pause requires an explicit conflict mode" not in usage.stderr:
        raise AssertionError("CLI usage must require the remote-pause policy")
    if "remote-pause defaults to" in usage.stderr:
        raise AssertionError("CLI usage must not advertise a remote-pause policy default")

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

    random_base = binary_seven.copy()
    random_base[random_base.index("--data-bits") + 1] = "8"
    for item, label in (
        ("D100", "random read without width"),
        ("auto:D100", "random read unknown width"),
        ("word:", "random read missing device"),
        ("dword:D100:D", "random read suffix width"),
    ):
        require_parse_error(cli, [*random_base[:-1], "random-read", item], label)

    require_open_error(
        cli,
        [*random_base[:-1], "random-read", "word:D100", "dword:D200"],
        "explicit mixed random read widths",
    )
    require_parse_error(
        cli,
        [*random_base[:-1], "random-write-words", "D100=65536"],
        "random Word write overflow",
    )
    require_open_error(
        cli,
        [*random_base[:-1], "random-write-words", "D100=65535"],
        "random Word write maximum",
    )
    require_open_error(
        cli,
        [*random_base[:-1], "random-write-dwords", "D100=0xFFFFFFFF"],
        "random DWord write maximum",
    )
    for command, item, label in (
        ("random-write-words", "D100", "random Word write missing value"),
        ("random-write-words", "D100=", "random Word write empty value"),
        ("random-write-dwords", "D100", "random DWord write missing value"),
        ("random-write-dwords", "D100=", "random DWord write empty value"),
        ("random-write-bits", "M100", "random bit write missing value"),
        ("random-write-bits", "M100=", "random bit write empty value"),
        ("random-write-bits", "M100=2", "random bit write unknown value"),
        ("random-write-link-direct-words", "J1\\W100", "link-direct Word write missing value"),
        ("random-write-link-direct-words", "J1\\W100=", "link-direct Word write empty value"),
        ("random-write-link-direct-bits", "J1\\B10", "link-direct bit write missing value"),
        ("random-write-link-direct-bits", "J1\\B10=2", "link-direct bit write unknown value"),
    ):
        require_parse_error(cli, [*random_base[:-1], command, item], label)
    for command, item, label in (
        ("random-write-words", "D100=0", "random Word write explicit zero"),
        ("random-write-dwords", "D100=0", "random DWord write explicit zero"),
        ("random-write-bits", "M100=0", "random bit write explicit OFF"),
        ("random-write-bits", "M100=1", "random bit write explicit ON"),
        ("random-write-link-direct-words", "J1\\W100=0", "link-direct Word write explicit zero"),
        ("random-write-link-direct-bits", "J1\\B10=0", "link-direct bit write explicit OFF"),
    ):
        require_open_error(cli, [*random_base[:-1], command, item], label)

    for value, label in (
        ("1", "one millisecond response timeout"),
        ("3000", "default response timeout"),
        ("2147483647", "maximum wrap-safe response timeout"),
    ):
        valid_timeout = binary_seven.copy()
        valid_timeout[-1:-1] = ["--response-timeout-ms", value]
        require_validation_error(cli, valid_timeout, label)

    for value, label in (
        ("0", "zero response timeout"),
        ("2147483648", "non-wrap-safe response timeout"),
        ("4294967295", "maximum uint32 response timeout"),
    ):
        invalid_timeout = binary_seven.copy()
        invalid_timeout[invalid_timeout.index("--data-bits") + 1] = "8"
        invalid_timeout[-1:-1] = ["--response-timeout-ms", value]
        require_protocol_validation_error(cli, invalid_timeout, label)

    for value, label in (("-1", "negative response timeout"), ("invalid", "nonnumeric response timeout")):
        invalid_timeout_text = binary_seven.copy()
        invalid_timeout_text[-1:-1] = ["--response-timeout-ms", value]
        require_parse_error(cli, invalid_timeout_text, label)

    for value, label in (
        ("1", "one millisecond inter-byte timeout"),
        ("250", "default inter-byte timeout"),
        ("2147483647", "maximum wrap-safe inter-byte timeout"),
    ):
        valid_inter_byte = binary_seven.copy()
        valid_inter_byte[-1:-1] = ["--inter-byte-timeout-ms", value]
        require_validation_error(cli, valid_inter_byte, label)

    for value, label in (
        ("0", "zero inter-byte timeout"),
        ("2147483648", "non-wrap-safe inter-byte timeout"),
        ("4294967295", "maximum uint32 inter-byte timeout"),
    ):
        invalid_inter_byte = binary_seven.copy()
        invalid_inter_byte[invalid_inter_byte.index("--data-bits") + 1] = "8"
        invalid_inter_byte[-1:-1] = ["--inter-byte-timeout-ms", value]
        require_protocol_validation_error(cli, invalid_inter_byte, label)

    for value, label in (("-1", "negative inter-byte timeout"), ("invalid", "nonnumeric inter-byte timeout")):
        invalid_inter_byte_text = binary_seven.copy()
        invalid_inter_byte_text[-1:-1] = ["--inter-byte-timeout-ms", value]
        require_parse_error(cli, invalid_inter_byte_text, label)

    remote_run = [
        *explicit_serial[:-1],
        "--frame",
        "c4-binary",
        "--plc-profile",
        "melsec:iq-r",
        "--sum-check",
        "on",
        "--route",
        "host",
        "remote-run",
    ]
    require_parse_error(cli, remote_run, "remote-run missing both policies")
    require_parse_error(
        cli,
        [*remote_run, "no-force"],
        "remote-run missing clear policy",
    )
    require_parse_error(
        cli,
        [*remote_run, "outside-latch"],
        "remote-run clear policy cannot replace conflict policy",
    )
    require_parse_error(
        cli,
        [*remote_run, "invalid", "no-clear"],
        "remote-run unknown conflict policy",
    )
    require_parse_error(
        cli,
        [*remote_run, "no-force", "invalid"],
        "remote-run unknown clear policy",
    )
    require_parse_error(
        cli,
        [*remote_run, "no-force", "no-clear", "extra"],
        "remote-run extra policy argument",
    )
    for alias in ("normal", "safe", "1", "0001", "3", "0003"):
        require_parse_error(
            cli,
            [*remote_run, alias, "no-clear"],
            f"remote-run rejects conflict alias {alias}",
        )
    for conflict_policy in ("no-force", "force"):
        for clear_policy in ("no-clear", "outside-latch", "all-clear"):
            require_open_error(
                cli,
                [*remote_run, conflict_policy, clear_policy],
                f"remote-run {conflict_policy} {clear_policy}",
            )

    remote_pause = [*remote_run[:-1], "remote-pause"]
    require_parse_error(cli, remote_pause, "remote-pause missing conflict policy")
    require_parse_error(
        cli,
        [*remote_pause, "invalid"],
        "remote-pause unknown conflict policy",
    )
    require_parse_error(
        cli,
        [*remote_pause, "no-force", "extra"],
        "remote-pause extra policy argument",
    )
    for alias in ("normal", "safe", "1", "0001", "3", "0003"):
        require_parse_error(
            cli,
            [*remote_pause, alias],
            f"remote-pause rejects conflict alias {alias}",
        )
    for conflict_policy in ("no-force", "force"):
        require_open_error(
            cli,
            [*remote_pause, conflict_policy],
            f"remote-pause {conflict_policy}",
        )

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
        "--topology",
        "standard",
        "--network",
        "0",
        "--pc-target",
        "connected",
        "--module-target",
        "own",
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
        "--topology",
        "standard",
        "--station",
        "0",
        "--pc-target",
        "connected",
        "--module-target",
        "own",
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
        "--topology",
        "standard",
        "--station",
        "0",
        "--network",
        "0",
        "--pc-target",
        "connected",
        "--module-target",
        "own",
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
        "--topology",
        "standard",
        "--station",
        "0",
        "--network",
        "0",
        "--module-target",
        "own",
        "cpu-model",
    ]
    require_parse_error(cli, multidrop_missing_pc_target, "4C multidrop missing PC target")

    multidrop_missing_topology = without_option(explicit_zero_multidrop, "--topology")
    require_parse_error(cli, multidrop_missing_topology, "4C multidrop missing topology")

    standard_with_self_station = explicit_zero_multidrop.copy()
    standard_with_self_station[-1:-1] = ["--self-station", "0"]
    require_parse_error(
        cli, standard_with_self_station, "standard topology with self-station"
    )

    mn_missing_self_station = explicit_zero_multidrop.copy()
    mn_missing_self_station[mn_missing_self_station.index("--topology") + 1] = "mn"
    require_parse_error(cli, mn_missing_self_station, "m:n topology missing self-station")

    for value, label in (("0", "m:n self-station zero"), ("31", "m:n self-station 31")):
        valid_mn = mn_missing_self_station.copy()
        valid_mn[-1:-1] = ["--self-station", value]
        require_validation_error(cli, valid_mn, label)

    invalid_mn = mn_missing_self_station.copy()
    invalid_mn[invalid_mn.index("--data-bits") + 1] = "8"
    invalid_mn[-1:-1] = ["--self-station", "32"]
    require_protocol_validation_error(cli, invalid_mn, "m:n self-station 32")

    for value, label in (("-1", "negative self-station"), ("invalid", "nonnumeric self-station")):
        invalid_self_text = mn_missing_self_station.copy()
        invalid_self_text[-1:-1] = ["--self-station", value]
        require_parse_error(cli, invalid_self_text, label)

    multidrop_missing_module_target = explicit_zero_multidrop.copy()
    module_index = multidrop_missing_module_target.index("--module-target")
    del multidrop_missing_module_target[module_index : module_index + 2]
    require_parse_error(
        cli, multidrop_missing_module_target, "4C multidrop missing module target"
    )

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
        ("own", "own-station module target"),
        ("cpu-1", "multiple CPU 1 module target"),
        ("cpu-4", "multiple CPU 4 module target"),
        ("redundant-control", "redundant control module target"),
        ("redundant-standby", "redundant standby module target"),
        ("redundant-a", "redundant system A module target"),
        ("redundant-b", "redundant system B module target"),
        ("explicit:0x1234:0x56", "explicit module target"),
    ):
        named_module_target = explicit_zero_multidrop.copy()
        named_module_target[named_module_target.index("--module-target") + 1] = value
        require_validation_error(cli, named_module_target, label)

    for value, label in (
        ("cpu-5", "out-of-range multiple CPU selector"),
        ("explicit:0x10000:0", "overflow module I/O number"),
        ("explicit:0:0x100", "overflow module station number"),
    ):
        invalid_module_target = explicit_zero_multidrop.copy()
        invalid_module_target[invalid_module_target.index("--data-bits") + 1] = "8"
        invalid_module_target[invalid_module_target.index("--module-target") + 1] = value
        require_protocol_validation_error(cli, invalid_module_target, label)

    for value, label in (
        ("explicit:-1:0", "negative module I/O number"),
        ("explicit:1:invalid", "nonnumeric module station number"),
        ("explicit:1", "missing explicit module station number"),
    ):
        malformed_module_target = explicit_zero_multidrop.copy()
        malformed_module_target[malformed_module_target.index("--module-target") + 1] = value
        require_parse_error(cli, malformed_module_target, label)

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

    for value, label in (
        ("0", "1E zero monitoring timer"),
        ("250", "1E one-tick monitoring timer"),
        ("4000", "1E default monitoring timer"),
        ("16383750", "1E maximum monitoring timer"),
    ):
        valid_e1_timer = e1_raw_connected.copy()
        valid_e1_timer[-1:-1] = ["--e1-monitoring-timer-ms", value]
        require_validation_error(cli, valid_e1_timer, label)

    for value, label in (
        ("1", "1E non-unit monitoring timer"),
        ("249", "1E below-one-tick monitoring timer"),
        ("251", "1E non-unit monitoring timer above one tick"),
        ("16384000", "1E monitoring timer overflow"),
    ):
        invalid_e1_timer = e1_raw_connected.copy()
        invalid_e1_timer[invalid_e1_timer.index("--data-bits") + 1] = "8"
        invalid_e1_timer[-1:-1] = ["--e1-monitoring-timer-ms", value]
        require_protocol_validation_error(cli, invalid_e1_timer, label)

    for value, label in (("-1", "negative 1E monitoring timer"), ("invalid", "nonnumeric 1E monitoring timer")):
        invalid_e1_timer_text = e1_raw_connected.copy()
        invalid_e1_timer_text[-1:-1] = ["--e1-monitoring-timer-ms", value]
        require_parse_error(cli, invalid_e1_timer_text, label)

    c4_with_e1_timer = explicit_zero_multidrop.copy()
    c4_with_e1_timer[-1:-1] = ["--e1-monitoring-timer-ms", "4000"]
    require_parse_error(cli, c4_with_e1_timer, "1E monitoring timer on a 4C route")

    recover_with_e1_timer = explicit_serial.copy()
    recover_with_e1_timer[-1:-1] = ["--e1-monitoring-timer-ms", "4000"]
    require_parse_error(cli, recover_with_e1_timer, "1E monitoring timer on raw C24 recovery")

    host_with_pc_target = host_with_station.copy()
    station_index = host_with_pc_target.index("--station")
    host_with_pc_target[station_index : station_index + 2] = ["--pc-target", "connected"]
    require_parse_error(cli, host_with_pc_target, "host route with PC target")

    host_with_module_target = host_with_station.copy()
    station_index = host_with_module_target.index("--station")
    host_with_module_target[station_index : station_index + 2] = ["--module-target", "own"]
    require_parse_error(cli, host_with_module_target, "host route with module target")

    host_with_topology = host_with_station.copy()
    station_index = host_with_topology.index("--station")
    host_with_topology[station_index : station_index + 2] = ["--topology", "standard"]
    require_parse_error(cli, host_with_topology, "host route with topology")

    c3_with_module_target = explicit_zero_multidrop.copy()
    c3_with_module_target[c3_with_module_target.index("--frame") + 1] = "c3-ascii-f4"
    require_parse_error(cli, c3_with_module_target, "3C route with module target")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
