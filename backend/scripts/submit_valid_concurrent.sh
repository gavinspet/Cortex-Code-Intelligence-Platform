#!/usr/bin/env bash
set -euo pipefail

COUNT="${1:-5}"
OUT="${2:-/tmp/valid_jobs.tsv}"
API="http://127.0.0.1:8080/repositories"
URL="https://github.com/octocat/Hello-World"

: > "$OUT"

post_one() {
  local i="$1"
  local code
  code=$(curl -sS -o /tmp/valid_job_${i}.json -w "%{http_code}" -X POST "$API" \
    -H "Content-Type: application/json" \
    -d "{\"repositoryUrl\":\"${URL}\"}") || {
      echo -e "${i}\t000\t" >> "$OUT"
      return
    }
  local id
  id=$(grep -o '"jobId":"[^"]*"' /tmp/valid_job_${i}.json | head -n1 | cut -d'"' -f4 || true)
  echo -e "${i}\t${code}\t${id}" >> "$OUT"
}

for i in $(seq 1 "$COUNT"); do
  post_one "$i" &
done
wait

echo "out_file=$OUT"
cat "$OUT"
