/**
 * @file Job.h
 * @brief Domain value object representing a repository analysis job and its complete lifecycle
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

#include <string>
#include <chrono>
#include <optional>

namespace cortex::domain {

/**
 * Job status enumeration
 * Tracks the lifecycle of a repository analysis job
 */
enum class JobStatus {
    QUEUED,      ///< Job created, waiting to be processed
    RUNNING,     ///< Job is currently being processed
    COMPLETED,   ///< Job completed successfully
    FAILED       ///< Job failed during processing
};

/**
 * Convert JobStatus to string representation
 * Used for logging and JSON serialization
 */
constexpr std::string_view jobStatusToString(JobStatus status) noexcept {
    switch (status) {
        case JobStatus::QUEUED:    return "QUEUED";
        case JobStatus::RUNNING:   return "RUNNING";
        case JobStatus::COMPLETED: return "COMPLETED";
        case JobStatus::FAILED:    return "FAILED";
    }
    return "UNKNOWN";
}

/**
 * Job domain model
 * Represents a repository analysis job in the system
 * 
 * Immutable after creation (const correctness enforced)
 */
class Job {
public:
    /**
     * Create a new job
     * 
     * @param id Unique identifier for this job (UUID)
     * @param repositoryUrl Git repository URL to analyze
     * @param status Initial job status (typically QUEUED)
     * @param createdAt Timestamp when job was created
     */
    explicit Job(std::string id, std::string repositoryUrl, JobStatus status,
                std::chrono::system_clock::time_point createdAt) noexcept
        : id_(std::move(id)), 
          repositoryUrl_(std::move(repositoryUrl)),
          status_(status),
          createdAt_(createdAt) {}

    // Const accessors
    const std::string& getId() const noexcept { return id_; }
    const std::string& getRepositoryUrl() const noexcept { return repositoryUrl_; }
    JobStatus getStatus() const noexcept { return status_; }
    std::chrono::system_clock::time_point getCreatedAt() const noexcept { return createdAt_; }
    
    /**
     * Get the timestamp when job processing started
     * @return Optional timestamp (empty until status changes to RUNNING)
     */
    std::optional<std::chrono::system_clock::time_point> getStartedAt() const noexcept { return startedAt_; }
    
    /**
     * Get the timestamp when job processing completed
     * @return Optional timestamp (empty until status changes to COMPLETED/FAILED)
     */
    std::optional<std::chrono::system_clock::time_point> getCompletedAt() const noexcept { return completedAt_; }

    // Mutable status update for worker processing
    void setStatus(JobStatus newStatus) noexcept { status_ = newStatus; }
    
    /**
     * Set the timestamp when job processing started
     * Called by worker when transitioning to RUNNING status
     */
    void setStartedAt(std::chrono::system_clock::time_point timestamp) noexcept { startedAt_ = timestamp; }
    
    /**
     * Set the timestamp when job processing completed
     * Called by worker when transitioning to COMPLETED/FAILED status
     */
    void setCompletedAt(std::chrono::system_clock::time_point timestamp) noexcept { completedAt_ = timestamp; }

    // Allow copy/move for storage in containers
    Job(const Job&) = default;
    Job& operator=(const Job&) = default;
    Job(Job&&) = default;
    Job& operator=(Job&&) = default;

private:
    std::string id_;
    std::string repositoryUrl_;
    JobStatus status_;
    std::chrono::system_clock::time_point createdAt_;
    std::optional<std::chrono::system_clock::time_point> startedAt_;
    std::optional<std::chrono::system_clock::time_point> completedAt_;
};

} // namespace cortex::domain
