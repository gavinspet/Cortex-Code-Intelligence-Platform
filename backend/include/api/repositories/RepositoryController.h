/**
 * @file RepositoryController.h
 * @brief HTTP handler for POST /repositories — accepts repository submission and returns a job ID
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

#include "api/repositories/RepositoryService.h"
#include <drogon/drogon.h>
#include <memory>

namespace cortex::api::repositories {

/**
 * Repository HTTP controller
 * 
 * Handles POST /repositories endpoint.
 * Demonstrates clean separation:
 * - Controller validates HTTP concerns only
 * - Delegates business logic to service
 * - Service has no knowledge of HTTP
 */
class RepositoryController {
public:
    /**
     * Create controller with service dependency
     * 
     * @param service RepositoryService for business logic
     */
    explicit RepositoryController(std::shared_ptr<RepositoryService> service) noexcept
        : service_(std::move(service)) {}

    /**
     * Handle POST /repositories request
     * 
     * Request body:
     * {
     *   "repositoryUrl": "https://github.com/user/project.git"
     * }
     * 
     * Response (HTTP 202):
     * {
     *   "success": true,
     *   "message": "Repository accepted for analysis.",
     *   "data": {
     *     "jobId": "uuid",
     *     "status": "QUEUED"
     *   }
     * }
     * 
     * Error Response (HTTP 400):
     * {
     *   "success": false,
     *   "message": "Invalid repository URL"
     * }
     * 
     * @param req HTTP request
     * @param callback Function to send response
     */
    void submitRepository(const drogon::HttpRequestPtr& req,
                         std::function<void(const drogon::HttpResponsePtr&)>&& callback) const noexcept;

    // Delete copy/move
    RepositoryController(const RepositoryController&) = delete;
    RepositoryController& operator=(const RepositoryController&) = delete;
    RepositoryController(RepositoryController&&) = default;
    RepositoryController& operator=(RepositoryController&&) = default;

private:
    std::shared_ptr<RepositoryService> service_;
};

} // namespace cortex::api::repositories
