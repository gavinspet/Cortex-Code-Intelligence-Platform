#include "logging/SpdlogLogger.h"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <iostream>

namespace cortex::logging {

SpdlogLogger::SpdlogLogger(const std::string& name) {
    try {
        // Create console sink with color output
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::trace);

        // Create rotating file sink (max 10MB, keep 3 files)
        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            "logs/cortex.log", 10 * 1024 * 1024, 3);
        file_sink->set_level(spdlog::level::info);

        // Combine sinks
        spdlog::sinks_init_list sink_list = {console_sink, file_sink};

        // Create logger with multiple sinks
        logger_ = std::make_shared<spdlog::logger>(name, sink_list);
        logger_->set_level(spdlog::level::info);

        // Set pattern: [timestamp] [logger_name] [level] message
        logger_->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] %v");

        // Register globally
        spdlog::register_logger(logger_);

        // Ensure async flushing
        spdlog::flush_every(std::chrono::seconds(3));
    } catch (const spdlog::spdlog_ex& ex) {
        std::cerr << "Log initialization failed: " << ex.what() << std::endl;
        // Fallback to basic stdout logging
        logger_ = spdlog::stdout_color_mt(name);
    }
}

spdlog::level::level_enum SpdlogLogger::toSpdlogLevel(LogLevel level) {
    switch (level) {
        case LogLevel::Trace:    return spdlog::level::trace;
        case LogLevel::Debug:    return spdlog::level::debug;
        case LogLevel::Info:     return spdlog::level::info;
        case LogLevel::Warn:     return spdlog::level::warn;
        case LogLevel::Err:      return spdlog::level::err;
        case LogLevel::Critical: return spdlog::level::critical;
        case LogLevel::Off:      return spdlog::level::off;
        default:                 return spdlog::level::info;
    }
}

LogLevel SpdlogLogger::fromSpdlogLevel(spdlog::level::level_enum level) {
    switch (level) {
        case spdlog::level::trace:    return LogLevel::Trace;
        case spdlog::level::debug:    return LogLevel::Debug;
        case spdlog::level::info:     return LogLevel::Info;
        case spdlog::level::warn:     return LogLevel::Warn;
        case spdlog::level::err:      return LogLevel::Err;
        case spdlog::level::critical: return LogLevel::Critical;
        case spdlog::level::off:      return LogLevel::Off;
        default:                      return LogLevel::Info;
    }
}

void SpdlogLogger::trace(const std::string& message) noexcept {
    try { logger_->trace(message); } catch (...) {}
}

void SpdlogLogger::debug(const std::string& message) noexcept {
    try { logger_->debug(message); } catch (...) {}
}

void SpdlogLogger::info(const std::string& message) noexcept {
    try { logger_->info(message); } catch (...) {}
}

void SpdlogLogger::warn(const std::string& message) noexcept {
    try { logger_->warn(message); } catch (...) {}
}

void SpdlogLogger::error(const std::string& message) noexcept {
    try { logger_->error(message); } catch (...) {}
}

void SpdlogLogger::critical(const std::string& message) noexcept {
    try { logger_->critical(message); } catch (...) {}
}

void SpdlogLogger::setLevel(LogLevel level) noexcept {
    try { logger_->set_level(toSpdlogLevel(level)); } catch (...) {}
}

LogLevel SpdlogLogger::getLevel() const noexcept {
    try { return fromSpdlogLevel(logger_->level()); } catch (...) { return LogLevel::Info; }
}

void SpdlogLogger::flush() noexcept {
    try { logger_->flush(); } catch (...) {}
}

} // namespace cortex::logging
