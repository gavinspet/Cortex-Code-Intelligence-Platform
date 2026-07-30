/**
 * @file IAnalysisRepository.h
 * @brief Abstract interface defining the storage contract for code analysis results
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

#include "domain/AnalysisResult.h"
#include <optional>
#include <string>

namespace cortex::analysis {

class IAnalysisRepository {
public:
    virtual ~IAnalysisRepository() = default;

    virtual void save(const cortex::domain::AnalysisResult& result) noexcept = 0;

    virtual std::optional<cortex::domain::AnalysisResult> findByJobId(
        const std::string& jobId) const noexcept = 0;

    IAnalysisRepository(const IAnalysisRepository&) = delete;
    IAnalysisRepository& operator=(const IAnalysisRepository&) = delete;

protected:
    IAnalysisRepository() = default;
};

} // namespace cortex::analysis
