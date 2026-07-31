/**
 * @file RepositoryInsightResult.h
 * @brief Domain model for synthesised repository insights.
 * @project Cortex Code Intelligence Platform
 * @author Kartick Kumar Ghosh
 * @github https://github.com/gavinspet
 * @email kartick.ghosh.dev@gmail.com
 * @copyright © 2026
 * @license MIT
 */
#pragma once

#include <string>
#include <vector>
#include <chrono>

namespace cortex::insight {

/**
 * @struct RepositoryInsightResult
 * @brief Human-readable synthesis of all analysis layers.
 *
 * All fields are generated deterministically from:
 *   AnalysisResult + GitHubMetadata + TechnologyAnalysis + RepositoryHealthResult
 *
 * No AI or external APIs are used. Every string is produced by
 * rule-based template logic operating on structured data.
 */
struct RepositoryInsightResult {
    std::string jobId;

    // ── Narrative fields ──────────────────────────────────────────────────
    std::string summary;             ///< 2-4 sentence paragraph describing the repo
    std::string technologyOverview;  ///< Sentence describing tech stack
    std::string qualityOverview;     ///< Sentence describing health / maturity

    // ── Inferred classifications ──────────────────────────────────────────
    std::string estimatedProjectSize;   ///< Tiny | Small | Medium | Large | Enterprise
    std::string estimatedMaturity;      ///< Prototype | Personal Project | Production Ready |
                                        ///  Open Source Library | Enterprise Grade
    std::string estimatedComplexity;    ///< Low | Medium | High | Very High

    // ── Enumerated insights ───────────────────────────────────────────────
    std::vector<std::string> strengths;    ///< What the repo does well
    std::vector<std::string> risks;        ///< Gaps or red flags
    std::vector<std::string> suggestions;  ///< Actionable improvement steps

    std::chrono::system_clock::time_point generatedAt;
};

} // namespace cortex::insight
