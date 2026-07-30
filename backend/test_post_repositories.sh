#!/bin/bash
curl -X POST http://127.0.0.1:8080/repositories \
  -H 'Content-Type: application/json' \
  -d '{"repositoryUrl":"https://github.com/torvalds/linux.git"}' \
  -v 2>&1
