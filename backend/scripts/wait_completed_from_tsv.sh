#!/usr/bin/env bash
set -euo pipefail

IN_FILE="${1:-}"
if [[ -z "$IN_FILE" || ! -f "$IN_FILE" ]]; then
  echo "usage: $0 <tsv_file>"
  exit 1
fi

mapfile -t ids < <(awk -F'\t' '$2=="202" && $3!="" {print $3}' "$IN_FILE")
if [[ ${#ids[@]} -eq 0 ]]; then
  echo "no_accepted_ids"
  exit 1
fi

for _ in $(seq 1 160); do
  all_done=1
  for id in "${ids[@]}"; do
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

echo "ids=${ids[*]}"
/home/kartick-wsl/projects/Cortex-Code-Intelligence-Platform/backend/scripts/mysql_jobs_status.sh "${ids[@]}"
