#!/usr/bin/env bash
set -euo pipefail
SQL="${1:-}"
if [[ -z "$SQL" ]]; then
  echo "usage: $0 \"SQL\""
  exit 1
fi
docker exec cortex-mysql-test mysql -N -B -ucortex -pcortex -D cortex -e "$SQL"
