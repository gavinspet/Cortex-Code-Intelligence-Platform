/**
 * @file Config.h
 * @brief Typed configuration key constants and accessor helpers
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

#include <string>
#include <string_view>
#include <cstdint>

namespace cortex::config {

/**
 * @enum Environment
 * @brief Application runtime environment
 * 
 * Determines behavior and logging level based on deployment context.
 */
enum class Environment {
    Development,  // Local development with verbose logging
    Testing,      // Automated testing environment
    Production    // Production deployment with strict logging
};

/**
 * Convert string to Environment enum
 * @param env Environment string (e.g., "development", "dev", "production", "prod")
 * @return Parsed Environment enum
 */
Environment parseEnvironment(std::string_view env) noexcept;

/**
 * Convert Environment enum to string representation
 * @param env Environment enum value
 * @return String representation (e.g., "development", "production")
 */
std::string_view environmentToString(Environment env) noexcept;

/**
 * @class Config
 * @brief Thread-safe centralized application configuration (Meyers Singleton)
 * 
 * Serves as the single source of truth for all application configuration.
 * Provides a stable, strongly-typed API regardless of configuration source.
 * 
 * Responsibility:
 * - Load application configuration at startup
 * - Provide application-wide access to configuration values
 * - Ensure configuration immutability after initialization
 * - Support multiple configuration sources (extensible design)
 * 
 * Design Pattern: Meyers Singleton
 * - Instances created via static local variable (C++11 thread-safe)
 * - No manual lifetime management needed (RAII)
 * - Created on first access, destroyed at program exit
 * 
 * Why Singleton for Configuration:
 * - Configuration is application-global by nature (needed everywhere)
 * - Single point of initialization and management
 * - Easy access from any component without dependency injection
 * - Can be replaced with mock configuration for testing
 * - Prevents configuration inconsistencies (one source of truth)
 * 
 * Future Extensibility:
 * - Currently loads from FileConfiguration (config.json)
 * - Can add environment variable override without API changes
 * - Can integrate secrets manager without API changes
 * - Can add hot-reload capability without breaking consumers
 * - Can add configuration validation layer
 * 
 * Thread Safety:
 * - Instance creation is thread-safe (C++11 static initialization)
 * - All getters are const and thread-safe
 * - No mutable state exposed
 * 
 * Usage:
 * @code
 *   auto& config = Config::instance();
 *   
 *   if (config.isDevelopment()) {
 *       // Development-specific code
 *   }
 *   
 *   auto port = config.serverPort();  // uint16_t
 *   auto host = config.serverHost();  // std::string_view
 * @endcode
 * 
 * SOLID Principles:
 * - Single Responsibility: Only manages configuration
 * - Open/Closed: Can be extended via derived class or wrapper
 * - Liskov Substitution: Can be replaced with test mock
 * - Interface Segregation: Only exposes necessary getters
 * - Dependency Inversion: Other components depend on configuration, not vice versa
 */
class Config {
public:
    /**
     * Get singleton instance
     * 
     * Thread-safe by default due to C++11 static initialization.
     * Instance is created on first call and persists until program exit.
     * 
     * @return Reference to the Config singleton instance
     */
    static Config& instance() noexcept;

    // Prevent copy operations
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    // Prevent move operations
    Config(Config&&) = delete;
    Config& operator=(Config&&) = delete;

    // =================================================
    // Application Identity
    // =================================================

    /**
     * Get application name
     * @return Application name (e.g., "Cortex")
     */
    std::string_view applicationName() const noexcept;

    /**
     * Get application version
     * @return Semantic version string (e.g., "0.1.0")
     */
    std::string_view version() const noexcept;

    // =================================================
    // Environment & Mode
    // =================================================

    /**
     * Get runtime environment
     * @return Current environment (Development, Testing, or Production)
     */
    Environment environment() const noexcept;

    /**
     * Check if running in development mode
     * @return true if environment is Development
     */
    bool isDevelopment() const noexcept;

    /**
     * Check if running in testing mode
     * @return true if environment is Testing
     */
    bool isTesting() const noexcept;

    /**
     * Check if running in production mode
     * @return true if environment is Production
     */
    bool isProduction() const noexcept;

    // =================================================
    // Server Configuration
    // =================================================

    /**
     * Get server listening host
     * @return IP address or hostname (e.g., "127.0.0.1", "0.0.0.0")
     */
    std::string_view serverHost() const noexcept;

    /**
     * Get server listening port
     * @return Port number in range [1, 65535]
     */
    uint16_t serverPort() const noexcept;

    /**
     * Get number of HTTP server threads
     * @return Thread count for request handling
     */
    uint16_t serverThreads() const noexcept;

    // =================================================
    // Logging Configuration
    // =================================================

    /**
     * Get logging level
     * @return Log level string (e.g., "trace", "debug", "info", "warn", "error", "critical")
     */
    std::string_view logLevel() const noexcept;

    /**
     * Get log file path
     * @return Path to rotating log file (e.g., "logs/cortex.log")
     */
    std::string_view logFilePath() const noexcept;

    // =================================================
    // Database Configuration (Placeholder for future)
    // =================================================

    /**
     * Get database server host
     * @return Database server address (e.g., "localhost", "db.example.com")
     */
    std::string_view databaseHost() const noexcept;

    /**
     * Get database server port
     * @return Database port (typically 5432 for PostgreSQL)
     */
    uint16_t databasePort() const noexcept;

    /**
     * Get database name
     * @return Database name (e.g., "cortex_prod")
     */
    std::string_view databaseName() const noexcept;

private:
    /**
     * Private constructor (enforces singleton pattern)
     * 
     * Loads configuration from FileConfiguration (config.json).
     * Uses sensible defaults if configuration file is unavailable.
     * Non-throwing to ensure singleton is always available.
     */
    Config() noexcept;

    // Configuration storage (immutable after construction)
    std::string appName_;
    std::string appVersion_;
    Environment env_;
    std::string serverHost_;
    uint16_t serverPort_;
    uint16_t serverThreads_;
    std::string logLevel_;
    std::string logFilePath_;
    std::string databaseHost_;
    uint16_t databasePort_;
    std::string databaseName_;
};

} // namespace cortex::config
