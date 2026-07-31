/**
 * @file GitHubMetadataService.cpp
 * @brief Business logic for GitHub URL parsing and metadata orchestration.
 * @project Cortex Code Intelligence Platform
 * @author Kartick Kumar Ghosh
 * @github https://github.com/gavinspet
 * @email kartick.ghosh.dev@gmail.com
 * @copyright © 2026
 * @license MIT
 */
#include "github/GitHubMetadataService.h"
#include "logging/Logger.h"
#include <algorithm>
#include <sstream>
#include <vector>

namespace cortex::github {

using cortex::logging::Logger;

// ─── URL Parsing ─────────────────────────────────────────────────────────────

/**
 * Splits a string by a delimiter into a vector of tokens.
 */
static std::vector<std::string> splitStr(const std::string& s, char delim) {
    std::vector<std::string> parts;
    std::istringstream stream(s);
    std::string token;
    while (std::getline(stream, token, delim)) {
        if (!token.empty()) parts.push_back(token);
    }
    return parts;
}

std::optional<ParsedGitHubUrl> GitHubMetadataService::parseGitHubUrl(
    const std::string& url) noexcept
{
    try {
        // Normalise: trim whitespace
        std::string u = url;
        while (!u.empty() && (u.back() == ' ' || u.back() == '/')) u.pop_back();

        // Strip trailing .git
        if (u.size() > 4 && u.substr(u.size() - 4) == ".git")
            u = u.substr(0, u.size() - 4);

        // Must contain "github.com"
        const std::string ghMarker = "github.com/";
        const auto pos = u.find(ghMarker);
        if (pos == std::string::npos) return std::nullopt;

        // Everything after "github.com/"
        const std::string path = u.substr(pos + ghMarker.size());
        const auto parts = splitStr(path, '/');

        if (parts.size() < 2) return std::nullopt;

        ParsedGitHubUrl result;
        result.owner = parts[0];
        result.repo  = parts[1];

        if (result.owner.empty() || result.repo.empty()) return std::nullopt;

        return result;
    } catch (...) {
        return std::nullopt;
    }
}

// ─── Service Methods ─────────────────────────────────────────────────────────

std::optional<GitHubMetadata> GitHubMetadataService::fetchAndStore(
    const std::string& jobId,
    const std::string& repoUrl) noexcept
{
    auto& log = Logger::instance();

    const auto parsed = parseGitHubUrl(repoUrl);
    if (!parsed) {
        log.warn("GitHubMetadataService: cannot parse URL: " + repoUrl);
        return std::nullopt;
    }

    log.info("GitHubMetadataService: fetching metadata for "
             + parsed->owner + "/" + parsed->repo
             + " (job=" + jobId + ")");

    auto metadata = client_->fetchMetadata(parsed->owner, parsed->repo);
    if (!metadata) {
        log.warn("GitHubMetadataService: metadata unavailable for "
                 + parsed->owner + "/" + parsed->repo);
        return std::nullopt;
    }

    // Bind to job
    metadata->jobId = jobId;

    if (repository_) {
        repository_->save(*metadata);
        log.info("GitHubMetadataService: metadata stored for job=" + jobId);
    }

    return metadata;
}

std::optional<GitHubMetadata> GitHubMetadataService::getMetadata(
    const std::string& jobId) const noexcept
{
    if (!repository_) return std::nullopt;
    return repository_->findByJobId(jobId);
}

} // namespace cortex::github
