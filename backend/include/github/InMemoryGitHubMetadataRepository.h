/**
 * @file InMemoryGitHubMetadataRepository.h
 * @brief Thread-safe in-memory implementation of IGitHubMetadataRepository.
 * @project Cortex Code Intelligence Platform
 * @author Kartick Kumar Ghosh
 * @github https://github.com/gavinspet
 * @email kartick.ghosh.dev@gmail.com
 * @copyright © 2026
 * @license MIT
 */
#pragma once

#include "github/IGitHubMetadataRepository.h"
#include <unordered_map>
#include <mutex>

namespace cortex::github {

/**
 * @class InMemoryGitHubMetadataRepository
 * @brief Thread-safe in-process metadata store.
 *
 * Uses std::unordered_map protected by std::mutex.
 * Data is ephemeral and lost on restart; suitable for
 * development and demo environments.
 */
class InMemoryGitHubMetadataRepository final : public IGitHubMetadataRepository {
public:
    InMemoryGitHubMetadataRepository() = default;

    void save(const GitHubMetadata& metadata) noexcept override {
        try {
            std::lock_guard<std::mutex> lock(mutex_);
            store_[metadata.jobId] = metadata;
        } catch (...) {}
    }

    [[nodiscard]] std::optional<GitHubMetadata> findByJobId(
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
    std::unordered_map<std::string, GitHubMetadata> store_;
};

} // namespace cortex::github
