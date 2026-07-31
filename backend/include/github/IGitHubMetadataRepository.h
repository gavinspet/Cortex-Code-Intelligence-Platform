/**
 * @file IGitHubMetadataRepository.h
 * @brief Storage interface for persisting and retrieving GitHub metadata.
 * @project Cortex Code Intelligence Platform
 * @author Kartick Kumar Ghosh
 * @github https://github.com/gavinspet
 * @email kartick.ghosh.dev@gmail.com
 * @copyright © 2026
 * @license MIT
 */
#pragma once

#include "github/GitHubMetadata.h"
#include <optional>
#include <string>

namespace cortex::github {

/**
 * @class IGitHubMetadataRepository
 * @brief Abstract repository for storing and retrieving GitHub metadata.
 *
 * Follows the Repository Pattern used throughout Cortex.
 * Current default: InMemoryGitHubMetadataRepository.
 */
class IGitHubMetadataRepository {
public:
    virtual ~IGitHubMetadataRepository() = default;

    /**
     * Persist metadata keyed by jobId.
     * @param metadata GitHubMetadata to store
     */
    virtual void save(const GitHubMetadata& metadata) noexcept = 0;

    /**
     * Retrieve metadata by jobId.
     * @param jobId Analysis job identifier
     * @return Stored metadata or std::nullopt if not found
     */
    [[nodiscard]] virtual std::optional<GitHubMetadata> findByJobId(
        const std::string& jobId) const noexcept = 0;

    IGitHubMetadataRepository(const IGitHubMetadataRepository&) = delete;
    IGitHubMetadataRepository& operator=(const IGitHubMetadataRepository&) = delete;

protected:
    IGitHubMetadataRepository() = default;
};

} // namespace cortex::github
