/**
 * @file RepositoryService.cpp
 * @brief Validates repository URLs, creates jobs, persists them, and notifies the background worker
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

#include "api/repositories/RepositoryService.h"
#include "worker/WorkerService.h"
#include "utils/UrlValidator.h"
#include "utils/UuidGenerator.h"
#include <chrono>

namespace cortex::api::repositories {

using cortex::logging::Logger;
using cortex::utils::UrlValidator;
using cortex::utils::UuidGenerator;
using cortex::domain::Job;
using cortex::domain::JobStatus;

SubmitRepositoryResult RepositoryService::submitRepository(const RepositoryRequest& request) const noexcept {
    try {
        const auto submitStart = std::chrono::steady_clock::now();

        // Log incoming submission
        Logger::instance().info("Incoming repository submission request");

        // Validate URL
        std::string_view url = request.getRepositoryUrl();
        
        if (request.isEmpty()) {
            Logger::instance().warn("Repository submission rejected: empty URL");
            return {SubmitRepositoryStatus::INVALID_REQUEST, std::nullopt};
        }

        if (!UrlValidator::isValidRepositoryUrl(url)) {
            Logger::instance().warn(std::string("Repository submission rejected: invalid URL - ") + 
                                   std::string(url));
            return {SubmitRepositoryStatus::INVALID_REQUEST, std::nullopt};
        }

        // Generate UUID for this job
        std::string jobId = UuidGenerator::generate();
        Logger::instance().info(std::string("Generated job ID: ") + jobId);

        // Create job
        auto createdAt = std::chrono::system_clock::now();
        Job job(jobId, std::string(url), JobStatus::QUEUED, createdAt);

        // Store job in repository
        const auto mysqlSaveStart = std::chrono::steady_clock::now();
        jobRepository_->save(job);
        const auto mysqlSaveEnd = std::chrono::steady_clock::now();
        const auto mysqlSaveMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            mysqlSaveEnd - mysqlSaveStart).count();
        Logger::instance().info(
            "submit_timing jobId=" + jobId + " mysql_save_ms=" + std::to_string(mysqlSaveMs));
        Logger::instance().info(std::string("Job stored in repository: ") + jobId);

        if (dispatchQueue_) {
            const auto publishStart = std::chrono::steady_clock::now();
            if (!dispatchQueue_->publishJob(jobId)) {
                const auto publishEnd = std::chrono::steady_clock::now();
                const auto publishMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                    publishEnd - publishStart).count();
                Logger::instance().warn(
                    "submit_timing jobId=" + jobId + " redis_publish_ms=" + std::to_string(publishMs) +
                    " status=failed");
                Logger::instance().warn(
                    std::string("Backpressure or dispatch publish failure for job: ") + jobId);
                return {SubmitRepositoryStatus::BACKPRESSURED, std::nullopt};
            }
            const auto publishEnd = std::chrono::steady_clock::now();
            const auto publishMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                publishEnd - publishStart).count();
            Logger::instance().info(
                "submit_timing jobId=" + jobId + " redis_publish_ms=" + std::to_string(publishMs) +
                " status=ok");
            Logger::instance().info(std::string("Job published to Redis stream: ") + jobId);
        }

        // Notify worker of new job
        if (workerService_) {
            workerService_->notifyJobAvailable();
            Logger::instance().info("Worker notified of new job");
        } else {
            Logger::instance().warn("Worker service not available for notifications");
        }

        // Log submission accepted
        Logger::instance().info(std::string("Repository accepted for analysis: ") + std::string(url));
        const auto submitEnd = std::chrono::steady_clock::now();
        const auto totalMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            submitEnd - submitStart).count();
        Logger::instance().info("submit_timing jobId=" + jobId + " total_ms=" + std::to_string(totalMs));

        return {SubmitRepositoryStatus::ACCEPTED, job};

    } catch (const std::exception& e) {
        Logger::instance().error(std::string("Exception in submitRepository: ") + e.what());
        return {SubmitRepositoryStatus::INTERNAL_ERROR, std::nullopt};
    } catch (...) {
        Logger::instance().critical("Unknown exception in submitRepository");
        return {SubmitRepositoryStatus::INTERNAL_ERROR, std::nullopt};
    }
}

} // namespace cortex::api::repositories
