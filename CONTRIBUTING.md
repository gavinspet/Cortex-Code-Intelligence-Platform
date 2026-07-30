# Contributing to Cortex Code Intelligence Platform

Thank you for your interest in contributing. This document outlines the standards and process for contributing to this project.

---

## Coding Standards

### C++ (Backend)

- **Standard:** C++20
- **Style:** Follow the existing code style — 4-space indentation, `camelCase` methods, `PascalCase` classes
- **Headers:** All new first-party files must include the standard Doxygen file header (see existing files)
- **Architecture:** Respect the layered Clean Architecture:
  - Controllers handle HTTP only — no business logic
  - Services contain business logic — no HTTP types
  - Repository implementations are the only place SQL appears
- **Error handling:** Service and worker methods must be `noexcept` and use `try/catch` with `Logger::instance().error()`
- **SQL:** Use prepared statements only — never concatenate user input into SQL strings
- **Smart pointers:** Prefer `std::shared_ptr` for shared ownership, `std::unique_ptr` for exclusive ownership
- **No raw `new`/`delete`**

### React (Frontend)

- **Functional components and hooks only** — no class components
- **No state management libraries** — `useState` and `useCallback` are sufficient
- **Styling:** Edit `App.css` — no inline styles

---

## Branch Naming

```
feature/<short-description>     # New features
fix/<short-description>         # Bug fixes
docs/<short-description>        # Documentation only
refactor/<short-description>    # Code restructuring (no behaviour change)
chore/<short-description>       # Tooling, CI, dependencies
```

Examples:
- `feature/connection-pool`
- `fix/worker-shutdown-race`
- `docs/api-examples`

---

## Commit Message Convention

Follow [Conventional Commits](https://www.conventionalcommits.org/):

```
<type>(<scope>): <short description>

[optional body]

[optional footer]
```

**Types:** `feat`, `fix`, `docs`, `refactor`, `test`, `chore`

**Examples:**
```
feat(worker): add thread pool for parallel job processing
fix(mysql): handle connection timeout in MySQLJobRepository
docs(api): add curl examples for all endpoints
refactor(di): extract DI wiring into dedicated builder class
```

---

## Pull Request Checklist

Before opening a PR, confirm:

- [ ] The backend compiles without errors: `cmake --build build`
- [ ] No new compiler warnings introduced
- [ ] Existing behaviour is preserved (run `test_e2e.sh`)
- [ ] New C++ files have the standard Doxygen file header
- [ ] Architecture boundaries are respected (no HTTP types in service layer, etc.)
- [ ] No SQL string concatenation (use prepared statements)
- [ ] PR description explains *what* changed and *why*

---

## Issue Reporting

Use the [bug report template](.github/ISSUE_TEMPLATE/bug_report.md) for bugs and the [feature request template](.github/ISSUE_TEMPLATE/feature_request.md) for enhancements.

When reporting a bug, include:
- Steps to reproduce
- Expected behaviour
- Actual behaviour
- Backend log output (`backend/logs/cortex.log`)
- OS and compiler version

---

## Development Setup

See [docs/SETUP.md](docs/SETUP.md) for prerequisites and build instructions.

---

## Questions

Open a [GitHub Discussion](https://github.com/gavinspet/Cortex-Code-Intelligence-Platform/discussions) for questions about the architecture or design decisions.
