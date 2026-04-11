#!/usr/bin/env bash
set -euo pipefail

cli="${MCPROTOCOL_CLI:-./build/mcprotocol_cli}"
device="${MCPROTOCOL_DEVICE:-/dev/ttyUSB0}"
baud="${MCPROTOCOL_BAUD:-19200}"
data_bits="${MCPROTOCOL_DATA_BITS:-8}"
stop_bits="${MCPROTOCOL_STOP_BITS:-1}"
parity="${MCPROTOCOL_PARITY:-E}"
frame="${MCPROTOCOL_FRAME:-c4-ascii-f4}"
sum_check="${MCPROTOCOL_SUM_CHECK:-off}"
station="${MCPROTOCOL_STATION:-0}"
series="${MCPROTOCOL_SERIES:-}"
rts_toggle="${MCPROTOCOL_RTS_TOGGLE:-off}"
duration_sec="${MCPROTOCOL_DURATION_SEC:-180}"
response_timeout_ms="${MCPROTOCOL_RESPONSE_TIMEOUT_MS:-}"
inter_byte_timeout_ms="${MCPROTOCOL_INTER_BYTE_TIMEOUT_MS:-}"

# These families completed non-low-address read/write/read/restore command
# screening without protocol-level errors. Some values may be overwritten again
# by the PLC program while RUN is active, so this soak treats readback mismatch
# as observation data instead of a hard failure.
targets=(
  "bit:STS10"
  "bit:STC10"
  "word:STN10"
  "bit:TS10"
  "bit:TC10"
  "word:TN10"
  "bit:CS10"
  "bit:CC10"
  "word:CN10"
  "bit:SB10"
  "word:SW10"
  "bit:DX10"
  "bit:DY10"
  "word:ZR10"
  "bit:X10"
  "bit:Y10"
  "bit:M100"
  "bit:L100"
  "bit:F100"
  "bit:V100"
  "bit:B10"
  "word:D100"
  "word:W10"
  "word:Z10"
  "word:R100"
)

common_args=(
  --device "${device}"
  --baud "${baud}"
  --data-bits "${data_bits}"
  --stop-bits "${stop_bits}"
  --parity "${parity}"
  --frame "${frame}"
  --sum-check "${sum_check}"
  --station "${station}"
  --rts-toggle "${rts_toggle}"
)

if [[ -n "${series}" ]]; then
  common_args+=(--series "${series}")
fi

if [[ -n "${response_timeout_ms}" ]]; then
  common_args+=(--response-timeout-ms "${response_timeout_ms}")
fi

if [[ -n "${inter_byte_timeout_ms}" ]]; then
  common_args+=(--inter-byte-timeout-ms "${inter_byte_timeout_ms}")
fi

run_cli_capture() {
  "${cli}" "${common_args[@]}" "$@"
}

read_scalar() {
  local kind="$1"
  local target="$2"
  local output=""
  local value=""

  if [[ "${kind}" == "bit" ]]; then
    if ! output="$(run_cli_capture read-bits "${target}" 1)"; then
      printf '%s\n' "${output}" >&2
      return 1
    fi
  else
    if ! output="$(run_cli_capture read-words "${target}" 1)"; then
      printf '%s\n' "${output}" >&2
      return 1
    fi
  fi

  value="$(printf '%s\n' "${output}" | awk 'NR == 1 {print $NF}')"
  if [[ ! "${value}" =~ ^[0-9]+$ ]]; then
    printf 'unexpected read output for %s: %s\n' "${target}" "${output}" >&2
    return 1
  fi

  REPLY="${value}"
}

write_scalar() {
  local kind="$1"
  local target="$2"
  local value="$3"
  local output=""

  if [[ "${kind}" == "bit" ]]; then
    if ! output="$(run_cli_capture write-bits "${target}=${value}")"; then
      printf '%s\n' "${output}" >&2
      return 1
    fi
  else
    if ! output="$(run_cli_capture write-words "${target}=${value}")"; then
      printf '%s\n' "${output}" >&2
      return 1
    fi
  fi
}

format_value() {
  local kind="$1"
  local value="$2"
  if [[ "${kind}" == "bit" ]]; then
    printf '%s' "${value}"
  else
    printf '0x%04X' "${value}"
  fi
}

attempt_restore() {
  local kind="$1"
  local target="$2"
  local original="$3"
  if ! write_scalar "${kind}" "${target}" "${original}"; then
    printf '%-6s restore-write error\n' "${target}" >&2
    return 1
  fi
  return 0
}

exercise_target() {
  local kind="$1"
  local target="$2"
  local original=""
  local test_value=""
  local readback=""
  local restored=""
  local verify_note=""
  local restore_note=""

  if ! read_scalar "${kind}" "${target}"; then
    printf '%-6s read error\n' "${target}" >&2
    return 1
  fi
  original="${REPLY}"

  if [[ "${kind}" == "bit" ]]; then
    if [[ "${original}" == "0" ]]; then
      test_value="1"
    else
      test_value="0"
    fi
  else
    test_value="$(((original ^ 1) & 0xFFFF))"
  fi

  if ! write_scalar "${kind}" "${target}" "${test_value}"; then
    printf '%-6s write error\n' "${target}" >&2
    return 1
  fi

  if ! read_scalar "${kind}" "${target}"; then
    printf '%-6s verify-read error\n' "${target}" >&2
    attempt_restore "${kind}" "${target}" "${original}" || true
    return 1
  fi
  readback="${REPLY}"
  if [[ "${readback}" != "${test_value}" ]]; then
    verify_note=" verify-mismatch"
  fi

  if ! attempt_restore "${kind}" "${target}" "${original}"; then
    return 1
  fi

  if ! read_scalar "${kind}" "${target}"; then
    printf '%-6s restore-read error\n' "${target}" >&2
    return 1
  fi
  restored="${REPLY}"
  if [[ "${restored}" != "${original}" ]]; then
    restore_note=" restore-mismatch"
  fi

  printf '%-6s ok %s->%s->%s%s%s\n' \
    "${target}" \
    "$(format_value "${kind}" "${original}")" \
    "$(format_value "${kind}" "${test_value}")" \
    "$(format_value "${kind}" "${restored}")" \
    "${verify_note}" \
    "${restore_note}"
}

if [[ ! "${duration_sec}" =~ ^[0-9]+$ ]]; then
  printf 'invalid MCPROTOCOL_DURATION_SEC: %s\n' "${duration_sec}" >&2
  exit 2
fi

model_output="$(run_cli_capture cpu-model)"
printf '%s\n' "${model_output}"
printf 'supported-device-rw-soak: duration_sec=%s targets=%s\n' "${duration_sec}" "${#targets[@]}"

start_time="${SECONDS}"
end_time=$((SECONDS + duration_sec))
cycles=0
checks=0

while (( SECONDS < end_time )); do
  for entry in "${targets[@]}"; do
    if (( SECONDS >= end_time )); then
      break
    fi

    IFS=: read -r kind target <<<"${entry}"
    if ! exercise_target "${kind}" "${target}"; then
      elapsed_sec=$((SECONDS - start_time))
      printf 'supported-device-rw-soak: fail cycle=%s checks=%s elapsed_sec=%s target=%s\n' \
        "${cycles}" \
        "${checks}" \
        "${elapsed_sec}" \
        "${target}" >&2
      exit 1
    fi
    checks=$((checks + 1))
  done
  cycles=$((cycles + 1))
done

elapsed_sec=$((SECONDS - start_time))
printf 'supported-device-rw-soak: pass cycles=%s checks=%s elapsed_sec=%s\n' \
  "${cycles}" \
  "${checks}" \
  "${elapsed_sec}"
