/**
 * @file JobController.cpp
 * @brief Implementation of the job status HTTP endpoint
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

#include "api/jobs/JobController.h"
#include "logging/Logger.h"

namespace cortex::api::jobs {

using cortex::logging::Logger;

void JobController::getJob(const drogon::HttpRequestPtr& req,
                          std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                          const std::string& jobId) const noexcept {
    try {
        // Validate HTTP method
        if (req->getMethod() != drogon::HttpMethod::Get) {
            Logger::instance().warn("Invalid method for GET /jobs/{jobId}");
            
            Json::Value error;
            error["success"] = false;
            error["message"] = "Method not allowed";
            
            auto response = drogon::HttpResponse::newHttpJsonResponse(error);
            response->setStatusCode(drogon::k405MethodNotAllowed);
            callback(response);
            return;
        }

        Logger::instance().info(std::string("Incoming GET /jobs/{jobId}: ") + jobId);

        // Query service for job
        auto job = service_->getJob(jobId);

        if (job) {
            // Job found - return 200 OK with job details
            JobResponse response(job.value());
            auto httpResponse = drogon::HttpResponse::newHttpJsonResponse(response.toJson());
            httpResponse->setStatusCode(drogon::k200OK);
            callback(httpResponse);
        } else {
            // Job not found - return 404
            Logger::instance().info(std::string("Returning 404 for job: ") + jobId);
            
            auto httpResponse = drogon::HttpResponse::newHttpJsonResponse(JobResponse::notFoundResponse(jobId));
            httpResponse->setStatusCode(drogon::k404NotFound);
            callback(httpResponse);
        }

    } catch (const std::exception& e) {
        Logger::instance().error(std::string("Error in GET /jobs/{jobId}: ") + e.what());
        
        Json::Value error;
        error["success"] = false;
        error["message"] = "Internal server error";
        
        auto response = drogon::HttpResponse::newHttpJsonResponse(error);
        response->setStatusCode(drogon::k500InternalServerError);
        callback(response);
    }
}

} // namespace cortex::api::jobs
