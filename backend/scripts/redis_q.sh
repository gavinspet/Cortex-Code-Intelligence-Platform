#!/usr/bin/env bash
set -euo pipefail
docker exec cortex-redis-test redis-cli "$@"
