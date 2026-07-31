#pragma once

#include "domain/IJobRepository.h"
#include "analysis/IAnalysisRepository.h"
#include "github/GitHubMetadataService.h"
#include "technology/TechnologyService.h"
#include "health/RepositoryHealthService.h"
#include "logging/Logger.h"
#include <thread>
#include <condition_variable>
#include <mutex>
#include <atomic>
#include <memory>
#include <chrono>

namespace cortex::worker {

/**
 * Background worker for processing queued repository analysis jobs
 * 
 * Implements producer-consumer pattern:
 * - RepositoryService (producer): enqueues jobs via repository
 * - JobWorker (consumer): dequeues and processes jobs in separate thread
 * 
 * Thread-safe communication via condition_variable and mutex.
 * Graceful shutdown via atomic flag and thread join.
 */
class JobWorker {
public:
    /**
     * Create worker with injected repository
     * 
     * @param repository Job repository for retrieving and updating jobs
     */
    explicit JobWorker(
        std::shared_ptr<cortex::domain::IJobRepository> repository,
        std::shared_ptr<cortex::analysis::IAnalysisRepository> analysisRepository,
        std::shared_ptr<cortex::github::GitHubMetadataService> metadataService = nullptr,
        std::shared_ptr<cortex::technology::TechnologyService> technologyService = nullptr,
        std::shared_ptr<cortex::health::RepositoryHealthService> healthService = nullptr) noexcept;

    /**
     * Destructor - ensures worker thread is stopped
     */
    ~JobWorker() noexcept;

    /**
     * Start the worker thread
     * Worker will begin processing QUEUED jobs immediately
     */
    void start() noexcept;

    /**
     * Signal worker to stop and wait for graceful shutdown
     * Stops accepting new work, finishes current job, then exits
     */
    void stop() noexcept;

    /**
     * Check if worker is running
     * @return true if worker thread is active and processing, false if stopped
     */
    bool isRunning() const noexcept { return running_.load(); }

    /**
     * Notify worker that new job is available
     * Called by RepositoryService when job is enqueued
     */
    void notifyJobAvailable() noexcept;

    // Delete copy operations (worker owns thread)
    JobWorker(const JobWorker&) = delete;
    JobWorker& operator=(const JobWorker&) = delete;
    JobWorker(JobWorker&&) = delete;
    JobWorker& operator=(JobWorker&&) = delete;

private:
    /**
     * Worker thread main loop
     * Continuously processes QUEUED jobs until stop is signaled
     */
    void workerLoop() noexcept;

    /**
     * Process a single job
     * Updates status to RUNNING, simulates analysis, updates to COMPLETED
     * 
     * @param job The job to process
     */
    void processJob(const cortex::domain::Job& job) noexcept;

    /**
     * Simulate analysis with logging and delay
     * 
     * @param repositoryUrl URL of repository being analyzed
     */
    void analyzeRepository(const std::string& jobId, const std::string& repoUrl) noexcept;

    // Dependencies (injected)
    std::shared_ptr<cortex::domain::IJobRepository> repository_;
    std::shared_ptr<cortex::analysis::IAnalysisRepository> analysisRepository_;
    std::shared_ptr<cortex::github::GitHubMetadataService> metadataService_;
    std::shared_ptr<cortex::technology::TechnologyService> technologyService_;
    std::shared_ptr<cortex::health::RepositoryHealthService> healthService_;

    // Thread management
    std::unique_ptr<std::thread> worker_thread_;
    
    // Synchronization primitives
    std::mutex work_mutex_;
    std::condition_variable work_cv_;
    
    // Control flags
    std::atomic<bool> running_{false};
    std::atomic<bool> shutdown_requested_{false};
};

} // namespace cortex::worker
