#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 2 ]]; then
  echo "usage: $0 <job_id_1> <job_id_2>"
  exit 1
fi

RID="$1"
NID="$2"

get_status() {
  local id="$1"
  docker exec cortex-mysql-test mysql -N -B -ucortex -pcortex -D cortex \
    -e "SELECT status FROM jobs WHERE id='${id}';" 2>/dev/null | tr -d '\r'
}

for _ in $(seq 1 60); do
  RS="$(get_status "$RID")"
  NS="$(get_status "$NID")"
  if [[ "$RS" == "FAILED" && "$NS" == "FAILED" ]]; then
    break
  fi
  sleep 0.5
done

echo "retry_status=${RS:-}"
echo "nonretry_status=${NS:-}"
echo "retry_db_row:"
docker exec cortex-mysql-test mysql -N -B -ucortex -pcortex -D cortex \
  -e "SELECT id,status,created_at,started_at,completed_at,error_message FROM jobs WHERE id='${RID}';"
echo "nonretry_db_row:"
docker exec cortex-mysql-test mysql -N -B -ucortex -pcortex -D cortex \
  -e "SELECT id,status,created_at,started_at,completed_at,error_message FROM jobs WHERE id='${NID}';"
