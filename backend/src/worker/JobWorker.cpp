/**
 * @file JobWorker.cpp
 * @brief Background thread loop: git clone, std::filesystem scan, analysis storage, and status updates
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
    std::shared_ptr<cortex::analysis::IAnalysisRepository> analysisRepository) noexcept
    : repository_(std::move(repository)),
      analysisRepository_(std::move(analysisRepository))
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

    while (!shutdown_requested_.load()) {
        try {
            // Wait for notification or timeout
            std::unique_lock<std::mutex> lock(work_mutex_);
            work_cv_.wait_for(lock, std::chrono::seconds(1), [this] {
                return shutdown_requested_.load() || 
                       repository_->dequeueNextJob().has_value();
            });

            // Check if shutdown was requested during wait
            if (shutdown_requested_.load()) {
                break;
            }

            // Try to get next job
            auto job = repository_->dequeueNextJob();
            if (job) {
                lock.unlock();  // Release lock before processing
                processJob(job.value());
            }

        } catch (const std::exception& e) {
            cortex::logging::Logger::instance().error(
                std::string("Worker error: ") + e.what());
        }
    }

    cortex::logging::Logger::instance().info("Worker thread exiting");
}

void JobWorker::processJob(const cortex::domain::Job& job) noexcept {
    try {
        const std::string& jobId = job.getId();
        const std::string& repoUrl = job.getRepositoryUrl();

        // Log job start
        cortex::logging::Logger::instance().info(
            std::string("Job dequeued: ") + jobId);

        // Update status to RUNNING and set startedAt timestamp
        auto now = std::chrono::system_clock::now();
        if (!repository_->updateStatus(jobId, cortex::domain::JobStatus::RUNNING)) {
            cortex::logging::Logger::instance().warn(
                std::string("Failed to update job status to RUNNING: ") + jobId);
            return;
        }
        if (!repository_->setStartedAt(jobId, now)) {
            cortex::logging::Logger::instance().warn(
                std::string("Failed to set startedAt timestamp: ") + jobId);
        }

        cortex::logging::Logger::instance().info(
            std::string("Job started processing: ") + jobId);

        // Run real git clone and analysis
        analyzeRepository(jobId, repoUrl);

        // Update status to COMPLETED and set completedAt timestamp
        auto completedNow = std::chrono::system_clock::now();
        if (!repository_->updateStatus(jobId, cortex::domain::JobStatus::COMPLETED)) {
            cortex::logging::Logger::instance().warn(
                std::string("Failed to update job status to COMPLETED: ") + jobId);
            return;
        }
        if (!repository_->setCompletedAt(jobId, completedNow)) {
            cortex::logging::Logger::instance().warn(
                std::string("Failed to set completedAt timestamp: ") + jobId);
        }

        cortex::logging::Logger::instance().info(
            std::string("Job completed successfully: ") + jobId);

    } catch (const std::exception& e) {
        cortex::logging::Logger::instance().error(
            std::string("Error processing job: ") + e.what());
    }
}

void JobWorker::analyzeRepository(const std::string& jobId, const std::string& repoUrl) noexcept {
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
            return;
        }
        while (fgets(buf.data(), buf.size(), pipe.get())) {
            output += buf.data();
        }
        int exitCode = pclose(pipe.release());

        if (exitCode != 0) {
            cortex::logging::Logger::instance().error(
                "git clone failed for job " + jobId + ": " + output);
            repository_->updateStatus(jobId, cortex::domain::JobStatus::FAILED);
            return;
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

    } catch (const std::exception& e) {
        cortex::logging::Logger::instance().error(
            std::string("Error in analyzeRepository: ") + e.what());
        repository_->updateStatus(jobId, cortex::domain::JobStatus::FAILED);
    }
}

} // namespace cortex::worker
