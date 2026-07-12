#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"

: "${MCPROTOCOL_DEVICE:?set MCPROTOCOL_DEVICE explicitly, e.g. /dev/ttyUSB0}"
: "${MCPROTOCOL_BAUD:?set MCPROTOCOL_BAUD explicitly, e.g. 19200}"
: "${MCPROTOCOL_DATA_BITS:?set MCPROTOCOL_DATA_BITS explicitly, 7 or 8}"
: "${MCPROTOCOL_STOP_BITS:?set MCPROTOCOL_STOP_BITS explicitly, 1 or 2}"
: "${MCPROTOCOL_PARITY:?set MCPROTOCOL_PARITY explicitly, N, E, or O}"
: "${MCPROTOCOL_HARDWARE_FLOW:?set MCPROTOCOL_HARDWARE_FLOW explicitly, none or rts-cts}"
export MCPROTOCOL_DEVICE MCPROTOCOL_BAUD MCPROTOCOL_DATA_BITS MCPROTOCOL_STOP_BITS
export MCPROTOCOL_PARITY MCPROTOCOL_HARDWARE_FLOW
: "${MCPROTOCOL_FRAME:?set MCPROTOCOL_FRAME explicitly, e.g. c4-binary}"
export MCPROTOCOL_FRAME
: "${MCPROTOCOL_SUM_CHECK:?set MCPROTOCOL_SUM_CHECK explicitly, on or off}"
export MCPROTOCOL_SUM_CHECK
: "${MCPROTOCOL_ROUTE:?set MCPROTOCOL_ROUTE explicitly, host or multidrop}"
export MCPROTOCOL_ROUTE
if [[ -n "${MCPROTOCOL_STATION:-}" ]]; then
  export MCPROTOCOL_STATION
fi
if [[ -n "${MCPROTOCOL_NETWORK:-}" ]]; then
  export MCPROTOCOL_NETWORK
fi
if [[ -n "${MCPROTOCOL_PC_TARGET:-}" ]]; then
  export MCPROTOCOL_PC_TARGET
fi
if [[ -n "${MCPROTOCOL_MODULE_TARGET:-}" ]]; then
  export MCPROTOCOL_MODULE_TARGET
fi
if [[ -n "${MCPROTOCOL_TOPOLOGY:-}" ]]; then
  export MCPROTOCOL_TOPOLOGY
fi
if [[ -n "${MCPROTOCOL_SELF_STATION:-}" ]]; then
  export MCPROTOCOL_SELF_STATION
fi
if [[ -n "${MCPROTOCOL_E1_MONITORING_TIMER_MS:-}" ]]; then
  export MCPROTOCOL_E1_MONITORING_TIMER_MS
fi
: "${MCPROTOCOL_PLC_PROFILE:?set MCPROTOCOL_PLC_PROFILE explicitly, e.g. melsec:iq-f}"
export MCPROTOCOL_PLC_PROFILE
if [[ -n "${MCPROTOCOL_RESPONSE_TIMEOUT_MS:-}" ]]; then
  export MCPROTOCOL_RESPONSE_TIMEOUT_MS
fi
if [[ -n "${MCPROTOCOL_INTER_BYTE_TIMEOUT_MS:-}" ]]; then
  export MCPROTOCOL_INTER_BYTE_TIMEOUT_MS
fi
export MCPROTOCOL_TARGETS="${MCPROTOCOL_TARGETS:-bit:STS10 bit:STC10 word:STN10 bit:TS10 bit:TC10 word:TN10 bit:CS10 bit:CC10 word:CN10 bit:SB10 word:SW10 bit:X10 bit:Y10 bit:M100 bit:L100 bit:F100 bit:B10 word:D100 word:W10 word:Z10 word:R100}"

exec "${script_dir}/supported_device_rw_soak.sh"
