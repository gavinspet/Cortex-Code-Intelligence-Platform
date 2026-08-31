#!/usr/bin/env bash
set -euo pipefail

API_BASE="http://127.0.0.1:8080"
JOB_IDS_FILE="/tmp/cortex_job_ids.txt"

urls=(
  "https://github.com/octocat/Hello-World"
  "https://github.com/octocat/Spoon-Knife"
  "https://github.com/githubtraining/hellogitworld"
  "https://github.com/github/gitignore"
  "https://github.com/docker-library/hello-world"
)

rm -f "$JOB_IDS_FILE"

submit_one() {
  local url="$1"
  local response
  local id

  response=$(curl -sS -X POST "$API_BASE/repositories" \
    -H "Content-Type: application/json" \
    -d "{\"repositoryUrl\":\"${url}\"}")

  id=$(echo "$response" | sed -n 's/.*"jobId":"\([a-f0-9-]\{36\}\)".*/\1/p')
  if [[ -z "$id" ]]; then
    echo "Failed to parse jobId from response: $response" >&2
    exit 1
  fi

  echo "$id"
}

export -f submit_one
export API_BASE

printf '%s\n' "${urls[@]}" | xargs -I{} -P 5 bash -lc 'submit_one "$@"' _ {} > "$JOB_IDS_FILE"

echo "Submitted job IDs:"
cat "$JOB_IDS_FILE"

poll_job() {
  local job_id="$1"
  local status=""

  for _ in $(seq 1 120); do
    status=$(curl -sS "$API_BASE/jobs/${job_id}" | sed -n 's/.*"status":"\([A-Z]*\)".*/\1/p')
    if [[ "$status" == "COMPLETED" ]]; then
      echo "${job_id}:COMPLETED"
      return 0
    fi
    if [[ "$status" == "FAILED" ]]; then
      echo "${job_id}:FAILED"
      return 1
    fi
    sleep 1
  done

  echo "${job_id}:TIMEOUT"
  return 1
}

while IFS= read -r job_id; do
  poll_job "$job_id"
done < "$JOB_IDS_FILE"

echo "Checking analysis endpoint for all completed jobs..."
while IFS= read -r job_id; do
  code=$(curl -sS -o /tmp/analysis_${job_id}.json -w "%{http_code}" "$API_BASE/analysis/${job_id}")
  if [[ "$code" != "200" ]]; then
    echo "Analysis fetch failed for ${job_id} with HTTP ${code}" >&2
    exit 1
  fi
  echo "${job_id}:analysis_ok"
done < "$JOB_IDS_FILE"

echo "All concurrent jobs completed with analysis available."
