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
            if (!meta && jobRepository_) {
                auto job = jobRepository_->findById(jobId);
                if (job) {
                    Logger::instance().info(
                        "GitHub metadata cache miss for job=" + jobId +
                        "; attempting on-demand fetch");
                    meta = metadataService_->fetchAndStore(jobId, job->getRepositoryUrl());
                }
            }

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
                    "No GitHub metadata available for job=" + jobId +
                    " after cache lookup/fetch attempt");
            }
        }

        // ── Technology analysis ───────────────────────────────────────────
        if (technologyService_) {
            auto tech = technologyService_->getTechnology(jobId);
            if (tech) {
                auto serializeItems = [](const std::vector<cortex::technology::TechnologyItem>& vec) {
                    Json::Value arr(Json::arrayValue);
                    for (const auto& t : vec) {
                        Json::Value v;
                        v["name"]       = t.name;
                        v["confidence"] = t.confidence;
                        v["reason"]     = t.reason;
                        arr.append(v);
                    }
                    return arr;
                };

                Json::Value ta;
                ta["repositoryType"]     = tech->repositoryType;
                ta["confidenceScore"]    = tech->confidenceScore;
                ta["frameworks"]         = serializeItems(tech->frameworks);
                ta["frontendFrameworks"] = serializeItems(tech->frontendFrameworks);
                ta["backendFrameworks"]  = serializeItems(tech->backendFrameworks);
                ta["buildSystems"]       = serializeItems(tech->buildSystems);
                ta["packageManagers"]    = serializeItems(tech->packageManagers);
                ta["testingFrameworks"]  = serializeItems(tech->testingFrameworks);
                ta["ciSystems"]          = serializeItems(tech->ciSystems);
                ta["containers"]         = serializeItems(tech->containers);
                ta["cloudProviders"]     = serializeItems(tech->cloudProviders);
                ta["databases"]          = serializeItems(tech->databases);

                Json::Value doc;
                doc["readme"]        = tech->documentation.readme;
                doc["license"]       = tech->documentation.license;
                doc["changelog"]     = tech->documentation.changelog;
                doc["contributing"]  = tech->documentation.contributing;
                doc["security"]      = tech->documentation.security;
                doc["codeOfConduct"] = tech->documentation.codeOfConduct;
                ta["documentation"]  = doc;

                data["technologyAnalysis"] = ta;
                Logger::instance().info(
                    "Analysis response enriched with technology analysis for job=" + jobId
                    + " type=" + tech->repositoryType);
            } else {
                data["technologyAnalysis"] = Json::Value(Json::nullValue);
            }
        }

        // ── Repository health ─────────────────────────────────────────────
        if (healthService_) {
            auto health = healthService_->getHealth(jobId);
            if (health) {
                auto serializeCat = [](const cortex::health::CategoryScore& cs) {
                    Json::Value v;
                    v["score"]    = cs.score;
                    v["maxScore"] = cs.maxScore;
                    v["grade"]    = cs.grade;
                    return v;
                };

                Json::Value cats;
                cats["documentation"]   = serializeCat(health->categories.documentation);
                cats["testing"]         = serializeCat(health->categories.testing);
                cats["ciCd"]            = serializeCat(health->categories.ciCd);
                cats["security"]        = serializeCat(health->categories.security);
                cats["maintainability"] = serializeCat(health->categories.maintainability);
                cats["configuration"]   = serializeCat(health->categories.configuration);
                cats["projectStructure"]= serializeCat(health->categories.projectStructure);

                auto toJsonArr = [](const std::vector<std::string>& v) {
                    Json::Value a(Json::arrayValue);
                    for (const auto& s : v) a.append(s);
                    return a;
                };

                Json::Value rh;
                rh["overallScore"]    = health->overallScore;
                rh["grade"]           = health->grade;
                rh["categories"]      = cats;
                rh["strengths"]       = toJsonArr(health->strengths);
                rh["warnings"]        = toJsonArr(health->warnings);
                rh["recommendations"] = toJsonArr(health->recommendations);

                data["repositoryHealth"] = rh;
                Logger::instance().info(
                    "Analysis response enriched with health score for job=" + jobId
                    + " score=" + std::to_string(health->overallScore)
                    + " grade=" + health->grade);
            } else {
                data["repositoryHealth"] = Json::Value(Json::nullValue);
            }
        }

        // ── Repository insights ───────────────────────────────────────────
        if (insightService_) {
            auto insights = insightService_->getInsights(jobId);
            if (insights) {
                auto toArr = [](const std::vector<std::string>& v) {
                    Json::Value a(Json::arrayValue);
                    for (const auto& s : v) a.append(s);
                    return a;
                };

                Json::Value ri;
                ri["summary"]              = insights->summary;
                ri["technologyOverview"]   = insights->technologyOverview;
                ri["qualityOverview"]      = insights->qualityOverview;
                ri["estimatedProjectSize"] = insights->estimatedProjectSize;
                ri["estimatedMaturity"]    = insights->estimatedMaturity;
                ri["estimatedComplexity"]  = insights->estimatedComplexity;
                ri["strengths"]            = toArr(insights->strengths);
                ri["risks"]                = toArr(insights->risks);
                ri["suggestions"]          = toArr(insights->suggestions);

                data["repositoryInsights"] = ri;
                Logger::instance().info(
                    "Analysis response enriched with insights for job=" + jobId);
            } else {
                data["repositoryInsights"] = Json::Value(Json::nullValue);
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
