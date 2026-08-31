/**
 * @file RepositoryController.cpp
 * @brief Implementation of the repository submission HTTP endpoint
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

#include "api/repositories/RepositoryController.h" 
#include "api/repositories/RepositoryRequest.h"
#include "api/repositories/RepositoryResponse.h"
#include "logging/Logger.h"
#include <drogon/HttpResponse.h>
#include <json/json.h>

namespace cortex::api::repositories {

using cortex::logging::Logger;

void RepositoryController::submitRepository(const drogon::HttpRequestPtr& req,
                                            std::function<void(const drogon::HttpResponsePtr&)>&& callback) const noexcept {
    try {
        // Log incoming request
        Logger::instance().info("Incoming POST /repositories");

        // Validate HTTP method (should be POST)
        if (req->getMethod() != drogon::HttpMethod::Post) {
            Logger::instance().warn("Invalid HTTP method for /repositories: " + 
                                    std::string(req->methodString()));
            
            auto response = drogon::HttpResponse::newHttpResponse();
            response->setStatusCode(drogon::k405MethodNotAllowed);
            callback(response);
            return;
        }

        // Parse JSON body
        auto jsonBody = req->getJsonObject();
        if (!jsonBody) {
            Logger::instance().warn("POST /repositories: invalid JSON body");
            
            Json::Value error;
            error["success"] = false;
            error["message"] = "Invalid request body. Expected JSON.";
            
            auto response = drogon::HttpResponse::newHttpJsonResponse(error);
            response->setStatusCode(drogon::k400BadRequest);
            callback(response);
            return;
        }

        // Extract repository URL from request
        std::string repositoryUrl;
        if (jsonBody->isMember("repositoryUrl")) {
            repositoryUrl = jsonBody->get("repositoryUrl", "").asString();
        }

        // Create request DTO
        RepositoryRequest request(repositoryUrl);

        // Call service to submit repository
        auto result = service_->submitRepository(request);

        if (result.status == SubmitRepositoryStatus::INVALID_REQUEST) {
            Logger::instance().warn("Repository submission failed validation");

            Json::Value error;
            error["success"] = false;
            error["message"] = "Invalid repository URL. Must be HTTPS GitHub or GitLab repository.";

            auto response = drogon::HttpResponse::newHttpJsonResponse(error);
            response->setStatusCode(drogon::k400BadRequest);
            callback(response);
            return;
        }

        if (result.status == SubmitRepositoryStatus::BACKPRESSURED) {
            Logger::instance().warn("Repository submission rejected due to backpressure");

            Json::Value error;
            error["success"] = false;
            error["message"] = "System is busy. Please retry shortly.";

            auto response = drogon::HttpResponse::newHttpJsonResponse(error);
            response->setStatusCode(drogon::k429TooManyRequests);
            callback(response);
            return;
        }

        if (result.status == SubmitRepositoryStatus::INTERNAL_ERROR || !result.job.has_value()) {
            Logger::instance().error("Repository submission failed due to internal dispatch error");

            Json::Value error;
            error["success"] = false;
            error["message"] = "Internal server error";

            auto response = drogon::HttpResponse::newHttpJsonResponse(error);
            response->setStatusCode(drogon::k500InternalServerError);
            callback(response);
            return;
        }

        // Successfully accepted - return 202 Accepted
        const auto& job = result.job.value();
        RepositoryResponse response(job);
        
        auto httpResponse = drogon::HttpResponse::newHttpJsonResponse(response.toJson());
        httpResponse->setStatusCode(drogon::k202Accepted);

        Logger::instance().info(std::string("Repository submission accepted. Job ID: ") + job.getId());
        callback(httpResponse);

    } catch (const std::exception& e) {
        // Log error (don't expose exception text to client)
        Logger::instance().error(std::string("Exception in submitRepository handler: ") + e.what());

        // Return generic error response (HTTP 500)
        Json::Value error;
        error["success"] = false;
        error["message"] = "Internal server error";
        
        auto response = drogon::HttpResponse::newHttpJsonResponse(error);
        response->setStatusCode(drogon::k500InternalServerError);
        callback(response);

    } catch (...) {
        // Log unknown error
        Logger::instance().critical("Unknown exception in submitRepository handler");

        // Return generic error response (HTTP 500)
        Json::Value error;
        error["success"] = false;
        error["message"] = "Internal server error";
        
        auto response = drogon::HttpResponse::newHttpJsonResponse(error);
        response->setStatusCode(drogon::k500InternalServerError);
        callback(response);
    }
}

} // namespace cortex::api::repositories
