/**
 * @file InMemoryJobRepository.h
 * @brief Thread-safe in-memory implementation of IJobRepository for development and fallback use
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
#include <unordered_map>
#include <mutex>

namespace cortex::infrastructure {

/**
 * In-memory implementation of Job repository
 * 
 * Stores jobs in a hash map protected by mutex for thread-safety.
 * This is a temporary implementation for development.
 * 
 * Future implementations (PostgreSQL, etc.) will implement IJobRepository
 * without requiring changes to RepositoryService.
 */
class InMemoryJobRepository : public cortex::domain::IJobRepository {
public:
    InMemoryJobRepository() = default;

    /**
     * Store a job in memory
     * Thread-safe via mutex
     */
    void save(const cortex::domain::Job& job) noexcept override {
        try {
            std::lock_guard<std::mutex> lock(mutex_);
            // Store by job ID for O(1) lookup
            jobs_.emplace(job.getId(), job);
        } catch (...) {
            // Silently fail (no throwing in noexcept)
        }
    }

    /**
     * Find job by ID
     * Thread-safe via mutex
     */
    std::optional<cortex::domain::Job> findById(const std::string& jobId) const noexcept override {
        try {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = jobs_.find(jobId);
            if (it != jobs_.end()) {
                return it->second;
            }
            return std::nullopt;
        } catch (...) {
            return std::nullopt;
        }
    }

    /**
     * Get all jobs
     * Thread-safe via mutex
     */
    std::vector<cortex::domain::Job> findAll() const noexcept override {
        try {
            std::lock_guard<std::mutex> lock(mutex_);
            std::vector<cortex::domain::Job> result;
            result.reserve(jobs_.size());
            for (const auto& pair : jobs_) {
                result.push_back(pair.second);
            }
            return result;
        } catch (...) {
            return {};
        }
    }

    /**
     * Dequeue next job with QUEUED status for processing
     * Thread-safe via mutex
     * @return First QUEUED job found, or std::nullopt if queue is empty
     */
    std::optional<cortex::domain::Job> dequeueNextJob() noexcept override {
        try {
            std::lock_guard<std::mutex> lock(mutex_);
            // Find first job with QUEUED status
            for (auto& pair : jobs_) {
                if (pair.second.getStatus() == cortex::domain::JobStatus::QUEUED) {
                    return pair.second;
                }
            }
            return std::nullopt;
        } catch (...) {
            return std::nullopt;
        }
    }

    /**
     * Update job status
     * Thread-safe via mutex
     * @return true if job was found and updated, false otherwise
     */
    bool updateStatus(const std::string& jobId, cortex::domain::JobStatus newStatus) noexcept override {
        try {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = jobs_.find(jobId);
            if (it != jobs_.end()) {
                it->second.setStatus(newStatus);
                return true;
            }
            return false;
        } catch (...) {
            return false;
        }
    }

    /**
     * Set the timestamp when job processing started
     * Thread-safe via mutex
     * @return true if job was found and updated, false otherwise
     */
    bool setStartedAt(const std::string& jobId, 
                     std::chrono::system_clock::time_point timestamp) noexcept override {
        try {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = jobs_.find(jobId);
            if (it != jobs_.end()) {
                it->second.setStartedAt(timestamp);
                return true;
            }
            return false;
        } catch (...) {
            return false;
        }
    }

    /**
     * Set the timestamp when job processing completed
     * Thread-safe via mutex
     * @return true if job was found and updated, false otherwise
     */
    bool setCompletedAt(const std::string& jobId,
                       std::chrono::system_clock::time_point timestamp) noexcept override {
        try {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = jobs_.find(jobId);
            if (it != jobs_.end()) {
                it->second.setCompletedAt(timestamp);
                return true;
            }
            return false;
        } catch (...) {
            return false;
        }
    }

    // Delete copy/move operations (singleton-like repository)
    InMemoryJobRepository(const InMemoryJobRepository&) = delete;
    InMemoryJobRepository& operator=(const InMemoryJobRepository&) = delete;
    InMemoryJobRepository(InMemoryJobRepository&&) = default;
    InMemoryJobRepository& operator=(InMemoryJobRepository&&) = default;

private:
    // Mutable to allow modification in const methods (mutex lock management)
    mutable std::mutex mutex_;
    // Hash map for O(1) job lookup: jobId -> Job
    std::unordered_map<std::string, cortex::domain::Job> jobs_;
};

} // namespace cortex::infrastructure
