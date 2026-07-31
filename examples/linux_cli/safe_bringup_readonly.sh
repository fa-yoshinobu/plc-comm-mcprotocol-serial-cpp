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
# Frame and profile are intentionally required. See docsrc/user/GOTCHAS.md.
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
e1_monitoring_timer_ms="${MCPROTOCOL_E1_MONITORING_TIMER_MS:-}"
if [[ "${route}" == "multidrop" && "${frame}" != e1-* && -z "${station}" ]]; then
  echo "MCPROTOCOL_STATION is required for a multidrop route" >&2
  exit 2
fi
if [[ "${route}" == "multidrop" && ( "${frame}" == c3-* || "${frame}" == c4-* ) && -z "${network}" ]]; then
  echo "MCPROTOCOL_NETWORK is required for a 3C/4C multidrop route" >&2
  exit 2
fi
if [[ "${route}" == "multidrop" && ( "${frame}" == c3-* || "${frame}" == c4-* || "${frame}" == e1-* ) && -z "${pc_target}" ]]; then
  echo "MCPROTOCOL_PC_TARGET is required for a 3C/4C/1E non-host route" >&2
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
if [[ "${frame}" != e1-* && -n "${e1_monitoring_timer_ms}" ]]; then
  echo "MCPROTOCOL_E1_MONITORING_TIMER_MS is valid only for an E1 frame" >&2
  exit 2
fi
head_device="${1:-D100}"
points="${2:-2}"

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
if [[ -n "${e1_monitoring_timer_ms}" ]]; then
  common_args+=(--e1-monitoring-timer-ms "${e1_monitoring_timer_ms}")
fi
# Start with read-only commands before any write-oriented validation.
"${cli}" "${common_args[@]}" cpu-model
"${cli}" "${common_args[@]}" loopback ABCDE
"${cli}" "${common_args[@]}" read-words "${head_device}" "${points}"
