/**
 * @file RepositoryResponse.h
 * @brief Serializes a newly created job into the POST /repositories HTTP response
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

#include "domain/Job.h"
#include <json/json.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <ctime>

namespace cortex::api::repositories {

/**
 * Repository response DTO
 * Represents the JSON response for POST /repositories
 * 
 * Converts Job domain model to HTTP response format
 */
class RepositoryResponse {
public:
    explicit RepositoryResponse(const cortex::domain::Job& job) noexcept
        : job_(job) {}

    /**
     * Convert response to JSON format
     * 
     * Response structure:
     * {
     *   "success": true,
     *   "message": "Repository accepted for analysis.",
     *   "data": {
     *     "jobId": "uuid",
     *     "status": "QUEUED"
     *   }
     * }
     */
    Json::Value toJson() const noexcept {
        try {
            Json::Value root;
            root["success"] = true;
            root["message"] = "Repository accepted for analysis.";

            Json::Value data;
            data["jobId"] = job_.getId();
            data["status"] = std::string(cortex::domain::jobStatusToString(job_.getStatus()));

            root["data"] = data;
            return root;
        } catch (...) {
            // Return error JSON if conversion fails
            Json::Value error;
            error["success"] = false;
            error["message"] = "Internal server error";
            return error;
        }
    }

    // Delete copy/move
    RepositoryResponse(const RepositoryResponse&) = default;
    RepositoryResponse& operator=(const RepositoryResponse&) = default;
    RepositoryResponse(RepositoryResponse&&) = default;
    RepositoryResponse& operator=(RepositoryResponse&&) = default;

private:
    const cortex::domain::Job& job_;
};

} // namespace cortex::api::repositories
