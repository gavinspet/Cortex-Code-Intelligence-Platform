/**
 * @file Logger.cpp
 * @brief Logger singleton initialization with dual-sink spdlog configuration
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

#include "logging/Logger.h"
#include "config/Config.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/pattern_formatter.h>
#include <filesystem>
#include <algorithm>
#include <cstring>

namespace cortex::logging {

// ====================================================
// Static Initialization & Singleton Access
// ====================================================

Logger& Logger::instance() noexcept {
    // Thread-safe singleton via static local variable (C++11)
    static Logger logger;
    return logger;
}

bool Logger::initialize(std::string_view applicationName, std::string_view logLevel) noexcept {
    try {
        auto& logger = Logger::instance();
        
        if (!logger.initialized_) {
            logger.configureConsoleSink();
            logger.configureFileSink();
            logger.setPattern();
            logger.setLogLevel(Logger::parseLogLevel(logLevel));
            logger.initialized_ = true;
            
            logger.info(std::string(applicationName));
            return true;
        }
        return true;
    } catch (...) {
        return false;
    }
}

// ====================================================
// Logging Methods Implementation
// ====================================================

void Logger::trace(std::string_view message) const noexcept {
    try {
        if (auto spdlog_logger = static_cast<std::shared_ptr<spdlog::logger>*>(logger_)) {
            (*spdlog_logger)->trace("{}", message);
        }
    } catch (...) {
        // Silently ignore logging errors
    }
}

void Logger::debug(std::string_view message) const noexcept {
    try {
        if (auto spdlog_logger = static_cast<std::shared_ptr<spdlog::logger>*>(logger_)) {
            (*spdlog_logger)->debug("{}", message);
        }
    } catch (...) {
        // Silently ignore logging errors
    }
}

void Logger::info(std::string_view message) const noexcept {
    try {
        if (auto spdlog_logger = static_cast<std::shared_ptr<spdlog::logger>*>(logger_)) {
            (*spdlog_logger)->info("{}", message);
        }
    } catch (...) {
        // Silently ignore logging errors
    }
}

void Logger::warn(std::string_view message) const noexcept {
    try {
        if (auto spdlog_logger = static_cast<std::shared_ptr<spdlog::logger>*>(logger_)) {
            (*spdlog_logger)->warn("{}", message);
        }
    } catch (...) {
        // Silently ignore logging errors
    }
}

void Logger::error(std::string_view message) const noexcept {
    try {
        if (auto spdlog_logger = static_cast<std::shared_ptr<spdlog::logger>*>(logger_)) {
            (*spdlog_logger)->error("{}", message);
            (*spdlog_logger)->flush();  // Flush on error
        }
    } catch (...) {
        // Silently ignore logging errors
    }
}

void Logger::critical(std::string_view message) const noexcept {
    try {
        if (auto spdlog_logger = static_cast<std::shared_ptr<spdlog::logger>*>(logger_)) {
            (*spdlog_logger)->critical("{}", message);
            (*spdlog_logger)->flush();  // Flush on critical
        }
    } catch (...) {
        // Silently ignore logging errors
    }
}

void Logger::setLogLevel(LogLevel level) noexcept {
    try {
        currentLevel_ = level;
        if (auto spdlog_logger = static_cast<std::shared_ptr<spdlog::logger>*>(logger_)) {
            switch (level) {
                case LogLevel::Trace:
                    (*spdlog_logger)->set_level(spdlog::level::trace);
                    break;
                case LogLevel::Debug:
                    (*spdlog_logger)->set_level(spdlog::level::debug);
                    break;
                case LogLevel::Info:
                    (*spdlog_logger)->set_level(spdlog::level::info);
                    break;
                case LogLevel::Warn:
                    (*spdlog_logger)->set_level(spdlog::level::warn);
                    break;
                case LogLevel::Error:
                    (*spdlog_logger)->set_level(spdlog::level::err);
                    break;
                case LogLevel::Critical:
                    (*spdlog_logger)->set_level(spdlog::level::critical);
                    break;
                case LogLevel::Off:
                    (*spdlog_logger)->set_level(spdlog::level::off);
                    break;
            }
        }
    } catch (...) {
        // Silently ignore level setting errors
    }
}

// ====================================================
// Private Constructor & Configuration
// ====================================================

Logger::Logger() noexcept
    : logger_(nullptr), currentLevel_(LogLevel::Info), initialized_(false) {
    try {
        // Initialize spdlog logger with dual sinks
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            "logs/cortex.log", 
            10 * 1024 * 1024,  // 10MB max file size
            3                   // Keep 3 rotated files
        );

        spdlog::sinks_init_list sink_list{console_sink, file_sink};
        auto spdlog_logger = std::make_shared<spdlog::logger>("Cortex", sink_list);
        
        // Set default level to info
        spdlog_logger->set_level(spdlog::level::info);
        
        // Store as opaque pointer
        void* stored = new std::shared_ptr<spdlog::logger>(spdlog_logger);
        logger_ = stored;

        // Set pattern
        setPattern();
    } catch (...) {
        // Silent failure - logger will be unavailable but won't crash
    }
}

bool Logger::configureConsoleSink() noexcept {
    try {
        if (auto spdlog_logger = static_cast<std::shared_ptr<spdlog::logger>*>(logger_)) {
            auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            (*spdlog_logger)->sinks().clear();
            (*spdlog_logger)->sinks().push_back(console_sink);
        }
        return true;
    } catch (...) {
        return false;
    }
}

bool Logger::configureFileSink() noexcept {
    try {
        // Create logs directory if it doesn't exist
        std::filesystem::create_directories("logs");

        if (auto spdlog_logger = static_cast<std::shared_ptr<spdlog::logger>*>(logger_)) {
            auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                "logs/cortex.log",
                10 * 1024 * 1024,  // 10MB max file size
                3                   // Keep 3 rotated files
            );
            (*spdlog_logger)->sinks().push_back(file_sink);
        }
        return true;
    } catch (...) {
        return false;
    }
}

bool Logger::setPattern() noexcept {
    try {
        if (auto spdlog_logger = static_cast<std::shared_ptr<spdlog::logger>*>(logger_)) {
            // Pattern: [YYYY-MM-DD HH:MM:SS.mmm] [THREAD] [LEVEL] [FILE:LINE FUNCTION] message
            const char* pattern = "[%Y-%m-%d %H:%M:%S.%e] [%t] [%^%l%$] [%s:%# %!] %v";
            (*spdlog_logger)->set_pattern(pattern);
        }
        return true;
    } catch (...) {
        return false;
    }
}

LogLevel Logger::parseLogLevel(std::string_view levelStr) noexcept {
    // Normalize to lowercase
    std::string normalized(levelStr);
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    if (normalized == "trace") {
        return LogLevel::Trace;
    }
    if (normalized == "debug") {
        return LogLevel::Debug;
    }
    if (normalized == "info") {
        return LogLevel::Info;
    }
    if (normalized == "warn" || normalized == "warning") {
        return LogLevel::Warn;
    }
    if (normalized == "error" || normalized == "err") {
        return LogLevel::Error;
    }
    if (normalized == "critical" || normalized == "crit") {
        return LogLevel::Critical;
    }
    if (normalized == "off") {
        return LogLevel::Off;
    }

    // Default to info
    return LogLevel::Info;
}

} // namespace cortex::logging
