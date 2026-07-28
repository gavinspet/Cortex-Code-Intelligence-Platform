#pragma once

#include <string>
#include <memory>
#include <functional>

namespace cortex::logging {

/**
 * Log level enumeration
 */
enum class LogLevel {
    Trace,
    Debug,
    Info,
    Warn,
    Err,    // 'Error' conflicts with Windows macros
    Critical,
    Off
};

/**
 * @class Logger
 * @brief Abstract interface for structured logging.
 * 
 * Design Pattern: Strategy Pattern + Facade Pattern
 * - Strategy: Different logging implementations (console, file, etc.)
 * - Facade: Simplifies logging interface for clients
 * 
 * SOLID Principles:
 * - Dependency Inversion: Clients depend on interface, not spdlog directly
 * - Interface Segregation: Minimal, focused logging interface
 * - Single Responsibility: Only responsible for logging
 * 
 * Why this design:
 * - Decouples application from spdlog
 * - Easy to swap implementations (e.g., to ELK/Splunk integration)
 * - Testable with mock loggers
 * - Consistent logging across application
 * 
 * Usage:
 * auto logger = LoggerFactory::create("Cortex");
 * logger->info("Server started on port {}", port);
 * logger->error("Connection failed: {}", error_msg);
 */
class Logger {
public:
    virtual ~Logger() = default;

    // Delete copy operations - loggers are not copyable
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    virtual void trace(const std::string& message) noexcept = 0;
    virtual void debug(const std::string& message) noexcept = 0;
    virtual void info(const std::string& message) noexcept = 0;
    virtual void warn(const std::string& message) noexcept = 0;
    virtual void error(const std::string& message) noexcept = 0;
    virtual void critical(const std::string& message) noexcept = 0;

    /**
     * Set log level filter
     */
    virtual void setLevel(LogLevel level) noexcept = 0;

    /**
     * Get current log level
     */
    virtual LogLevel getLevel() const noexcept = 0;

    /**
     * Flush pending log messages
     */
    virtual void flush() noexcept = 0;

protected:
    Logger() = default;
};

using LoggerPtr = std::shared_ptr<Logger>;

} // namespace cortex::logging
