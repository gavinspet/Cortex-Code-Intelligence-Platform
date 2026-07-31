/**
 * @file GitHubClient.h
 * @brief GitHub REST API client using curl subprocess for reliable HTTPS.
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
 * @brief Calls the GitHub REST API via a curl subprocess.
 *
 * Uses popen("curl ...") for HTTPS reliability — consistent with the
 * existing git clone pattern in JobWorker. Handles:
 * - 404 (not found)
 * - 403/429 (rate limit)
 * - network timeout (--max-time 10)
 * - JSON parse errors
 *
 * Future alternative: replace with a proper async HTTP library
 * by swapping this implementation behind IGitHubClient.
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
