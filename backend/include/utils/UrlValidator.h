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

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace cortex::utils {

/**
 * Strict allowlist validator for repository URLs.
 *
 * Accepted form: https://<host>/<path>[.git][/]
 * - scheme and host are matched ASCII-case-insensitively; path case is preserved
 * - host must equal github.com, www.github.com or gitlab.com
 *   (no userinfo, port, query, fragment, percent-encoding or non-ASCII)
 * - GitHub path: exactly owner/repo
 * - GitLab path: group[/subgroup...]/project, 2..kMaxGitLabSegments segments
 */
class UrlValidator {
public:
    static constexpr std::size_t kMaxUrlLength = 512;
    static constexpr std::size_t kMaxGitHubOwnerLength = 39;
    static constexpr std::size_t kMaxSegmentLength = 100;
    static constexpr std::size_t kMaxGitLabSegments = 20;

    /**
     * Validate a repository URL and build the canonical clone URL from the
     * validated components only (never from the raw input).
     *
     * @return "https://<host>/<path>.git" if valid, std::nullopt otherwise
     */
    static std::optional<std::string> canonicalCloneUrl(std::string_view url) noexcept {
        try {
            if (url.empty() || url.size() > kMaxUrlLength) {
                return std::nullopt;
            }
            for (const char ch : url) {
                const auto c = static_cast<unsigned char>(ch);
                if (c <= 0x20 || c >= 0x7f) {
                    return std::nullopt;  // whitespace, control or non-ASCII
                }
            }

            constexpr std::string_view kScheme = "https://";
            if (!equalsIgnoreAsciiCase(url.substr(0, kScheme.size()), kScheme)) {
                return std::nullopt;
            }

            const std::string_view rest = url.substr(kScheme.size());
            const auto slash = rest.find('/');
            if (slash == std::string_view::npos) {
                return std::nullopt;
            }

            const std::string_view host = rest.substr(0, slash);
            std::string_view path = rest.substr(slash + 1);

            // canonicalHost is always a literal, never derived from the input bytes.
            bool isGitHub = false;
            std::string_view canonicalHost;
            if (equalsIgnoreAsciiCase(host, "github.com") || equalsIgnoreAsciiCase(host, "www.github.com")) {
                isGitHub = true;
                canonicalHost = "github.com";
            } else if (equalsIgnoreAsciiCase(host, "gitlab.com")) {
                canonicalHost = "gitlab.com";
            } else {
                return std::nullopt;
            }

            if (!path.empty() && path.back() == '/') {
                path.remove_suffix(1);
            }
            constexpr std::string_view kGitSuffix = ".git";
            if (path.size() > kGitSuffix.size() &&
                path.substr(path.size() - kGitSuffix.size()) == kGitSuffix) {
                path.remove_suffix(kGitSuffix.size());
            }

            std::vector<std::string_view> segments;
            std::size_t start = 0;
            while (true) {
                const auto next = path.find('/', start);
                const auto segment = path.substr(start, next == std::string_view::npos ? next : next - start);
                if (!isValidPathSegment(segment)) {
                    return std::nullopt;
                }
                segments.push_back(segment);
                if (next == std::string_view::npos) {
                    break;
                }
                start = next + 1;
            }

            if (isGitHub) {
                if (segments.size() != 2 || !isValidGitHubOwner(segments[0])) {
                    return std::nullopt;
                }
            } else if (segments.size() < 2 || segments.size() > kMaxGitLabSegments) {
                return std::nullopt;
            }

            std::string canonical = "https://";
            canonical += canonicalHost;
            for (const auto segment : segments) {
                canonical += '/';
                canonical += segment;
            }
            canonical += kGitSuffix;
            return canonical;
        } catch (...) {
            return std::nullopt;
        }
    }

    /**
     * Validate repository URL
     *
     * @param url URL to validate
     * @return true if the URL is an allowed HTTPS GitHub/GitLab repository URL
     */
    static bool isValidRepositoryUrl(std::string_view url) noexcept {
        return canonicalCloneUrl(url).has_value();
    }

    // Static utility class - no instances
    UrlValidator() = delete;

private:
    static constexpr bool isAsciiAlnum(char c) noexcept {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
    }

    static constexpr char toAsciiLower(char c) noexcept {
        return (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c;
    }

    // ASCII-only folding (A-Z); no locale or Unicode case mapping.
    static constexpr bool equalsIgnoreAsciiCase(std::string_view a, std::string_view b) noexcept {
        if (a.size() != b.size()) {
            return false;
        }
        for (std::size_t i = 0; i < a.size(); ++i) {
            if (toAsciiLower(a[i]) != toAsciiLower(b[i])) {
                return false;
            }
        }
        return true;
    }

    // [A-Za-z0-9._-], 1..100 chars, must not start with '-', must not be all dots.
    static constexpr bool isValidPathSegment(std::string_view segment) noexcept {
        if (segment.empty() || segment.size() > kMaxSegmentLength || segment.front() == '-') {
            return false;
        }
        bool allDots = true;
        for (const char c : segment) {
            if (!isAsciiAlnum(c) && c != '.' && c != '_' && c != '-') {
                return false;
            }
            if (c != '.') {
                allDots = false;
            }
        }
        return !allDots;
    }

    // GitHub usernames: [A-Za-z0-9-], 1..39 chars, no leading/trailing '-'.
    static constexpr bool isValidGitHubOwner(std::string_view owner) noexcept {
        if (owner.empty() || owner.size() > kMaxGitHubOwnerLength ||
            owner.front() == '-' || owner.back() == '-') {
            return false;
        }
        for (const char c : owner) {
            if (!isAsciiAlnum(c) && c != '-') {
                return false;
            }
        }
        return true;
    }
};

} // namespace cortex::utils
