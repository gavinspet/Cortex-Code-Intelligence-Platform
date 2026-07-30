/**
 * @file JobResponse.h
 * @brief Serializes Job domain objects to JSON with ISO 8601 timestamp formatting
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

namespace cortex::api::jobs {

/**
 * Job response DTO
 * Represents the JSON response for GET /jobs/{jobId}
 * 
 * Converts Job domain model to HTTP response format
 * Includes all job details: ID, URL, status, and timestamps
 */
class JobResponse {
public:
    explicit JobResponse(const cortex::domain::Job& job) noexcept
        : job_(job) {}

    /**
     * Convert job to ISO8601 timestamp string
     * 
     * @param tp Time point to convert
     * @return ISO8601 formatted timestamp string
     */
    static std::string timePointToString(std::chrono::system_clock::time_point tp) noexcept {
        try {
            auto time_t_val = std::chrono::system_clock::to_time_t(tp);
            std::stringstream ss;
            ss << std::put_time(std::gmtime(&time_t_val), "%Y-%m-%dT%H:%M:%SZ");
            return ss.str();
        } catch (...) {
            return "";
        }
    }

    /**
     * Convert response to JSON format
     * 
     * Response structure (HTTP 200):
     * {
     *   "success": true,
     *   "message": "Job found",
     *   "data": {
     *     "jobId": "uuid",
     *     "repositoryUrl": "https://github.com/user/repo.git",
     *     "status": "RUNNING",
     *     "createdAt": "2026-07-29T12:54:46Z",
     *     "startedAt": "2026-07-29T12:54:47Z",  // null if not started
     *     "completedAt": null                    // null if not completed
     *   }
     * }
     * 
     * Not Found (HTTP 404):
     * {
     *   "success": false,
     *   "message": "Job not found"
     * }
     */
    Json::Value toJson() const noexcept {
        try {
            Json::Value root;
            root["success"] = true;
            root["message"] = "Job found";

            Json::Value data;
            data["jobId"] = job_.getId();
            data["repositoryUrl"] = job_.getRepositoryUrl();
            data["status"] = std::string(cortex::domain::jobStatusToString(job_.getStatus()));
            data["createdAt"] = timePointToString(job_.getCreatedAt());
            
            // Include startedAt if available
            if (job_.getStartedAt()) {
                data["startedAt"] = timePointToString(job_.getStartedAt().value());
            } else {
                data["startedAt"] = Json::Value(Json::nullValue);
            }
            
            // Include completedAt if available
            if (job_.getCompletedAt()) {
                data["completedAt"] = timePointToString(job_.getCompletedAt().value());
            } else {
                data["completedAt"] = Json::Value(Json::nullValue);
            }

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

    /**
     * Create not-found response
     * 
     * @param jobId The job ID that was not found
     * @return JSON response with 404 format
     */
    static Json::Value notFoundResponse(const std::string& /*jobId*/) noexcept {
        try {
            Json::Value root;
            root["success"] = false;
            root["message"] = "Job not found";
            return root;
        } catch (...) {
            Json::Value error;
            error["success"] = false;
            error["message"] = "Internal server error";
            return error;
        }
    }

    // Delete copy/move
    JobResponse(const JobResponse&) = default;
    JobResponse& operator=(const JobResponse&) = default;
    JobResponse(JobResponse&&) = default;
    JobResponse& operator=(JobResponse&&) = default;

private:
    const cortex::domain::Job& job_;
};

} // namespace cortex::api::jobs
