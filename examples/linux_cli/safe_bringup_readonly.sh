#!/usr/bin/env bash
set -euo pipefail

cli="${MCPROTOCOL_CLI:-./build/mcprotocol_cli}"
device="${MCPROTOCOL_DEVICE:-/dev/ttyUSB0}"
baud="${MCPROTOCOL_BAUD:-19200}"
data_bits="${MCPROTOCOL_DATA_BITS:-8}"
stop_bits="${MCPROTOCOL_STOP_BITS:-1}"
parity="${MCPROTOCOL_PARITY:-E}"
: "${MCPROTOCOL_FRAME:?set MCPROTOCOL_FRAME explicitly, e.g. c4-ascii-f4}"
: "${MCPROTOCOL_PLC_PROFILE:?set MCPROTOCOL_PLC_PROFILE explicitly, e.g. melsec:q-l}"
frame="${MCPROTOCOL_FRAME}"
plc_profile="${MCPROTOCOL_PLC_PROFILE}"
sum_check="${MCPROTOCOL_SUM_CHECK:-off}"
station="${MCPROTOCOL_STATION:-0}"
head_device="${1:-D100}"
points="${2:-2}"

common_args=(
  --device "${device}"
  --baud "${baud}"
  --data-bits "${data_bits}"
  --stop-bits "${stop_bits}"
  --parity "${parity}"
  --frame "${frame}"
  --plc-profile "${plc_profile}"
  --sum-check "${sum_check}"
  --station "${station}"
)

"${cli}" "${common_args[@]}" cpu-model
"${cli}" "${common_args[@]}" loopback ABCDE
"${cli}" "${common_args[@]}" read-words "${head_device}" "${points}"
