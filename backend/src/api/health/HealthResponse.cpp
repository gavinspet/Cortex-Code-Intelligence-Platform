/**
 * @file HealthResponse.cpp
 * @brief Constructs the health check JSON response with uptime, version, and environment metadata
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

#include "api/health/HealthResponse.h"

namespace cortex::api::health {

Json::Value HealthResponse::toJson() const noexcept {
    Json::Value root;
    
    // Top-level response structure
    root["success"] = true;
    root["message"] = "Service is healthy";
    
    // Data object with health details
    Json::Value data;
    data["status"] = status;
    data["service"] = service;
    data["version"] = version;
    data["environment"] = environment;
    data["uptimeSeconds"] = static_cast<Json::Value::UInt64>(uptimeSeconds);
    data["timestamp"] = timestamp;
    
    root["data"] = data;
    
    return root;
}

} // namespace cortex::api::health
