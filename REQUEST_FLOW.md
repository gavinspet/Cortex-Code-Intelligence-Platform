# Backend Working Flow (SL -> BL -> SL)

This note explains exactly how a request travels from the frontend (Service Layer / UI) to backend business logic (BL), how processing happens, and how data returns to the frontend.

## 1) High-Level Architecture

- Frontend (React) sends HTTP requests.
- Backend (Drogon + C++) exposes REST endpoints.
- Controller layer validates HTTP and request body.
- Service layer applies business logic.
- Repository layer stores/retrieves jobs.
- Background worker performs async repository analysis.
- Analysis endpoint returns final aggregated result JSON.

## 2) Frontend Entry (SL)

In the frontend:
- User enters repository URL and clicks Analyze.
- Frontend sends:
  - POST /repositories with JSON body { "repositoryUrl": "..." }
- Then frontend polls:
  - GET /jobs/{jobId} every 2 seconds
- When status is COMPLETED, frontend fetches:
  - GET /analysis/{jobId}

Reference code:
- frontend/src/App.jsx

## 3) Backend Route Registration

Routes are registered in backend/src/app/Application.cpp:
- POST /repositories -> repository handler
- GET /jobs/{jobId} -> job handler
- GET /analysis/{jobId} -> analysis handler
- GET /health -> health handler

CORS headers are added to support frontend calls.

## 4) Request 1: Submit Repository (POST /repositories)

Flow:
1. Application route dispatches to RepositoryController::submitRepository.
2. Controller validates:
   - HTTP method is POST
   - body is valid JSON
   - repositoryUrl exists
3. Controller calls RepositoryService::submitRepository.
4. Service validates URL using UrlValidator.
5. Service creates a new Job with:
   - UUID jobId
   - status QUEUED
   - createdAt timestamp
6. Service saves job via IJobRepository (MySQL if available, otherwise InMemory).
7. Service notifies WorkerService that a new job is available.
8. Controller returns 202 Accepted with:
   - success: true
   - data.jobId
   - data.status = QUEUED

Important files:
- backend/src/api/repositories/RepositoryController.cpp
- backend/src/api/repositories/RepositoryService.cpp
- backend/include/api/repositories/RepositoryResponse.h

## 5) BL Async Processing: JobWorker

The worker runs in background thread (started in Application::run).

Flow in JobWorker:
1. Wait for queued jobs (condition variable + dequeueNextJob).
2. Pick next QUEUED job.
3. Update status -> RUNNING, set startedAt.
4. Clone repo to /tmp/cortex-workspace/{jobId}.
5. Perform static scan:
   - fileCount
   - dirCount
   - totalLines
   - languageDistribution by extension
6. Save analysis result to analysis repository.
7. Enrich data:
   - GitHub metadata
   - Technology detection
   - Repository health scoring
   - Repository insights generation
8. Update status -> COMPLETED, set completedAt.
9. On error/clone failure, status -> FAILED.

Important files:
- backend/src/worker/JobWorker.cpp
- backend/include/worker/JobWorker.h
- backend/include/domain/Job.h

## 6) Request 2: Poll Job Status (GET /jobs/{jobId})

Flow:
1. Application route dispatches to JobController::getJob.
2. Controller validates method is GET.
3. Controller calls JobService::getJob(jobId).
4. Service fetches from IJobRepository.
5. Response:
   - 200 with job status (QUEUED/RUNNING/COMPLETED/FAILED), timestamps
   - or 404 if job not found

Important files:
- backend/src/api/jobs/JobController.cpp
- backend/src/api/jobs/JobService.cpp
- backend/include/api/jobs/JobResponse.h

## 7) Request 3: Fetch Analysis (GET /analysis/{jobId})

Flow:
1. Application route dispatches to AnalysisController::getAnalysis.
2. Controller calls AnalysisService::getAnalysis(jobId).
3. Service reads analysis from analysis repository.
4. Controller builds final response JSON:
   - Core analysis (counts, language distribution, analyzedAt)
   - metadata (if available)
   - technologyAnalysis (if available)
   - repositoryHealth (if available)
   - repositoryInsights (if available)
5. Returns 200 success with data, or 404 if analysis is missing.

Important files:
- backend/src/analysis/AnalysisController.cpp
- backend/include/analysis/AnalysisService.h

## 8) Response Back to Frontend (SL)

Frontend behavior in App.jsx:
- Receives 202 from POST /repositories, stores jobId.
- Polls GET /jobs/{jobId} until status is COMPLETED or FAILED.
- On COMPLETED, calls GET /analysis/{jobId}.
- On success, stores result in state and renders dashboard cards.
- On FAILED/error, sets error status message in UI.

## 9) End-to-End Sequence

1. User submits GitHub URL in React UI.
2. React -> POST /repositories.
3. Backend validates + enqueues job + returns jobId.
4. React polls GET /jobs/{jobId}.
5. Worker processes repository asynchronously.
6. Job status transitions QUEUED -> RUNNING -> COMPLETED (or FAILED).
7. React detects COMPLETED.
8. React -> GET /analysis/{jobId}.
9. Backend returns aggregated analysis JSON.
10. React renders the full analysis dashboard.

## 10) One-Line Mental Model

Frontend is non-blocking and asynchronous: it submits once, polls status, then fetches final result; backend does heavy work in JobWorker and exposes progress/result via job and analysis endpoints.

## 11) Flow Diagram

React
  │
  │ POST /repositories
  ▼
RepositoryController
  │
  ▼
RepositoryService
  │
  ▼
IJobRepository
  │
  ▼
WorkerService
  │
  ▼
JobWorker
  │
  ├── Git Clone
  ├── Filesystem Scan
  ├── Metadata
  ├── Technology
  ├── Health
  └── Insights
  │
  ▼
AnalysisRepository

React
  │
  │ GET /analysis/{jobId}
  ▼
AnalysisController
  │
  ▼
AnalysisService
  │
  ▼
Repositories
  │
  ▼
JSON Response
