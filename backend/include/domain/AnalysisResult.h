/**
 * @file AnalysisResult.h
 * @brief Domain value object representing the output of a repository code analysis scan
 *
 * @project Cortex Code Intelligence Platform
 *
 * @author Kartick Kumar Ghosh
 * @github https://github.com/gavinspet
 * @email kartick.ghosh.dev@gmail.com
 *
 * @copyright Copyright (c) 2026 Kartick Kumar Ghosh
 * @license MIT
 */

#pragma once

#include <string>
#include <unordered_map>
#include <chrono>

namespace cortex::domain {

struct AnalysisResult {
    std::string jobId;
    int fileCount = 0;
    int dirCount = 0;
    long long totalLines = 0;
    std::unordered_map<std::string, int> languageDistribution; // extension -> file count
    std::chrono::system_clock::time_point analyzedAt;
    std::string clonePath;
};

} // namespace cortex::domain
