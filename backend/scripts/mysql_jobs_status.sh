#!/usr/bin/env bash
set -euo pipefail
if [[ $# -lt 1 ]]; then
  echo "usage: $0 <job_id> [job_id...]"
  exit 1
fi
in=""
for id in "$@"; do
  if [[ -n "$in" ]]; then
    in+="','"
  fi
  in+="$id"
done
SQL="SELECT id,status,created_at,started_at,completed_at,error_message FROM jobs WHERE id IN ('$in') ORDER BY created_at;"
docker exec cortex-mysql-test mysql -N -B -ucortex -pcortex -D cortex -e "$SQL"
