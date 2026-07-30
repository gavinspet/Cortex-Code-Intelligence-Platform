/**
 * @file Application.cpp
 * @brief Wires the dependency graph, initializes Drogon, registers HTTP routes, and manages application lifecycle
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

#include "app/Application.h"
#include "logging/Logger.h"
#include <drogon/HttpAppFramework.h>
#include <drogon/drogon.h>
#include <drogon/HttpResponse.h>
#include <json/json.h>
#include <csignal>
#include <atomic>
#include <functional>

namespace cortex::app {

using cortex::logging::Logger;

// Static application instance for handler registration
static Application* g_appInstance = nullptr;

// Handler wrapper using explicit lambda to work with Drogon's template system
// Drogon can properly introspect this since it matches the expected callback pattern
class HealthHandler {
public:
    void handle(const drogon::HttpRequestPtr& req,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback) const {
        try {
            if (g_appInstance && g_appInstance->healthController_) {
                callback(g_appInstance->healthController_->handleHealth(req));
            } else {
                // Build JSON response manually (jsoncpp doesn't support initializer list syntax)
                Json::Value error;
                error["success"] = false;
                error["message"] = "Health service not available";
                
                auto response = drogon::HttpResponse::newHttpJsonResponse(error);
                response->setStatusCode(drogon::k500InternalServerError);
                callback(response);
            }
        } catch (const std::exception& e) {
            cortex::logging::Logger::instance().error(std::string("Error in health handler: ") + e.what());
            
            Json::Value error;
            error["success"] = false;
            error["message"] = "Internal server error";
            
            auto response = drogon::HttpResponse::newHttpJsonResponse(error);
            response->setStatusCode(drogon::k500InternalServerError);
            callback(response);
        }
    }
};

static HealthHandler healthHandler;

// Handler wrapper for repository endpoint
class RepositoryHandler {
public:
    void submitRepository(const drogon::HttpRequestPtr& req,
                         std::function<void(const drogon::HttpResponsePtr&)>&& callback) const {
        try {
            if (g_appInstance && g_appInstance->repositoryController_) {
                g_appInstance->repositoryController_->submitRepository(req, std::move(callback));
            } else {
                Json::Value error;
                error["success"] = false;
                error["message"] = "Repository service not available";
                
                auto response = drogon::HttpResponse::newHttpJsonResponse(error);
                response->setStatusCode(drogon::k500InternalServerError);
                callback(response);
            }
        } catch (const std::exception& e) {
            cortex::logging::Logger::instance().error(std::string("Error in repository handler: ") + e.what());
            
            Json::Value error;
            error["success"] = false;
            error["message"] = "Internal server error";
            
            auto response = drogon::HttpResponse::newHttpJsonResponse(error);
            response->setStatusCode(drogon::k500InternalServerError);
            callback(response);
        }
    }
};

static RepositoryHandler repositoryHandler;

// Handler wrapper for job query endpoint
class JobHandler {
public:
    void getJob(const drogon::HttpRequestPtr& req,
               std::function<void(const drogon::HttpResponsePtr&)>&& callback,
               const std::string& jobId) const {
        try {
            if (g_appInstance && g_appInstance->jobController_) {
                g_appInstance->jobController_->getJob(req, std::move(callback), jobId);
            } else {
                Json::Value error;
                error["success"] = false;
                error["message"] = "Job service not available";
                
                auto response = drogon::HttpResponse::newHttpJsonResponse(error);
                response->setStatusCode(drogon::k500InternalServerError);
                callback(response);
            }
        } catch (const std::exception& e) {
            cortex::logging::Logger::instance().error(std::string("Error in job handler: ") + e.what());
            
            Json::Value error;
            error["success"] = false;
            error["message"] = "Internal server error";
            
            auto response = drogon::HttpResponse::newHttpJsonResponse(error);
            response->setStatusCode(drogon::k500InternalServerError);
            callback(response);
        }
    }
};

static JobHandler jobHandler;

// Handler for GET /analysis/{jobId}
class AnalysisHandler {
public:
    void getAnalysis(const drogon::HttpRequestPtr& req,
                    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                    const std::string& jobId) const {
        try {
            if (g_appInstance && g_appInstance->analysisController_) {
                g_appInstance->analysisController_->getAnalysis(req, std::move(callback), jobId);
            } else {
                Json::Value error;
                error["success"] = false;
                error["message"] = "Analysis service not available";
                auto response = drogon::HttpResponse::newHttpJsonResponse(error);
                response->setStatusCode(drogon::k500InternalServerError);
                callback(response);
            }
        } catch (const std::exception& e) {
            cortex::logging::Logger::instance().error(
                std::string("Error in analysis handler: ") + e.what());
            Json::Value error;
            error["success"] = false;
            error["message"] = "Internal server error";
            auto response = drogon::HttpResponse::newHttpJsonResponse(error);
            response->setStatusCode(drogon::k500InternalServerError);
            callback(response);
        }
    }
};

static AnalysisHandler analysisHandler;
static std::atomic<bool> shutdown_requested(false);

// Signal handler
static void signalHandler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        shutdown_requested.store(true);
        drogon::app().quit();  // Gracefully stop the Drogon server
    }
}

Application::Application(ConfigurationPtr config)
    : config_(config) {
    if (!config_) {
        throw std::invalid_argument("Configuration cannot be null");
    }
    
    // Set static instance for handler registration
    g_appInstance = this;
}

Application::~Application() = default;

void Application::buildDependencyGraph() noexcept {
    try {
        Logger::instance().info("Building dependency graph...");
        
        // Health services
        // 1. Create HealthService (demonstrates service layer pattern)
        healthService_ = std::make_shared<HealthService>();
        Logger::instance().info("Registered HealthService");
        
        // 2. Create HealthController (demonstrates controller layer pattern)
        healthController_ = std::make_shared<HealthController>(healthService_);
        Logger::instance().info("Registered HealthController");

        // Repository services
        // 3. Create job repository — try MySQL, fall back to InMemory
        bool mysqlOk = cortex::database::Database::instance().initialize();
        if (mysqlOk) {
            jobRepository_ = std::make_shared<cortex::infrastructure::MySQLJobRepository>();
            Logger::instance().info("Using MySQLJobRepository");
        } else {
            Logger::instance().warn("MySQL unavailable — falling back to InMemoryJobRepository");
            jobRepository_ = std::make_shared<cortex::infrastructure::InMemoryJobRepository>();
        }
        
        // 4. Create RepositoryService (business logic layer)
        repositoryService_ = std::make_shared<RepositoryService>(jobRepository_, 
                                                                  nullptr);  // Will set workerService after creating it
        Logger::instance().info("Registered RepositoryService");
        
        // 5. Create RepositoryController (HTTP handler layer)
        repositoryController_ = std::make_shared<RepositoryController>(repositoryService_);
        Logger::instance().info("Registered RepositoryController");

        // Job query services
        // 6. Create JobService (query business logic layer)
        jobService_ = std::make_shared<JobService>(jobRepository_);
        Logger::instance().info("Registered JobService");
        
        // 7. Create JobController (HTTP handler layer for job queries)
        jobController_ = std::make_shared<JobController>(jobService_);
        Logger::instance().info("Registered JobController");

        // Background worker services
        // 8. Create analysis repository (in-memory)
        analysisRepository_ = std::make_shared<cortex::analysis::InMemoryAnalysisRepository>();
        Logger::instance().info("Registered InMemoryAnalysisRepository");

        // 9. Create JobWorker (background processing)
        jobWorker_ = std::make_shared<JobWorker>(jobRepository_, analysisRepository_);
        Logger::instance().info("Registered JobWorker");
        
        // 10. Create WorkerService (worker lifecycle management)
        workerService_ = std::make_shared<WorkerService>(jobWorker_);
        Logger::instance().info("Registered WorkerService");

        // 11. Create AnalysisService and AnalysisController
        analysisService_ = std::make_shared<cortex::analysis::AnalysisService>(analysisRepository_);
        analysisController_ = std::make_shared<cortex::analysis::AnalysisController>(analysisService_);
        Logger::instance().info("Registered AnalysisService and AnalysisController");
        
        // Now set the workerService in repositoryService for notifications
        if (repositoryService_) {
            repositoryService_->setWorkerService(workerService_);
            Logger::instance().info("Wired WorkerService into RepositoryService");
        }
        
        Logger::instance().info("Dependency graph built successfully");
    } catch (const std::exception& e) {
        Logger::instance().error(std::string("Failed to build dependency graph: ") + e.what());
    }
}

int Application::run() noexcept {
    try {
        logStartupInfo();

        // Build dependency graph (DB init attempted inside)
        buildDependencyGraph();

        initializeDrogon();
        registerRoutes();

        // Start background worker before HTTP server
        if (workerService_) {
            workerService_->start();
            Logger::instance().info("Background job worker started");
        }

        // Register signal handlers for graceful shutdown
        std::signal(SIGINT, signalHandler);
        std::signal(SIGTERM, signalHandler);

        auto& app = drogon::HttpAppFramework::instance();

        // Start the framework (blocking)
        Logger::instance().info("Starting HTTP server...");
        app.run();

        // Stop background worker after HTTP server stops
        if (workerService_) {
            workerService_->stop();
            Logger::instance().info("Background job worker stopped");
        }

        // Shutdown database gracefully
        cortex::database::Database::instance().shutdown();

        logShutdownInfo();
        return 0;
    } catch (const std::exception& e) {
        Logger::instance().critical(std::string("Application error: ") + e.what());
        cortex::database::Database::instance().shutdown();
        return 1;
    } catch (...) {
        Logger::instance().critical("Unknown application error");
        cortex::database::Database::instance().shutdown();
        return 1;
    }
}

void Application::initializeDrogon() {
    auto& app = drogon::HttpAppFramework::instance();

    // Read configuration
    auto host = config_->getString("server.host").value_or("127.0.0.1");
    auto port = config_->getUInt("server.port").value_or(8080);
    auto threads = config_->getUInt("server.threads").value_or(4);

    // Configure Drogon
    app.addListener(host, port);
    app.setThreadNum(threads);

    Logger::instance().info("Drogon configured: host=" + host + ", port=" + std::to_string(port) + 
             ", threads=" + std::to_string(threads));
}

void Application::logStartupInfo() {
    Logger::instance().info("========================================");
    Logger::instance().info("Cortex Code Intelligence Platform");
    Logger::instance().info("Production Foundation");
    Logger::instance().info("========================================");
    Logger::instance().info("Loading configuration...");
}

void Application::logShutdownInfo() {
    Logger::instance().info("========================================");
    Logger::instance().info("Server shutting down gracefully...");
    Logger::instance().info("========================================");
}

void Application::registerRoutes() {
    try {
        Logger::instance().info("Registering HTTP routes...");

        // Register GET /health endpoint using handler member function
        // Bind to the handler instance's method
        drogon::app().registerHandler(
            "/health",
            [](const drogon::HttpRequestPtr& req,
               std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
                healthHandler.handle(req, std::move(callback));
            },
            {drogon::HttpMethod::Get}
        );
        
        Logger::instance().info("Registered route: GET /health");

        // Register POST /repositories endpoint using handler member function
        drogon::app().registerHandler(
            "/repositories",
            [](const drogon::HttpRequestPtr& req,
               std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
                repositoryHandler.submitRepository(req, std::move(callback));
            },
            {drogon::HttpMethod::Post}
        );
        
        Logger::instance().info("Registered route: POST /repositories");
        
        // Register GET /jobs/{jobId} endpoint using handler member function
        drogon::app().registerHandler(
            "/jobs/{jobId}",
            [](const drogon::HttpRequestPtr& req,
               std::function<void(const drogon::HttpResponsePtr&)>&& callback,
               const std::string& jobId) {
                jobHandler.getJob(req, std::move(callback), jobId);
            },
            {drogon::HttpMethod::Get}
        );
        
        Logger::instance().info("Registered route: GET /jobs/{jobId}");

        // Register GET /analysis/{jobId} endpoint
        drogon::app().registerHandler(
            "/analysis/{jobId}",
            [](const drogon::HttpRequestPtr& req,
               std::function<void(const drogon::HttpResponsePtr&)>&& callback,
               const std::string& jobId) {
                analysisHandler.getAnalysis(req, std::move(callback), jobId);
            },
            {drogon::HttpMethod::Get}
        );

        Logger::instance().info("Registered route: GET /analysis/{jobId}");
        Logger::instance().info("HTTP routes registered successfully");

    } catch (const std::exception& e) {
        Logger::instance().error(std::string("Failed to register routes: ") + e.what());
    }
}

} // namespace cortex::app
