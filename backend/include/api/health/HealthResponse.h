/**
 * @file HealthResponse.h
 * @brief Serializes health check metadata into a structured JSON HTTP response
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

#include <string>
#include <cstdint>
#include <json/json.h>

namespace cortex::api::health {

/**
 * @struct HealthResponse
 * @brief Health status response data structure
 * 
 * Represents the current health status of the application.
 * Contains metadata about the service, version, environment, and uptime.
 * 
 * Responsibility:
 * - Hold health status information
 * - Serialize to JSON for HTTP response
 * - Maintain strong typing (no magic strings)
 */
struct HealthResponse {
    // Status enum values
    static constexpr std::string_view STATUS_UP = "UP";
    static constexpr std::string_view STATUS_DOWN = "DOWN";
    static constexpr std::string_view STATUS_DEGRADED = "DEGRADED";

    std::string status;           // Health status (UP, DOWN, DEGRADED)
    std::string service;          // Service name (e.g., "Cortex Code Intelligence Platform")
    std::string version;          // Service version (e.g., "1.0.0")
    std::string environment;      // Deployment environment (Development, Testing, Production)
    uint64_t uptimeSeconds;       // Seconds since application startup
    std::string timestamp;        // ISO8601 UTC timestamp (e.g., "2026-07-30T08:45:23Z")

    /**
     * Convert response to JSON object for HTTP response body
     * 
     * Format:
     * {
     *   "success": true,
     *   "message": "Service is healthy",
     *   "data": {
     *     "status": "UP",
     *     "service": "Cortex Code Intelligence Platform",
     *     "version": "1.0.0",
     *     "environment": "Development",
     *     "uptimeSeconds": 123,
     *     "timestamp": "2026-07-30T08:45:23Z"
     *   }
     * }
     * 
     * @return JSON::Value suitable for drogon HTTP response
     */
    Json::Value toJson() const noexcept;
};

} // namespace cortex::api::health
