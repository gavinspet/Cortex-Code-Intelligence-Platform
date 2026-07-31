/**
 * @file GitHubClient.cpp
 * @brief GitHub REST API client using curl subprocess for HTTPS reliability.
 * @project Cortex Code Intelligence Platform
 * @author Kartick Kumar Ghosh
 * @github https://github.com/gavinspet
 * @email kartick.ghosh.dev@gmail.com
 * @copyright © 2026
 * @license MIT
 */
#include "github/GitHubClient.h"
#include "logging/Logger.h"
#include <json/json.h>
#include <array>
#include <chrono>
#include <cstdio>
#include <memory>
#include <sstream>

namespace cortex::github {

using cortex::logging::Logger;

// ─── Helper: safe JSON field extraction ──────────────────────────────────────

static std::string safeStr(const Json::Value& v, const char* key,
                            const std::string& fallback = "") noexcept
{
    try {
        if (v.isMember(key) && !v[key].isNull() && v[key].isString())
            return v[key].asString();
    } catch (...) {}
    return fallback;
}

static int safeInt(const Json::Value& v, const char* key, int fallback = 0) noexcept
{
    try {
        if (v.isMember(key) && !v[key].isNull() && v[key].isInt())
            return v[key].asInt();
    } catch (...) {}
    return fallback;
}

// ─── Response Parser ──────────────────────────────────────────────────────────

GitHubMetadata GitHubClient::parseResponse(const Json::Value& json,
                                            const std::string& jobId) noexcept
{
    GitHubMetadata m;
    m.jobId = jobId;

    try {
        m.name      = safeStr(json, "name");
        m.fullName  = safeStr(json, "full_name");

        if (json.isMember("owner") && json["owner"].isObject()) {
            m.owner          = safeStr(json["owner"], "login");
            m.ownerAvatarUrl = safeStr(json["owner"], "avatar_url");
        }

        m.description     = safeStr(json, "description");
        m.homepage        = safeStr(json, "homepage");
        m.defaultBranch   = safeStr(json, "default_branch", "main");
        m.primaryLanguage = safeStr(json, "language");
        m.visibility      = safeStr(json, "visibility", "public");
        m.stars           = safeInt(json, "stargazers_count");
        m.forks           = safeInt(json, "forks_count");
        m.watchers        = safeInt(json, "watchers_count");
        m.openIssues      = safeInt(json, "open_issues_count");

        if (json.isMember("size") && json["size"].isInt())
            m.sizeKb = static_cast<long long>(json["size"].asInt());

        if (json.isMember("archived") && json["archived"].isBool())
            m.archived = json["archived"].asBool();

        if (json.isMember("fork") && json["fork"].isBool())
            m.fork = json["fork"].asBool();

        if (json.isMember("license") && json["license"].isObject()
            && !json["license"].isNull()) {
            m.license = safeStr(json["license"], "spdx_id");
            if (m.license.empty() || m.license == "NOASSERTION")
                m.license = safeStr(json["license"], "name");
        }

        if (json.isMember("topics") && json["topics"].isArray()) {
            for (const auto& t : json["topics"])
                if (t.isString()) m.topics.push_back(t.asString());
        }

        m.createdAt = safeStr(json, "created_at");
        m.updatedAt = safeStr(json, "updated_at");
        m.pushedAt  = safeStr(json, "pushed_at");

    } catch (const std::exception& e) {
        Logger::instance().warn(
            std::string("GitHubClient: parse error: ") + e.what());
    }

    return m;
}

// ─── HTTP fetch via curl subprocess ──────────────────────────────────────────

std::optional<GitHubMetadata> GitHubClient::fetchMetadata(
    const std::string& owner,
    const std::string& repo) const noexcept
{
    auto& log = Logger::instance();

    try {
        const std::string url =
            "https://api.github.com/repos/" + owner + "/" + repo;

        log.info("GitHub API request: GET " + url);
        const auto startTime = std::chrono::steady_clock::now();

        // Build curl command: write HTTP status code on last line,
        // follow redirects, 10-second timeout, minimal noise.
        const std::string cmd =
            "curl -s --max-time 10 "
            "-H 'User-Agent: CortexCodeIntelligencePlatform/1.0' "
            "-H 'Accept: application/vnd.github.v3+json' "
            "-H 'X-GitHub-Api-Version: 2022-11-28' "
            "-w '\\n%{http_code}' "
            "'" + url + "' 2>/dev/null";

        std::array<char, 4096> buf{};
        std::string output;
        std::unique_ptr<FILE, decltype(&pclose)> pipe(
            popen(cmd.c_str(), "r"), pclose);

        if (!pipe) {
            log.error("GitHubClient: failed to spawn curl for " + owner + "/" + repo);
            return std::nullopt;
        }

        while (fgets(buf.data(), buf.size(), pipe.get()))
            output += buf.data();

        const int exitCode = pclose(pipe.release());

        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startTime).count();

        if (exitCode != 0) {
            log.warn("GitHubClient: curl exited " + std::to_string(exitCode)
                     + " for " + owner + "/" + repo);
            return std::nullopt;
        }

        // Separate JSON body and status code (last line)
        const auto lastNewline = output.rfind('\n', output.size() - 2);
        if (lastNewline == std::string::npos) {
            log.warn("GitHubClient: unexpected curl output for " + owner + "/" + repo);
            return std::nullopt;
        }

        const int httpStatus = std::stoi(
            output.substr(lastNewline + 1));
        const std::string body = output.substr(0, lastNewline);

        if (httpStatus == 404) {
            log.warn("GitHub API: repository not found: " + owner + "/" + repo);
            return std::nullopt;
        }
        if (httpStatus == 403 || httpStatus == 429) {
            log.warn("GitHub API: rate limit exceeded (HTTP "
                     + std::to_string(httpStatus) + ") for "
                     + owner + "/" + repo);
            return std::nullopt;
        }
        if (httpStatus != 200) {
            log.warn("GitHub API: unexpected HTTP " + std::to_string(httpStatus)
                     + " for " + owner + "/" + repo);
            return std::nullopt;
        }

        // Parse JSON
        Json::Value json;
        Json::CharReaderBuilder builder;
        std::string errs;
        std::istringstream ss(body);
        if (!Json::parseFromStream(builder, ss, &json, &errs)) {
            log.warn("GitHubClient: JSON parse error for " + owner + "/" + repo
                     + ": " + errs);
            return std::nullopt;
        }

        auto metadata = parseResponse(json);
        log.info("GitHub API response: " + owner + "/" + repo
                 + " ★" + std::to_string(metadata.stars)
                 + "  (" + std::to_string(elapsed) + "ms)");

        return metadata;

    } catch (const std::exception& e) {
        log.error(std::string("GitHubClient::fetchMetadata exception: ") + e.what());
        return std::nullopt;
    } catch (...) {
        log.error("GitHubClient::fetchMetadata unknown exception");
        return std::nullopt;
    }
}

} // namespace cortex::github
