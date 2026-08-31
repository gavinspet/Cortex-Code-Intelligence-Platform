/**
 * @file WorkerService.h
 * @brief Lifecycle manager for the JobWorker background thread (start, stop, notify)
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

#include "worker/JobWorkerPool.h"
#include "domain/IJobRepository.h"
#include <memory>

namespace cortex::worker {

/**
 * Service wrapper for background job worker
 * 
 * Manages worker lifecycle within the application's dependency injection framework.
 * Enables Clean Architecture by encapsulating worker initialization and management.
 */
class WorkerService {
public:
    /**
     * Create worker service with injected dependencies
     * 
     * @param jobWorker The job worker instance to manage
     */
    explicit WorkerService(std::shared_ptr<JobWorkerPool> jobWorkerPool) noexcept
        : job_worker_pool_(std::move(jobWorkerPool)) {}

    /**
     * Start background job processing
     * Should be called during application startup after DI container initialization
     */
    void start() noexcept {
        if (job_worker_pool_) {
            job_worker_pool_->start();
        }
    }

    /**
     * Stop background job processing gracefully
     * Should be called during application shutdown
     * Waits for current job to complete before returning
     */
    void stop() noexcept {
        if (job_worker_pool_) {
            job_worker_pool_->stop();
        }
    }

    /**
     * Check if worker is running
     * @return true if background processing is active
     */
    bool isRunning() const noexcept {
        return job_worker_pool_ && job_worker_pool_->isRunning();
    }

    /**
     * Notify worker that a new job is available for processing
     * Called by RepositoryService after enqueueing a job
     */
    void notifyJobAvailable() noexcept {
        if (job_worker_pool_) {
            job_worker_pool_->notifyJobAvailable();
        }
    }

    /**
     * Get underlying job worker (for advanced operations)
     */
    std::shared_ptr<JobWorkerPool> getWorkerPool() const noexcept {
        return job_worker_pool_;
    }

    // Delete copy operations (service owns worker)
    WorkerService(const WorkerService&) = delete;
    WorkerService& operator=(const WorkerService&) = delete;
    WorkerService(WorkerService&&) = delete;
    WorkerService& operator=(WorkerService&&) = delete;

private:
    std::shared_ptr<JobWorkerPool> job_worker_pool_;
};

} // namespace cortex::worker
