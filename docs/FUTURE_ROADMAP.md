# Future Roadmap

---

## Version 1.1 — Persistence & Reliability

**Goal:** Make the system production-ready for multi-session use.

- **MySQL always-on** — Docker Compose brings up MySQL automatically; connection is required at startup (no silent in-memory fallback in production mode)
- **Connection pool** — replace single `sql::Connection` with a pool of connections for concurrent workers
- **Migration runner** — auto-apply pending SQL migrations at startup
- **Job retry** — `FAILED` jobs can be requeued via `POST /jobs/{jobId}/retry`
- **Worker crash recovery** — on startup, move `RUNNING` jobs (interrupted by crash) back to `QUEUED`
- **Structured logging** — JSON log output for ingestion by log aggregators (ELK, Loki)

---

## Version 1.2 — Richer Analysis

**Goal:** Make the analysis results genuinely useful to developers.

- **GitHub API metadata** — fetch repository stars, forks, open issues, primary language, and last commit date using the GitHub REST API
- **Per-file analysis** — expose the top 20 largest files by line count
- **Dependency graph** — parse `package.json`, `pom.xml`, `requirements.txt`, `Cargo.toml`, `CMakeLists.txt` to extract declared dependencies
- **Language mapping** — map extensions to language names (`.tsx` → TypeScript, `.rs` → Rust)
- **Binary file exclusion** — skip binary files (images, compiled artifacts) from line counting
- **Clone duration metric** — record and expose how long the clone took

---

## Version 1.3 — Parallel Processing

**Goal:** Handle multiple repositories concurrently.

- **Thread pool worker** — replace single worker thread with configurable pool (default: 4 workers)
- **Job queue depth** — expose queue depth in `GET /health`
- **Concurrency-safe MySQL** — verified thread-safe connection pool with per-thread connections
- **Rate limiting** — limit submissions per IP address to prevent abuse
- **Job expiry** — automatically clean up old completed jobs and clones after 24 hours

---

## Version 2.0 — Intelligence Layer

**Goal:** Add AI-powered insights and a richer frontend.

- **AI code summaries** — send the top-level file tree and README to an LLM API; return a natural-language description of what the repository does
- **Static analysis rules** — detect common patterns: TODO density, average function length, test coverage (if test files exist)
- **Dependency vulnerability scan** — cross-reference extracted dependencies against the OSV database
- **Export reports** — `GET /analysis/{jobId}/export?format=json|csv|pdf`
- **Historical analysis** — analyze the same repository multiple times and show trends over time
- **WebSocket status updates** — replace polling with a WebSocket push for real-time job status
- **Authentication** — API key authentication; user accounts; private repository support via OAuth tokens
- **Frontend dashboard** — job history table, charts for language distribution, comparison between repositories
- **CLI tool** — `cortex analyze https://github.com/user/repo` command-line interface
