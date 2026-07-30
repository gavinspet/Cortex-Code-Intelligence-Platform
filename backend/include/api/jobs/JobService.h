/**
 * @file JobService.h
 * @brief Business logic layer for retrieving job status from the repository by identifier
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
#include "domain/IJobRepository.h"
#include "logging/Logger.h"
#include <memory>
#include <optional>

namespace cortex::api::jobs {

/**
 * Job query service
 * 
 * Business logic for retrieving job information.
 * Demonstrates clean architecture:
 * - Retrieves job by ID
 * - No HTTP concerns
 * - Uses repository abstraction for storage
 * 
 * Dependency injection of IJobRepository allows:
 * - Easy testing with mock repositories
 * - Future migration to PostgreSQL without code changes
 */
class JobService {
public:
    /**
     * Create service with repository dependency
     * 
     * @param jobRepository Storage implementation (injected)
     */
    explicit JobService(std::shared_ptr<cortex::domain::IJobRepository> jobRepository) noexcept
        : jobRepository_(std::move(jobRepository)) {}

    /**
     * Get job by ID
     * 
     * @param jobId The job ID to retrieve
     * @return Job if found, std::nullopt if not found
     */
    std::optional<cortex::domain::Job> getJob(const std::string& jobId) const noexcept;

    // Delete copy/move
    JobService(const JobService&) = delete;
    JobService& operator=(const JobService&) = delete;
    JobService(JobService&&) = default;
    JobService& operator=(JobService&&) = default;

private:
    std::shared_ptr<cortex::domain::IJobRepository> jobRepository_;
};

} // namespace cortex::api::jobs
