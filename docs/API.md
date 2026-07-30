# API Reference

Base URL: `http://localhost:8080`

All responses follow this envelope:

```json
{
  "success": true | false,
  "message": "Human-readable description",
  "data": { ... }
}
```

---

## GET /health

Health check endpoint. Returns service status and uptime.

**Request:** No body required.

**Response — 200 OK**
```json
{
  "success": true,
  "message": "Service is healthy",
  "data": {
    "status": "UP",
    "service": "Cortex",
    "version": "0.1.0",
    "environment": "Development",
    "timestamp": "2026-07-30T12:00:00Z",
    "uptimeSeconds": 42
  }
}
```

**curl**
```bash
curl http://localhost:8080/health
```

---

## POST /repositories

Submit a public GitHub or GitLab repository for analysis. Returns immediately with a job ID. The actual clone and analysis runs asynchronously in a background worker.

**Request body**
```json
{
  "repositoryUrl": "https://github.com/user/repo"
}
```

| Field | Type | Required | Notes |
|---|---|---|---|
| `repositoryUrl` | string | Yes | Must start with `https://github.com` or `https://gitlab.com`. `.git` suffix is optional. |

**Response — 202 Accepted**
```json
{
  "success": true,
  "message": "Repository accepted for analysis.",
  "data": {
    "jobId": "9027534d-22c7-4f68-8145-62e339630ae6",
    "status": "QUEUED"
  }
}
```

**Response — 400 Bad Request** (invalid URL)
```json
{
  "success": false,
  "message": "Invalid repository URL. Must be HTTPS GitHub or GitLab repository."
}
```

**Response — 400 Bad Request** (missing or malformed JSON)
```json
{
  "success": false,
  "message": "Invalid request body. Expected JSON."
}
```

**curl**
```bash
curl -X POST http://localhost:8080/repositories \
  -H 'Content-Type: application/json' \
  -d '{"repositoryUrl": "https://github.com/octocat/Hello-World"}'
```

---

## GET /jobs/{jobId}

Poll the status of an analysis job. Call this every 2 seconds until `status` is `COMPLETED` or `FAILED`.

**Path parameter:** `jobId` — UUID returned by `POST /repositories`

**Response — 200 OK**
```json
{
  "success": true,
  "message": "Job found",
  "data": {
    "jobId": "9027534d-22c7-4f68-8145-62e339630ae6",
    "repositoryUrl": "https://github.com/octocat/Hello-World",
    "status": "COMPLETED",
    "createdAt": "2026-07-30T12:00:00Z",
    "startedAt": "2026-07-30T12:00:01Z",
    "completedAt": "2026-07-30T12:00:15Z"
  }
}
```

**Job status values**

| Status | Meaning |
|---|---|
| `QUEUED` | Job created, waiting for worker |
| `RUNNING` | Worker is cloning and scanning |
| `COMPLETED` | Analysis finished successfully |
| `FAILED` | Clone or scan encountered an error |

`startedAt` and `completedAt` are `null` until the corresponding transition occurs.

**Response — 404 Not Found**
```json
{
  "success": false,
  "message": "Job not found"
}
```

**curl**
```bash
curl http://localhost:8080/jobs/9027534d-22c7-4f68-8145-62e339630ae6
```

---

## GET /analysis/{jobId}

Retrieve the analysis results for a completed job.

**Path parameter:** `jobId` — UUID returned by `POST /repositories`

**Response — 200 OK**
```json
{
  "success": true,
  "data": {
    "jobId": "9027534d-22c7-4f68-8145-62e339630ae6",
    "fileCount": 26,
    "dirCount": 6,
    "totalLines": 66098,
    "languages": {
      ".tsx": 4,
      ".ts": 2,
      ".json": 3,
      ".md": 3,
      ".css": 1,
      ".svg": 5,
      ".jpg": 4,
      ".mjs": 2,
      ".ico": 1,
      "(none)": 1
    },
    "analyzedAt": "2026-07-30T12:00:15Z"
  }
}
```

**Fields**

| Field | Type | Description |
|---|---|---|
| `fileCount` | integer | Total number of regular files (excluding `.git/`) |
| `dirCount` | integer | Total number of directories (excluding `.git/`) |
| `totalLines` | integer | Sum of line counts across all files |
| `languages` | object | Map of file extension → file count. Files with no extension use key `"(none)"` |
| `analyzedAt` | ISO 8601 | When the scan completed |

**Response — 404 Not Found** (job not found or analysis not yet available)
```json
{
  "success": false,
  "message": "Analysis not found for job: 9027534d-..."
}
```

> Call `GET /jobs/{jobId}` first to confirm status is `COMPLETED` before calling this endpoint.

**curl**
```bash
curl http://localhost:8080/analysis/9027534d-22c7-4f68-8145-62e339630ae6
```

---

## Full Workflow Example

```bash
# 1. Submit
JOB=$(curl -s -X POST http://localhost:8080/repositories \
  -H 'Content-Type: application/json' \
  -d '{"repositoryUrl":"https://github.com/octocat/Hello-World"}' \
  | python3 -c 'import sys,json; print(json.load(sys.stdin)["data"]["jobId"])')

echo "Job ID: $JOB"

# 2. Poll until done
while true; do
  STATUS=$(curl -s http://localhost:8080/jobs/$JOB | python3 -c 'import sys,json; print(json.load(sys.stdin)["data"]["status"])')
  echo "Status: $STATUS"
  [ "$STATUS" = "COMPLETED" ] || [ "$STATUS" = "FAILED" ] && break
  sleep 2
done

# 3. Fetch results
curl -s http://localhost:8080/analysis/$JOB | python3 -m json.tool
```
