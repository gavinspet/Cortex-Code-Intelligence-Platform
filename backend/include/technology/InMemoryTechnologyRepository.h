/**
 * @file InMemoryTechnologyRepository.h
 * @brief Thread-safe in-memory implementation of ITechnologyRepository.
 * @project Cortex Code Intelligence Platform
 * @author Kartick Kumar Ghosh
 * @github https://github.com/gavinspet
 * @email kartick.ghosh.dev@gmail.com
 * @copyright © 2026
 * @license MIT
 */
#pragma once

#include "technology/ITechnologyRepository.h"
#include <unordered_map>
#include <mutex>

namespace cortex::technology {

class InMemoryTechnologyRepository final : public ITechnologyRepository {
public:
    InMemoryTechnologyRepository() = default;

    void save(const TechnologyAnalysis& analysis) noexcept override {
        try {
            std::lock_guard<std::mutex> lock(mutex_);
            store_[analysis.jobId] = analysis;
        } catch (...) {}
    }

    [[nodiscard]] std::optional<TechnologyAnalysis> findByJobId(
        const std::string& jobId) const noexcept override
    {
        try {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = store_.find(jobId);
            if (it != store_.end()) return it->second;
            return std::nullopt;
        } catch (...) {
            return std::nullopt;
        }
    }

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, TechnologyAnalysis> store_;
};

} // namespace cortex::technology
