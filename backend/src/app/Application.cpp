#include "app/Application.h"
#include <drogon/HttpAppFramework.h>
#include <csignal>
#include <atomic>

namespace cortex::app {

// Global flag for graceful shutdown
static std::atomic<bool> shutdown_requested(false);

// Signal handler
static void signalHandler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        shutdown_requested.store(true);
        drogon::app().quit();  // Gracefully stop the Drogon server
    }
}

Application::Application(ConfigurationPtr config, LoggerPtr logger)
    : config_(config), logger_(logger) {
    if (!config_) {
        throw std::invalid_argument("Configuration cannot be null");
    }
    if (!logger_) {
        throw std::invalid_argument("Logger cannot be null");
    }
    
    // Build dependency graph during construction
    // This ensures all services are registered before run() is called
    buildDependencyGraph();
}

Application::~Application() = default;

void Application::buildDependencyGraph() noexcept {
    try {
        logger_->info("Building dependency graph...");
        
        // Register core infrastructure services
        
        // 1. Register Configuration as a singleton
        // Services can now inject Configuration via:
        //   auto config = container.resolve<ConfigurationPtr>();
        // But this requires making Configuration interface more public
        // For now, configuration is accessed via Application::getConfig()
        
        // 2. Register Logger as a singleton (already available via Application::getLogger())
        // Logger is managed by Application, not ServiceContainer
        // This is appropriate as Logger is infrastructure, not a business service
        
        // 3. Future services will be registered here:
        // container.registerSingleton<IUserService>([](const ServiceContainer& c) {
        //     auto logger = c.resolve<ILogger>();
        //     return std::make_shared<UserService>(logger);
        // });
        
        logger_->info("Dependency graph built successfully");
        logger_->info("Total services registered: (extension point)");
    } catch (const std::exception& e) {
        logger_->error(std::string("Failed to build dependency graph: ") + e.what());
    }
}

int Application::run() noexcept {
    try {
        logStartupInfo();
        initializeDrogon();

        // Register signal handlers for graceful shutdown
        std::signal(SIGINT, signalHandler);
        std::signal(SIGTERM, signalHandler);

        auto& app = drogon::HttpAppFramework::instance();

        // Start the framework (blocking)
        logger_->info("Starting HTTP server...");
        app.run();

        logShutdownInfo();
        logger_->info("Application terminated successfully");
        return 0;
    } catch (const std::exception& e) {
        if (logger_) {
            logger_->critical(std::string("Application error: ") + e.what());
        }
        return 1;
    } catch (...) {
        if (logger_) {
            logger_->critical("Unknown application error");
        }
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

    logger_->info("Drogon configured: host=" + host + ", port=" + std::to_string(port) + 
                  ", threads=" + std::to_string(threads));
}

void Application::logStartupInfo() {
    logger_->info("========================================");
    logger_->info("Cortex Code Intelligence Platform");
    logger_->info("Production Foundation");
    logger_->info("========================================");
    logger_->info("Loading configuration...");
}

void Application::logShutdownInfo() {
    logger_->info("========================================");
    logger_->info("Server shutting down gracefully...");
    logger_->info("========================================");
}

} // namespace cortex::app
