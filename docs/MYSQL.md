# MySQL Persistence (Phase 2 Step 1)

This document describes the MySQL persistence layer used by the Cortex backend, including architecture, schema, configuration, concurrency behavior, and validation steps.

## Scope

Implemented in this phase:
- Persistent storage for jobs using MySQL
- Persistent storage for analysis results using MySQL
- Runtime selection between in-memory and MySQL repositories
- Connection management hardened for multithreaded runtime behavior

Out of scope for this phase:
- Rewriting service/controller business logic
- Full database migration framework beyond bootstrap SQL files

## Architecture

The application keeps the existing abstraction boundary:
- Controller -> Service -> Repository interface -> Repository implementation

MySQL implementations:
- Job repository: [backend/src/infrastructure/MySQLJobRepository.cpp](backend/src/infrastructure/MySQLJobRepository.cpp)
- Analysis repository: [backend/src/infrastructure/MySQLAnalysisRepository.cpp](backend/src/infrastructure/MySQLAnalysisRepository.cpp)

Repository interfaces remain unchanged:
- Job interface: [backend/include/domain/IJobRepository.h](backend/include/domain/IJobRepository.h)
- Analysis interface: [backend/include/analysis/IAnalysisRepository.h](backend/include/analysis/IAnalysisRepository.h)

Runtime wiring is done in:
- [backend/src/app/Application.cpp](backend/src/app/Application.cpp)

Database access is centralized in:
- [backend/src/database/Database.cpp](backend/src/database/Database.cpp)
- [backend/include/database/Database.h](backend/include/database/Database.h)

## Schema

Bootstrap migrations are mounted into MySQL init directory from:
- [backend/migrations/001_create_jobs.sql](backend/migrations/001_create_jobs.sql)
- [backend/migrations/002_create_analysis_results.sql](backend/migrations/002_create_analysis_results.sql)

Tables:
- jobs
- analysis_results

`analysis_results` stores:
- job_id (unique key)
- clone_path
- file_count
- dir_count
- total_lines
- language_distribution_json
- analyzed_at

## Configuration

Backend selection:
- `storage.backend` in [backend/config/config.json](backend/config/config.json)
- `STORAGE_BACKEND` environment variable overrides config

MySQL connection environment variables:
- `MYSQL_HOST`
- `MYSQL_PORT`
- `MYSQL_DATABASE`
- `MYSQL_USER`
- `MYSQL_PASSWORD`

Connection retry controls:
- `MYSQL_CONNECT_RETRIES` (default 5)
- `MYSQL_CONNECT_RETRY_DELAY_MS` (default 2000)

## Prepared Statements and Safety

Repository implementations use prepared statements for all writes and parameterized reads.

Benefits:
- Prevent SQL injection from request-derived data
- Keep SQL operations explicit and repository-local
- Preserve clean separation from service and controller layers

## Concurrency and Root Cause Fix

### Observed issue

Worker logs repeatedly showed:
- `Commands out of sync; you can't run this command now`

### Root cause

The database checkout path performed connection-validity probing on every repository call. In this connector/runtime combination, that probe could desynchronize protocol state on the same thread-local MySQL session, causing the next query (commonly dequeue) to fail with `Commands out of sync`.

### Final fix

In [backend/src/database/Database.cpp](backend/src/database/Database.cpp):
- Keep one connection per thread using `thread_local` storage.
- Make `getConnection()` side-effect free in hot path:
  - create new connection if missing
  - reconnect only if closed or if closed-check throws
  - do not run SQL probe queries during checkout
- Keep startup connect test in `initialize()`.

In [backend/src/worker/JobWorker.cpp](backend/src/worker/JobWorker.cpp):
- Removed dequeue call from wait predicate.
- Dequeue is now executed once per wake cycle, avoiding duplicate side-effecting DB operations.

Why this is thread-safe:
- Each thread owns an independent MySQL connection instance.
- No connection object is shared concurrently between worker and HTTP threads.
- Repository operations are serialized per thread on that thread-local connection.

## Transactions

Current operations are mostly single-statement updates/inserts and rely on default autocommit behavior.

For future multi-step state transitions requiring atomic guarantees, introduce explicit transaction boundaries in repository methods.

## Local Startup (MySQL)

1. Start MySQL container and mount migrations:
- mount [backend/migrations](backend/migrations) to `/docker-entrypoint-initdb.d`

2. Start backend with MySQL backend enabled:
- `STORAGE_BACKEND=mysql`
- set `MYSQL_*` env vars

3. Verify service:
- `GET /health`
- `POST /repositories`
- `GET /jobs/{jobId}`
- `GET /analysis/{jobId}`

## Persistence Verification Checklist

1. Submit concurrent repository jobs (for example 5 in parallel).
2. Poll until all jobs reach `COMPLETED`.
3. Confirm `GET /analysis/{jobId}` returns HTTP 200 for each job.
4. Confirm worker log contains no `Commands out of sync`.
5. Restart backend process only (do not reset DB volume).
6. Re-query all previously returned job IDs.
7. Confirm statuses remain `COMPLETED` and analysis remains available.

## Known Limitations

- Startup SQL files run only on fresh MySQL datadir initialization.
- No formal migration version table yet.
- Cross-repository transactional orchestration is not implemented in this phase.
