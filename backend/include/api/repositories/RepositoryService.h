/**
 * @file RepositoryService.h
 * @brief Business logic for URL validation, job creation, and background worker notification
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

#include "api/repositories/RepositoryRequest.h"
#include "domain/Job.h"
#include "domain/IJobRepository.h"
#include "logging/Logger.h"
#include <memory>
#include <optional>

namespace cortex::worker {
// Forward declaration to avoid circular dependency
class WorkerService;
}

namespace cortex::api::repositories {

/**
 * Repository submission service
 * 
 * Business logic for handling repository submissions.
 * Demonstrates clean architecture:
 * - Validates input
 * - Interacts with domain models
 * - Uses repository abstraction for storage
 * - Notifies worker when job is enqueued
 * - No knowledge of HTTP concerns
 * 
 * Dependency injection of IJobRepository allows:
 * - Easy testing with mock repositories
 * - Future migration to PostgreSQL without code changes
 * 
 * Worker integration:
 * - Sets workerService after construction (circular dependency avoidance)
 * - Notifies worker of new jobs for background processing
 */
class RepositoryService {
public:
    /**
     * Create service with repository dependency
     * 
     * @param jobRepository Storage implementation (injected)
     * @param workerService Worker service for notifications (initially nullptr, set via setWorkerService)
     */
    explicit RepositoryService(
        std::shared_ptr<cortex::domain::IJobRepository> jobRepository,
        std::shared_ptr<cortex::worker::WorkerService> workerService = nullptr) noexcept
        : jobRepository_(std::move(jobRepository)),
          workerService_(std::move(workerService)) {}

    /**
     * Set worker service after construction (for DI circular dependency resolution)
     * 
     * @param workerService The worker service to notify on job submissions
     */
    void setWorkerService(std::shared_ptr<cortex::worker::WorkerService> workerService) noexcept {
        workerService_ = std::move(workerService);
    }

    /**
     * Submit a repository for analysis
     * 
     * Validates URL, creates job, stores it, notifies worker, and returns the job.
     * 
     * @param request Repository submission request
     * @return Job if successful, std::nullopt if validation fails
     */
    std::optional<cortex::domain::Job> submitRepository(const RepositoryRequest& request) const noexcept;

    // Delete copy/move
    RepositoryService(const RepositoryService&) = delete;
    RepositoryService& operator=(const RepositoryService&) = delete;
    RepositoryService(RepositoryService&&) = default;
    RepositoryService& operator=(RepositoryService&&) = default;

private:
    std::shared_ptr<cortex::domain::IJobRepository> jobRepository_;
    std::shared_ptr<cortex::worker::WorkerService> workerService_;
};

} // namespace cortex::api::repositories
