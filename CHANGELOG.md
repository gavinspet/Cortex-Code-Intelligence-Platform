# Changelog

All notable changes to Cortex Code Intelligence Platform are documented here.

Format follows [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).  
Versioning follows [Semantic Versioning](https://semver.org/).

---

## [Unreleased]

### Planned
- MySQL connection pool for concurrent job processing
- Thread pool worker for parallel analysis
- GitHub API metadata integration
- WebSocket-based job status push

---

## [1.0.0] — 2026-07-30

### Added

**Backend**
- `GET /health` — service health check with uptime and version metadata
- `POST /repositories` — accepts public GitHub/GitLab repository URLs; returns job ID immediately (202 Accepted)
- `GET /jobs/{jobId}` — returns job lifecycle status (`QUEUED`, `RUNNING`, `COMPLETED`, `FAILED`) with timestamps
- `GET /analysis/{jobId}` — returns code analysis results (file count, directory count, total lines, language distribution)
- Background worker thread — `std::condition_variable`-based producer-consumer; wakes on notification, polls on 1-second timeout
- Real `git clone --depth 1` — clones into `/tmp/cortex-workspace/<jobId>/`
- `std::filesystem::recursive_directory_iterator` scan — counts files, directories, lines, and file extensions
- MySQL persistence via `MySQLJobRepository` with prepared statements
- Automatic fallback to `InMemoryJobRepository` when MySQL is unavailable
- `Database` singleton with health check (`SELECT 1`) and graceful shutdown
- `Logger` singleton — dual-sink spdlog (colorized console + rotating file, 10 MB max, 3 backups)
- Clean Architecture layering — HTTP → Service → Repository → Domain
- Dependency Injection via constructor — full object graph wired in `Application::buildDependencyGraph()`
- `IJobRepository` and `IAnalysisRepository` abstract interfaces enabling storage backend swaps
- URL validation — HTTPS GitHub/GitLab only; `.git` suffix auto-appended if missing
- UUID v4 job identifiers
- Doxygen file headers on all 57 first-party source and header files
- `config/config.json` — configurable host, port, threads, log level

**Frontend**
- React 18 + Vite 6 single-page application
- URL input, Analyze button, polling status display, analysis results with language distribution bar chart
- 2-second polling via `GET /jobs/{jobId}`; auto-fetches `GET /analysis/{jobId}` on completion
- Vite dev proxy — all API calls proxied to backend `:8080`, no CORS configuration required
- Dark-theme minimal UI

**Repository**
- `docker-compose.yml` — MySQL 8 development environment with auto-migration
- `backend/database/migrations/001_create_jobs.sql` — jobs table schema
- `stop.sh` — convenience script to kill backend and frontend processes
- Full documentation suite: `README.md`, `docs/API.md`, `docs/ARCHITECTURE.md`, `docs/SETUP.md`, `docs/ENGINEERING_DECISIONS.md`, `docs/FUTURE_ROADMAP.md`, `docs/INTERVIEW_TALKING_POINTS.md`, `docs/PROJECT_STRUCTURE.md`
- `CONTRIBUTING.md`, `CODE_OF_CONDUCT.md`, `SECURITY.md`, `CHANGELOG.md`
- GitHub issue templates and PR template
- `.gitignore` covering build artifacts, node_modules, logs, and runtime clones
- MIT License
