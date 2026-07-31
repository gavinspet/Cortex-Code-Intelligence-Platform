#pragma once

#include "analysis/AnalysisService.h"
#include "github/GitHubMetadataService.h"
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
        std::shared_ptr<cortex::github::GitHubMetadataService> metadataService = nullptr) noexcept
        : service_(std::move(service))
        , metadataService_(std::move(metadataService))
    {}

    void getAnalysis(
        const drogon::HttpRequestPtr& req,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
        const std::string& jobId) const noexcept;

private:
    std::shared_ptr<AnalysisService> service_;
    std::shared_ptr<cortex::github::GitHubMetadataService> metadataService_;
};

} // namespace cortex::analysis
