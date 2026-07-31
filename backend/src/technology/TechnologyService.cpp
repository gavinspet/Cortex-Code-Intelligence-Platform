/**
 * @file TechnologyService.cpp
 * @brief Repository Technology Intelligence Engine — static detection only.
 * @project Cortex Code Intelligence Platform
 * @author Kartick Kumar Ghosh
 * @github https://github.com/gavinspet
 * @email kartick.ghosh.dev@gmail.com
 * @copyright © 2026
 * @license MIT
 *
 * Detection methodology
 * ─────────────────────
 * 1. FILE SIGNATURE — check whether a specific file/directory exists.
 *    Confidence: 70–80 (file presence is strong but not conclusive)
 *
 * 2. CONTENT PATTERN — read up to 64 KB of a known config file and search
 *    for a literal string pattern.
 *    Confidence: 90–95 (explicit config reference is highly reliable)
 *
 * No repository code is ever executed. All analysis is purely static.
 */
#include "technology/TechnologyService.h"
#include "logging/Logger.h"
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <chrono>
#include <numeric>
#include <sstream>

namespace cortex::technology {

namespace fs = std::filesystem;
using cortex::logging::Logger;

// ─── File I/O helpers ─────────────────────────────────────────────────────────

namespace {

constexpr size_t MAX_READ_BYTES = 65536; // 64 KB max per file

bool fileExists(const fs::path& root, const std::string& rel) noexcept {
    try { return fs::exists(root / rel) && fs::is_regular_file(root / rel); }
    catch (...) { return false; }
}

bool anyFileExists(const fs::path& root,
                   std::initializer_list<const char*> rels) noexcept {
    for (auto r : rels)
        if (fileExists(root, r)) return true;
    return false;
}

bool dirExists(const fs::path& root, const std::string& rel) noexcept {
    try { return fs::is_directory(root / rel); }
    catch (...) { return false; }
}

/// Read up to MAX_READ_BYTES from a file. Returns empty string on failure.
std::string readFile(const fs::path& root, const std::string& rel) noexcept {
    try {
        fs::path p = root / rel;
        if (!fs::exists(p) || !fs::is_regular_file(p)) return {};
        std::ifstream f(p, std::ios::binary);
        if (!f) return {};
        std::string buf;
        buf.resize(std::min(static_cast<uintmax_t>(MAX_READ_BYTES),
                            fs::file_size(p)));
        f.read(buf.data(), static_cast<std::streamsize>(buf.size()));
        return buf;
    } catch (...) {
        return {};
    }
}

/// Common subdirectory names used in monorepos and standard project layouts.
static const std::vector<std::string> COMMON_SUBDIRS = {
    "backend", "frontend", "src", "server", "client", "app",
    "web", "api", "lib", "packages/backend", "packages/frontend",
    "apps/backend", "apps/frontend", "apps/api", "apps/web"
};

/// Read a file from root OR first matching standard subdirectory.
/// This handles monorepos (e.g., backend/CMakeLists.txt, frontend/package.json).
std::string readFileAnywhere(const fs::path& root, const std::string& filename) noexcept {
    auto c = readFile(root, filename);
    if (!c.empty()) return c;
    for (const auto& sub : COMMON_SUBDIRS) {
        c = readFile(root / sub, filename);
        if (!c.empty()) return c;
    }
    return {};
}

/// Check existence in root OR standard subdirectories.
bool fileExistsAnywhere(const fs::path& root,
                         std::initializer_list<const char*> rels) noexcept {
    for (auto r : rels) {
        if (fileExists(root, r)) return true;
        for (const auto& sub : COMMON_SUBDIRS)
            if (fileExists(root / sub, r)) return true;
    }
    return false;
}

bool contains(const std::string& haystack, const std::string& needle) noexcept {
    return haystack.find(needle) != std::string::npos;
}

/// Case-insensitive search.
bool containsCI(const std::string& haystack, const std::string& needle) noexcept {
    try {
        std::string h = haystack, n = needle;
        std::transform(h.begin(), h.end(), h.begin(), ::tolower);
        std::transform(n.begin(), n.end(), n.begin(), ::tolower);
        return h.find(n) != std::string::npos;
    } catch (...) {
        return false;
    }
}

/// Scan for any YAML file inside .github/workflows/
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

/// Scan requirements.txt line-by-line (case-insensitive package name prefix match).
bool requirementContains(const std::string& content,
                          const std::string& pkg) noexcept {
    std::istringstream ss(content);
    std::string line;
    while (std::getline(ss, line)) {
        // strip leading whitespace
        size_t s = line.find_first_not_of(" \t");
        if (s == std::string::npos || line[s] == '#') continue;
        line = line.substr(s);
        std::string ll = line;
        std::transform(ll.begin(), ll.end(), ll.begin(), ::tolower);
        std::string lp = pkg;
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

TechnologyItem item(std::string name, int conf, std::string reason,
                    std::string category = "") noexcept {
    return { std::move(name), conf, std::move(reason), std::move(category) };
}

} // anonymous namespace

// ─── Framework detection ──────────────────────────────────────────────────────

static void detectFrontendFrameworks(const fs::path& root,
                                     TechnologyAnalysis& out) noexcept
{
    const std::string pkg = readFileAnywhere(root, "package.json");

    // React
    if (contains(pkg, "\"react\"") || contains(pkg, "'react'")) {
        int conf = contains(pkg, "\"react\":") ? 95 : 80;
        out.frontendFrameworks.push_back(item("React", conf,
            "Detected in package.json dependencies"));
    }

    // Next.js (overrides plain React)
    if (fileExistsAnywhere(root, {"next.config.js","next.config.mjs","next.config.ts"})
        || contains(pkg, "\"next\"")) {
        out.frontendFrameworks.push_back(item("Next.js", 97,
            "next.config.* file or package.json dependency detected"));
    }

    // Vue
    if (fileExistsAnywhere(root, {"vue.config.js","vue.config.ts"})
        || contains(pkg, "\"vue\"")) {
        out.frontendFrameworks.push_back(item("Vue.js", 90,
            "vue.config.* or package.json vue dependency"));
    }

    // Nuxt (on top of Vue)
    if (fileExistsAnywhere(root, {"nuxt.config.js","nuxt.config.ts"})
        || contains(pkg, "\"nuxt\"")) {
        out.frontendFrameworks.push_back(item("Nuxt.js", 95,
            "nuxt.config.* file or package.json nuxt dependency"));
    }

    // Angular
    if (fileExistsAnywhere(root, {"angular.json"})) {
        out.frontendFrameworks.push_back(item("Angular", 97,
            "angular.json workspace configuration detected"));
    }

    // Svelte
    if (fileExistsAnywhere(root, {"svelte.config.js","svelte.config.ts"})
        || contains(pkg, "\"svelte\"")) {
        out.frontendFrameworks.push_back(item("Svelte", 92,
            "svelte.config.* or package.json svelte dependency"));
    }

    // Vite (bundler / framework)
    if (fileExistsAnywhere(root, {"vite.config.js","vite.config.ts","vite.config.mjs"})
        || contains(pkg, "\"vite\"")) {
        out.frontendFrameworks.push_back(item("Vite", 90,
            "vite.config.* file or package.json vite dependency"));
    }

    // Astro
    if (fileExistsAnywhere(root, {"astro.config.mjs","astro.config.ts"})
        || contains(pkg, "\"astro\"")) {
        out.frontendFrameworks.push_back(item("Astro", 93,
            "astro.config.* file detected"));
    }

    // Electron
    if (contains(pkg, "\"electron\"")) {
        out.frontendFrameworks.push_back(item("Electron", 88,
            "electron dependency in package.json"));
    }
}

static void detectBackendFrameworks(const fs::path& root,
                                    TechnologyAnalysis& out) noexcept
{
    const std::string pkg  = readFileAnywhere(root, "package.json");
    const std::string req  = readFileAnywhere(root, "requirements.txt");
    const std::string req2 = readFileAnywhere(root, "requirements-dev.txt");
    const std::string cmake = readFileAnywhere(root, "CMakeLists.txt");
    const std::string pom  = readFileAnywhere(root, "pom.xml");
    const std::string pyproject = readFileAnywhere(root, "pyproject.toml");
    const std::string mainCpp   = readFileAnywhere(root, "src/main.cpp");
    const std::string cargoToml = readFileAnywhere(root, "Cargo.toml");
    const std::string gomod     = readFileAnywhere(root, "go.mod");
    const std::string gradle    = readFileAnywhere(root, "build.gradle");

    // ── Node.js ──────────────────────────────────────────────────────────

    // Express
    if (contains(pkg, "\"express\"")) {
        out.backendFrameworks.push_back(item("Express.js", 95,
            "express dependency found in package.json"));
    }

    // NestJS
    if (contains(pkg, "@nestjs/core") || contains(pkg, "\"@nestjs/core\"")) {
        out.backendFrameworks.push_back(item("NestJS", 97,
            "@nestjs/core found in package.json"));
    }

    // Fastify
    if (contains(pkg, "\"fastify\"")) {
        out.backendFrameworks.push_back(item("Fastify", 94,
            "fastify dependency found in package.json"));
    }

    // Koa
    if (contains(pkg, "\"koa\"")) {
        out.backendFrameworks.push_back(item("Koa.js", 93,
            "koa dependency found in package.json"));
    }

    // Hono
    if (contains(pkg, "\"hono\"")) {
        out.backendFrameworks.push_back(item("Hono", 93,
            "hono dependency found in package.json"));
    }

    // ── Python ────────────────────────────────────────────────────────────

    auto detectPyReq = [&](const std::string& name, const std::string& pkgName,
                            int conf, const std::string& reason) {
        if (requirementContains(req, pkgName) ||
            requirementContains(req2, pkgName) ||
            requirementContains(readFile(root, "pyproject.toml"), pkgName)) {
            out.backendFrameworks.push_back(item(name, conf, reason));
        }
    };

    detectPyReq("Flask",    "flask",   95, "flask in requirements.txt");
    detectPyReq("Django",   "django",  95, "django in requirements.txt");
    detectPyReq("FastAPI",  "fastapi", 95, "fastapi in requirements.txt");
    detectPyReq("Starlette","starlette",88,"starlette in requirements.txt");
    detectPyReq("Tornado",  "tornado", 85, "tornado in requirements.txt");
    detectPyReq("Sanic",    "sanic",   88, "sanic in requirements.txt");

    // ── C++ ───────────────────────────────────────────────────────────────

    // Drogon
    if (contains(cmake, "find_package(Drogon") ||
        contains(cmake, "find_package(drogon") ||
        contains(mainCpp, "#include <drogon/drogon.h>")) {
        out.backendFrameworks.push_back(item("Drogon", 97,
            "find_package(Drogon) in CMakeLists.txt"));
    }

    // Crow
    if (anyFileExists(root, {"crow.h","include/crow.h"}) ||
        contains(cmake, "Crow") || contains(cmake, "crow")) {
        out.backendFrameworks.push_back(item("Crow", 80,
            "Crow C++ web framework detected"));
    }

    // Qt
    if (contains(cmake, "find_package(Qt6") ||
        contains(cmake, "find_package(Qt5") ||
        contains(cmake, "find_package(Qt)")) {
        int conf = contains(cmake, "find_package(Qt6") ? 97 : 90;
        out.backendFrameworks.push_back(item("Qt", conf,
            "find_package(Qt) in CMakeLists.txt"));
    }

    // Boost.Beast / Asio
    if (contains(cmake, "Boost") &&
        (contains(cmake, "beast") || contains(cmake, "asio"))) {
        out.backendFrameworks.push_back(item("Boost.Beast/Asio", 82,
            "Boost.Beast or Asio detected in CMakeLists.txt"));
    }

    // ── Java ──────────────────────────────────────────────────────────────

    // Spring Boot
    if (contains(pom, "spring-boot-starter") ||
        contains(readFile(root, "build.gradle"), "org.springframework.boot")) {
        out.backendFrameworks.push_back(item("Spring Boot", 96,
            "spring-boot-starter in pom.xml or build.gradle"));
    }

    // Micronaut
    if (contains(pom, "io.micronaut") ||
        contains(gradle, "io.micronaut")) {
        out.backendFrameworks.push_back(item("Micronaut", 90,
            "io.micronaut detected in build configuration"));
    }

    // Quarkus
    if (contains(pom, "io.quarkus")) {
        out.backendFrameworks.push_back(item("Quarkus", 90,
            "io.quarkus in pom.xml"));
    }

    // ── Go ────────────────────────────────────────────────────────────────
    if (!gomod.empty()) {
        if (contains(gomod, "github.com/gin-gonic/gin"))
            out.backendFrameworks.push_back(item("Gin", 95, "go.mod gin-gonic dependency"));
        if (contains(gomod, "github.com/labstack/echo"))
            out.backendFrameworks.push_back(item("Echo", 95, "go.mod labstack/echo dependency"));
        if (contains(gomod, "github.com/gofiber/fiber"))
            out.backendFrameworks.push_back(item("Fiber", 95, "go.mod gofiber/fiber dependency"));
        if (contains(gomod, "github.com/gorilla/mux"))
            out.backendFrameworks.push_back(item("Gorilla Mux", 90, "go.mod gorilla/mux dependency"));
    }

    // ── Rust ──────────────────────────────────────────────────────────────
    // cargoToml already read above
    if (!cargoToml.empty()) {
        if (contains(cargoToml, "actix-web"))
            out.backendFrameworks.push_back(item("Actix Web", 95, "actix-web in Cargo.toml"));
        if (contains(cargoToml, "axum"))
            out.backendFrameworks.push_back(item("Axum", 95, "axum in Cargo.toml"));
        if (contains(cargoToml, "rocket"))
            out.backendFrameworks.push_back(item("Rocket", 95, "rocket in Cargo.toml"));
        if (contains(cargoToml, "warp"))
            out.backendFrameworks.push_back(item("Warp", 90, "warp in Cargo.toml"));
    }
}

// ─── ML frameworks ───────────────────────────────────────────────────────────

static void detectMLFrameworks(const fs::path& root,
                                TechnologyAnalysis& out) noexcept
{
    const std::string req = readFile(root, "requirements.txt");
    const std::string pkg = readFile(root, "package.json");

    auto addML = [&](const std::string& name, bool found) {
        if (found) out.frameworks.push_back(item(name, 92, name + " dependency detected", "ml"));
    };

    addML("TensorFlow",  requirementContains(req, "tensorflow") || contains(pkg, "\"@tensorflow/tfjs\""));
    addML("PyTorch",     requirementContains(req, "torch"));
    addML("scikit-learn",requirementContains(req, "scikit-learn") || requirementContains(req, "sklearn"));
    addML("NumPy",       requirementContains(req, "numpy"));
    addML("Pandas",      requirementContains(req, "pandas"));
    addML("Keras",       requirementContains(req, "keras"));
    addML("Hugging Face",requirementContains(req, "transformers") || requirementContains(req, "datasets"));
    addML("LangChain",   requirementContains(req, "langchain"));
    addML("OpenAI SDK",  requirementContains(req, "openai") || contains(pkg, "\"openai\""));
}

// ─── Build systems ────────────────────────────────────────────────────────────

static void detectBuildSystems(const fs::path& root,
                                TechnologyAnalysis& out) noexcept
{
    if (fileExistsAnywhere(root, {"CMakeLists.txt"}))
        out.buildSystems.push_back(item("CMake", 98, "CMakeLists.txt present"));

    if (fileExistsAnywhere(root, {"Makefile","makefile","GNUmakefile"}))
        out.buildSystems.push_back(item("Make", 95, "Makefile detected"));

    if (fileExistsAnywhere(root, {"meson.build","meson_options.txt"}))
        out.buildSystems.push_back(item("Meson", 97, "meson.build detected"));

    if (fileExistsAnywhere(root, {"BUILD","WORKSPACE","BUILD.bazel","WORKSPACE.bazel"}))
        out.buildSystems.push_back(item("Bazel", 95, "BUILD/WORKSPACE file detected"));

    if (fileExistsAnywhere(root, {"Cargo.toml"}))
        out.buildSystems.push_back(item("Cargo", 98, "Cargo.toml present"));

    if (fileExistsAnywhere(root, {"build.gradle","build.gradle.kts"}))
        out.buildSystems.push_back(item("Gradle", 97, "build.gradle detected"));

    if (fileExistsAnywhere(root, {"pom.xml"}))
        out.buildSystems.push_back(item("Maven", 98, "pom.xml detected"));

    if (fileExistsAnywhere(root, {"package.json"}))
        out.buildSystems.push_back(item("npm scripts", 85, "package.json present"));

    if (fileExistsAnywhere(root, {"SConstruct","SConscript"}))
        out.buildSystems.push_back(item("SCons", 93, "SConstruct/SConscript detected"));

    if (fileExistsAnywhere(root, {"setup.py","setup.cfg"}))
        out.buildSystems.push_back(item("setuptools", 90, "setup.py/setup.cfg detected"));

    if (fileExistsAnywhere(root, {"pyproject.toml"}))
        out.buildSystems.push_back(item("Poetry / PEP 517", 88, "pyproject.toml detected"));

    if (fileExistsAnywhere(root, {"justfile","Justfile"}))
        out.buildSystems.push_back(item("Just", 85, "Justfile detected"));
}

// ─── Package managers ────────────────────────────────────────────────────────

static void detectPackageManagers(const fs::path& root,
                                   TechnologyAnalysis& out) noexcept
{
    if (fileExistsAnywhere(root, {"pnpm-lock.yaml"}))
        out.packageManagers.push_back(item("pnpm", 98, "pnpm-lock.yaml detected"));
    else if (fileExistsAnywhere(root, {"yarn.lock"}))
        out.packageManagers.push_back(item("Yarn", 98, "yarn.lock detected"));
    else if (fileExistsAnywhere(root, {"package-lock.json"}))
        out.packageManagers.push_back(item("npm", 98, "package-lock.json detected"));
    else if (fileExistsAnywhere(root, {"bun.lockb","bun.lock"}))
        out.packageManagers.push_back(item("Bun", 97, "bun.lock detected"));

    if (fileExistsAnywhere(root, {"poetry.lock"}))
        out.packageManagers.push_back(item("Poetry", 98, "poetry.lock detected"));
    else if (fileExistsAnywhere(root, {"Pipfile.lock","Pipfile"}))
        out.packageManagers.push_back(item("Pipenv", 95, "Pipfile detected"));
    else if (fileExistsAnywhere(root, {"requirements.txt"}))
        out.packageManagers.push_back(item("pip", 85, "requirements.txt detected"));

    if (fileExistsAnywhere(root, {"Cargo.lock"}))
        out.packageManagers.push_back(item("Cargo", 98, "Cargo.lock detected"));

    if (fileExistsAnywhere(root, {"go.sum"}))
        out.packageManagers.push_back(item("Go Modules", 98, "go.sum detected"));

    if (fileExistsAnywhere(root, {"conanfile.txt","conanfile.py"}))
        out.packageManagers.push_back(item("Conan", 96, "conanfile.txt/py detected"));

    if (fileExistsAnywhere(root, {"vcpkg.json","vcpkg.txt"}))
        out.packageManagers.push_back(item("vcpkg", 96, "vcpkg.json detected"));

    if (fileExistsAnywhere(root, {"Gemfile","Gemfile.lock"}))
        out.packageManagers.push_back(item("Bundler", 97, "Gemfile detected"));

    if (fileExistsAnywhere(root, {"composer.json"}))
        out.packageManagers.push_back(item("Composer", 97, "composer.json detected"));
}

// ─── Testing frameworks ───────────────────────────────────────────────────────

static void detectTestingFrameworks(const fs::path& root,
                                     TechnologyAnalysis& out) noexcept
{
    const std::string cmake = readFileAnywhere(root, "CMakeLists.txt");
    const std::string pkg   = readFileAnywhere(root, "package.json");
    const std::string req   = readFileAnywhere(root, "requirements.txt");
    const std::string cargo = readFileAnywhere(root, "Cargo.toml");
    const std::string pom   = readFileAnywhere(root, "pom.xml");

    // C++ testing
    if (contains(cmake, "GTest") || contains(cmake, "gtest") ||
        contains(cmake, "GoogleTest") || contains(cmake, "google/googletest"))
        out.testingFrameworks.push_back(item("GoogleTest", 93, "GTest found in CMakeLists.txt"));

    if (contains(cmake, "Catch2") || contains(cmake, "catch2"))
        out.testingFrameworks.push_back(item("Catch2", 93, "Catch2 found in CMakeLists.txt"));

    if (contains(cmake, "Boost.Test") || contains(cmake, "unit_test_framework"))
        out.testingFrameworks.push_back(item("Boost.Test", 88, "Boost.Test in CMakeLists.txt"));

    if (contains(cmake, "doctest"))
        out.testingFrameworks.push_back(item("doctest", 88, "doctest in CMakeLists.txt"));

    // JavaScript testing
    auto hasPkg = [&](const std::string& name) {
        return contains(pkg, "\"" + name + "\"");
    };

    if (hasPkg("vitest"))
        out.testingFrameworks.push_back(item("Vitest", 95, "vitest in package.json"));
    if (hasPkg("jest") || hasPkg("@jest/core"))
        out.testingFrameworks.push_back(item("Jest", 95, "jest in package.json"));
    if (hasPkg("mocha"))
        out.testingFrameworks.push_back(item("Mocha", 93, "mocha in package.json"));
    if (hasPkg("cypress"))
        out.testingFrameworks.push_back(item("Cypress", 93, "cypress in package.json"));
    if (hasPkg("playwright") || hasPkg("@playwright/test"))
        out.testingFrameworks.push_back(item("Playwright", 93, "playwright in package.json"));
    if (hasPkg("jasmine"))
        out.testingFrameworks.push_back(item("Jasmine", 90, "jasmine in package.json"));
    if (hasPkg("@testing-library/react"))
        out.testingFrameworks.push_back(item("React Testing Library", 92, "RTL in package.json"));

    // Python testing
    if (requirementContains(req, "pytest"))
        out.testingFrameworks.push_back(item("pytest", 95, "pytest in requirements.txt"));
    if (requirementContains(req, "unittest2"))
        out.testingFrameworks.push_back(item("unittest", 85, "unittest2 in requirements.txt"));

    // Java testing
    if (contains(pom, "junit") || contains(pom, "JUnit"))
        out.testingFrameworks.push_back(item("JUnit", 93, "JUnit in pom.xml"));
    if (contains(pom, "mockito"))
        out.testingFrameworks.push_back(item("Mockito", 90, "mockito in pom.xml"));

    // Rust testing (cargo built-in + tokio-test)
    if (!cargo.empty() && contains(cargo, "tokio"))
        out.testingFrameworks.push_back(item("Tokio Test", 82, "tokio detected (async tests)"));
}

// ─── CI/CD systems ────────────────────────────────────────────────────────────

static void detectCISystems(const fs::path& root,
                              TechnologyAnalysis& out) noexcept
{
    if (hasGitHubWorkflow(root))
        out.ciSystems.push_back(item("GitHub Actions", 98, ".github/workflows/*.yml detected"));

    if (fileExists(root, ".gitlab-ci.yml"))
        out.ciSystems.push_back(item("GitLab CI", 98, ".gitlab-ci.yml detected"));

    if (fileExists(root, ".circleci/config.yml"))
        out.ciSystems.push_back(item("CircleCI", 98, ".circleci/config.yml detected"));

    if (fileExists(root, "azure-pipelines.yml"))
        out.ciSystems.push_back(item("Azure Pipelines", 98, "azure-pipelines.yml detected"));

    if (fileExists(root, "Jenkinsfile") || fileExists(root, "jenkins.yml"))
        out.ciSystems.push_back(item("Jenkins", 97, "Jenkinsfile detected"));

    if (fileExists(root, ".travis.yml"))
        out.ciSystems.push_back(item("Travis CI", 98, ".travis.yml detected"));

    if (fileExists(root, "bitbucket-pipelines.yml"))
        out.ciSystems.push_back(item("Bitbucket Pipelines", 97, "bitbucket-pipelines.yml detected"));

    if (fileExists(root, ".drone.yml") || fileExists(root, ".drone.yaml"))
        out.ciSystems.push_back(item("Drone CI", 97, ".drone.yml detected"));
}

// ─── Containers ───────────────────────────────────────────────────────────────

static void detectContainers(const fs::path& root,
                               TechnologyAnalysis& out) noexcept
{
    if (anyFileExists(root, {"Dockerfile","dockerfile","Dockerfile.dev","Dockerfile.prod"}))
        out.containers.push_back(item("Docker", 98, "Dockerfile detected"));

    if (fileExists(root, "docker-compose.yml") || fileExists(root, "docker-compose.yaml") ||
        fileExists(root, "docker-compose.dev.yml") || fileExists(root, "docker-compose.prod.yml"))
        out.containers.push_back(item("Docker Compose", 98, "docker-compose.yml detected"));

    if (dirExists(root, "k8s") || dirExists(root, "kubernetes") ||
        dirExists(root, "manifests") || fileExists(root, "k8s.yaml") ||
        fileExists(root, "deployment.yaml"))
        out.containers.push_back(item("Kubernetes", 85, "k8s/ or deployment.yaml detected"));

    if (dirExists(root, "helm") || dirExists(root, "charts") ||
        fileExists(root, "Chart.yaml"))
        out.containers.push_back(item("Helm", 90, "helm/ or Chart.yaml detected"));

    if (fileExists(root, "skaffold.yaml") || fileExists(root, "skaffold.yml"))
        out.containers.push_back(item("Skaffold", 92, "skaffold.yaml detected"));
}

// ─── Cloud providers ─────────────────────────────────────────────────────────

static void detectCloudProviders(const fs::path& root,
                                  TechnologyAnalysis& out) noexcept
{
    if (fileExists(root, "serverless.yml") || fileExists(root, "serverless.yaml"))
        out.cloudProviders.push_back(item("Serverless Framework", 92, "serverless.yml detected"));

    if (fileExists(root, "template.yaml") || fileExists(root, "template.yml")) {
        auto content = readFile(root, "template.yaml");
        if (contains(content, "AWSTemplateFormatVersion") || contains(content, "AWS::"))
            out.cloudProviders.push_back(item("AWS SAM / CloudFormation", 90,
                "template.yaml with AWS::* resources"));
    }

    if (anyFileExists(root, {"terraform.tf","main.tf","variables.tf"}))
        out.cloudProviders.push_back(item("Terraform", 95, ".tf files detected"));

    if (fileExists(root, ".pulumi") || dirExists(root, ".pulumi"))
        out.cloudProviders.push_back(item("Pulumi", 90, ".pulumi directory detected"));

    if (fileExists(root, "render.yaml") || fileExists(root, "render.yml"))
        out.cloudProviders.push_back(item("Render", 95, "render.yaml detected"));

    if (fileExists(root, "vercel.json") || fileExists(root, ".vercel"))
        out.cloudProviders.push_back(item("Vercel", 95, "vercel.json detected"));

    if (fileExists(root, "netlify.toml") || fileExists(root, ".netlify"))
        out.cloudProviders.push_back(item("Netlify", 95, "netlify.toml detected"));

    if (fileExists(root, "fly.toml"))
        out.cloudProviders.push_back(item("Fly.io", 97, "fly.toml detected"));

    if (fileExists(root, "railway.json") || fileExists(root, "railway.toml"))
        out.cloudProviders.push_back(item("Railway", 97, "railway.json detected"));

    if (fileExists(root, "app.yaml")) {
        auto content = readFile(root, "app.yaml");
        if (containsCI(content, "runtime:"))
            out.cloudProviders.push_back(item("Google App Engine", 80, "app.yaml detected"));
    }
}

// ─── Databases ────────────────────────────────────────────────────────────────

static void detectDatabases(const fs::path& root,
                              TechnologyAnalysis& out) noexcept
{
    const std::string pkg   = readFileAnywhere(root, "package.json");
    const std::string req   = readFileAnywhere(root, "requirements.txt");
    const std::string cmake = readFileAnywhere(root, "CMakeLists.txt");
    const std::string cargo = readFileAnywhere(root, "Cargo.toml");
    const std::string pom   = readFileAnywhere(root, "pom.xml");
    const std::string gomod = readFileAnywhere(root, "go.mod");

    auto db = [&](const std::string& name, bool found, const std::string& reason) {
        if (found) out.databases.push_back(item(name, 88, reason, "database"));
    };

    // MySQL
    db("MySQL",
       contains(pkg,"mysql") || contains(pkg,"mysql2") ||
       requirementContains(req,"mysql") || requirementContains(req,"pymysql") ||
       contains(cmake,"mysqlcppconn") || contains(pom,"mysql-connector"),
       "MySQL driver detected");

    // PostgreSQL
    db("PostgreSQL",
       contains(pkg,"pg") || contains(pkg,"postgres") ||
       requirementContains(req,"psycopg") || requirementContains(req,"asyncpg") ||
       contains(cmake,"libpq") || contains(pom,"postgresql"),
       "PostgreSQL driver detected");

    // SQLite
    db("SQLite",
       contains(pkg,"better-sqlite3") || contains(pkg,"sqlite3") ||
       requirementContains(req,"sqlite") ||
       contains(cmake,"SQLite") || contains(cmake,"sqlite3"),
       "SQLite driver detected");

    // MongoDB
    db("MongoDB",
       contains(pkg,"mongodb") || contains(pkg,"mongoose") ||
       requirementContains(req,"pymongo") || requirementContains(req,"motor"),
       "MongoDB driver detected");

    // Redis
    db("Redis",
       contains(pkg,"redis") || contains(pkg,"ioredis") ||
       requirementContains(req,"redis") || requirementContains(req,"aioredis"),
       "Redis driver detected");

    // Prisma ORM
    db("Prisma", fileExistsAnywhere(root, {"prisma/schema.prisma"}), "prisma/schema.prisma detected");

    // Drizzle ORM
    db("Drizzle ORM", contains(pkg,"drizzle-orm"), "drizzle-orm in package.json");

    // TypeORM
    db("TypeORM", contains(pkg,"typeorm"), "typeorm in package.json");

    // Sequelize
    db("Sequelize", contains(pkg,"sequelize"), "sequelize in package.json");

    // SQLAlchemy
    db("SQLAlchemy", requirementContains(req,"sqlalchemy"), "SQLAlchemy in requirements.txt");

    // Diesel (Rust)
    db("Diesel", contains(cargo,"diesel"), "diesel in Cargo.toml");

    // sqlx (Rust)
    db("sqlx", contains(cargo,"sqlx"), "sqlx in Cargo.toml");

    // Spring Data JPA
    db("Spring Data JPA", contains(pom,"spring-data-jpa"), "spring-data-jpa in pom.xml");

    // GORM (Go)
    db("GORM", contains(gomod,"gorm.io/gorm"), "gorm.io/gorm in go.mod");
}

// ─── Documentation detection ─────────────────────────────────────────────────

static DocumentationStatus detectDocumentation(const fs::path& root) noexcept
{
    DocumentationStatus d;
    d.readme       = anyFileExists(root, {"README.md","README.rst","README.txt","README","readme.md"});
    d.license      = anyFileExists(root, {"LICENSE","LICENSE.md","LICENSE.txt","LICENCE","COPYING"});
    d.changelog    = anyFileExists(root, {"CHANGELOG.md","CHANGELOG.txt","CHANGELOG","HISTORY.md"});
    d.contributing = anyFileExists(root, {"CONTRIBUTING.md","CONTRIBUTING.txt","CONTRIBUTING"});
    d.security     = anyFileExists(root, {"SECURITY.md","SECURITY.txt","SECURITY"});
    d.codeOfConduct = anyFileExists(root,
        {"CODE_OF_CONDUCT.md","CODE_OF_CONDUCT.txt",".github/CODE_OF_CONDUCT.md"});
    return d;
}

// ─── Repository type inference ────────────────────────────────────────────────

static std::string inferRepositoryType(const TechnologyAnalysis& a) noexcept
{
    int frontend = 0, backend = 0, desktop = 0, ml = 0, embedded = 0;
    int game = 0, mobile = 0, library = 0;

    for (auto& f : a.frontendFrameworks) {
        (void)f;
        ++frontend;
    }
    for (auto& f : a.backendFrameworks) {
        (void)f;
        ++backend;
    }
    for (auto& f : a.frameworks) {
        if (f.category == "ml")    ++ml;
    }

    // Desktop signals
    for (auto& f : a.backendFrameworks) {
        if (f.name == "Qt" || f.name == "Electron" || f.name == "Tauri")
            ++desktop;
    }

    const std::string cmake = "";  // already read — just use counts

    // CMakeLists without web frameworks suggests C++ library/app
    bool hasCMake    = !a.buildSystems.empty() &&
                       std::any_of(a.buildSystems.begin(), a.buildSystems.end(),
                           [](auto& x){ return x.name == "CMake"; });
    bool hasCargo    = !a.buildSystems.empty() &&
                       std::any_of(a.buildSystems.begin(), a.buildSystems.end(),
                           [](auto& x){ return x.name == "Cargo"; });

    if (ml >= 2)                                return "Machine Learning";
    if (desktop >= 1)                           return "Desktop Application";
    if (frontend >= 1 && backend >= 1)          return "Full-Stack Application";
    if (frontend >= 1 && backend == 0)          return "Frontend SPA";
    if (backend  >= 1 && frontend == 0)         return "Backend API";
    if (hasCMake && backend == 0 && frontend == 0)
                                                return "C++ Application";
    if (hasCargo && backend == 0 && frontend == 0)
                                                return "Rust Application";
    if (!a.ciSystems.empty() && backend == 0 && frontend == 0)
                                                return "CLI Application";
    if (!a.databases.empty() && backend == 0)   return "Backend API";
    if (ml >= 1)                                return "Machine Learning";

    return "Unknown";
}

// ─── Confidence score ─────────────────────────────────────────────────────────

static int calculateConfidenceScore(const TechnologyAnalysis& a) noexcept
{
    std::vector<int> scores;
    auto collect = [&](const std::vector<TechnologyItem>& v) {
        for (auto& i : v) scores.push_back(i.confidence);
    };
    collect(a.frontendFrameworks);
    collect(a.backendFrameworks);
    collect(a.frameworks);
    collect(a.buildSystems);
    collect(a.packageManagers);
    collect(a.testingFrameworks);
    collect(a.ciSystems);
    collect(a.containers);
    collect(a.cloudProviders);
    collect(a.databases);

    if (scores.empty()) return 50;
    int total = std::accumulate(scores.begin(), scores.end(), 0);
    return total / static_cast<int>(scores.size());
}

// ─── Service implementation ───────────────────────────────────────────────────

std::optional<TechnologyAnalysis> TechnologyService::detectAndStore(
    const std::string& jobId,
    const std::string& clonePath) noexcept
{
    auto& log = Logger::instance();

    try {
        const auto startTime = std::chrono::steady_clock::now();
        log.info("Technology detection started for job=" + jobId
                 + " path=" + clonePath);

        fs::path root(clonePath);
        if (!fs::is_directory(root)) {
            log.warn("TechnologyService: path not found: " + clonePath);
            return std::nullopt;
        }

        TechnologyAnalysis result;
        result.jobId = jobId;
        result.detectedAt = std::chrono::system_clock::now();

        // Run all detectors
        detectFrontendFrameworks(root, result);
        detectBackendFrameworks(root, result);
        detectMLFrameworks(root, result);
        detectBuildSystems(root, result);
        detectPackageManagers(root, result);
        detectTestingFrameworks(root, result);
        detectCISystems(root, result);
        detectContainers(root, result);
        detectCloudProviders(root, result);
        detectDatabases(root, result);

        // Merge frontend/backend into general frameworks list
        for (auto& f : result.frontendFrameworks) result.frameworks.push_back(f);
        for (auto& f : result.backendFrameworks)  result.frameworks.push_back(f);

        // Post-process
        result.documentation    = detectDocumentation(root);
        result.repositoryType   = inferRepositoryType(result);
        result.confidenceScore  = calculateConfidenceScore(result);

        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startTime).count();

        log.info("Technology detection completed for job=" + jobId
                 + " type=" + result.repositoryType
                 + " frameworks=" + std::to_string(result.frameworks.size())
                 + " confidence=" + std::to_string(result.confidenceScore)
                 + " elapsed=" + std::to_string(elapsed) + "ms");

        if (repository_) {
            repository_->save(result);
            log.info("Technology analysis stored for job=" + jobId);
        }

        return result;

    } catch (const std::exception& e) {
        log.error(std::string("TechnologyService::detectAndStore exception: ") + e.what());
        return std::nullopt;
    } catch (...) {
        log.error("TechnologyService::detectAndStore unknown exception");
        return std::nullopt;
    }
}

std::optional<TechnologyAnalysis> TechnologyService::getTechnology(
    const std::string& jobId) const noexcept
{
    if (!repository_) return std::nullopt;
    return repository_->findByJobId(jobId);
}

} // namespace cortex::technology
