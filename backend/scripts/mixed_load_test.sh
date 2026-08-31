#!/usr/bin/env bash
set -euo pipefail

API="http://127.0.0.1:8080/repositories"
OUT="${1:-/tmp/mixed_load_results.tsv}"

: > "$OUT"

post_job() {
  local kind="$1"
  local url="$2"
  local idx="$3"
  local resp code
  resp=$(curl -sS -o /tmp/mixed_${kind}_${idx}.json -w "%{http_code}" -X POST "$API" -H "Content-Type: application/json" -d "{\"repositoryUrl\":\"${url}\"}") || {
    echo -e "${kind}\t${idx}\t000\t" >> "$OUT"
    return
  }
  code="$resp"
  local job_id
  job_id=$(grep -o '"jobId":"[^"]*"' /tmp/mixed_${kind}_${idx}.json | head -n1 | cut -d'"' -f4 || true)
  echo -e "${kind}\t${idx}\t${code}\t${job_id}" >> "$OUT"
}

# 5 valid + 5 failing submitted concurrently
for i in $(seq 1 5); do
  post_job valid "https://github.com/octocat/Hello-World" "$i" &
  post_job failing "https://github.com/octocat/Hello-World%zz" "$i" &
done
wait

echo "results_file=$OUT"
cat "$OUT"
