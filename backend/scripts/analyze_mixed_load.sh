#!/usr/bin/env bash
set -euo pipefail

IN_FILE="${1:-/tmp/mixed_load_results.tsv}"
if [[ ! -f "$IN_FILE" ]]; then
  echo "missing_file=$IN_FILE"
  exit 1
fi

mapfile -t valid_ids < <(awk -F'\t' '$1=="valid" && $3=="202" {print $4}' "$IN_FILE")
mapfile -t failing_ids < <(awk -F'\t' '$1=="failing" && $3=="202" {print $4}' "$IN_FILE")

if [[ ${#valid_ids[@]} -eq 0 ]]; then
  echo "no_valid_ids"
  exit 1
fi

for _ in $(seq 1 80); do
  all_done=1
  for id in "${valid_ids[@]}"; do
    st=$(docker exec cortex-mysql-test mysql -N -B -ucortex -pcortex -D cortex -e "SELECT status FROM jobs WHERE id='${id}';" 2>/dev/null | tr -d '\r')
    if [[ "$st" != "COMPLETED" ]]; then
      all_done=0
      break
    fi
  done
  if [[ $all_done -eq 1 ]]; then
    break
  fi
  sleep 0.5
done

echo "valid_ids=${valid_ids[*]}"
echo "failing_ids=${failing_ids[*]}"

echo "valid_statuses:"
/home/kartick-wsl/projects/Cortex-Code-Intelligence-Platform/backend/scripts/mysql_jobs_status.sh "${valid_ids[@]}"

echo "failing_statuses:"
/home/kartick-wsl/projects/Cortex-Code-Intelligence-Platform/backend/scripts/mysql_jobs_status.sh "${failing_ids[@]}"
