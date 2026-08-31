#include "worker/JobWorker.h"
#include "domain/AnalysisResult.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <array>
#include <cstdio>

namespace cortex::worker {

JobWorker::JobWorker(
    std::shared_ptr<cortex::domain::IJobRepository> repository,
    std::shared_ptr<cortex::analysis::IAnalysisRepository> analysisRepository,
        std::shared_ptr<cortex::worker::IJobDispatchQueue> dispatchQueue,
        std::string consumerName,
    std::shared_ptr<cortex::github::GitHubMetadataService> metadataService,
    std::shared_ptr<cortex::technology::TechnologyService> technologyService,
    std::shared_ptr<cortex::health::RepositoryHealthService> healthService,
    std::shared_ptr<cortex::insight::RepositoryInsightService> insightService) noexcept
    : repository_(std::move(repository)),
      analysisRepository_(std::move(analysisRepository)),
            dispatchQueue_(std::move(dispatchQueue)),
            consumerName_(std::move(consumerName)),
      metadataService_(std::move(metadataService)),
      technologyService_(std::move(technologyService)),
      healthService_(std::move(healthService)),
      insightService_(std::move(insightService))
{
    cortex::logging::Logger::instance().info("JobWorker constructed");
}

JobWorker::~JobWorker() noexcept {
    stop();
}

void JobWorker::start() noexcept {
    try {
        if (running_.load()) {
            return;  // Already running
        }

        running_.store(true);
        shutdown_requested_.store(false);

        // Create and detach worker thread
        worker_thread_ = std::make_unique<std::thread>(&JobWorker::workerLoop, this);
        
        cortex::logging::Logger::instance().info("Background worker started");
    } catch (const std::exception& e) {
        cortex::logging::Logger::instance().error(
            std::string("Failed to start worker: ") + e.what());
    }
}

void JobWorker::stop() noexcept {
    try {
        if (!running_.load()) {
            return;  // Not running
        }

        // Signal worker to stop
        shutdown_requested_.store(true);
        work_cv_.notify_one();

        // Wait for worker thread to finish
        if (worker_thread_ && worker_thread_->joinable()) {
            worker_thread_->join();
        }

        running_.store(false);
        cortex::logging::Logger::instance().info("Background worker stopped gracefully");
    } catch (const std::exception& e) {
        cortex::logging::Logger::instance().error(
            std::string("Error stopping worker: ") + e.what());
    }
}

void JobWorker::notifyJobAvailable() noexcept {
    try {
        work_cv_.notify_one();
    } catch (...) {
        // Silently fail to maintain noexcept guarantee
    }
}

void JobWorker::workerLoop() noexcept {
    cortex::logging::Logger::instance().info("Worker thread started processing jobs");

    if (dispatchQueue_) {
        if (!dispatchQueue_->ensureConsumerGroup()) {
            cortex::logging::Logger::instance().error("Redis consumer group initialization failed");
        }
    }

    while (!shutdown_requested_.load()) {
        try {
            if (dispatchQueue_) {
                auto message = dispatchQueue_->consumeNext(consumerName_, 1000);
                if (!message.has_value()) {
                    continue;
                }

                auto job = repository_->findById(message->jobId);
                if (!job.has_value()) {
                    cortex::logging::Logger::instance().error(
                        std::string("Redis message references unknown jobId: ") + message->jobId);
                    if (!dispatchQueue_->ack(message->streamId)) {
                        cortex::logging::Logger::instance().error(
                            std::string("Failed to ack unknown-job message: ") + message->streamId);
                    }
                    continue;
                }

                const auto status = job->getStatus();
                if (status == cortex::domain::JobStatus::COMPLETED ||
                    status == cortex::domain::JobStatus::FAILED)
                {
                    if (!dispatchQueue_->ack(message->streamId)) {
                        cortex::logging::Logger::instance().error(
                            std::string("Failed to XACK terminal-job message: ") + message->streamId);
                    }
                    continue;
                }

                const bool processed = processJob(job.value());
                if (processed) {
                    if (!dispatchQueue_->ack(message->streamId)) {
                        cortex::logging::Logger::instance().error(
                            std::string("Failed to XACK message: ") + message->streamId);
                    }
                } else {
                    cortex::logging::Logger::instance().warn(
                        std::string("Job processing failed; leaving message pending for recovery: ") +
                        message->streamId);
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }

                continue;
            }

            // Wait for notification or periodic wake-up.
            std::unique_lock<std::mutex> lock(work_mutex_);
            work_cv_.wait_for(lock, std::chrono::seconds(1), [this] {
                return shutdown_requested_.load();
            });

            // Check if shutdown was requested during wait
            if (shutdown_requested_.load()) {
                break;
            }

            // Try to get next job
            lock.unlock();
            auto job = repository_->dequeueNextJob();
            if (job) {
                (void)processJob(job.value());
            }

        } catch (const std::exception& e) {
            cortex::logging::Logger::instance().error(
                std::string("Worker error: ") + e.what());
        }
    }

    cortex::logging::Logger::instance().info("Worker thread exiting");
}

bool JobWorker::processJob(const cortex::domain::Job& job) noexcept {
    try {
        const std::string& jobId = job.getId();
        const std::string& repoUrl = job.getRepositoryUrl();

        // Log job start
        cortex::logging::Logger::instance().info(
            std::string("Job dequeued: ") + jobId);

        // Update status to RUNNING unless this is a recovered in-flight message.
        auto now = std::chrono::system_clock::now();
        if (job.getStatus() == cortex::domain::JobStatus::RUNNING) {
            cortex::logging::Logger::instance().info(
                std::string("Resuming RUNNING job from Redis pending state: ") + jobId);
        } else {
            if (!repository_->updateStatus(jobId, cortex::domain::JobStatus::RUNNING)) {
                cortex::logging::Logger::instance().warn(
                    std::string("Failed to update job status to RUNNING: ") + jobId);
                return false;
            }
            if (!repository_->setStartedAt(jobId, now)) {
                cortex::logging::Logger::instance().warn(
                    std::string("Failed to set startedAt timestamp: ") + jobId);
            }
        }

        cortex::logging::Logger::instance().info(
            std::string("Job started processing: ") + jobId);

        // Run real git clone and analysis
        if (!analyzeRepository(jobId, repoUrl)) {
            return false;
        }

        // Update status to COMPLETED and set completedAt timestamp
        auto completedNow = std::chrono::system_clock::now();
        if (!repository_->updateStatus(jobId, cortex::domain::JobStatus::COMPLETED)) {
            cortex::logging::Logger::instance().warn(
                std::string("Failed to update job status to COMPLETED: ") + jobId);
            return false;
        }
        if (!repository_->setCompletedAt(jobId, completedNow)) {
            cortex::logging::Logger::instance().warn(
                std::string("Failed to set completedAt timestamp: ") + jobId);
        }

        cortex::logging::Logger::instance().info(
            std::string("Job completed successfully: ") + jobId);
        return true;

    } catch (const std::exception& e) {
        cortex::logging::Logger::instance().error(
            std::string("Error processing job: ") + e.what());
        return false;
    }
}

    bool JobWorker::analyzeRepository(const std::string& jobId, const std::string& repoUrl) noexcept {
    namespace fs = std::filesystem;
    try {
        const std::string workspaceBase = "/tmp/cortex-workspace";
        const std::string clonePath = workspaceBase + "/" + jobId;

        // Ensure URL ends with .git for git clone compatibility
        std::string cloneUrl = repoUrl;
        if (cloneUrl.size() < 4 || cloneUrl.substr(cloneUrl.size() - 4) != ".git") {
            cloneUrl += ".git";
        }

        fs::create_directories(workspaceBase);

        // Remove any previous clone for this jobId
        if (fs::exists(clonePath)) {
            fs::remove_all(clonePath);
        }

        cortex::logging::Logger::instance().info("Cloning repository: " + cloneUrl);

        // git clone --depth 1 into the clone path
        std::string cmd = "git clone --depth 1 --quiet " + cloneUrl + " " + clonePath + " 2>&1";

        std::array<char, 256> buf{};
        std::string output;
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
        if (!pipe) {
            cortex::logging::Logger::instance().error("Failed to run git clone for job: " + jobId);
            repository_->updateStatus(jobId, cortex::domain::JobStatus::FAILED);
            return false;
        }
        while (fgets(buf.data(), buf.size(), pipe.get())) {
            output += buf.data();
        }
        int exitCode = pclose(pipe.release());

        if (exitCode != 0) {
            cortex::logging::Logger::instance().error(
                "git clone failed for job " + jobId + ": " + output);
            repository_->updateStatus(jobId, cortex::domain::JobStatus::FAILED);
            return false;
        }

        cortex::logging::Logger::instance().info("Clone complete: " + clonePath);

        // Scan cloned repository
        cortex::domain::AnalysisResult result;
        result.jobId = jobId;
        result.clonePath = clonePath;
        result.analyzedAt = std::chrono::system_clock::now();

        std::error_code ec;
        for (auto& entry : fs::recursive_directory_iterator(clonePath, ec)) {
            // Skip .git directory
            auto rel = fs::relative(entry.path(), clonePath, ec);
            if (!rel.empty() && rel.begin()->string() == ".git") {
                continue;
            }

            if (entry.is_directory(ec)) {
                result.dirCount++;
            } else if (entry.is_regular_file(ec)) {
                result.fileCount++;

                // Count lines
                std::ifstream file(entry.path(), std::ios::binary);
                if (file) {
                    long long lines = std::count(
                        std::istreambuf_iterator<char>(file),
                        std::istreambuf_iterator<char>(), '\n');
                    result.totalLines += lines;
                }

                // Track extension
                std::string ext = entry.path().extension().string();
                if (!ext.empty()) {
                    result.languageDistribution[ext]++;
                } else {
                    result.languageDistribution["(none)"]++;
                }
            }
        }

        cortex::logging::Logger::instance().info(
            "Analysis complete for job " + jobId +
            ": files=" + std::to_string(result.fileCount) +
            " dirs=" + std::to_string(result.dirCount) +
            " lines=" + std::to_string(result.totalLines));

        if (analysisRepository_) {
            analysisRepository_->save(result);
        }

        // Fetch GitHub metadata asynchronously from the worker thread
        if (metadataService_) {
            cortex::logging::Logger::instance().info(
                "Fetching GitHub metadata for job " + jobId);
            auto metadata = metadataService_->fetchAndStore(jobId, repoUrl);
            if (metadata) {
                cortex::logging::Logger::instance().info(
                    "GitHub metadata stored for job " + jobId
                    + ": " + metadata->fullName
                    + " (★" + std::to_string(metadata->stars) + ")");
            }
        }
        // Run static technology detection on the cloned repository
        if (technologyService_) {
            cortex::logging::Logger::instance().info(
                "Technology detection starting for job " + jobId);
            auto tech = technologyService_->detectAndStore(jobId, clonePath);
            if (tech) {
                cortex::logging::Logger::instance().info(
                    "Technology detection done for job " + jobId
                    + ": type=" + tech->repositoryType
                    + " confidence=" + std::to_string(tech->confidenceScore));
            }
        }

        // Evaluate repository health and quality score
        if (healthService_) {
            cortex::logging::Logger::instance().info(
                "Repository health evaluation starting for job " + jobId);
            auto health = healthService_->evaluateAndStore(jobId, clonePath);
            if (health) {
                cortex::logging::Logger::instance().info(
                    "Repository health evaluated for job " + jobId
                    + ": score=" + std::to_string(health->overallScore)
                    + " grade=" + health->grade);
            }
        }

        // Generate human-readable repository insights (aggregates all layers)
        if (insightService_) {
            // Retrieve already-stored analysis results to pass as context
            std::optional<cortex::domain::AnalysisResult>         storedAnalysis;
            std::optional<cortex::github::GitHubMetadata>         storedMeta;
            std::optional<cortex::technology::TechnologyAnalysis> storedTech;
            std::optional<cortex::health::RepositoryHealthResult> storedHealth;

            if (analysisRepository_)  storedAnalysis = analysisRepository_->findByJobId(jobId);
            if (metadataService_)     storedMeta = metadataService_->getMetadata(jobId);
            if (technologyService_)   storedTech = technologyService_->getTechnology(jobId);
            if (healthService_)       storedHealth = healthService_->getHealth(jobId);

            cortex::logging::Logger::instance().info(
                "Repository insight generation starting for job " + jobId);

            auto insights = insightService_->generateAndStore(
                jobId,
                storedAnalysis ? &(*storedAnalysis) : nullptr,
                storedMeta     ? &(*storedMeta)     : nullptr,
                storedTech     ? &(*storedTech)     : nullptr,
                storedHealth   ? &(*storedHealth)   : nullptr);

            if (insights) {
                cortex::logging::Logger::instance().info(
                    "Repository insights generated for job " + jobId
                    + ": maturity=" + insights->estimatedMaturity
                    + " size=" + insights->estimatedProjectSize);
            }
        }
        return true;
    } catch (const std::exception& e) {
        cortex::logging::Logger::instance().error(
            std::string("Error in analyzeRepository: ") + e.what());
        repository_->updateStatus(jobId, cortex::domain::JobStatus::FAILED);
        return false;
    }
}

} // namespace cortex::worker
