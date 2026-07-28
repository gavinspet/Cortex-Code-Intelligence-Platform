#include "app/ApplicationFactory.h"
#include <iostream>
#include <stdexcept>

/**
 * Main Entry Point
 * 
 * Cortex Code Intelligence Platform
 * Production-ready backend foundation
 * 
 * This main function is intentionally minimal:
 * - Uses ApplicationFactory to create the application with dependencies
 * - Delegates all initialization to Application (Facade)
 * - Delegates all configuration to external config.json
 * - Delegates all logging to structured logging system
 * 
 * Total lines: ~25 (< 40 requirement)
 */
int main() {
    try {
        // Create application with all dependencies from config.json
        auto app = cortex::app::ApplicationFactory::create("config/config.json");

        // Run application (blocking until shutdown)
        return app->run();
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown fatal error" << std::endl;
        return 1;
    }
}

