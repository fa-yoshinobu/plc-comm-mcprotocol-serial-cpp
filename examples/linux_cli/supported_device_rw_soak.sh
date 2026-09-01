#!/usr/bin/env bash
set -euo pipefail

cli="${MCPROTOCOL_CLI:-./build/mcprotocol_cli}"
: "${MCPROTOCOL_DEVICE:?set MCPROTOCOL_DEVICE explicitly, e.g. /dev/ttyUSB0}"
: "${MCPROTOCOL_BAUD:?set MCPROTOCOL_BAUD explicitly, e.g. 19200}"
: "${MCPROTOCOL_DATA_BITS:?set MCPROTOCOL_DATA_BITS explicitly, 7 or 8}"
: "${MCPROTOCOL_STOP_BITS:?set MCPROTOCOL_STOP_BITS explicitly, 1 or 2}"
: "${MCPROTOCOL_PARITY:?set MCPROTOCOL_PARITY explicitly, N, E, or O}"
: "${MCPROTOCOL_HARDWARE_FLOW:?set MCPROTOCOL_HARDWARE_FLOW explicitly, none or rts-cts}"
device="${MCPROTOCOL_DEVICE}"
baud="${MCPROTOCOL_BAUD}"
data_bits="${MCPROTOCOL_DATA_BITS}"
stop_bits="${MCPROTOCOL_STOP_BITS}"
parity="${MCPROTOCOL_PARITY}"
hardware_flow="${MCPROTOCOL_HARDWARE_FLOW}"
: "${MCPROTOCOL_FRAME:?set MCPROTOCOL_FRAME explicitly, e.g. c4-ascii-f4}"
: "${MCPROTOCOL_PLC_PROFILE:?set MCPROTOCOL_PLC_PROFILE explicitly, e.g. melsec:qcpu}"
: "${MCPROTOCOL_SUM_CHECK:?set MCPROTOCOL_SUM_CHECK explicitly, on or off}"
: "${MCPROTOCOL_ROUTE:?set MCPROTOCOL_ROUTE explicitly, host or multidrop}"
frame="${MCPROTOCOL_FRAME}"
plc_profile="${MCPROTOCOL_PLC_PROFILE}"
sum_check="${MCPROTOCOL_SUM_CHECK}"
route="${MCPROTOCOL_ROUTE}"
station="${MCPROTOCOL_STATION:-}"
network="${MCPROTOCOL_NETWORK:-}"
pc_target="${MCPROTOCOL_PC_TARGET:-}"
module_target="${MCPROTOCOL_MODULE_TARGET:-}"
topology="${MCPROTOCOL_TOPOLOGY:-}"
self_station="${MCPROTOCOL_SELF_STATION:-}"
if [[ "${route}" == "multidrop" && -z "${station}" ]]; then
  echo "MCPROTOCOL_STATION is required for a multidrop route" >&2
  exit 2
fi
if [[ "${route}" == "multidrop" && ( "${frame}" == c3-* || "${frame}" == c4-* ) && -z "${network}" ]]; then
  echo "MCPROTOCOL_NETWORK is required for a 3C/4C multidrop route" >&2
  exit 2
fi
if [[ "${route}" == "multidrop" && ( "${frame}" == c3-* || "${frame}" == c4-* ) && -z "${pc_target}" ]]; then
  echo "MCPROTOCOL_PC_TARGET is required for a 3C/4C non-host route" >&2
  exit 2
fi
if [[ "${route}" == "multidrop" && "${frame}" == c4-* && -z "${module_target}" ]]; then
  echo "MCPROTOCOL_MODULE_TARGET is required for a 4C multidrop route" >&2
  exit 2
fi
if [[ "${route}" == "multidrop" && "${frame}" == c[234]-* ]]; then
  if [[ "${topology}" != "standard" && "${topology}" != "mn" ]]; then
    echo "MCPROTOCOL_TOPOLOGY is required for 2C/3C/4C multidrop (standard or mn)" >&2
    exit 2
  fi
  if [[ "${topology}" == "standard" && -n "${self_station}" ]]; then
    echo "MCPROTOCOL_SELF_STATION is invalid for standard topology" >&2
    exit 2
  fi
  if [[ "${topology}" == "mn" && -z "${self_station}" ]]; then
    echo "MCPROTOCOL_SELF_STATION is required for mn topology" >&2
    exit 2
  fi
elif [[ -n "${topology}" || -n "${self_station}" ]]; then
  echo "MCPROTOCOL_TOPOLOGY and MCPROTOCOL_SELF_STATION are invalid for this route/frame" >&2
  exit 2
fi
rts_toggle="${MCPROTOCOL_RTS_TOGGLE:-off}"
duration_sec="${MCPROTOCOL_DURATION_SEC:-180}"
response_timeout_ms="${MCPROTOCOL_RESPONSE_TIMEOUT_MS:-}"
inter_byte_timeout_ms="${MCPROTOCOL_INTER_BYTE_TIMEOUT_MS:-}"

targets_override="${MCPROTOCOL_TARGETS:-}"

if [[ -n "${targets_override}" ]]; then
  read -r -a targets <<<"${targets_override}"
else
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
fi

common_args=(
  --device "${device}"
  --baud "${baud}"
  --data-bits "${data_bits}"
  --stop-bits "${stop_bits}"
  --parity "${parity}"
  --hardware-flow "${hardware_flow}"
  --frame "${frame}"
  --plc-profile "${plc_profile}"
  --sum-check "${sum_check}"
  --route "${route}"
  --rts-toggle "${rts_toggle}"
)
if [[ -n "${station}" ]]; then
  common_args+=(--station "${station}")
fi
if [[ -n "${network}" ]]; then
  common_args+=(--network "${network}")
fi
if [[ -n "${pc_target}" ]]; then
  common_args+=(--pc-target "${pc_target}")
fi
if [[ -n "${module_target}" ]]; then
  common_args+=(--module-target "${module_target}")
fi
if [[ -n "${topology}" ]]; then
  common_args+=(--topology "${topology}")
fi
if [[ -n "${self_station}" ]]; then
  common_args+=(--self-station "${self_station}")
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
