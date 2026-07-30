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

std::optional<Job> RepositoryService::submitRepository(const RepositoryRequest& request) const noexcept {
    try {
        // Log incoming submission
        Logger::instance().info("Incoming repository submission request");

        // Validate URL
        std::string_view url = request.getRepositoryUrl();
        
        if (request.isEmpty()) {
            Logger::instance().warn("Repository submission rejected: empty URL");
            return std::nullopt;
        }

        if (!UrlValidator::isValidRepositoryUrl(url)) {
            Logger::instance().warn(std::string("Repository submission rejected: invalid URL - ") + 
                                   std::string(url));
            return std::nullopt;
        }

        // Generate UUID for this job
        std::string jobId = UuidGenerator::generate();
        Logger::instance().info(std::string("Generated job ID: ") + jobId);

        // Create job
        auto createdAt = std::chrono::system_clock::now();
        Job job(jobId, std::string(url), JobStatus::QUEUED, createdAt);

        // Store job in repository
        jobRepository_->save(job);
        Logger::instance().info(std::string("Job stored in repository: ") + jobId);

        // Notify worker of new job
        if (workerService_) {
            workerService_->notifyJobAvailable();
            Logger::instance().info("Worker notified of new job");
        } else {
            Logger::instance().warn("Worker service not available for notifications");
        }

        // Log submission accepted
        Logger::instance().info(std::string("Repository accepted for analysis: ") + std::string(url));

        return job;

    } catch (const std::exception& e) {
        Logger::instance().error(std::string("Exception in submitRepository: ") + e.what());
        return std::nullopt;
    } catch (...) {
        Logger::instance().critical("Unknown exception in submitRepository");
        return std::nullopt;
    }
}

} // namespace cortex::api::repositories
