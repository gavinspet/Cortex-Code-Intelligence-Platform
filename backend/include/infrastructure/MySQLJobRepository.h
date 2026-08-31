/**
 * @file MySQLJobRepository.h
 * @brief MySQL-backed implementation of IJobRepository using prepared statements
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

#include "domain/IJobRepository.h"
#include <cppconn/resultset.h>
#include <memory>

namespace cortex::infrastructure {

/**
 * MySQL implementation of Job repository
 * 
 * Provides persistent storage for jobs using MySQL database.
 * Uses prepared statements for all SQL operations (no string concatenation).
 * 
 * Replaces InMemoryJobRepository for production deployments.
 * Service and Controller layers remain unchanged.
 * 
 * Design:
 * - Implements IJobRepository interface
 * - All methods thread-safe
 * - Prepared statements prevent SQL injection
 * - Proper error handling and logging
 * - No business logic, only storage operations
 */
class MySQLJobRepository : public cortex::domain::IJobRepository {
public:
    MySQLJobRepository() = default;

    /**
     * Store a job in MySQL
     * 
     * Inserts new job row with current timestamp.
     * @param job The job to store
     */
    void save(const cortex::domain::Job& job) noexcept override;

    /**
     * Retrieve a job by ID
     * 
     * @param jobId The job ID to look up
     * @return The job if found, std::nullopt otherwise
     */
    std::optional<cortex::domain::Job> findById(const std::string& jobId) const noexcept override;

    /**
     * Get all jobs
     * 
     * @return Vector of all stored jobs
     */
    std::vector<cortex::domain::Job> findAll() const noexcept override;

    /**
     * Get next queued job for processing
     * 
     * Returns oldest job with QUEUED status.
     * @return The oldest queued job, or std::nullopt if queue is empty
     */
    std::optional<cortex::domain::Job> dequeueNextJob() noexcept override;

    /**
     * Update job status
     * 
     * @param jobId The job ID to update
     * @param newStatus The new status
     * @return true if update succeeded, false if job not found
     */
    bool updateStatus(const std::string& jobId, cortex::domain::JobStatus newStatus) noexcept override;

    /**
     * Set the timestamp when job processing started
     * 
     * @param jobId The job ID to update
     * @param timestamp The start time
     * @return true if update succeeded, false if job not found
     */
    bool setStartedAt(const std::string& jobId, 
                     std::chrono::system_clock::time_point timestamp) noexcept override;

    /**
     * Set the timestamp when job processing completed
     * 
     * @param jobId The job ID to update
     * @param timestamp The completion time
     * @return true if update succeeded, false if job not found
     */
    bool setCompletedAt(const std::string& jobId,
                       std::chrono::system_clock::time_point timestamp) noexcept override;

    /**
     * Update clone operation details
     * 
     * @param jobId The job ID to update
     * @param repositoryPath Path where repository was cloned
     * @param durationMs Duration of clone operation in milliseconds
     * @param errorMessage Error message if clone failed (nullptr if successful)
     * @return true if update succeeded, false if job not found
     */
    bool updateCloneInfo(const std::string& jobId,
                        const std::string& repositoryPath,
                        long long durationMs,
                        const char* errorMessage = nullptr) noexcept;

    /**
     * Update job after error
     * 
     * @param jobId The job ID to update
     * @param errorMessage Error description
     * @return true if update succeeded, false if job not found
     */
    bool updateError(const std::string& jobId,
                    const std::string& errorMessage) noexcept;

    // Delete copy/move
    MySQLJobRepository(const MySQLJobRepository&) = delete;
    MySQLJobRepository& operator=(const MySQLJobRepository&) = delete;

private:
    /**
     * Convert timestamp to MySQL DATETIME string
     * 
     * @param tp Time point to convert
     * @return ISO8601 string for MySQL
     */
    static std::string timePointToDatetimeString(
        std::chrono::system_clock::time_point tp) noexcept;

    /**
     * Convert MySQL DATETIME string to time_point
     * 
     * @param dateStr MySQL DATETIME string
     * @return Converted time point
     */
    static std::chrono::system_clock::time_point datetimeStringToTimePoint(
        const std::string& dateStr) noexcept;

    /**
     * Build Job object from database row
     * 
     * @param resultSet Database result set
     * @return Job constructed from row data
     */
    static cortex::domain::Job buildJobFromResultSet(
        std::shared_ptr<sql::ResultSet>& resultSet);
};

} // namespace cortex::infrastructure
