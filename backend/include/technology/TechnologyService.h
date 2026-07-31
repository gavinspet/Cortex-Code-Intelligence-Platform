/**
 * @file TechnologyService.h
 * @brief Orchestrates static technology detection for a cloned repository.
 * @project Cortex Code Intelligence Platform
 * @author Kartick Kumar Ghosh
 * @github https://github.com/gavinspet
 * @email kartick.ghosh.dev@gmail.com
 * @copyright © 2026
 * @license MIT
 */
#pragma once

#include "technology/ITechnologyRepository.h"
#include <memory>
#include <optional>
#include <string>

namespace cortex::technology {

/**
 * @class TechnologyService
 * @brief Business-logic layer for repository technology detection.
 *
 * Detection strategy:
 * 1. Walk well-known file signatures (existence checks)
 * 2. Perform bounded content-pattern matching (≤64 KB per file)
 * 3. Infer repository type from detected technology signals
 * 4. Calculate per-item and overall confidence scores
 *
 * NO code from the repository is ever executed.
 * All analysis is purely static (file presence + content search).
 *
 * Injected via constructor — fully unit-testable.
 */
class TechnologyService {
public:
    explicit TechnologyService(
        std::shared_ptr<ITechnologyRepository> repository) noexcept
        : repository_(std::move(repository))
    {}

    /**
     * Run static detection on a cloned repository and persist the result.
     *
     * @param jobId     Analysis job identifier
     * @param clonePath Absolute path to the cloned repository root
     * @return Populated TechnologyAnalysis, or std::nullopt on failure
     */
    [[nodiscard]] std::optional<TechnologyAnalysis> detectAndStore(
        const std::string& jobId,
        const std::string& clonePath) noexcept;

    /**
     * Retrieve a previously stored technology analysis for a job.
     *
     * @param jobId Analysis job identifier
     * @return Stored analysis or std::nullopt
     */
    [[nodiscard]] std::optional<TechnologyAnalysis> getTechnology(
        const std::string& jobId) const noexcept;

private:
    std::shared_ptr<ITechnologyRepository> repository_;
};

} // namespace cortex::technology
