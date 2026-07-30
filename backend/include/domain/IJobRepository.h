/**
 * @file IJobRepository.h
 * @brief Abstract interface defining the storage contract for repository analysis jobs
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
#include <memory>
#include <optional>
#include <vector>

namespace cortex::domain {

/**
 * Repository interface for Job storage
 * 
 * This abstraction allows different storage implementations (in-memory, PostgreSQL, etc.)
 * without changing the service layer. Following the Dependency Inversion Principle.
 */
class IJobRepository {
public:
    virtual ~IJobRepository() = default;

    /**
     * Store a job
     * @param job The job to store
     */
    virtual void save(const Job& job) noexcept = 0;

    /**
     * Retrieve a job by ID
     * @param jobId The job ID to look up
     * @return The job if found, std::nullopt otherwise
     */
    virtual std::optional<Job> findById(const std::string& jobId) const noexcept = 0;

    /**
     * Get all jobs (for listing/monitoring)
     * @return Vector of all stored jobs
     */
    virtual std::vector<Job> findAll() const noexcept = 0;

    /**
     * Get next queued job for processing (dequeue from processing queue)
     * @return The oldest queued job, or std::nullopt if queue is empty
     */
    virtual std::optional<Job> dequeueNextJob() noexcept = 0;

    /**
     * Update job status (worker uses this to track progress)
     * @param jobId The job ID to update
     * @param newStatus The new status
     * @return true if update succeeded, false if job not found
     */
    virtual bool updateStatus(const std::string& jobId, JobStatus newStatus) noexcept = 0;

    /**
     * Set the timestamp when job processing started
     * @param jobId The job ID to update
     * @param timestamp The start time
     * @return true if update succeeded, false if job not found
     */
    virtual bool setStartedAt(const std::string& jobId, 
                             std::chrono::system_clock::time_point timestamp) noexcept = 0;

    /**
     * Set the timestamp when job processing completed
     * @param jobId The job ID to update
     * @param timestamp The completion time
     * @return true if update succeeded, false if job not found
     */
    virtual bool setCompletedAt(const std::string& jobId,
                               std::chrono::system_clock::time_point timestamp) noexcept = 0;

    /**
     * Delete copy operations (prevent slicing)
     */
    IJobRepository(const IJobRepository&) = delete;
    IJobRepository& operator=(const IJobRepository&) = delete;

protected:
    IJobRepository() = default;
};

} // namespace cortex::domain
