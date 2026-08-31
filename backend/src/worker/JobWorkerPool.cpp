#include "worker/JobWorkerPool.h"
#include "logging/Logger.h"

namespace cortex::worker {

using cortex::logging::Logger;

JobWorkerPool::JobWorkerPool(
    size_t workerCount,
    std::shared_ptr<cortex::domain::IJobRepository> repository,
    std::shared_ptr<cortex::analysis::IAnalysisRepository> analysisRepository,
    std::shared_ptr<cortex::worker::IJobDispatchQueue> dispatchQueue,
    std::shared_ptr<cortex::github::GitHubMetadataService> metadataService,
    std::shared_ptr<cortex::technology::TechnologyService> technologyService,
    std::shared_ptr<cortex::health::RepositoryHealthService> healthService,
    std::shared_ptr<cortex::insight::RepositoryInsightService> insightService,
    std::string consumerPrefix) noexcept
    : repository_(std::move(repository)),
      analysisRepository_(std::move(analysisRepository)),
      dispatchQueue_(std::move(dispatchQueue)),
      metadataService_(std::move(metadataService)),
      technologyService_(std::move(technologyService)),
      healthService_(std::move(healthService)),
      insightService_(std::move(insightService)),
      workerCount_(workerCount),
      consumerPrefix_(std::move(consumerPrefix)) {}

JobWorkerPool::~JobWorkerPool() noexcept {
    stop();
}

void JobWorkerPool::start() noexcept {
    try {
        std::lock_guard<std::mutex> lock(workersMutex_);
        if (running_.load()) {
            return;
        }

        workers_.clear();
        workers_.reserve(workerCount_);

        for (size_t i = 0; i < workerCount_; ++i) {
            const std::string consumerName = consumerPrefix_ + std::to_string(i + 1);
            auto worker = std::make_shared<JobWorker>(
                repository_,
                analysisRepository_,
                dispatchQueue_,
                consumerName,
                metadataService_,
                technologyService_,
                healthService_,
                insightService_);
            workers_.push_back(worker);
        }

        running_.store(true);

        for (const auto& worker : workers_) {
            worker->start();
        }

        Logger::instance().info(
            "Job worker pool started with " + std::to_string(workerCount_) + " workers");
    } catch (const std::exception& e) {
        running_.store(false);
        Logger::instance().error(std::string("Failed to start worker pool: ") + e.what());
    }
}

void JobWorkerPool::stop() noexcept {
    try {
        std::vector<std::shared_ptr<JobWorker>> localWorkers;
        {
            std::lock_guard<std::mutex> lock(workersMutex_);
            if (!running_.load()) {
                return;
            }
            localWorkers = workers_;
        }

        for (const auto& worker : localWorkers) {
            worker->stop();
        }

        {
            std::lock_guard<std::mutex> lock(workersMutex_);
            workers_.clear();
            running_.store(false);
        }

        Logger::instance().info("Job worker pool stopped gracefully");
    } catch (const std::exception& e) {
        Logger::instance().error(std::string("Error stopping worker pool: ") + e.what());
    }
}

void JobWorkerPool::notifyJobAvailable() noexcept {
    try {
        std::lock_guard<std::mutex> lock(workersMutex_);
        for (const auto& worker : workers_) {
            worker->notifyJobAvailable();
        }
    } catch (...) {
        // Keep noexcept contract.
    }
}

} // namespace cortex::worker
