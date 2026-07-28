#pragma once

#include "Configuration.h"
#include <json/json.h>
#include <string>

namespace cortex::config {

/**
 * @class FileConfiguration
 * @brief JSON file-based configuration implementation.
 * 
 * Design Pattern: Strategy Pattern
 * - Concrete implementation of Configuration interface for JSON files
 * 
 * Why JSON:
 * - Human-readable and easy to edit
 * - Native support via jsoncpp library (already installed)
 * - Structured data support (nested objects, arrays)
 * - Wide industry adoption
 * 
 * Implementation Details:
 * - Parses JSON on construction
 * - Provides type-safe access via std::optional
 * - Immutable after construction (thread-safe reads)
 */
class FileConfiguration : public Configuration {
public:
    /**
     * Create configuration from JSON file
     * @param filePath Path to JSON configuration file
     * @throws std::runtime_error if file cannot be read or JSON is invalid
     */
    explicit FileConfiguration(const std::string& filePath);

    std::optional<std::string> getString(const std::string& key) const override;
    std::optional<int32_t> getInt(const std::string& key) const override;
    std::optional<uint32_t> getUInt(const std::string& key) const override;
    std::optional<bool> getBool(const std::string& key) const override;

private:
    Json::Value root_;

    /**
     * Helper to access nested JSON values using dot notation
     * Example: "server.port" → root["server"]["port"]
     */
    const Json::Value& getJsonValue(const std::string& key) const;
};

} // namespace cortex::config
