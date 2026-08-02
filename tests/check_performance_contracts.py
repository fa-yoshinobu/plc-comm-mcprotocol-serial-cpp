#!/usr/bin/env python3
"""Guard approved hot-path contracts that are not observable through the public API."""

from __future__ import annotations

from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def read(relative: str) -> str:
    return (ROOT / relative).read_text(encoding="utf-8")


def function_body(source: str, signature: str) -> str:
    start = source.find(signature)
    if start < 0:
        raise AssertionError(f"missing function: {signature}")
    opening = source.find("{", start)
    if opening < 0:
        raise AssertionError(f"missing function body: {signature}")
    depth = 0
    for index in range(opening, len(source)):
        char = source[index]
        if char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
            if depth == 0:
                return source[opening : index + 1]
    raise AssertionError(f"unterminated function body: {signature}")


def require_before(body: str, earlier: str, later: str) -> None:
    earlier_index = body.find(earlier)
    later_index = body.find(later)
    if earlier_index < 0 or later_index < 0 or earlier_index >= later_index:
        raise AssertionError(f"expected {earlier!r} before {later!r}")


def check_configuration_validation() -> None:
    client = read("src/client.cpp")
    configure = function_body(client, "Status MelsecSerialClient::configure(")
    assert configure.count("FrameCodec::validate_config") == 1
    assert "apply_validated_config(config)" in configure
    assert client.count("FrameCodec::validate_config") == 1
    for forbidden in (
        "FrameCodec::encode_request",
        "FrameCodec::validate_request_capacity",
        "FrameCodec::validate_response_capacity",
        "FrameCodec::decode_response",
    ):
        assert forbidden not in client, f"client hot path regressed to public codec: {forbidden}"
    for required in (
        "detail::encode_request_validated",
        "detail::validate_response_capacity_validated",
        "detail::decode_response_validated",
    ):
        assert required in client, f"missing validated client codec path: {required}"

    host = read("src/host_sync.cpp")
    open_body = function_body(host, "Status PosixSyncClient::open(")
    assert open_body.count("FrameCodec::validate_config") == 1
    assert "client_.apply_validated_config" in open_body
    assert "client_.configure" not in open_body
    require_before(open_body, "FrameCodec::validate_config", "close();")
    require_before(open_body, "close();", "client_.apply_validated_config")


def check_long_state_aggregate() -> None:
    host = read("src/host_sync.cpp")
    body = function_body(
        host,
        "Status PosixSyncClient::read_long_state_bits(\n"
        "    std::string_view head_device,\n"
        "    std::uint16_t points,",
    )
    assert body.count("CommandCodec::encode_batch_read_words") == 1
    assert "FrameCodec::encode_request" not in body
    assert "detail::validate_request_capacity_validated" in body
    assert "detail::validate_response_capacity_validated" in body
    assert "detail::long_state_status_block_response_bytes(protocol_config_.code_mode())" in body
    assert "detail::execute_long_state_read_aggregate" in body
    assert "std::array<std::uint8_t, 64U> validation_request_data" in body
    for forbidden in ("kMaxRequestFrameBytes", "kMaxResponseFrameBytes", "65535U>"):
        assert forbidden not in body, f"fixed maximum-size staging returned: {forbidden}"

    aggregate = read("include/mcprotocol/serial/detail/long_state_aggregate.hpp")
    assert "(static_cast<std::size_t>(points) + 7U) / 8U" in aggregate
    first_loop = aggregate.find("for (std::uint16_t index")
    second_loop = aggregate.find("for (std::uint16_t index", first_loop + 1)
    assert 0 <= aggregate.find("allocate_bytes(staged_size)") < first_loop < second_loop
    assert "StatusCode::OutOfMemory" in aggregate


def check_drain_deadline_and_wait_policy() -> None:
    helper = function_body(
        read("include/mcprotocol/serial/detail/yield_first_wait.hpp"),
        "[[nodiscard]] Status drain_tx_with_yield_first(",
    )
    first_remaining = helper.find("remaining = remaining_timeout")
    query = helper.find("query_pending(pending)")
    second_remaining = helper.find("remaining = remaining_timeout", first_remaining + 1)
    queue_empty = helper.find("if (pending == 0U)")
    observe = helper.find("wait_policy.observe(pending)")
    assert 0 <= first_remaining < query < second_remaining < queue_empty < observe
    assert "YieldFirstWaitAction::SleepOneMillisecond" in helper
    assert "sleep_one(remaining > 1U ? 1U : remaining)" in helper
    assert "yield_now()" in helper

    cases = (
        (
            "src/posix_serial.cpp",
            "Status PosixSerialPort::drain_tx_until(",
            "::ioctl",
            "::poll",
        ),
        (
            "src/win32_serial.cpp",
            "Status PosixSerialPort::drain_tx_until(",
            "ClearCommError",
            "Sleep(",
        ),
    )
    for relative, signature, queue_query, bounded_sleep in cases:
        body = function_body(read(relative), signature)
        assert "drain_tx_with_yield_first" in body
        assert "remaining_timeout_ms" in body
        assert queue_query in body
        assert "std::this_thread::yield()" in body
        assert bounded_sleep in body


def main() -> None:
    check_configuration_validation()
    check_long_state_aggregate()
    check_drain_deadline_and_wait_policy()
    print("approved performance source contracts passed")


if __name__ == "__main__":
    main()
