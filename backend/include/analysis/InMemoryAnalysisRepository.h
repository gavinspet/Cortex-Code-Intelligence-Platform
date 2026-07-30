/**
 * @file InMemoryAnalysisRepository.h
 * @brief Thread-safe in-memory implementation of IAnalysisRepository using std::unordered_map
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
#include <unordered_map>
#include <mutex>

namespace cortex::analysis {

class InMemoryAnalysisRepository : public IAnalysisRepository {
public:
    InMemoryAnalysisRepository() = default;

    void save(const cortex::domain::AnalysisResult& result) noexcept override {
        try {
            std::lock_guard<std::mutex> lock(mutex_);
            results_[result.jobId] = result;
        } catch (...) {}
    }

    std::optional<cortex::domain::AnalysisResult> findByJobId(
        const std::string& jobId) const noexcept override
    {
        try {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = results_.find(jobId);
            if (it != results_.end()) {
                return it->second;
            }
            return std::nullopt;
        } catch (...) {
            return std::nullopt;
        }
    }

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, cortex::domain::AnalysisResult> results_;
};

} // namespace cortex::analysis
