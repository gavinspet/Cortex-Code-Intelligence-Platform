# Interview Talking Points

## Project Overview (30-second pitch)

> "Cortex is a backend system written in C++20 that accepts a GitHub URL, clones the repository asynchronously in a background worker thread, scans the codebase with std::filesystem, and exposes the results through a REST API. The frontend polls for status and displays file counts, line counts, and language distribution. The backend follows Clean Architecture with a Repository Pattern — I can swap the storage backend without touching a single line of service code."

---

## Interesting Engineering Decisions

### 1. The Repository Pattern paid off immediately

When replacing `InMemoryJobRepository` with `MySQLJobRepository`, I changed exactly **one line** in `Application.cpp`. Every service, controller, and the worker continued to compile and work identically. This is the Repository Pattern working as designed.

### 2. `noexcept` as an architectural boundary

All service and worker methods are `noexcept`. They wrap their bodies in `try/catch` and log errors. This is a deliberate choice: a C++ exception escaping a thread function or a Drogon callback results in `std::terminate`. The `noexcept` keyword enforces the contract at compile time and makes the error-handling boundary visible.

### 3. Shallow clone is a product decision

`git clone --depth 1` fetches only the current snapshot. For analyzing a repository's current state (file count, language breakdown), history is irrelevant. This makes the analysis 10–100× faster for large repositories like the Linux kernel.

### 4. No framework magic in the DI layer

There is no DI framework. `Application::buildDependencyGraph()` is plain C++ — `make_shared` calls, assignment, method calls. The entire object graph is visible in one function. This is intentional: DI frameworks add indirection that makes debugging harder.

---

## Concurrency Implementation

**How does the worker communicate with the HTTP layer?**

Via a `std::condition_variable`. The HTTP thread (Drogon callback) calls `notifyJobAvailable()` which calls `cv_.notify_one()`. The worker thread is sleeping on `cv_.wait_for()` with a 1-second timeout. This is efficient — no polling, no busy-waiting.

**How is shutdown handled?**

`shutdown_requested_` is a `std::atomic<bool>`. Setting it to `true` and calling `notify_one()` causes the worker to exit its loop. `stop()` then calls `thread.join()`, waiting for the current job to finish. No detached threads.

**Is the in-memory repository thread-safe?**

Yes. Every method in `InMemoryJobRepository` takes a `std::lock_guard<std::mutex>` before accessing the map. The HTTP threads (write) and the worker thread (read/write) access it safely.

---

## Scalability Considerations

**What is the current bottleneck?**
The single worker thread. For large repositories, `git clone` is the bottleneck — it's I/O-bound, not CPU-bound. The solution (thread pool) requires no interface changes — just change `JobWorker` to manage multiple threads.

**How would you scale the database layer?**
The `IJobRepository` interface hides the storage backend. Adding read replicas would mean a `ShardedJobRepository` wrapping a write connection and a read-replica pool — zero service layer changes.

**How would you scale horizontally?**
Replace `InMemoryJobRepository` with a distributed queue (Redis, RabbitMQ) and have multiple Cortex instances consume from it. The `IJobRepository` interface abstracts this — a `RedisJobRepository` implementation would be the only change.

---

## Tradeoffs Made

| Decision | Tradeoff |
|---|---|
| In-memory analysis repository | Fast, simple — lost on restart. MySQL persistence is next. |
| Single worker thread | Simple, no race conditions — limited throughput. Thread pool is a clean upgrade. |
| `popen()` for git clone | Simple — could use `libgit2` for better error handling and no shell dependency. |
| Shallow clone only | Fast — means commit history analysis is not possible. |
| No connection pool | Simple — a single connection can block if MySQL is slow. Pool is straightforward to add to `Database`. |

---

## Common Interview Questions

**Q: Why C++ instead of Go or Rust for this?**

> C++ is the dominant language in high-performance backends, game engines, embedded systems, and trading systems. Demonstrating production-quality C++ — with proper RAII, `noexcept`, smart pointers, and C++20 features — is more differentiated than another Go CRUD service.

**Q: How would you add authentication?**

> Add an `AuthMiddleware` that intercepts requests before they reach controllers. Extract a token from the `Authorization` header, validate it against a `ITokenRepository` (in-memory or database), and reject with 401 if invalid. The controller layer never changes — middleware sits between Drogon's dispatch and the handler.

**Q: How would you test this system?**

> The DI architecture makes this straightforward. For unit tests: create a `MockJobRepository` implementing `IJobRepository` and inject it into `RepositoryService`. Assert the correct calls were made. For integration tests: start a real Drogon server, call the endpoints with `curl` or a C++ HTTP client, assert the JSON responses. The existing test scripts (`test_e2e.sh`) are the integration test foundation.

**Q: What happens if git clone takes 10 minutes for a huge repo?**

> The HTTP request returned immediately after submission (202 Accepted). The client polls `GET /jobs/{jobId}` every 2 seconds. The worker processes the job to completion. The only issue is the cloned directory consuming disk space — which is why job expiry and cleanup is planned in v1.3.

**Q: Why is the frontend polling instead of WebSockets?**

> Polling is simpler to implement correctly and sufficient for a demo. The interval is 2 seconds — for analysis jobs that take 10–60 seconds, this is acceptable. WebSocket push is planned in v2.0 and would replace the polling loop in `App.jsx` with a single WebSocket connection.

**Q: How did you handle SQL injection?**

> `MySQLJobRepository` uses prepared statements exclusively — `conn->prepareStatement(sql)` with positional `?` parameters. No string concatenation is used to build SQL queries anywhere in the codebase. Additionally, `UrlValidator` validates all user-supplied URLs before they reach the database layer.

**Q: What would you do differently?**

> Three things: (1) Use `libgit2` instead of `popen(git clone...)` for better error handling and no shell dependency. (2) Start with MySQL enabled and a Docker Compose setup so the demo is stateful by default. (3) Add a thread pool to the worker from day one, since sequential processing is the most obvious bottleneck.
