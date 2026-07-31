#include "analysis/AnalysisController.h"
#include "logging/Logger.h"
#include <json/json.h>
#include <drogon/HttpResponse.h>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace cortex::analysis {

using cortex::logging::Logger;

static std::string timePointToIso8601(std::chrono::system_clock::time_point tp) {
    auto t = std::chrono::system_clock::to_time_t(tp);
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&t), "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

void AnalysisController::getAnalysis(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& jobId) const noexcept
{
    try {
        Logger::instance().info(std::string("GET /analysis/") + jobId);

        auto result = service_->getAnalysis(jobId);

        if (!result) {
            Json::Value body;
            body["success"] = false;
            body["message"] = "Analysis not found for job: " + jobId;
            auto resp = drogon::HttpResponse::newHttpJsonResponse(body);
            resp->setStatusCode(drogon::k404NotFound);
            callback(resp);
            return;
        }

        // ── Core analysis data (unchanged contract) ───────────────────────
        Json::Value languages;
        for (const auto& [ext, count] : result->languageDistribution) {
            languages[ext] = count;
        }

        Json::Value data;
        data["jobId"]       = result->jobId;
        data["fileCount"]   = result->fileCount;
        data["dirCount"]    = result->dirCount;
        data["totalLines"]  = static_cast<Json::Int64>(result->totalLines);
        data["languages"]   = languages;
        data["analyzedAt"]  = timePointToIso8601(result->analyzedAt);

        // ── GitHub metadata (optional enrichment) ────────────────────────
        if (metadataService_) {
            auto meta = metadataService_->getMetadata(jobId);
            if (meta) {
                Json::Value m;
                m["name"]             = meta->name;
                m["fullName"]         = meta->fullName;
                m["owner"]            = meta->owner;
                m["ownerAvatarUrl"]   = meta->ownerAvatarUrl;
                m["description"]      = meta->description;
                m["homepage"]         = meta->homepage;
                m["defaultBranch"]    = meta->defaultBranch;
                m["primaryLanguage"]  = meta->primaryLanguage;
                m["visibility"]       = meta->visibility;
                m["stars"]            = meta->stars;
                m["forks"]            = meta->forks;
                m["watchers"]         = meta->watchers;
                m["openIssues"]       = meta->openIssues;
                m["sizeKb"]           = static_cast<Json::Int64>(meta->sizeKb);
                m["archived"]         = meta->archived;
                m["fork"]             = meta->fork;
                m["license"]          = meta->license;
                m["createdAt"]        = meta->createdAt;
                m["updatedAt"]        = meta->updatedAt;
                m["pushedAt"]         = meta->pushedAt;

                Json::Value topics(Json::arrayValue);
                for (const auto& t : meta->topics) topics.append(t);
                m["topics"] = topics;

                data["metadata"] = m;
                Logger::instance().info(
                    "Analysis response enriched with GitHub metadata for job=" + jobId);
            } else {
                data["metadata"] = Json::Value(Json::nullValue);
                Logger::instance().info(
                    "No GitHub metadata available for job=" + jobId);
            }
        }

        Json::Value body;
        body["success"] = true;
        body["data"]    = data;

        auto resp = drogon::HttpResponse::newHttpJsonResponse(body);
        resp->setStatusCode(drogon::k200OK);
        callback(resp);

    } catch (const std::exception& e) {
        Logger::instance().error(std::string("Error in AnalysisController: ") + e.what());
        Json::Value body;
        body["success"] = false;
        body["message"] = "Internal server error";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(body);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
    }
}

} // namespace cortex::analysis
