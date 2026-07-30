/**
 * @file AnalysisController.h
 * @brief HTTP handler for GET /analysis/{jobId} — returns code analysis results for a completed job
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

#include "analysis/AnalysisService.h"
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <functional>
#include <memory>
#include <string>

namespace cortex::analysis {

class AnalysisController {
public:
    explicit AnalysisController(std::shared_ptr<AnalysisService> service) noexcept
        : service_(std::move(service)) {}

    void getAnalysis(
        const drogon::HttpRequestPtr& req,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
        const std::string& jobId) const noexcept;

private:
    std::shared_ptr<AnalysisService> service_;
};

} // namespace cortex::analysis
