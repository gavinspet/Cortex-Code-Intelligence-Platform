#pragma once

#include "Application.h"
#include <memory>
#include <string>

namespace cortex::app {

/**
 * @class ApplicationFactory
 * @brief Factory for creating Application instances with all dependencies.
 * 
 * Design Pattern: Factory Pattern
 * - Encapsulates complex object creation (Application + dependencies)
 * - Single place to manage dependency injection
 * - Easy to modify initialization logic
 * 
 * SOLID Principles:
 * - Single Responsibility: Only responsible for creating Application
 * - Dependency Inversion: Factory hides dependency construction
 * 
 * Why separate factory:
 * - Keeps Application class small and focused
 * - Dependencies are constructed in one place
 * - Easy to add more dependencies later
 * - Testable: can create with mock dependencies
 * 
 * Usage:
 * auto app = ApplicationFactory::create("config/config.json");
 * app->run();
 */
class ApplicationFactory {
public:
    /**
     * Create fully initialized Application
     * @param configFilePath Path to JSON configuration file
     * @return Unique pointer to Application
     * @throws std::runtime_error if configuration file is invalid
     */
    static std::unique_ptr<Application> create(const std::string& configFilePath);

private:
    ApplicationFactory() = default;
};

} // namespace cortex::app
