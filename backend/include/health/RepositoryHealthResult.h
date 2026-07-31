/**
 * @file RepositoryHealthResult.h
 * @brief Domain model for the repository health and quality evaluation.
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

namespace cortex::health {

/**
 * @struct CategoryScore
 * @brief Score earned in one evaluation category.
 */
struct CategoryScore {
    int  score    = 0; ///< Points earned
    int  maxScore = 0; ///< Maximum achievable points
    std::string grade;  ///< "A"–"F" derived from score/maxScore ratio
};

/**
 * @struct HealthCategories
 * @brief Per-category breakdown of the health evaluation.
 *
 * Total maximum across all categories = 100:
 *   Documentation   20  — community / maintenance docs
 *   Testing         20  — test framework, test dirs, coverage
 *   CI/CD           15  — automated pipelines
 *   Security        15  — policy, Dependabot, SAST, gitignore
 *   Maintainability 15  — linters, formatters, lockfiles
 *   Configuration   10  — .gitignore, Docker, env templates
 *   ProjectStructure 5  — standard directory conventions
 */
struct HealthCategories {
    CategoryScore documentation;
    CategoryScore testing;
    CategoryScore ciCd;
    CategoryScore security;
    CategoryScore maintainability;
    CategoryScore configuration;
    CategoryScore projectStructure;
};

/**
 * @struct RepositoryHealthResult
 * @brief Complete health evaluation result for a repository.
 */
struct RepositoryHealthResult {
    std::string jobId;

    int         overallScore = 0;  ///< 0–100 composite score
    std::string grade;             ///< "A" | "B" | "C" | "D" | "F"

    HealthCategories categories;

    std::vector<std::string> strengths;       ///< What the repo does well
    std::vector<std::string> warnings;        ///< Notable absences or risks
    std::vector<std::string> recommendations; ///< Actionable improvement steps

    std::chrono::system_clock::time_point evaluatedAt;
};

} // namespace cortex::health
