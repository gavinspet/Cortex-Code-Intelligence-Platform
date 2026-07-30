/**
 * @file HealthService.h
 * @brief Business logic for constructing health check metadata including uptime and environment
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

#include "api/health/HealthResponse.h"
#include <chrono>
#include <memory>

namespace cortex::api::health {

/**
 * @class HealthService
 * @brief Business logic for health status
 * 
 * Responsibility:
 * - Compute application health status
 * - Track application uptime
 * - Gather application metadata from Config
 * - Return structured health response
 * 
 * Design:
 * - Records startup time at construction (RAII)
 * - Computes uptime dynamically (no polling or updates needed)
 * - Immutable after construction (no mutable state)
 * - Thread-safe (const methods only)
 * 
 * Why Service Layer?
 * - Business logic (uptime calculation, health determination) belongs here
 * - Controllers should never compute logic, only validate/route
 * - Easy to test independently of HTTP handling
 * - Easy to reuse from different controllers (REST, gRPC, etc.)
 */
class HealthService {
public:
    /**
     * Create health service
     * 
     * Records current time as application startup time.
     * This time is used to compute uptime on each request.
     */
    HealthService() noexcept;

    // Delete copy operations (startup time is immutable per instance)
    HealthService(const HealthService&) = delete;
    HealthService& operator=(const HealthService&) = delete;

    // Allow move operations
    HealthService(HealthService&&) = default;
    HealthService& operator=(HealthService&&) = default;

    /**
     * Get current health status
     * 
     * Computes uptime from startup time.
     * Retrieves application metadata from Config module.
     * Returns structured health response.
     * 
     * @return HealthResponse with current status
     */
    HealthResponse getHealth() const noexcept;

private:
    /**
     * Generate ISO8601 UTC timestamp
     * 
     * Format: YYYY-MM-DDTHH:MM:SSZ
     * Example: 2026-07-30T08:45:23Z
     * 
     * @return ISO8601 timestamp string
     */
    static std::string generateTimestamp() noexcept;

    // Application startup time (recorded at construction)
    std::chrono::steady_clock::time_point startupTime_;
};

} // namespace cortex::api::health
