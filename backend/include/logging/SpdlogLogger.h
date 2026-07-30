/**
 * @file SpdlogLogger.h
 * @brief spdlog adapter implementing the Logger backend with console and rotating file sinks
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
#include <spdlog/spdlog.h>
#include <memory>

namespace cortex::logging {

/**
 * @class SpdlogLogger
 * @brief spdlog-based implementation of Logger interface.
 * 
 * Design Pattern: Adapter Pattern
 * - Adapts spdlog::logger to our Logger interface
 * - Provides consistent interface regardless of underlying logging framework
 * 
 * Why spdlog:
 * - High-performance: ~80ns per log call
 * - Async logging: Non-blocking operations
 * - Multiple sinks: Console, file, rotating files, etc.
 * - Thread-safe by default
 * - Header-only library
 * 
 * Benefits of adapter:
 * - If we need to switch to another framework, only this class changes
 * - Clients remain unaffected by underlying logging implementation
 */
class SpdlogLogger : public Logger {
public:
    /**
     * Create logger with given name
     * @param name Logger name (appears in log output)
     */
    explicit SpdlogLogger(const std::string& name);

    void trace(const std::string& message) noexcept override;
    void debug(const std::string& message) noexcept override;
    void info(const std::string& message) noexcept override;
    void warn(const std::string& message) noexcept override;
    void error(const std::string& message) noexcept override;
    void critical(const std::string& message) noexcept override;

    void setLevel(LogLevel level) noexcept override;
    LogLevel getLevel() const noexcept override;
    void flush() noexcept override;

private:
    std::shared_ptr<spdlog::logger> logger_;

    /**
     * Convert our LogLevel to spdlog::level::level_enum
     */
    static spdlog::level::level_enum toSpdlogLevel(LogLevel level);
    static LogLevel fromSpdlogLevel(spdlog::level::level_enum level);
};

} // namespace cortex::logging
