/**
 * @file IGitHubClient.h
 * @brief Interface for fetching repository metadata from GitHub REST API.
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
 * @class IGitHubClient
 * @brief Abstract interface for GitHub API communication.
 *
 * Isolated behind this interface so future providers (GitLab, Bitbucket, mock)
 * can be substituted without changing business logic.
 */
class IGitHubClient {
public:
    virtual ~IGitHubClient() = default;

    /**
     * Fetch metadata for a repository.
     *
     * @param owner Repository owner login
     * @param repo  Repository name
     * @return Populated GitHubMetadata on success, std::nullopt on failure
     */
    [[nodiscard]] virtual std::optional<GitHubMetadata> fetchMetadata(
        const std::string& owner,
        const std::string& repo) const noexcept = 0;

    // Non-copyable interface
    IGitHubClient(const IGitHubClient&) = delete;
    IGitHubClient& operator=(const IGitHubClient&) = delete;

protected:
    IGitHubClient() = default;
};

} // namespace cortex::github
