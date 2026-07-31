/**
 * @file RepositoryInsightService.cpp
 * @brief Deterministic insight generation from structured analysis data.
 * @project Cortex Code Intelligence Platform
 * @author Kartick Kumar Ghosh
 * @github https://github.com/gavinspet
 * @email kartick.ghosh.dev@gmail.com
 * @copyright © 2026
 * @license MIT
 *
 * InsightGenerator (internal class)
 * ─────────────────────────────────
 * Accepts four structured inputs (AnalysisResult, GitHubMetadata,
 * TechnologyAnalysis, RepositoryHealthResult) and produces every output
 * field of RepositoryInsightResult by pure rule-based template logic.
 *
 * No AI is used. No external API is called. All output is deterministic
 * and reproducible given the same inputs.
 */
#include "insight/RepositoryInsightService.h"
#include "logging/Logger.h"
#include <sstream>
#include <algorithm>
#include <chrono>
#include <numeric>

namespace cortex::insight {

using cortex::logging::Logger;

// ─── InsightGenerator (internal) ─────────────────────────────────────────────

namespace {

/**
 * @class InsightGenerator
 * @brief Stateless rule engine that transforms structured data into prose.
 *
 * All member functions are const. The generator holds raw const-pointers to
 * avoid copies — the inputs are owned by the caller and outlive the generator.
 */
class InsightGenerator {
public:
    InsightGenerator(
        const cortex::domain::AnalysisResult*         analysis,
        const cortex::github::GitHubMetadata*         metadata,
        const cortex::technology::TechnologyAnalysis* tech,
        const cortex::health::RepositoryHealthResult* health) noexcept
        : analysis_(analysis), metadata_(metadata), tech_(tech), health_(health)
    {}

    RepositoryInsightResult generate(const std::string& jobId) const noexcept;

private:
    const cortex::domain::AnalysisResult*         analysis_;
    const cortex::github::GitHubMetadata*         metadata_;
    const cortex::technology::TechnologyAnalysis* tech_;
    const cortex::health::RepositoryHealthResult* health_;

    // ── Classifiers ────────────────────────────────────────────────────────
    std::string inferProjectSize()     const noexcept;
    std::string inferProjectMaturity() const noexcept;
    std::string inferComplexity()      const noexcept;

    // ── Narrative generators ───────────────────────────────────────────────
    std::string generateSummary()           const noexcept;
    std::string generateTechnologyOverview()const noexcept;
    std::string generateQualityOverview()   const noexcept;

    // ── Enumerated generators ──────────────────────────────────────────────
    std::vector<std::string> generateStrengths()   const noexcept;
    std::vector<std::string> generateRisks()       const noexcept;
    std::vector<std::string> generateSuggestions() const noexcept;

    // ── Convenience accessors ──────────────────────────────────────────────
    long long loc() const noexcept { return analysis_ ? analysis_->totalLines : 0; }
    int  fileCount() const noexcept { return analysis_ ? analysis_->fileCount : 0; }
    int  healthScore() const noexcept { return health_ ? health_->overallScore : 0; }
    bool hasCi() const noexcept {
        return tech_ && !tech_->ciSystems.empty();
    }
    bool hasTesting() const noexcept {
        return health_ && health_->categories.testing.score >= 10;
    }
    bool hasTestDir() const noexcept {
        return health_ && health_->categories.testing.score >= 5;
    }
    bool hasAllDocs() const noexcept {
        return health_ && health_->categories.documentation.score >= 18;
    }
    bool hasLicense() const noexcept {
        return health_ && health_->categories.documentation.score >= 4;
    }
    bool hasDocker() const noexcept {
        return tech_ && !tech_->containers.empty();
    }
    bool hasDependabot() const noexcept {
        return health_ && health_->categories.security.score >= 8;
    }
    int techCount() const noexcept {
        if (!tech_) return 0;
        return static_cast<int>(
            tech_->frameworks.size() +
            tech_->buildSystems.size() +
            tech_->ciSystems.size() +
            tech_->containers.size() +
            tech_->databases.size());
    }
    std::string repoType() const noexcept {
        return tech_ ? tech_->repositoryType : "Unknown";
    }
    std::string primaryLang() const noexcept {
        if (metadata_ && !metadata_->primaryLanguage.empty())
            return metadata_->primaryLanguage;
        if (tech_ && !tech_->backendFrameworks.empty())
            return tech_->backendFrameworks[0].name;
        if (tech_ && !tech_->frontendFrameworks.empty())
            return tech_->frontendFrameworks[0].name;
        return "";
    }

    // Join a list of technology names (up to maxItems)
    static std::string joinNames(
        const std::vector<cortex::technology::TechnologyItem>& items,
        int maxItems = 3) noexcept
    {
        std::ostringstream ss;
        int n = std::min(static_cast<int>(items.size()), maxItems);
        for (int i = 0; i < n; ++i) {
            if (i > 0) ss << (i == n - 1 ? " and " : ", ");
            ss << items[i].name;
        }
        if (static_cast<int>(items.size()) > maxItems)
            ss << " and " << (items.size() - maxItems) << " more";
        return ss.str();
    }
};

// ─── Classifiers ─────────────────────────────────────────────────────────────

/**
 * Project size classification
 * ─────────────────────────────────────────────────────────────────────────────
 * Tier        LOC           Files     sizeKb
 * Tiny        <500          <30       —
 * Small       500-5000      30-150    <500
 * Medium      5000-25000    150-500   <5000
 * Large       25000-100000  500-2000  <50000
 * Enterprise  >=100000      >=2000    —
 */
std::string InsightGenerator::inferProjectSize() const noexcept
{
    const long long l = loc();
    const int f = fileCount();
    const long long kb = metadata_ ? metadata_->sizeKb : 0;

    if (l >= 100000 || f >= 2000) return "Enterprise";
    if (l >= 25000  || f >= 500)  return "Large";
    if (l >= 5000   || f >= 150)  return "Medium";
    if (l >= 500    || f >= 30)   return "Small";
    return "Tiny";
}

/**
 * Project maturity classification
 * ─────────────────────────────────────────────────────────────────────────────
 * Level              Health  CI   Docs   Licence  Tests  Stars/forks
 * Prototype          <30     ✗    ✗      ✗        ✗
 * Personal Project   30-50   ✗    ✓      ✗        ✗
 * Production Ready   50-75   ✓    ✓      ✓        ✗ or ✓
 * Open Source Lib    65+     ✓    ✓      ✓        ✓      or stars>10
 * Enterprise Grade   85+     ✓    ✓      ✓        ✓      and all security
 */
std::string InsightGenerator::inferProjectMaturity() const noexcept
{
    const int hs = healthScore();
    const bool ci    = hasCi();
    const bool tests = hasTesting();
    const bool docs  = hasAllDocs();
    const bool lic   = hasLicense();
    const bool sec   = health_ && health_->categories.security.score >= 8;
    const int  stars = metadata_ ? metadata_->stars : 0;

    if (hs >= 85 && ci && tests && docs && sec)
        return "Enterprise Grade";
    if (hs >= 65 && ci && docs && lic && (tests || stars > 10))
        return "Open Source Library";
    if (hs >= 50 && ci && lic)
        return "Production Ready";
    if (hs >= 30 && docs)
        return "Personal Project";
    return "Prototype";
}

/**
 * Complexity classification
 * ─────────────────────────────────────────────────────────────────────────────
 * Level     Tech signals  LOC
 * Low       <=3           <2000
 * Medium    4-6           2000-25000
 * High      7-10          25000-100000
 * Very High >10           or >100000
 */
std::string InsightGenerator::inferComplexity() const noexcept
{
    const int tc = techCount();
    const long long l = loc();

    if (tc > 10 || l > 100000) return "Very High";
    if (tc > 6  || l > 25000)  return "High";
    if (tc > 3  || l > 2000)   return "Medium";
    return "Low";
}

// ─── Narrative generators ─────────────────────────────────────────────────────

std::string InsightGenerator::generateSummary() const noexcept
{
    std::ostringstream s;

    // Opening sentence: describe what it is
    const std::string type = repoType();
    const std::string lang = primaryLang();

    if (type == "Backend API") {
        s << "This repository contains a";
        if (!lang.empty()) s << " " << lang;
        s << " backend API";
    } else if (type == "Frontend SPA") {
        s << "This is a modern frontend single-page application (SPA)";
    } else if (type == "Full-Stack Application") {
        s << "This is a full-stack";
        if (!lang.empty()) s << " " << lang;
        s << " application";
    } else if (type == "C++ Application") {
        s << "This is a C++20 application";
    } else if (type == "Rust Application") {
        s << "This is a Rust application";
    } else if (type == "Desktop Application") {
        s << "This is a desktop application";
    } else if (type == "Machine Learning") {
        s << "This is a machine learning project";
    } else if (type == "CLI Application") {
        s << "This is a command-line application";
    } else {
        if (!lang.empty())
            s << "This is a " << lang << " project";
        else
            s << "This repository";
    }

    // Frameworks
    if (tech_) {
        std::vector<std::string> fwks;
        for (auto& f : tech_->backendFrameworks)  fwks.push_back(f.name);
        for (auto& f : tech_->frontendFrameworks) fwks.push_back(f.name);
        if (!fwks.empty()) {
            s << " built with ";
            for (size_t i = 0; i < std::min(fwks.size(), size_t(3)); ++i) {
                if (i > 0) s << (i == std::min(fwks.size(), size_t(3)) - 1 ? " and " : ", ");
                s << fwks[i];
            }
        }
        // Build system
        if (!tech_->buildSystems.empty())
            s << ", using " << tech_->buildSystems[0].name << " as its build system";
    }
    s << ". ";

    // GitHub presence
    if (metadata_) {
        if (!metadata_->description.empty())
            s << metadata_->description << ". ";
        s << "The repository is " << metadata_->visibility << " on GitHub";
        if (metadata_->stars > 0)
            s << " with " << metadata_->stars
              << " star" << (metadata_->stars != 1 ? "s" : "");
        if (metadata_->forks > 0)
            s << " and " << metadata_->forks
              << " fork" << (metadata_->forks != 1 ? "s" : "");
        s << ". ";
    }

    // Documentation quality
    if (health_) {
        int docPct = (health_->categories.documentation.maxScore > 0)
                     ? health_->categories.documentation.score * 100
                         / health_->categories.documentation.maxScore
                     : 0;
        if (docPct >= 90)      s << "Documentation is comprehensive. ";
        else if (docPct >= 60) s << "Documentation is partially complete. ";
        else                   s << "Documentation is minimal. ";
    }

    // CI/CD
    if (tech_ && !tech_->ciSystems.empty()) {
        s << tech_->ciSystems[0].name << " is configured";
        if (hasTesting()) s << " with automated testing";
        if (hasDocker())  s << " and Docker containerisation";
        s << ". ";
    } else {
        s << "No CI/CD pipeline has been detected. ";
    }

    // Testing
    if (!hasTesting()) {
        if (hasTestDir())
            s << "A test directory exists but no test framework has been detected.";
        else
            s << "No automated testing infrastructure has been detected.";
    }

    return s.str();
}

std::string InsightGenerator::generateTechnologyOverview() const noexcept
{
    if (!tech_) return "Technology stack information is not available.";
    std::ostringstream s;

    // Primary language
    const std::string lang = primaryLang();
    if (!lang.empty()) s << "Primary language: " << lang << ". ";

    // Frameworks
    std::vector<std::string> allFw;
    for (auto& f : tech_->backendFrameworks)  allFw.push_back(f.name);
    for (auto& f : tech_->frontendFrameworks) allFw.push_back(f.name);
    if (!allFw.empty()) {
        s << "Framework" << (allFw.size() > 1 ? "s" : "") << ": ";
        for (size_t i = 0; i < allFw.size(); ++i) {
            if (i > 0) s << (i == allFw.size() - 1 ? " and " : ", ");
            s << allFw[i];
        }
        s << ". ";
    }

    // Build & package manager
    if (!tech_->buildSystems.empty()) {
        s << "Build system: " << joinNames(tech_->buildSystems, 2) << ". ";
    }
    if (!tech_->packageManagers.empty()) {
        s << "Package manager: " << joinNames(tech_->packageManagers, 2) << ". ";
    }

    // Infrastructure
    std::vector<std::string> infra;
    if (!tech_->ciSystems.empty())   infra.push_back(joinNames(tech_->ciSystems, 1) + " (CI/CD)");
    if (!tech_->containers.empty())  infra.push_back(joinNames(tech_->containers, 1) + " (containers)");
    if (!tech_->databases.empty())   infra.push_back(joinNames(tech_->databases, 2) + " (database)");
    if (!infra.empty()) {
        s << "Infrastructure: ";
        for (size_t i = 0; i < infra.size(); ++i) {
            if (i > 0) s << ", ";
            s << infra[i];
        }
        s << ". ";
    }

    // Repository type
    s << "Repository type: " << tech_->repositoryType << ".";

    return s.str();
}

std::string InsightGenerator::generateQualityOverview() const noexcept
{
    std::ostringstream s;

    if (!health_) {
        s << "Quality data is not available.";
        return s.str();
    }

    // Overall score
    s << "Repository health score: " << health_->overallScore
      << "/100 (grade " << health_->grade << "). ";

    // Per-category highlights
    auto& cats = health_->categories;

    // Documentation
    if (cats.documentation.score == cats.documentation.maxScore)
        s << "Documentation is complete. ";
    else if (cats.documentation.score < 5)
        s << "Documentation is critically incomplete. ";

    // Testing
    if (cats.testing.score >= 15)
        s << "Testing infrastructure is solid. ";
    else if (cats.testing.score >= 10)
        s << "Test framework is present. ";
    else
        s << "No test framework has been detected. ";

    // CI/CD
    if (cats.ciCd.score >= 12)
        s << "CI/CD is active. ";
    else
        s << "No CI/CD pipeline detected. ";

    // Security
    if (cats.security.score >= 10)
        s << "Security practices are strong. ";
    else if (cats.security.score >= 4)
        s << "Basic security hygiene is in place. ";
    else
        s << "Security configuration is minimal. ";

    // Maturity
    s << "Project maturity is assessed as: " << inferProjectMaturity() << ".";

    return s.str();
}

// ─── Enumerated generators ────────────────────────────────────────────────────

std::vector<std::string> InsightGenerator::generateStrengths() const noexcept
{
    std::vector<std::string> result;

    // Documentation
    if (health_ && health_->categories.documentation.score == health_->categories.documentation.maxScore)
        result.push_back("Complete community documentation (README, LICENSE, CHANGELOG, "
                         "CONTRIBUTING, SECURITY, CODE_OF_CONDUCT)");
    else if (health_ && health_->categories.documentation.score >= 12)
        result.push_back("Good project documentation with major community files present");

    // CI/CD
    if (tech_ && !tech_->ciSystems.empty()) {
        std::string s = "Automated CI/CD pipeline configured (" + tech_->ciSystems[0].name + ")";
        if (tech_->ciSystems.size() > 1) s += " with multiple providers";
        result.push_back(s);
    }

    // Testing
    if (hasTesting())
        result.push_back("Automated test framework detected");
    else if (hasTestDir())
        result.push_back("Dedicated test directory present");

    // Docker
    if (hasDocker()) {
        std::string s = "Containerised deployment";
        if (tech_) {
            auto it = std::find_if(tech_->containers.begin(), tech_->containers.end(),
                                   [](auto& c){ return c.name == "Docker Compose"; });
            if (it != tech_->containers.end()) s += " with Docker Compose orchestration";
        }
        result.push_back(s);
    }

    // Cloud / deployment
    if (tech_ && !tech_->cloudProviders.empty())
        result.push_back("Cloud deployment configuration present (" +
                         tech_->cloudProviders[0].name + ")");

    // Maintainability
    if (health_ && health_->categories.maintainability.score >= 10)
        result.push_back("Code quality tooling configured (linting and formatting)");

    // Security
    if (hasDependabot())
        result.push_back("Automated dependency updates via Dependabot");

    // Multiple technologies
    if (techCount() >= 6)
        result.push_back("Modern, diverse technology stack with " +
                         std::to_string(techCount()) + " detected components");

    return result;
}

std::vector<std::string> InsightGenerator::generateRisks() const noexcept
{
    std::vector<std::string> result;

    // Testing
    if (!hasTesting())
        result.push_back("No automated testing — regressions may not be caught on code changes");

    // CI/CD
    if (!hasCi())
        result.push_back("No CI/CD pipeline — code quality is not automatically validated");

    // License
    if (!hasLicense())
        result.push_back("No LICENSE file — usage and redistribution rights are unclear");

    // Security
    if (!hasDependabot())
        result.push_back("No automated dependency updates — vulnerabilities may accumulate");

    if (health_ && health_->categories.security.score < 7)
        result.push_back("Security configuration is weak — consider adding SECURITY.md and CodeQL");

    // Docs
    if (health_ && health_->categories.documentation.score < 5)
        result.push_back("Missing README — project is difficult to discover and onboard contributors");

    // Maintainability
    if (health_ && health_->categories.maintainability.score < 6)
        result.push_back("No code quality tooling detected — code style may be inconsistent");

    // Size vs testing
    if (loc() > 10000 && !hasTesting())
        result.push_back("Large codebase (" + std::to_string(loc()) +
                         " LOC) with no test coverage is high risk");

    return result;
}

std::vector<std::string> InsightGenerator::generateSuggestions() const noexcept
{
    std::vector<std::string> result;

    // Testing
    if (!hasTesting()) {
        std::string suggestion = "Add automated tests";
        if (tech_ && !tech_->buildSystems.empty()) {
            const std::string& bs = tech_->buildSystems[0].name;
            if (bs == "CMake")
                suggestion += " using GoogleTest or Catch2 (integrate with CMake via FetchContent)";
            else if (bs == "npm scripts" || bs == "Vite / Webpack")
                suggestion += " using Vitest or Jest";
            else if (bs == "Cargo")
                suggestion += " using Rust's built-in test framework";
            else if (bs == "Gradle" || bs == "Maven")
                suggestion += " using JUnit 5";
        }
        result.push_back(suggestion);
    }

    // CI/CD
    if (!hasCi())
        result.push_back("Configure GitHub Actions (.github/workflows/ci.yml) for automated "
                         "build, test, and lint on every push");

    // Dependabot
    if (!hasDependabot())
        result.push_back("Enable Dependabot (.github/dependabot.yml) for automatic dependency "
                         "security updates");

    // Coverage
    if (hasTestDir() && health_ && health_->categories.testing.score < 13)
        result.push_back("Integrate code coverage reporting (lcov, nyc, pytest-cov) and "
                         "add a coverage badge to README");

    // CodeQL
    if (health_ && health_->categories.security.score < 7)
        result.push_back("Enable CodeQL security scanning via GitHub Actions for static "
                         "vulnerability detection");

    // CHANGELOG
    if (health_ && health_->categories.documentation.score < 12)
        result.push_back("Add CHANGELOG.md to document version history and help users "
                         "track breaking changes");

    // Maintainability
    if (health_ && health_->categories.maintainability.score < 7) {
        if (tech_ && !tech_->buildSystems.empty()) {
            const std::string& bs = tech_->buildSystems[0].name;
            if (bs == "CMake")
                result.push_back("Add .clang-format and integrate clang-tidy for consistent "
                                 "C++ code style");
            else if (bs == "npm scripts" || bs.find("npm") != std::string::npos)
                result.push_back("Add ESLint + Prettier for consistent JavaScript/TypeScript style");
            else
                result.push_back("Configure a linter and formatter appropriate for your language "
                                 "and add .editorconfig for editor consistency");
        } else {
            result.push_back("Configure a linter/formatter and add .editorconfig to enforce "
                             "consistent code style");
        }
    }

    // .env.example
    if (health_ && health_->categories.configuration.score < 10)
        result.push_back("Add .env.example to document required environment variables without "
                         "exposing secrets");

    // License
    if (!hasLicense())
        result.push_back("Add a LICENSE file to clarify usage rights for contributors and users");

    return result;
}

// ─── Main generation entry point ─────────────────────────────────────────────

RepositoryInsightResult InsightGenerator::generate(const std::string& jobId) const noexcept
{
    RepositoryInsightResult r;
    r.jobId                = jobId;
    r.generatedAt          = std::chrono::system_clock::now();
    r.estimatedProjectSize = inferProjectSize();
    r.estimatedMaturity    = inferProjectMaturity();
    r.estimatedComplexity  = inferComplexity();
    r.summary              = generateSummary();
    r.technologyOverview   = generateTechnologyOverview();
    r.qualityOverview      = generateQualityOverview();
    r.strengths            = generateStrengths();
    r.risks                = generateRisks();
    r.suggestions          = generateSuggestions();
    return r;
}

} // anonymous namespace

// ─── Service implementation ───────────────────────────────────────────────────

std::optional<RepositoryInsightResult> RepositoryInsightService::generateAndStore(
    const std::string& jobId,
    const cortex::domain::AnalysisResult*         analysis,
    const cortex::github::GitHubMetadata*         metadata,
    const cortex::technology::TechnologyAnalysis* tech,
    const cortex::health::RepositoryHealthResult* health) noexcept
{
    auto& log = Logger::instance();

    try {
        const auto startTime = std::chrono::steady_clock::now();
        log.info("Repository insight generation started for job=" + jobId);

        InsightGenerator gen(analysis, metadata, tech, health);
        auto result = gen.generate(jobId);

        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startTime).count();

        log.info("Repository insights generated for job=" + jobId
                 + " size=" + result.estimatedProjectSize
                 + " maturity=" + result.estimatedMaturity
                 + " complexity=" + result.estimatedComplexity
                 + " strengths=" + std::to_string(result.strengths.size())
                 + " risks=" + std::to_string(result.risks.size())
                 + " elapsed=" + std::to_string(elapsed) + "ms");

        if (repository_) {
            repository_->save(result);
            log.info("Repository insights stored for job=" + jobId);
        }

        return result;

    } catch (const std::exception& e) {
        log.error(std::string("RepositoryInsightService::generateAndStore exception: ") + e.what());
        return std::nullopt;
    } catch (...) {
        log.error("RepositoryInsightService::generateAndStore unknown exception");
        return std::nullopt;
    }
}

std::optional<RepositoryInsightResult> RepositoryInsightService::getInsights(
    const std::string& jobId) const noexcept
{
    if (!repository_) return std::nullopt;
    return repository_->findByJobId(jobId);
}

} // namespace cortex::insight
