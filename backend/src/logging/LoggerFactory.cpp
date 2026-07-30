/**
 * @file LoggerFactory.cpp
 * @brief Creates and configures the Logger instance from application configuration
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

#include "logging/LoggerFactory.h"
#include "logging/SpdlogLogger.h"
#include <algorithm>

namespace cortex::logging {

LogLevel LoggerFactory::parseLogLevel(const std::string& level) noexcept {
    std::string lower_level = level;
    std::transform(lower_level.begin(), lower_level.end(), lower_level.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    if (lower_level == "trace") return LogLevel::Trace;
    if (lower_level == "debug") return LogLevel::Debug;
    if (lower_level == "info") return LogLevel::Info;
    if (lower_level == "warn") return LogLevel::Warn;
    if (lower_level == "error") return LogLevel::Err;
    if (lower_level == "critical") return LogLevel::Critical;
    if (lower_level == "off") return LogLevel::Off;
    
    return LogLevel::Info;  // Default
}

Result<LoggerPtr> LoggerFactory::create(
    const std::string& name,
    ConfigurationPtr config) noexcept {
    
    try {
        // Create spdlog-based logger
        auto logger = std::make_shared<SpdlogLogger>(name);
        
        if (config) {
            // Read log level from configuration
            if (auto level_str = config->getString("logging.level")) {
                LogLevel level = parseLogLevel(*level_str);
                logger->setLevel(level);
            }
        }
        
        return Result<LoggerPtr>(logger);
    } catch (const std::exception& e) {
        return Result<LoggerPtr>(
            cortex::utils::Error::runtimeError(
                std::string("Failed to create logger: ") + e.what()));
    }
}

} // namespace cortex::logging
