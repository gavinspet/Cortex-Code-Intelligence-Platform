/**
 * @file UrlValidator.h
 * @brief Validates that repository URLs are HTTPS GitHub or GitLab endpoints
 *
 * @project Cortex Code Intelligence Platform
 *
 * @author Kartick Kumar Ghosh
 * @github https://github.com/gavinspet
 * @email kartick.ghosh.dev@gmail.com
 *
 * @copyright Copyright (c) 2026 Kartick Kumar Ghosh
 * @license MIT
 */

#pragma once

#include <string>
#include <string_view>
#include <regex>

namespace cortex::utils {

/**
 * URL validation utility
 * Validates repository URLs against expected patterns
 */
class UrlValidator {
public:
    /**
     * Validate repository URL
     * 
     * Checks for:
     * - Non-empty URL
     * - Valid HTTPS protocol
     * - GitHub or GitLab domain
     * - Proper Git repository path
     * 
     * @param url URL to validate
     * @return true if valid, false otherwise
     */
    static bool isValidRepositoryUrl(std::string_view url) noexcept {
        try {
            // Check for empty URL
            if (url.empty()) {
                return false;
            }

            std::string urlStr(url);

            // Check for HTTPS protocol
            if (urlStr.find("https://") != 0) {
                return false;
            }

            // Check for GitHub or GitLab
            bool isGitHub = urlStr.find("github.com") != std::string::npos;
            bool isGitLab = urlStr.find("gitlab.com") != std::string::npos;

            if (!isGitHub && !isGitLab) {
                return false;
            }

            // .git suffix is optional — append it later if missing
            // Basic structure check: should have at least user/repo pattern
            // e.g., https://github.com/user/repo or https://github.com/user/repo.git
            size_t lastSlash = urlStr.find_last_of('/');
            if (lastSlash == std::string::npos || lastSlash == urlStr.length() - 1) {
                return false;
            }

            // Check that there's content between last slash and .git
            std::string lastPart = urlStr.substr(lastSlash + 1);
            if (lastPart.empty() || lastPart == ".git") {
                return false;
            }

            return true;
        } catch (...) {
            return false;
        }
    }

    // Static utility class - no instances
    UrlValidator() = delete;
};

} // namespace cortex::utils
