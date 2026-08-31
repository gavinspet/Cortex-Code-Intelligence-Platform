#!/usr/bin/env bash
set -euo pipefail

API_URL="${API_URL:-http://127.0.0.1:8080}"
COUNT="${1:-1}"
MODE="${2:-serial}"
OUT_FILE="${3:-/tmp/measure_publish_latency.out}"

: > "$OUT_FILE"

submit_once() {
  local idx="$1"
  local url="${2:-https://github.com/octocat/Hello-World}"
  local body="{\"repositoryUrl\":\"${url}\"}"

  local result
  result=$(curl -sS -o /tmp/measure_resp_${idx}.json \
    -w "HTTP=%{http_code} TOTAL=%{time_total}" \
    -X POST "$API_URL/repositories" \
    -H "Content-Type: application/json" \
    -d "$body") || {
      echo "submit_${idx} curl_error" >> "$OUT_FILE"
      return 1
    }

  local http_code total_sec job_id
  http_code=$(echo "$result" | sed -n 's/.*HTTP=\([0-9]*\).*/\1/p')
  total_sec=$(echo "$result" | sed -n 's/.*TOTAL=\([0-9.]*\).*/\1/p')
  job_id=$(grep -o '"jobId":"[^"]*"' /tmp/measure_resp_${idx}.json | head -n1 | cut -d'"' -f4 || true)

  echo "submit_${idx} http=${http_code} total_sec=${total_sec} job_id=${job_id}" >> "$OUT_FILE"
}

if [[ "$MODE" == "concurrent" ]]; then
  pids=()
  for i in $(seq 1 "$COUNT"); do
    submit_once "$i" &
    pids+=("$!")
  done
  for p in "${pids[@]}"; do
    wait "$p" || true
  done
else
  for i in $(seq 1 "$COUNT"); do
    submit_once "$i"
  done
fi

echo "--- submit summary ---" >> "$OUT_FILE"
sed -n '1,200p' "$OUT_FILE"
