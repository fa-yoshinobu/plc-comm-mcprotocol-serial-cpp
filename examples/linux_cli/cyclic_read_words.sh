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
duration_sec="${MCPROTOCOL_DURATION_SEC:-10}"
interval_sec="${MCPROTOCOL_INTERVAL_SEC:-1}"
head_device="${1:-D100}"
points="${2:-4}"

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

end_time=$((SECONDS + duration_sec))
completed_cycles=0

while (( SECONDS < end_time )); do
  "${cli}" "${common_args[@]}" read-words "${head_device}" "${points}"
  completed_cycles=$((completed_cycles + 1))
  sleep "${interval_sec}"
done

echo "completed_cycles=${completed_cycles}"
