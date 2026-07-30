/**
 * @file HealthController.h
 * @brief HTTP handler for GET /health — returns service status, version, and uptime
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

#include "api/health/HealthService.h"
#include <drogon/drogon.h>
#include <memory>

namespace cortex::api::health {

/**
 * @class HealthController
 * @brief REST API controller for health endpoint
 * 
 * Responsibility:
 * - Validate HTTP requests (GET /health)
 * - Route requests to service layer
 * - Return HTTP responses
 * - Log request/response lifecycle
 * - Handle errors gracefully
 * 
 * Why Controller is Not Business Logic:
 * - Controllers are HTTP infrastructure concerns
 * - Business logic (health computation) belongs in HealthService
 * - Controller only: validate input -> call service -> format output
 * - Service can be tested independently of HTTP
 * - Service can be called from REST, gRPC, CLI without changes
 * 
 * Design:
 * - Depends on HealthService (injected via constructor)
 * - All public methods are const (no state changes)
 * - Follows Dependency Inversion (depends on service abstraction)
 * 
 * Request Flow:
 * 1. Client: GET /health
 * 2. Drogon: Routes to HealthController::health()
 * 3. Controller: Logs incoming request
 * 4. Controller: Calls service.getHealth()
 * 5. Service: Computes health status
 * 6. Controller: Converts response to JSON
 * 7. Controller: Returns HTTP 200 + JSON
 * 8. Controller: Logs success
 */
class HealthController {
public:
    /**
     * Create health controller
     * 
     * @param service Health service instance (injected)
     */
    explicit HealthController(std::shared_ptr<HealthService> service) noexcept
        : service_(service) {
    }

    // Delete copy operations
    HealthController(const HealthController&) = delete;
    HealthController& operator=(const HealthController&) = delete;

    // Allow move operations
    HealthController(HealthController&&) = default;
    HealthController& operator=(HealthController&&) = default;

    /**
     * Handle GET /health request (synchronous version)
     * 
     * Validates request, calls service, returns health status as JSON.
     * 
     * Request:
     *   GET /health
     * 
     * Response (HTTP 200):
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
     * Error Response (HTTP 500):
     * {
     *   "success": false,
     *   "message": "Internal server error"
     * }
     * 
     * @param req HTTP request
     * @return HTTP response with health status
     */
    drogon::HttpResponsePtr handleHealth(const drogon::HttpRequestPtr& req) const noexcept;

private:
    std::shared_ptr<HealthService> service_;
};

} // namespace cortex::api::health
