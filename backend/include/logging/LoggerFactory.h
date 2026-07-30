/**
 * @file LoggerFactory.h
 * @brief Factory for creating and configuring the spdlog-backed Logger instance from application config
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

#pragma once

#include "Logger.h"
#include "../config/Configuration.h"
#include "../utils/Result.h"
#include <memory>

namespace cortex::logging {

using cortex::config::Configuration;
using cortex::config::ConfigurationPtr;
using cortex::utils::Result;

/**
 * @class LoggerFactory
 * @brief Factory for creating Logger instances.
 * 
 * Design Pattern: Factory Pattern
 * SOLID: Single Responsibility - creates loggers only
 * 
 * Why:
 * - Centralizes logger creation
 * - Configuration-driven behavior
 * - Can create different logger types based on config
 * - Easy to extend (add file loggers, remote loggers, etc.)
 * - Enables dependency injection (factory returns interface)
 * 
 * Rationale:
 * - Logger creation is complex (multiple sinks, configuration)
 * - Separating creation from usage (Single Responsibility)
 * - Allows changing logger implementation without modifying Application
 * - Follows Factory Pattern for complex object creation
 */
class LoggerFactory {
public:
    /**
     * Create logger with name, optionally configured from settings
     * 
     * Configuration options (optional):
     * - logging.level: DEBUG, INFO, WARN, ERROR, CRITICAL (default: INFO)
     * - logging.console_enabled: true/false (default: true)
     * - logging.file_enabled: true/false (default: true)
     * - logging.file_path: path to log file (default: logs/cortex.log)
     * 
     * @param name Logger name (appears in output)
     * @param config Optional configuration for logger settings
     * @return Configured logger or error
     */
    static Result<LoggerPtr> create(
        const std::string& name,
        ConfigurationPtr config = nullptr) noexcept;

private:
    LoggerFactory() = default;

    /**
     * Convert configuration string to LogLevel
     */
    static LogLevel parseLogLevel(const std::string& level) noexcept;
};

} // namespace cortex::logging
