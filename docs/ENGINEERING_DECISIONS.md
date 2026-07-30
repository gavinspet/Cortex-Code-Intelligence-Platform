# Engineering Decisions

## Why Clean Architecture?

**The problem it solves:** In most C++ server projects, HTTP handling, business logic, and SQL are mixed together. This makes testing impossible and changes expensive.

**The decision:** Strict layering — controllers know nothing about databases, services know nothing about HTTP status codes, domain models know nothing about either.

**The payoff in this project:** When replacing `InMemoryJobRepository` with `MySQLJobRepository`, zero lines changed in `RepositoryService`, `JobService`, or `JobWorker`. The swap was purely a DI binding change in `Application::buildDependencyGraph()`.

---

## Why the Repository Pattern?

**The problem it solves:** Coupling business logic to a specific storage backend means any storage change requires touching service code.

**The decision:** Define `IJobRepository` as a pure virtual interface. Services hold a `shared_ptr<IJobRepository>` — they never know whether it points to memory, MySQL, or a test mock.

**The payoff:** The backend runs today with `InMemoryJobRepository`. When MySQL is available, it upgrades automatically at startup. The switch required no service-layer changes — exactly as designed.

---

## Why Dependency Injection via Constructor?

**The problem it solves:** Global singletons and service locators hide dependencies, making code hard to reason about and impossible to unit test in isolation.

**The decision:** Every class receives its dependencies through its constructor. `Application::buildDependencyGraph()` is the single wiring point for the entire object graph.

**Why constructor injection over setter injection:** Dependencies are guaranteed to be present from object construction. There are no partially-initialized objects.

---

## Why C++20?

**The decision:** C++20 provides exactly the features needed for this domain cleanly:

| Feature | Usage |
|---|---|
| `std::optional<T>` | Repository methods return `nullopt` instead of null pointers |
| `std::atomic<bool>` | Lock-free flags for worker thread control |
| `std::filesystem` | Recursive directory iteration for code scanning |
| `std::string_view` | Zero-copy string passing in validators |
| `constexpr` | `jobStatusToString()` with no runtime cost |
| Structured bindings | Range-based iteration over language distribution map |

**Why not Rust or Go?** The project deliberately chose C++ to demonstrate proficiency with the language most common in high-performance backend systems, embedded, and systems programming interviews.

---

## Why Drogon?

**The decision:** Drogon is the highest-performance C++ HTTP framework available, consistently ranking at the top of TechEmpower benchmarks.

**Key reasons:**
- Async I/O by default — non-blocking request handling across multiple threads
- Familiar API — `registerHandler(path, lambda, {HttpMethod})` is straightforward
- jsoncpp integration — consistent JSON handling throughout
- Production-proven — used in real production systems

**Why not Crow, Pistache, or Boost.Beast?** Drogon has the best combination of performance, API ergonomics, and community support for a production-quality REST API.

---

## Why a Background Worker Instead of Async HTTP?

**The problem it solves:** `git clone` of large repositories takes seconds to minutes. Blocking the HTTP thread would exhaust the thread pool for all other requests.

**The decision:** HTTP request returns immediately with a job ID (202 Accepted). A single background `std::thread` processes jobs sequentially from a queue.

**Why a single thread?** Sequential processing keeps the implementation simple and correct. The bottleneck is network I/O for cloning, not CPU. A thread pool can be added in a future version without changing the interface.

**Why `std::condition_variable` instead of polling?** The condition variable allows the worker to sleep efficiently and wake immediately when a new job is available. The 1-second timeout fallback prevents missed notifications.

---

## Why `noexcept` on Worker and Controller Methods?

**The decision:** All service, worker, and controller methods are marked `noexcept`. They wrap their bodies in `try/catch` and log errors rather than propagate them.

**Why:** A C++ exception escaping a Drogon callback or a thread function results in `std::terminate`. Explicit `noexcept` boundaries make this contract visible and enforced by the compiler.

---

## Why Prepared Statements Everywhere in MySQLJobRepository?

**The decision:** `MySQLJobRepository` uses `conn->prepareStatement(sql)` with positional `?` parameters for every operation. No string concatenation is used to build SQL.

**Why:** SQL injection is a critical vulnerability. Even though repository URLs are validated by `UrlValidator`, defense in depth requires that the database layer never trust its inputs. Prepared statements eliminate the injection surface entirely.

---

## Why React + Vite for the Frontend?

**The decision:** Minimal, functional, fast to build. The frontend is deliberately simple — its purpose is to demonstrate the API working end-to-end, not to showcase frontend engineering.

**Why Vite over CRA:** Vite starts in milliseconds, hot-reloads instantly, and has a built-in proxy configuration that eliminates CORS issues during development.

**Why no state management library:** The application has one piece of state — the current analysis job. `useState` and `useCallback` are sufficient. Adding Redux or Zustand would be over-engineering.

---

## Why MySQL Over PostgreSQL?

**The practical reason:** MySQL Connector/C++ is packaged in Ubuntu's standard apt repository (`libmysqlcppconn-dev`) with no extra PPA. PostgreSQL C++ connectors require additional setup.

**The architectural reason:** The `IJobRepository` interface means the storage backend is swappable. A `PostgreSQLJobRepository` could be implemented without touching any other code.

---

## Why `git clone --depth 1`?

**The decision:** Shallow clone (`--depth 1`) fetches only the latest commit, not the full history.

**Why:** History can add gigabytes to large repositories. For code analysis (file count, line count, language distribution), only the current state of the codebase is needed. Shallow clones are 10–100× faster for large repositories.
