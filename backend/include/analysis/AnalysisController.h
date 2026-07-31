#pragma once

#include "analysis/AnalysisService.h"
#include "github/GitHubMetadataService.h"
#include "technology/TechnologyService.h"
#include "health/RepositoryHealthService.h"
#include "insight/RepositoryInsightService.h"
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <functional>
#include <memory>
#include <string>

namespace cortex::analysis {

class AnalysisController {
public:
    explicit AnalysisController(
        std::shared_ptr<AnalysisService> service,
        std::shared_ptr<cortex::github::GitHubMetadataService> metadataService = nullptr,
        std::shared_ptr<cortex::technology::TechnologyService> technologyService = nullptr,
        std::shared_ptr<cortex::health::RepositoryHealthService> healthService = nullptr,
        std::shared_ptr<cortex::insight::RepositoryInsightService> insightService = nullptr) noexcept
        : service_(std::move(service))
        , metadataService_(std::move(metadataService))
        , technologyService_(std::move(technologyService))
        , healthService_(std::move(healthService))
        , insightService_(std::move(insightService))
    {}

    void getAnalysis(
        const drogon::HttpRequestPtr& req,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
        const std::string& jobId) const noexcept;

private:
    std::shared_ptr<AnalysisService> service_;
    std::shared_ptr<cortex::github::GitHubMetadataService> metadataService_;
    std::shared_ptr<cortex::technology::TechnologyService> technologyService_;
    std::shared_ptr<cortex::health::RepositoryHealthService> healthService_;
    std::shared_ptr<cortex::insight::RepositoryInsightService> insightService_;
};

} // namespace cortex::analysis
