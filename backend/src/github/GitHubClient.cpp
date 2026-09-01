/**
 * @file GitHubClient.cpp
 * @brief GitHub REST API client using native libcurl.
 * @project Cortex Code Intelligence Platform
 * @author Kartick Kumar Ghosh
 * @github https://github.com/gavinspet
 * @email kartick.ghosh.dev@gmail.com
 * @copyright © 2026
 * @license MIT
 */
#include "github/GitHubClient.h"
#include "logging/Logger.h"
#include <curl/curl.h>
#include <json/json.h>
#include <chrono>
#include <cstdlib>
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

static std::string getEnvOrDefault(const char* name, const std::string& fallback) {
    const char* value = std::getenv(name);
    if (!value || *value == '\0') {
        return fallback;
    }
    return std::string(value);
}

static long getEnvLongOrDefault(const char* name, long fallback) {
    const char* value = std::getenv(name);
    if (!value || *value == '\0') {
        return fallback;
    }

    try {
        long parsed = std::stol(value);
        return parsed > 0 ? parsed : fallback;
    } catch (...) {
        return fallback;
    }
}

static size_t writeBodyCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    const size_t totalSize = size * nmemb;
    auto* responseBody = static_cast<std::string*>(userp);
    responseBody->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

static std::string trimTrailingSlash(std::string value) {
    while (!value.empty() && value.back() == '/') {
        value.pop_back();
    }
    return value;
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

// ─── HTTP fetch via libcurl ───────────────────────────────────────────────────

std::optional<GitHubMetadata> GitHubClient::fetchMetadata(
    const std::string& owner,
    const std::string& repo) const noexcept
{
    auto& log = Logger::instance();

    try {
        const std::string apiBaseUrl =
            trimTrailingSlash(getEnvOrDefault("GITHUB_API_BASE_URL", "https://api.github.com"));
        const std::string token = getEnvOrDefault("GITHUB_TOKEN", "");
        const long timeoutMs = getEnvLongOrDefault("GITHUB_HTTP_TIMEOUT_MS", 10000);
        const long connectTimeoutMs = getEnvLongOrDefault("GITHUB_HTTP_CONNECT_TIMEOUT_MS", 3000);

        const std::string url = apiBaseUrl + "/repos/" + owner + "/" + repo;

        log.info("GitHub API request: GET " + url);
        const auto startTime = std::chrono::steady_clock::now();

        CURL* curl = curl_easy_init();
        if (!curl) {
            log.error("GitHubClient: failed to initialize curl");
            return std::nullopt;
        }

        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "User-Agent: CortexCodeIntelligencePlatform/1.0");
        headers = curl_slist_append(headers, "Accept: application/vnd.github.v3+json");
        headers = curl_slist_append(headers, "X-GitHub-Api-Version: 2022-11-28");

        if (!token.empty()) {
            const std::string auth = "Authorization: Bearer " + token;
            headers = curl_slist_append(headers, auth.c_str());
        }

        std::string responseBody;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeBodyCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, connectTimeoutMs);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeoutMs);
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

        const CURLcode curlResult = curl_easy_perform(curl);

        long httpStatus = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpStatus);

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        const auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startTime).count();

        if (curlResult != CURLE_OK) {
            log.warn("GitHubClient: request failed for " + owner + "/" + repo
                     + " error='" + std::string(curl_easy_strerror(curlResult)) + "'");
            return std::nullopt;
        }

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

        if (responseBody.empty()) {
            log.warn("GitHubClient: empty response body for " + owner + "/" + repo);
            return std::nullopt;
        }

        // Parse JSON
        Json::Value json;
        Json::CharReaderBuilder builder;
        std::string errs;
        std::istringstream ss(responseBody);
        if (!Json::parseFromStream(builder, ss, &json, &errs)) {
            log.warn("GitHubClient: JSON parse error for " + owner + "/" + repo
                     + ": " + errs);
            return std::nullopt;
        }

        auto metadata = parseResponse(json);
        log.info("GitHub API response: " + owner + "/" + repo
                 + " ★" + std::to_string(metadata.stars)
                 + "  (" + std::to_string(elapsedMs) + "ms)");

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
