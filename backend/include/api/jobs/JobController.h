/**
 * @file JobController.h
 * @brief HTTP handler for GET /jobs/{jobId} — returns job lifecycle status and timestamps
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

#include "api/jobs/JobService.h"
#include "api/jobs/JobResponse.h"
#include <drogon/drogon.h>
#include <memory>

namespace cortex::api::jobs {

/**
 * Job HTTP controller
 * 
 * Handles GET /jobs/{jobId} endpoint.
 * Demonstrates clean separation:
 * - Controller validates HTTP concerns only
 * - Delegates business logic to service
 * - Service has no knowledge of HTTP
 */
class JobController {
public:
    /**
     * Create controller with service dependency
     * 
     * @param service JobService for business logic
     */
    explicit JobController(std::shared_ptr<JobService> service) noexcept
        : service_(std::move(service)) {}

    /**
     * Handle GET /jobs/{jobId} request
     * 
     * Path parameter:
     *   jobId - The job ID to retrieve
     * 
     * Response (HTTP 200):
     * {
     *   "success": true,
     *   "message": "Job found",
     *   "data": {
     *     "jobId": "uuid",
     *     "repositoryUrl": "https://github.com/user/repo.git",
     *     "status": "RUNNING",
     *     "createdAt": "2026-07-29T12:54:46Z",
     *     "startedAt": "2026-07-29T12:54:47Z",
     *     "completedAt": null
     *   }
     * }
     * 
     * Error Response (HTTP 404):
     * {
     *   "success": false,
     *   "message": "Job not found"
     * }
     * 
     * @param req HTTP request
     * @param callback Function to send response
     * @param jobId Job ID from URL path
     */
    void getJob(const drogon::HttpRequestPtr& req,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                const std::string& jobId) const noexcept;

    // Delete copy/move
    JobController(const JobController&) = delete;
    JobController& operator=(const JobController&) = delete;

private:
    std::shared_ptr<JobService> service_;
};

} // namespace cortex::api::jobs
