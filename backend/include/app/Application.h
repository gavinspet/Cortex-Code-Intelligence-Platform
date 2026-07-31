#pragma once

#include "config/Configuration.h"
#include "core/di/ServiceContainer.h"
#include "api/health/HealthService.h"
#include "api/health/HealthController.h"
#include "api/repositories/RepositoryService.h"
#include "api/repositories/RepositoryController.h"
#include "api/jobs/JobService.h"
#include "api/jobs/JobController.h"
#include "analysis/InMemoryAnalysisRepository.h"
#include "analysis/AnalysisService.h"
#include "analysis/AnalysisController.h"
#include "infrastructure/MySQLJobRepository.h"
#include "infrastructure/InMemoryJobRepository.h"
#include "database/Database.h"
#include "worker/JobWorker.h"
#include "worker/WorkerService.h"
#include "github/GitHubClient.h"
#include "github/GitHubMetadataService.h"
#include "github/InMemoryGitHubMetadataRepository.h"
#include "technology/TechnologyService.h"
#include "technology/InMemoryTechnologyRepository.h"
#include "health/RepositoryHealthService.h"
#include "health/InMemoryRepositoryHealthRepository.h"
#include <drogon/drogon.h>
#include <memory>
#include <string>

namespace cortex::app {

// Forward declarations for friend classes
class HealthHandler;
class RepositoryHandler;
class JobHandler;
class AnalysisHandler;

using cortex::config::ConfigurationPtr;
using cortex::core::di::ServiceContainer;
using cortex::api::health::HealthService;
using cortex::api::health::HealthController;
using cortex::api::repositories::RepositoryService;
using cortex::api::repositories::RepositoryController;
using cortex::api::jobs::JobService;
using cortex::api::jobs::JobController;
using cortex::worker::JobWorker;
using cortex::worker::WorkerService;

/**
 * @class Application
 * @brief Main application orchestrator with Dependency Injection.
 * 
 * Responsibilities:
 * 1. Initialize logger singleton with application configuration
 * 2. Build dependency graph
 * 3. Register all services in ServiceContainer
 * 4. Configure Drogon framework
 * 5. Start application
 * 6. Handle graceful shutdown
 * 
 * Design Pattern: Facade Pattern + Builder Pattern
 * - Facade: Hides Drogon + Config + Logging complexity
 * - Builder: Constructs dependency graph
 * 
 * SOLID Principles:
 * - Single Responsibility: Orchestrate startup/shutdown + DI setup
 * - Dependency Inversion: Depends on interfaces (Configuration, Logger::instance())
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
 * Logger Access:
 * - Uses Logger::instance() (singleton)
 * - No logger dependency injection needed
 * - Initialized during ApplicationFactory::create()
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
     * Initialize application with configuration
     * @param config Configuration provider
     */
    explicit Application(ConfigurationPtr config);

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

    // Friend declaration for HTTP handler
    friend class HealthHandler;
    friend class RepositoryHandler;
    friend class JobHandler;
    friend class AnalysisHandler;
private:
    ConfigurationPtr config_;
    ServiceContainer serviceContainer_;
    
    // Health endpoint services
    std::shared_ptr<HealthService> healthService_;
    std::shared_ptr<HealthController> healthController_;
    
    // Repository endpoint services
    std::shared_ptr<cortex::domain::IJobRepository> jobRepository_;
    std::shared_ptr<RepositoryService> repositoryService_;
    std::shared_ptr<RepositoryController> repositoryController_;
    
    // Job query endpoint services
    std::shared_ptr<JobService> jobService_;
    std::shared_ptr<JobController> jobController_;

    // Background worker services
    std::shared_ptr<JobWorker> jobWorker_;
    std::shared_ptr<WorkerService> workerService_;

    // Analysis services
    std::shared_ptr<cortex::analysis::IAnalysisRepository> analysisRepository_;
    std::shared_ptr<cortex::analysis::AnalysisService> analysisService_;
    std::shared_ptr<cortex::analysis::AnalysisController> analysisController_;

    // GitHub metadata services
    std::shared_ptr<cortex::github::IGitHubMetadataRepository> metadataRepository_;
    std::shared_ptr<cortex::github::IGitHubClient> gitHubClient_;
    std::shared_ptr<cortex::github::GitHubMetadataService> gitHubMetadataService_;

    // Technology detection services
    std::shared_ptr<cortex::technology::ITechnologyRepository> technologyRepository_;
    std::shared_ptr<cortex::technology::TechnologyService> technologyService_;

    // Repository health services
    std::shared_ptr<cortex::health::IRepositoryHealthRepository> healthRepository_;
    std::shared_ptr<cortex::health::RepositoryHealthService> repoHealthService_;

    /**
     * Build the dependency graph by registering all services
     * 
     * Called during initialization to set up all services.
     * New services should be registered here.
     * 
     * Currently registers:
     * - Configuration (singleton)
     * - Logger (singleton)
     * - HealthService (business logic)
     * - Future: additional business services as they are created
     */
    void buildDependencyGraph() noexcept;

    /**
     * Initialize Drogon framework with settings from configuration
     */
    void initializeDrogon();

    /**
     * Register HTTP routes and handlers
     * 
     * Called after Drogon initialization.
     * Maps controller methods to HTTP endpoints.
     */
    void registerRoutes();

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
