/**
 * @file Logger.h
 * @brief Singleton logging facade with dual-sink output (colorized console + rotating file) via spdlog
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

namespace cortex::logging {

/**
 * @enum LogLevel
 * @brief Application logging severity levels
 */
enum class LogLevel {
    Trace,      // Detailed diagnostic information
    Debug,      // Debugging information
    Info,       // Informational messages
    Warn,       // Warning messages
    Error,      // Error messages
    Critical,   // Critical error messages
    Off         // Disable logging
};

/**
 * @class Logger
 * @brief Thread-safe centralized logging system (Meyers Singleton)
 * 
 * Serves as the ONLY logging interface used throughout the application.
 * 
 * Responsibility:
 * - Provide application-wide logging access
 * - Abstract spdlog implementation from the rest of the codebase
 * - Ensure thread-safe concurrent logging
 * - Configure and manage log sinks (console + rotating file)
 * - Support multiple log levels with runtime configuration
 * 
 * Why a Logging Wrapper?
 * - Decouples application from spdlog implementation
 * - Can replace spdlog with another logging library without code changes
 * - Enforces consistent logging API across codebase
 * - Enables global log level configuration
 * - Prevents direct spdlog usage (reduces cognitive load)
 * - Allows adding application-specific logging features later
 * 
 * Why Singleton?
 * - Logging is application-global by nature (needed everywhere)
 * - Single point of configuration and initialization
 * - Prevents multiple logger instances (cleaner output)
 * - C++11 static initialization ensures thread-safety
 * - Mirrors configuration access pattern (Config::instance())
 * 
 * Design Pattern: Meyers Singleton
 * - Created via static local variable (thread-safe by C++11 standard)
 * - Destroyed automatically at program exit
 * - No manual lifetime management needed
 * 
 * Thread Safety:
 * - All logging methods are const and thread-safe
 * - spdlog backend handles concurrent access with internal locks
 * - Instance creation is thread-safe (C++11 static initialization)
 * 
 * Log Configuration:
 * - Console sink: Colorized output to stdout for development
 * - File sink: Rotating log file (logs/cortex.log, 10MB max)
 * - Pattern: [YYYY-MM-DD HH:MM:SS.mmm] [THREAD] [LEVEL] [FILE:LINE] [FUNCTION] message
 * - Automatic flush on ERROR and CRITICAL levels
 * - Log directory created automatically if missing
 * 
 * Usage:
 * @code
 *   Logger::instance().info("Server started on port 8080");
 *   Logger::instance().error("Connection failed");
 *   
 *   // Or use convenience macros:
 *   LOG_INFO("Server started");
 *   LOG_ERROR("Connection failed");
 * @endcode
 * 
 * Log Level Configuration:
 * - Read from Config module during initialization
 * - Configurable via config.json (logging.level)
 * - Environment-specific defaults (verbose in dev, strict in prod)
 * 
 * SOLID Principles:
 * - Single Responsibility: Only manages logging
 * - Open/Closed: Can be extended for new sink types
 * - Liskov Substitution: Can be replaced with mock for testing
 * - Interface Segregation: Exposes only logging methods
 * - Dependency Inversion: Application depends on Logger interface
 */
class Logger {
public:
    /**
     * Get singleton instance
     * 
     * Thread-safe by default due to C++11 static initialization.
     * Instance is created on first call and persists until program exit.
     * 
     * @return Reference to the Logger singleton instance
     */
    static Logger& instance() noexcept;

    /**
     * Initialize logger with application configuration
     * 
     * Must be called during application startup before any logging occurs.
     * Configures log level, sinks, and patterns.
     * 
     * @param applicationName Application name for log header
     * @param logLevel Log level (from Config module)
     * @return true if initialization successful, false otherwise
     */
    static bool initialize(std::string_view applicationName, std::string_view logLevel) noexcept;

    // Prevent copy operations
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    // Prevent move operations
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;

    // =================================================
    // Logging Methods - All thread-safe and noexcept
    // =================================================

    /**
     * Log a trace-level message
     * 
     * Used for detailed diagnostic information.
     * Only visible when logging level is set to Trace.
     * 
     * @param message Log message (supports format strings)
     */
    void trace(std::string_view message) const noexcept;

    /**
     * Log a debug-level message
     * 
     * Used for debugging information.
     * Visible in Debug and Trace levels.
     * 
     * @param message Log message
     */
    void debug(std::string_view message) const noexcept;

    /**
     * Log an informational message
     * 
     * Used for important application events.
     * Standard logging level for production.
     * 
     * @param message Log message
     */
    void info(std::string_view message) const noexcept;

    /**
     * Log a warning message
     * 
     * Used for potentially problematic situations.
     * Indicates something unexpected but recoverable.
     * 
     * @param message Log message
     */
    void warn(std::string_view message) const noexcept;

    /**
     * Log an error message
     * 
     * Used for error conditions that may need investigation.
     * Automatically flushes to disk.
     * 
     * @param message Log message
     */
    void error(std::string_view message) const noexcept;

    /**
     * Log a critical message
     * 
     * Used for severe conditions that may require immediate action.
     * Automatically flushes to disk and terminates application.
     * 
     * @param message Log message
     */
    void critical(std::string_view message) const noexcept;

    /**
     * Set the current logging level
     * 
     * Changes which messages are output to sinks.
     * Thread-safe to call at runtime.
     * 
     * @param level New logging level
     */
    void setLogLevel(LogLevel level) noexcept;

private:
    /**
     * Private constructor (enforces singleton pattern)
     * 
     * Initializes spdlog with dual sinks (console + rotating file).
     * Creates log directory if it doesn't exist.
     * Non-throwing to ensure singleton always available.
     */
    Logger() noexcept;

    /**
     * Configure console sink with colorized output
     * 
     * @return true if configuration successful
     */
    bool configureConsoleSink() noexcept;

    /**
     * Configure rotating file sink
     * 
     * Creates logs/ directory if missing.
     * Rotates file when it exceeds 10MB.
     * Keeps up to 3 rotated backup files.
     * 
     * @return true if configuration successful
     */
    bool configureFileSink() noexcept;

    /**
     * Set logging pattern for all sinks
     * 
     * Pattern includes timestamp, thread ID, log level, filename, line number, function name.
     * 
     * @return true if pattern set successfully
     */
    bool setPattern() noexcept;

    /**
     * Parse log level string to enum
     * 
     * @param levelStr Log level string (e.g., "info", "debug", "error")
     * @return Parsed LogLevel enum
     */
    static LogLevel parseLogLevel(std::string_view levelStr) noexcept;

    // Internal state (opaque to clients)
    // spdlog objects managed here, not exposed
    mutable void* logger_;  // Opaque pointer to spdlog::logger
    LogLevel currentLevel_;
    bool initialized_;
};

} // namespace cortex::logging

// =================================================
// Convenience Macros for Logging
// =================================================

/**
 * Log a trace-level message
 * 
 * Example: LOG_TRACE("Entering function");
 */
#define LOG_TRACE(msg) cortex::logging::Logger::instance().trace(msg)

/**
 * Log a debug-level message
 * 
 * Example: LOG_DEBUG("Variable x = " + std::to_string(x));
 */
#define LOG_DEBUG(msg) cortex::logging::Logger::instance().debug(msg)

/**
 * Log an informational message
 * 
 * Example: LOG_INFO("Server started on port 8080");
 */
#define LOG_INFO(msg) cortex::logging::Logger::instance().info(msg)

/**
 * Log a warning message
 * 
 * Example: LOG_WARN("Configuration file not found, using defaults");
 */
#define LOG_WARN(msg) cortex::logging::Logger::instance().warn(msg)

/**
 * Log an error message
 * 
 * Example: LOG_ERROR("Database connection failed");
 */
#define LOG_ERROR(msg) cortex::logging::Logger::instance().error(msg)

/**
 * Log a critical message
 * 
 * Example: LOG_CRITICAL("System resource exhausted");
 */
#define LOG_CRITICAL(msg) cortex::logging::Logger::instance().critical(msg)
