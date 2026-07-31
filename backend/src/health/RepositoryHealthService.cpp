/**
 * @file RepositoryHealthService.cpp
 * @brief Repository Health & Quality Engine — static analysis only.
 * @project Cortex Code Intelligence Platform
 * @author Kartick Kumar Ghosh
 * @github https://github.com/gavinspet
 * @email kartick.ghosh.dev@gmail.com
 * @copyright © 2026
 * @license MIT
 *
 * Scoring algorithm (total = 100 points)
 * ────────────────────────────────────────────────────────────────────────────
 * Category           Max  Rules
 * ─────────────────  ───  ─────────────────────────────────────────────────
 * Documentation       20  README(5) LICENSE(4) CHANGELOG(3) CONTRIBUTING(3)
 *                        SECURITY.md(3) CODE_OF_CONDUCT(2)
 *
 * Testing             20  Framework detected(10) test/ dir(5)
 *                        Coverage config(3) E2E config(2)
 *
 * CI/CD               15  Any CI provider(12) +multi-provider bonus(2)
 *                        Deployment config(1)
 *
 * Security            15  SECURITY.md(4) Dependabot(4) CodeQL/SAST(3)
 *                        .gitignore present(2) .env.example(2)
 *
 * Maintainability     15  .editorconfig(3) Lint config(4) Format config(3)
 *                        Lockfile(3) .gitattributes(2)
 *
 * Configuration       10  .gitignore(4) Docker/Compose(4) .env.example(2)
 *
 * ProjectStructure     5  src/(1) tests/(1) docs/(1) include/(1) examples/(1)
 * ─────────────────  ───
 * Total              100
 *
 * Grade: A(90-100) B(80-89) C(70-79) D(60-69) F(<60)
 */
#include "health/RepositoryHealthService.h"
#include "logging/Logger.h"
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <chrono>
#include <sstream>
#include <numeric>

namespace cortex::health {

namespace fs = std::filesystem;
using cortex::logging::Logger;

// ─── Constants ────────────────────────────────────────────────────────────────

constexpr size_t MAX_READ_BYTES = 65536; // 64 KB

static const std::vector<std::string> COMMON_SUBDIRS = {
    "backend", "frontend", "src", "server", "client", "app",
    "web", "api", "lib", "packages/backend", "packages/frontend",
    "apps/backend", "apps/frontend", "apps/api", "apps/web"
};

// ─── File utilities ───────────────────────────────────────────────────────────

namespace {

bool fileExists(const fs::path& root, const std::string& rel) noexcept {
    try { return fs::exists(root / rel) && fs::is_regular_file(root / rel); }
    catch (...) { return false; }
}

bool dirExists(const fs::path& root, const std::string& rel) noexcept {
    try { return fs::is_directory(root / rel); }
    catch (...) { return false; }
}

bool anyFileExists(const fs::path& root,
                   std::initializer_list<const char*> rels) noexcept {
    for (auto r : rels)
        if (fileExists(root, r)) return true;
    return false;
}

bool anyDirExists(const fs::path& root,
                  std::initializer_list<const char*> rels) noexcept {
    for (auto r : rels)
        if (dirExists(root, r)) return true;
    return false;
}

std::string readFile(const fs::path& root, const std::string& rel) noexcept {
    try {
        fs::path p = root / rel;
        if (!fs::exists(p) || !fs::is_regular_file(p)) return {};
        std::ifstream f(p, std::ios::binary);
        if (!f) return {};
        std::string buf;
        buf.resize(std::min(static_cast<uintmax_t>(MAX_READ_BYTES), fs::file_size(p)));
        f.read(buf.data(), static_cast<std::streamsize>(buf.size()));
        return buf;
    } catch (...) { return {}; }
}

/// Check root + all COMMON_SUBDIRS.
bool fileExistsAnywhere(const fs::path& root,
                         std::initializer_list<const char*> rels) noexcept {
    if (anyFileExists(root, rels)) return true;
    for (const auto& sub : COMMON_SUBDIRS)
        if (anyFileExists(root / sub, rels)) return true;
    return false;
}

bool dirExistsAnywhere(const fs::path& root, const std::string& name) noexcept {
    if (dirExists(root, name)) return true;
    for (const auto& sub : COMMON_SUBDIRS)
        if (dirExists(root / sub, name)) return true;
    return false;
}

std::string readFileAnywhere(const fs::path& root, const std::string& fn) noexcept {
    auto c = readFile(root, fn);
    if (!c.empty()) return c;
    for (const auto& sub : COMMON_SUBDIRS) {
        c = readFile(root / sub, fn);
        if (!c.empty()) return c;
    }
    return {};
}

bool contains(const std::string& h, const std::string& n) noexcept {
    return h.find(n) != std::string::npos;
}

bool requirementContains(const std::string& content,
                          const std::string& pkg) noexcept {
    std::istringstream ss(content);
    std::string line;
    while (std::getline(ss, line)) {
        size_t s = line.find_first_not_of(" \t");
        if (s == std::string::npos || line[s] == '#') continue;
        line = line.substr(s);
        std::string ll = line, lp = pkg;
        std::transform(ll.begin(), ll.end(), ll.begin(), ::tolower);
        std::transform(lp.begin(), lp.end(), lp.begin(), ::tolower);
        if (ll.substr(0, lp.size()) == lp &&
            (ll.size() == lp.size() ||
             ll[lp.size()] == '=' || ll[lp.size()] == '>' ||
             ll[lp.size()] == '<' || ll[lp.size()] == '[' ||
             ll[lp.size()] == ' '))
            return true;
    }
    return false;
}

/// Check whether any YAML exists in .github/workflows/.
bool hasGitHubWorkflow(const fs::path& root) noexcept {
    try {
        fs::path wf = root / ".github" / "workflows";
        if (!fs::is_directory(wf)) return false;
        for (auto& e : fs::directory_iterator(wf)) {
            auto ext = e.path().extension().string();
            if (ext == ".yml" || ext == ".yaml") return true;
        }
    } catch (...) {}
    return false;
}

/// Map a 0-100 integer to a letter grade.
std::string letterGrade(int score) noexcept {
    if (score >= 90) return "A";
    if (score >= 80) return "B";
    if (score >= 70) return "C";
    if (score >= 60) return "D";
    return "F";
}

/// Compute a per-category letter grade from raw/max.
CategoryScore makeCategory(int score, int maxScore) noexcept {
    CategoryScore cs;
    cs.score    = score;
    cs.maxScore = maxScore;
    int pct     = (maxScore > 0) ? (score * 100 / maxScore) : 0;
    cs.grade    = letterGrade(pct);
    return cs;
}

} // anonymous namespace

// ─── Category evaluators ──────────────────────────────────────────────────────

/**
 * Documentation (max 20 points)
 *
 * Rule          Points  File(s)
 * ─────────────────────────────────────────────
 * README            5   README.md / README.rst / README
 * LICENSE           4   LICENSE / LICENSE.md / LICENCE
 * CHANGELOG         3   CHANGELOG.md / HISTORY.md
 * CONTRIBUTING      3   CONTRIBUTING.md
 * SECURITY          3   SECURITY.md
 * CODE_OF_CONDUCT   2   CODE_OF_CONDUCT.md
 */
static CategoryScore evaluateDocumentation(const fs::path& root) noexcept
{
    int score = 0;
    const int MAX = 20;

    if (anyFileExists(root, {"README.md","README.rst","README.txt","README","readme.md"}))
        score += 5;
    if (anyFileExists(root, {"LICENSE","LICENSE.md","LICENSE.txt","LICENCE","COPYING"}))
        score += 4;
    if (anyFileExists(root, {"CHANGELOG.md","CHANGELOG.txt","CHANGELOG","HISTORY.md"}))
        score += 3;
    if (anyFileExists(root, {"CONTRIBUTING.md","CONTRIBUTING.txt","CONTRIBUTING"}))
        score += 3;
    if (anyFileExists(root, {"SECURITY.md","SECURITY.txt","SECURITY",
                              ".github/SECURITY.md"}))
        score += 3;
    if (anyFileExists(root, {"CODE_OF_CONDUCT.md","CODE_OF_CONDUCT.txt",
                              ".github/CODE_OF_CONDUCT.md"}))
        score += 2;

    return makeCategory(score, MAX);
}

/**
 * Testing (max 20 points)
 *
 * Rule                      Points  Detection
 * ─────────────────────────────────────────────────────────────────────
 * Test framework detected      10   CMake GTest/Catch2, package.json jest/vitest/mocha,
 *                                   requirements.txt pytest, pom.xml junit, Cargo test deps
 * Test directory present        5   test/, tests/, __tests__, spec/
 * Coverage config detected      3   .coveragerc, jest --coverage, tarpaulin.toml, .codecov.yml
 * E2E test config               2   cypress.config.*, playwright.config.*
 */
static CategoryScore evaluateTesting(const fs::path& root) noexcept
{
    int score = 0;
    const int MAX = 20;

    // ── Test framework ──────────────────────────────────────────────────────
    bool hasFramework = false;

    const auto cmake = readFileAnywhere(root, "CMakeLists.txt");
    if (contains(cmake, "GTest") || contains(cmake, "Catch2") ||
        contains(cmake, "doctest") || contains(cmake, "Boost.Test"))
        hasFramework = true;

    const auto pkg = readFileAnywhere(root, "package.json");
    for (const char* t : {"\"jest\"","\"vitest\"","\"mocha\"","\"jasmine\"",
                           "\"ava\"","@testing-library"})
        if (contains(pkg, t)) { hasFramework = true; break; }

    const auto req = readFileAnywhere(root, "requirements.txt");
    if (requirementContains(req, "pytest") || requirementContains(req, "unittest2"))
        hasFramework = true;

    const auto pom = readFileAnywhere(root, "pom.xml");
    if (contains(pom, "junit") || contains(pom, "testng"))
        hasFramework = true;

    const auto cargo = readFileAnywhere(root, "Cargo.toml");
    // Cargo has built-in test support, but we check for additional test dependencies
    if (contains(cargo, "tokio") && contains(cargo, "[dev-dependencies]"))
        hasFramework = true;

    if (hasFramework) score += 10;

    // ── Test directory ──────────────────────────────────────────────────────
    bool hasTestDir = false;
    for (const char* d : {"test","tests","__tests__","spec","specs","testing"}) {
        if (dirExistsAnywhere(root, d)) { hasTestDir = true; break; }
    }
    if (hasTestDir) score += 5;

    // ── Coverage config ─────────────────────────────────────────────────────
    if (fileExistsAnywhere(root, {".coveragerc",".nycrc",".c8rc",
                                   "tarpaulin.toml",".codecov.yml","codecov.yaml"}) ||
        (contains(pkg, "coverage") && !pkg.empty()))
        score += 3;

    // ── E2E test config ─────────────────────────────────────────────────────
    if (fileExistsAnywhere(root, {"cypress.config.js","cypress.config.ts",
                                   "playwright.config.js","playwright.config.ts",
                                   "wdio.conf.js"}) ||
        contains(pkg, "\"cypress\"") || contains(pkg, "\"playwright\""))
        score += 2;

    return makeCategory(score, MAX);
}

/**
 * CI/CD (max 15 points)
 *
 * Rule                     Points
 * ─────────────────────────────────────────────────────────────────────
 * Primary CI detected          12   GitHub Actions / GitLab CI /
 *                                   CircleCI / Jenkins / Travis CI
 * Secondary CI (+bonus)        +2   If ≥ 2 distinct providers detected
 * Deployment config             1   render.yaml / vercel.json / Dockerfile
 */
static CategoryScore evaluateCICD(const fs::path& root) noexcept
{
    int score = 0;
    const int MAX = 15;

    int providers = 0;
    if (hasGitHubWorkflow(root))           ++providers;
    if (fileExists(root, ".gitlab-ci.yml")) ++providers;
    if (fileExists(root, ".circleci/config.yml")) ++providers;
    if (fileExists(root, "Jenkinsfile") || fileExists(root, "jenkins.yml")) ++providers;
    if (fileExists(root, ".travis.yml"))   ++providers;
    if (fileExists(root, "azure-pipelines.yml")) ++providers;
    if (fileExists(root, "bitbucket-pipelines.yml")) ++providers;
    if (fileExists(root, ".drone.yml") || fileExists(root, ".drone.yaml")) ++providers;

    if (providers >= 1) score += 12;
    if (providers >= 2) score += 2;

    // Deployment/hosting config
    if (anyFileExists(root, {"render.yaml","vercel.json","netlify.toml",
                              "fly.toml","railway.json","heroku.yml",
                              "Dockerfile","docker-compose.yml","docker-compose.yaml"}))
        score += 1;

    return makeCategory(std::min(score, MAX), MAX);
}

/**
 * Security (max 15 points)
 *
 * Rule                    Points  Detection
 * ─────────────────────────────────────────
 * SECURITY.md                  4  SECURITY.md / .github/SECURITY.md
 * Dependabot config            4  .github/dependabot.yml
 * CodeQL / SAST scanning       3  codeql*.yml in .github/workflows/
 * .gitignore present           2  Prevents accidental secret commits
 * .env.example / template      2  Documents expected secrets safely
 */
static CategoryScore evaluateSecurity(const fs::path& root) noexcept
{
    int score = 0;
    const int MAX = 15;

    if (anyFileExists(root, {"SECURITY.md","SECURITY",".github/SECURITY.md"}))
        score += 4;

    if (fileExists(root, ".github/dependabot.yml") ||
        fileExists(root, ".github/dependabot.yaml"))
        score += 4;

    // CodeQL or security scanning workflow
    try {
        fs::path wf = root / ".github" / "workflows";
        if (fs::is_directory(wf)) {
            for (auto& e : fs::directory_iterator(wf)) {
                std::string name = e.path().filename().string();
                std::transform(name.begin(), name.end(), name.begin(), ::tolower);
                if (name.find("codeql") != std::string::npos ||
                    name.find("security") != std::string::npos ||
                    name.find("snyk") != std::string::npos ||
                    name.find("semgrep") != std::string::npos) {
                    score += 3;
                    break;
                }
            }
        }
    } catch (...) {}

    if (anyFileExists(root, {".gitignore",".gitignore_global"}))
        score += 2;

    if (anyFileExists(root, {".env.example",".env.sample",".env.template",
                              ".env.dist","env.example"}))
        score += 2;

    return makeCategory(std::min(score, MAX), MAX);
}

/**
 * Maintainability (max 15 points)
 *
 * Rule                   Points  Detection
 * ──────────────────────────────────────────────────────────────────────
 * .editorconfig               3  .editorconfig
 * Linting config              4  .eslintrc* / .rubocop.yml / clang-format / etc.
 * Formatting config           3  .prettierrc* / rustfmt.toml / .clang-format
 * Lockfile present            3  package-lock.json / yarn.lock / Cargo.lock / poetry.lock
 * .gitattributes              2  Consistent line endings
 */
static CategoryScore evaluateMaintainability(const fs::path& root) noexcept
{
    int score = 0;
    const int MAX = 15;

    if (fileExistsAnywhere(root, {".editorconfig"}))
        score += 3;

    // Linting configs
    if (fileExistsAnywhere(root, {".eslintrc",".eslintrc.js",".eslintrc.json",
                                   ".eslintrc.yml",".eslintrc.yaml","eslint.config.js",
                                   ".rubocop.yml",".pylintrc","setup.cfg"}) ||
        fileExistsAnywhere(root, {".clang-tidy","compile_commands.json"}) ||
        !readFileAnywhere(root, "pyproject.toml").empty())
        score += 4;

    // Formatting configs
    if (fileExistsAnywhere(root, {".prettierrc",".prettierrc.js",".prettierrc.json",
                                   ".prettierrc.yaml","prettier.config.js",
                                   "rustfmt.toml",".rustfmt.toml",
                                   ".clang-format","_clang-format"}))
        score += 3;

    // Lockfile (dependency pinning = deterministic builds)
    if (fileExistsAnywhere(root, {"package-lock.json","yarn.lock","pnpm-lock.yaml",
                                   "bun.lockb","Cargo.lock","poetry.lock",
                                   "Pipfile.lock","go.sum"}))
        score += 3;

    if (fileExists(root, ".gitattributes"))
        score += 2;

    return makeCategory(std::min(score, MAX), MAX);
}

/**
 * Configuration (max 10 points)
 *
 * Rule               Points  Detection
 * ────────────────────────────────────────────────────────────
 * .gitignore              4  .gitignore at root
 * Container config        4  Dockerfile / docker-compose.yml
 * .env template           2  .env.example / .env.sample
 */
static CategoryScore evaluateConfiguration(const fs::path& root) noexcept
{
    int score = 0;
    const int MAX = 10;

    if (fileExists(root, ".gitignore"))
        score += 4;

    if (anyFileExists(root, {"Dockerfile","dockerfile","docker-compose.yml",
                              "docker-compose.yaml","docker-compose.dev.yml"}))
        score += 4;

    if (anyFileExists(root, {".env.example",".env.sample",".env.template",
                              ".env.dist","env.example"}))
        score += 2;

    return makeCategory(std::min(score, MAX), MAX);
}

/**
 * Project Structure (max 5 points)
 *
 * Rule               Points  Detection
 * ─────────────────────────────────────────────
 * src/ or lib/            1  Standard source root
 * tests/ or test/         1  Dedicated test root
 * docs/ or doc/           1  Documentation directory
 * include/                1  C/C++ header convention
 * examples/ or samples/   1  Usage examples
 */
static CategoryScore evaluateProjectStructure(const fs::path& root) noexcept
{
    int score = 0;
    const int MAX = 5;

    if (anyDirExists(root, {"src","lib","source","Sources"})) score += 1;
    if (anyDirExists(root, {"tests","test","__tests__","spec"})) score += 1;
    if (anyDirExists(root, {"docs","doc","documentation"})) score += 1;
    if (anyDirExists(root, {"include","headers","inc"})) score += 1;
    if (anyDirExists(root, {"examples","example","samples","sample","demo"})) score += 1;

    return makeCategory(score, MAX);
}

// ─── Narrative generators ─────────────────────────────────────────────────────

static void generateNarrative(const RepositoryHealthResult& r,
                               RepositoryHealthResult& out) noexcept
{
    auto& S = out.strengths;
    auto& W = out.warnings;
    auto& R = out.recommendations;

    const auto& c = r.categories;

    // ── Strengths ────────────────────────────────────────────────────────────

    if (c.documentation.score == c.documentation.maxScore)
        S.push_back("Complete project documentation (README, LICENSE, CHANGELOG, "
                    "CONTRIBUTING, SECURITY, CODE_OF_CONDUCT)");
    else if (c.documentation.score >= 12)
        S.push_back("Well-documented project with core community files present");

    if (c.ciCd.score >= 12)
        S.push_back("Automated CI/CD pipeline configured");

    if (c.testing.score >= 15)
        S.push_back("Solid testing infrastructure with framework and test directory");
    else if (c.testing.score >= 10)
        S.push_back("Test framework detected");

    if (c.security.score >= 8)
        S.push_back("Security practices in place (policy + Dependabot)");
    else if (c.security.score >= 4)
        S.push_back("Security policy (SECURITY.md) documented");

    if (c.configuration.score >= 8)
        S.push_back("Docker containerization and deployment configuration present");
    else if (c.configuration.score >= 4)
        S.push_back("Standard project configuration files present");

    if (c.maintainability.score >= 12)
        S.push_back("Consistent code style enforced via linting and formatting tools");
    else if (c.maintainability.score >= 6)
        S.push_back("Code quality tools configured");

    if (c.projectStructure.score == c.projectStructure.maxScore)
        S.push_back("Exemplary project directory structure");

    // ── Warnings ─────────────────────────────────────────────────────────────

    if (c.documentation.score < 5)
        W.push_back("README file not found — essential for project discovery");

    const bool hasLicense = c.documentation.score >= 4 ||
                            c.documentation.score >= 9; // crude check
    // Re-evaluate license specifically by score contribution
    if (c.documentation.score < 4)
        W.push_back("No LICENSE file detected");

    if (c.testing.score == 0)
        W.push_back("No test framework or test directory found");
    else if (c.testing.score < 10)
        W.push_back("Test infrastructure is incomplete");

    if (c.ciCd.score == 0)
        W.push_back("No CI/CD pipeline detected — code changes are not automatically validated");

    if (c.security.score < 4)
        W.push_back("No security policy file (SECURITY.md) found");

    if (c.security.score < 8 && c.security.score >= 4)
        W.push_back("Dependabot not configured — dependency vulnerabilities may go undetected");

    if (c.maintainability.score < 5)
        W.push_back("No code quality tooling (linters/formatters) detected");

    if (c.configuration.score < 4)
        W.push_back("No .gitignore file — risk of accidentally committing generated files");

    if (r.overallScore < 60)
        W.push_back("Overall repository health score is below acceptable threshold");

    // ── Recommendations ───────────────────────────────────────────────────────

    if (c.documentation.score < 5)
        R.push_back("Add a README.md with project overview, installation instructions, and usage examples");

    if (c.documentation.score < 9)
        R.push_back("Add a LICENSE file to clearly define usage and distribution rights");

    if (c.documentation.score < 12)
        R.push_back("Add CHANGELOG.md to document version history and breaking changes");

    if (c.documentation.score < 15)
        R.push_back("Add SECURITY.md describing the vulnerability reporting process");

    if (c.testing.score < 10)
        R.push_back("Integrate a test framework (GoogleTest, Jest, pytest) and add a tests/ directory");

    if (c.testing.score >= 10 && c.testing.score < 13)
        R.push_back("Add code coverage reporting (e.g., lcov, nyc, pytest-cov) to measure test completeness");

    if (c.ciCd.score == 0)
        R.push_back("Add a GitHub Actions workflow (.github/workflows/ci.yml) for automated build and test");

    if (c.security.score < 8)
        R.push_back("Configure Dependabot (.github/dependabot.yml) for automated security dependency updates");

    if (c.security.score >= 4 && c.security.score < 7)
        R.push_back("Enable CodeQL analysis in GitHub Actions for static security scanning");

    if (c.maintainability.score < 3)
        R.push_back("Add .editorconfig to enforce consistent indentation and line endings across editors");

    if (c.maintainability.score < 7)
        R.push_back("Configure a linter (ESLint, clang-tidy, pylint) to enforce code standards");

    if (c.maintainability.score >= 7 && c.maintainability.score < 10)
        R.push_back("Add a code formatter (.prettierrc, .clang-format) for consistent style");

    if (c.configuration.score < 4)
        R.push_back("Add .gitignore to prevent generated artifacts, logs, and IDE files from being committed");

    if (c.configuration.score >= 4 && c.configuration.score < 8)
        R.push_back("Consider Docker containerisation to ensure reproducible builds and deployments");

    if (c.configuration.score < 10)
        R.push_back("Add .env.example to document required environment variables without exposing secrets");
}

// ─── Service implementation ───────────────────────────────────────────────────

std::optional<RepositoryHealthResult> RepositoryHealthService::evaluateAndStore(
    const std::string& jobId,
    const std::string& clonePath) noexcept
{
    auto& log = Logger::instance();

    try {
        const auto startTime = std::chrono::steady_clock::now();
        log.info("Repository health evaluation started for job=" + jobId
                 + " path=" + clonePath);

        fs::path root(clonePath);
        if (!fs::is_directory(root)) {
            log.warn("RepositoryHealthService: path not found: " + clonePath);
            return std::nullopt;
        }

        RepositoryHealthResult result;
        result.jobId       = jobId;
        result.evaluatedAt = std::chrono::system_clock::now();

        // Evaluate all 7 categories
        result.categories.documentation   = evaluateDocumentation(root);
        result.categories.testing         = evaluateTesting(root);
        result.categories.ciCd            = evaluateCICD(root);
        result.categories.security        = evaluateSecurity(root);
        result.categories.maintainability = evaluateMaintainability(root);
        result.categories.configuration   = evaluateConfiguration(root);
        result.categories.projectStructure = evaluateProjectStructure(root);

        // Composite score (categories sum to 100)
        result.overallScore =
            result.categories.documentation.score +
            result.categories.testing.score +
            result.categories.ciCd.score +
            result.categories.security.score +
            result.categories.maintainability.score +
            result.categories.configuration.score +
            result.categories.projectStructure.score;

        result.grade = letterGrade(result.overallScore);

        // Generate human-readable narrative
        generateNarrative(result, result);

        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startTime).count();

        log.info("Repository health evaluation completed for job=" + jobId
                 + " score=" + std::to_string(result.overallScore)
                 + " grade=" + result.grade
                 + " strengths=" + std::to_string(result.strengths.size())
                 + " warnings=" + std::to_string(result.warnings.size())
                 + " elapsed=" + std::to_string(elapsed) + "ms");

        if (repository_) {
            repository_->save(result);
            log.info("Repository health stored for job=" + jobId);
        }

        return result;

    } catch (const std::exception& e) {
        log.error(std::string("RepositoryHealthService::evaluateAndStore exception: ") + e.what());
        return std::nullopt;
    } catch (...) {
        log.error("RepositoryHealthService::evaluateAndStore unknown exception");
        return std::nullopt;
    }
}

std::optional<RepositoryHealthResult> RepositoryHealthService::getHealth(
    const std::string& jobId) const noexcept
{
    if (!repository_) return std::nullopt;
    return repository_->findByJobId(jobId);
}

} // namespace cortex::health
