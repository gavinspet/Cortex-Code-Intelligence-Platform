/**
 * @file JobService.cpp
 * @brief Retrieves job status from the repository and logs the lookup result
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

#include "api/jobs/JobService.h"

namespace cortex::api::jobs {

using cortex::logging::Logger;
using cortex::domain::Job;

std::optional<Job> JobService::getJob(const std::string& jobId) const noexcept {
    try {
        Logger::instance().info(std::string("Incoming request for job: ") + jobId);
        
        auto job = jobRepository_->findById(jobId);
        
        if (job) {
            Logger::instance().info(std::string("Job found: ") + jobId);
            return job;
        } else {
            Logger::instance().info(std::string("Job not found: ") + jobId);
            return std::nullopt;
        }
    } catch (const std::exception& e) {
        Logger::instance().error(std::string("Error retrieving job: ") + e.what());
        return std::nullopt;
    }
}

} // namespace cortex::api::jobs
