#pragma once

#include "config/Configuration.h"
#include "logging/Logger.h"
#include "core/di/ServiceContainer.h"
#include <memory>
#include <string>

namespace cortex::app {

using cortex::config::ConfigurationPtr;
using cortex::logging::LoggerPtr;
using cortex::core::di::ServiceContainer;

/**
 * @class Application
 * @brief Main application orchestrator with Dependency Injection.
 * 
 * Responsibilities:
 * 1. Build dependency graph
 * 2. Register all services in ServiceContainer
 * 3. Configure Drogon framework
 * 4. Start application
 * 5. Handle graceful shutdown
 * 
 * Design Pattern: Facade Pattern + Builder Pattern
 * - Facade: Hides Drogon + Config + Logging complexity
 * - Builder: Constructs dependency graph
 * 
 * SOLID Principles:
 * - Single Responsibility: Orchestrate startup/shutdown + DI setup
 * - Dependency Inversion: Depends on interfaces (Configuration, Logger)
 * - Interface Segregation: Only exposes necessary methods
 * - Open/Closed: New services added via ServiceContainer (extensible)
 * 
 * Why it exists:
 * - Centralizes DI configuration
 * - Ensures all services are properly registered
 * - Makes application structure explicit and discoverable
 * - Coordinates framework initialization
 * 
 * Why DI belongs here:
 * - Application is the highest-level orchestrator
 * - Only place where all services are known
 * - Builder pattern: Application "builds" the dependency graph
 * - Makes unit testing possible (can inject test ServiceContainer)
 * 
 * Lifecycle:
 * 1. Create via ApplicationFactory::create()
 * 2. Call buildDependencyGraph() to register all services
 * 3. Call run() to start server
 * 4. On Ctrl+C, calls shutdown and exits
 * 5. Destructor cleans up resources
 */
class Application {
public:
    /**
     * Initialize application with dependencies
     * @param config Configuration provider
     * @param logger Logger instance
     */
    Application(ConfigurationPtr config, LoggerPtr logger);

    ~Application();

    // Delete copy operations
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    // Allow move operations
    Application(Application&&) = default;
    Application& operator=(Application&&) = default;

    /**
     * Run the application (blocking call)
     * Returns when server is stopped (e.g., Ctrl+C)
     * @return Exit code (0 = success)
     */
    int run() noexcept;

    /**
     * Get configuration (for dependency injection in future)
     */
    ConfigurationPtr getConfig() const noexcept { return config_; }

    /**
     * Get logger (for dependency injection in future)
     */
    LoggerPtr getLogger() const noexcept { return logger_; }

    /**
     * Get the service container for dependency resolution
     * 
     * Used to access registered services:
     *   auto myService = app.getServiceContainer().resolve<IMyService>();
     */
    const ServiceContainer& getServiceContainer() const noexcept { 
        return serviceContainer_; 
    }

    /**
     * Get the service container for registration (before run())
     * 
     * Used to register additional services if needed:
     *   const_cast<ServiceContainer&>(app.getServiceContainer())
     *       .registerSingleton<INewService>([](const ServiceContainer& c) {
     *           return std::make_shared<NewService>(c.resolve<IDep>());
     *       });
     */
    ServiceContainer& getServiceContainer() noexcept { 
        return serviceContainer_; 
    }

private:
    ConfigurationPtr config_;
    LoggerPtr logger_;
    ServiceContainer serviceContainer_;

    /**
     * Build the dependency graph by registering all services
     * 
     * Called during initialization to set up all services.
     * New services should be registered here.
     * 
     * Currently registers:
     * - Configuration (singleton)
     * - Logger (singleton)
     * - Future: business services as they are created
     */
    void buildDependencyGraph() noexcept;

    /**
     * Initialize Drogon framework with settings from configuration
     */
    void initializeDrogon();

    /**
     * Log startup banner
     */
    void logStartupInfo();

    /**
     * Log shutdown message
     */
    void logShutdownInfo();
};

} // namespace cortex::app
