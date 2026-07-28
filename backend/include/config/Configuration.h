#pragma once

#include <string>
#include <optional>
#include <cstdint>
#include <memory>

namespace cortex::config {

/**
 * @class Configuration
 * @brief Abstract interface for configuration management.
 * 
 * Design Pattern: Strategy Pattern
 * - Allows different configuration sources (JSON, YAML, env vars, etc.)
 * - Implementations can be swapped without changing client code
 * 
 * SOLID Principles:
 * - Dependency Inversion: Clients depend on this interface, not concrete implementations
 * - Interface Segregation: Minimal, focused interface
 * - Open/Closed: Open for extension (new sources), closed for modification
 * 
 * Why this design:
 * - Separates configuration logic from business logic
 * - Enables testing with mock configurations
 * - Supports multiple configuration sources
 */
class Configuration {
public:
    virtual ~Configuration() = default;

    // Delete copy operations - configurations are not copyable
    Configuration(const Configuration&) = delete;
    Configuration& operator=(const Configuration&) = delete;

    /**
     * Get string configuration value
     * @param key Configuration key
     * @return std::optional containing value if key exists, std::nullopt otherwise
     */
    virtual std::optional<std::string> getString(const std::string& key) const = 0;

    /**
     * Get integer configuration value
     * @param key Configuration key
     * @return std::optional containing value if key exists, std::nullopt otherwise
     */
    virtual std::optional<int32_t> getInt(const std::string& key) const = 0;

    /**
     * Get integer configuration value (unsigned)
     * @param key Configuration key
     * @return std::optional containing value if key exists, std::nullopt otherwise
     */
    virtual std::optional<uint32_t> getUInt(const std::string& key) const = 0;

    /**
     * Get boolean configuration value
     * @param key Configuration key
     * @return std::optional containing value if key exists, std::nullopt otherwise
     */
    virtual std::optional<bool> getBool(const std::string& key) const = 0;

protected:
    Configuration() = default;
};

using ConfigurationPtr = std::shared_ptr<Configuration>;

} // namespace cortex::config
