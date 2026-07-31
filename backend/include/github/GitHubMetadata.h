/**
 * @file GitHubMetadata.h
 * @brief Domain model for GitHub repository metadata fetched via GitHub REST API.
 * @project Cortex Code Intelligence Platform
 * @author Kartick Kumar Ghosh
 * @github https://github.com/gavinspet
 * @email kartick.ghosh.dev@gmail.com
 * @copyright © 2026
 * @license MIT
 */
#pragma once

#include <string>
#include <vector>
#include <optional>

namespace cortex::github {

/**
 * @struct GitHubMetadata
 * @brief Immutable value object holding metadata for a GitHub repository.
 *
 * Maps directly to the fields returned by GET /repos/{owner}/{repo}
 * from the GitHub REST API v3.
 */
struct GitHubMetadata {
    // Identity
    std::string jobId;           ///< Correlated analysis job ID
    std::string name;            ///< Repository name (e.g., "react")
    std::string fullName;        ///< Full name (e.g., "facebook/react")
    std::string owner;           ///< Owner login
    std::string ownerAvatarUrl;  ///< Owner avatar image URL

    // Content
    std::string description;     ///< Repository description
    std::string homepage;        ///< Project homepage URL
    std::string defaultBranch;   ///< Default branch (e.g., "main")
    std::string primaryLanguage; ///< Primary language detected by GitHub
    std::vector<std::string> topics; ///< Repository topic tags

    // Metrics
    int stars       = 0;         ///< Stargazer count
    int forks       = 0;         ///< Fork count
    int watchers    = 0;         ///< Watcher count
    int openIssues  = 0;         ///< Open issues count
    long long sizeKb = 0;        ///< Repository size in KB

    // Status
    std::string visibility;      ///< "public" or "private"
    bool archived   = false;     ///< Whether repository is archived
    bool fork       = false;     ///< Whether this is a fork

    // License
    std::string license;         ///< SPDX license identifier (e.g., "MIT")

    // Timestamps (ISO 8601)
    std::string createdAt;       ///< Repository creation date
    std::string updatedAt;       ///< Last update date
    std::string pushedAt;        ///< Last push date
};

} // namespace cortex::github
