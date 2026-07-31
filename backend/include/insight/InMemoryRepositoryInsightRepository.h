/**
 * @file InMemoryRepositoryInsightRepository.h
 * @brief Thread-safe in-memory implementation of IRepositoryInsightRepository.
 * @project Cortex Code Intelligence Platform
 * @author Kartick Kumar Ghosh
 * @github https://github.com/gavinspet
 * @email kartick.ghosh.dev@gmail.com
 * @copyright © 2026
 * @license MIT
 */
#pragma once

#include "insight/IRepositoryInsightRepository.h"
#include <unordered_map>
#include <mutex>

namespace cortex::insight {

class InMemoryRepositoryInsightRepository final : public IRepositoryInsightRepository {
public:
    InMemoryRepositoryInsightRepository() = default;

    void save(const RepositoryInsightResult& result) noexcept override {
        try { std::lock_guard<std::mutex> lock(mutex_); store_[result.jobId] = result; }
        catch (...) {}
    }

    [[nodiscard]] std::optional<RepositoryInsightResult> findByJobId(
        const std::string& jobId) const noexcept override
    {
        try {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = store_.find(jobId);
            if (it != store_.end()) return it->second;
            return std::nullopt;
        } catch (...) { return std::nullopt; }
    }

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, RepositoryInsightResult> store_;
};

} // namespace cortex::insight
