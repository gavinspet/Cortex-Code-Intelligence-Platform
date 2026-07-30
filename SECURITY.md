# Security Policy

## Supported Versions

| Version | Supported |
|---|---|
| `main` (latest) | ✅ |

---

## Reporting a Vulnerability

**Please do not open a public GitHub issue for security vulnerabilities.**

Report security issues privately by emailing:

**kartick.ghosh.dev@gmail.com**

Include in your report:
- Description of the vulnerability
- Steps to reproduce
- Potential impact
- Suggested fix (if any)

You will receive a response within 48 hours. If the issue is confirmed, a fix will be prepared and a coordinated disclosure will be made.

---

## Security Design Notes

This project was designed with the following security practices:

- **No SQL injection surface** — `MySQLJobRepository` uses prepared statements exclusively; no user input is concatenated into SQL strings
- **URL validation** — All repository URLs are validated against a strict allowlist (HTTPS GitHub/GitLab only) before processing
- **No authentication tokens stored** — The system does not handle user credentials or private repository tokens
- **`noexcept` error boundaries** — Exceptions never escape worker threads or HTTP handlers, preventing undefined behavior from reaching the HTTP framework
- **No eval or shell expansion** — The `git clone` command is constructed from a validated URL with no shell metacharacter risk
