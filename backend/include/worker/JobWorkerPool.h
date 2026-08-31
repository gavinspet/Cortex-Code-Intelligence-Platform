#pragma once

#include "domain/IJobRepository.h"
#include "analysis/IAnalysisRepository.h"
#include "worker/IJobDispatchQueue.h"
#include "worker/JobWorker.h"
#include "github/GitHubMetadataService.h"
#include "technology/TechnologyService.h"
#include "health/RepositoryHealthService.h"
#include "insight/RepositoryInsightService.h"
#include "observability/IMetrics.h"
#include "observability/ITracing.h"
#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace cortex::worker {

class JobWorkerPool {
public:
    JobWorkerPool(
        size_t workerCount,
        std::shared_ptr<cortex::domain::IJobRepository> repository,
        std::shared_ptr<cortex::analysis::IAnalysisRepository> analysisRepository,
        std::shared_ptr<cortex::worker::IJobDispatchQueue> dispatchQueue,
        std::shared_ptr<cortex::github::GitHubMetadataService> metadataService = nullptr,
        std::shared_ptr<cortex::technology::TechnologyService> technologyService = nullptr,
        std::shared_ptr<cortex::health::RepositoryHealthService> healthService = nullptr,
        std::shared_ptr<cortex::insight::RepositoryInsightService> insightService = nullptr,
        std::shared_ptr<cortex::observability::IMetrics> metrics = nullptr,
        std::shared_ptr<cortex::observability::ITracing> tracing = nullptr,
        std::string consumerPrefix = "cortex-worker-") noexcept;

    ~JobWorkerPool() noexcept;

    void start() noexcept;
    void stop() noexcept;
    void notifyJobAvailable() noexcept;

    bool isRunning() const noexcept { return running_.load(); }
    size_t workerCount() const noexcept { return workerCount_; }

    JobWorkerPool(const JobWorkerPool&) = delete;
    JobWorkerPool& operator=(const JobWorkerPool&) = delete;
    JobWorkerPool(JobWorkerPool&&) = delete;
    JobWorkerPool& operator=(JobWorkerPool&&) = delete;

private:
    std::shared_ptr<cortex::domain::IJobRepository> repository_;
    std::shared_ptr<cortex::analysis::IAnalysisRepository> analysisRepository_;
    std::shared_ptr<cortex::worker::IJobDispatchQueue> dispatchQueue_;
    std::shared_ptr<cortex::github::GitHubMetadataService> metadataService_;
    std::shared_ptr<cortex::technology::TechnologyService> technologyService_;
    std::shared_ptr<cortex::health::RepositoryHealthService> healthService_;
    std::shared_ptr<cortex::insight::RepositoryInsightService> insightService_;
    std::shared_ptr<cortex::observability::IMetrics> metrics_;
    std::shared_ptr<cortex::observability::ITracing> tracing_;

    size_t workerCount_;
    std::string consumerPrefix_;

    std::atomic<bool> running_{false};
    mutable std::mutex workersMutex_;
    std::vector<std::shared_ptr<JobWorker>> workers_;
};

} // namespace cortex::worker
