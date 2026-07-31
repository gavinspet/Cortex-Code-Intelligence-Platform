/**
 * @file TechnologyResult.h
 * @brief Domain model for static technology detection results.
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

namespace cortex::technology {

/**
 * @struct TechnologyItem
 * @brief A single detected technology with its evidence and confidence.
 */
struct TechnologyItem {
    std::string name;       ///< Technology name (e.g., "React", "CMake")
    int         confidence; ///< 0–100 confidence score
    std::string reason;     ///< Human-readable detection evidence
    std::string category;   ///< Internal category tag
};

/**
 * @struct DocumentationStatus
 * @brief Presence of standard community/documentation files.
 */
struct DocumentationStatus {
    bool readme       = false;
    bool license      = false;
    bool changelog    = false;
    bool contributing = false;
    bool security     = false;
    bool codeOfConduct = false;
};

/**
 * @struct TechnologyAnalysis
 * @brief Complete static analysis result for a cloned repository.
 *
 * All detection is heuristic-based (file signatures + content patterns).
 * No repository code is executed.
 */
struct TechnologyAnalysis {
    std::string jobId;

    // Detected technologies by category
    std::vector<TechnologyItem> frameworks;          ///< All detected frameworks
    std::vector<TechnologyItem> backendFrameworks;   ///< Backend-specific frameworks
    std::vector<TechnologyItem> frontendFrameworks;  ///< Frontend-specific frameworks
    std::vector<TechnologyItem> buildSystems;        ///< Build tools (CMake, Make, npm…)
    std::vector<TechnologyItem> packageManagers;     ///< Dependency managers
    std::vector<TechnologyItem> testingFrameworks;   ///< Test frameworks detected
    std::vector<TechnologyItem> ciSystems;           ///< CI/CD pipeline tooling
    std::vector<TechnologyItem> containers;          ///< Container/orchestration tools
    std::vector<TechnologyItem> cloudProviders;      ///< Cloud configuration detected
    std::vector<TechnologyItem> databases;           ///< Database drivers/ORMs detected

    // Summary
    std::string          repositoryType;    ///< Inferred type (e.g., "Frontend SPA")
    DocumentationStatus  documentation;     ///< Presence of community files
    int                  confidenceScore;   ///< Overall detection confidence 0–100
    std::chrono::system_clock::time_point detectedAt;
};

} // namespace cortex::technology
