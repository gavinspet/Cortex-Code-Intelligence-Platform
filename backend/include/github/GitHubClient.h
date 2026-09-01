/**
 * @file GitHubClient.h
 * @brief GitHub REST API client using native libcurl.
 * @project Cortex Code Intelligence Platform
 * @author Kartick Kumar Ghosh
 * @github https://github.com/gavinspet
 * @email kartick.ghosh.dev@gmail.com
 * @copyright © 2026
 * @license MIT
 */
#pragma once

#include "github/IGitHubClient.h"
#include <json/json.h>
#include <string>

namespace cortex::github {

/**
 * @class GitHubClient
 * @brief Calls the GitHub REST API via native libcurl.
 *
 * Supports environment-driven behavior:
 * - GITHUB_API_BASE_URL (default: https://api.github.com)
 * - GITHUB_TOKEN (optional bearer auth token)
 * - GITHUB_HTTP_TIMEOUT_MS (default: 10000)
 * - GITHUB_HTTP_CONNECT_TIMEOUT_MS (default: 3000)
 *
 * Handles:
 * - 404 (not found)
 * - 403/429 (rate limit)
 * - network timeouts
 * - JSON parse errors
 */
class GitHubClient final : public IGitHubClient {
public:
    GitHubClient() = default;

    [[nodiscard]] std::optional<GitHubMetadata> fetchMetadata(
        const std::string& owner,
        const std::string& repo) const noexcept override;

private:
    static GitHubMetadata parseResponse(const Json::Value& json,
                                        const std::string& jobId = "") noexcept;
};

} // namespace cortex::github
