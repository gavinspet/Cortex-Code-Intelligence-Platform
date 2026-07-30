/**
 * @file Config.cpp
 * @brief Configuration key constants and typed accessor helper implementations
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

#include "config/Config.h"
#include "config/FileConfiguration.h"
#include <algorithm>

namespace cortex::config {

// ====================================================
// Environment Helper Functions Implementation
// ====================================================

Environment parseEnvironment(std::string_view env) noexcept {
    // Normalize to lowercase for comparison
    std::string normalized(env);
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    if (normalized == "development" || normalized == "dev") {
        return Environment::Development;
    }
    if (normalized == "testing" || normalized == "test") {
        return Environment::Testing;
    }
    if (normalized == "production" || normalized == "prod") {
        return Environment::Production;
    }

    // Default to development if unknown
    return Environment::Development;
}

std::string_view environmentToString(Environment env) noexcept {
    switch (env) {
        case Environment::Development:
            return "development";
        case Environment::Testing:
            return "testing";
        case Environment::Production:
            return "production";
    }
    return "unknown";
}

// ====================================================
// Config Implementation (Meyers Singleton)
// ====================================================

Config& Config::instance() noexcept {
    // Thread-safe singleton via static local variable (C++11)
    // Instantiated on first call, destroyed at program exit
    static Config config;
    return config;
}

Config::Config() noexcept
    : appName_("Cortex"),
      appVersion_("0.1.0"),
      env_(Environment::Development),
      serverHost_("127.0.0.1"),
      serverPort_(8080),
      serverThreads_(4),
      logLevel_("info"),
      logFilePath_("logs/cortex.log"),
      databaseHost_("localhost"),
      databasePort_(5432),
      databaseName_("cortex") {

    // Attempt to load configuration from file
    // If file is missing or invalid, defaults above are used
    try {
        auto fileConfig = std::make_shared<FileConfiguration>("config/config.json");

        // Load environment
        if (auto envStr = fileConfig->getString("environment")) {
            env_ = parseEnvironment(*envStr);
        }

        // Load server configuration
        if (auto host = fileConfig->getString("server.host")) {
            serverHost_ = *host;
        }
        if (auto port = fileConfig->getUInt("server.port")) {
            serverPort_ = static_cast<uint16_t>(*port);
        }
        if (auto threads = fileConfig->getUInt("server.threads")) {
            serverThreads_ = static_cast<uint16_t>(*threads);
        }

        // Load logging configuration
        if (auto level = fileConfig->getString("logging.level")) {
            logLevel_ = *level;
        }
        if (auto filePath = fileConfig->getString("logging.file")) {
            logFilePath_ = *filePath;
        }

        // Load database configuration (currently placeholder values)
        if (auto host = fileConfig->getString("database.host")) {
            databaseHost_ = *host;
        }
        if (auto port = fileConfig->getUInt("database.port")) {
            databasePort_ = static_cast<uint16_t>(*port);
        }
        if (auto db = fileConfig->getString("database.name")) {
            databaseName_ = *db;
        }

    } catch (const std::exception&) {
        // Configuration file not found or invalid
        // Sensible defaults are already set above
        // This is acceptable as it allows running without config file
    }
}

// =================================================
// Application Identity Getters
// =================================================

std::string_view Config::applicationName() const noexcept {
    return appName_;
}

std::string_view Config::version() const noexcept {
    return appVersion_;
}

// =================================================
// Environment & Mode Getters
// =================================================

Environment Config::environment() const noexcept {
    return env_;
}

bool Config::isDevelopment() const noexcept {
    return env_ == Environment::Development;
}

bool Config::isTesting() const noexcept {
    return env_ == Environment::Testing;
}

bool Config::isProduction() const noexcept {
    return env_ == Environment::Production;
}

// =================================================
// Server Configuration Getters
// =================================================

std::string_view Config::serverHost() const noexcept {
    return serverHost_;
}

uint16_t Config::serverPort() const noexcept {
    return serverPort_;
}

uint16_t Config::serverThreads() const noexcept {
    return serverThreads_;
}

// =================================================
// Logging Configuration Getters
// =================================================

std::string_view Config::logLevel() const noexcept {
    return logLevel_;
}

std::string_view Config::logFilePath() const noexcept {
    return logFilePath_;
}

// =================================================
// Database Configuration Getters
// =================================================

std::string_view Config::databaseHost() const noexcept {
    return databaseHost_;
}

uint16_t Config::databasePort() const noexcept {
    return databasePort_;
}

std::string_view Config::databaseName() const noexcept {
    return databaseName_;
}

} // namespace cortex::config
