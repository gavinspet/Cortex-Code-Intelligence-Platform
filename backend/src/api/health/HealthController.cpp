/**
 * @file HealthController.cpp
 * @brief Implementation of the health check HTTP endpoint
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

#include "api/health/HealthController.h"
#include "logging/Logger.h"
#include <drogon/HttpResponse.h>

namespace cortex::api::health {

using cortex::logging::Logger;

drogon::HttpResponsePtr HealthController::handleHealth(const drogon::HttpRequestPtr& req) const noexcept {
    try {
        // Log incoming request
        Logger::instance().info("Incoming GET /health");

        // Validate request method (should be GET)
        if (req->getMethod() != drogon::HttpMethod::Get) {
            Logger::instance().warn("Invalid HTTP method for /health: " + 
                                    std::string(req->methodString()));
            
            auto response = drogon::HttpResponse::newHttpResponse();
            response->setStatusCode(drogon::k405MethodNotAllowed);
            return response;
        }

        // Call service to compute health status
        HealthResponse healthResponse = service_->getHealth();

        // Convert response to JSON
        Json::Value jsonResponse = healthResponse.toJson();

        // Create HTTP response
        auto response = drogon::HttpResponse::newHttpJsonResponse(jsonResponse);
        response->setStatusCode(drogon::k200OK);

        // Log success
        Logger::instance().info("Health endpoint served successfully");

        return response;

    } catch (const std::exception& e) {
        // Log error (don't expose exception text to client)
        Logger::instance().error(std::string("Exception in health endpoint: ") + e.what());

        // Return generic error response (HTTP 500)
        Json::Value error;
        error["success"] = false;
        error["message"] = "Internal server error";
        
        auto response = drogon::HttpResponse::newHttpJsonResponse(error);
        response->setStatusCode(drogon::k500InternalServerError);
        return response;

    } catch (...) {
        // Log unknown error
        Logger::instance().critical("Unknown exception in health endpoint");

        // Return generic error response (HTTP 500)
        Json::Value error;
        error["success"] = false;
        error["message"] = "Internal server error";
        
        auto response = drogon::HttpResponse::newHttpJsonResponse(error);
        response->setStatusCode(drogon::k500InternalServerError);
        return response;
    }
}

} // namespace cortex::api::health
