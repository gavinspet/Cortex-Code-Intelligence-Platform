/**
 * @file RepositoryHealthService.h
 * @brief Orchestrates repository health and quality evaluation.
 * @project Cortex Code Intelligence Platform
 * @author Kartick Kumar Ghosh
 * @github https://github.com/gavinspet
 * @email kartick.ghosh.dev@gmail.com
 * @copyright © 2026
 * @license MIT
 */
#pragma once

#include "health/IRepositoryHealthRepository.h"
#include <memory>
#include <optional>
#include <string>

namespace cortex::health {

/**
 * @class RepositoryHealthService
 * @brief Static quality evaluation engine for cloned repositories.
 *
 * Evaluates 7 health categories using purely static file-system inspection:
 *
 *   Category           Max   Criteria (examples)
 *   ─────────────────────────────────────────────────────────────
 *   Documentation       20   README, LICENSE, CHANGELOG, SECURITY…
 *   Testing             20   test framework, test dir, coverage cfg
 *   CI/CD               15   GitHub Actions, GitLab CI, Jenkins…
 *   Security            15   SECURITY.md, Dependabot, CodeQL…
 *   Maintainability     15   linter, formatter, lockfile, .editorconfig
 *   Configuration       10   .gitignore, Docker, env template
 *   Project Structure    5   src/, tests/, docs/, include/, examples/
 *   ─────────────────────────────────────────────────────────────
 *   Total              100
 *
 * Grade scale: A(90–100) B(80–89) C(70–79) D(60–69) F(<60)
 *
 * No repository code is executed. All analysis is purely static.
 * Target performance: < 10 ms for medium repositories.
 */
class RepositoryHealthService {
public:
    explicit RepositoryHealthService(
        std::shared_ptr<IRepositoryHealthRepository> repository) noexcept
        : repository_(std::move(repository))
    {}

    /**
     * Evaluate health of a cloned repository and persist the result.
     *
     * @param jobId     Analysis job identifier
     * @param clonePath Absolute path to the cloned repository root
     * @return RepositoryHealthResult or std::nullopt on failure
     */
    [[nodiscard]] std::optional<RepositoryHealthResult> evaluateAndStore(
        const std::string& jobId,
        const std::string& clonePath) noexcept;

    /**
     * Retrieve a previously stored health evaluation.
     *
     * @param jobId Analysis job identifier
     * @return Stored result or std::nullopt
     */
    [[nodiscard]] std::optional<RepositoryHealthResult> getHealth(
        const std::string& jobId) const noexcept;

private:
    std::shared_ptr<IRepositoryHealthRepository> repository_;
};

} // namespace cortex::health
