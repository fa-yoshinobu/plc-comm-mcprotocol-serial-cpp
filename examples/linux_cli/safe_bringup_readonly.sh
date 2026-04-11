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
head_device="${1:-D100}"
points="${2:-2}"

common_args=(
  --device "${device}"
  --baud "${baud}"
  --data-bits "${data_bits}"
  --stop-bits "${stop_bits}"
  --parity "${parity}"
  --frame "${frame}"
  --sum-check "${sum_check}"
  --station "${station}"
)

"${cli}" "${common_args[@]}" cpu-model
"${cli}" "${common_args[@]}" loopback ABCDE
"${cli}" "${common_args[@]}" read-words "${head_device}" "${points}"
