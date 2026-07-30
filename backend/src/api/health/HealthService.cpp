/**
 * @file HealthService.cpp
 * @brief Returns current service health metadata including process uptime
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

#include "api/health/HealthService.h"
#include "config/Config.h"
#include "logging/Logger.h"
#include <chrono>
#include <iomanip>
#include <sstream>
#include <ctime>

namespace cortex::api::health {

using cortex::logging::Logger;

HealthService::HealthService() noexcept
    : startupTime_(std::chrono::steady_clock::now()) {
    Logger::instance().info("HealthService initialized");
}

HealthResponse HealthService::getHealth() const noexcept {
    try {
        // Calculate uptime in seconds
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - startupTime_);
        uint64_t uptimeSeconds = static_cast<uint64_t>(elapsed.count());

        // Get application metadata from Config singleton
        auto& config = cortex::config::Config::instance();
        
        HealthResponse response;
        response.status = std::string(HealthResponse::STATUS_UP);
        response.service = std::string(config.applicationName());
        response.version = std::string(config.version());
        response.environment = std::string(
            config.isDevelopment() ? "Development" : 
            config.isTesting() ? "Testing" : 
            "Production"
        );
        response.uptimeSeconds = uptimeSeconds;
        response.timestamp = generateTimestamp();

        return response;
    } catch (...) {
        // Return degraded status on error
        HealthResponse response;
        response.status = std::string(HealthResponse::STATUS_DEGRADED);
        response.service = "Cortex Code Intelligence Platform";
        response.version = "1.0.0";
        response.environment = "Unknown";
        response.uptimeSeconds = 0;
        response.timestamp = generateTimestamp();
        
        Logger::instance().error("Error computing health status");
        return response;
    }
}

std::string HealthService::generateTimestamp() noexcept {
    try {
        // Get current system time
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        
        // Convert to UTC
        std::tm utc_tm;
        #ifdef _WIN32
            gmtime_s(&utc_tm, &time_t_now);
        #else
            gmtime_r(&time_t_now, &utc_tm);
        #endif
        
        // Format as ISO8601 UTC: YYYY-MM-DDTHH:MM:SSZ
        std::ostringstream oss;
        oss << std::put_time(&utc_tm, "%Y-%m-%dT%H:%M:%SZ");
        return oss.str();
    } catch (...) {
        // Fallback timestamp on error
        return "1970-01-01T00:00:00Z";
    }
}

} // namespace cortex::api::health
