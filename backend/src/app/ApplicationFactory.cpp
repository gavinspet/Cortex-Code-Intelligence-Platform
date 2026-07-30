/**
 * @file ApplicationFactory.cpp
 * @brief Reads configuration from disk and constructs a fully-initialized Application instance
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

#include "app/ApplicationFactory.h"
#include "config/FileConfiguration.h"
#include "config/ConfigurationValidator.h"
#include "logging/Logger.h"
#include <stdexcept>
#include <iostream>

namespace cortex::app {

using cortex::config::FileConfiguration;
using cortex::config::EnhancedConfiguration;
using cortex::config::ConfigurationValidator;
using cortex::logging::Logger;

std::unique_ptr<Application> ApplicationFactory::create(const std::string& configFilePath) {
    // ========================================
    // STEP 1: Load configuration file
    // ========================================
    std::cerr << "[Setup] Loading configuration from: " << configFilePath << std::endl;
    
    std::shared_ptr<cortex::config::Configuration> base_config;
    try {
        base_config = std::make_shared<FileConfiguration>(configFilePath);
    } catch (const std::exception& e) {
        throw std::runtime_error(
            std::string("Failed to load configuration file: ") + e.what());
    }

    // ========================================
    // STEP 2: Enhance configuration with defaults and env var support
    // ========================================
    std::unordered_map<std::string, std::string> defaults = {
        {"server.host", "127.0.0.1"},
        {"server.port", "8080"},
        {"server.threads", "4"},
        {"logging.level", "info"}
    };

    auto enhanced_config_result = EnhancedConfiguration::create(base_config, defaults);
    if (!enhanced_config_result) {
        throw std::runtime_error(
            "Failed to create enhanced configuration: " + 
            enhanced_config_result.error().message());
    }

    auto enhanced_config = *enhanced_config_result;

    // ========================================
    // STEP 3: Initialize Logger singleton
    // ========================================
    std::cerr << "[Setup] Initializing logger..." << std::endl;
    
    auto log_level = enhanced_config->getString("logging.level", "LOGGING_LEVEL", "info");
    if (!Logger::initialize("Cortex Code Intelligence Platform", log_level)) {
        throw std::runtime_error("Failed to initialize logger");
    }

    // Now we can use Logger singleton
    Logger::instance().info("========================================");
    Logger::instance().info("Cortex Code Intelligence Platform");
    Logger::instance().info("Production Foundation Starting");
    Logger::instance().info("========================================");
    Logger::instance().info("Configuration loaded from: " + configFilePath);

    // Log resolved configuration
    auto host = enhanced_config->getString("server.host", "SERVER_HOST", "127.0.0.1");
    auto port = enhanced_config->getUIntOrDefault("server.port", 8080);
    auto threads = enhanced_config->getUIntOrDefault("server.threads", 4);

    LOG_INFO("Server configuration: host=" + host + ", port=" + std::to_string(port) + 
             ", threads=" + std::to_string(threads));

    // ========================================
    // STEP 4: Create and return Application
    // ========================================
    try {
        return std::make_unique<Application>(base_config);
    } catch (const std::exception& e) {
        throw std::runtime_error(
            std::string("Failed to create application: ") + e.what());
    }
}

} // namespace cortex::app
