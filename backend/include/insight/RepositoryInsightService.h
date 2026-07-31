/**
 * @file RepositoryInsightService.h
 * @brief Aggregates all analysis layers into human-readable insights.
 * @project Cortex Code Intelligence Platform
 * @author Kartick Kumar Ghosh
 * @github https://github.com/gavinspet
 * @email kartick.ghosh.dev@gmail.com
 * @copyright © 2026
 * @license MIT
 */
#pragma once

#include "insight/IRepositoryInsightRepository.h"
#include "domain/AnalysisResult.h"
#include "github/GitHubMetadata.h"
#include "technology/TechnologyResult.h"
#include "health/RepositoryHealthResult.h"
#include <memory>
#include <optional>
#include <string>

namespace cortex::insight {

/**
 * @class RepositoryInsightService
 * @brief Deterministic insight generator — no AI, no external APIs.
 *
 * The InsightGenerator (implemented inside the translation unit) transforms
 * structured data from four analysis layers into concise, professional prose
 * and enumerated lists.
 *
 * Classifiers:
 *   Project size:     Tiny / Small / Medium / Large / Enterprise
 *   Project maturity: Prototype / Personal Project / Production Ready /
 *                     Open Source Library / Enterprise Grade
 *   Complexity:       Low / Medium / High / Very High
 */
class RepositoryInsightService {
public:
    explicit RepositoryInsightService(
        std::shared_ptr<IRepositoryInsightRepository> repository) noexcept
        : repository_(std::move(repository))
    {}

    /**
     * Generate insights from all available analysis results and persist them.
     *
     * @param jobId    Analysis job identifier
     * @param analysis Filesystem scan result (may be nullptr / nullopt)
     * @param metadata GitHub repository metadata (may be nullptr)
     * @param tech     Technology detection result (may be nullptr)
     * @param health   Repository health evaluation (may be nullptr)
     * @return Generated insights, or std::nullopt on failure
     */
    [[nodiscard]] std::optional<RepositoryInsightResult> generateAndStore(
        const std::string& jobId,
        const cortex::domain::AnalysisResult*             analysis,
        const cortex::github::GitHubMetadata*             metadata,
        const cortex::technology::TechnologyAnalysis*     tech,
        const cortex::health::RepositoryHealthResult*     health) noexcept;

    /**
     * Retrieve previously generated insights.
     *
     * @param jobId Analysis job identifier
     * @return Stored insights or std::nullopt
     */
    [[nodiscard]] std::optional<RepositoryInsightResult> getInsights(
        const std::string& jobId) const noexcept;

private:
    std::shared_ptr<IRepositoryInsightRepository> repository_;
};

} // namespace cortex::insight
