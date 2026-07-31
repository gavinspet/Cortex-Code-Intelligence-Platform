/**
 * @file InMemoryRepositoryHealthRepository.h
 * @brief Thread-safe in-memory implementation of IRepositoryHealthRepository.
 * @project Cortex Code Intelligence Platform
 * @author Kartick Kumar Ghosh
 * @github https://github.com/gavinspet
 * @email kartick.ghosh.dev@gmail.com
 * @copyright © 2026
 * @license MIT
 */
#pragma once

#include "health/IRepositoryHealthRepository.h"
#include <unordered_map>
#include <mutex>

namespace cortex::health {

class InMemoryRepositoryHealthRepository final : public IRepositoryHealthRepository {
public:
    InMemoryRepositoryHealthRepository() = default;

    void save(const RepositoryHealthResult& result) noexcept override {
        try {
            std::lock_guard<std::mutex> lock(mutex_);
            store_[result.jobId] = result;
        } catch (...) {}
    }

    [[nodiscard]] std::optional<RepositoryHealthResult> findByJobId(
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
    std::unordered_map<std::string, RepositoryHealthResult> store_;
};

} // namespace cortex::health
