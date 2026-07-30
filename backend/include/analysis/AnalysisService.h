/**
 * @file AnalysisService.h
 * @brief Business logic layer for retrieving code analysis results by job identifier
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

#include "analysis/IAnalysisRepository.h"
#include <memory>
#include <optional>
#include <string>

namespace cortex::analysis {

class AnalysisService {
public:
    explicit AnalysisService(std::shared_ptr<IAnalysisRepository> repo) noexcept
        : repo_(std::move(repo)) {}

    std::optional<cortex::domain::AnalysisResult> getAnalysis(
        const std::string& jobId) const noexcept
    {
        if (!repo_) return std::nullopt;
        return repo_->findByJobId(jobId);
    }

private:
    std::shared_ptr<IAnalysisRepository> repo_;
};

} // namespace cortex::analysis
