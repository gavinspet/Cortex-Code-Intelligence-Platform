# Project Structure

```
Cortex-Code-Intelligence-Platform/
├── backend/
│   ├── CMakeLists.txt
│   ├── build.sh
│   ├── config/
│   │   └── config.json
│   ├── database/
│   │   └── migrations/
│   │       └── 001_create_jobs.sql
│   ├── include/
│   │   ├── analysis/
│   │   ├── api/
│   │   ├── app/
│   │   ├── config/
│   │   ├── core/
│   │   ├── database/
│   │   ├── domain/
│   │   ├── infrastructure/
│   │   ├── logging/
│   │   ├── utils/
│   │   └── worker/
│   ├── logs/
│   └── src/
│       ├── analysis/
│       ├── api/
│       ├── app/
│       ├── config/
│       ├── core/
│       ├── database/
│       ├── infrastructure/
│       ├── logging/
│       ├── main.cpp
│       └── worker/
├── docs/
├── frontend/
│   ├── index.html
│   ├── package.json
│   ├── src/
│   │   ├── App.css
│   │   ├── App.jsx
│   │   └── main.jsx
│   └── vite.config.js
├── docker-compose.yml
├── stop.sh
└── LICENSE
```

---

## Root Level

| File / Folder | Purpose |
|---|---|
| `backend/` | C++20 Drogon backend — all server-side code |
| `frontend/` | React + Vite single-page application |
| `docs/` | Project documentation |
| `docker-compose.yml` | MySQL 8 development environment |
| `stop.sh` | Convenience script to kill backend and frontend processes |
| `LICENSE` | MIT license |

---

## `backend/`

### `CMakeLists.txt`

CMake build definition. All source files are explicitly listed (no `GLOB_RECURSE`). Links: `drogon trantor jsoncpp spdlog fmt mysqlcppconn`.

### `config/config.json`

Runtime configuration. Loaded at startup by `FileConfiguration`. Controls server bind address, port, thread count, and logging.

### `database/migrations/`

SQL migration files. Applied manually or automatically via Docker Compose `docker-entrypoint-initdb.d`.

### `include/`

All header files. The `include/` tree mirrors `src/` exactly. Headers define the **interface**; implementations are in `src/`.

#### `include/analysis/`

| File | Purpose |
|---|---|
| `IAnalysisRepository.h` | Abstract interface: `save()`, `findByJobId()` |
| `InMemoryAnalysisRepository.h` | Thread-safe in-memory implementation (header-only) |
| `AnalysisService.h` | Service: delegates to repository |
| `AnalysisController.h` | HTTP handler for `GET /analysis/{jobId}` |

#### `include/api/health/`

| File | Purpose |
|---|---|
| `HealthService.h` | Returns uptime and service metadata |
| `HealthController.h` | HTTP handler for `GET /health` |
| `HealthResponse.h` | Builds the JSON health response |

#### `include/api/jobs/`

| File | Purpose |
|---|---|
| `JobService.h` | Looks up a job by ID |
| `JobController.h` | HTTP handler for `GET /jobs/{jobId}` |
| `JobResponse.h` | Serializes `Job` to JSON with ISO 8601 timestamps |

#### `include/api/repositories/`

| File | Purpose |
|---|---|
| `RepositoryService.h` | Validates URL, creates job, notifies worker |
| `RepositoryController.h` | HTTP handler for `POST /repositories` |
| `RepositoryRequest.h` | Deserializes POST body |
| `RepositoryResponse.h` | Serializes accepted job to JSON |

#### `include/app/`

| File | Purpose |
|---|---|
| `Application.h` | Main orchestrator — DI wiring, Drogon init, route registration, lifecycle |
| `ApplicationFactory.h` | Creates `Application` from config file path |

#### `include/config/`

| File | Purpose |
|---|---|
| `Configuration.h` | Abstract interface (`getString`, `getInt`, `getUInt`) |
| `FileConfiguration.h` | Reads `config.json` via jsoncpp |
| `ConfigurationValidator.h` | Validates required keys at startup |

#### `include/core/di/`

| File | Purpose |
|---|---|
| `ServiceContainer.h` | Type-erased service registry (used internally by `Application`) |

#### `include/database/`

| File | Purpose |
|---|---|
| `Database.h` | Meyers singleton — `initialize()`, `getConnection()`, `isHealthy()`, `shutdown()` |

#### `include/domain/`

| File | Purpose |
|---|---|
| `Job.h` | Value object: id, url, status, timestamps. `JobStatus` enum. `jobStatusToString()` |
| `IJobRepository.h` | Abstract storage interface for jobs |
| `AnalysisResult.h` | Struct: fileCount, dirCount, totalLines, languageDistribution, analyzedAt |

#### `include/infrastructure/`

| File | Purpose |
|---|---|
| `InMemoryJobRepository.h` | `std::unordered_map` + `std::mutex`; header-only |
| `MySQLJobRepository.h` | MySQL implementation using prepared statements |

#### `include/logging/`

| File | Purpose |
|---|---|
| `Logger.h` | Singleton facade: `info()`, `warn()`, `error()`, `critical()`. Macro definitions. |
| `SpdlogLogger.h` | spdlog backend with dual-sink (console + rotating file) |
| `LoggerFactory.h` | Factory for creating the logger from configuration |

#### `include/utils/`

| File | Purpose |
|---|---|
| `UrlValidator.h` | `isValidRepositoryUrl()` — validates HTTPS GitHub/GitLab URLs |
| `UuidGenerator.h` | `generate()` — returns a UUID v4 string |
| `Result.h` | Generic `Result<T, E>` type (reserved for future use) |

#### `include/worker/`

| File | Purpose |
|---|---|
| `JobWorker.h` | Background thread: `start()`, `stop()`, `notifyJobAvailable()`, `workerLoop()`, `analyzeRepository()` |
| `WorkerService.h` | Thin lifecycle wrapper around `JobWorker` |

---

## `frontend/`

| File | Purpose |
|---|---|
| `src/App.jsx` | Entire application: URL input, submit, polling loop, results display |
| `src/App.css` | Dark-theme minimal styles |
| `src/main.jsx` | React root mount |
| `index.html` | HTML entry point |
| `vite.config.js` | Dev server config + proxy rules to backend `:8080` |
| `package.json` | Dependencies: `react`, `react-dom`, `vite`, `@vitejs/plugin-react` |

---

## `docs/`

| File | Purpose |
|---|---|
| `API.md` | Full endpoint reference with curl examples |
| `ARCHITECTURE.md` | Layer design, DI, worker, database deep-dive |
| `ENGINEERING_DECISIONS.md` | Why each technology and pattern was chosen |
| `FUTURE_ROADMAP.md` | Planned features by version |
| `INTERVIEW_TALKING_POINTS.md` | Interview preparation guide |
| `PROJECT_STRUCTURE.md` | This file |
| `SETUP.md` | Prerequisites, installation, troubleshooting |
