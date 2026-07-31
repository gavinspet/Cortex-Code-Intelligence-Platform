/**
 * @file GitHubMetadataService.h
 * @brief Orchestrates GitHub URL parsing and metadata fetching.
 * @project Cortex Code Intelligence Platform
 * @author Kartick Kumar Ghosh
 * @github https://github.com/gavinspet
 * @email kartick.ghosh.dev@gmail.com
 * @copyright © 2026
 * @license MIT
 */
#pragma once

#include "github/IGitHubClient.h"
#include "github/IGitHubMetadataRepository.h"
#include <memory>
#include <optional>
#include <string>

namespace cortex::github {

/**
 * @struct ParsedGitHubUrl
 * @brief Result of parsing a GitHub repository URL.
 */
struct ParsedGitHubUrl {
    std::string owner;
    std::string repo;
};

/**
 * @class GitHubMetadataService
 * @brief Business-logic layer for GitHub metadata acquisition.
 *
 * Responsibilities:
 * 1. Parse a GitHub URL into owner/repo components
 * 2. Delegate HTTP fetching to IGitHubClient
 * 3. Persist metadata via IGitHubMetadataRepository
 * 4. Provide retrieval for the analysis controller
 *
 * Accepts any IGitHubClient implementation (real, mock, stub),
 * making the service fully unit-testable.
 */
class GitHubMetadataService {
public:
    GitHubMetadataService(
        std::shared_ptr<IGitHubClient> client,
        std::shared_ptr<IGitHubMetadataRepository> repository) noexcept
        : client_(std::move(client))
        , repository_(std::move(repository))
    {}

    /**
     * Parse a GitHub URL and fetch + store metadata for a job.
     *
     * @param jobId      Analysis job identifier (used as storage key)
     * @param repoUrl    Full GitHub URL, e.g. https://github.com/facebook/react
     * @return Fetched metadata, or std::nullopt on any failure
     */
    [[nodiscard]] std::optional<GitHubMetadata> fetchAndStore(
        const std::string& jobId,
        const std::string& repoUrl) noexcept;

    /**
     * Retrieve previously stored metadata for a job.
     *
     * @param jobId Analysis job identifier
     * @return Stored metadata or std::nullopt
     */
    [[nodiscard]] std::optional<GitHubMetadata> getMetadata(
        const std::string& jobId) const noexcept;

    /**
     * Parse a GitHub URL into owner and repo components.
     * Static utility — available without constructing a service instance.
     *
     * Supports:
     *   https://github.com/owner/repo
     *   https://github.com/owner/repo.git
     *   https://github.com/owner/repo/
     *
     * @param url GitHub repository URL
     * @return ParsedGitHubUrl on success, std::nullopt for non-GitHub or malformed URLs
     */
    [[nodiscard]] static std::optional<ParsedGitHubUrl> parseGitHubUrl(
        const std::string& url) noexcept;

private:
    std::shared_ptr<IGitHubClient>             client_;
    std::shared_ptr<IGitHubMetadataRepository> repository_;
};

} // namespace cortex::github
