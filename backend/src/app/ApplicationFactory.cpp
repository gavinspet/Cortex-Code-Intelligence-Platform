#include "app/ApplicationFactory.h"
#include "config/FileConfiguration.h"
#include "config/ConfigurationValidator.h"
#include "logging/LoggerFactory.h"
#include <stdexcept>
#include <iostream>

namespace cortex::app {

using cortex::config::FileConfiguration;
using cortex::config::EnhancedConfiguration;
using cortex::config::ConfigurationValidator;
using cortex::logging::LoggerFactory;

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
    // STEP 3: Create logger
    // ========================================
    std::cerr << "[Setup] Initializing logger..." << std::endl;
    
    auto logger_result = LoggerFactory::create("Cortex", base_config);
    if (!logger_result) {
        throw std::runtime_error(
            "Failed to create logger: " + logger_result.error().message());
    }

    auto logger = *logger_result;

    // Log configuration loaded
    logger->info("========================================");
    logger->info("Cortex Code Intelligence Platform");
    logger->info("Production Foundation Starting");
    logger->info("========================================");
    logger->info("Configuration loaded from: " + configFilePath);

    // Log resolved configuration
    auto host = enhanced_config->getString("server.host", "SERVER_HOST", "127.0.0.1");
    auto port = enhanced_config->getUIntOrDefault("server.port", 8080);
    auto threads = enhanced_config->getUIntOrDefault("server.threads", 4);

    logger->info("Server configuration: host=" + host + ", port=" + std::to_string(port) + 
                 ", threads=" + std::to_string(threads));

    // ========================================
    // STEP 4: Create and return Application
    // ========================================
    try {
        return std::make_unique<Application>(base_config, logger);
    } catch (const std::exception& e) {
        throw std::runtime_error(
            std::string("Failed to create application: ") + e.what());
    }
}

} // namespace cortex::app
