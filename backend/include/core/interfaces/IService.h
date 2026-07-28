#pragma once

namespace cortex::core::interfaces {

/**
 * @class IService
 * @brief Base interface for all injectable services.
 * 
 * Responsibility: Defines contract for any service in the application
 * 
 * SOLID: Interface Segregation (minimal interface)
 * Design Pattern: Strategy Pattern (allows multiple implementations)
 * 
 * Why it exists:
 * - Establishes common interface for all services
 * - Makes services injectable and testable
 * - Allows swapping implementations easily
 * - Enables clear service contracts
 * 
 * Why it belongs in core/interfaces/:
 * - Fundamental service abstraction
 * - Cross-cutting concern (all services implement this)
 * - Infrastructure-level, not business-specific
 * 
 * Lifetime: N/A (this is an interface)
 * 
 * Usage:
 *   class MyService : public IService {
 *   public:
 *       std::string getName() const noexcept override {
 *           return "MyService";
 *       }
 *   };
 */
class IService {
public:
    virtual ~IService() noexcept = default;

    /**
     * Get human-readable service name
     * Used for logging and diagnostics
     */
    virtual std::string getName() const noexcept = 0;
};

} // namespace cortex::core::interfaces
